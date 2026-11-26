"""Closed-loop test bench: the Python B204 model driving the real firmware (all tasks) through ecusim.

Engine -> firmware: crank tooth edges (60-2, tooth 1 at `trigger_angle` BTDC cyl 1), MAP,
                    wideband AFR, temperatures, battery.
Firmware -> engine: injector pulses (fuel per cylinder) and spark timing (from the trigger edge
                    angle), exactly as the timer model produced them.
"""
from dataclasses import dataclass, field
import math
import ecusim
from b204 import Engine

TDC = (0.0, 540.0, 180.0, 360.0)        # cylinder 1..4 TDC (compression) in the 720 cycle, order 1-3-4-2


@dataclass
class Bench:
    engine: Engine = field(default_factory=Engine)
    dt: float = 50e-6                    # engine step, s
    trigger_angle: float = 117.0
    iat_c: float = 25.0
    log: list = field(default_factory=list)

    def __post_init__(self):
        if not ecusim.started():
            ecusim.start()
        ecusim.set_tune(trigger_angle=int(self.trigger_angle), advance=10, spark_pulse_us=600,
                        rev_limit=6500, fuel_mode=1, outputs_enable=3)
        self.t_us = float(ecusim.now_us())
        self._inj_on = {}
        self._next_fuel_us = self.t_us
        self.spark_log = []              # (t, cyl, advance_deg)

    # crank angle (0..360) of tooth k, k = 0..57 (0 = first tooth after the gap)
    def _tooth_angles(self):
        return [((k * 6.0 - self.trigger_angle) % 360.0, k) for k in range(58)]

    def step(self):
        e = self.engine
        a0 = e.angle % 360.0
        e.step(self.dt)
        self.t_us += self.dt * 1e6
        a1 = e.angle % 360.0
        sweep = (a1 - a0) % 360.0

        # tooth edges crossed during this step, in order, each at its interpolated time
        if sweep > 0:
            hits = []
            for ang, k in self._tooth_angles():
                d = (ang - a0) % 360.0
                if 0 < d <= sweep:
                    hits.append(d)
            for d in sorted(hits):
                t = self.t_us - self.dt * 1e6 * (1 - d / sweep)
                ecusim.advance_to(int(t))
                ecusim.crank_edge()
        ecusim.advance_to(int(self.t_us))

        # firmware outputs -> engine
        for (t, kind, cyl, on) in ecusim.take_edges():
            if kind == "inj":
                if on: self._inj_on[cyl] = t
                elif cyl in self._inj_on:
                    e.inject(cyl, (t - self._inj_on.pop(cyl)) * 1e-6)
            elif kind == "spark" and on:
                # engine angle at the edge time (rewind from now at current speed)
                ang = e.angle - e.rpm * 6.0 * (self.t_us - t) * 1e-6
                adv = (TDC[cyl] - ang) % 360.0
                if adv > 180.0: adv -= 360.0
                e.spark(cyl, adv)
                self.spark_log.append((t, cyl, adv))

        # sensors, every millisecond (the ECU's fuel task reads them every 10 ms)
        if self.t_us >= self._next_fuel_us:
            ecusim.set_sensors(map_kpa=e.map_kpa, iat_c=self.iat_c, clt_c=e.clt_c,
                               battery_v=e.battery_v, afr=e.afr)
            self._next_fuel_us = self.t_us + 1000.0

    def run(self, seconds, every=0.1, on_step=None):
        n_log = max(1, int(every / self.dt))
        for i in range(int(seconds / self.dt)):
            if on_step: on_step(self)
            self.step()
            if i % n_log == 0:
                c, f = ecusim.crank_status(), ecusim.fuel_status()
                self.log.append(dict(t=self.t_us * 1e-6, rpm=self.engine.rpm, ecu_rpm=c["rpm"], synced=c["synced"],
                                     map=self.engine.map_kpa, afr=self.engine.afr, pw=f["pw_us"], trim=f["trim"],
                                     ve=f["ve"], cl=f["closed_loop"], throttle=self.engine.throttle,
                                     misfires=self.engine.misfires))
        return self
