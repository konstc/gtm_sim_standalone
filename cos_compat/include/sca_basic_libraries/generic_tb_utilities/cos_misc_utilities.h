// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - small COSIDE utilities: time literals and channel access.
#ifndef COS_COMPAT_COS_MISC_UTILITIES_H_
#define COS_COMPAT_COS_MISC_UTILITIES_H_

#include <systemc>

namespace sca_basic_libraries_namespace
{

/** Calls the protected sc_prim_channel::request_update(). */
void request_channel_update(sc_core::sc_prim_channel& channel);

// Time literals: 12_SC_NS, 5_SC_US, ...
sc_core::sc_time operator"" _SC_FS(unsigned long long v);
sc_core::sc_time operator"" _SC_PS(unsigned long long v);
sc_core::sc_time operator"" _SC_NS(unsigned long long v);
sc_core::sc_time operator"" _SC_US(unsigned long long v);
sc_core::sc_time operator"" _SC_MS(unsigned long long v);
sc_core::sc_time operator"" _SC_SEC(unsigned long long v);
sc_core::sc_time operator"" _SC_FS(long double v);
sc_core::sc_time operator"" _SC_PS(long double v);
sc_core::sc_time operator"" _SC_NS(long double v);
sc_core::sc_time operator"" _SC_US(long double v);
sc_core::sc_time operator"" _SC_MS(long double v);
sc_core::sc_time operator"" _SC_SEC(long double v);

} // namespace sca_basic_libraries_namespace

using sca_basic_libraries_namespace::operator"" _SC_FS;
using sca_basic_libraries_namespace::operator"" _SC_PS;
using sca_basic_libraries_namespace::operator"" _SC_NS;
using sca_basic_libraries_namespace::operator"" _SC_US;
using sca_basic_libraries_namespace::operator"" _SC_MS;
using sca_basic_libraries_namespace::operator"" _SC_SEC;

#endif // COS_COMPAT_COS_MISC_UTILITIES_H_
