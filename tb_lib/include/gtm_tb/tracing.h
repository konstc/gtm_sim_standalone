// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// gtm_tb - VCD tracing helpers.
#ifndef GTM_TB_TRACING_H_
#define GTM_TB_TRACING_H_

#include <systemc>
#include <string>

#include "gtm_tb/bit_adapters.h"

namespace gtm_tb
{

/**
 * Traces bit 'bit' of an sc_bv<N> signal under 'name'. Creates a bit_extract
 * adapter plus a bool signal (in the adapter's own scope, so no clash with
 * design names). Must be called before the end of elaboration.
 */
template <int N>
void trace_bit(sc_core::sc_trace_file* tf, sc_core::sc_signal<sc_dt::sc_bv<N> >& sig, int bit,
               const std::string& name)
{
    struct bit_tap : sc_core::sc_module
    {
        bit_extract<N> extract;
        sc_core::sc_signal<bool> value;

        bit_tap(sc_core::sc_module_name nm, sc_core::sc_signal<sc_dt::sc_bv<N> >& s, int b)
            : sc_core::sc_module(nm), extract("extract", b), value("value")
        {
            extract.in(s);
            extract.out(value);
        }
    };

    auto* tap = new bit_tap(sc_core::sc_gen_unique_name("trace_bit_tap"), sig, bit);
    sc_core::sc_trace(tf, tap->value, name);
}

} // namespace gtm_tb

#endif // GTM_TB_TRACING_H_
