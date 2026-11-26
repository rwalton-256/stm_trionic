"""The driveable world: the B204 engine model loaded by the drivetrain, wrapped around the real
firmware (ecusim). Set controls (throttle, brake, gear, clutch, ignition); it steps the engine
and vehicle, feeds crank edges to the firmware and reads the firmware's injector and spark outputs
back, and exposes a telemetry snapshot.

No GUI or threading here: simloop.py runs this on its own thread and samples telemetry(); the GUI
only draws. Tested headless in tests/test_world_drive.py.
"""
from dataclasses import dataclass, field
import math
import ecusim
from b204 import Engine
from vehicle import Drivetrain

TDC = (0.0, 540.0, 180.0, 360.0)        # cyl 1..4 TDC (compression) in the 720 deg cycle, order 1-3-4-2
GEAR_NAMES = {-1: "R", 0: "N", 1: "1", 2: "2", 3: "3", 4: "4", 5: "5"}
KPH_PER_MPH = 1.609344


@dataclass
class Controls:
    throttle: float = 0.0     # 0..1
    brake: float = 0.0        # 0..1
    clutch: float = 0.0       # 0..1, 1 = fully disengaged (manual override of the auto-clutch)
    engage: float = None      # not None: command clutch engagement directly (dyno/autodrive mode)
    gear: int = 0             # -1 reverse, 0 neutral, 1..5
    ignition: bool = True


