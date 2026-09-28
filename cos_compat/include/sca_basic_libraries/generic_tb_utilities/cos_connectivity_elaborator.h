// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - replacement for the COSIDE connectivity elaborator.
//
// The elaborator implements deferred, size-propagating binding of
// sc_core::sc_vector_n instances: when two vectors are bound and one of them
// learns its size (via init()), the registered elaboration function of every
// pending binding that involves this vector is executed exactly once. That
// function sizes the other side and binds the elements.
//
// ABI: objects of this class are embedded by value in sc_vector_n (16 bytes,
// at offset 0x90 on x86_64) inside the prebuilt GTM libraries. All member
// functions are out-of-line, so only the size must be kept.
#ifndef COS_COMPAT_COS_CONNECTIVITY_ELABORATOR_H_
#define COS_COMPAT_COS_CONNECTIVITY_ELABORATOR_H_

#include <systemc>
#include <functional>

namespace sca_basic_libraries_namespace
{

class cos_connectivity_elaborator
{
public:
    /** Polymorphic payload passed to elaboration functions (vector size, ...). */
    struct elaboration_data_base
    {
        virtual ~elaboration_data_base() {}
    };

    typedef std::function<void(elaboration_data_base&)> elaboration_function;

    cos_connectivity_elaborator();
    ~cos_connectivity_elaborator();

    /** Sets the function the next bind() registers. */
    void set_elaboration_function(elaboration_function func);

    /** Registers a pending binding between two objects (usually vectors). */
    void bind(sc_core::sc_object& obj, sc_core::sc_object& bound_obj);

    /** Executes the most recent pending binding of this elaborator. */
    void elaborate(elaboration_data_base& data);

    /** Notifies all pending bindings involving obj (obj just got elaborated). */
    void elaborate(elaboration_data_base& data, sc_core::sc_object& obj);

    struct impl;

private:
    cos_connectivity_elaborator(const cos_connectivity_elaborator&);
    cos_connectivity_elaborator& operator=(const cos_connectivity_elaborator&);

    impl* m_impl;
    void* m_reserved;
};

} // namespace sca_basic_libraries_namespace

#endif // COS_COMPAT_COS_CONNECTIVITY_ELABORATOR_H_
