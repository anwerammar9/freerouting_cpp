#!/usr/bin/env python3
"""Write a Specctra DSN from the JSON produced by dsn_to_json.py.

Usage:
    python3 json_to_dsn.py board.json -o board.rt.dsn     # write DSN
    python3 json_to_dsn.py board.json --compare            # write DSN, re-parse it,
                                                           # and diff against the input JSON
    python3 json_to_dsn.py -j a.json -d b.dsn --diff       # convert b.dsn, diff a vs b

The writer reproduces the scopes dsn_to_json.py understands (structure, library,
network, placement, wiring) so the two tools round-trip: DSN -> JSON -> DSN' -> JSON'
with JSON' == JSON (float tolerance). Intended for debugging the migration toolchain,
not as a full-featured Specctra emitter.
"""

import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import dsn_to_json  # noqa: E402


# ---------------------------------------------------------------------------
# number / string formatting helpers
# ---------------------------------------------------------------------------

def fmt_num(v):
    if v is None:
        return "0"
    if isinstance(v, bool):
        return "1" if v else "0"
    if isinstance(v, int):
        return str(v)
    f = float(v)
    if f == int(f):
        return "{:.1f}".format(f)
    return repr(f)


_IDENT = re.compile(r"^[A-Za-z0-9_./:\-\[\]]+$")


def rt(name):
    """Return a DSN token for a symbol name, quoting only when necessary."""
    name = str(name)
    if name == "" or not _IDENT.match(name):
        return '"' + name + '"'
    return name


# ---------------------------------------------------------------------------
# serializer
# ---------------------------------------------------------------------------

