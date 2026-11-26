# Developer workflows for stm_trionic. CMake still does the firmware build; this just wraps it.
#
#   make            build firmware
#   make flash      build, upload over Ethernet, confirm the reboot
#   make flash-jtag build, program over JTAG/SWD with OpenOCD
#   make bridge     TunerStudio serial bridge (socat): /dev/ttyUSB9 <-> ECU TCP port
#   make status     is the ECU up? (uptime over the OTA port)
#   make sim        build the host simulation and run all its tests
#   make sim-ecu    run the simulated ECU (FreeRTOS + TunerStudio protocol on 127.0.0.1:29001)
#   make sim-scenario S=start_idle   run one engine scenario (sim/engine/scenario_<S>.py)
#   make sim-deps   download every sim dependency into sim-deps.zip (for offline builds)
#   make sim SIM_DEPS=<dir>          build offline from an unpacked sim-deps.zip
#   make clean      remove firmware and sim build output

ECU_IP     ?= 192.168.7.2
TS_PORT    ?= 29001
OTA_PORT   ?= 29002
TS_TTY     ?= /dev/ttyUSB9
BUILD_DIR  ?= build
ELF        := $(BUILD_DIR)/stm_ecu
OPENOCD_CFG ?= openocd-jlink.cfg

.PHONY: all build flash flash-jtag bridge status sim sim-build sim-ecu sim-scenario sim-deps clean

all: build

$(BUILD_DIR)/build.ninja:
	cmake -B $(BUILD_DIR) -G Ninja

build: $(BUILD_DIR)/build.ninja
	cmake --build $(BUILD_DIR)
	@arm-none-eabi-size $(ELF)

flash: build
	python3 tools/ota_upload.py $(ELF) --host $(ECU_IP) --port $(OTA_PORT)

flash-jtag: build
	openocd -f $(OPENOCD_CFG) -c "program $(ELF) verify reset exit"

# Restarts automatically when the ECU reboots or TunerStudio disconnects. Ctrl+C to stop.
bridge:
	@echo "TunerStudio: serial port $(TS_TTY) -> $(ECU_IP):$(TS_PORT)   (Ctrl+C to stop)"
	@while true; do \
		sudo socat pty,link=$(TS_TTY),raw,echo=0,wait-slave,mode=666 tcp:$(ECU_IP):$(TS_PORT); \
		sleep 1; \
	done

status:
	@python3 -c "import socket,struct;s=socket.create_connection(('$(ECU_IP)',$(OTA_PORT)),timeout=2);s.sendall(b'OTAQ');print('ECU up for',struct.unpack('<I',s.recv(4))[0],'s')" \
		|| echo "ECU not answering on $(ECU_IP):$(OTA_PORT)"

# ---------------- host simulation ----------------
SIM_BUILD ?= build-sim
SIM_VENV  ?= .venv-sim
SIM_DEPS  ?=
S         ?= start_idle
SIM_PY    := $(SIM_VENV)/bin/python

ifneq ($(SIM_DEPS),)
  SIM_FETCH := -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
               -DFETCHCONTENT_SOURCE_DIR_FREERTOS_KERNEL=$(abspath $(SIM_DEPS))/src/freertos_kernel \
               -DFETCHCONTENT_SOURCE_DIR_MDSPAN=$(abspath $(SIM_DEPS))/src/mdspan \
               -DFETCHCONTENT_SOURCE_DIR_PYBIND11=$(abspath $(SIM_DEPS))/src/pybind11
  SIM_PIP   := --no-index --find-links $(SIM_DEPS)/wheels
endif

$(SIM_PY):
	python3 -m venv --system-site-packages $(SIM_VENV)
	$(SIM_PY) -m pip install -q $(SIM_PIP) -r sim/requirements.txt

$(SIM_BUILD)/build.ninja: | $(SIM_PY)
	cmake -S sim -B $(SIM_BUILD) -G Ninja -DPython3_EXECUTABLE=$(abspath $(SIM_PY)) $(SIM_FETCH)

sim-build: $(SIM_BUILD)/build.ninja
	cmake --build $(SIM_BUILD)

sim: sim-build
	ctest --test-dir $(SIM_BUILD) --output-on-failure -j4

# Engine + ECU in real time for TunerStudio: then  make bridge ECU_IP=127.0.0.1
sim-ecu: sim-build
	PYTHONPATH=$(SIM_BUILD):sim/engine $(SIM_PY) sim/engine/run_ecu.py

# One test file, verbose with its printout, e.g.  make sim-test T=start_idle
sim-test: sim-build
	ECUSIM_DIR=$(SIM_BUILD) $(SIM_PY) -m pytest -v -s sim/tests/test_$(T).py

# Drive the simulated car in a window (needs pygame; installs it into the sim venv if missing).
sim-drive: sim-build
	@$(SIM_PY) -c "import pygame" 2>/dev/null || $(SIM_PY) -m pip install -q $(SIM_PIP) pygame
	PYTHONPATH=$(SIM_BUILD):sim/engine $(SIM_PY) sim/engine/simdrive.py


# Run on a machine with internet. Produces sim-deps.zip: src/<dep>/ trees + wheels/.
sim-deps: $(SIM_BUILD)/build.ninja
	rm -rf $(SIM_BUILD)/deps-pack && mkdir -p $(SIM_BUILD)/deps-pack/src $(SIM_BUILD)/deps-pack/wheels
	for d in freertos_kernel mdspan pybind11; do cp -r $(SIM_BUILD)/_deps/$$d-src $(SIM_BUILD)/deps-pack/src/$$d; done
	$(SIM_PY) -m pip download -q -r sim/requirements.txt -d $(SIM_BUILD)/deps-pack/wheels
	cd $(SIM_BUILD)/deps-pack && rm -f ../../sim-deps.zip && zip -qr ../../sim-deps.zip src wheels
	@echo "wrote sim-deps.zip"

clean:
	rm -rf $(BUILD_DIR) $(SIM_BUILD)
