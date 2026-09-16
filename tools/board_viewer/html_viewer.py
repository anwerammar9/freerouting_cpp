#!/usr/bin/env python3
"""Generate a self-contained interactive HTML board viewer from a
dsn_to_json.py JSON document.

Usage:
    python3 board_viewer.py input.json [-o view.html] [--open]

Features
    - pan (drag) / zoom (wheel, centred on cursor) / fit (double-click or 'F')
    - per-layer visibility toggles
    - per-net highlight: type in the search box, click a net to isolate it
    - hover tooltips with item info
    - board outline, pads/pins, traces (per-layer tints) and vias
"""

import argparse
import json
import os
import sys
import webbrowser

try:
    from pathlib import Path
except ImportError:
    Path = None


def sanitize_coords(coords):
    xs = coords[0::2]
    ys = coords[1::2]
    return xs, ys


def prepare(json_data):
    """Build a compact, viewer-friendly structure from the full JSON board."""
    board = json_data

    layers = [
        {"name": l["name"], "index": l.get("index", i), "signal": l.get("signal", True)}
        for i, l in enumerate(board.get("layers", []))
    ]
    layer_of = {l["name"]: idx for idx, l in enumerate(layers)}

    nets = []
    for n in board.get("nets", []):
        nets.append({"id": n["id"], "name": n["name"]})
    net_of = {n["name"]: i for i, n in enumerate(nets)}

    # wires: [[layerIdx, netIdx, [x0,y0,x1,y1,...]], ...]
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
        fixed = bool(w.get("fixed"))
        width = float(w.get("width", 0.0))
        it = {"l": li, "n": ni, "x": xs, "y": ys, "w": width, "f": 1 if fixed else 0}
        # drop degenerate single-point wires for the renderer
        if len(xs) >= 2:
            wires.append(it)

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
        "stats": {
            "nets": len(nets),
            "wires": len(wires),
            "vias": len(vias),
            "pins": len(pins),
        },
    }


HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Board viewer</title>
<style>
  html, body { margin:0; padding:0; height:100%; font-family: system-ui, sans-serif;
               display:flex; flex-direction:column; background:#101418; color:#dfe5ea; }
  #topbar { display:flex; align-items:center; gap:12px; padding:6px 10px;
            background:#1a2027; border-bottom:1px solid #2c3742; flex-wrap:wrap; }
  #topbar .btn { cursor:pointer; background:#2a3540; border:1px solid #3b4a59;
                 color:#dfe5ea; border-radius:4px; padding:3px 8px; font-size:12px; }
  #topbar .btn:hover { background:#35434f; }
  #stats { font-size:12px; color:#9fb3c2; }
  #main { flex:1; display:flex; min-height:0; }
  #canvasWrap { flex:1; position:relative; min-width:0; }
  #canvas { width:100%; height:100%; display:block; cursor:grab; background:#0a0e11; }
  #canvas.panning { cursor:grabbing; }
  #tooltip { position:absolute; pointer-events:none; background:#141a1f; border:1px solid #3b4a59;
             color:#dfe5ea; font-size:11px; padding:5px 8px; border-radius:4px; display:none;
             white-space:pre; max-width:360px; z-index:10; }
  #panel { width:260px; background:#161b21; border-left:1px solid #2c3742;
           display:flex; flex-direction:column; min-height:0; overflow:hidden; }
  #panel h3 { font-size:11px; text-transform:uppercase; letter-spacing:.06em; color:#7c93a6;
              margin:10px 10px 6px; }
  #netSearch { margin:0 10px 6px; padding:5px 8px; background:#0f1418; color:#dfe5ea;
               border:1px solid #3b4a59; border-radius:4px; font-size:12px; }
  #netList { flex:1; overflow-y:auto; padding:0 6px 10px; min-height:40px; }
  .netRow { display:flex; align-items:center; gap:6px; padding:2px 6px; border-radius:4px;
            cursor:pointer; font-size:12px; width:100%; border:0 none; background:none; color:#dfe5ea; text-align:left; }
  .netRow:hover { background:#232c35; }
  .netRow.selected { background:#2e4260; }
  .dot { width:10px; height:10px; border-radius:50%; flex:none; }
  .netRow .name { overflow:hidden; text-overflow:ellipsis; white-space:nowrap; flex:1; }
  #panel .layers { padding:0 10px 6px; border-bottom:1px solid #2c3742; }
  .layerRow { display:flex; align-items:center; gap:6px; font-size:12px; padding:2px 0; }
  .layerRow input { cursor:pointer; }
  #hint { position:absolute; right:10px; top:10px; font-size:11px; color:#5d7de8;
          background:rgba(93,125,232,.1); padding:4px 8px; border-radius:4px; pointer-events:none; }
</style>
</head>
<body>
<div id="topbar">
  <span style="font-weight:600">Board viewer</span>
  <span class="btn" id="fitBtn">Fit (F)</span>
  <span class="btn" id="resetBtn">Reset (R)</span>
  <span id="stats"></span>
</div>
<div id="main">
  <div id="canvasWrap">
    <canvas id="canvas"></canvas>
    <div id="tooltip"></div>
    <div id="hint">drag to pan &middot; wheel to zoom</div>
  </div>
  <div id="panel">
    <h3>Layers</h3>
    <div class="layers" id="layerList"></div>
    <h3>Nets</h3>
    <input id="netSearch" placeholder="filter nets..." spellcheck="false">
    <div id="netList"></div>
  </div>
</div>
<script id="boardData" type="application/json">@@DATA@@</script>
<script>
"use strict";
const B = JSON.parse(document.getElementById("boardData").textContent);
const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");
const wrap = document.getElementById("canvasWrap");
const tooltip = document.getElementById("tooltip");

const layers = B.layers, nets = B.nets, wires = B.wires,
      vias = B.vias, pins = B.pins, outlines = B.outlines;

const layerVisible = new Array(layers.length).fill(true);
const layerColor = layers.map((_, i) => hsl((i * 137.5) % 360, "70%", "60%"));

function hashHue(s) {
  let h = 0;
  for (let i = 0; i < s.length; i++) h = (h * 31 + s.charCodeAt(i)) & 0x7fffffff;
  return h % 360;
}
const netHue = nets.map((n, i) => n && n.name ? hashHue(n.name) : (i * 29) % 360);
const NO_NET_COLOR = "#8892a0";

// ---- coordinate system --------------------------------------------------
let vp = { x: 0, y: 0, scale: 1 };   // world->screen: sx = (wx-vp.x)*vp.scale + W/2
let pan = { sx: 0, sy: 0, wx: 0, wy: 0 };

function worldSX(wx) { return (wx - vp.x) * vp.scale + canvas.width  / 2; }
function worldSY(wy) { return (wy - vp.y) * vp.scale + canvas.height / 2; }
function screenWX(sx) { return (sx - canvas.width  / 2) / vp.scale + vp.x; }
function screenWY(sy) { return (sy - canvas.height / 2) / vp.scale + vp.y; }

function fitAll() {
  let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
  function acc(x, y) { if (!isFinite(x) || !isFinite(y)) return;
    if (x < minX) minX = x; if (y < minY) minY = y;
    if (x > maxX) maxX = x; if (y > maxY) maxY = y; }
  for (const o of outlines) for (const p of o.points) acc(p[0], p[1]);
  for (const p of pins) acc(p.x, p.y);
  for (const v of vias) acc(v.x, v.y);
  for (const w of wires) { for (let i = 0; i < w.x.length; i++) acc(w.x[i], w.y[i]); }
  if (!isFinite(minX)) { vp = { x: 0, y: 0, scale: 1 }; return; }
  const pad = 40;
  const w = maxX - minX, h = maxY - minY;
  const scale = Math.min(canvas.width / (w + 2 * pad), canvas.height / (h + 2 * pad));
  vp = { x: minX - pad - (canvas.width / scale - (maxX - minX + 2 * pad)) / 2,
         y: minY - pad - (canvas.height / scale - (maxY - minY + 2 * pad)) / 2,
         scale: Math.max(scale, 1e-9) };
}
function resetView() { fitAll(); }

// ---- drawing ------------------------------------------------------------
function netColor(i) {
  if (i < 0) return NO_NET_COLOR;
  return "hsl(" + netHue[i] + ",60%,58%)";
}
let selectedNet = -1;        // isolated net index, -1 = show all
let hovered = null;

function drawPath(xs, ys, lw, color, lwScale) {
  ctx.beginPath();
  ctx.moveTo(worldSX(xs[0]), worldSY(ys[0]));
  for (let i = 1; i < xs.length; i++) ctx.lineTo(worldSX(xs[i]), worldSY(ys[i]));
  ctx.strokeStyle = color;
  ctx.lineWidth = Math.max(lw * lwScale, 1);
  ctx.lineJoin = "round"; ctx.lineCap = "round";
  ctx.stroke();
}

function render() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  const outlineAlpha = layerVisible.every(v => !v) ? 0.4 : 0.9;
  ctx.strokeStyle = outlineAlpha > 0.5 ? "#3d4b58" : "rgba(61,75,88,.5)";
  ctx.lineWidth = 1.5;
  for (const o of outlines) {
    const pts = o.points;
    ctx.beginPath();
    ctx.moveTo(worldSX(pts[0][0]), worldSY(pts[0][1]));
    for (let i = 1; i < pts.length; i++) ctx.lineTo(worldSX(pts[i][0]), worldSY(pts[i][1]));
    ctx.closePath();
    ctx.stroke();
  }

  // traces, back to front, respecting layer visibility
  for (const w of wires) {
    if (!layerVisible[w.l]) continue;
    const dim = selectedNet >= 0 && w.n !== selectedNet;
    if (dim && w.n >= 0) continue;            // isolate: skip other nets
    const base = w.n < 0 ? NO_NET_COLOR : netColor(w.n);
    const tint = layerColor[w.l] || "#666";
    // blend net colour with a fixed layer tint via two passes is heavy; use base.
    drawPath(w.x, w.y, (w.w > 0 ? w.w : 60) * 0.5, base, vp.scale);
  }

  // vias
  for (const v of vias) {
    if (selectedNet >= 0 && v.n !== selectedNet) continue;
    const sx = worldSX(v.x), sy = worldSY(v.y);
    const r = Math.max(6 * vp.scale, 2);
    ctx.beginPath(); ctx.arc(sx, sy, r, 0, Math.PI * 2);
    ctx.fillStyle = v.n < 0 ? NO_NET_COLOR : netColor(v.n);
    ctx.fill();
    ctx.beginPath(); ctx.arc(sx, sy, Math.max(r * 0.45, 1.2), 0, Math.PI * 2);
    ctx.fillStyle = "#101418";
    ctx.fill();
  }

  // pins
  for (const p of pins) {
    if (selectedNet >= 0 && p.n !== selectedNet) continue;
    const sx = worldSX(p.x), sy = worldSY(p.y);
    const s = Math.max(4 * vp.scale, 2);
    ctx.fillStyle = p.n < 0 ? "#9aa4b0" : "rgba(200,210,220,.92)";
    ctx.fillRect(sx - s, sy - s, s * 2, s * 2);
  }
}

// ---- interaction --------------------------------------------------------
function resize() {
  const dpr = window.devicePixelRatio || 1;
  canvas.width = wrap.clientWidth * dpr;
  canvas.height = wrap.clientHeight * dpr;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  canvas.style.width = wrap.clientWidth + "px";
  canvas.style.height = wrap.clientHeight + "px";
  canvas.width = wrap.clientWidth; canvas.height = wrap.clientHeight; // non-retina ok
  render();
}
window.addEventListener("resize", resize);

canvas.addEventListener("mousedown", e => { canvas.classList.add("panning");
  pan.sx = e.clientX; pan.sy = e.clientY; pan.wx = vp.x; pan.wy = vp.y; });
window.addEventListener("mouseup", () => canvas.classList.remove("panning"));
window.addEventListener("mousemove", e => {
  if (canvas.classList.contains("panning")) {
    vp.x = pan.wx - (e.clientX - pan.sx) / vp.scale;
    vp.y = pan.wy - (e.clientY - pan.sy) / vp.scale;
    render();
  }
});
canvas.addEventListener("wheel", e => {
  e.preventDefault();
  const rect = canvas.getBoundingClientRect();
  const mx = e.clientX - rect.left, my = e.clientY - rect.top;
  const factor = Math.exp(-e.deltaY * 0.0012);
  const newScale = Math.min(Math.max(vp.scale * factor, 1e-9), 1e7);
  const wx = screenWX(mx), wy = screenWY(my);
  vp.scale = newScale;
  vp.x = wx - (mx - canvas.width / 2) / newScale;
  vp.y = wy - (my - canvas.height / 2) / newScale;
  render();
}, { passive: false });

canvas.addEventListener("dblclick", () => { fitAll(); render(); });
document.getElementById("fitBtn").addEventListener("click", () => { fitAll(); render(); });
document.getElementById("resetBtn").addEventListener("click", () => { resetView(); render(); });

canvas.addEventListener("mousemove", e => { pick(e); });
function pick(e) {
  const rect = canvas.getBoundingClientRect();
  const wx = screenWX(e.clientX - rect.left), wy = screenWY(e.clientY - rect.top);
  const scale = vp.scale;
  let best = null, bestD = Infinity;
  for (const p of pins) { const d = (p.x - wx) ** 2 + (p.y - wy) ** 2;
    if (d < bestD && Math.sqrt(d) * scale < 14) { bestD = d;
      best = { type: "pin", txt: p.comp + "-" + p.pin + "  net " + (p.n >= 0 ? nets[p.n].name : "?" ) }; } }
  for (const v of vias) { const d = (v.x - wx) ** 2 + (v.y - wy) ** 2;
    if (d < bestD && Math.sqrt(d) * scale < 14) { bestD = d;
      best = { type: "via", txt: "via " + (v.pad||"") + "\\net " + (v.n >= 0 ? nets[v.n].name : "?") }; } }
  if (!best) {
    for (const w of wires) {
      if (!layerVisible[w.l]) continue;
      for (let i = 0; i < w.x.length - 1; i++) {
        const d = segDist(wx, wy, w.x[i], w.y[i], w.x[i + 1], w.y[i + 1]);
        if (d * scale < 12 && d < bestD) { bestD = d;
          best = { type: "wire", txt: "trace " + (layers[w.l] ? layers[w.l].name : "?") +
                    "  width " + w.w + "  fixed:" + w.f + "\\net " + (w.n >= 0 ? nets[w.n].name : "unassigned") }; }
      } }
  }
  hovered = best;
  if (best) {
    tooltip.style.display = "block";
    tooltip.style.left = Math.min(e.clientX - wrap.getBoundingClientRect().left + 12, wrap.clientWidth - 180) + "px";
    tooltip.style.top = Math.max(e.clientY - wrap.getBoundingClientRect().top - 10, 4) + "px";
    tooltip.textContent = best.txt;
  } else tooltip.style.display = "none";
}
function segDist(px, py, ax, ay, bx, by) {
  const dx = bx - ax, dy = by - ay;
  const L2 = dx * dx + dy * dy;
  if (L2 === 0) return Math.hypot(px - ax, py - ay);
  let t = ((px - ax) * dx + (py - ay) * dy) / L2;
  t = Math.max(0, Math.min(1, t));
  return Math.hypot(px - (ax + t * dx), py - (ay + t * dy));
}

// ---- panels -------------------------------------------------------------
function hsl(h, s, l) { return "hsl(" + h + "," + s + "," + l + ")"; }

const layerList = document.getElementById("layerList");
layers.forEach((L, i) => {
  const row = document.createElement("label");
  row.className = "layerRow";
  const cb = document.createElement("input");
  cb.type = "checkbox"; cb.checked = true;
  cb.onchange = () => { layerVisible[i] = cb.checked; render(); };
  const dot = document.createElement("span");
  dot.className = "dot"; dot.style.background = layerColor[i];
  const label = document.createElement("span");
  label.textContent = L.name;
  row.appendChild(cb); row.appendChild(dot); row.appendChild(label);
  layerList.appendChild(row);
});

const netList = document.getElementById("netList");
const netSearch = document.getElementById("netSearch");
let netRows = [];

function buildNetRows() {
  netList.innerHTML = "";
  netRows = nets.map((n, i) => {
    const row = document.createElement("button");
    row.className = "netRow";
    const dot = document.createElement("span");
    dot.className = "dot"; dot.style.background = netColor(i);
    const name = document.createElement("span");
    name.className = "name"; name.textContent = n.name;
    row.appendChild(dot); row.appendChild(name);
    row.onclick = () => {
      if (selectedNet === i) { selectedNet = -1; }
      else { selectedNet = i; }
      updateNetSelection(); render();
    };
    netList.appendChild(row);
    return { i, row, name };
  });
}
function updateNetSelection() {
  for (const r of netRows) r.row.classList.toggle("selected", r.i === selectedNet);
  document.dispatchEvent(new CustomEvent("netsel"));
}
netSearch.addEventListener("input", () => {
  const q = netSearch.value.toLowerCase();
  for (const r of netRows) r.row.style.display = r.name.textContent.toLowerCase().includes(q) ? "" : "none";
});
netRows = [];

// ---- stats & init -------------------------------------------------------
document.getElementById("stats").textContent =
  B.stats.nets + " nets, " + B.stats.wires + " traces, " + B.stats.vias +
  " vias, " + B.stats.pins + " pins" + (B.box && B.box.width ?
    "    " + (B.box.width / 1000).toFixed(1) + " x " + (B.box.height / 1000).toFixed(1) + " kU" : "");

window.addEventListener("keydown", e => {
  if (document.activeElement === netSearch) return;
  if (e.key === "f" || e.key === "F") { fitAll(); render(); }
  if (e.key === "r" || e.key === "R") { resetView(); render(); }
});

buildNetRows();
resize();
fitAll();
render();
</script>
</body>
</html>
"""


def main():
    parser = argparse.ArgumentParser(
        description="Generate an interactive HTML board viewer from dsn_to_json output.")
    parser.add_argument("input", help="path to the .json document (from dsn_to_json.py)")
    parser.add_argument("-o", "--output", default=None,
                        help="output .html path (default: <input>.html)")
    parser.add_argument("--open", action="store_true",
                        help="open the generated HTML in the default browser")
    args = parser.parse_args()

    try:
        with open(args.input, "r", encoding="utf-8") as fh:
            raw = fh.read()
        board = json.loads(raw)
    except (OSError, ValueError) as exc:
        print("error: {}".format(exc), file=sys.stderr)
        return 1

    if isinstance(board, list) and board and "board_name" in board[0]:
        board = board[0]
    if "board_name" not in board:
        print("error: not a dsn_to_json document (missing board_name)", file=sys.stderr)
        return 1

    data = prepare(board)
    html = HTML_TEMPLATE.replace("@@DATA@@", json.dumps(data))

    output = args.output
    if not output:
        base = args.input
        output = base[:-5] + ".html" if base.lower().endswith(".json") else base + ".html"

    with open(output, "w", encoding="utf-8") as fh:
        fh.write(html)
    print("wrote {}".format(output))

    if args.open:
        webbrowser.open("file://" + os.path.abspath(output))
    return 0


if __name__ == "__main__":
    sys.exit(main())