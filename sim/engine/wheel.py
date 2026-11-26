"""An ideal 60-2 crank wheel driving ecusim: tooth edges at a commanded rpm, no engine model.
Also converts output edge times back to crank angle, for checking where sparks/injections land."""
import ecusim

TEETH, SLOTS = 58, 60


class Wheel:
    def __init__(self, trigger_angle=117.0):
        self.trigger_angle = trigger_angle
        self.slot = 0                  # next slot 0..59 (58, 59 = the gap)
        self.next_us = None            # time of the next slot, None = stopped
        self.teeth = []                # (t_us, tooth index 0..57) of every edge sent
        self.rpm = 0.0

    def run_to(self, t_us, rpm):
        """Spin at `rpm` (0 = stopped) until t_us, sending each tooth edge at its time."""
        self.rpm = rpm
        if rpm <= 0:
            self.next_us = None
            ecusim.advance_to(int(t_us))
            return
        slot_us = 60e6 / (rpm * SLOTS)
        if self.next_us is None:       # starting: first edge one slot from now, after the gap
            self.next_us, self.slot = ecusim.now_us() + slot_us, 0
        while self.next_us <= t_us:
            ecusim.advance_to(int(self.next_us))
            if self.slot < TEETH:
                ecusim.crank_edge()
                self.teeth.append((int(self.next_us), self.slot))
            self.slot = (self.slot + 1) % SLOTS
            self.next_us += slot_us
        ecusim.advance_to(int(t_us))

    def angle_at(self, t_us):
        """Crank angle 0..360 (deg after cyl 1 TDC) at t_us, from the last tooth before it."""
        import bisect
        i = bisect.bisect_right(self.teeth, (t_us, 99)) - 1
        if i < 0 or self.rpm <= 0:
            return None
        t0, k = self.teeth[i]
        slot_us = 60e6 / (self.rpm * SLOTS)
        return (k * 6.0 - self.trigger_angle + (t_us - t0) / slot_us * 6.0) % 360.0


def angle_error(a, want):
    return abs((a - want + 540.0) % 360.0 - 180.0)


def pulses(edges):
    """Pair rising/falling edges: [(kind, cyl, t_on, t_off)]."""
    on, out = {}, []
    for t, kind, cyl, level in edges:
        if level:
            on[(kind, cyl)] = t
        elif (kind, cyl) in on:
            out.append((kind, cyl, on.pop((kind, cyl)), t))
    return out
