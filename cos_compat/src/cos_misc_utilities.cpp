// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - time literals.
#include "sca_basic_libraries/generic_tb_utilities/cos_misc_utilities.h"

namespace sca_basic_libraries_namespace
{

#define COS_COMPAT_TIME_LITERAL(SUFFIX, UNIT)                                                  \
    sc_core::sc_time operator"" SUFFIX(unsigned long long v)                                  \
    {                                                                                          \
        return sc_core::sc_time(static_cast<double>(v), UNIT);                                 \
    }                                                                                          \
    sc_core::sc_time operator"" SUFFIX(long double v)                                         \
    {                                                                                          \
        return sc_core::sc_time(static_cast<double>(v), UNIT);                                 \
    }

COS_COMPAT_TIME_LITERAL(_SC_FS, sc_core::SC_FS)
COS_COMPAT_TIME_LITERAL(_SC_PS, sc_core::SC_PS)
COS_COMPAT_TIME_LITERAL(_SC_NS, sc_core::SC_NS)
COS_COMPAT_TIME_LITERAL(_SC_US, sc_core::SC_US)
COS_COMPAT_TIME_LITERAL(_SC_MS, sc_core::SC_MS)
COS_COMPAT_TIME_LITERAL(_SC_SEC, sc_core::SC_SEC)

#undef COS_COMPAT_TIME_LITERAL

} // namespace sca_basic_libraries_namespace
