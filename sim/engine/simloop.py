"""Run the simulation on its own thread, paced to the wall clock, so the engine keeps 1:1 real
time no matter how slow (or fast) the drawing is. The GUI thread never touches the World: it reads
latest() / scope() snapshots and pushes controls and actions, all guarded by one lock.

perf() reports how it is doing: ratio is simulated-seconds per real-second (1.0 = real time), and
headroom is how many times faster than real time it could run if unthrottled (>1 means CPU to
spare). If ratio drops below ~1 the machine can't keep up and the engine is running slow.
"""
import atexit
import threading
import time
from collections import deque

from world import World, Controls
from autodrive import Autodrive

# Stop and join every runner at interpreter exit, so no background thread is still inside ecusim
# when its C++ statics (the FreeRTOS scheduler) tear down (that race aborts the process).
_live = []


@atexit.register
def _shutdown_all():
    for sim in list(_live):
        sim.stop()

# scalar channels kept for the scope: key -> (label, colour, full-scale, unit)
SCOPE_CHANNELS = [
    ("rpm",       "RPM",      (240, 176, 64),  7000.0, ""),
    ("throttle",  "Throttle", (70, 200, 120),  1.0,    ""),
    ("map_kpa",   "MAP",      (86, 190, 220),  110.0,  "kPa"),
    ("afr",       "AFR",      (224, 120, 200), 20.0,   ""),
    ("speed_mph", "Speed",    (120, 160, 240), 140.0,  "mph"),
    ("ve",        "VE",       (176, 130, 230), 120.0,  "%"),
    ("trim",      "Trim",     (240, 210, 120), 1.3,    ""),
    ("duty",      "Inj duty", (230, 150, 90),  100.0,  "%"),
    ("clt_c",     "Coolant",  (210, 110, 90),  120.0,  "\u00b0C"),
    ("battery_v", "Battery",  (150, 200, 150), 16.0,   "V"),
]
_SCOPE_KEYS = [c[0] for c in SCOPE_CHANNELS]
SAMPLE_DT = 0.01        # scope sample period, sim-seconds (100 Hz)
MAX_WINDOW = 60.0       # longest scrollable window


class SimThread(threading.Thread):
    def __init__(self, **world_kwargs):
        super().__init__(daemon=True)
        self._kwargs = world_kwargs
        self.controls = Controls()
        self._lock = threading.Lock()
        self._latest = {}
        self._hist = deque(maxlen=int(MAX_WINDOW / SAMPLE_DT) + 10)
        self._actions = deque()
        self._running = True
        self._ready = threading.Event()
        self.error = None
        self.perf = {"ratio": 1.0, "headroom": 1.0}
        self.autodrive = False
        self._auto = None
        self.auto_state = {"on": False, "target": None, "coverage": 0.0}
        _live.append(self)

    # ---- called from the GUI thread --------------------------------------------------------
    def wait_ready(self, timeout=30.0):
        self._ready.wait(timeout)

    def latest(self):
        with self._lock:
            return self._latest

    def scope(self, window_s, keys=None, max_points=700):
        """Return (t_rel, {key: [values]}) over the last window_s, t_rel seconds-before-now <= 0."""
        with self._lock:
            samples = list(self._hist)
        if not samples:
            return [], {}
        now = samples[-1][0]
        lo = now - window_s
        samples = [s for s in samples if s[0] >= lo]
        if len(samples) > max_points:                 # stride down for drawing
            step = len(samples) // max_points + 1
            samples = samples[::step]
        keys = keys or _SCOPE_KEYS
        t_rel = [s[0] - now for s in samples]
        series = {k: [s[1][k] for s in samples] for k in keys}
        return t_rel, series

    def action(self, name, *args):
        with self._lock:
            self._actions.append((name, args))

    def stop(self):
        self._running = False
        if threading.current_thread() is not self and self.is_alive():
            self.join(timeout=2.0)
        if self in _live:
            _live.remove(self)

    # ---- the thread ------------------------------------------------------------------------
    def run(self):
        try:
            self.world = World(controls=self.controls, **self._kwargs)
            self._publish()
        except BaseException as exc:          # don't leave the GUI waiting on a dead thread
            self.error = exc
            self._running = False
            self._ready.set()
            return
        self._ready.set()

        last = time.perf_counter()
        hist_next = self.world.sim_time
        win_t0, win_sim0, win_busy = last, self.world.sim_time, 0.0

        while self._running:
            now = time.perf_counter()
            dt = min(now - last, 0.1)
            last = now

            self._drain_actions()

            a0 = time.perf_counter()
            self.world.advance(dt)
            win_busy += time.perf_counter() - a0

            if self.world.sim_time >= hist_next:
                step = self.world.sim_time - (hist_next - SAMPLE_DT)
                if self.autodrive and self._auto is not None:
                    import ecusim
                    tel = self.world.telemetry()
                    thr, brk, gear, eng = self._auto.update(tel["rpm"], tel["map_kpa"],
                                                            ecusim.ve_status(), tel["speed_mph"], step)
                    self.world.controls.throttle = thr
                    self.world.controls.brake = brk
                    self.world.controls.gear = gear
                    self.world.controls.engage = eng
                    self.auto_state = {"on": True, "target": self._auto.target,
                                       "coverage": self._auto.coverage(ecusim.ve_status())}
                else:
                    self.auto_state = {"on": False, "target": None,
                                       "coverage": self.auto_state.get("coverage", 0.0)}
                self._publish()
                hist_next = self.world.sim_time + SAMPLE_DT

            # perf over ~0.5 s windows
            if now - win_t0 >= 0.5:
                wall = now - win_t0
                sim = self.world.sim_time - win_sim0
                with self._lock:
                    self.perf = {"ratio": sim / wall if wall > 0 else 1.0,
                                 "headroom": wall / win_busy if win_busy > 0 else 99.0}
                win_t0, win_sim0, win_busy = now, self.world.sim_time, 0.0

            time.sleep(0.001)          # yield to the GUI thread; keeps us ~<=1 kHz, still real time

    def _drain_actions(self):
        with self._lock:
            acts = list(self._actions)
            self._actions.clear()
        for name, args in acts:
            if name == "shift_up":
                self.world.shift_up()
            elif name == "shift_down":
                self.world.shift_down()
            elif name == "gear":
                self.world.controls.gear = args[0]
            elif name == "reset":
                self.world.v.v = 0.0
                self.world.v.distance = 0.0
                self.world.controls.gear = 0
            elif name == "closed_loop":
                import ecusim
                ecusim.set_closed_loop(args[0], args[1])
            elif name == "starter":
                self.world.e.starter_nm = 45.0 if (args[0] and self.world.e.rpm < 450) else 0.0
            elif name == "autodrive":
                self.autodrive = bool(args[0])
                if self.autodrive and self._auto is None:
                    import ecusim
                    rpm_bins, map_bins = ecusim.ve_axes()
                    self._auto = Autodrive(rpm_bins, [int(m * 10) for m in map_bins])
                if not self.autodrive:
                    self.world.controls.engage = None      # hand clutch back to the auto-clutch
                    self.world.controls.brake = 0.0

    def _publish(self):
        tel = self.world.telemetry()
        sample = (tel["sim_time"], {k: tel[k] for k in _SCOPE_KEYS})
        with self._lock:
            self._latest = tel
            self._hist.append(sample)