class DsnWriter:
    def __init__(self, board):
        self.board = board
        self.out = []
        self._indent = 0

    def line(self, text=""):
        self.out.append("  " * self._indent + text)

    def scope(self, head, body):
        self.line("(" + head)
        self._indent += 1
        body()
        self._indent -= 1
        self.line(")")

    # ---- top level -------------------------------------------------------
    def write(self):
        name = self.board.get("board_name", "")
        pcb_head = 'PCB "{}"'.format(name) if name else "PCB"
        self.scope(pcb_head, self._write_pcb_body)

    def _write_pcb_body(self):
        self.scope("parser", self._parser_body)
        res = self.board.get("resolution", {}) or {}
        if res.get("unit"):
            self.line("(resolution {} {})".format(res["unit"],
                                                  fmt_num(res.get("factor", 1))))
        if self.board.get("boundaries") or self.board.get("vias") \
                or self.board.get("layers"):
            self.scope("structure", self.write_structure)
        placement = [c for c in self.board.get("components", [])
                     if c.get("placed", True)]
        if placement:
            self.scope("placement", self.write_placement)
        if self.board.get("padstacks") or self.board.get("images"):
            self.scope("library", self.write_library)
        if self.board.get("nets"):
            self.scope("network", self.write_network)
        if self.board.get("wires") or self.board.get("via_placements"):
            self.scope("wiring", self.write_wiring)

    def _parser_body(self):
        # the lone `"` line is the string-quote declaration character
        self.line('(string_quote ")')
        self.line("(space_in_quoted_tokens on)")
        self.line("(generated_by_freeroute)")

    # ---- structure --------------------------------------------------------
    def write_structure(self):
        for bnd in self.board.get("boundaries", []):
            self.write_boundary(bnd)
        for via in self.board.get("vias", []):
            self.write_via_def(via)
        for layer in self.board.get("layers", []):
            self._layer(layer)

    def write_boundary(self, bnd):
        shape = bnd.get("shape", "polygon")
        pts = bnd.get("points", [])
        if shape == "rect" and len(pts) >= 4:
            self.scope("boundary", lambda: self.line(
                "(rect pcb {} {} {} {})".format(fmt_num(pts[0][0]),
                                                fmt_num(pts[0][1]),
                                                fmt_num(pts[2][0]),
                                                fmt_num(pts[2][1]))))
        elif len(pts) >= 3:
            coords = " ".join("{} {}".format(fmt_num(x), fmt_num(y))
                              for x, y in pts)
            self.scope("boundary",
                       lambda c=coords: self.line("(polygon signal 0 " + c + ")"))

    def write_via_def(self, via):
        name = via.get("name", "")
        drill = via.get("drill", 0)
        if not drill:
            self.line("(via {})".format(rt(name)))
            return

        def body():
            self.line("(attach off)")
            self.line("(hole {})".format(fmt_num(drill)))

        self.scope("via {}".format(rt(name)), body)

    def _layer(self, layer):
        name = layer.get("name", "")
        typ = "signal" if layer.get("signal", True) else "mixed"

        def body():
            self.line("(type {})".format(typ))

        self.scope("layer {}".format(rt(name)), body)

    # ---- placement ---------------------------------------------------------
    def write_placement(self):
        for comp in self.board.get("components", []):
            if not comp.get("placed", True):
                continue
            fp = rt(comp.get("footprint") or comp.get("package") or "")
            self.line("(component {}".format(fp))
            self._indent += 1
            head = "(place {} {} {} {} {}".format(
                rt(comp.get("refdes", "")), fmt_num(comp.get("x", 0.0)),
                fmt_num(comp.get("y", 0.0)), comp.get("side", "front"),
                fmt_num(comp.get("rotation", 0.0)))
            self.line(head)
            self._indent += 1
            for p in comp.get("pin_references", []):
                self.line("(pin {})".format(rt(p)))
            self._indent -= 1
            self.line(")")
            self._indent -= 1
            self.line(")")

    # ---- library -----------------------------------------------------------
    def write_library(self):
        for ps in self.board.get("padstacks", []):
            self.write_padstack(ps)
        for img in self.board.get("images", []):
            self.write_image(img)

    def write_padstack(self, ps):
        name = rt(ps.get("name", ""))
        self.line("(padstack {}".format(name))
        self._indent += 1
        for sh in ps.get("shapes", []):
            self.line("(shape")
            self._indent += 1
            params = " ".join(fmt_num(p) for p in sh.get("params", []))
            self.line("({} {} {})".format(sh.get("kind", "polygon"),
                                          rt(sh.get("layer", "")), params))
            self._indent -= 1
            self.line(")")
        self.line("(attach off)")
        self._indent -= 1
        self.line(")")

    def write_image(self, img):
        name = rt(img.get("name", ""))
        self.line("(image {}".format(name))
        self._indent += 1
        self.line("(side {})".format(img.get("side", "front")))
        for p in img.get("pads", []):
            self.line("(pin {} {} {} {})".format(rt(p["padstack"]),
                                                 rt(p["number"]),
                                                 fmt_num(p["x"]),
                                                 fmt_num(p["y"])))
        self._indent -= 1
        self.line(")")

    # ---- network ------------------------------------------------------------
    def write_network(self):
        for net in self.board.get("nets", []):
            refs = net.get("pins", [])
            name = rt(net.get("name", ""))
            subnet = net.get("subnet")
            head = "net {}".format(name)
            if subnet is not None:
                head += " {}".format(fmt_num(subnet))
            if not refs:
                self.line("({})".format(head))
                continue
            self.line("(" + head)
            self._indent += 1
            self.line("(pins")
            self._indent += 1
            for p in refs:
                self.line(rt(p))
            self._indent -= 1
            self.line(")")
            self._indent -= 1
            self.line(")")

    def net_ref(self, net_names):
        if not net_names:
            return ""
        first = net_names[0]
        subnet = None
        for n in self.board.get("nets", []):
            if n["name"] == first:
                subnet = n.get("subnet")
                break
        if subnet is not None:
            return "(net {} {})".format(rt(first), fmt_num(subnet))
        return "(net {})".format(rt(first))

    # ---- wiring ------------------------------------------------------------
    def write_wiring(self):
        for w in self.board.get("wires", []):
            self.write_wire(w)
        for v in self.board.get("via_placements", []):
            self.write_via(v)

    def write_wire(self, w):
        arch = "polygon_path" if w.get("is_polygon_path") else "polyline_path"
        layer = w.get("layer", "")
        coords = w.get("coords", [])
        if len(coords) < 4:
            return
        width = fmt_num(w.get("width", 0.0))
        coord_txt = " ".join(fmt_num(p) for p in coords)

        def body():
            self.line("({} {} {} {})".format(arch, rt(layer), width, coord_txt))
            net = self.net_ref(w.get("net_names", []))
            if net:
                self.line(net)
            if w.get("clearance_class"):
                self.line("(clearance_class {})".format(rt(w["clearance_class"])))
            if w.get("fixed"):
                self.line("(type shove_fixed)")

        self.scope("wire", body)

    def write_via(self, v):
        pad = rt(v.get("padstack_name", ""))
        loc = v.get("location", [])
        if len(loc) < 2:
            return
        head = "via {} {} {}".format(pad, fmt_num(loc[0]), fmt_num(loc[1]))
        net = self.net_ref(v.get("net_names", []))
        cc = v.get("clearance_class")

        def body():
            if net:
                self.line(net)
            if cc:
                self.line("(clearance_class {})".format(rt(cc)))
            if v.get("fixed"):
                self.line("(type shove_fixed)")

        self.scope(head, body)


