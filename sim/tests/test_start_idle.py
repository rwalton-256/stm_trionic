"""Crank, start, idle with closed loop + VE learning, throttle blip: the B204 model driving the
full firmware."""
import ecusim
from bench import Bench

b = Bench()
ecusim.advance_to(ecusim.now_us() + 100_000)       # +15 debounce before cranking
b.t_us = float(ecusim.now_us())
ecusim.set_closed_loop(True, True)
t0 = b.t_us * 1e-6


def driver(b):
    t, e = b.t_us * 1e-6 - t0, b.engine
    e.starter_nm = 45.0 if (t < 2.0 and e.rpm < 450) else 0.0      # starter until it catches
    e.throttle = 0.12 if 6.0 <= t < 7.0 else 0.0                   # blip at 6 s


b.run(10.0, every=0.5, on_step=driver)


def test_print_log():
    print(f"\n{'t':>4} {'rpm':>6} {'ecu':>5} {'sync':>4} {'MAP':>5} {'AFR':>5} {'pw us':>6} {'trim':>5} {'VE':>5} CL")
    for r in b.log:
        print(f"{r['t'] - t0:4.1f} {r['rpm']:6.0f} {r['ecu_rpm']:5d} {int(r['synced']):4d} {r['map']:5.1f} "
              f"{r['afr']:5.1f} {r['pw']:6d} {r['trim']:5.2f} {r['ve']:5.1f} {int(r['cl'])}")


def test_starts_and_idles():
    last = b.log[-1]
    assert last["synced"] and 500 < last["rpm"] < 1500


def test_idle_afr_on_target():
    assert abs(b.log[-1]["afr"] - 14.7) < 0.7


def test_firmware_running_tasks():
    assert ecusim.power_status()["state"] == "ign_on" and ecusim.fuel_status()["closed_loop"]
