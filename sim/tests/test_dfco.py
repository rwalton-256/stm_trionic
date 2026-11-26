"""Overrun fuel cut (DFCO): injectors close when coasting with the throttle shut, the closed-loop
trim freezes while cut, fuel returns when the throttle opens, and it never cuts while driving."""
from world import World

world = World()


def drive(secs, thr, brk, gear, step=0.02):
    world.controls.throttle, world.controls.brake, world.controls.gear = thr, brk, gear
    for _ in range(int(secs / step)):
        world.advance(step)


def get_up_to_speed():
    world.controls.gear = 1
    drive(4, 0.8, 0, 1)
    world.shift_up()
    drive(4, 0.8, 0, 2)


def test_cut_on_overrun():
    get_up_to_speed()
    assert not world.telemetry()["dfco"] and world.telemetry()["rpm"] > 1800
    world.controls.throttle = 0.0
    for _ in range(100):
        world.advance(0.02)
        if world.telemetry()["dfco"]:
            break
    t = world.telemetry()
    assert t["dfco"] and t["pw_us"] == 0


def test_trim_frozen_and_no_misfires_during_cut():
    trim0 = world.telemetry()["trim"]
    misf0 = world.telemetry()["misfires"]
    drive(1.0, 0.0, 0.0, 2)                 # stay in overrun
    t = world.telemetry()
    assert t["dfco"] and abs(t["trim"] - trim0) < 1e-6      # frozen
    assert t["misfires"] - misf0 == 0                       # cut is not a misfire


def test_injectors_actually_idle_during_cut():
    drive(0.5, 0.0, 0.0, 2)                 # let the retained events prune out
    assert all(len(lst) == 0 for lst in world.telemetry()["inj"])


def test_fuel_returns_when_throttle_opens():
    drive(1.0, 0.4, 0.0, 2)
    t = world.telemetry()
    assert not t["dfco"] and t["pw_us"] > 100


def test_no_cut_while_driving():
    drive(2.0, 0.6, 0.0, 2)
    assert not world.telemetry()["dfco"]
