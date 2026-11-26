"""Key on/off through the real power task: +15 debounce, relays, outputs gate, cranking
ride-through, sleep. One ECU, scenarios in sequence on one timeline (times in ms)."""
import ecusim
from wheel import Wheel

ecusim.start(ignition=False)
ecusim.set_tune(fuel_mode=0, outputs_enable=3, rev_limit=7000)
wheel = Wheel()


def cut():
    return not ecusim.power_status()["outputs_allowed"] and not ecusim.relays()["main"]


def run(t_from, t_to, key, rpm):
    """Step 1 ms at a time; key(t) and rpm(t) give +15 and engine speed. Returns ms of first cut."""
    first_cut = None
    for t in range(t_from, t_to):
        ecusim.set_ignition(key(t))
        wheel.run_to(t * 1000, rpm(t))
        if first_cut is None and cut():
            first_cut = t
    return first_cut


def test_key_on_primes_pump():
    run(0, 1000, key=lambda t: t >= 100, rpm=lambda t: 0)
    assert ecusim.relays() == {"main": True, "fuel_pump": True}
    run(1000, 3000, key=lambda t: True, rpm=lambda t: 0)
    assert ecusim.relays() == {"main": True, "fuel_pump": False}, "2 s prime then off"


def test_noise_spike_ignored():
    # running at 900 rpm, 50 ms dropout on +15
    assert run(3000, 8000, key=lambda t: not 6000 <= t < 6050, rpm=lambda t: 900) is None


def test_dip_while_cranking_rides_through():
    # stop, then crank at 200 rpm; +15 drops for 0.5 s (battery sag)
    assert run(8000, 14000, key=lambda t: not 12000 <= t < 12500,
               rpm=lambda t: 0 if t < 10000 else 200) is None


def test_cranking_exemption_ends_after_3s():
    c = run(14000, 21000, key=lambda t: t < 15000, rpm=lambda t: 200 if t < 20000 else 0)
    assert c is not None and 2900 <= c - 15000 <= 3100


def test_running_engine_cuts_on_key_off_then_sleeps():
    run(21000, 35000, key=lambda t: True, rpm=lambda t: 0 if t < 25000 else 900)   # past 30 s boot grace
    assert ecusim.power_status()["outputs_allowed"] and ecusim.crank_status()["synced"]
    c = run(35000, 37000, key=lambda t: False, rpm=lambda t: 900 if t < 35000 else (600 if t < 36500 else 0))
    assert c is not None and c - 35000 <= 150, "a running engine must stop within ~0.1 s of key-off"
    late = [e for e in ecusim.take_edges() if e[0] > (c + 2) * 1000 and e[3]]
    assert not late, "no injector/spark pulses after the cut"
    run(37000, 42000, key=lambda t: False, rpm=lambda t: 0)
    p = ecusim.power_status()
    assert p["stopped"] and p["reset"], "STOP mode after the 5 s run-on"
