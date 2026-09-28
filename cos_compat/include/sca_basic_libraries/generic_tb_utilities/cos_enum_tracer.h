// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - tracing of enum values as ASCII strings (COSIDE replacement).
//
// cos_sc_trace_preprocessor_base: sc_object whose trace_preprocess() is
//   called before trace values are sampled (SystemC phase callbacks).
// cos_sc_enum_tracer_base: traces a string (converted from an enum) as an
//   8*max_length bit vector of ASCII characters.
//
// ABI (x86_64): sc_object (0x70) + 16 bytes of enum tracer state = 0x80;
// the prebuilt libraries derive cos_sc_enum_tracer<E> (0x90 bytes) from it.
// Virtual order after the sc_object slots: trace_preprocess, convert_to_string.
// Inheritance as in COSIDE: sc_object <-public- cos_sc_trace_preprocessor_base
//   <-private- cos_sc_enum_tracer_base <-private- cos_sc_enum_tracer<E>.
#ifndef COS_COMPAT_COS_ENUM_TRACER_H_
#define COS_COMPAT_COS_ENUM_TRACER_H_

#include <systemc>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace sca_basic_libraries_namespace
{

class cos_sc_trace_preprocessor_base : public sc_core::sc_object
{
public:
    cos_sc_trace_preprocessor_base();
    virtual ~cos_sc_trace_preprocessor_base();

private:
    virtual void trace_preprocess() = 0;
    void simulation_phase_callback() override;
};

// Private base: matches the COSIDE type information (__vmi_class_type_info
// with a non-public base) contained in the prebuilt libraries.
class cos_sc_enum_tracer_base : private cos_sc_trace_preprocessor_base
{
public:
    cos_sc_enum_tracer_base(sc_core::sc_trace_file* tf, const char* nm, int max_length);
    virtual ~cos_sc_enum_tracer_base();

    /** Re-reads the string and updates the traced bit vector. */
    void update();

private:
    virtual std::string convert_to_string() = 0;

    sc_dt::sc_unsigned* m_value;
    int m_max_length;
};

/** Default number of characters for enum traces. */
int cos_get_default_number_of_enum_trace_characters();
void cos_set_default_number_of_enum_trace_characters(int n);

/**
 * Enum tracer using a user supplied conversion (default: numeric value).
 * The GTM libraries instantiate their own variant (sca_basic_libraries_namespace::
 * cos_sc_enum_tracer<E>); this one has a distinct name to avoid ODR clashes.
 */
template <typename EnumT>
class cos_compat_enum_tracer : private cos_sc_enum_tracer_base
{
public:
    typedef std::string (*converter)(const EnumT&);

    cos_compat_enum_tracer(sc_core::sc_trace_file* tf, const EnumT& val, const char* nm,
                           int max_length, converter conv = nullptr)
        : cos_sc_enum_tracer_base(tf, nm, max_length), m_val(&val), m_last(val), m_conv(conv)
    {
        update();
    }

private:
    void trace_preprocess() override
    {
        if (m_last != *m_val)
        {
            m_last = *m_val;
            update();
        }
    }

    std::string convert_to_string() override
    {
        if (m_conv) return m_conv(*m_val);
        return std::to_string(static_cast<long long>(*m_val));
    }

    const EnumT* m_val;
    EnumT m_last;
    converter m_conv;
};

} // namespace sca_basic_libraries_namespace

#endif // COS_COMPAT_COS_ENUM_TRACER_H_
