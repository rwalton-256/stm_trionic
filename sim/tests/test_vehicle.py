"""Drivetrain + road load (vehicle.py), with the engine speed supplied directly so the physics is
tested on its own: gear ratios, launch, braking, reverse, engine-braking sign."""
import math
from vehicle import Drivetrain


def hold(d, secs, rpm, throttle, brake, gear, clutch=0.0, dt=5e-5):
    for _ in range(int(secs / dt)):
        d.update(rpm, dt, throttle, brake, gear, clutch)


def test_neutral_idle_stays_put():
    d = Drivetrain()
    hold(d, 2.0, 800, 0.0, 0.0, 0)
    assert abs(d.v) < 0.01


def test_locked_to_gear_ratio():
    d = Drivetrain()
    hold(d, 8.0, 3000, 1.0, 0.0, 1)
    locked = 3000 * 2 * math.pi / 60 * d.wheel_radius / d.ratio(1) * 3.6
    assert abs(d.speed_kph - locked) < 2.0 and d.engagement > 0.95


def test_brake_with_clutch_in_stops():
    d = Drivetrain()
    hold(d, 6.0, 3000, 1.0, 0.0, 1)
    hold(d, 5.0, 900, 0.0, 1.0, 1, clutch=1.0)
    assert abs(d.v) < 0.2


def test_brake_in_neutral_stops():
    d = Drivetrain(); d.v = 30.0 / 3.6
    hold(d, 4.0, 800, 0.0, 1.0, 0)
    assert abs(d.v) < 0.2


def test_engine_braking_is_negative_torque():
    d = Drivetrain(); d.v = 20.0; d.engagement = 1.0
    Tc = d.update(d.input_rpm(3) - 500, 5e-5, 0.0, 0.0, 3)
    assert Tc < 0


def test_reverse_goes_backwards():
    d = Drivetrain()
    hold(d, 3.0, 1500, 0.4, 0.0, -1)
    assert d.v < -0.5


def test_launch_needs_throttle():
    d = Drivetrain()
    hold(d, 1.0, 800, 0.0, 0.0, 1)       # idle in gear, clutch stays open
    assert abs(d.v) < 0.5 and d.engagement < 0.2
    hold(d, 4.0, 2500, 0.6, 0.0, 1)      # blip: clutch bites, car moves
    assert d.v > 1.0
