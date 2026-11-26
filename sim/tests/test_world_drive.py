"""The whole driveable stack: firmware + engine + drivetrain through world.World. Boots one ECU
and drives it, checking telemetry stays sane (sync, mph, clutch state, per-cylinder timing)."""
from world import World

world = World()


def drive(secs, thr, brk, gear, step=0.02):
    world.controls.throttle, world.controls.brake, world.controls.gear = thr, brk, gear
    for _ in range(int(secs / step)):
        world.advance(step)


def test_arrives_idling_and_synced():
    t = world.telemetry()
    assert t["synced"] and 600 < t["rpm"] < 1100 and t["power_state"] == "ign_on"
    assert t["clutch_state"] == "OPEN" and abs(t["speed_mph"]) < 0.1


def test_launches_and_pulls_in_first():
    drive(1.0, 0.0, 0.0, 0)
    world.controls.gear = 1
    drive(6.0, 0.7, 0.0, 1)
    t = world.telemetry()
    assert t["speed_mph"] > 6 and t["clutch_state"] in ("SLIPPING", "LOCKED")


def test_upshift_accelerates():
    before = world.telemetry()["speed_mph"]
    world.shift_up()
    drive(5.0, 0.8, 0.0, 2)
    t = world.telemetry()
    assert t["speed_mph"] > before and t["power_kw"] > 20


def test_brake_slows_it():
    before = world.telemetry()["speed_mph"]
    drive(4.0, 0.0, 1.0, 2)
    assert world.telemetry()["speed_mph"] < before - 6


def test_both_sparks_and_injection_present():
    t = world.telemetry()
    assert any(len(lst) >= 1 for lst in t["inj"])
    assert any(len(lst) >= 2 for lst in t["spk"])        # wasted spark: two firings per cycle
    assert all(abs(a) < 60 for a in t["last_adv_deg"])


def test_mph_matches_kph():
    t = world.telemetry()
    assert abs(t["speed_mph"] * 1.609344 - t["speed_kph"]) < 0.1
