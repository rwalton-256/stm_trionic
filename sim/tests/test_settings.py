"""Settings on the virtual SD card, through the real SD/settings tasks: burn, periodic save
while VE changes, key-off save, a power cut mid-write, and reboots that load the newest
good file (each reboot is a fresh ECU in a subprocess)."""
import json, os, subprocess, sys
import ecusim

ecusim.start()                        # empty card
A, B = "ECU/tune_a.bin", "ECU/tune_b.bin"


def run_s(seconds):
    ecusim.advance_to(ecusim.now_us() + int(seconds * 1e6))


def reboot(files):
    """Boot a second ECU with these card files; return its settings status and VE[3][4]."""
    code = ("import sys, json, ecusim\n"
            "files = {k: bytes.fromhex(v) for k, v in json.load(sys.stdin).items()}\n"
            "ecusim.start(card=files); ecusim.advance_to(1_500_000)\n"
            "print(json.dumps(dict(ecusim.settings_status(), ve=ecusim.ve_table()[3][4])))\n")
    r = subprocess.run([sys.executable, "-c", code], input=json.dumps({k: v.hex() for k, v in files.items()}),
                       capture_output=True, text=True, env=dict(os.environ, PYTHONPATH=os.pathsep.join(sys.path)))
    assert r.returncode == 0, r.stderr
    return json.loads(r.stdout)


def test_boots_on_defaults_with_empty_card():
    run_s(2)
    s = ecusim.settings_status()
    assert s["source"] == "defaults" and not s["dirty"] and ecusim.card_files() == {}


def test_burn_saves_to_file_a():
    ecusim.burn(); run_s(1.1)
    assert list(ecusim.card_files()) == [A] and ecusim.settings_status()["seq"] == 1


def test_ve_change_saved_within_a_minute():
    ecusim.set_ve(3, 4, 81.0); run_s(1.1)
    assert ecusim.settings_status()["dirty"]
    run_s(60)
    s = ecusim.settings_status()
    assert not s["dirty"] and s["seq"] == 2 and B in ecusim.card_files()


def test_power_cut_mid_write_keeps_last_good():
    ecusim.set_ve(3, 4, 82.0)
    ecusim.card_tear_next_write(40); ecusim.burn(); run_s(1.1)
    assert ecusim.settings_status()["save_errors"] == 1
    r = reboot(ecusim.card_files())
    assert r["source"] == "tune_b" and r["seq"] == 2 and r["ve"] == 81.0


def test_key_off_saves_changes():
    ecusim.set_ve(3, 4, 83.0); run_s(1.1)
    ecusim.set_ignition(False); run_s(1.5)
    s = ecusim.settings_status()
    assert not s["dirty"] and s["seq"] == 3
    r = reboot(ecusim.card_files())
    assert r["source"] == "tune_a" and r["seq"] == 3 and r["ve"] == 83.0


def test_corrupt_card_falls_back_to_defaults():
    files = {k: bytes([v[0] ^ 0xFF]) + v[1:] for k, v in ecusim.card_files().items()}
    r = reboot(files)
    assert r["source"] == "defaults"
