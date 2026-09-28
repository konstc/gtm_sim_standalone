#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
"""Checks the ATOM SOMP waveforms against the expected behaviour.

Reference: "Coside Testcase: ATOM SOMP" (Bosch, 2023-11-10), LAB0/LAB1:
  - CMU_CLK0 = 200 MHz (5 ns) drives ATOM0 channel 0, CMU_CLK1 = 40 MHz
    (25 ns) drives channel 1 (SEL_CMU_CLK_EN periods 5000 ps / 25000 ps)
  - PWM with period 15 / duty 10 ticks, then 25 / 12 ticks after the shadow
    registers are updated (SR0/SR1 -> CM0/CM1 at the end of the period)
  - CN0 counts up and wraps at CM0 - 1
  - LAB1: channel 0 starts with an initial delay (CM0 = 150, CM1 = 100)

usage: check_atom_somp.py TRACE.vcd [--lab1]
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools"))
from vcdlib import Vcd

NS = 1000  # VCD time unit is 1 ps
ATOM0 = "i_atom_somp_tb.i_cos_gtm.i_gtm_top1.atom0."

failures = []

def check(cond, what):
    print(("  ok    " if cond else "  FAIL  ") + what)
    if not cond:
        failures.append(what)

def segments(sig, t_from, t_to):
    """(start, duration, value) of the constant segments fully inside [t_from, t_to]."""
    ch = sig.changes
    out = []
    for i in range(len(ch) - 1):
        t0, t1 = ch[i][0], ch[i + 1][0]
        if t0 >= t_from and t1 <= t_to:
            out.append((t0, t1 - t0, ch[i][1]))
    return out

def pwm_shape(sig, t_from, t_to):
    """Set of (low duration, high duration) pairs observed in the window."""
    segs = segments(sig, t_from, t_to)
    low = {d for _, d, v in segs if v == 0}
    high = {d for _, d, v in segs if v == 1}
    return low, high

def final_value(sig):
    return sig.changes[-1][1] if sig and sig.changes else None

def values(sig):
    return [v for _, v in sig.changes] if sig else []

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("vcd")
    ap.add_argument("--lab1", action="store_true", help="expect the LAB1 initial delay on channel 0")
    args = ap.parse_args()

    vcd = Vcd.load(args.vcd)
    print(f"checking {args.vcd} ({'LAB1' if args.lab1 else 'LAB0'}, {len(vcd.signals)} signals, "
          f"end {vcd.end_time / NS:.0f} ns)")

    sig = lambda name: vcd.signals.get(name)

    # --- register values --------------------------------------------------------
    for ch in (0, 1):
        p = f"{ATOM0}atom0_ch{ch}."
        check(values(sig(p + "SR0"))[-2:] == [15, 25], f"ch{ch} SR0: 15 -> 25")
        check(values(sig(p + "SR1"))[-2:] == [10, 12], f"ch{ch} SR1: 10 -> 12")
        check(values(sig(p + "CM0"))[-2:] == [15, 25], f"ch{ch} CM0: 15 -> 25 (updated from SR0)")
        check(values(sig(p + "CM1"))[-2:] == [10, 12], f"ch{ch} CM1: 10 -> 12 (updated from SR1)")

    # --- clock selection ---------------------------------------------------------
    check(final_value(sig(ATOM0 + "atom0_ch0.SEL_CMU_CLK_EN")) == 5000, "ch0 uses CMU_CLK0 (5000 ps)")
    check(final_value(sig(ATOM0 + "atom0_ch1.SEL_CMU_CLK_EN")) == 25000, "ch1 uses CMU_CLK1 (25000 ps)")

    # --- counters ----------------------------------------------------------------
    for ch, tick in ((0, 5 * NS), (1, 25 * NS)):
        cn0 = sig(f"{ATOM0}atom0_ch{ch}.CN0:SOMP_UP")
        check(cn0 is not None, f"ch{ch} CN0 traced")
        if not cn0:
            continue
        steps = [cn0.changes[i + 1][0] - cn0.changes[i][0] for i in range(1, len(cn0.changes) - 1)
                 if cn0.changes[i + 1][1] == cn0.changes[i][1] + 1]
        check(steps and all(s == tick for s in steps), f"ch{ch} CN0 increments every {tick // NS} ns")
        late = [v for t, v in cn0.changes if t > 4000 * NS]
        check(late and max(late) == 24, f"ch{ch} CN0 wraps at CM0-1 = 24 after the update")
        if not (args.lab1 and ch == 0):
            early = [v for t, v in cn0.changes if t < 2500 * NS]
            check(early and max(early) == 14, f"ch{ch} CN0 wraps at CM0-1 = 14 before the update")

    # --- PWM outputs ---------------------------------------------------------------
    out0, out1 = sig("ATOM0_OUT(0)"), sig("ATOM0_OUT(1)")
    check(out0 is not None and out1 is not None, "ATOM0_OUT(0/1) traced")
    if out0 and out1:
        if not args.lab1:
            low, high = pwm_shape(out0, 800 * NS, 2600 * NS)
            check(low == {50 * NS} and high == {25 * NS}, f"ch0 PWM 15/10 ticks: low 50 ns, high 25 ns ({low}, {high})")
        low, high = pwm_shape(out0, 4000 * NS, vcd.end_time)
        check(low == {60 * NS} and high == {65 * NS}, f"ch0 PWM 25/12 ticks: low 60 ns, high 65 ns ({low}, {high})")
        low, high = pwm_shape(out1, 1000 * NS, 2600 * NS)
        check(low == {250 * NS} and high == {125 * NS}, f"ch1 PWM 15/10 ticks: low 250 ns, high 125 ns ({low}, {high})")
        low, high = pwm_shape(out1, 4000 * NS, vcd.end_time)
        check(low == {300 * NS} and high == {325 * NS}, f"ch1 PWM 25/12 ticks: low 300 ns, high 325 ns ({low}, {high})")

        first_fall0 = out0.edges(rising=False)[0] if out0.edges(rising=False) else None
        if args.lab1:
            # CN0 starts at 0 and counts to CM0=150 before the first period ends.
            check(first_fall0 is not None and first_fall0 > 1300 * NS,
                  f"ch0 first PWM edge delayed by the initial delay ({first_fall0 / NS if first_fall0 else None} ns)")
        else:
            check(first_fall0 is not None and first_fall0 < 800 * NS,
                  f"ch0 first PWM edge without initial delay ({first_fall0 / NS if first_fall0 else None} ns)")

    # --- loop-back ATOM0_OUT(7) -> TIM0_IN(7) ---------------------------------------
    tim7, out7 = sig("TIM0_IN(7)"), sig("ATOM0_OUT(7)")
    check(tim7 is not None and out7 is not None and values(tim7) == values(out7),
          "TIM0_IN(7) follows ATOM0_OUT(7)")

    print(f"{len(failures)} check(s) failed" if failures else "all checks passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
