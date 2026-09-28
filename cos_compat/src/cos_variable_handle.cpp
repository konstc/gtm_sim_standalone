// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - base class of the COSIDE variable handles, which make plain
// C++ member variables of the GTM model visible (and traceable) as sc_objects.
// The class declaration is taken from the bundle to keep its exact layout.
#include "cos_utilities/cos_sc_variable_handle.h"  // from <bundle>/cos_gtm_lib

namespace sca_basic_libraries_namespace
{

cos_sc_variable_handle_base::cos_sc_variable_handle_base(const char* name)
    : sc_core::sc_object(name)
{
}

cos_sc_variable_handle_base::~cos_sc_variable_handle_base() {}

std::string cos_sc_variable_handle_base::to_string() const { return std::string(); }

bool cos_sc_variable_handle_base::from_string(const std::string&) { return true; }

std::size_t cos_sc_variable_handle_base::get_size() const { return 0; }

void* cos_sc_variable_handle_base::get_memory_ref() const { return nullptr; }

bool cos_sc_variable_handle_base::trace_var_sc(sc_core::sc_trace_file*, const std::string&) const
{
    return true;  // not traceable
}

const char* cos_sc_variable_handle_base::kind() const { return "cos_sc_variable_handle"; }

void cos_sc_variable_handle_base::print(std::ostream& os) const { os << to_string(); }

void cos_sc_variable_handle_base::dump(std::ostream& os) const
{
    sc_core::sc_object::dump(os);
    os << "value = " << to_string() << std::endl;
}

void cos_sc_variable_handle_base::trace_sc(sc_core::sc_trace_file* tf, const std::string& nm) const
{
    trace_var_sc(tf, nm.empty() ? std::string(name()) : nm);
}

} // namespace sca_basic_libraries_namespace
