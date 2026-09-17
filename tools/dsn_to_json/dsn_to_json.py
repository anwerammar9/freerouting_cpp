#!/usr/bin/env python3
"""Convert a freerouting Specctra DSN file into a debug-friendly JSON document.

The JSON structure mirrors the C++ parse result (src/designforms/specctra/Scanner.h)
so it can be compared side by side with the C++ loader, plus a few conveniences
(resolved net names, derived trace segments, positioned pins).

Usage:
    python3 dsn_to_json.py <input.dsn> [-o output.json] [--stdout] [--minify]

JSON document layout:
    board_name, resolution {unit, factor}, board_box {min_x,min_y,width,height}
    boundaries   [{layer, shape, points[[x,y],...]}]
    layers       [{index,name,signal}]
    vias         [{name,drill,layer_indices}]
    padstacks    [{name,width,height}]
    images       [{name, pads[{padstack,number,x,y}]}]
    nets         [{id,name,class_index,pins[comp-pin,...]}]
    pins         [{component,pin,net_no,net_name,x,y,board_x,board_y,resolved}]
    components   [{footprint,refdes,package,x,y,side,rotation,placed}]
    wires        [{layer,layer_index,width,coords,segments,net_nos,net_names,
                   fixed,is_polygon_path,is_conduction_area,clearance_class}]
    via_placements [{padstack_name,location[x,y],net_nos,net_names,fixed,clearance_class}]
"""

import argparse
import json
import math
import os
import sys


# ---------------------------------------------------------------------------
# S-expression tokenizer / parser (pure standard library)
# ---------------------------------------------------------------------------

def tokenize(text):
    """Split DSN text into tokens: '(' , ')' , ('s', str) and ('n', number)."""
    tokens = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c in " \t\r\n":
            i += 1
        elif c == ";":  # line comment
            while i < n and text[i] != "\n":
                i += 1
        elif c == "(":
            tokens.append("(")
            i += 1
        elif c == ")":
            tokens.append(")")
            i += 1
        elif c == '"':
            i += 1
            if i < n and text[i] == ')':
                # lone quote character token, e.g. the (string_quote ") declaration
                tokens.append(("s", '"'))
                continue
            buf = []
            while i < n and text[i] != '"':
                buf.append(text[i])
                i += 1
            i += 1
            tokens.append(("s", "".join(buf)))
        else:
            j = i
            while j < n and text[j] not in "()\" \t\r\n;":
                j += 1
            word = text[i:j]
            try:
                if any(ch in word for ch in ".eE"):
                    tokens.append(("n", float(word)))
                else:
                    tokens.append(("n", int(word)))
            except ValueError:
                tokens.append(("s", word))
            i = j
    return tokens


def parse_sexp(tokens):
    """Build a nested list tree from the token stream."""

    def parse_list(it):
        node = []
        for t in it:
            if t == "(":
                node.append(parse_list(it))
            elif t == ")":
                return node
            else:
                node.append(t[1])
        raise ValueError("unbalanced parentheses in DSN file")

    it = iter(tokens)
    top = []
    for t in it:
        if t == "(":
            top.append(parse_list(it))
        else:
            raise ValueError("unexpected token at top level in DSN file")
    return top


# ---------------------------------------------------------------------------
# Converter
# ---------------------------------------------------------------------------

def kw(node):
    return node[0].lower() if node and isinstance(node[0], str) else None


def num(v):
    return float(v) if isinstance(v, (int, float)) else 0.0


def norm_pin(p):
    p = str(p)
    try:
        return str(int(p))
    except ValueError:
        return p.strip()


