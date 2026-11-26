"""Longitudinal vehicle model sitting behind the B204: clutch, gearbox, final drive, and road
load (aero drag, rolling resistance, braking, optional grade) on a flat road.

The engine model owns crank inertia and makes torque; this model turns that into road speed and,
crucially, reflects road load back to the crank as `Engine.load_nm`, so the engine actually feels
the car. One call per engine step:

    Tc = vehicle.update(engine_rpm, dt, controls)   # also integrates road speed
    engine.load_nm = Tc                              # clutch torque loads the crank

Clutch is an auto-clutch (a stand-in for a driver's left foot): open at idle, slipped in on
throttle to launch, locked once rolling, dipped near stall. A manual clutch command can override
it. Units are SI unless the name says otherwise. Gear ratios are engine-to-wheel, reverse signed.
"""
from dataclasses import dataclass, field
import math

G = 9.81
RHO_AIR = 1.2


@dataclass
class Drivetrain:
    # vehicle (roughly a Saab 900/9000: ~1400 kg, 195/60R15 ~ 0.30 m rolling radius)
    mass: float = 1400.0
    rot_factor: float = 1.08            # rotating-inertia allowance (wheels, shafts) on top of mass
    wheel_radius: float = 0.30
    cd: float = 0.34
    frontal_area: float = 2.1
    crr: float = 0.013                  # rolling resistance coefficient
    brake_force_max: float = 12000.0    # N at full brake (~0.87 g)
    grade: float = 0.0                  # road slope, rad (0 = flat)
    # driveline
    final_drive: float = 3.88
    gears: dict = field(default_factory=lambda: {
        -1: -3.17, 0: 0.0, 1: 3.38, 2: 1.76, 3: 1.18, 4: 0.89, 5: 0.68})
    efficiency: float = 0.90
    # clutch
    clutch_capacity: float = 420.0      # N m it can transmit, fully engaged
    slip_eps: float = 2.0               # rad/s: slip over which friction saturates (near-lock stiffness)
    stall_rpm: float = 550.0            # below this, in gear, the auto-clutch slips/opens
    engage_time: float = 0.25           # s to take up the clutch
    release_time: float = 0.08          # s to drop it

    def __post_init__(self):
        self.v = 0.0                    # road speed, m/s (signed; negative = reverse travel)
        self.engagement = 0.0           # 0 open .. 1 locked
        self.slip = 0.0                 # engine - input speed, rad/s
        self.clutch_torque = 0.0        # N m taken from the crank this step
        self.distance = 0.0             # m travelled

    def ratio(self, gear: int) -> float:
        return self.gears.get(gear, 0.0) * self.final_drive

    @property
    def speed_kph(self) -> float:
        return self.v * 3.6

    @property
    def wheel_rpm(self) -> float:
        return self.v / self.wheel_radius * 60.0 / (2 * math.pi)

    def input_rpm(self, gear: int) -> float:
        """Engine rpm the clutch would be locked to at the current road speed, in this gear."""
        return (self.v / self.wheel_radius) * self.ratio(gear) * 60.0 / (2 * math.pi)

    # ---- clutch engagement: the "driver's left foot" --------------------------------------
    def _target_engagement(self, engine_rpm: float, gear: int, throttle: float) -> float:
        if gear == 0:
            return 0.0
        if engine_rpm < self.stall_rpm:          # about to stall: slip/open to keep it alive
            return 0.0
        if abs(self.v) > 1.0:                     # rolling: locked
            return 1.0
        return max(0.0, min(1.0, (throttle - 0.05) / 0.5))   # launching: slip in with throttle

    def _update_engagement(self, engine_rpm, gear, throttle, clutch_cmd, dt):
        want = self._target_engagement(engine_rpm, gear, throttle)
        rate = dt / self.engage_time if want > self.engagement else dt / self.release_time
        self.engagement += max(-rate, min(rate, want - self.engagement))
        self.engagement = max(0.0, min(1.0, self.engagement))
        if clutch_cmd > 0.0:                      # manual clutch pedal: 1 = fully disengaged
            self.engagement = min(self.engagement, 1.0 - clutch_cmd)

    # ---- one step -------------------------------------------------------------------------
    def update(self, engine_rpm: float, dt: float, throttle: float, brake: float,
               gear: int, clutch_cmd: float = 0.0, engage_override=None) -> float:
        N = self.ratio(gear)
        w_e = engine_rpm * 2 * math.pi / 60.0
        w_in = (self.v / self.wheel_radius) * N
        self.slip = w_e - w_in
        if engage_override is not None:
            # dyno mode: autodrive commands clutch engagement directly as a load knob
            self.engagement = max(0.0, min(1.0, engage_override))
        else:
            self._update_engagement(engine_rpm, gear, throttle, clutch_cmd, dt)

        Tc = 0.0
        if N != 0.0:
            Tc = self.engagement * self.clutch_capacity * math.tanh(self.slip / self.slip_eps)
        self.clutch_torque = Tc

        f_drive = Tc * N * self.efficiency / self.wheel_radius     # tractive force at the road
        self._integrate_speed(f_drive, brake, dt)
        return Tc

    def _integrate_speed(self, f_drive, brake, dt):
        v = self.v
        m_eff = self.mass * self.rot_factor

        f_aero = 0.5 * RHO_AIR * self.cd * self.frontal_area * v * abs(v)
        f_grade = self.mass * G * math.sin(self.grade)
        f_net = f_drive - f_aero - f_grade                      # forces that have a fixed direction

        # Rolling + brake can only resist motion (or hold a stopped car). Magnitudes:
        f_hold = self.crr * self.mass * G + brake * self.brake_force_max

        V_EPS = 0.02
        if abs(v) < V_EPS and abs(f_net) <= f_hold:
            self.v = 0.0                                        # held still by friction/brake
        else:
            direction = (1.0 if v > 0 else -1.0) if abs(v) >= V_EPS else (1.0 if f_net > 0 else -1.0)
            a = (f_net - f_hold * direction) / m_eff
            v_new = v + a * dt
            if brake > 0 and v != 0 and (v_new * v) < 0:        # brake must not reverse the car
                v_new = 0.0
            self.v = v_new
        self.distance += self.v * dt
