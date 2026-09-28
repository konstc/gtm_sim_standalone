#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
"""Checks the TOM PWM + interrupt example waveforms.

Expected (see tom_pwm_irq_app.cpp):
  - TOM0_OUT(0): PWM with period 500 ns (100 ticks of 5 ns). Each period
    starts with a falling edge; the output stays low for the duty cycle
    (CM1 = 25 ticks = 125 ns), and for 250 ns (CM1 = 50) once the ISR has
    updated the duty cycle after the 5th interrupt
  - TOM0_IRQ(0): exactly 10 interrupt pulses, one per period end, then disabled
  - each interrupt is acknowledged by the ISR (the IRQ line returns to 0)

usage: check_tom_pwm_irq.py TRACE.vcd
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools"))
from vcdlib import Vcd

NS = 1000
failures = []

def check(cond, what):
    print(("  ok    " if cond else "  FAIL  ") + what)
    if not cond:
        failures.append(what)

def main():
    vcd = Vcd.load(sys.argv[1])
    print(f"checking {sys.argv[1]} ({len(vcd.signals)} signals, end {vcd.end_time / NS:.0f} ns)")

    out = vcd.signals.get("TOM0_OUT(0)")
    irq = vcd.signals.get("TOM0_IRQ(0)")
    check(out is not None and irq is not None, "TOM0_OUT(0) and TOM0_IRQ(0) traced")
    if not (out and irq):
        return 1

    irq_rises = irq.edges(rising=True)
    irq_falls = irq.edges(rising=False)
    check(len(irq_rises) == 10, f"10 interrupt pulses ({len(irq_rises)})")
    check(len(irq_falls) == len(irq_rises), "every interrupt is acknowledged")
    if len(irq_rises) >= 2:
        spacing = {b - a for a, b in zip(irq_rises, irq_rises[1:])}
        check(spacing == {500 * NS}, f"one interrupt per PWM period ({sorted(s / NS for s in spacing)})")

    # Period starts = falling edges once the channel runs (the first interrupt
    # marks the first complete period; earlier edges are reset/configuration).
    start = irq_rises[0] if irq_rises else 0
    falls = [t for t in out.edges(rising=False) if t >= start]
    rises = [t for t in out.edges(rising=True) if t >= start]
    periods = {b - a for a, b in zip(falls, falls[1:])}
    check(periods == {500 * NS}, f"PWM period 500 ns ({sorted(p / NS for p in periods)})")

    lows = []  # (period start, low time)
    for f in falls:
        nxt = [r for r in rises if r > f]
        if nxt:
            lows.append((f, nxt[0] - f))
    widths = [w for _, w in lows]
    check(125 * NS in widths and 250 * NS in widths and set(widths) <= {125 * NS, 250 * NS},
          f"duty cycle 125 ns, later 250 ns ({sorted(set(w / NS for w in widths))})")
    first_250 = next((t for t, w in lows if w == 250 * NS), None)
    last_125 = max((t for t, w in lows if w == 125 * NS), default=None)
    check(first_250 is not None and last_125 is not None and last_125 < first_250,
          "duty cycle changes exactly once (125 ns -> 250 ns)")
    if len(irq_rises) >= 5 and first_250 is not None:
        check(irq_rises[4] < first_250, "duty cycle change follows the 5th interrupt")
    # The first interrupt ends the start-up period (output already low).
    check(all(any(abs(f - i) <= 1 * NS for f in falls) for i in irq_rises[1:]),
          "each interrupt coincides with a period start")

    print(f"{len(failures)} check(s) failed" if failures else "all checks passed")
    return 1 if failures else 0

if __name__ == "__main__":
    sys.exit(main())
