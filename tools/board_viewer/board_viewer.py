#!/usr/bin/env python3
"""Interactive pygame board viewer for dsn_to_json.py output.

Usage:
    python3 board_viewer.py board.json                 # interactive window
    python3 board_viewer.py board.json --png out.png   # headless render

Controls
    left-drag ............... pan
    mouse wheel ............. zoom (around cursor)
    double-click / F ........ fit all
    R ....................... reset view
    1..9 .................... toggle layer <n> visibility
    0 ....................... show all layers
    Up / Down ............... step through nets (isolate highlight)
    Enter / Esc ............. clear net selection
    H ....................... toggle HUD
    S ....................... toggle auto-hide of other nets while selected

Requires pygame (stdlib only otherwise):
    python3 -m pip install --target ~/pygame_pkgs pygame
    PYTHONPATH=~/pygame_pkgs python3 board_viewer.py board.json
"""

import argparse
import json
import os
import sys

try:
    import pygame
except ImportError:
    print("error: pygame is required. Install it with:", file=sys.stderr)
    print("  python3 -m pip install --target ~/pygame_pkgs pygame", file=sys.stderr)
    print("then run with:  PYTHONPATH=~/pygame_pkgs {0} <board.json>".format(sys.argv[0]),
          file=sys.stderr)
    sys.exit(1)


# --------------------------------------------------------------------------
# data preparation (shared shape with html_viewer.py)
# --------------------------------------------------------------------------

def net_color(index, net_hue):
    return net_hue[index % len(net_hue)] if 0 <= index < len(net_hue) else 200


def prepare(json_data):
    board = json_data
    layers = [
        {"name": l["name"], "index": l.get("index", i), "signal": l.get("signal", True)}
        for i, l in enumerate(board.get("layers", []))
    ]
    layer_of = {l["name"]: idx for idx, l in enumerate(layers)}

    nets = [{"id": n["id"], "name": n["name"]} for n in board.get("nets", [])]
    net_of = {n["name"]: i for i, n in enumerate(nets)}

    wires = []
    for w in board.get("wires", []):
        layer_name = w.get("layer", "")
        if layer_name not in layer_of:
            layer_of[layer_name] = len(layers)
            layers.append({"name": layer_name, "index": len(layers), "signal": True})
        li = layer_of[layer_name]
        ni = -1
        for name in w.get("net_names", []):
            if name in net_of:
                ni = net_of[name]
                break
        coords = [float(v) for v in w.get("coords", [])]
        xs = coords[0::2]
        ys = coords[1::2]
        width = float(w.get("width", 0.0))
        fixed = bool(w.get("fixed"))
        if len(xs) >= 2 and len(xs) == len(ys):
            wires.append({"l": li, "n": ni, "x": xs, "y": ys, "w": width,
                          "f": 1 if fixed else 0})

    vias = []
    for v in board.get("via_placements", []):
        loc = v.get("location", [])
        if len(loc) < 2:
            continue
        ni = -1
        for name in v.get("net_names", []):
            if name in net_of:
                ni = net_of[name]
                break
        vias.append({"x": float(loc[0]), "y": float(loc[1]), "n": ni,
                     "pad": v.get("padstack_name", "")})

    pins = []
    for p in board.get("pins", []):
        if not p.get("resolved", False):
            continue
        ni = -1
        if p.get("net_no"):
            for i, n in enumerate(nets):
                if n["id"] == p["net_no"]:
                    ni = i
                    break
        pins.append({"x": float(p["board_x"]), "y": float(p["board_y"]),
                     "n": ni, "comp": p.get("component", ""), "pin": p.get("pin", "")})

    outlines = []
    box = board.get("board_box", {})
    if box and box.get("width"):
        outlines.append({"points": [
            [box["min_x"], box["min_y"]], [box["min_x"] + box["width"], box["min_y"]],
            [box["min_x"] + box["width"], box["min_y"] + box["height"]],
            [box["min_x"], box["min_y"] + box["height"]]]})
    for bnd in board.get("boundaries", []):
        points = bnd.get("points", [])
        if len(points) >= 3:
            outlines.append({"points": points})

    return {
        "layers": layers,
        "nets": nets,
        "wires": wires,
        "vias": vias,
        "pins": pins,
        "outlines": outlines,
        "box": box,
    }