@dataclass
class World:
    engine_dt: float = 5e-5
    trigger_angle: float = 117.0
    iat_c: float = 25.0
    closed_loop: bool = True
    learn: bool = True
    controls: Controls = field(default_factory=Controls)

    def __post_init__(self):
        if not ecusim.started():
            ecusim.start()
        ecusim.set_tune(trigger_angle=int(self.trigger_angle), advance=12, spark_pulse_us=600,
                        rev_limit=6500, fuel_mode=1, outputs_enable=3,
                        dfco_enable=1, dfco_tps_pct=3, dfco_cut_rpm=1800, dfco_resume_rpm=1300)
        ecusim.set_closed_loop(self.closed_loop, self.learn)
        self.e = Engine()
        self.v = Drivetrain()
        self.t_us = float(ecusim.now_us())
        self._inj_on = {}
        self._spk_on = {}
        self._next_sensor_us = self.t_us
        # recent events over the last engine cycle, for the timing view
        self.inj = [[] for _ in range(4)]   # (start_deg, dur_deg, dur_us, t_us)
        self.spk = [[] for _ in range(4)]   # (fire_deg, advance_deg, t_us)
        self.sim_time = 0.0
        self._teeth = [((k * 6.0 - self.trigger_angle) % 360.0, k) for k in range(58)]
        self._warm_start()

    def _warm_start(self):
        for _ in range(int(2.0 / self.engine_dt)):
            self.e.starter_nm = 45.0 if self.e.rpm < 450 else 0.0
            self._step_once()
            if self.e.rpm > 650 and self.e.starter_nm == 0.0:
                break
        self.e.starter_nm = 0.0

    def _prune(self, t_now_us):
        cycle_us = 120e6 / max(self.e.rpm, 100.0)        # one 4-stroke cycle (2 crank revs)
        keep = t_now_us - cycle_us * 1.02
        for lst in self.inj:
            lst[:] = [x for x in lst if x[-1] >= keep]
        for lst in self.spk:
            lst[:] = [x for x in lst if x[-1] >= keep]

    def _step_once(self):
        e, v, c = self.e, self.v, self.controls
        Tc = v.update(e.rpm, self.engine_dt, c.throttle, c.brake, c.gear, c.clutch, c.engage)
        e.load_nm = Tc
        e.throttle = c.throttle

        a0 = e.angle % 360.0
        e.step(self.engine_dt)
        self.t_us += self.engine_dt * 1e6
        a1 = e.angle % 360.0
        sweep = (a1 - a0) % 360.0

        if sweep > 0:
            hits = sorted(d for d in ((ang - a0) % 360.0 for ang, _ in self._teeth) if 0 < d <= sweep)
            for d in hits:
                t = self.t_us - self.engine_dt * 1e6 * (1 - d / sweep)
                ecusim.advance_to(int(t))
                ecusim.crank_edge()
        ecusim.advance_to(int(self.t_us))

        for (t, kind, cyl, on) in ecusim.take_edges():
            if kind == "inj":
                if on:
                    self._inj_on[cyl] = t
                elif cyl in self._inj_on:
                    ton = self._inj_on.pop(cyl)
                    us = t - ton
                    e.inject(cyl, us * 1e-6)
                    start = (e.angle - e.rpm * 6.0 * (self.t_us - ton) * 1e-6) % 720.0
                    self.inj[cyl].append((start, e.rpm * 6.0 * us * 1e-6, us, self.t_us))
            else:  # spark
                if on:
                    self._spk_on[cyl] = t
                    ang = e.angle - e.rpm * 6.0 * (self.t_us - t) * 1e-6
                    adv = (TDC[cyl] - ang) % 360.0
                    if adv > 180.0:
                        adv -= 360.0
                    e.spark(cyl, adv)
                    self.spk[cyl].append((ang % 720.0, adv, self.t_us))

        if self.t_us >= self._next_sensor_us:
            ecusim.set_sensors(map_kpa=e.map_kpa, iat_c=self.iat_c, clt_c=e.clt_c,
                               battery_v=e.battery_v, afr=e.afr, throttle=c.throttle)
            self._next_sensor_us = self.t_us + 1000.0

        self._prune(self.t_us)

    def advance(self, dt_sim_s: float):
        n = max(1, int(round(dt_sim_s / self.engine_dt)))
        for _ in range(n):
            self._step_once()
        self.sim_time += n * self.engine_dt

    def shift_up(self):
        order = [-1, 0, 1, 2, 3, 4, 5]
        i = order.index(self.controls.gear)
        if i < len(order) - 1:
            self.controls.gear = order[i + 1]

    def shift_down(self):
        order = [-1, 0, 1, 2, 3, 4, 5]
        i = order.index(self.controls.gear)
        if i > 0:
            self.controls.gear = order[i - 1]

    def _clutch_state(self, slip_rpm):
        if self.controls.gear == 0 or self.v.engagement < 0.05:
            return "OPEN"
        if self.v.engagement > 0.9 and abs(slip_rpm) < 40:
            return "LOCKED"
        return "SLIPPING"

    def telemetry(self) -> dict:
        cr = ecusim.crank_status()
        fu = ecusim.fuel_status()
        ou = ecusim.output_status()
        po = ecusim.power_status()
        rpm = max(1.0, self.e.rpm)
        duty = min(100.0, fu["pw_us"] / (120e6 / rpm) * 100.0)
        w = self.e.rpm * 2 * math.pi / 60.0
        torque = max(0.0, self.e.last_torque)
        slip_rpm = self.v.slip * 60.0 / (2 * math.pi)
        last_inj = [(lst[-1][2] if lst else 0.0) for lst in self.inj]       # us
        last_adv = [(lst[-1][1] if lst else 0.0) for lst in self.spk]       # deg
        return dict(
            rpm=self.e.rpm, ecu_rpm=cr["rpm"], synced=cr["synced"],
            speed_mph=self.v.speed_kph / KPH_PER_MPH, speed_kph=self.v.speed_kph,
            gear=GEAR_NAMES[self.controls.gear], throttle=self.controls.throttle,
            brake=self.controls.brake, clutch_engage=self.v.engagement,
            clutch_state=self._clutch_state(slip_rpm), slip_rpm=slip_rpm,
            wheel_rpm=self.v.wheel_rpm, map_kpa=self.e.map_kpa, afr=self.e.afr,
            afr_target=fu["afr_target"], lam=self.e.afr / 14.7, pw_us=fu["pw_us"], duty=duty,
            trim=fu["trim"], ve=fu["ve"], closed_loop=fu["closed_loop"], learning=fu["learning"],
            cranking=fu["cranking"], dfco=fu["dfco"], iat_c=self.iat_c, clt_c=self.e.clt_c, battery_v=self.e.battery_v,
            torque_nm=torque, power_kw=torque * w / 1000.0, sparks=ou["sparks"],
            injections=ou["injections"], rev_limited=ou["rev_limited"], misfires=self.e.misfires,
            power_state=po["state"], outputs_allowed=po["outputs_allowed"], distance_m=self.v.distance,
            inj=[list(x) for x in self.inj], spk=[list(x) for x in self.spk], tdc=TDC,
            last_inj_us=last_inj, last_adv_deg=last_adv, sim_time=self.sim_time, ts_port=ecusim.ts_port(),
        )
