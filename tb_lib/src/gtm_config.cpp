// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// gtm_tb - configuration presets (see gtm_config.h).
#include "gtm_tb/gtm_config.h"

#include <stdexcept>

namespace gtm_tb
{

vendor_dev_cfg_type umbrella_dev_cfg()
{
    vendor_dev_cfg_type cfg = {};
    cfg.irq_mode_rst_val = 0;
    cfg.atom_trig_chain = 1;
    cfg.atom_trig_intchain = 4;
    cfg.tom_trig_chain = 1;
    cfg.tom_trig_intchain = 8;
    cfg.atom_out_rst = 1;
    cfg.tom_out_rst = 1;
    cfg.tio_out_rst = 1;
    return cfg;
}

vendor_dev_cfg_type ifx_dev_cfg()
{
    vendor_dev_cfg_type cfg = umbrella_dev_cfg();
    cfg.irq_mode_rst_val = 2;
    cfg.atom_out_rst = 0;
    cfg.tom_out_rst = 0;
    cfg.tio_out_rst = 0;
    return cfg;
}

namespace
{
struct port_layout
{
    gtm_irq_port_enum port;
    int instances;  // -1: port without instance index
    int bits;       // interrupt lines per instance
};

// Port widths as exposed by the GTM 4.1 model (instance 0 of each module
// family is visible at cos_gtm_c1).
const port_layout layout[] = {
    {GTM_AEI_IRQ, -1, 1},     {GTM_ARU_IRQ, -1, 3},    {GTM_BRC_IRQ, -1, 1},
    {GTM_CMP_IRQ, -1, 1},     {GTM_SPE_IRQ, 6, 1},     {GTM_PSM_IRQ, 3, 8},
    {GTM_DPLL_IRQ, -1, 27},   {GTM_ERR_IRQ, -1, 1},    {GTM_TIM_IRQ, 8, 8},
    {GTM_MCS_IRQ, 10, 8},     {GTM_MCS_S_IRQ, 10, 8},  {GTM_TOM_IRQ, 6, 8},
    {GTM_ATOM_IRQ, 12, 4},    {GTM_TIO_G0_IRQ, 12, 8}, {GTM_TIO_G1_IRQ, 12, 8},
    {GTM_TIO_G2_IRQ, 12, 8},  {DBG_MCS_HBP_IRQ, 10, 2},
};
} // namespace

cos_gtm_port_irq_table_type linear_port_irq_table()
{
    cos_gtm_port_irq_table_type table;
    int next = 0;
    for (const port_layout& l : layout)
    {
        const int n = l.instances < 0 ? 1 : l.instances;
        for (int i = 0; i < n; ++i)
        {
            table.push_back({std::make_tuple(l.port, l.instances < 0 ? -1 : i, -1), next});
            next += l.bits;
        }
    }
    return table;
}

int linear_irq_number(gtm_irq_port_enum port, int instance, int bit)
{
    for (const auto& entry : linear_port_irq_table())
    {
        const auto& key = entry.first;
        if (std::get<0>(key) == port && (std::get<1>(key) == -1 || std::get<1>(key) == instance))
            return entry.second + bit;
    }
    throw std::out_of_range("linear_irq_number: unknown port");
}

} // namespace gtm_tb
