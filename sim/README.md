# stm_trionic host simulation

The real firmware, compiled for the PC: its FreeRTOS tasks run on the real scheduler, against
fake STM32 registers, in lockstep with a Python model of the B204. Everything is driven and
tested from Python through one pybind11 module, `ecusim`.

## Everyday use (repo root)

    sudo apt install cmake ninja-build g++ python3-dev python3-venv zip   # once
    make sim                         # set up, build, run all tests
    make sim-test T=start_idle       # one test file, verbose, with its printout
    make sim-drive                   # drive the car in a window (installs pygame if needed)
    make sim-ecu                     # engine + ECU in real time; in another terminal:
    make bridge ECU_IP=127.0.0.1     # ...then connect TunerStudio as usual

`make sim` creates a Python venv in `.venv-sim` and a CMake build in `build-sim`. Dependencies
(FreeRTOS-Kernel, Kokkos mdspan, pybind11) are pinned in `sim/deps.cmake` and downloaded by
CMake; Python packages are in `sim/requirements.txt`.

## Driving simulator

`make sim-drive` opens a window with the engine idling and lets you drive a flat-road vehicle
(auto-clutch, 5-speed + reverse, aero and rolling load) on the keyboard. The real firmware runs
inside, so the injector and spark timing shown is the actual scheduler output, and TunerStudio can
connect at the same time (the status panel shows the port) to tune while you drive.

    W           throttle (spring by default: returns to 0 on release)
    S           ease throttle off / faster lift        SPACE  brake
    H           toggle throttle hold vs spring         -/=  throttle sensitivity
    A/D or Left/Right  shift      1-5 gear, 0 neutral   C  clutch (hold)   K  starter
    L           closed loop on/off     G  autodrive (sweep VE map)
    [ ]         scope time window
    G           autodrive (auto-sweep the VE map)
    R           reset             mouse: click a probe name to show/hide it on the scope
    ESC         quit

