"""Simple naturally aspirated Saab B204 engine model for the stm_trionic test bench.

State: crank angle, engine speed, manifold pressure. One cylinder fires every 180 deg
(order 1-3-4-2). The model holds the *true* engine (VE, injectors, best spark timing),
which the ECU has to learn. Units: SI unless the name says otherwise.

Inputs from the ECU (per cylinder): injector pulse widths, spark advance.
Outputs to the ECU: crank angle (for teeth), MAP, wideband lambda/AFR, temps, battery.
"""
from dataclasses import dataclass, field
import math
import numpy as np

R_AIR = 287.05          # J/(kg K)
LHV   = 43.0e6          # J/kg, petrol
STOICH = 14.7
FIRING_ORDER = (0, 2, 3, 1)  # cylinders 1,3,4,2 (0-based)


@dataclass
class Params:
    displacement: float = 1.985e-3          # m^3
    cylinders: int = 4
    inertia: float = 0.18                   # kg m^2, crank + flywheel + clutch
    p_atm: float = 101.3e3                  # Pa
    t_intake: float = 298.0                 # K
    # friction mean effective pressure, bar: fmep0 + fmep1*krpm + fmep2*krpm^2 (Heywood-style)
    fmep0: float = 0.97
    fmep1: float = 0.15
    fmep2: float = 0.05
    manifold_volume: float = 2.5e-3         # m^3, plenum + runners
    idle_air_flow: float = 2.8e-3           # kg/s through the closed throttle + idle valve (choked)
    max_air_flow: float = 0.30              # kg/s through the wide-open throttle (choked)
    # true injectors (the ECU's belief may differ)
    inj_flow: float = 4.1e-3                # kg/s per injector: B204L stock ~315-350 cc/min @ 3 bar (~330 cc/min x 0.74 g/cc)
    dead_time_12v: float = 0.80e-3          # s at 12 V
    dead_time_slope: float = -0.15e-3       # s per volt above 12 V
    inj_spread: tuple = (1.00, 1.02, 0.98, 1.01)   # per-cylinder flow variation
    # combustion
    eta_peak: float = 0.36                  # indicated (gross) efficiency at best timing
    mbt_base_deg: float = 14.0              # best advance at idle; rises with rpm
    mbt_per_krpm: float = 4.0
    lean_misfire: float = 1.45              # lambda
    rich_misfire: float = 0.60
    # wideband sensor
    wb_delay: float = 0.08                  # s transport delay
    wb_tau: float = 0.04                    # s sensor response
    wb_noise: float = 0.005                 # lambda


def true_ve(rpm: float, map_kpa: float) -> float:
    """True volumetric efficiency (0..1): ~0.70 at idle speed, ~0.92 peak near 4000 rpm, worse at low MAP."""
    peak = 0.70 + 0.22 * math.exp(-((rpm - 4000.0) / 3000.0) ** 2)
    load = 0.82 + 0.18 * min(map_kpa, 100.0) / 100.0
    return max(0.2, peak * load)


