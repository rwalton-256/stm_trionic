"""Drive the simulated B204 in a window.

    make sim-drive

The simulation (real firmware + engine + drivetrain) runs on its own thread, paced to the wall
clock, so the engine keeps real time no matter the frame rate (watch the "sim speed" readout). This
thread only turns the keyboard into controls and draws the dashboard. The car
arrives idling. TunerStudio can connect at the same time (status panel shows the port).

Controls:  W throttle          S ease throttle off / faster lift        SPACE brake
           A/D or Left/Right shift   1-5 gear, 0 neutral   C clutch (hold)   K starter
           H  throttle hold vs spring (default spring: returns to 0 on release)
           -/=  throttle sensitivity      L closed loop      [ ] scope window
           R reset    mouse: toggle scope probes    ESC quit

Needs pygame:  make sim-drive  installs it into the sim venv, or  pip install pygame
"""
import sys

try:
    import pygame
except ImportError:
    sys.exit("pygame is not installed.  Run  make sim-drive , or  pip install pygame")

import ecusim
from simloop import SimThread
from dashboard import Dashboard

W, H = 1180, 720


def main():
    sim = SimThread()
    sim.start()
    sim.wait_ready()
    if getattr(sim, "error", None):
        sys.exit(f"simulation failed to start: {sim.error!r}")

    pygame.init()
    pygame.display.set_caption("stm_trionic \u2014 B204 driving simulator")
    screen = pygame.display.set_mode((W, H))
    clock = pygame.time.Clock()
    dash = Dashboard(W, H)

    throttle = 0.0
    hold = False
    autodrive = False                 # False = spring (throttle returns to 0 on release)
    ramp_s = 0.6                 # seconds from closed to full on W (sensitivity)
    cl, learn = True, True
    running = True
    while running:
        dt = clock.tick(60) / 1000.0

        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                running = False
            elif ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
                dash.handle_click(ev.pos)
            elif ev.type == pygame.KEYDOWN:
                k = ev.key
                if k == pygame.K_ESCAPE:
                    running = False
                elif k in (pygame.K_d, pygame.K_RIGHT):
                    sim.action("shift_up")
                elif k in (pygame.K_a, pygame.K_LEFT):
                    sim.action("shift_down")
                elif pygame.K_0 <= k <= pygame.K_5:
                    sim.action("gear", k - pygame.K_0)
                elif k == pygame.K_l:
                    cl = not cl
                    sim.action("closed_loop", cl, learn)
                elif k == pygame.K_r:
                    sim.action("reset")
                elif k == pygame.K_g:
                    autodrive = not autodrive
                    sim.action("autodrive", autodrive)
                elif k == pygame.K_h:
                    hold = not hold
                elif k in (pygame.K_MINUS, pygame.K_KP_MINUS):
                    ramp_s = min(2.0, ramp_s + 0.1)       # slower
                elif k in (pygame.K_EQUALS, pygame.K_KP_PLUS):
                    ramp_s = max(0.15, ramp_s - 0.1)      # faster
                elif k == pygame.K_LEFTBRACKET:
                    dash.cycle_window(-1)
                elif k == pygame.K_RIGHTBRACKET:
                    dash.cycle_window(1)

        keys = pygame.key.get_pressed()
        w_held = keys[pygame.K_w] or keys[pygame.K_UP]
        s_held = keys[pygame.K_s] or keys[pygame.K_DOWN]
        if w_held:
            throttle += dt / ramp_s
        elif not hold:
            throttle -= dt / (ramp_s * 0.9)               # spring back to zero
        if s_held:
            throttle -= dt / 0.2                          # forced faster lift, both modes
        throttle = max(0.0, min(1.0, throttle))
        if not autodrive:
            sim.controls.throttle = throttle
            sim.controls.brake = 1.0 if keys[pygame.K_SPACE] else 0.0
            sim.controls.clutch = 1.0 if keys[pygame.K_c] else 0.0
        sim.action("starter", bool(keys[pygame.K_k]))

        tel = sim.latest()
        if tel:
            mode = "AUTO" if autodrive else ("HOLD" if hold else "spring")
            extra = f"throttle: {mode}   sens {ramp_s:.1f}s"
            dash.render(screen, tel, sim.perf, clock.get_fps(), sim.scope, dt, extra,
                        sim.auto_state, ecusim.ve_status())
            pygame.display.flip()

    sim.stop()
    pygame.quit()


if __name__ == "__main__":
    main()
