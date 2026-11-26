"""The dashboard, drawn directly with pygame. Speed (mph) / gear / tacho, engine vitals, the live
720 deg injector+spark timing (both wasted-spark events per plug), a status column with the
auto-clutch state and the sim-speed readout, a configurable multi-probe scope, and the road view.

The scope probes are mouse-toggled checkboxes and the time window is adjustable; the Dashboard
object holds that UI state. All simulation lives elsewhere (simloop); this only reads a telemetry
snapshot and a scope snapshot.
"""
import pygame
from simloop import SCOPE_CHANNELS
from road_view import RoadView

BG       = (18, 20, 24)
PANEL    = (28, 31, 37)
PANEL_HI = (38, 42, 50)
EDGE     = (55, 60, 70)
INK      = (225, 228, 233)
DIM      = (140, 146, 156)
GREEN    = (70, 200, 120)
RED      = (224, 86, 86)
AMBER    = (240, 176, 64)
CYAN     = (86, 190, 220)
BLUE     = (96, 150, 240)
PURPLE   = (176, 130, 230)
CYL_COL  = [(240, 176, 64), (86, 190, 220), (70, 200, 120), (176, 130, 230)]
WINDOWS  = [3.0, 6.0, 12.0, 20.0, 40.0]


class Dashboard:
    def __init__(self, w, h):
        self.W, self.H = w, h
        self._fonts = {}
        self.enabled = {"rpm", "throttle", "map_kpa", "afr"}
        self.win_idx = 1
        self._checkboxes = []
        self.road = RoadView()

    def font(self, size, bold=False):
        key = (int(size), bold)
        if key not in self._fonts:
            self._fonts[key] = pygame.font.SysFont("dejavusans,arial", int(size), bold=bold)
        return self._fonts[key]

    def text(self, s, x, y, color=INK, size=14, anchor="nw", bold=False):
        surf = self.font(size, bold).render(str(s), True, color)
        attr = {"nw": "topleft", "n": "midtop", "ne": "topright", "w": "midleft",
                "center": "center", "e": "midright", "sw": "bottomleft", "s": "midbottom",
                "se": "bottomright"}[anchor]
        self.scr.blit(surf, surf.get_rect(**{attr: (int(x), int(y))}))

    def panel(self, x, y, w, h, title=None):
        pygame.draw.rect(self.scr, PANEL, (x, y, w, h), border_radius=8)
        pygame.draw.rect(self.scr, EDGE, (x, y, w, h), 1, border_radius=8)
        if title:
            self.text(title.upper(), x + 12, y + 8, DIM, 12, "nw", bold=True)

    def bar(self, x, y, w, h, frac, color):
        frac = max(0.0, min(1.0, frac))
        pygame.draw.rect(self.scr, PANEL_HI, (x, y, w, h), border_radius=4)
        if frac > 0:
            pygame.draw.rect(self.scr, color, (x, y, max(3, int(w * frac)), h), border_radius=4)

    def labelled_bar(self, x, y, w, frac, color, label, value):
        self.text(label, x, y - 1, DIM, 12, "nw")
        self.text(value, x + w, y - 1, INK, 12, "ne", bold=True)
        self.bar(x, y + 15, w, 8, frac, color)

    def handle_click(self, pos):
        for rect, key in self._checkboxes:
            if rect.collidepoint(pos):
                self.enabled.symmetric_difference_update({key})
                return True
        return False

    def cycle_window(self, delta):
        self.win_idx = max(0, min(len(WINDOWS) - 1, self.win_idx + delta))

    def render(self, scr, tel, perf, fps, scope_fn, frame_dt, extra="", auto=None, ve_status=None):
        self.scr = scr
        scr.fill(BG)
        M = 10
        colW = self.W - 2 * M

        ax, ay, ah = M, M, 132
        self._top(ax, ay, ah, tel)

        by = ay + ah + M
        bh = 300
        vw = 350
        self.panel(M, by, vw, bh, "engine vitals")
        self._vitals(M + 16, by + 34, vw - 32, tel)
        tmx = M + vw + M
        tmw = 476
        self.panel(tmx, by, tmw, bh, "injector + spark timing  (720 deg cycle)")
        self._timing(tmx + 14, by + 30, tmw - 28, bh - 44, tel)
        stx = tmx + tmw + M
        stw = self.W - M - stx
        self.panel(stx, by, stw, bh, "status")
        self._status(stx + 14, by + 34, stw - 28, tel, perf, fps)

        cy = by + bh + M
        ch = 118
        self._scope(M, cy, colW, ch, scope_fn)

        dy = cy + ch + M
        dh = self.H - M - dy
        self.panel(M, dy, colW, dh)
        self.road.update(frame_dt, tel["speed_mph"])
        self.road.draw(scr, (M + 1, dy + 1, colW - 2, dh - 2), tel)

        if ve_status is not None:
            self._ve_heatmap(self.W - 10 - 168, dy + 8, 160, dh - 16, ve_status, auto)
        if auto and auto.get("on"):
            tc = auto.get("target")
            self.text(f"AUTODRIVE  cov {auto['coverage']*100:.0f}%" +
                      (f"  cell r{tc[0]} c{tc[1]}" if tc else "  (done)"),
                      M + 14, dy + 8, GREEN, 13, "nw", bold=True)

        self.text("W throttle  S ease  SPACE brake  A/D shift  1-5/0 gear  C clutch  K start  "
                  "L closed-loop  H hold  G autodrive  -/= sens  [ ] window  R reset  ESC quit",
                  M + 14, self.H - 6, DIM, 11, "sw")
        if extra:
            self.text(extra, self.W - M - 14, self.H - 6, AMBER, 12, "se", bold=True)

    def _ve_heatmap(self, x, y, w, h, grid, auto):
        rows, cols = len(grid), len(grid[0])
        cw, ch = w / cols, (h - 14) / rows
        pygame.draw.rect(self.scr, (10, 12, 15), (x - 2, y - 2, w + 4, h + 2), border_radius=4)
        self.text("VE tune status", x, y - 2, DIM, 10, "nw")
        for r in range(rows):                      # row 0 = lowest MAP, drawn at the bottom
            for c in range(cols):
                v = grid[rows - 1 - r][c] / 100.0
                col = (int(200 * (1 - v) + 40 * v), int(40 + 180 * v), 50)
                pygame.draw.rect(self.scr, col, (x + c * cw, y + 12 + r * ch, cw + 1, ch + 1))
        if auto and auto.get("target"):
            tr, tc = auto["target"]
            pygame.draw.rect(self.scr, (255, 255, 255),
                             (x + tc * cw, y + 12 + (rows - 1 - tr) * ch, cw, ch), 1)

    def _top(self, ax, ay, ah, t):
        sw = 220
        self.panel(ax, ay, sw, ah, "speed")
        self.text(f"{abs(t['speed_mph']):.0f}", ax + sw / 2, ay + 58, INK, 58, "center", bold=True)
        self.text("mph", ax + sw / 2, ay + 98, DIM, 14, "center")
        gx, gw = ax + sw + 10, 110
        self.panel(gx, ay, gw, ah, "gear")
        gcol = GREEN if t["gear"] not in ("N",) else DIM
        if t["gear"] == "R":
            gcol = RED
        self.text(t["gear"], gx + gw / 2, ay + 70, gcol, 64, "center", bold=True)
        tx = gx + gw + 10
        tw = self.W - 10 - tx
        self.panel(tx, ay, tw, ah, "engine rpm")
        self._tacho(tx + 16, ay + 44, tw - 32, 50, t)

    def _tacho(self, x, y, w, h, t):
        rpm = max(0.0, t["rpm"])
        rmax, redline = 7000.0, 6500.0
        pygame.draw.rect(self.scr, PANEL_HI, (x, y, w, h), border_radius=6)
        rx = x + w * redline / rmax
        pygame.draw.rect(self.scr, (70, 34, 34), (rx, y, x + w - rx, h))
        col = RED if (rpm >= redline or t["rev_limited"]) else AMBER
        pygame.draw.rect(self.scr, col, (x, y, max(3, int(w * rpm / rmax)), h), border_radius=6)
        for k in range(8):
            gx = x + w * (k * 1000) / rmax
            pygame.draw.line(self.scr, EDGE, (gx, y), (gx, y + h))
            self.text(str(k), gx, y + h + 3, DIM, 11, "n")
        self.text(f"{rpm:.0f}", x + w, y - 6, INK, 30, "ne", bold=True)
        if not t["synced"]:
            self.text("NO SYNC", x + 6, y + h / 2, RED, 16, "w", bold=True)

    def _vitals(self, x, y, w, t):
        rows = [("Throttle", t["throttle"], GREEN, f"{t['throttle']*100:.0f}%"),
                ("Brake", t["brake"], RED, f"{t['brake']*100:.0f}%"),
                ("Clutch", t["clutch_engage"], BLUE,
                 f"{t['clutch_state']} {t['clutch_engage']*100:.0f}%"),
                ("MAP", t["map_kpa"] / 110.0, CYAN, f"{t['map_kpa']:.0f} kPa"),
                ("Inj duty", t["duty"] / 100.0, AMBER, f"{t['duty']:.0f}%"),
                ("VE", t["ve"] / 120.0, PURPLE, f"{t['ve']:.0f}%")]
        dy = 30
        for i, (lab, frac, col, val) in enumerate(rows):
            self.labelled_bar(x, y + i * dy, w, frac, col, lab, val)
        ya = y + len(rows) * dy
        self.text("AFR", x, ya - 1, DIM, 12, "nw")
        self.text(f"{t['afr']:.1f}  (tgt {t['afr_target']:.1f})", x + w, ya - 1, INK, 12, "ne", bold=True)
        lo, hi = 10.0, 18.0
        self.bar(x, ya + 15, w, 8, (t["afr"] - lo) / (hi - lo),
                 GREEN if abs(t["afr"] - t["afr_target"]) < 0.6 else AMBER)
        tgx = x + w * max(0.0, min(1.0, (t["afr_target"] - lo) / (hi - lo)))
        pygame.draw.line(self.scr, INK, (tgx, ya + 13), (tgx, ya + 25), 2)
        yt = ya + dy
        self.text("CL trim", x, yt - 1, DIM, 12, "nw")
        self.text(f"{t['trim']:.3f}", x + w, yt - 1, INK, 12, "ne", bold=True)
        pygame.draw.rect(self.scr, PANEL_HI, (x, yt + 15, w, 8), border_radius=4)
        mid = x + w / 2
        tf = t["trim"] - 1.0
        bw = w / 2 * max(0.0, min(1.0, abs(tf) / 0.25))
        col = GREEN if abs(tf) < 0.05 else AMBER
        pygame.draw.rect(self.scr, col, (mid if tf >= 0 else mid - bw, yt + 15, max(1, bw), 8), border_radius=4)
        pygame.draw.line(self.scr, EDGE, (mid, yt + 13), (mid, yt + 25))

    def _timing(self, x, y, w, h, t):
        gutter, rlab = 40, 96
        axx, aw = x + gutter, w - gutter - rlab

        def gx(deg):
            return axx + aw * (deg % 720.0) / 720.0

        for d in range(0, 721, 90):
            gxp = axx + aw * d / 720.0
            pygame.draw.line(self.scr, EDGE, (gxp, y), (gxp, y + h - 16))
            self.text(str(d), gxp, y + h - 14, DIM, 10, "n")
        lane_h = (h - 20) / 4
        tdc = t["tdc"]
        for cyl in range(4):
            ly = y + cyl * lane_h
            col = CYL_COL[cyl]
            pygame.draw.line(self.scr, (90, 96, 108), (gx(tdc[cyl]), ly + 2), (gx(tdc[cyl]), ly + lane_h - 4))
            for (start, dur_deg, dur_us, _t) in t["inj"][cyl]:
                dur = min(dur_deg, 720.0)
                segs = [(start, start + dur)] if start + dur <= 720 else [(start, 720), (0, start + dur - 720)]
                for a, b in segs:
                    pygame.draw.rect(self.scr, col, (gx(a), ly + 6, max(2, (b - a) / 720.0 * aw), lane_h - 18),
                                     border_radius=3)
            for (fire, adv, _t) in t["spk"][cyl]:
                pygame.draw.line(self.scr, RED, (gx(fire), ly + 2), (gx(fire), ly + lane_h - 4), 2)
            self.text(f"cyl{cyl+1}", x, ly + lane_h / 2 - 7, col, 12, "nw", bold=True)
            self.text(f"{t['last_inj_us'][cyl]/1000:.1f} ms  {t['last_adv_deg'][cyl]:+.0f}\u00b0",
                      x + w, ly + lane_h / 2 - 7, INK, 11, "ne")
        self.text("bars = injector open    red = spark (fires twice / cycle)    grey = TDC",
                  axx, y + h - 2, DIM, 10, "nw")
        if t["dfco"]:
            self.text("FUEL CUT", x + w, y - 2, CYAN, 13, "ne", bold=True)

    def _status(self, x, y, w, t, perf, fps):
        def row(i, lab, val, col=INK):
            yy = y + i * 18
            self.text(lab, x, yy, DIM, 12, "nw")
            self.text(val, x + w, yy, col, 13, "ne", bold=True)
        cs_col = {"LOCKED": GREEN, "SLIPPING": AMBER, "OPEN": DIM}[t["clutch_state"]]
        row(0, "clutch", f"{t['clutch_state']}  slip {t['slip_rpm']:+.0f}", cs_col)
        row(1, "AFR / target", f"{t['afr']:.1f} / {t['afr_target']:.1f}")
        row(2, "lambda", f"{t['lam']:.2f}")
        row(3, "pulse width", f"{t['pw_us']/1000:.2f} ms")
        row(4, "VE", f"{t['ve']:.0f} %")
        row(5, "power / torque", f"{t['power_kw']:.0f} kW / {t['torque_nm']:.0f} Nm")
        row(6, "coolant / IAT", f"{t['clt_c']:.0f} / {t['iat_c']:.0f} \u00b0C")
        row(7, "battery", f"{t['battery_v']:.1f} V")
        row(8, "overrun cut", "FUEL CUT" if t["dfco"] else "-", CYAN if t["dfco"] else DIM)
        row(9, "misfires", f"{t['misfires']}")
        row(10, "closed loop", ("ON +learn" if t["learning"] else "ON") if t["closed_loop"] else "off",
            GREEN if t["closed_loop"] else DIM)
        row(11, "ECU sync", "locked" if t["synced"] else "NO SYNC", GREEN if t["synced"] else RED)
        row(12, "TunerStudio", f"port {t['ts_port']}" if t["ts_port"] else "-", DIM)
        ratio = perf["ratio"]
        rc = GREEN if ratio > 0.95 else (AMBER if ratio > 0.6 else RED)
        row(13, "sim speed", f"{ratio:.2f}x  (head {perf['headroom']:.1f}x)", rc)
        row(14, "fps", f"{fps:.0f}")

    def _scope(self, x, y, w, h, scope_fn):
        self.panel(x, y, w, h)
        win = WINDOWS[self.win_idx]
        self.text(f"SCOPE  \u2014  last {win:.0f}s   ([ ] to change)", x + 12, y + 8, DIM, 12, "nw", bold=True)
        self._checkboxes = []
        cxp = x + 230
        for key, label, color, scale, unit in SCOPE_CHANNELS:
            box = pygame.Rect(cxp, y + 9, 12, 12)
            on = key in self.enabled
            pygame.draw.rect(self.scr, color if on else PANEL_HI, box, border_radius=2)
            pygame.draw.rect(self.scr, color, box, 1, border_radius=2)
            self.text(label, cxp + 16, y + 8, INK if on else DIM, 11, "nw")
            lw = self.font(11).size(label)[0]
            self._checkboxes.append((pygame.Rect(cxp, y + 8, 16 + lw, 14), key))
            cxp += 16 + lw + 14

        px, py, pw, ph = x + 14, y + 28, w - 28, h - 38
        pygame.draw.rect(self.scr, (14, 16, 19), (px, py, pw, ph), border_radius=4)
        for f in (0.25, 0.5, 0.75):
            pygame.draw.line(self.scr, (30, 34, 40), (px, py + ph * f), (px + pw, py + ph * f))
        keys = [c[0] for c in SCOPE_CHANNELS if c[0] in self.enabled]
        if not keys:
            return
        t_rel, series = scope_fn(win, keys)
        if len(t_rel) < 2:
            return
        span = max(1e-3, -t_rel[0])
        scale = {c[0]: c[3] for c in SCOPE_CHANNELS}
        color = {c[0]: c[2] for c in SCOPE_CHANNELS}
        for key in keys:
            pts = []
            sc = scale[key]
            for tr, val in zip(t_rel, series[key]):
                sx = px + pw * (1 + tr / span)
                sy = py + ph - ph * max(0.0, min(1.0, val / sc))
                pts.append((sx, sy))
            if len(pts) > 1:
                pygame.draw.lines(self.scr, color[key], False, pts, 2)
