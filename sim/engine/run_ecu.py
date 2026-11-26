"""Run the simulated engine + ECU in real time, for TunerStudio.

    make sim-ecu            then, in another terminal:  make bridge ECU_IP=127.0.0.1

The engine is started and idles; tune it live from TunerStudio. Ctrl+C to stop.
Options: --port N (default 29001), --seconds S (default: until Ctrl+C)."""
import argparse, os, time

ap = argparse.ArgumentParser()
ap.add_argument("--port", type=int, default=29001)
ap.add_argument("--seconds", type=float, default=0)
args = ap.parse_args()
os.environ["ECU_SIM_PORT"] = str(args.port)

import ecusim
from bench import Bench

b = Bench()
print(f"sim ECU: TunerStudio port {args.port}; cranking...", flush=True)
t0_sim, t0_real, last_print = b.t_us, time.monotonic(), 0.0


def driver(b):
    t = (b.t_us - t0_sim) * 1e-6
    b.engine.starter_nm = 45.0 if (0.1 < t < 3.0 and b.engine.rpm < 450) else 0.0


try:
    while not args.seconds or (b.t_us - t0_sim) * 1e-6 < args.seconds:
        b.run(0.05, every=1.0, on_step=driver)
        ahead = (b.t_us - t0_sim) * 1e-6 - (time.monotonic() - t0_real)
        if ahead > 0:
            time.sleep(ahead)                     # hold simulated time to the wall clock
        t = (b.t_us - t0_sim) * 1e-6
        if t - last_print >= 2.0:
            last_print = t
            c, f = ecusim.crank_status(), ecusim.fuel_status()
            print(f"{t:6.0f} s  {b.engine.rpm:5.0f} rpm  MAP {b.engine.map_kpa:5.1f}  AFR {b.engine.afr:4.1f}  "
                  f"pw {f['pw_us']} us  trim {f['trim']:.2f}", flush=True)
except KeyboardInterrupt:
    pass