class DsnConverter:
    def __init__(self, source):
        self.source = source
        self.board = {
            "format": "freerouting-dsn-json",
            "source_file": os.path.basename(source),
            "board_name": "",
            "resolution": {"unit": "", "factor": 1},
            "board_box": {"min_x": 0, "min_y": 0, "width": 0, "height": 0},
            "boundaries": [],
            "layers": [],
            "vias": [],
            "padstacks": [],
            "images": [],
            "nets": [],
            "pins": [],
            "components": [],
            "wires": [],
            "via_placements": [],
        }

    # -- boxes -------------------------------------------------------------
    def expand_box(self, min_x, min_y, max_x, max_y):
        box = self.board["board_box"]
        if box["width"] <= 0 or box["height"] <= 0:
            box["min_x"], box["min_y"] = min_x, min_y
            box["width"], box["height"] = max_x - min_x, max_y - min_y
        else:
            box["min_x"] = min(box["min_x"], min_x)
            box["min_y"] = min(box["min_y"], min_y)
            box["width"] = max(box["min_x"] + box["width"], max_x) - box["min_x"]
            box["height"] = max(box["min_y"] + box["height"], max_y) - box["min_y"]

    def layer_index(self, name):
        for layer in self.board["layers"]:
            if layer["name"] == name:
                return layer["index"]
        return -1

    # -- scopes ------------------------------------------------------------
    def read_boundary(self, node):
        shape_name = None
        pts = []
        for sub in node[1:]:
            if isinstance(sub, list):
                k = kw(sub)
                nums = [v for v in sub if isinstance(v, (int, float))]
                if k == "rect" and len(nums) >= 4:
                    shape_name = "rect"
                    x1, y1, x2, y2 = nums[0], nums[1], nums[2], nums[3]
                    pts = [[x1, y1], [x2, y1], [x2, y2], [x1, y2]]
                    self.expand_box(x1, y1, x2, y2)
                elif k == "polygon":
                    shape_name = "polygon"
                    layer_name = sub[1] if len(sub) > 1 and isinstance(sub[1], str) else ""
                    vals = [v for v in sub[2:] if isinstance(v, (int, float))]
                    if vals and isinstance(vals[0], int) and len(vals) % 2 == 1:
                        vals = vals[1:]  # drop the polygon point count
                    pts = [[vals[i], vals[i + 1]] for i in range(0, len(vals) - 1, 2)]
                    if pts:
                        xs = [p[0] for p in pts]
                        ys = [p[1] for p in pts]
                        self.expand_box(min(xs), min(ys), max(xs), max(ys))
                elif k == "path":
                    shape_name = "path"
                    pts = []
                    vals = [v for v in sub[3:] if isinstance(v, (int, float))]
                    for i in range(0, len(vals) - 1, 2):
                        pts.append([vals[i], vals[i + 1]])
                    if pts:
                        xs = [p[0] for p in pts]
                        ys = [p[1] for p in pts]
                        self.expand_box(min(xs), min(ys), max(xs), max(ys))
        if shape_name:
            self.board["boundaries"].append(
                {"layer": "pcb", "shape": shape_name, "points": pts})

    def read_layer(self, node):
        if len(node) < 2:
            return
        name = node[1]
        if not isinstance(name, str):
            name = ""
        is_signal = True
        for sub in node[2:]:
            if isinstance(sub, list) and kw(sub) == "type" and len(sub) >= 2:
                is_signal = (str(sub[1]).lower() == "signal")
        self.board["layers"].append(
            {"index": len(self.board["layers"]), "name": name, "signal": is_signal})

    def read_via_padstack(self, node):
        name = node[1] if len(node) > 1 else ""
        drill = 0
        layer_indices = []
        for sub in node[2:]:
            if isinstance(sub, list):
                if kw(sub) == "hole" and len(sub) >= 2:
                    drill = int(num(sub[1]))
            elif isinstance(sub, int):
                layer_indices.append(sub)
        self.board["vias"].append(
            {"name": str(name), "drill": drill, "layer_indices": layer_indices})

    def read_structure(self, node):
        for sub in node[1:]:
            if not isinstance(sub, list):
                continue
            k = kw(sub)
            if k == "layer":
                self.read_layer(sub)
            elif k == "via":
                self.read_via_padstack(sub)
            elif k == "boundary":
                self.read_boundary(sub)
            # control / rule / snap_angle / autoroute_settings are ignored.

    def shape_size(self, node):
        """Return (width, height) measured from one padstack shape node."""
        k = kw(node)
        nums = [v for v in node if isinstance(v, (int, float))]
        if k == "circle" and len(nums) >= 3:
            return nums[2], nums[2]
        if k == "rect" and len(nums) >= 4:
            return abs(nums[2] - nums[0]), abs(nums[3] - nums[1])
        if k == "polygon" and nums:
            if len(nums) % 2 == 1:
                nums = nums[1:]  # drop the point-count header
            xs = nums[::2]
            ys = nums[1::2]
            if xs and ys:
                return max(xs) - min(xs), max(ys) - min(ys)
        if k == "ellipse" and len(nums) >= 3:
            return nums[1], nums[2]
        return 0.0, 0.0

    def read_library(self, node):
        for sub in node[1:]:
            if not isinstance(sub, list) or kw(sub) not in ("image", "padstack"):
                continue
            if kw(sub) == "padstack":
                self.read_padstack(sub)
                continue
            image = {"name": str(sub[1]) if len(sub) > 1 else "",
                     "side": "front", "pads": []}
            for item in sub[2:]:
                if isinstance(item, list) and kw(item) == "side" and len(item) >= 2:
                    image["side"] = str(item[1]).lower()
                elif isinstance(item, list) and kw(item) == "pin" and len(item) >= 4:
                    image["pads"].append(
                        {"padstack": str(item[1]), "number": str(item[2]),
                         "x": num(item[3]), "y": num(item[4])})
            self.board["images"].append(image)

    def read_padstack(self, item):
        """Record padstack outer-width/height taken from its shape scopes."""
        w = h = 0.0
        shapes = []
        name = str(item[1]) if len(item) > 1 else ""
        for shape in item[2:]:
            if isinstance(shape, list) and kw(shape) == "shape":
                for sh in shape[1:]:
                    if isinstance(sh, list):
                        sw, shh = self.shape_size(sh)
                        w = max(w, sw)
                        h = max(h, shh)
                        shapes.append({
                            "kind": kw(sh),
                            "layer": str(sh[1]) if len(sh) > 1
                                     and isinstance(sh[1], str) else "",
                            "params": [v for v in sh if isinstance(v, (int, float))],
                        })
        self.board["padstacks"].append(
            {"name": name, "width": w, "height": h, "shapes": shapes})

    def read_net(self, node):
        """Read a single (net NAME [no] (pins ...)) entry."""
        if len(node) < 2 or not isinstance(node[1], str):
            return
        name = node[1]
        pins = []
        subnet = None
        for item in node[2:]:
            if isinstance(item, (int, float)):
                subnet = item
            elif isinstance(item, list) and kw(item) == "pins":
                pins = [str(p) for p in item[1:] if isinstance(p, str)]
            # class / rule scopes ignored
        net = {
            "id": len(self.board["nets"]) + 1,
            "name": name,
            "subnet": subnet,
            "class_index": 0,
            "pins": pins,
        }
        self.board["nets"].append(net)
        for pin_ref in pins:
            comp, _, pin_name = pin_ref.partition("-")
            self.board["pins"].append({
                "component": comp, "pin": pin_name, "net_no": net["id"],
                "net_name": name, "x": 0, "y": 0,
                "board_x": 0, "board_y": 0, "resolved": False,
            })

    def read_network(self, node):
        if kw(node) == "network":
            for sub in node[1:]:
                if isinstance(sub, list) and kw(sub) == "net":
                    self.read_net(sub)
        elif kw(node) == "net":
            self.read_net(node)

    def read_placement(self, node):
        for sub in node[1:]:
            if not isinstance(sub, list):
                continue
            k = kw(sub)
            if k == "component" or k == "package":
                footprint = str(sub[1]) if len(sub) > 1 else ""
                package = footprint
                for item in sub[2:]:
                    if not isinstance(item, list):
                        continue
                    ik = kw(item)
                    if ik == "component" or ik == "package":
                        self.read_placement([item])
                    elif ik == "place":
                        place = self.read_place(item, footprint)
                        if place:
                            place["package"] = package
                            self.board["components"].append(place)

    def read_place(self, node, footprint):
        """Handle the flat KT form and the named Electra-style form."""
        toks = node[1:]
        if not toks:
            return None
        refdes = None
        x = y = rotation = 0.0
        side = "front"
        pins = []
        index = 0

        if isinstance(toks[0], str):
            refdes = toks[0]
            index = 1
            nums = []
            while index < len(toks) and isinstance(toks[index], (int, float)):
                nums.append(num(toks[index]))
                index += 1
            if len(nums) >= 2:
                x, y = nums[0], nums[1]
            if index < len(toks) and isinstance(toks[index], str):
                if toks[index].lower() in ("front", "back"):
                    side = toks[index].lower()
                    index += 1
                    if index < len(toks) and isinstance(toks[index], (int, float)):
                        rotation = num(toks[index])
                elif len(nums) >= 3:
                    rotation = nums[2]
            elif len(nums) >= 3:
                rotation = nums[2]

        for item in toks[index:]:
            if not isinstance(item, list):
                continue
            ik = kw(item)
            if ik == "xy" and len(item) >= 3:
                x, y = num(item[1]), num(item[2])
            elif ik == "rotate" and len(item) >= 2:
                rotation = num(item[1])
            elif ik == "side" and len(item) >= 2:
                side = str(item[1]).lower()
            elif ik == "pin" and len(item) >= 2:
                pins.append(str(item[1]))
            elif ik == "pn" and len(item) >= 2 and not refdes:
                refdes = str(item[1])

        return {
            "footprint": footprint,
            "refdes": refdes,
            "package": footprint,
            "x": x, "y": y,
            "side": side, "rotation": rotation,
            "placed": True,
            "pin_references": pins,
        }

    # -- wiring ------------------------------------------------------------
    def read_path(self, node):
        """Return (layer_name, width, coords, is_polygon) for a path node."""
        layer = str(node[1]) if len(node) > 1 else ""
        vals = [num(v) for v in node[2:] if isinstance(v, (int, float))]
        width = vals[0] if vals else 0.0
        coords = vals[1:] if len(vals) > 1 else []
        if len(coords) % 2 == 1:
            coords = coords[:-1]
        return layer, width, coords, kw(node) == "polygon_path"

    def read_net_scope(self, node):
        """Return (name, subnet) for a (net ...) scope."""
        if len(node) < 2:
            return "", None
        subnet = None
        for v in node[2:]:
            if isinstance(v, (int, float)):
                subnet = v
        return str(node[1]), subnet

    def read_wire(self, node):
        wire = {
            "layer": "", "layer_index": -1, "width": 0.0, "coords": [],
            "segments": [], "net_nos": [], "net_names": [], "fixed": False,
            "is_polygon_path": False, "is_conduction_area": False,
            "clearance_class": 0, "shape": None,
        }
        for sub in node[1:]:
            if not isinstance(sub, list):
                continue
            k = kw(sub)
            if k in ("path", "polyline_path", "polygon_path"):
                layer, width, coords, poly = self.read_path(sub)
                wire["layer"] = layer
                wire["width"] = width
                wire["coords"] = coords
                wire["is_polygon_path"] = poly
                wire["layer_index"] = self.layer_index(layer)
            elif k in ("rect", "rectangle", "polygon", "circle"):
                wire["is_conduction_area"] = True
                wire["shape"] = k
            elif k == "net":
                name, subnet = self.read_net_scope(sub)
                if name:
                    wire["net_names"].append(name)
            elif k == "clearance_class" and len(sub) >= 2:
                wire["clearance_class"] = str(sub[1])
            elif k == "type" and len(sub) >= 2:
                wire["fixed"] = str(sub[1]).lower() != "normal"
        coords = wire["coords"]
        wire["segments"] = [
            [coords[i], coords[i + 1], coords[i + 2], coords[i + 3]]
            for i in range(0, len(coords) - 3, 2)
        ]
        self.resolve_net_names(wire)
        return wire

    def read_via_placement(self, node):
        via = {
            "padstack_name": str(node[1]) if len(node) > 1 else "",
            "location": [],
            "net_nos": [], "net_names": [], "fixed": False,
            "clearance_class": 0,
        }
        loc = [num(v) for v in node[2:] if isinstance(v, (int, float))]
        # first two numbers are the placement
        via["location"] = loc[:2] if len(loc) >= 2 else []
        for sub in node[2:]:
            if not isinstance(sub, list):
                continue
            k = kw(sub)
            if k == "net":
                name, subnet = self.read_net_scope(sub)
                if name:
                    via["net_names"].append(name)
            elif k == "clearance_class" and len(sub) >= 2:
                via["clearance_class"] = str(sub[1])
            elif k == "type" and len(sub) >= 2:
                via["fixed"] = str(sub[1]).lower() != "normal"
        self.resolve_net_names(via)
        return via

    def read_wiring(self, node):
        for sub in node[1:]:
            if not isinstance(sub, list):
                continue
            k = kw(sub)
            if k == "wire":
                self.board["wires"].append(self.read_wire(sub))
            elif k == "via":
                self.board["via_placements"].append(self.read_via_placement(sub))
            # conduction_area / window / rules ignored

    def resolve_net_names(self, item):
        for net in self.board["nets"]:
            if net["name"] in item["net_names"] and net["id"] not in item["net_nos"]:
                item["net_nos"].append(net["id"])

    # -- pin positioning ---------------------------------------------------
    def position_pins(self):
        images = {img["name"]: img for img in self.board["images"]}
        for pin in self.board["pins"]:
            place = next(
                (c for c in self.board["components"]
                 if c["refdes"] == pin["component"]), None)
            if place is None:
                continue
            image = images.get(place["footprint"])
            if image is None:
                continue
            pad = next(
                (p for p in image["pads"]
                 if norm_pin(p["number"]) == norm_pin(pin["pin"])), None)
            if pad is None:
                continue
            rad = math.radians(place["rotation"])
            cos_r, sin_r = math.cos(rad), math.sin(rad)
            ox, oy = num(pad["x"]), num(pad["y"])
            px = place["x"] + ox * cos_r - oy * sin_r
            py = place["y"] + ox * sin_r + oy * cos_r
            pin["board_x"], pin["board_y"] = px, py
            pin["resolved"] = True

    # -- entry point -------------------------------------------------------
    def clean_wires(self):
        """Drop trace points that fall outside the board box (phantom segments
        produced by buggy freerouting DSN writers), keeping only the maximal
        in-board runs as separate wires."""
        box = self.board["board_box"]
        if box["width"] <= 0 or box["height"] <= 0:
            return
        margin = 0.6
        min_x = box["min_x"] - margin
        max_x = box["min_x"] + box["width"] + margin
        min_y = box["min_y"] - margin
        max_y = box["min_y"] + box["height"] + margin
        cleaned = []
        dropped = 0
        for wire in self.board["wires"]:
            coords = wire["coords"]
            run = []
            runs = []

            def flush():
                if len(run) >= 2:
                    runs.append(list(run))
                run.clear()

            for i in range(0, len(coords) - 1, 2):
                x, y = coords[i], coords[i + 1]
                if min_x <= x <= max_x and min_y <= y <= max_y:
                    run.append(x)
                    run.append(y)
                else:
                    dropped += 1
                    flush()
            flush()
            for flat in runs:
                new = dict(wire)
                new["coords"] = flat
                xs = flat[0::2]
                ys = flat[1::2]
                new["segments"] = [
                    [xs[i], ys[i], xs[i + 1], ys[i + 1]]
                    for i in range(len(xs) - 1)]
                cleaned.append(new)
        self.board["wires"] = cleaned
        self.board["dropped_phantom_points"] = dropped
        self.board["clean"] = True

    def convert(self, tree, clean=False):
        for node in tree:
            if not isinstance(node, list):
                continue
            k = kw(node)
            if k == "pcb":
                if len(node) > 1 and isinstance(node[1], str):
                    self.board["board_name"] = node[1]
                for sub in node[2:]:
                    if not isinstance(sub, list):
                        continue
                    sk = kw(sub)
                    if sk == "resolution" and len(sub) >= 3:
                        self.board["resolution"] = {
                            "unit": str(sub[1]), "factor": num(sub[2])}
                    elif sk == "structure":
                        self.read_structure(sub)
                    elif sk == "library":
                        self.read_library(sub)
                    elif sk in ("network", "net"):
                        self.read_network(sub)
                    elif sk == "placement":
                        self.read_placement(sub)
                    elif sk == "wiring":
                        self.read_wiring(sub)
                    # parser / host_cad / host_version / rule / ... ignored
        self.position_pins()
        if clean:
            self.clean_wires()
        return self.board


