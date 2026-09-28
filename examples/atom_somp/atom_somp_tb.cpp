// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// ATOM SOMP example - test bench netlist (see atom_somp_tb.h).
#include "atom_somp_tb.h"

using namespace sc_core;
using sc_dt::sc_bv;

atom_somp_tb::atom_somp_tb(sc_module_name nm, const params& p) : gtm_tb::gtm_harness(nm, p)
{
    // Pattern for TIM0 inputs 5..2.
    const std::vector<gtm_tb::pwc_source<sc_bv<4> >::point> pattern = {
        {sc_time(1, SC_US), 0x00},  {sc_time(5, SC_US), 0x55},  {sc_time(6, SC_US), 0x2A},
        {sc_time(7, SC_US), 0x55},  {sc_time(8, SC_US), 0x2A},  {sc_time(9, SC_US), 0x55},
        {sc_time(10, SC_US), 0x2A}, {sc_time(11, SC_US), 0x55}, {sc_time(12, SC_US), 0x2A},
        {sc_time(13, SC_US), 0x55}};
    i_pwc_src_sc2.reset(new gtm_tb::pwc_source<sc_bv<4> >("i_pwc_src_sc2", 42, pattern));
    i_pwc_src_sc2->out(tim0_in_5_2);

    // ATOM0 outputs 7/6 looped back to TIM0 inputs 7/1.
    i_conv_de1.reset(new gtm_tb::bit_extract<8>("i_conv_de1", 7));
    i_conv_de1->in(gtm_atom0_out);
    i_conv_de1->out(tim0_in_7);
    i_conv_de2.reset(new gtm_tb::bit_extract<8>("i_conv_de2", 6));
    i_conv_de2->in(gtm_atom0_out);
    i_conv_de2->out(tim0_in_1);

    i_tim0_in_merge.reset(new gtm_tb::bv_combiner<8>("i_tim0_in_merge"));
    i_tim0_in_merge->connect_bit(tim0_in_7, 7);
    i_tim0_in_merge->connect_bit(tim0_in_1, 1);
    i_tim0_in_merge->connect_range(tim0_in_5_2, 2);
    i_tim0_in_merge->out(gtm_tim0_in);
}
