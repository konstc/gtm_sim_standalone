#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
"""Compare the class layouts reported by layout_probe with the layouts the
prebuilt GTM libraries expect (see docs/ABI_NOTES.md).

usage: check_layout.py <layout_probe executable> <linux64|mingw-w64>
"""
import re
import subprocess
import sys

# Sizes expected by the libraries, per platform. Keys are the names printed
# by layout_probe.
EXPECTED = {
    "linux64": {
        "sizeof(cos_gtm_c1)": 0x1af8,
        "offsetof(cos_gtm_c1, c)": 0x1af0,
        "sizeof(sc_vector_n<sc_in<bool>>)": 0xc0,
        "sizeof(cos_connectivity_elaborator)": 0x10,  # member at 0x90, next at 0xa0
        "sizeof(cos_sc_variable_handle<int>)": 0x80,
        "sizeof(cos_sc_enum_tracer_base)": 0x80,      # derived members at 0x80/0x88
        "sizeof(object_searcher<sc_object>)": 0x20,
        "sizeof(cos_trace_type_registrar_base)": 0x10,  # flag at +8
        "sizeof(sca_obj_floc_trace)": 0x230,
        "sizeof(std::filebuf)": 0xf0,                 # filebuf at 0x38 .. 0x128
    },
    "mingw-w64": {
        "sizeof(cos_gtm_c1)": 0x1af8,
        "offsetof(cos_gtm_c1, c)": 0x1af0,
        "sizeof(sc_vector_n<sc_in<bool>>)": 0xc0,
        "sizeof(cos_connectivity_elaborator)": 0x10,
        "sizeof(cos_sc_variable_handle<int>)": 0x80,
        "sizeof(cos_sc_enum_tracer_base)": 0x80,
        "sizeof(object_searcher<sc_object>)": 0x18,   # 'long' is 32 bit on Windows
        "sizeof(cos_trace_type_registrar_base)": 0x10,
        "sizeof(std::filebuf)": 0xc0,                 # MinGW libstdc++
    },
}


def main():
    probe, platform = sys.argv[1], sys.argv[2]
    out = subprocess.run([probe], capture_output=True, text=True, check=True).stdout
    actual = {m.group(1).strip(): int(m.group(2), 16)
              for m in re.finditer(r"^(.*?)\s*=\s*0x([0-9a-f]+)\s*$", out, re.M)}

    # sca_obj_floc_trace = std::string + int (0x28) + std::ifstream
    expected = dict(EXPECTED[platform])
    if "sizeof(std::ifstream)" in actual:
        expected.setdefault("sizeof(sca_obj_floc_trace)", 0x28 + actual["sizeof(std::ifstream)"])

    bad = 0
    for key, value in expected.items():
        got = actual.get(key)
        ok = got == value
        bad += not ok
        print(f"  {'ok  ' if ok else 'FAIL'}  {key:44s} expected 0x{value:x}, got {hex(got) if got is not None else 'missing'}")
    print(f"{len(expected) - bad}/{len(expected)} layouts match ({platform})")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