def convert_file(path, clean=False):
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    tokens = tokenize(text)
    tree = parse_sexp(tokens)
    if not tree:
        raise ValueError("DSN file contains no scopes")
    board = DsnConverter(path).convert(tree, clean=clean)
    board["board_box"]["width"] = max(board["board_box"]["width"], 0)
    board["board_box"]["height"] = max(board["board_box"]["height"], 0)
    return board


def main():
    parser = argparse.ArgumentParser(
        description="Convert a Specctra DSN file into a debug-friendly JSON document.")
    parser.add_argument("input", help="path to the .dsn input file")
    parser.add_argument("-o", "--output", default=None,
                        help="output .json path (default: <input>.json)")
    parser.add_argument("--stdout", action="store_true",
                        help="write the JSON to stdout instead of a file")
    parser.add_argument("--minify", action="store_true",
                        help="compact output instead of pretty-printed")
    parser.add_argument("--clean", action="store_true",
                        help="drop phantom trace points that fall outside the "
                             "board box (cosmetic filter for buggy DSNs)")
    args = parser.parse_args()

    try:
        board = convert_file(args.input, clean=args.clean)
    except (OSError, ValueError) as exc:
        print("error: {}".format(exc), file=sys.stderr)
        return 1

    if args.minify:
        text = json.dumps(board, separators=(",", ":"))
    else:
        text = json.dumps(board, indent=2)

    if args.stdout:
        sys.stdout.write(text + "\n")
        return 0

    output = args.output or (args.input[:-4] + ".json" if args.input.lower().endswith(".dsn")
                             else args.input + ".json")
    with open(output, "w", encoding="utf-8") as fh:
        fh.write(text + "\n")
    print("wrote {}".format(output))
    return 0


if __name__ == "__main__":
    sys.exit(main())