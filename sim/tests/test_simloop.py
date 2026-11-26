"""The threaded runner: the engine keeps (near) real time on its own thread while the main thread
reads snapshots, and the scope history and controls work."""
import time
from simloop import SimThread


def test_runs_realtime_and_drives():
    sim = SimThread()
    sim.start()
    sim.wait_ready(timeout=30)
    assert not getattr(sim, "error", None)
    time.sleep(1.0)
    t = sim.latest()
    assert t and 600 < t["rpm"] < 1100 and t["clutch_state"] == "OPEN"

    sim.action("gear", 1)
    sim.controls.throttle = 0.7
    time.sleep(3.0)
    t = sim.latest()
    assert t["speed_mph"] > 4 and t["clutch_state"] in ("SLIPPING", "LOCKED")
    assert sim.perf["ratio"] > 0.4            # lenient: CI may run several in parallel
    assert sim.perf["headroom"] > 0.0

    tr, series = sim.scope(2.0, keys=["rpm", "throttle"])
    assert len(tr) > 5 and max(series["rpm"]) > min(series["rpm"])

    sim.controls.throttle = 0.0
    sim.controls.brake = 1.0
    time.sleep(2.5)
    assert sim.latest()["speed_mph"] < t["speed_mph"]
    sim.stop()
    time.sleep(0.1)
