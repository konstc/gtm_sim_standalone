// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// ATOM SOMP example - GTM application (runs on the GTM controller thread).
#ifndef EXAMPLES_ATOM_SOMP_APP_H_
#define EXAMPLES_ATOM_SOMP_APP_H_

namespace atom_somp
{

struct app_config
{
    /** LAB1: channel 0 starts with an initial delay (CM0=150, CM1=100). */
    bool initial_delay = false;
};

/** GTM application: configures ATOM0 channels 0/1 in SOMP (PWM) mode. */
void program(const app_config& cfg);

/** Interrupt service routine. */
void isr();

} // namespace atom_somp

#endif // EXAMPLES_ATOM_SOMP_APP_H_
