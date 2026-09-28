// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// TOM PWM + interrupt example - GTM application and interrupt service routine.
//
//   - CMU_FXCLK0 = 200 MHz (5 ns) clocks TOM0 channel 0
//   - PWM: period 100 ticks (500 ns), duty cycle 25 ticks (125 ns)
//   - the CCU0 (period end) interrupt is enabled; the ISR acknowledges it,
//     doubles the duty cycle after 5 interrupts (via the shadow register, so
//     the change takes effect with the next period) and disables the
//     interrupt after 10 interrupts
#include "tom_pwm_irq_app.h"

#include <cos_gtm_lib/gal/gal_app.h>

namespace tom_pwm_irq
{

namespace
{
const uint32_t CCU0TC = 0x1;  // IRQ_NOTIFY/IRQ_EN bit: CN0 reached CM0 (period end)
int isr_calls = 0;
int serviced = 0;
} // namespace

void program()
{
    gal_wait_ps(100000);
    gal_msg_set_mode(MSG_MODE_USE_INFO | MSG_MODE_USE_WARNING | MSG_MODE_USE_TIME_STAMP);
    GAL_INFO("TOM PWM + interrupt example");

    // software reset, cluster 0 clock divider 1
    GTM.CLS[0].ARCH.CTRL = 0x0;
    GTM.CLS[0].ARCH.RST = 0x1;
    GTM.CLS[0].ARCH.CTRL = 0x1;
    GTM.CLS[0].ARCH.CTRL = 0x0;
    GTM.CLS[0].ARCH.CLK_CFG = 0x1;

    // CMU: fixed clocks from the global clock, enable FXCLK
    GTM.CLS[0].CMU.FXCLK_CTRL = 0x0;
    GTM.CLS[0].CMU.CLK_EN = 0x00800000;  // EN_FXCLK = 0b10

    // TOM0 channel 0: FXCLK0, SL = 1
    GTM.CLS[0].TOM.TGC[0].GLB_CTRL = 0x00020000;  // UPEN_CTRL0: SR -> CM update
    GTM.CLS[0].TOM.CH[0].CTRL_SR = 0x00000000;    // CLK_SRC_SR = FXCLK0
    GTM.CLS[0].TOM.CH[0].CTRL = 0x00000800;       // CLK_SRC = FXCLK0, SL = 1
    GTM.CLS[0].TOM.CH[0].SR0 = 100;               // period
    GTM.CLS[0].TOM.CH[0].SR1 = 25;                // duty cycle
    GTM.CLS[0].TOM.CH[0].IRQ_NOTIFY = CCU0TC;     // clear stale notification
    GTM.CLS[0].TOM.CH[0].IRQ_EN = CCU0TC;

    GAL_INFO("start TOM0 channel 0");
    GTM.CLS[0].TOM.TGC[0].ENDIS_CTRL = 0x2;
    GTM.CLS[0].TOM.TGC[0].OUTEN_CTRL = 0x2;
    GTM.CLS[0].TOM.TGC[0].GLB_CTRL = 0x00000001;  // host trigger

    gal_wait(8);  // us

    GAL_INFO("ISR called %d times, %d CCU0 interrupts serviced", isr_calls, serviced);
}

void isr()
{
    ++isr_calls;
    const uint32_t notify = GTM.CLS[0].TOM.CH[0].IRQ_NOTIFY;
    if (!(notify & CCU0TC)) return;

    GTM.CLS[0].TOM.CH[0].IRQ_NOTIFY = CCU0TC;  // acknowledge
    ++serviced;

    if (serviced == 5)
    {
        GAL_INFO("interrupt %d: duty cycle 25 -> 50 ticks", serviced);
        GTM.CLS[0].TOM.CH[0].SR1 = 50;
    }
    else if (serviced == 10)
    {
        GAL_INFO("interrupt %d: disabling the interrupt", serviced);
        GTM.CLS[0].TOM.CH[0].IRQ_EN = 0;
    }
}

} // namespace tom_pwm_irq
