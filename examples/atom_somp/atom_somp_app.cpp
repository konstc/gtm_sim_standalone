// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// ATOM SOMP example - GTM application.
//
// Based on the ATOM SOMP example (atom_somp.cpp, atom_somp_program.cpp) of the
// Bosch GTM SystemC model distributed by COSEDA Technologies GmbH under the
// Apache License 2.0: same register configuration sequence, restructured and
// with a selectable LAB0/LAB1 variant.
//
// Two ATOM0 channels generate PWM signals in SOMP mode:
//   - CMU_CLK0 = cluster clock / 1 (200 MHz, 5 ns) drives channel 0
//   - CMU_CLK1 = cluster clock / 5 ( 40 MHz, 25 ns) drives channel 1
//   - both start synchronously with period 15 / duty 10 clock ticks
//   - after 2 us the shadow registers are changed to period 25 / duty 12;
//     the operation registers take them over at the end of the running period
//
// Register accesses go through the GAL register structures (GTM.CLS[..]...),
// which the GTM controller of the model turns into AEI bus transactions.
#include "atom_somp_app.h"

#include <cos_gtm_lib/gal/gal_app.h>

namespace atom_somp
{

namespace
{
void software_reset()
{
    GTM.CLS[0].ARCH.CTRL = 0x0;
    GTM.CLS[0].ARCH.RST = 0x1;
    GTM.CLS[0].ARCH.CTRL = 0x1;
}

void configure_clocks()
{
    GAL_INFO("configure cluster 0 clock (divider 1), switch off clusters 1..n");
    GTM.CLS[0].ARCH.CTRL = 0x0;
    GTM.CLS[0].ARCH.CLK_CFG = 0x1;
    GAL_INFO("GTM_REV = 0x%08X", (uint32_t)GTM.CLS[0].ARCH.REV);

    GAL_INFO("configure CMU: CMU_CLK0 = clk/1, CMU_CLK1 = clk/5");
    GTM.CLS[0].CMU.CLK_0_CTRL = 0;
    GTM.CLS[0].CMU.CLK_1_CTRL = 4;
    GTM.CLS[0].CMU.CLK_EN = 0x0000000A;  // enable CMU_CLK0 and CMU_CLK1
}

void configure_atom_channels(bool initial_delay)
{
    GAL_INFO("configure ATOM0 channels 0 and 1 (SOMP)");

    // Enable update of the operation registers (SR0/SR1 -> CM0/CM1) for ch0/1.
    GTM.CLS[0].ATOM.AGC.GLB_CTRL = 0x000A0000;

    GTM.CLS[0].ATOM.CH[0].CTRL_SR = 0x00000000;  // CLK_SRC_SR = CMU_CLK0
    GTM.CLS[0].ATOM.CH[0].CTRL = 0x00000002;     // CLK_SRC = CMU_CLK0, MODE = SOMP
    GTM.CLS[0].ATOM.CH[1].CTRL_SR = 0x00001000;  // CLK_SRC_SR = CMU_CLK1
    GTM.CLS[0].ATOM.CH[1].CTRL = 0x00001002;     // CLK_SRC = CMU_CLK1, MODE = SOMP

    if (initial_delay)
    {
        GTM.CLS[0].ATOM.CH[0].CM0 = 150;
        GTM.CLS[0].ATOM.CH[0].CM1 = 100;
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        GTM.CLS[0].ATOM.CH[ch].SR1 = 10;  // duty cycle
        GTM.CLS[0].ATOM.CH[ch].SR0 = 15;  // period
    }
}

void start_channels()
{
    GAL_INFO("start ATOM0 channels 0 and 1");
    GTM.CLS[0].ATOM.AGC.OUTEN_STAT = 0x0000000A;  // enable outputs of ch0/1
    GTM.CLS[0].ATOM.AGC.ENDIS_CTRL = 0x0000000A;  // enable ch0/1 on trigger
    GTM.CLS[0].ATOM.AGC.GLB_CTRL = 0x00000001;    // host trigger
}

void change_pwm()
{
    GAL_INFO("change PWM: period 25, duty cycle 12");
    for (int ch = 0; ch < 2; ++ch)
    {
        GTM.CLS[0].ATOM.CH[ch].SR0 = 25;
        GTM.CLS[0].ATOM.CH[ch].SR1 = 12;
    }
}
} // namespace

void program(const app_config& cfg)
{
    gal_wait_ps(100000);  // let the reset (released at 12 ns) settle

    gal_msg_set_mode(MSG_MODE_USE_INFO | MSG_MODE_USE_DEBUG | MSG_MODE_USE_WARNING
                     | MSG_MODE_USE_TIME_STAMP);

    GAL_INFO("ATOM SOMP example (%s)", cfg.initial_delay ? "LAB1: initial delay" : "LAB0");
    software_reset();
    configure_clocks();
    configure_atom_channels(cfg.initial_delay);
    start_channels();

    gal_wait(2);  // us
    change_pwm();
    gal_wait(4);

    GAL_INFO("application finished");
}

void isr()
{
    GAL_INFO("interrupt service routine");
}

} // namespace atom_somp
