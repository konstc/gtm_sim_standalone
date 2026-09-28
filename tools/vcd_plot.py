#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
"""Render VCD signals as SVG waveform plots (no dependencies besides Python).

The plots are described in a JSON file, one entry per SVG:

    {"plots": [{
        "output": "overview.svg",           # relative to the JSON file
        "title": "LAB0: PWM outputs",
        "vcd": "atom_somp_lab0.vcd",        # relative to --vcd-dir
        "from_ns": 0, "to_ns": 6700,
        "signals": [
            {"name": "ATOM0_OUT(0)"},                          # 1 bit: logic
            {"name": "*.atom0_ch0.CN0:SOMP_UP", "label": "ch0 CN0",
             "style": "analog"},                               # step line
            {"name": "*.atom0_ch0.CM0", "format": "dec"},      # vector: bus
            {"name": "ATOM0_OUT(0)", "vcd": "atom_somp_lab1.vcd", "label": "LAB1"}
        ],
        "markers": [{"t_ns": 2625, "label": "SR0 written"}]
    }]}

Names are glob patterns on the full signal name (without the top scope);
the first match is plotted. Styles: "logic" (default for 1 bit), "bus"
(default for vectors, value printed in "hex" or "dec"), "analog".

usage: vcd_plot.py PLOTS.json --vcd-dir DIR [--only NAME]
"""
import argparse
import json
import math
import os
import sys
from xml.sax.saxutils import escape

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vcdlib import Vcd

LABEL_W = 170   # label column
PLOT_W = 780    # waveform area
PAD = 16
TITLE_H = 30
AXIS_H = 34
ROW_H = {"logic": 30, "bus": 30, "analog": 76}
MARKER_H = 16   # extra space above the rows for marker labels

COLORS = {
    "bg": "#ffffff", "frame": "#d0d7de", "text": "#1f2328", "muted": "#656d76",
    "grid": "#eaeef2", "logic": "#1a7f37", "bus": "#0969da", "bus_fill": "#ddf4ff",
    "analog": "#8250df", "undef": "#cf222e", "marker": "#bc4c00",
}
FONT = "font-family='ui-monospace,SFMono-Regular,Consolas,Menlo,monospace'"

def nice_step(span, target=8):
    raw = span / target
    mag = 10 ** math.floor(math.log10(raw))
    for m in (1, 2, 2.5, 5, 10):
        if raw <= m * mag:
            return m * mag
    return 10 * mag

def fmt_time(t_ns, unit):
    v = t_ns / 1000 if unit == "µs" else t_ns
    return f"{v:g}"

def fmt_value(v, fmt, width):
    if v is None:
        return "x"
    if isinstance(v, float):
        return f"{v:g}"
    if fmt == "dec":
        return str(v)
    return f"0x{v:0{max(1, (width + 3) // 4)}X}" if width <= 32 else f"0x{v:X}"

