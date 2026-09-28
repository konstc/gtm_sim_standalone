#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
"""Compare two VCD files signal by signal.

Signals are matched by hierarchical name; for each signal present in both
files the value changes are compared. Header blocks ($date, $version, ...)
and the order of declarations do not matter, so traces from different
builds/platforms/simulators can be compared.

usage: vcd_diff.py A.vcd B.vcd [--ignore GLOB ...] [--max N]
exit status: 0 if all compared signals are identical, 1 otherwise

Example (Linux vs. Windows build of the same example):
    vcd_diff.py build/linux-release/examples/atom_somp/atom_somp_lab0.vcd \\
                build/windows-release/examples/atom_somp/atom_somp_lab0.vcd
"""
import argparse
import fnmatch
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vcdlib import Vcd  # noqa: E402

def first_difference(a, b):
    n = min(len(a), len(b))
    i = next((k for k in range(n) if a[k] != b[k]), n)
    ta = a[i] if i < len(a) else None
    tb = b[i] if i < len(b) else None
    return (ta or tb)[0], ta, tb

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--ignore", action="append", default=[], metavar="GLOB",
                    help="signal names to skip (glob on the full name, repeatable)")
    ap.add_argument("--max", type=int, default=30, help="maximum number of differences to list")
    args = ap.parse_args()

    va, vb = Vcd.load(args.a), Vcd.load(args.b)
    ignored = lambda n: any(fnmatch.fnmatchcase(n, g) for g in args.ignore)
    names_a = {n for n in va.signals if not ignored(n)}
    names_b = {n for n in vb.signals if not ignored(n)}

    diffs = []
    for name in sorted(names_a & names_b):
        ca, cb = va.signals[name].changes, vb.signals[name].changes
        if ca != cb:
            diffs.append((first_difference(ca, cb), name))

    only_a, only_b = sorted(names_a - names_b), sorted(names_b - names_a)
    print(f"{len(names_a & names_b)} common signals: {len(diffs)} differ; "
          f"{len(only_a)} only in A, {len(only_b)} only in B")
    for (t, ca, cb), name in sorted(diffs)[: args.max]:
        print(f"  t={t / 1000:12.3f} ns  {name}\n      A: {ca}\n      B: {cb}")
    for name in only_a[: args.max]:
        print("  only in A:", name)
    for name in only_b[: args.max]:
        print("  only in B:", name)
    return 1 if diffs or only_a or only_b else 0

if __name__ == "__main__":
    sys.exit(main())
