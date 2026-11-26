"""VE tune-status: cells gain status as closed-loop learning puts AFR on target, status is exposed
as read-only page 3, and the page reads/CRCs over the protocol matching the module view."""
import socket, struct, time, zlib
import ecusim
from simloop import SimThread

sim = SimThread()
sim.start()
sim.wait_ready(timeout=30)
time.sleep(0.3)


def test_idle_cells_gain_status():
    sim.controls.gear = 0
    sim.controls.throttle = 0.0
    time.sleep(14)
    st = ecusim.ve_status()
    best = max(max(row) for row in st)
    # untouched high-rpm/high-load corner stays 0; idle region climbs
    assert st[15][15] == 0 and best > 25


def test_page3_reads_and_matches():
    s = socket.create_connection(("127.0.0.1", sim.latest()["ts_port"]), timeout=5)

    def rx(n):
        d = b""
        while len(d) < n:
            d += s.recv(n - len(d))
        return d

    def cmd(pl):
        s.sendall(struct.pack(">H", len(pl)) + pl + struct.pack(">I", zlib.crc32(pl)))
        n = struct.unpack(">H", rx(2))[0]
        return rx(n + 4)[:n]

    out, off = b"", 0
    while off < 320:
        c = min(256, 320 - off)
        r = cmd(b"p" + bytes([0, 3]) + struct.pack("<H", off) + struct.pack("<H", c))
        assert r[0] == 0
        out += r[1:]
        off += c
    assert len(out) == 320
    d = cmd(b"d" + bytes([0, 3]))
    assert struct.unpack(">I", d[1:5])[0] == (zlib.crc32(out) & 0xFFFFFFFF)
    tuned = [[out[64 + (r * 16 + c)] for c in range(16)] for r in range(16)]
    ref = ecusim.ve_status()
    assert all(tuned[r][c] == ref[r][c] for r in range(16) for c in range(16))
    s.close()
    sim.stop()
