#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
"""Compare the signal list of a COSIDE .wave layout with a VCD file.

usage: wave_signals.py LAYOUT.wave TRACE.vcd [--verbose]
exit status: 0 if every referenced signal is present, 1 otherwise
"""
import argparse
import fnmatch
import os
import re
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vcdlib import Vcd  # noqa: E402

def wave_signal_names(path, top_scope="SystemC"):
    """Signal names referenced by a COSIDE .wave file (top scope stripped)."""
    names = []
    for elem in ET.parse(path).iter():
        if elem.tag == "signals" and "name" in elem.attrib:
            name = elem.attrib["name"]
            if name.startswith(top_scope + "."):
                name = name[len(top_scope) + 1:]
            names.append(name)
    return names

def normalize(name):
    """COSIDE writes vector elements as name(i); SystemC VCD as name(i) too."""
    return re.sub(r"\[(\d+)\]", r"(\1)", name)

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("wave")
    ap.add_argument("vcd")
    ap.add_argument("--verbose", action="store_true", help="also list the matched signals")
    ap.add_argument("--allow-missing", action="append", default=[], metavar="GLOB",
                    help="signals known to be absent (reported, but not an error)")
    args = ap.parse_args()

    expected = [normalize(n) for n in wave_signal_names(args.wave)]
    vcd = Vcd.load(args.vcd)
    present = {normalize(n) for n in vcd.signals}

    missing = [n for n in expected if n not in present]
    allowed = [n for n in missing if any(fnmatch.fnmatchcase(n, g) for g in args.allow_missing)]
    missing = [n for n in missing if n not in allowed]
    print(f"{args.wave}: {len(expected)} signals referenced, "
          f"{len(expected) - len(missing) - len(allowed)} present in {os.path.basename(args.vcd)}, "
          f"{len(missing)} missing, {len(allowed)} known to be absent")
    for n in missing:
        print("  missing:", n)
    for n in allowed:
        print("  absent (allowed):", n)
    if args.verbose:
        for n in expected:
            if n in present:
                print("  ok:     ", n)
    return 1 if missing else 0

if __name__ == "__main__":
    sys.exit(main())
