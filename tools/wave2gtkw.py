#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
"""Convert a COSIDE .wave layout into a GTKWave save file (.gtkw).

The .wave file lists the plotted signals (in display order) together with
their display format (DECIMAL, HEX, BIN, ...) and interpolation. The
generated .gtkw shows the same signals, in the same order, for a VCD
produced by the stand-alone simulation:

    wave2gtkw.py atom_somp_cnt_out.wave atom_somp_tb_object_trace.vcd -o atom_somp.gtkw
    gtkwave atom_somp.gtkw

Signals missing in the VCD are skipped (and reported).
"""
import argparse
import os
import re
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vcdlib import Vcd  # noqa: E402

XMI_ID = "{http://www.omg.org/XMI}id"

# GTKWave trace flags
TR_HEX, TR_DEC, TR_BIN, TR_RJUSTIFY, TR_SIGNED, TR_ASCII = 0x2, 0x4, 0x8, 0x20, 0x400, 0x800
TR_ANALOG_STEP = 0x8000

FORMATS = {"DECIMAL": TR_DEC, "UNSIGNED": TR_DEC, "SIGNED": TR_DEC | TR_SIGNED, "HEX": TR_HEX,
           "HEXADECIMAL": TR_HEX, "BIN": TR_BIN, "BINARY": TR_BIN, "ASCII": TR_ASCII}

def decode_format(hex_blob):
    """The display format is a serialized Java enum; its constant name is the last word."""
    try:
        text = bytes.fromhex(hex_blob).decode("latin-1")
    except ValueError:
        return None
    words = re.findall(r"[A-Z_]{3,}", text)
    return words[-1] if words else None

def read_layout(path, top_scope="SystemC"):
    """[(name without top scope, width, format name, analog)] in display order."""
    root = ET.parse(path).getroot()
    signals = {}
    for sig in root.iter("signals"):
        width = 1
        for t in sig.iter("type"):
            width = int(t.attrib.get("bitWidth", "1"))
        name = sig.attrib["name"]
        if name.startswith(top_scope + "."):
            name = name[len(top_scope) + 1:]
        signals[sig.attrib[XMI_ID]] = (name, width)

    layout = []
    for plot in root.findall("plots"):
        ref = plot.attrib.get("signal")
        if ref not in signals:
            continue
        name, width = signals[ref]
        fmt = None
        for d in plot.iter("displayFormat"):
            fmt = decode_format(d.attrib.get("data", "")) or d.attrib.get("type")
        analog = False
        for i in plot.iter("interpolation"):
            analog = i.attrib.get("type") == "Step" and int(plot.attrib.get("plotHeight", "25")) > 50
        layout.append((name, width, fmt, analog))
    return layout

def gtkw_name(name, width, top_scope="SystemC"):
    full = f"{top_scope}.{name}"
    return f"{full}[{width - 1}:0]" if width > 1 else full

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("wave", help="COSIDE .wave layout")
    ap.add_argument("vcd", help="VCD from the stand-alone simulation")
    ap.add_argument("-o", "--output", help="output .gtkw (default: <vcd>.gtkw)")
    args = ap.parse_args()

    vcd = Vcd.load(args.vcd)
    out = args.output or os.path.splitext(args.vcd)[0] + ".gtkw"

    lines = [f'[dumpfile] "{os.path.abspath(args.vcd)}"', "[timestart] 0", "[size] 1600 900",
             "[signals_width] 420", "[sst_width] 250", "[sst_expanded] 1", "*-24.0 0 -1"]
    missing = []
    shown = 0
    for name, width, fmt, analog in read_layout(args.wave):
        sig = vcd.signals.get(name)
        if sig is None:
            missing.append(name)
            continue
        flags = FORMATS.get((fmt or "").upper(), TR_BIN if sig.width == 1 else TR_HEX) | TR_RJUSTIFY
        if analog and sig.width > 1:
            flags |= TR_ANALOG_STEP
        lines.append(f"@{flags:x}")
        lines.append(gtkw_name(name, sig.width))
        if analog and sig.width > 1:
            lines += ["@20000", "-", "@20000", "-", "@20000", "-"]  # extra height for analog view
        shown += 1

    with open(out, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"wrote {out}: {shown} signals" + (f", {len(missing)} not in the VCD" if missing else ""))
    for name in missing:
        print("  not in VCD:", name)
    return 0

if __name__ == "__main__":
    sys.exit(main())