@dataclass
class Engine:
    p: Params = field(default_factory=Params)
    rpm: float = 0.0
    angle: float = 0.0                       # deg, 0..720 from cyl 1 TDC (compression)
    map_pa: float = 101.3e3
    throttle: float = 0.0                    # 0..1
    load_nm: float = 0.0                     # external load (dyno / gearbox)
    battery_v: float = 13.8
    clt_c: float = 85.0
    starter_nm: float = 0.0                  # cranking torque while the starter is engaged
    seed: int = 1

    def __post_init__(self):
        self.rng = np.random.default_rng(self.seed)
        self.fuel_kg = [0.0] * self.p.cylinders          # fuel delivered for the next event
        self.spark_deg = [10.0] * self.p.cylinders
        self.next_fire = 0                               # index into FIRING_ORDER
        self.lambda_true = 1.0
        self._wb_queue = []                              # (time, lambda)
        self.wb_lambda = 1.0
        self.t = 0.0
        self.last_torque = 0.0
        self.misfires = 0
        self.events = []                                 # (t, cyl, air_kg, fuel_kg, lambda, torque_nm)

    # ---- inputs from the ECU ------------------------------------------------------------
    def inject(self, cyl: int, pulse_s: float):
        """One injector pulse on cylinder `cyl` (0-based), electrical pulse width in seconds."""
        dead = self.p.dead_time_12v + self.p.dead_time_slope * (self.battery_v - 12.0)
        open_s = max(0.0, pulse_s - dead)
        self.fuel_kg[cyl] += open_s * self.p.inj_flow * self.p.inj_spread[cyl]

    def spark(self, cyl: int, advance_deg: float):
        self.spark_deg[cyl] = advance_deg

    # ---- outputs for the ECU ------------------------------------------------------------
    @property
    def map_kpa(self) -> float:
        return self.map_pa / 1000.0

    @property
    def afr(self) -> float:
        return self.wb_lambda * STOICH

    # ---- physics ------------------------------------------------------------------------
    def _air_per_cyl(self) -> float:
        vd = self.p.displacement / self.p.cylinders
        return true_ve(self.rpm, self.map_kpa) * self.map_pa * vd / (R_AIR * self.p.t_intake)

    def _combust(self, cyl: int):
        air = self._air_per_cyl()
        fuel = self.fuel_kg[cyl]
        self.fuel_kg[cyl] = 0.0
        no_fuel = fuel <= 1e-9              # injectors cut (overrun): no combustion, not a misfire
        lam = (air / fuel / STOICH) if fuel > 0 else 99.0
        torque = 0.0
        if self.p.rich_misfire < lam < self.p.lean_misfire and self.rpm > 50:
            burned = min(fuel, air / STOICH)
            mbt = self.p.mbt_base_deg + self.p.mbt_per_krpm * self.rpm / 1000.0
            spark_eff = max(0.0, 1.0 - 0.0006 * (self.spark_deg[cyl] - mbt) ** 2)   # ~ -6% at 10 deg off
            mix_eff = 1.0 - 0.8 * max(0.0, lam - 1.05) - 0.4 * max(0.0, 0.85 - lam)
            # light-load heat losses: ~78% of peak efficiency at idle MAP, full at WOT
            load_eff = 0.70 + 0.30 * min(self.map_kpa, 100.0) / 100.0
            work = burned * LHV * self.p.eta_peak * load_eff * spark_eff * max(0.0, mix_eff)
            torque = work / math.pi                       # averaged over the 180 deg to the next event
        elif not no_fuel:
            self.misfires += 1
        self.lambda_true = min(lam, 3.0)
        self._wb_queue.append((self.t + self.p.wb_delay, self.lambda_true))
        self.last_torque = torque
        self.events.append((self.t, cyl, air, fuel, lam, torque))

    def step(self, dt: float):
        p = self.p
        # manifold: air in through the throttle, out into the cylinders (filling/emptying)
        pr = min(self.map_pa / p.p_atm, 1.0)
        psi = 1.0 if pr <= 0.528 else math.sqrt(max(0.0, 1.0 - ((pr - 0.528) / 0.472) ** 2))
        m_in = (p.idle_air_flow + (p.max_air_flow - p.idle_air_flow) * self.throttle ** 2) * psi
        vd = p.displacement
        m_out = true_ve(self.rpm, self.map_kpa) * self.map_pa * vd * self.rpm / 120.0 / (R_AIR * p.t_intake)
        self.map_pa += (m_in - m_out) * R_AIR * p.t_intake / p.manifold_volume * dt
        self.map_pa = min(max(self.map_pa, 5e3), p.p_atm)

        # crank: combustion torque from the last event, friction, pumping (in friction), load
        w = self.rpm * 2 * math.pi / 60.0
        # friction + pumping losses as mean effective pressures -> torque = mep * Vd / (4 pi)
        krpm = self.rpm / 1000.0
        fmep = (p.fmep0 + p.fmep1 * krpm + p.fmep2 * krpm ** 2) * 1e5
        pmep = max(0.0, p.p_atm - self.map_pa)
        friction = (fmep + pmep) * p.displacement / (4 * math.pi) if self.rpm > 1 else 0.0
        net = self.last_torque + self.starter_nm - friction - self.load_nm
        w = max(0.0, w + net / p.inertia * dt)
        self.rpm = w * 60.0 / (2 * math.pi)

        # crank angle; fire a cylinder every 180 deg
        old = self.angle
        self.angle = (self.angle + self.rpm * 6.0 * dt) % 720.0
        crossed = (self.angle // 180.0) != (old // 180.0) or (self.rpm * 6.0 * dt >= 180.0)
        if crossed and self.rpm > 0:
            self._combust(FIRING_ORDER[self.next_fire])
            self.next_fire = (self.next_fire + 1) % p.cylinders

        # wideband: delayed, filtered, noisy
        while self._wb_queue and self._wb_queue[0][0] <= self.t:
            target = self._wb_queue.pop(0)[1]
            self._wb_target = target
        tgt = getattr(self, "_wb_target", self.wb_lambda)
        self.wb_lambda += (tgt - self.wb_lambda) * min(1.0, dt / p.wb_tau)
        self.wb_lambda += self.rng.normal(0.0, p.wb_noise) * math.sqrt(min(1.0, dt / 0.01))
        self.t += dt
