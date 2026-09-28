// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// TOM PWM + interrupt example - simulation entry point.
//
//   tom_pwm_irq [--sim_time=1e-5] [--vcd=tom_pwm_irq]
#include <systemc>
#include <cstdlib>

#include "tom_pwm_irq_app.h"

#include "cos_gtm_lib/traces/gtm_trace_functions.h"
#include "gtm_tb/cmdline.h"
#include "gtm_tb/gtm_harness.h"
#include "gtm_tb/tracing.h"
#include "sca_basic_libraries/coside_utilities.h"

int sc_main(int argc, char* argv[])
{
    sc_core::sc_set_time_resolution(1.0, sc_core::SC_PS);

    double sim_time = 10e-6;
    std::string vcd_name = "tom_pwm_irq";
    gtm_tb::cmdline options("TOM PWM example with period interrupt and ISR-driven duty-cycle update");
    options.add("sim_time", sim_time, "simulation time in seconds");
    options.add("vcd", vcd_name, "VCD file name (without extension)");
    if (!options.parse(argc, argv)) return EXIT_FAILURE;

    gtm_tb::gtm_harness::params p;
    p.test_program = tom_pwm_irq::program;
    p.isr_func = tom_pwm_irq::isr;
    gtm_tb::gtm_harness tb("i_tom_pwm_irq_tb", p);

    sc_core::sc_trace_file* tf = sc_core::sc_create_vcd_trace_file(vcd_name.c_str());
    trace_tom(tf, 0);
    trace_tom_ch(tf, 0, 0, true);
    gtm_tb::trace_bit(tf, tb.gtm_tom0_out, 0, "TOM0_OUT(0)");
    gtm_tb::trace_bit(tf, tb.gtm_tom0_irq, 0, "TOM0_IRQ(0)");
    sc_object_trace(tf, "*i_cos_gtm*.*i_cos_irq_ctrl.irqs", "irqs");

    sc_core::sc_start(sim_time, sc_core::SC_SEC);
    if (sc_core::sc_is_running()) sc_core::sc_stop();
    sc_core::sc_close_vcd_trace_file(tf);
    return sc_core::sc_report_handler::get_count(sc_core::SC_ERROR) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
