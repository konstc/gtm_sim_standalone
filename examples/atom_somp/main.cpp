// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// ATOM SOMP example - simulation entry point.
//
//   atom_somp [--sim_time=1e-5] [--lab1] [--vcd=atom_somp_tb_object_trace]
//
// Writes <vcd>.vcd with the same traces as the COSIDE example.
#include <systemc>
#include <cstdlib>

#include "atom_somp_app.h"
#include "atom_somp_tb.h"

#include "cos_gtm_lib/gtm_variable_registration/cos_register_gtm_variables.h"
#include "cos_gtm_lib/traces/gtm_trace_functions.h"
#include "gtm_tb/cmdline.h"
#include "gtm_tb/tracing.h"
#include "sca_basic_libraries/coside_utilities.h"

int sc_main(int argc, char* argv[])
{
    sc_core::sc_set_time_resolution(1.0, sc_core::SC_PS);

    double sim_time = 10e-6;
    bool lab1 = false;
    bool secondary = false;
    std::string vcd_name = "atom_somp_tb_object_trace";

    gtm_tb::cmdline options("ATOM SOMP example: two ATOM channels generating PWM signals");
    options.add("sim_time", sim_time, "simulation time in seconds");
    options.add("lab1", lab1, "LAB1: initial delay on channel 0");
    options.add("secondary_variables", secondary, "also register/trace secondary GTM variables");
    options.add("vcd", vcd_name, "VCD file name (without extension)");
    if (!options.parse(argc, argv)) return EXIT_FAILURE;

    if (secondary) cos_enable_gtm_secondary_variables();

    atom_somp::app_config app_cfg;
    app_cfg.initial_delay = lab1;

    atom_somp_tb::params p;
    p.test_program = [app_cfg] { atom_somp::program(app_cfg); };
    p.isr_func = [] { atom_somp::isr(); };
    atom_somp_tb tb("i_atom_somp_tb", p);

    // --- tracing (as in the COSIDE example) -----------------------------------
    sc_core::sc_trace_file* tf = sc_core::sc_create_vcd_trace_file(vcd_name.c_str());

    trace_gtm_ctl(tf);
    trace_gtm_top(tf);
    trace_atom(tf, 0);
    trace_atom_ch(tf, 0, 0, true);
    trace_atom_ch(tf, 0, 1, true);

    for (int i = 0; i < 4; ++i)
        gtm_tb::trace_bit(tf, tb.gtm_atom0_irq, i, "ATOM_IRQ(" + std::to_string(i) + ")");
    gtm_tb::trace_bit(tf, tb.gtm_tim0_in, 1, "TIM0_IN(1)");
    gtm_tb::trace_bit(tf, tb.gtm_tim0_in, 7, "TIM0_IN(7)");
    for (int i = 0; i < 8; ++i)
        gtm_tb::trace_bit(tf, tb.gtm_atom0_out, i, "ATOM0_OUT(" + std::to_string(i) + ")");

    sc_object_trace(tf, "*i_cos_gtm*.*i_cos_irq_ctrl.irqs", "irqs");
    if (auto irq_ctrl = GET_OBJECT<sc_core::sc_object>("*.i_cos_irq_ctrl", false))
        irq_ctrl->trace(tf);  // all inputs of the interrupt controller

    // --- simulation -------------------------------------------------------------
    sc_core::sc_start(sim_time, sc_core::SC_SEC);
    SC_REPORT_INFO("sc_main", ("simulation finished at " + sc_core::sc_time_stamp().to_string()).c_str());

    if (sc_core::sc_is_running()) sc_core::sc_stop();
    sc_core::sc_close_vcd_trace_file(tf);
    return sc_core::sc_report_handler::get_count(sc_core::SC_ERROR) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
