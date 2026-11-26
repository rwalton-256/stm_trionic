"""Crank decoder + output scheduler: where every spark and injection lands, in crank degrees,
on a perfect 60-2 wheel. Fixed pulse width (fuel_mode 0) so the pulse length is known."""
import pytest
import ecusim
from wheel import Wheel, angle_error, pulses

ecusim.start()
ecusim.set_tune(trigger_angle=117, spark_pulse_us=600, rev_limit=7500, fuel_mode=0, outputs_enable=3)
wheel = Wheel(117)
ecusim.advance_to(100_000)                      # +15 debounce, tasks up


@pytest.mark.parametrize("rpm,advance,inj_angle,pw_us", [
    (300, 10, 0, 3000), (900, 15, 60, 3000), (3000, 25, 120, 4000), (6000, 30, 200, 4000), (6000, 30, 200, 2000)])
def test_spark_and_injection_angles(rpm, advance, inj_angle, pw_us):
    ecusim.set_tune(advance=advance, inj_angle=inj_angle, inj_pw_us=pw_us)
    t = ecusim.now_us()
    wheel.run_to(t + 500_000, 0)                # stopped for 0.5 s: decoder loses sync
    start = ecusim.now_us()
    wheel.run_to(start + 12 * 60e6 / rpm, rpm)  # 12 revolutions
    assert ecusim.crank_status()["synced"]

    sparks = injs = 0
    for kind, cyl, t_on, t_off in pulses(e for e in ecusim.take_edges() if e[0] >= start):
        a = wheel.angle_at(t_on)
        tdc = 0 if cyl in (0, 3) else 180       # wasted spark / batch pairs 1+4, 2+3
        if kind == "spark":
            sparks += 1
            assert angle_error(a, tdc - advance) < 0.5, f"spark at {a:.2f} deg"
            assert 590 <= t_off - t_on <= 610, "DI trigger pulse length"
        else:
            injs += 1
            assert angle_error(a, tdc - inj_angle) < 0.5, f"injection at {a:.2f} deg"
            assert abs((t_off - t_on) - pw_us) < 2, "injector pulse width"
    assert sparks > 10 and injs > 10
