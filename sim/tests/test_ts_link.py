"""TunerStudio protocol over TCP against the running ts_link task (port from ecusim.ts_port()).
The client runs on a thread; the main thread keeps simulated time moving."""
import socket, struct, threading, zlib
import ecusim

ecusim.start()


def with_client(fn):
    """Run fn(cmd) on a client thread while advancing time; return its result."""
    out = {}
    def client():
        try:
            s = socket.create_connection(("127.0.0.1", ecusim.ts_port()), timeout=5)
            def rx(n):
                d = b""
                while len(d) < n:
                    c = s.recv(n - len(d))
                    if not c: raise RuntimeError("connection closed")
                    d += c
                return d
            def cmd(pl):
                s.sendall(struct.pack(">H", len(pl)) + pl + struct.pack(">I", zlib.crc32(pl)))
                n = struct.unpack(">H", rx(2))[0]; d = rx(n + 4)
                assert struct.unpack(">I", d[n:])[0] == zlib.crc32(d[:n]), "bad CRC"
                return d[:n]
            out["result"] = fn(cmd)
            s.close()
        except BaseException as e:
            out["error"] = e
    th = threading.Thread(target=client); th.start()
    while th.is_alive():
        ecusim.advance_to(ecusim.now_us() + 1000)
    if "error" in out: raise out["error"]
    return out["result"]


def test_signature():
    sig = with_client(lambda cmd: cmd(b"Q"))
    assert sig.startswith(b"\x00speeduino")


def test_realtime_data_moves():
    def two_reads(cmd):
        a = cmd(b"r\x00\x30\x00\x00\x40\x00")
        t = ecusim.now_us()
        while ecusim.now_us() < t + 1_500_000: pass      # main thread advances the clock
        return a, cmd(b"r\x00\x30\x00\x00\x40\x00")
    a, b = with_client(two_reads)
    assert len(a) == len(b) == 65 and b[1] > a[1], "seconds counter advances"


def test_burn_writes_the_card():
    rc = with_client(lambda cmd: cmd(b"b\x00\x01"))
    assert rc[0] == 0x04                                   # RC_BURN_OK
    ecusim.advance_to(ecusim.now_us() + 1_100_000)
    assert "ECU/tune_a.bin" in ecusim.card_files()