# ---------------------------------------------------------------------------
# round-trip compare
# ---------------------------------------------------------------------------

METADATA_KEYS = {"format", "source_file", "clean", "dropped_phantom_points"}


def node_diff(a, b, path="", tol=1e-6, report=None, stats=None):
    """Recursive diff with float tolerance."""
    if report is None:
        report = []
    if stats is None:
        stats = [0]
    stats[0] += 1

    if isinstance(a, dict) and isinstance(b, dict):
        missing = [k for k in a if k not in b and k not in METADATA_KEYS]
        extra = [k for k in b if k not in a and k not in METADATA_KEYS]
        if missing or extra:
            for k in missing:
                report.append(("missing-in-b", path + "/" + k))
            for k in extra:
                report.append(("extra-in-b", path + "/" + k))
            return False
        ok = True
        for k in sorted(a):
            if k in METADATA_KEYS:
                continue
            if not node_diff(a[k], b[k], path + "/" + k, tol, report, stats):
                ok = False
        return ok
    if isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            report.append(("length", "{}: {} vs {}".format(path, len(a), len(b))))
            return False
        ok = True
        for i, (x, y) in enumerate(zip(a, b)):
            if not node_diff(x, y, "{}[{}]".format(path, i), tol, report, stats):
                ok = False
        return ok
    if isinstance(a, float) and isinstance(b, float):
        return abs(a - b) <= tol * max(1.0, abs(a), abs(b))
    if a == b:
        return True
    report.append(("value", "{}: {!r} vs {!r}".format(path, a, b)))
    return False


def build_dsn(board):
    w = DsnWriter(board)
    w.write()
    return "\n".join(w.out)


def load_board(path):
    with open(path, "r", encoding="utf-8") as fh:
        data = json.load(fh)
    if isinstance(data, list) and data and "board_name" in data[0]:
        data = data[0]
    if not isinstance(data, dict) or "board_name" not in data:
        raise ValueError("not a dsn_to_json document (missing board_name)")
    return data


def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Write a DSN from dsn_to_json JSON, optionally verifying the "
                    "round-trip.")
    ap.add_argument("input", nargs="?", default=None, help="path to the JSON")
    ap.add_argument("-j", "--json", default=None,
                    help="path to the JSON (alternative to INPUT)")
    ap.add_argument("-o", "--output", default=None,
                    help="path for the generated DSN (default: <input>.rt.dsn)")
    ap.add_argument("--compare", action="store_true",
                    help="after writing, re-parse the DSN and diff against the JSON")
    ap.add_argument("-d", "--dsn", default=None,
                    help="exisisting DSN path to convert (with --diff)")
    ap.add_argument("--diff", action="store_true",
                    help="convert --dsn and diff the result against the JSON")
    ap.add_argument("--tolerance", type=float, default=1e-6,
                    help="relative float tolerance for compare")
    args = ap.parse_args(argv)

    json_path = args.json or args.input
    if not json_path:
        ap.error("JSON input required")

    board = load_board(json_path)

    if args.diff:
        if not args.dsn:
            ap.error("--diff requires --dsn <file.dsn>")
        reparsed = dsn_to_json.convert_file(args.dsn)
        stats = [0]
        report = []
        node_diff(board, reparsed, "", args.tolerance, report, stats)
        print("compare original JSON vs converted DSN: {} nodes".format(stats[0]))
        for kind, msg in report[:40]:
            print("  [{}] {}".format(kind, msg))
        if report:
            print("DIFFS: {}".format(len(report)))
        else:
            print("MATCH")
        return 0

    dsn_text = build_dsn(board)
    output = args.output
    if not output:
        base = json_path
        output = base[:-5] + ".rt.dsn" if base.lower().endswith(".json") \
            else base + ".rt.dsn"
    with open(output, "w", encoding="utf-8") as fh:
        fh.write(dsn_text + "\n")
    print("wrote {}".format(output))

    if args.compare:
        reparsed = dsn_to_json.convert_file(output)
        stats = [0]
        report = []
        node_diff(board, reparsed, "", args.tolerance, report, stats)
        print("round-trip compare: {} nodes".format(stats[0]))
        for kind, msg in report[:40]:
            print("  [{}] {}".format(kind, msg))
        if report:
            print("DIFFS: {}".format(len(report)))
            return 2
        print("MATCH")
    return 0


if __name__ == "__main__":
    sys.exit(main())