"""Autodrive sweep: enabling it moves the engine off idle across the load map (throttle for MAP,
brake for rpm), discovers reachable cells at runtime (settles or skips), and makes progress."""
import time
import ecusim
from simloop import SimThread

sim = SimThread()
sim.start()
sim.wait_ready(timeout=30)
time.sleep(0.3)


def test_autodrive_sweeps_and_progresses():
    sim.action("autodrive", True)
    seen_rpm, seen_map = set(), set()
    t0 = time.time()
    while time.time() - t0 < 45:
        t = sim.latest()
        seen_rpm.add(round(t["rpm"] / 500) * 500)
        seen_map.add(round(t["map_kpa"] / 10) * 10)
        time.sleep(0.1)
    au = sim._auto
    assert any(m >= 50 for m in seen_map)                 # reached real load, not just idle
    assert len([m for m in seen_map if m > 0]) >= 3       # swept across several load bands
    assert len(au.done) + len(au.skip) >= 2               # settling/skipping cells at runtime
    sim.stop()


def test_update_always_returns_4_tuple_even_when_exhausted():
    """Every update() path returns (throttle, brake, gear, engage) -- including the 'frontier
    exhausted' path, which once returned a 3-tuple and crashed the sim after a full sweep."""
    from autodrive import Autodrive
    a = Autodrive([800, 1100], [320])
    done = [[100, 100]]                      # everything already learned
    for _ in range(200):
        out = a.update(900, 32, done, 0.0, 0.01)
        assert len(out) == 4
    # force true exhaustion: no targetable cells left
    a.done = {(0, 0), (0, 1)}
    a.frontier = set()
    assert len(a.update(900, 32, done, 0.0, 0.01)) == 4
