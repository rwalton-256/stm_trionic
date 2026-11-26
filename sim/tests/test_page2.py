"""The fuel tables exposed as TunerStudio page 2: read it back over the protocol and check it
matches the firmware's VE table, the CRC command agrees, and a written cell lands in the ECU."""
import socket, struct, time, zlib
import ecusim
from simloop import SimThread

sim = SimThread()
sim.start()
sim.wait_ready(timeout=30)
time.sleep(0.4)
_s = socket.create_connection(("127.0.0.1", sim.latest()["ts_port"]), timeout=5)


def _rx(n):
    d = b""
    while len(d) < n:
        d += _s.recv(n - len(d))
    return d


def _cmd(pl):
    _s.sendall(struct.pack(">H", len(pl)) + pl + struct.pack(">I", zlib.crc32(pl)))
    n = struct.unpack(">H", _rx(2))[0]
    d = _rx(n + 4)
    assert struct.unpack(">I", d[n:])[0] == zlib.crc32(d[:n])
    return d[:n]


def _read_page(page, size, chunk=256):
    out, off = b"", 0
    while off < size:
        c = min(chunk, size - off)
        r = _cmd(b"p" + bytes([0, page]) + struct.pack("<H", off) + struct.pack("<H", c))
        assert r[0] == 0
        out += r[1:]
        off += c
    return out


def test_ve_table_matches_firmware():
    pg = _read_page(2, 632)
    assert len(pg) == 632
    ve = [[struct.unpack_from("<H", pg, 64 + (r * 16 + c) * 2)[0] / 10.0 for c in range(16)] for r in range(16)]
    ref = ecusim.ve_table()
    assert all(abs(ve[r][c] - ref[r][c]) < 0.05 for r in range(16) for c in range(16))


def test_axes_and_afr_target():
    pg = _read_page(2, 632)
    rpm_bins = [struct.unpack_from("<H", pg, i * 2)[0] for i in range(16)]
    map_bins = [struct.unpack_from("<H", pg, 32 + i * 2)[0] / 10 for i in range(16)]
    afr = [pg[618 + i] / 10 for i in range(8)]
    assert rpm_bins[0] == 500 and rpm_bins[-1] == 6500
    assert map_bins[0] == 20.0
    assert afr[0] == 14.7 and afr[-1] == 12.5


def test_page_crc_agrees():
    pg = _read_page(2, 632)
    d = _cmd(b"d" + bytes([0, 2]))
    assert struct.unpack(">I", d[1:5])[0] == (zlib.crc32(pg) & 0xFFFFFFFF)


def test_write_cell_reaches_firmware():
    off = 64 + (3 * 16 + 4) * 2
    _cmd(b"M" + bytes([0, 2]) + struct.pack("<H", off) + struct.pack("<H", 2) + struct.pack("<H", 925))
    time.sleep(0.1)
    assert abs(ecusim.ve_table()[3][4] - 92.5) < 0.01