The engine runs on its own thread paced to the wall clock, separate from the drawing, so it keeps
real time however fast the window redraws; the status panel shows "sim speed" (1.0x = real time)
and headroom. The dashboard shows speed in mph, gear and a tacho; throttle/brake/clutch (with the
auto-clutch state OPEN/SLIPPING/LOCKED), MAP, injector duty, VE, AFR and trim; a live 720-degree
view of every injector pulse and both wasted-spark firings per plug; a configurable rolling scope
(tick the probes you want); and a pseudo-3D road with real-scale trees and oncoming traffic.
Overrun fuel cut (DFCO) is enabled
in the sim, so lifting off in gear closes the injectors and shows "FUEL CUT" until fuel resumes;
cut/resume rpm and the enable live in the TunerStudio "Fuel" menu, which also holds the VE table,
AFR-target and dead-time curves, and fuel setup (the firmware publishes the fuel tables as
TunerStudio page 2, so free TunerStudio can view them live; the VE table is read-only there since
the ECU's closed-loop learning owns it). The firmware also tracks how well-tuned each VE cell is
(enough closed-loop dwell with AFR on target) and publishes that as a read-only "VE tune status"
heatmap on page 3. Press G in the driving window to autodrive: it sweeps the engine through its
reachable load points (throttle for MAP, brake for rpm), discovering reachability at runtime, to
learn the whole map; a live status heatmap shows progress. Autodrive holds the car stationary on the brake in a tall gear and uses the clutch as a load knob (a virtual dyno), so it can place the engine at any reachable rpm/MAP without driving anywhere.

## How time works## How time works

C++ owns the clock. `ecusim.advance_to(t_us)` runs the timer model up to `t_us`, and at every
simulated millisecond delivers one FreeRTOS tick, then waits until all firmware tasks are
blocked again (the idle task has run). So the firmware sees exactly one tick per simulated
millisecond however fast the PC is, and runs are repeatable. Python never touches ticks: it
sends crank edges and sensor values, and reads back injector/spark edges and status.

Running firmware: crank decoder and output scheduler (timer interrupts), and the ts_link
(TunerStudio), fuel, power and settings tasks. The SD card is a virtual card in memory driven
by the same `settings::service()` the real SD task calls. Sensor values come from Python.

One ECU per process: the scheduler can't be restarted, so `ecusim.start()` boots it once.
Each test file is therefore one boot (ctest runs one pytest per file); tests within a file
run in order on one timeline. To test a reboot, start a second ECU in a subprocess (see
`test_settings.py`).

## Handing the sim to an offline machine (or to Claude's sandbox)

    make sim-deps                          # on a machine with internet: writes sim-deps.zip
    unzip sim-deps.zip -d sim-deps         # on the offline machine
    make sim SIM_DEPS=sim-deps

## Layout

| Path | What |
|---|---|
| `py/ecusim.cpp` | the pybind11 module: start, advance_to, engine I/O, tune, status, SD card |
| `rtos/lockstep.*`, `rtos/port_lockstep.c` | FreeRTOS POSIX port with the tick from the simulated clock |
| `model/periph.*` | event-driven timer model: crank captures in, injector/spark edges out |
| `model/board.*` | rest of the board: power registers, sleep hooks, virtual SD card task |
| `fake/stm32f4xx_hal.h` | fake STM32 registers |
| `net/` | FreeRTOS+TCP socket API on Linux sockets (no pcap, no root) |
| `engine/b204.py` | engine model: crank inertia, manifold filling, VE, injectors, wideband |
| `engine/bench.py` | couples the engine model to the ECU |
| `engine/wheel.py` | ideal 60-2 wheel at a commanded rpm (no engine), angle helpers |
| `engine/run_ecu.py` | real-time engine + ECU for TunerStudio (`make sim-ecu`) |
| `engine/vehicle.py` | drivetrain + road load: clutch, gearbox, aero/rolling resistance |
| `engine/world.py` | couples firmware + engine + vehicle, holds controls, telemetry |
| `engine/simloop.py` | runs the world on its own thread, paced to real time; scope history |
| `engine/dashboard.py` | the dashboard (pygame): gauges, timing, status, scope |
| `engine/road_view.py` | pseudo-3D road scene (pygame) |
| `engine/autodrive.py` | auto-sweep controller: learns reachable VE cells (throttle/brake) |
| `engine/simdrive.py` | the driving window: input + drawing (`make sim-drive`) |
| `tests/test_*.py` | the tests, pytest, one ECU boot per file |

## Tests

| File | Checks |
|---|---|
| `test_timing.py` | spark/injection angles and pulse lengths, 300-6000 rpm |
| `test_power.py` | key-on prime, +15 noise, cranking ride-through and 3 s limit, key-off cut, sleep |
| `test_fuel.py` | speed-density maths, closed loop + VE learning, cranking pulse |
| `test_dfco.py` | overrun fuel cut: injectors close, trim freezes, fuel resumes |
| `test_status.py` | VE tune-status rises with learning; read-only page 3 over the protocol |
| `test_autodrive.py` | autodrive sweeps the load map and discovers reachable cells |
| `test_settings.py` | burn, periodic and key-off saves, power cut mid-write, reboots, corrupt card |
| `test_ts_link.py` | TunerStudio protocol over TCP: signature, realtime data, burn |
| `test_start_idle.py` | B204 model: crank, start, closed-loop idle, throttle blip |
| `test_vehicle.py` | drivetrain physics: gears, launch, braking, reverse, engine braking |
| `test_world_drive.py` | full driving stack: launch, upshift, brake, both-spark timing, mph |
| `test_simloop.py` | threaded runner keeps real time; scope history and controls |
| `test_engine_model.py` | engine model alone, with a crude Python ECU |

## Writing a test

Start the ECU at the top of the file (`ecusim.start()`, optionally with `card=` files or
`ignition=False`), set the tune with `ecusim.set_tune(...)`, then either spin an ideal wheel
(`Wheel().run_to(t_us, rpm)`) or run the engine (`Bench().run(seconds, on_step=driver)`) and
assert on `crank_status()`, `fuel_status()`, `power_status()`, `take_edges()` and friends.
