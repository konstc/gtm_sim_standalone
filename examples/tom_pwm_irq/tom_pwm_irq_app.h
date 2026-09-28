// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// TOM PWM + interrupt example - GTM application.
#ifndef EXAMPLES_TOM_PWM_IRQ_APP_H_
#define EXAMPLES_TOM_PWM_IRQ_APP_H_

namespace tom_pwm_irq
{
/** GTM application: TOM0 channel 0 PWM with period interrupt. */
void program();

/** Interrupt service routine (called by the GTM controller). */
void isr();
} // namespace tom_pwm_irq

#endif // EXAMPLES_TOM_PWM_IRQ_APP_H_
