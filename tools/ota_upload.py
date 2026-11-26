#!/usr/bin/env python3
"""Upload firmware to the stm_trionic ECU over Ethernet (see src/ota.cpp).

usage: tools/ota_upload.py [build/stm_ecu] [--host 192.168.7.2]
The engine must be stopped. All outputs are switched off for the update.
"""
import argparse, socket, struct, subprocess, sys, time, zlib

ap = argparse.ArgumentParser()
ap.add_argument("elf", nargs="?", default="build/stm_ecu")
ap.add_argument("--host", default="192.168.7.2")
ap.add_argument("--port", type=int, default=29002)
a = ap.parse_args()

bin_path = a.elf + ".bin"
subprocess.run(["arm-none-eabi-objcopy", "-O", "binary", a.elf, bin_path], check=True)
image = open(bin_path, "rb").read()

# Sanity: vector table must start with a RAM stack pointer and a flash reset handler
sp, reset = struct.unpack_from("<II", image)
if not (0x20000000 <= sp <= 0x20020000 and 0x08000000 <= reset < 0x08080000):
    sys.exit(f"{bin_path} doesn't look like a firmware image (sp={sp:#x}, reset={reset:#x})")
if len(image) > 512 * 1024:
    sys.exit(f"image is {len(image)} bytes, the limit is 512 KB")

crc = zlib.crc32(image)
print(f"{bin_path}: {len(image)} bytes, crc32 {crc:08x}")

def connect(timeout):
    """The ECU's OTA socket takes a moment to free up after the previous connection: retry."""
    for attempt in range(20):
        try:
            return socket.create_connection((a.host, a.port), timeout=timeout)
        except ConnectionRefusedError:
            time.sleep(0.25)
    raise ConnectionRefusedError(f"{a.host}:{a.port} keeps refusing connections")

def uptime():
    """ECU uptime in seconds via the OTA port (free even while TunerStudio is connected), or None."""
    try:
        q = connect(1)
        q.sendall(b"OTAQ")
        d = q.recv(4)
        q.close()
        return struct.unpack("<I", d)[0] if len(d) == 4 else None
    except OSError:
        return None

before = uptime()
if before is None:
    sys.exit(f"no answer from {a.host}:{a.port}. Is the ECU running firmware with OTA support?")
print(f"ECU up for {before} s")

errors = {b"?": "bad request", b"S": "bad size", b"B": "engine is turning (or crank synced): stop it first",
          b"X": "flash erase failed", b"W": "flash write failed", b"C": "CRC mismatch after writing"}

s = connect(30)
s.sendall(b"OTA1" + struct.pack("<II", len(image), crc))

print("erasing staging area (the ECU stalls for a few seconds)...")
r = s.recv(1)
if r != b"E":
    sys.exit("ECU refused: " + errors.get(r, repr(r)))

sent, t0 = 0, time.time()
while sent < len(image):
    chunk = image[sent:sent + 4096]
    s.sendall(chunk)
    sent += len(chunk)
    print(f"\r  {sent * 100 // len(image):3d} %", end="", flush=True)
print(f"\nsent in {time.time() - t0:.1f} s, verifying...")

r = s.recv(1)
if r != b"V":
    sys.exit("update failed: " + errors.get(r, repr(r)) + " (the running firmware is untouched)")
print("verified. installing and rebooting (~5 s, don't power off)...")
s.close()

t_install = time.time()
for i in range(30):
    time.sleep(1)
    up = uptime()
    if up is not None and up <= time.time() - t_install + 2:
        print(f"ECU rebooted and is running (up {up} s). Outputs and simulator are off; re-enable in TunerStudio.")
        break
else:
    up = uptime()
    if up is not None:
        sys.exit(f"ECU answers but did NOT reboot (up {up} s): the install step didn't run.")
    sys.exit("ECU hasn't come back after 30 s. If it stays dead, reflash over JTAG.")
