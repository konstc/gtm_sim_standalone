# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
"""Minimal VCD reader (value change dump, as written by SystemC).

    vcd = Vcd.load("trace.vcd")
    sig = vcd.find("*.atom0_ch0.CN0*")        # glob on full names
    for t, value in sig.changes: ...           # t in timescale units (int)
    sig.value_at(t)                            # value (int, or None if x/z)

Signal names are the dot-joined scope path without the top "SystemC" scope,
e.g. "i_atom_somp_tb.i_cos_gtm.i_gtm_top1.atom0.atom_out".
"""
import fnmatch
import re

class Signal:
    def __init__(self, name, width, kind):
        self.name = name
        self.width = width
        self.kind = kind
        self.changes = []  # list of (time, int or None)

    def value_at(self, t):
        value = None
        for ct, v in self.changes:
            if ct > t:
                break
            value = v
        return value

    def edges(self, rising=True):
        """Times of 0->1 (rising) or 1->0 (falling) transitions (1-bit signals)."""
        out = []
        prev = None
        for t, v in self.changes:
            if prev is not None and v is not None:
                if rising and prev == 0 and v == 1:
                    out.append(t)
                if not rising and prev == 1 and v == 0:
                    out.append(t)
            prev = v
        return out

    def __repr__(self):
        return f"Signal({self.name}, width={self.width}, changes={len(self.changes)})"

class Vcd:
    def __init__(self):
        self.timescale_ps = 1.0
        self.signals = {}  # full name -> Signal
        self._by_id = {}   # id code -> [Signal]
        self.end_time = 0

    @staticmethod
    def load(path, top_scope="SystemC"):
        vcd = Vcd()
        scopes = []
        with open(path, "r", errors="replace") as f:
            text = f.read()
        header, _, body = text.partition("$enddefinitions")
        m = re.search(r"\$timescale\s+(\d+)\s*(\w+)\s+\$end", header)
        if m:
            unit = {"fs": 1e-3, "ps": 1.0, "ns": 1e3, "us": 1e6, "ms": 1e9, "s": 1e12}[m.group(2)]
            vcd.timescale_ps = int(m.group(1)) * unit
        for tok in re.finditer(r"\$(scope|upscope|var)\b(.*?)\$end", header, re.S):
            kind, rest = tok.group(1), tok.group(2).split()
            if kind == "scope":
                scopes.append(rest[1])
            elif kind == "upscope":
                scopes.pop()
            else:
                vtype, width, code, name = rest[0], int(rest[1]), rest[2], rest[3]
                path = scopes[1:] if scopes and scopes[0] == top_scope else scopes
                full = ".".join(path + [name])
                sig = Signal(full, width, vtype)
                vcd.signals[full] = sig
                vcd._by_id.setdefault(code, []).append(sig)
        t = 0
        for line in body.splitlines()[1:]:
            line = line.strip()
            if not line or line.startswith("$"):
                continue
            c = line[0]
            if c == "#":
                t = int(line[1:])
                vcd.end_time = max(vcd.end_time, t)
            elif c in "01xXzZ":
                vcd._add(line[1:], t, None if c in "xXzZ" else int(c))
            elif c in "bB":
                bits, code = line[1:].split()
                vcd._add(code, t, None if re.search("[xXzZ]", bits) else int(bits, 2))
            elif c in "rR":
                val, code = line[1:].split()
                vcd._add(code, t, float(val))
        return vcd

    def _add(self, code, t, value):
        for sig in self._by_id.get(code, []):
            if sig.changes and sig.changes[-1][0] == t:
                sig.changes[-1] = (t, value)
            else:
                sig.changes.append((t, value))

    def find(self, pattern):
        """First signal matching the glob pattern (None if there is none)."""
        matches = self.find_all(pattern)
        return matches[0] if matches else None

    def find_all(self, pattern):
        return [s for n, s in sorted(self.signals.items()) if fnmatch.fnmatchcase(n, pattern)]
