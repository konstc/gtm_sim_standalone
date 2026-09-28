#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
"""Compare vtable slot layouts between reference and candidate objects.

Used to verify that the SystemC headers we build against produce the same
virtual-function layout as the (COSEDA) SystemC used for the prebuilt GTM
libraries. For each vtable symbol present on both sides, the ordered list of
relocation targets (demangled) is compared slot by slot. Works for ELF
(Linux) and PE/COFF (MinGW) objects and archives.

usage: vtable_compare.py [--tool-dir DIR] [--pattern REGEX] REF... -- CAND...
"""
import argparse
import os
import re
import subprocess
import sys

class Tools:
    def __init__(self, tool_dir):
        def tool(name):
            if tool_dir:
                for cand in (name + ".exe", name):
                    path = os.path.join(tool_dir, cand)
                    if os.path.exists(path):
                        return path
            return name
        self.nm, self.objdump, self.cxxfilt = tool("nm"), tool("objdump"), tool("c++filt")

    def run(self, *args):
        return subprocess.run(list(args), capture_output=True, text=True, errors="replace").stdout

VTABLE_SECTION = re.compile(r"^(?:\.data\.rel\.ro(?:\.local)?\.|\.rdata\$)(_ZTV\S+)$")

def vtables(tools, paths, pattern):
    """{symbol: [slot targets]} for vtable symbols matching pattern."""
    sizes = {}
    for path in paths:
        # ELF: symbol sizes from nm -S
        for line in tools.run(tools.nm, "-S", "--defined-only", path).splitlines():
            parts = line.split()
            if len(parts) == 4 and parts[3].startswith("_ZTV") and re.search(pattern, parts[3]):
                sizes[parts[3]] = int(parts[1], 16)
        # PE/COFF (no symbol sizes): one COMDAT section per vtable
        for line in tools.run(tools.objdump, "-h", path).splitlines():
            parts = line.split()
            if len(parts) >= 3 and re.match(r"^\d+$", parts[0]):
                m = VTABLE_SECTION.match(parts[1])
                if m and re.search(pattern, m.group(1)):
                    sizes.setdefault(m.group(1), int(parts[2], 16))

    slots = {}
    for path in paths:
        section = None
        for line in tools.run(tools.objdump, "-r", path).splitlines():
            m = re.match(r"RELOCATION RECORDS FOR \[(.*)\]:", line)
            if m:
                sm = VTABLE_SECTION.match(m.group(1))
                section = sm.group(1) if sm and sm.group(1) in sizes else None
                if section:
                    slots[section] = ["0"] * (sizes[section] // 8)
                continue
            if section:
                f = line.split()
                if len(f) == 3 and re.match(r"^[0-9a-f]+$", f[0]):
                    idx = int(f[0], 16) // 8
                    if idx < len(slots[section]):
                        slots[section][idx] = re.sub(r"[+-]0x[0-9a-f]+$", "", f[2])
    return slots

def demangle(tools, names):
    out = subprocess.run([tools.cxxfilt], input="\n".join(names), capture_output=True, text=True).stdout
    return out.splitlines()

def main():
    argv = sys.argv[1:]
    if "--" not in argv:
        sys.exit(__doc__)
    split = argv.index("--")
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--tool-dir", help="directory of nm/objdump/c++filt (default: PATH)")
    ap.add_argument("--pattern", default=r"sc_core|tlm", help="regex on mangled vtable names")
    ap.add_argument("reference", nargs="+")
    args = ap.parse_args(argv[:split])
    candidates = argv[split + 1:]

    tools = Tools(args.tool_dir)
    a = vtables(tools, args.reference, args.pattern)
    b = vtables(tools, candidates, args.pattern)
    common = sorted(set(a) & set(b))
    bad = 0
    for sym in common:
        if a[sym] != b[sym]:
            bad += 1
            print("MISMATCH", demangle(tools, [sym])[0])
            da, db = demangle(tools, a[sym]), demangle(tools, b[sym])
            for i in range(max(len(da), len(db))):
                x = da[i] if i < len(da) else "-"
                y = db[i] if i < len(db) else "-"
                print(f"  {'  ' if x == y else '!='} [{i:2}] {x[:90]}  |  {y[:90]}")
    print(f"compared {len(common)} vtables, {bad} mismatches")
    return 1 if bad or not common else 0

if __name__ == "__main__":
    sys.exit(main())
