// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// gtm_tb - configuration presets for the cos_gtm_c1 GTM top module.
#ifndef GTM_TB_GTM_CONFIG_H_
#define GTM_TB_GTM_CONFIG_H_

#include "cos_gtm_lib/cos_lib/gtm_irq_port_enum.h"
#include "cos_gtm_lib/cos_lib/vendor_dev_cfg.h"

namespace gtm_tb
{

/** Generic ("umbrella") device: trigger chains of 4 ATOM / 8 TOM channels. */
vendor_dev_cfg_type umbrella_dev_cfg();

/** Infineon-style device configuration. */
vendor_dev_cfg_type ifx_dev_cfg();

/**
 * Interrupt mapping of all GTM interrupt ports to consecutive interrupt
 * numbers (bits of a port get consecutive numbers). Uses 692 of the 1024
 * interrupt lines of the GTM interrupt controller.
 */
cos_gtm_port_irq_table_type linear_port_irq_table();

/** First interrupt number assigned to (port, instance) in linear_port_irq_table(). */
int linear_irq_number(gtm_irq_port_enum port, int instance = 0, int bit = 0);

} // namespace gtm_tb

#endif // GTM_TB_GTM_CONFIG_H_