class Plot:
    def __init__(self, spec, vcds):
        self.spec = spec
        self.t0 = spec["from_ns"] * 1000.0
        self.t1 = spec["to_ns"] * 1000.0
        self.rows = []
        for s in spec["signals"]:
            vcd = vcds[s.get("vcd", spec.get("vcd"))]
            sig = vcd.signals.get(s["name"]) or vcd.find(s["name"])
            if sig is None:
                raise SystemExit(f"{spec['output']}: no signal matches {s['name']!r}")
            style = s.get("style", "logic" if sig.width == 1 else "bus")
            scale = vcd.timescale_ps
            changes = [(t * scale, v) for t, v in sig.changes]
            self.rows.append((s.get("label", sig.name.split(".")[-1]), style, s.get("format", "hex"),
                              sig.width, changes))
        self.markers = spec.get("markers", [])

    def x(self, t_ps):
        t = min(max(t_ps, self.t0), self.t1)
        return LABEL_W + PAD + (t - self.t0) / (self.t1 - self.t0) * PLOT_W

    def segments(self, changes):
        """(start, end, value) clipped to the window."""
        value, cursor, out = None, self.t0, []
        for t, v in changes:
            if t >= self.t1:
                break
            if t > self.t0:
                out.append((cursor, t, value))
                cursor = t
            value = v
        out.append((cursor, self.t1, value))
        return out

    def render(self):
        top = PAD + TITLE_H + (MARKER_H if self.markers else 0)
        rows_h = sum(ROW_H[r[1]] for r in self.rows)
        width = LABEL_W + PLOT_W + 2 * PAD + 10
        height = top + rows_h + AXIS_H + PAD
        x0, x1 = self.x(self.t0), self.x(self.t1)
        o = [f"<svg xmlns='http://www.w3.org/2000/svg' width='{width}' height='{height}' "
             f"viewBox='0 0 {width} {height}' {FONT} font-size='12'>",
             f"<rect x='0.5' y='0.5' width='{width - 1}' height='{height - 1}' rx='6' "
             f"fill='{COLORS['bg']}' stroke='{COLORS['frame']}'/>",
             f"<text x='{PAD}' y='{PAD + 14}' font-size='14' font-weight='bold' "
             f"fill='{COLORS['text']}'>{escape(self.spec.get('title', ''))}</text>"]

        # time axis and grid
        span_ns = (self.t1 - self.t0) / 1000
        unit = "µs" if span_ns >= 2000 else "ns"
        step = nice_step(span_ns)
        axis_y = top + rows_h + 6
        first = math.ceil(self.spec["from_ns"] / step) * step
        t = first
        while t <= self.spec["to_ns"] + 1e-9:
            xx = self.x(t * 1000)
            o.append(f"<line x1='{xx:.1f}' y1='{top}' x2='{xx:.1f}' y2='{axis_y}' stroke='{COLORS['grid']}'/>")
            o.append(f"<text x='{xx:.1f}' y='{axis_y + 14}' text-anchor='middle' "
                     f"fill='{COLORS['muted']}'>{fmt_time(t, unit)}</text>")
            t += step
        o.append(f"<line x1='{x0}' y1='{axis_y}' x2='{x1}' y2='{axis_y}' stroke='{COLORS['muted']}'/>")
        o.append(f"<text x='{x1}' y='{axis_y + 28}' text-anchor='end' fill='{COLORS['muted']}'>"
                 f"time [{unit}]</text>")

        # signal rows
        y = top
        for i, (label, style, fmt, width_bits, changes) in enumerate(self.rows):
            h = ROW_H[style]
            if i % 2:
                o.append(f"<rect x='1' y='{y}' width='{width - 2}' height='{h}' fill='#f6f8fa' opacity='0.6'/>")
            o.append(f"<text x='{PAD}' y='{y + h / 2 + 4:.1f}' fill='{COLORS['text']}'>{escape(label)}</text>")
            segs = self.segments(changes)
            if style == "logic":
                o += self.render_logic(segs, y + 7, y + h - 7)
            elif style == "bus":
                o += self.render_bus(segs, y + 6, y + h - 6, fmt, width_bits)
            else:
                o += self.render_analog(segs, y + 8, y + h - 8, fmt, width_bits)
            y += h

        # markers
        for m in self.markers:
            xx = self.x(m["t_ns"] * 1000)
            o.append(f"<line x1='{xx:.1f}' y1='{top - 4}' x2='{xx:.1f}' y2='{axis_y}' "
                     f"stroke='{COLORS['marker']}' stroke-dasharray='4 3'/>")
            fits_right = xx + 4 + len(m["label"]) * 7.3 <= width - PAD
            anchor = m.get("anchor", "start" if fits_right else "end")
            dx = 4 if anchor == "start" else -4
            o.append(f"<text x='{xx + dx:.1f}' y='{top - 6}' text-anchor='{anchor}' "
                     f"fill='{COLORS['marker']}'>{escape(m['label'])}</text>")
        o.append("</svg>")
        return "\n".join(o) + "\n"

    def render_logic(self, segs, y_hi, y_lo):
        pts = []
        undef = []
        for a, b, v in segs:
            xa, xb = self.x(a), self.x(b)
            if v is None:
                undef.append(f"<rect x='{xa:.1f}' y='{y_hi}' width='{max(xb - xa, 0.5):.1f}' "
                             f"height='{y_lo - y_hi}' fill='{COLORS['undef']}' opacity='0.25'/>")
                yy = (y_hi + y_lo) / 2
            else:
                yy = y_hi if v else y_lo
            pts += [(xa, yy), (xb, yy)]
        path = " ".join(f"{px:.1f},{py:.1f}" for px, py in pts)
        return undef + [f"<polyline points='{path}' fill='none' stroke='{COLORS['logic']}' stroke-width='1.5'/>"]

    def render_bus(self, segs, y_hi, y_lo, fmt, width_bits):
        out = []
        ym = (y_hi + y_lo) / 2
        for a, b, v in segs:
            xa, xb = self.x(a), self.x(b)
            e = min(3.0, (xb - xa) / 2)
            color = COLORS["undef"] if v is None else COLORS["bus"]
            out.append(f"<polygon points='{xa:.1f},{ym} {xa + e:.1f},{y_hi} {xb - e:.1f},{y_hi} "
                       f"{xb:.1f},{ym} {xb - e:.1f},{y_lo} {xa + e:.1f},{y_lo}' "
                       f"fill='{COLORS['bus_fill']}' stroke='{color}'/>")
            text = fmt_value(v, fmt, width_bits)
            if len(text) * 7.3 + 6 < xb - xa:
                out.append(f"<text x='{(xa + xb) / 2:.1f}' y='{ym + 4}' text-anchor='middle' "
                           f"fill='{COLORS['text']}'>{escape(text)}</text>")
        return out

    def render_analog(self, segs, y_hi, y_lo, fmt, width_bits):
        vals = [v for _, _, v in segs if v is not None]
        lo, hi = (min(vals), max(vals)) if vals else (0, 1)
        if hi == lo:
            hi = lo + 1
        yv = lambda v: y_lo - (v - lo) / (hi - lo) * (y_lo - y_hi)
        pts = []
        for a, b, v in segs:
            yy = yv(lo if v is None else v)
            pts += [(self.x(a), yy), (self.x(b), yy)]
        path = " ".join(f"{px:.1f},{py:.1f}" for px, py in pts)
        xr = LABEL_W + PAD - 4
        return [f"<text x='{xr}' y='{y_hi + 4}' text-anchor='end' font-size='10' fill='{COLORS['muted']}'>"
                f"{fmt_value(hi, 'dec', width_bits)}</text>",
                f"<text x='{xr}' y='{y_lo + 3}' text-anchor='end' font-size='10' fill='{COLORS['muted']}'>"
                f"{fmt_value(lo, 'dec', width_bits)}</text>",
                f"<polyline points='{path}' fill='none' stroke='{COLORS['analog']}' stroke-width='1.2'/>"]

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("spec", help="JSON plot description")
    ap.add_argument("--vcd-dir", required=True, help="folder with the VCD files")
    ap.add_argument("--only", help="render only the plot with this output name")
    args = ap.parse_args()

    with open(args.spec, encoding="utf-8") as f:
        plots = json.load(f)["plots"]
    out_dir = os.path.dirname(os.path.abspath(args.spec))

    vcds = {}
    for p in plots:
        if args.only and p["output"] != args.only:
            continue
        for name in {p.get("vcd")} | {s.get("vcd") for s in p["signals"]}:
            if name and name not in vcds:
                vcds[name] = Vcd.load(os.path.join(args.vcd_dir, name))
        svg = Plot(p, vcds).render()
        path = os.path.join(out_dir, p["output"])
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(svg)
        print(f"wrote {path}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
