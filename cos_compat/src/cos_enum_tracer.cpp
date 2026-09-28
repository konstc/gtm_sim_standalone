// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - enum tracing (see cos_enum_tracer.h).
#include "sca_basic_libraries/generic_tb_utilities/cos_enum_tracer.h"

#include <algorithm>

namespace sca_basic_libraries_namespace
{

#if defined(__x86_64__)
static_assert(sizeof(cos_sc_enum_tracer_base) == 0x80, "cos_sc_enum_tracer_base layout");
#endif

namespace
{
int default_enum_trace_characters = 16;
}

int cos_get_default_number_of_enum_trace_characters() { return default_enum_trace_characters; }

void cos_set_default_number_of_enum_trace_characters(int n)
{
    default_enum_trace_characters = std::max(1, n);
}

// ---------------------------------------------------------------------------
cos_sc_trace_preprocessor_base::cos_sc_trace_preprocessor_base()
    : sc_core::sc_object(sc_core::sc_gen_unique_name("cos_trace_preprocessor"))
{
    // SC_END_OF_UPDATE runs after every update phase, i.e. before the trace
    // files sample their values at SC_BEFORE_TIMESTEP.
    register_simulation_phase_callback(sc_core::SC_END_OF_UPDATE);
}

cos_sc_trace_preprocessor_base::~cos_sc_trace_preprocessor_base() {}

void cos_sc_trace_preprocessor_base::simulation_phase_callback()
{
    trace_preprocess();
}

// ---------------------------------------------------------------------------
cos_sc_enum_tracer_base::cos_sc_enum_tracer_base(sc_core::sc_trace_file* tf, const char* nm,
                                                 int max_length)
    : m_value(new sc_dt::sc_unsigned(8 * std::max(1, max_length))),
      m_max_length(std::max(1, max_length))
{
    *m_value = 0;
    if (tf) sc_core::sc_trace(tf, *m_value, nm ? nm : name());
}

cos_sc_enum_tracer_base::~cos_sc_enum_tracer_base()
{
    delete m_value;
}

void cos_sc_enum_tracer_base::update()
{
    const std::string str = convert_to_string();
    const std::size_t length = std::min<std::size_t>(str.size(), m_max_length);

    sc_dt::sc_unsigned value(8 * m_max_length);
    value = 0;
    for (std::size_t i = 0; i < length; ++i)
    {
        value <<= 8;
        value += static_cast<unsigned char>(str[i]);
    }
    *m_value = value;
}

} // namespace sca_basic_libraries_namespace
