"""Autodrive: automatically sweep the engine through its reachable load points to learn the VE
table. Reachability is discovered at runtime, not assumed: the driver aims the throttle/brake
loops at a target cell and, if it can't settle there (rpm and MAP both in tolerance and holding)
within a timeout, marks the cell unreachable and moves on. This naturally skips overrun-only cells
(low MAP at high rpm) and sub-idle cells without any hand-tuned envelope.

Steering (throttle sets manifold pressure, brake sets engine speed):
  throttle -> MAP   open to raise manifold pressure toward the cell's MAP
  brake    -> rpm   load to hold rpm down toward the cell's rpm; release to let it rev up
  gear chosen so the target rpm sits at a drivable road speed.

Pure Python, GUI-independent; tested headless (tests/test_autodrive.py).
"""
import math

KPH_PER_MPH = 1.609344


class Autodrive:
    DONE = 60           # cell status % that counts as learned -> move on
    NEAR_RPM = 200      # rpm tolerance for "in the cell"
    NEAR_MAP = 5.0      # kPa tolerance
    SETTLE_HOLD = 1.0   # s continuously in-cell to call it reachable
    SETTLE_TIMEOUT = 9.0   # s to reach the cell before marking it unreachable
    DWELL_TIMEOUT = 22.0   # s to hold a reachable cell before moving on regardless
    DYNO_GEAR = 5          # tall gear: clutch torque stays below the brake, so the car holds still
    MIN_RPM = 650          # below this the ECU closes the loop off, so those cells can't learn
    SLEW_PER_S = 0.65      # max throttle/clutch change per second (sluggish actuators)

    def __init__(self, rpm_bins, map_bins_x10, wheel_radius=0.30, final_drive=3.88, gears=None):
        self.rpm_bins = list(rpm_bins)
        self.map_bins = [m / 10.0 for m in map_bins_x10]
        self.wheel_r = wheel_radius
        self.final = final_drive
        self.gears = gears or {1: 3.38, 2: 1.76, 3: 1.18, 4: 0.89, 5: 0.68}
        self.target = None          # (row, col)
        self.state = "idle"         # idle | settling | dwelling
        self.frontier = set()       # cells to try, grown from reachable ones (flood fill)
        self._seeded = False
        self.skip = set()           # unreachable cells (found at runtime)
        self.done = set()           # learned cells
        self._t = 0.0               # time on current target
        self._hold = 0.0            # continuous in-cell time
        self._i_thr = 0.0
        self._i_eng = 0.0
        self._last_thr = 0.0
        self._last_eng = 0.0
        self._prev_map = 0.0
        self._prev_rpm = 0.0

    def _nearest_cell(self, rpm, map_kpa):
        cols = [i for i in range(len(self.rpm_bins)) if self.rpm_bins[i] >= self.MIN_RPM]
        c = min(cols, key=lambda i: abs(self.rpm_bins[i] - rpm))
        r = min(range(len(self.map_bins)), key=lambda i: abs(self.map_bins[i] - map_kpa))
        return (r, c)

    def _neighbors(self, cell):
        r, c = cell
        for dr, dc in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nr, nc = r + dr, c + dc
            if 0 <= nr < len(self.map_bins) and 0 <= nc < len(self.rpm_bins):
                yield (nr, nc)

    def _targetable(self, cell):
        return self.rpm_bins[cell[1]] >= self.MIN_RPM

    def _expand(self, cell):
        for n in self._neighbors(cell):
            if n not in self.done and n not in self.skip and self._targetable(n):
                self.frontier.add(n)

    def _pick_target(self, status):
        # flood out from the idle cell through reachable neighbors; pick the lowest-status cell
        # on the frontier so the sweep stays contiguous instead of marching through dead cells.
        cands = [x for x in self.frontier
                 if x not in self.done and x not in self.skip and self._targetable(x)]
        if not cands:
            return None
        return min(cands, key=lambda x: status[x[0]][x[1]])

    def coverage(self, status):
        cells = [(r, c) for r in range(len(self.map_bins)) for c in range(len(self.rpm_bins))
                 if self._targetable((r, c)) and (r, c) not in self.skip]
        if not cells:
            return 1.0
        got = sum(1 for (r, c) in cells if status[r][c] >= self.DONE)
        return got / len(cells)

    def _new_target(self, status):
        self.target = self._pick_target(status)
        self.state = "settling" if self.target else "idle"
        self._t = self._hold = 0.0
        self._i_thr = self._i_eng = 0.0
        self._last_thr = self._last_eng = 0.0
        self._prev_map = self._prev_rpm = None

    def update(self, rpm, map_kpa, status, speed_mph, dt):
        """Return (throttle, brake, gear). Advances the try-or-skip state machine."""
        if not self._seeded:
            self.frontier.add(self._nearest_cell(rpm, map_kpa))
            self._seeded = True
        if self.target is None:
            self._new_target(status)
            if self.target is None:
                return 0.0, 0.0, 0, None           # frontier exhausted: idle, hand clutch back

        r, c = self.target
        rpm_t, map_t = self.rpm_bins[c], self.map_bins[r]
        in_cell = abs(rpm - rpm_t) < self.NEAR_RPM and abs(map_kpa - map_t) < self.NEAR_MAP
        self._t += dt
        self._hold = self._hold + dt if in_cell else 0.0

        if self.state == "settling":
            if self._hold >= self.SETTLE_HOLD:
                self.state = "dwelling"
                self.frontier.discard(self.target)
                self._expand(self.target)          # reachable: open up its neighbours
            else:
                # fast-skip: actuator saturated but still out of the cell -> unreachable
                sat = (self._t > 2.5 and (
                    (self._last_thr >= 0.99 and map_kpa < map_t - self.NEAR_MAP) or   # can't reach MAP
                    (self._last_eng >= 0.99 and rpm > rpm_t + self.NEAR_RPM)))        # clutch maxed, engine overpowers
                if sat or self._t >= self.SETTLE_TIMEOUT:
                    self.skip.add(self.target)
                    self.frontier.discard(self.target)
                    self._new_target(status)
                    return self.update(rpm, map_kpa, status, speed_mph, 0.0)
        elif self.state == "dwelling":
            if status[r][c] >= self.DONE:
                self.done.add(self.target)
                self._new_target(status)
                return self.update(rpm, map_kpa, status, speed_mph, 0.0)
            if self._t >= self.DWELL_TIMEOUT:
                (self.done if status[r][c] >= self.DONE else self.skip).add(self.target)
                self._new_target(status)
                return self.update(rpm, map_kpa, status, speed_mph, 0.0)

        # ---- dyno steering: stationary, full brake; throttle -> MAP, clutch engagement -> rpm ----
        # Damped on the measured rate (no derivative kick), with the integrators frozen inside a
        # deadband so the two coupled loops stop pumping against each other once parked, plus
        # anti-windup and a per-step rate limit so neither actuator slams.
        if dt <= 0:
            return self._last_thr, 1.0, self.DYNO_GEAR, self._last_eng
        if self._prev_map is None:
            self._prev_map, self._prev_rpm = map_kpa, rpm
        d_map = (map_kpa - self._prev_map) / dt / 100.0
        d_rpm = (rpm - self._prev_rpm) / dt / 1000.0
        self._prev_map, self._prev_rpm = map_kpa, rpm

        # throttle -> MAP (open to raise MAP; ease off if MAP already climbing fast)
        e_map = (map_t - map_kpa) / 100.0
        if abs(e_map) > 0.03 and 0.0 < self._last_thr < 1.0:         # deadband + anti-windup freeze
            self._i_thr = max(-1.0, min(1.0, self._i_thr + e_map * dt * 0.4))
        thr = 1.2 * e_map - 0.15 * d_map + self._i_thr

        # engagement -> rpm (more load slows the engine; add load pre-emptively if rpm rising)
        e_rpm = (rpm - rpm_t) / 1000.0
        if abs(e_rpm) > 0.12 and 0.0 < self._last_eng < 1.0:
            self._i_eng = max(0.0, min(1.0, self._i_eng + e_rpm * dt * 0.4))
        eng = 0.5 * e_rpm + 0.20 * d_rpm + self._i_eng

        # sluggish, time-based rate limit (full travel in ~1.5 s), then clamp
        slew = self.SLEW_PER_S * dt
        thr = max(self._last_thr - slew, min(self._last_thr + slew, thr))
        eng = max(self._last_eng - slew, min(self._last_eng + slew, eng))
        thr = max(0.0, min(1.0, thr))
        eng = max(0.0, min(1.0, eng))
        self._last_thr, self._last_eng = thr, eng
        return thr, 1.0, self.DYNO_GEAR, eng
