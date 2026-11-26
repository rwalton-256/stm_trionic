"""Fuel control in the running fuel task: open-loop maths, closed-loop trim and VE learning
against a simple plant whose true VE is 15 % above the table, cranking pulse."""
import collections
import ecusim
from wheel import Wheel

ecusim.start()
ecusim.set_tune(fuel_mode=1, outputs_enable=0)
wheel = Wheel()
DEAD_US = 800 + (650 - 800) * 0.9          # dead time at 13.8 V (default curve)


def air_mg(ve):
    return ve * 50e3 * (1.985e-3 / 4) / (287.05 * 303.15)   # 50 kPa, 30 degC, per cylinder


def sensors(afr=14.7):
    ecusim.set_sensors(map_kpa=50, iat_c=30, clt_c=85, battery_v=13.8, afr=afr)


def spin(ms, rpm=2000, each_10ms=None):
    t0 = ecusim.now_us()
    for i in range(1, ms // 10 + 1):
        wheel.run_to(t0 + i * 10_000, rpm)
        if each_10ms: each_10ms(i)


def test_open_loop_matches_hand_calc():
    sensors(); ecusim.set_closed_loop(False, False)
    spin(500)
    pw = air_mg(0.75) / 14.7 / 2 / 4.07e-3 * 1e6 + DEAD_US     # two squirts per cycle
    assert abs(ecusim.fuel_status()["pw_us"] - pw) < 3


def test_closed_loop_learns_ve():
    ecusim.set_closed_loop(True, True)
    wb = collections.deque([14.7] * 15)                       # 150 ms wideband delay
    def plant(i):
        fuel = max(0.0, ecusim.fuel_status()["pw_us"] - DEAD_US) * 1e-6 * 4.07e-3 * 2
        wb.append(air_mg(0.75 * 1.15) / fuel if fuel > 0 else 25.0)
        sensors(round(wb.popleft(), 1))
    spin(60_000, each_10ms=plant)
    f = ecusim.fuel_status()
    assert abs(wb[0] - 14.7) <= 0.15, "AFR settles on target"
    assert abs(f["trim"] - 1.0) < 0.03, "trim relaxes back to 1.0"
    assert abs(f["ve"] - 86.25) < 2.0, "learned VE approaches the true 86 %"


def test_cranking_uses_fixed_pulse():
    spin(1000, rpm=200)
    f = ecusim.fuel_status()
    assert f["cranking"] and f["pw_us"] == 6000 and not f["closed_loop"]