def hash_hue(name):
    h = 0
    for ch in name:
        h = (h * 31 + ord(ch)) & 0x7FFFFFFF
    return h % 360


# --------------------------------------------------------------------------
# viewer
# --------------------------------------------------------------------------

class Viewer:
    def __init__(self, data):
        self.data = data
        self.layers = data["layers"]
        self.nets = data["nets"]
        self.wires = data["wires"]
        self.vias = data["vias"]
        self.pins = data["pins"]
        self.outlines = data["outlines"]

        self.layer_visible = [True] * len(self.layers)
        self.net_hue = [hash_hue(n["name"] if n.get("name") else "net%d" % i)
                        for i, n in enumerate(self.nets)]
        self.selected = -1            # selected net index
        self.isolate = True           # hide others when a net is selected
        self.show_hud = True

        self.vp = {"x": 0.0, "y": 0.0, "scale": 1.0}
        self.drag = None

        self.win = None
        self.font = None
        self.small_font = None

    # ---- coordinate helpers -------------------------------------------
    def world_to_screen(self, wx, wy, w, h):
        return ((wx - self.vp["x"]) * self.vp["scale"] + w / 2,
                (wy - self.vp["y"]) * self.vp["scale"] + h / 2)

    def screen_to_world(self, sx, sy, w, h):
        return ((sx - w / 2) / self.vp["scale"] + self.vp["x"],
                (sy - h / 2) / self.vp["scale"] + self.vp["y"])

    def fit_all(self):
        min_x, min_y, max_x, max_y = None, None, None, None

        def acc(x, y):
            nonlocal min_x, min_y, max_x, max_y
            for v in (x, y):
                if v is None or v != v:  # skip NaN
                    return
            if min_x is None or x < min_x:
                min_x = x
            if min_y is None or y < min_y:
                min_y = y
            if max_x is None or x > max_x:
                max_x = x
            if max_y is None or y > max_y:
                max_y = y

        for o in self.outlines:
            for p in o["points"]:
                acc(p[0], p[1])
        for p in self.pins:
            acc(p["x"], p["y"])
        for v in self.vias:
            acc(v["x"], v["y"])
        for w in self.wires:
            for i in range(len(w["x"])):
                acc(w["x"][i], w["y"][i])

        w = self.win.get_width()
        h = self.win.get_height()
        if min_x is None:
            self.vp = {"x": 0.0, "y": 0.0, "scale": 1.0}
            return
        bw = max_x - min_x
        bh = max_y - min_y
        pad = 40.0
        scale = min(w / (bw + 2 * pad), h / (bh + 2 * pad)) if bw and bh else 1.0
        scale = max(scale, 1e-9)
        # centre the board box
        cx = (min_x + max_x) / 2
        cy = (min_y + max_y) / 2
        self.vp = {"x": cx - w / (2 * scale), "y": cy - h / (2 * scale), "scale": scale}

    # ---- colour --------------------------------------------------------
    def hsl(self, hue, sat, light):
        c = pygame.Color(0, 0, 0, 0)
        c.hsla = (hue % 360, sat, light, 100)
        return c

    def net_color_rgb(self, index, light=58):
        if index < 0:
            return (0x88, 0x92, 0xA0)
        hue = self.net_hue[index]
        return [int(c) for c in self.hsl(hue, 60, light)]

    # ---- drawing -------------------------------------------------------
    def draw(self):
        win = self.win
        w, h = win.get_width(), win.get_height()
        win.fill((0x0A, 0x0E, 0x11))

        # board outline
        for o in self.outlines:
            pts = [self.world_to_screen(p[0], p[1], w, h) for p in o["points"]]
            if len(pts) < 2:
                continue
            pygame.draw.polygon(win, (0x33, 0x41, 0x4D), pts, 1)

        # traces
        for tr in self.wires:
            if not self.layer_visible[tr["l"]]:
                continue
            if self.selected >= 0 and self.isolate and tr["n"] != self.selected:
                continue
            color = self.net_color_rgb(tr["n"])
            if self.selected >= 0 and tr["n"] != self.selected:
                color = (color[0] // 4, color[1] // 4, color[2] // 4)
            pts = [self.world_to_screen(tr["x"][i], tr["y"][i], w, h)
                   for i in range(len(tr["x"]))]
            lw = int(max(tr["w"], 60) * 0.5 * self.vp["scale"])
            lw = max(1, min(lw, 24))
            if len(pts) >= 2:
                pygame.draw.lines(win, color, False, pts, lw)

        # vias
        for v in self.vias:
            if self.selected >= 0 and self.isolate and v["n"] != self.selected:
                continue
            sx, sy = self.world_to_screen(v["x"], v["y"], w, h)
            r = max(int(6 * self.vp["scale"]), 2)
            pygame.draw.circle(win, self.net_color_rgb(v["n"]), (int(sx), int(sy)), r)
            pygame.draw.circle(win, (0x10, 0x14, 0x18), (int(sx), int(sy)),
                               max(int(r * 0.45), 1))

        # pins
        for p in self.pins:
            if self.selected >= 0 and self.isolate and p["n"] != self.selected:
                continue
            sx, sy = self.world_to_screen(p["x"], p["y"], w, h)
            s = max(int(4 * self.vp["scale"]), 2)
            rect = pygame.Rect(int(sx - s), int(sy - s), 2 * s, 2 * s)
            pygame.draw.rect(win, (0xC8, 0xD2, 0xDC), rect)

        if self.show_hud:
            self.draw_hud(w, h)

    def draw_hud(self, w, h):
        f = self.font
        s = self.small_font
        scr = self.win
        lines = []
        lines.append("nets %d   traces %d   vias %d   pins %d"
                     % (len(self.nets), len(self.wires), len(self.vias), len(self.pins)))
        box = self.data["box"] or {}
        if box and box.get("width"):
            lines.append("board %.2f x %.2f  (%.0f..%.0f, %.0f..%.0f)"
                         % (box["width"] / 1000, box["height"] / 1000,
                            box["min_x"], box["min_x"] + box["width"],
                            box["min_y"], box["min_y"] + box["height"]))
        self.cursor_world = getattr(self, "cursor_world", None)
        if self.cursor_world is not None:
            lines.append("cursor %.1f, %.1f" % self.cursor_world)
        if self.selected >= 0:
            lines.append("net[%d/%d] %s" % (self.selected + 1, len(self.nets),
                                            self.nets[self.selected]["name"]))
            lines.append("  isolate: %s (S)" % ("on" if self.isolate else "off"))
        else:
            lines.append("net: none (Up/Down to pick)")

        hint = "drag pan | wheel zoom | F fit | R reset | 1..9 layers | H hud"
        y = 6
        for ln in lines:
            surf = f.render(ln, True, (0xDF, 0xE5, 0xEA))
            scr.blit(surf, (8, y))
            y += 16
        hs = s.render(hint, True, (0x8F, 0xA5, 0xB5))
        scr.blit(hs, (8, h - hs.get_height() - 6))

    # ---- events --------------------------------------------------------
    def handle_event(self, ev):
        w, h = self.win.get_width(), self.win.get_height()
        if ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
            self.drag = (ev.pos[0], ev.pos[1], self.vp["x"], self.vp["y"])
        elif ev.type == pygame.MOUSEBUTTONUP and ev.button == 1:
            self.drag = None
        elif ev.type == pygame.MOUSEMOTION and self.drag:
            dx = ev.pos[0] - self.drag[0]
            dy = ev.pos[1] - self.drag[1]
            self.vp["x"] = self.drag[2] - dx / self.vp["scale"]
            self.vp["y"] = self.drag[3] - dy / self.vp["scale"]
        elif ev.type == pygame.MOUSEMOTION:
            self.cursor_world = self.screen_to_world(ev.pos[0], ev.pos[1], w, h)
        elif ev.type == pygame.MOUSEWHEEL:
            mx, my = pygame.mouse.get_pos()
            wx, wy = self.screen_to_world(mx, my, w, h)
            factor = 1.18 ** ev.y
            self.vp["scale"] = max(1e-9, self.vp["scale"] * factor)
            self.vp["x"] = wx - (mx - w / 2) / self.vp["scale"]
            self.vp["y"] = wy - (my - h / 2) / self.vp["scale"]
        elif ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 3:
            self.selected = -1
        elif ev.type == pygame.KEYDOWN:
            k = ev.key
            if k == pygame.K_f:
                self.fit_all()
            elif k == pygame.K_r:
                self.fit_all()
            elif k == pygame.K_h:
                self.show_hud = not self.show_hud
            elif k == pygame.K_s:
                self.isolate = not self.isolate
            elif k == pygame.K_UP:
                if len(self.nets):
                    self.selected = -1 if self.selected >= len(self.nets) - 1 \
                        else (self.selected + 1 if self.selected >= 0 else 0)
            elif k == pygame.K_DOWN:
                if len(self.nets):
                    self.selected = -1 if self.selected <= 0 \
                        else self.selected - 1
            elif k in (pygame.K_KP_ENTER, pygame.K_RETURN, pygame.K_ESCAPE):
                self.selected = -1
            elif pygame.K_1 <= k <= pygame.K_9:
                idx = k - pygame.K_1
                if idx < len(self.layer_visible):
                    self.layer_visible[idx] = not self.layer_visible[idx]
            elif k == pygame.K_0:
                self.layer_visible = [True] * len(self.layer_visible)


# --------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------

def load_board(path):
    with open(path, "r", encoding="utf-8") as fh:
        board = json.load(fh)
    if isinstance(board, list) and board and "board_name" in board[0]:
        board = board[0]
    return board


def run(viewer, width=1400, height=900):
    pygame.init()
    pygame.display.set_caption("freerouting board viewer")
    screen = pygame.display.set_mode((width, height))
    viewer.win = screen
    viewer.font = pygame.font.SysFont("dejavusansmono, monospace", 14)
    viewer.small_font = pygame.font.SysFont("dejavusansmono, monospace", 12)
    viewer.fit_all()
    running = True
    while running:
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                running = False
            elif ev.type == pygame.WINDOWRESIZED:
                screen = pygame.display.set_mode(ev.size)
                viewer.win = screen
            else:
                viewer.handle_event(ev)
        viewer.draw()
        pygame.display.flip()
        pygame.time.wait(8)
    pygame.quit()


def render_png(viewer, path, width=1400, height=900):
    os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
    pygame.init()
    screen = pygame.display.set_mode((width, height))
    viewer.win = screen
    viewer.font = pygame.font.SysFont("dejavusansmono", 14)
    viewer.small_font = pygame.font.SysFont("dejavusansmono", 12)
    viewer.fit_all()
    viewer.draw()
    pygame.image.save(screen, path)
    pygame.quit()
    print("wrote", path)


def main(argv=None):
    ap = argparse.ArgumentParser(description="pygame board viewer for dsn_to_json output.")
    ap.add_argument("input", help="path to a dsn_to_json.py JSON document")
    ap.add_argument("--png", default=None, help="render a single frame to a PNG and exit "
                                                "(headless, no window)")
    ap.add_argument("--width", type=int, default=1400)
    ap.add_argument("--height", type=int, default=900)
    args = ap.parse_args(argv)

    board = load_board(args.input)
    data = prepare(board)
    viewer = Viewer(data)

    if args.png:
        render_png(viewer, args.png, args.width, args.height)
    else:
        run(viewer, args.width, args.height)
    return 0


if __name__ == "__main__":
    sys.exit(main())