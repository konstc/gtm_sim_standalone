// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// gtm_tb - GTM test harness: the cos_gtm_c1 model with clock, reset and a
// signal for every port. Test benches derive from it (or instantiate it) and
// add their own stimuli / loop-backs to the signals.
//
// Instance and signal names follow the COSIDE example schematics, so traces
// get the same hierarchical names as in COSIDE:
//   <harness>.i_cos_gtm        GTM model (cos_gtm_c1)
//   <harness>.i_const_src_sc1  clock source (the clk port carries the period in ps)
//   <harness>.i_pwc_src_sc1    reset source (rst_n, active low)
#ifndef GTM_TB_GTM_HARNESS_H_
#define GTM_TB_GTM_HARNESS_H_

#include <systemc>
#include <cstdint>
#include <functional>
#include <memory>

#include "cos_gtm_lib/gtm_configurations/cos_gtm_c1.h"
#include "gtm_tb/gtm_config.h"
#include "gtm_tb/sources.h"

namespace gtm_tb
{

class gtm_harness : public sc_core::sc_module
{
public:
    struct params
    {
        std::function<void()> test_program = [] {};  ///< GTM application (controller thread)
        std::function<void()> isr_func = [] {};      ///< interrupt service routine
        std::function<void()> isr_priority_func;     ///< optional
        std::uint64_t clock_period_ps = 5000;        ///< 200 MHz
        sc_core::sc_time reset_release = sc_core::sc_time(12, sc_core::SC_NS);
        vendor_dev_cfg_type vendor_dev_cfg = umbrella_dev_cfg();
        cos_gtm_port_irq_table_type port_irq_table = linear_port_irq_table();
    };

    gtm_harness(sc_core::sc_module_name nm, const params& p);

    // --- GTM port signals ---------------------------------------------------
    sc_core::sc_signal<sc_dt::sc_bv<2> > dbg_mcs0_hbp_irq{"dbg_mcs0_hbp_irq"};
    sc_core::sc_signal<bool> gtm_aei_irq{"gtm_aei_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<3> > gtm_aru_irq{"gtm_aru_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<4> > gtm_atom0_irq{"gtm_atom0_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_atom0_out{"gtm_atom0_out"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_atom0_out_n{"gtm_atom0_out_n"};
    sc_core::sc_signal<bool> gtm_brc_irq{"gtm_brc_irq"};
    sc_core::sc_vector<sc_core::sc_signal<sc_dt::sc_bv<2> > > gtm_cdtm0_aux_in{"gtm_cdtm0_aux_in", 8};
    sc_core::sc_signal<bool> gtm_cmp_irq{"gtm_cmp_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<27> > gtm_dpll_irq{"gtm_dpll_irq"};
    sc_core::sc_signal<bool> gtm_err_irq{"gtm_err_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_mcs0_irq{"gtm_mcs0_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_mcs0_s_irq{"gtm_mcs0_s_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_psm0_irq{"gtm_psm0_irq"};
    sc_core::sc_signal<bool> gtm_spe0_irq{"gtm_spe0_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_tim0_in{"gtm_tim0_in"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_tim0_irq{"gtm_tim0_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_tio0_g0_in{"gtm_tio0_g0_in"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_tio0_g0_irq{"gtm_tio0_g0_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_tio0_g0_out{"gtm_tio0_g0_out"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_tio0_g0_out_n{"gtm_tio0_g0_out_n"};
    sc_core::sc_signal<sc_dt::sc_bv<8> > gtm_tom0_irq{"gtm_tom0_irq"};
    sc_core::sc_signal<sc_dt::sc_bv<16> > gtm_tom0_out{"gtm_tom0_out"};
    sc_core::sc_signal<sc_dt::sc_bv<16> > gtm_tom0_out_n{"gtm_tom0_out_n"};
    sc_core::sc_signal<bool> s_ready_adc0{"s_ready_adc0"};
    sc_core::sc_signal<bool> s_ready_adc1{"s_ready_adc1"};
    sc_core::sc_signal<std::uint32_t> s_result_adc0{"s_result_adc0"};
    sc_core::sc_signal<std::uint32_t> s_result_adc1{"s_result_adc1"};
    sc_core::sc_signal<bool> gtm_rst_n{"gtm_rst_n"};
    sc_core::sc_signal<std::uint64_t> gtm_clk_period{"gtm_clk_period"};

    // --- components -----------------------------------------------------------
    std::unique_ptr<cos_gtm_c1> i_cos_gtm;
    std::unique_ptr<constant_source<std::uint64_t> > i_const_src_sc1;
    std::unique_ptr<pwc_source<bool> > i_pwc_src_sc1;
};

} // namespace gtm_tb

#endif // GTM_TB_GTM_HARNESS_H_
