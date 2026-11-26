# pytest setup for the ECU sim. One ECU per process, so each test file runs on its own (ctest
# runs one pytest per file), and the tests within a file share one boot, in order.
import os, sys
here = os.path.dirname(__file__)
sys.path.insert(0, os.path.join(here, "..", "engine"))
for d in (os.environ.get("ECUSIM_DIR"), os.path.join(here, "..", "..", "build-sim")):
    if d and os.path.isdir(d):
        sys.path.insert(0, d)

# Any free TCP port for TunerStudio, so several sim ECUs (reboot tests, ctest -j) can coexist.
os.environ.setdefault("ECU_SIM_PORT", "0")
