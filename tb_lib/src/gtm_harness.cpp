// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// gtm_tb - GTM test harness (see gtm_harness.h).
#include "gtm_tb/gtm_harness.h"

namespace gtm_tb
{

gtm_harness::gtm_harness(sc_core::sc_module_name nm, const params& p) : sc_core::sc_module(nm)
{
    cos_gtm_c1::params gp;
    gp.test_program = p.test_program;
    gp.isr_func = p.isr_func;
    gp.isr_priority_func = p.isr_priority_func;
    gp.vendor_dev_cfg = p.vendor_dev_cfg;
    gp.port_irq_table = p.port_irq_table;
    i_cos_gtm.reset(new cos_gtm_c1("i_cos_gtm", gp));

    i_const_src_sc1.reset(new constant_source<std::uint64_t>("i_const_src_sc1", p.clock_period_ps));
    i_const_src_sc1->out(gtm_clk_period);

    i_pwc_src_sc1.reset(new pwc_source<bool>("i_pwc_src_sc1", false, {{p.reset_release, true}}));
    i_pwc_src_sc1->out(gtm_rst_n);

    cos_gtm_c1& g = *i_cos_gtm;
    g.adc0_ready(s_ready_adc0);
    g.adc0_result(s_result_adc0);
    g.adc1_ready(s_ready_adc1);
    g.adc1_result(s_result_adc1);
    g.clk(gtm_clk_period);
    g.rst_n(gtm_rst_n);
    g.dbg_mcs0_hbp_irq(dbg_mcs0_hbp_irq);
    g.gtm_aei_irq(gtm_aei_irq);
    g.gtm_aru_irq(gtm_aru_irq);
    g.gtm_atom0_irq(gtm_atom0_irq);
    g.gtm_atom0_out(gtm_atom0_out);
    g.gtm_atom0_out_n(gtm_atom0_out_n);
    g.gtm_brc_irq(gtm_brc_irq);
    g.gtm_cdtm0_dtm0_aux_in(gtm_cdtm0_aux_in[0]);
    g.gtm_cdtm0_dtm1_aux_in(gtm_cdtm0_aux_in[1]);
    g.gtm_cdtm0_dtm2_aux_in(gtm_cdtm0_aux_in[2]);
    g.gtm_cdtm0_dtm3_aux_in(gtm_cdtm0_aux_in[3]);
    g.gtm_cdtm0_dtm4_aux_in(gtm_cdtm0_aux_in[4]);
    g.gtm_cdtm0_dtm5_aux_in(gtm_cdtm0_aux_in[5]);
    g.gtm_cdtm0_dtm6_aux_in(gtm_cdtm0_aux_in[6]);
    g.gtm_cdtm0_dtm7_aux_in(gtm_cdtm0_aux_in[7]);
    g.gtm_cmp_irq(gtm_cmp_irq);
    g.gtm_dpll_irq(gtm_dpll_irq);
    g.gtm_err_irq(gtm_err_irq);
    g.gtm_mcs0_irq(gtm_mcs0_irq);
    g.gtm_mcs0_s_irq(gtm_mcs0_s_irq);
    g.gtm_psm0_irq(gtm_psm0_irq);
    g.gtm_spe0_irq(gtm_spe0_irq);
    g.gtm_tim0_in(gtm_tim0_in);
    g.gtm_tim0_irq(gtm_tim0_irq);
    g.gtm_tio0_g0_in(gtm_tio0_g0_in);
    g.gtm_tio0_g0_irq(gtm_tio0_g0_irq);
    g.gtm_tio0_g0_out(gtm_tio0_g0_out);
    g.gtm_tio0_g0_out_n(gtm_tio0_g0_out_n);
    g.gtm_tom0_irq(gtm_tom0_irq);
    g.gtm_tom0_out(gtm_tom0_out);
    g.gtm_tom0_out_n(gtm_tom0_out_n);
}

} // namespace gtm_tb
