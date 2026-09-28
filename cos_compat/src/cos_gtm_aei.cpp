// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - AEI access functions expected by gtm_aei_hal (libcos_gtm_lib).
// Kept in a separate translation unit: it references the GTM controller and
// must only be linked in together with the GTM libraries.
#include <cstdint>

// AEI bus access used by gtm_aei_hal (gtm_aei_read/gtm_aei_write). The GTM
// controller in libcos_gtm_lib implements the transactions.
namespace cos_gtm_lib_namespace
{
std::uint32_t gal_aei_read_impl(std::uint32_t addr, std::uint32_t* aei_status);
void gal_aei_write_impl(std::uint32_t addr, std::uint32_t data, std::uint32_t* aei_status);

std::uint32_t aei_read(std::uint32_t addr, std::uint32_t* aei_status)
{
    return gal_aei_read_impl(addr, aei_status);
}

void aei_write(std::uint32_t addr, std::uint32_t data, std::uint32_t* aei_status)
{
    gal_aei_write_impl(addr, data, aei_status);
}
} // namespace cos_gtm_lib_namespace
