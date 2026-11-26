"""A pseudo-3D road scene for the driving sim: a road receding to the horizon, a dashed centre
line, roadside trees and oncoming traffic, all real-scale and projected with perspective so speed
reads from the scene. Runs in the GUI thread and keeps its own animation state, advanced by frame
dt and the current speed, so it is fully decoupled from the engine loop. pygame only.
"""
import random
import pygame

MPH_TO_MS = 0.44704
ROAD_HALF = 4.2          # m from centre to each road edge (two ~4 m lanes)
CAM_H = 1.25             # m eye height
NEAR, FAR = 4.0, 160.0   # m, visible road span
FOCAL = 360.0            # px

SKY_TOP = (44, 62, 92)
SKY_BOT = (120, 140, 168)
GROUND = (58, 74, 52)
ROAD = (54, 56, 60)
ROAD_EDGE = (210, 210, 210)
DASH = (226, 206, 120)
TRUNK = (84, 60, 40)
LEAF = (56, 120, 60)
LEAF2 = (44, 100, 50)


class RoadView:
    def __init__(self, seed=1):
        self.rng = random.Random(seed)
        self.trees = []          # [x_m, z_m, height_m, side_scale]
        self.traffic = []        # [x_m, z_m, colour, speed_ms]
        for _ in range(26):
            self.trees.append(self._new_tree(self.rng.uniform(NEAR, FAR)))
        for _ in range(5):
            self.traffic.append(self._new_car(self.rng.uniform(NEAR + 20, FAR)))

    def _new_tree(self, z):
        side = self.rng.choice((-1, 1))
        x = side * self.rng.uniform(ROAD_HALF + 1.5, ROAD_HALF + 9.0)
        return [x, z, self.rng.uniform(4.0, 7.5), self.rng.uniform(0.85, 1.2)]

    def _new_car(self, z):
        col = self.rng.choice([(200, 80, 80), (80, 120, 200), (210, 200, 90),
                               (210, 210, 220), (90, 170, 120)])
        return [-ROAD_HALF * 0.5, z, col, self.rng.uniform(11.0, 20.0)]

    def update(self, dt, speed_mph):
        v = speed_mph * MPH_TO_MS
        for t in self.trees:
            t[1] -= v * dt
            if t[1] < NEAR:
                t[:] = self._new_tree(FAR + self.rng.uniform(0, 20))
        for c in self.traffic:
            c[1] -= (v + c[3]) * dt            # closing speed
            if c[1] < NEAR:
                c[:] = self._new_car(FAR + self.rng.uniform(10, 40))

    def _project(self, x, z, cx, horizon):
        s = FOCAL / max(z, 0.5)
        return cx + x * s, horizon + CAM_H * s, s

    def draw(self, surf, rect, tel):
        x0, y0, w, h = rect
        cx = x0 + w / 2
        horizon = y0 + h * 0.30
        clip = surf.get_clip()
        surf.set_clip(pygame.Rect(rect))

        # sky gradient
        for i in range(int(horizon - y0)):
            f = i / max(1, (horizon - y0))
            col = tuple(int(a + (b - a) * f) for a, b in zip(SKY_TOP, SKY_BOT))
            pygame.draw.line(surf, col, (x0, y0 + i), (x0 + w, y0 + i))
        pygame.draw.rect(surf, GROUND, (x0, horizon, w, y0 + h - horizon))

        # road surface (trapezoid near->far)
        nl = self._project(-ROAD_HALF, NEAR, cx, horizon)
        nr = self._project(ROAD_HALF, NEAR, cx, horizon)
        fl = self._project(-ROAD_HALF, FAR, cx, horizon)
        fr = self._project(ROAD_HALF, FAR, cx, horizon)
        pygame.draw.polygon(surf, ROAD, [(nl[0], nl[1]), (nr[0], nr[1]), (fr[0], fr[1]), (fl[0], fl[1])])
        for edge in (-ROAD_HALF, ROAD_HALF):
            a = self._project(edge, NEAR, cx, horizon)
            b = self._project(edge, FAR, cx, horizon)
            pygame.draw.line(surf, ROAD_EDGE, (a[0], a[1]), (b[0], b[1]), max(1, int(a[2] * 0.06)))

        # dashed centre line, scrolling by distance
        off = tel["distance_m"] % 8.0
        z = NEAR + (8.0 - off)
        while z < FAR:
            a = self._project(0, z, cx, horizon)
            b = self._project(0, min(z + 3.0, FAR), cx, horizon)
            pygame.draw.line(surf, DASH, (a[0], a[1]), (b[0], b[1]), max(1, int(a[2] * 0.05)))
            z += 8.0

        # scene objects, far to near
        objs = [("tree", t) for t in self.trees] + [("car", c) for c in self.traffic]
        for kind, o in sorted(objs, key=lambda it: -it[1][1]):
            if kind == "tree":
                self._draw_tree(surf, o, cx, horizon)
            else:
                self._draw_car(surf, o, cx, horizon)

        self._draw_player(surf, x0, y0, w, h, tel)
        surf.set_clip(clip)

    def _draw_tree(self, surf, t, cx, horizon):
        x, z, height, sc = t
        base = self._project(x, z, cx, horizon)
        s = base[2]
        th = height * s
        tw = max(1, 0.4 * s)
        trunk_top = base[1] - th * 0.45
        pygame.draw.line(surf, TRUNK, (base[0], base[1]), (base[0], trunk_top), int(max(1, tw)))
        r = max(2, height * 0.5 * s * sc)
        pygame.draw.circle(surf, LEAF2, (int(base[0]), int(trunk_top)), int(r))
        pygame.draw.circle(surf, LEAF, (int(base[0] - r * 0.3), int(trunk_top - r * 0.2)), int(r * 0.7))

    def _draw_car(self, surf, c, cx, horizon):
        x, z, col, _ = c
        base = self._project(x, z, cx, horizon)
        s = base[2]
        cw = max(2, 1.8 * s)
        ch = max(2, 1.4 * s)
        rect = pygame.Rect(0, 0, cw, ch)
        rect.midbottom = (base[0], base[1])
        pygame.draw.rect(surf, col, rect, border_radius=max(1, int(ch * 0.15)))
        pygame.draw.rect(surf, (20, 20, 24), (rect.x, rect.y, cw, ch * 0.4), border_radius=int(ch * 0.1))
        for hx in (rect.x + cw * 0.2, rect.x + cw * 0.8):
            pygame.draw.circle(surf, (255, 230, 160), (int(hx), int(rect.bottom - ch * 0.2)), max(1, int(cw * 0.12)))

    def _draw_player(self, surf, x0, y0, w, h, tel):
        cx = x0 + w / 2
        by = y0 + h - 8
        bw, bh = 150, 46
        body = pygame.Rect(0, 0, bw, bh)
        body.midbottom = (cx, by)
        col = (70, 120, 210) if tel["gear"] != "R" else (170, 90, 90)
        # shadow
        pygame.draw.ellipse(surf, (20, 24, 20), (body.x - 8, by - 10, bw + 16, 20))
        pygame.draw.rect(surf, col, body, border_radius=12)
        # cabin
        cab = pygame.Rect(0, 0, bw * 0.62, bh * 0.7)
        cab.midbottom = (cx, body.top + 10)
        pygame.draw.rect(surf, (36, 44, 60), cab, border_radius=10)
        pygame.draw.rect(surf, (120, 160, 200), cab.inflate(-10, -14), border_radius=8)
        # tail lights
        for lx in (body.x + 14, body.right - 14):
            pygame.draw.circle(surf, (235, 70, 60) if tel["brake"] > 0 else (150, 40, 40),
                               (int(lx), int(body.bottom - 12)), 6)
        # wheels
        for wx in (body.x + 28, body.right - 28):
            pygame.draw.rect(surf, (18, 18, 20), (wx - 12, by - 8, 24, 12), border_radius=4)
