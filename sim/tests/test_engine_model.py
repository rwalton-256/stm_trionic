"""Engine model sanity checks with a crude Python stand-in for the ECU (no firmware involved)."""
import math, sys
from b204 import Engine, Params, STOICH, R_AIR, true_ve

def ecu_pulse(e: Engine, ve_guess: float, target_lambda=1.0) -> float:
    """Speed-density pulse width the ECU would command, with a correct dead-time model."""
    vd = e.p.displacement / e.p.cylinders
    air = ve_guess * e.map_pa * vd / (R_AIR * e.p.t_intake)
    fuel = air / (STOICH * target_lambda)
    dead = e.p.dead_time_12v + e.p.dead_time_slope * (e.battery_v - 12.0)
    return fuel / e.p.inj_flow + dead

def run(seconds, throttle, ve_error=1.0, dt=1e-4, start_rpm=900):
    e = Engine(rpm=start_rpm); e.throttle = throttle
    last = -1
    for _ in range(int(seconds / dt)):
        fire_idx = e.next_fire
        if fire_idx != last:                       # inject once per event for the next cylinder
            cyl = (0, 2, 3, 1)[fire_idx]
            e.inject(cyl, ecu_pulse(e, true_ve(e.rpm, e.map_kpa) * ve_error))
            e.spark(cyl, e.p.mbt_base_deg + e.p.mbt_per_krpm * e.rpm / 1000.0)
            last = fire_idx
        e.step(dt)
    return e


def test_idle_closed_throttle():
    e = run(3.0, throttle=0.0)
    assert 500 < e.rpm < 1500 and 25 < e.map_kpa < 60 and abs(e.wb_lambda - 1) < 0.05


def test_part_throttle_revs_up():
    assert run(3.0, throttle=0.15).rpm > 1500


def test_lean_when_ecu_ve_too_low():
    assert 1.10 < run(3.0, throttle=0.0, ve_error=0.85).wb_lambda < 1.25    # ~1.18 expected


def test_stops_without_fuel_or_starter():
    e = Engine()
    for _ in range(int(1.0 / 1e-4)):
        e.step(1e-4)
    assert e.rpm == 0
