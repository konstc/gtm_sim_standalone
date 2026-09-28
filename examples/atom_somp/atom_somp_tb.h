// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// ATOM SOMP example - test bench netlist around the GTM model.
//
// Based on the ATOM SOMP test bench (atom_somp_tb) of the Bosch GTM SystemC
// model distributed by COSEDA Technologies GmbH under the Apache License 2.0:
// instance/signal names and stimulus values are taken from it; the netlist
// is re-written in plain SystemC.
//
// Mirrors the COSIDE schematic atom_somp_tb (same instance/signal names, so
// that trace names match the COSIDE waveforms). On top of the GTM harness
// (GTM, 200 MHz clock, reset released at 12 ns):
//   - i_pwc_src_sc2 : pattern on TIM0 inputs 5..2
//   - ATOM0 outputs 7 and 6 are looped back to TIM0 inputs 7 and 1
#ifndef EXAMPLES_ATOM_SOMP_TB_H_
#define EXAMPLES_ATOM_SOMP_TB_H_

#include <systemc>
#include <memory>

#include "gtm_tb/bit_adapters.h"
#include "gtm_tb/gtm_harness.h"
#include "gtm_tb/sources.h"

class atom_somp_tb : public gtm_tb::gtm_harness
{
public:
    atom_somp_tb(sc_core::sc_module_name nm, const params& p);

    // TIM0 input composition (COSIDE: sc_slice bindings)
    sc_core::sc_signal<bool> tim0_in_7{"tim0_in_7"};
    sc_core::sc_signal<bool> tim0_in_1{"tim0_in_1"};
    sc_core::sc_signal<sc_dt::sc_bv<4> > tim0_in_5_2{"tim0_in_5_2"};

    std::unique_ptr<gtm_tb::pwc_source<sc_dt::sc_bv<4> > > i_pwc_src_sc2;
    std::unique_ptr<gtm_tb::bit_extract<8> > i_conv_de1;
    std::unique_ptr<gtm_tb::bit_extract<8> > i_conv_de2;
    std::unique_ptr<gtm_tb::bv_combiner<8> > i_tim0_in_merge;
};

#endif // EXAMPLES_ATOM_SOMP_TB_H_
