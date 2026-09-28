// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - replacement for the COSIDE sc_vector_n extension.
//
// sc_core::sc_vector_n<T, N> is an sc_core::sc_vector<T> that
//   - is created with N elements (N > 0) or stays empty (N == 0), and
//   - supports binding to other vectors before their size is known: the size
//     propagates through bound vectors via cos_connectivity_elaborator, and
//     the element-wise binding happens as soon as a size is available.
//
// ABI (must match the prebuilt GTM libraries, x86_64):
//   0x00 sc_vector<T>                    (0x90 bytes)
//   0x90 cos_connectivity_elaborator     (0x10 bytes)
//   0xa0 std::function<void(size_t)>     (0x20 bytes, init callback)
//   sizeof == 0xc0
#ifndef COS_COMPAT_SC_VECTOR_N_H_
#define COS_COMPAT_SC_VECTOR_N_H_

#include <systemc>
#include <functional>
#include <sstream>

#include "sca_basic_libraries/generic_tb_utilities/cos_connectivity_elaborator.h"

namespace sc_core
{

/** Elaboration payload of sc_vector_n: the size of the elaborated vector. */
struct elaboration_data
    : public sca_basic_libraries_namespace::cos_connectivity_elaborator::elaboration_data_base
{
    explicit elaboration_data(std::size_t n = 0) : size(n) {}
    std::size_t size;
};

template <class T, int N = 0>
class sc_vector_n : public sc_vector<T>
{
public:
    typedef sc_vector<T>                     base_type;
    typedef typename base_type::iterator     iterator;
    typedef typename base_type::size_type    size_type;

    sc_vector_n() : base_type(sc_gen_unique_name("vector")) { init_default(); }

    explicit sc_vector_n(const char* prefix) : base_type(prefix) { init_default(); }

    sc_vector_n(const char* prefix, size_type n) : base_type(prefix) { init(n); }

    /** Callback invoked with the size once the vector has been created. */
    void set_init_callback(std::function<void(std::size_t)> cb) { m_init_callback = cb; }

    void init(size_type n) { init(n, &base_type::create_element); }

    template <typename Creator>
    void init(size_type n, Creator c)
    {
        if (this->size() != 0)
        {
            if (n != this->size())
            {
                std::ostringstream str;
                str << "sc_vector_n '" << this->name() << "': re-initialization with size " << n
                    << " (current size: " << this->size() << ")";
                SC_REPORT_ERROR("sc_vector_n", str.str().c_str());
            }
            return;
        }

        create_elements(n, c);

        elaboration_data data(this->size());
        m_elaborator.elaborate(data, *this);
        if (m_init_callback) m_init_callback(n);
    }

    /** Binds this vector element-wise to another vector (sizes may be unknown yet). */
    template <typename Other>
    iterator bind(Other& other)
    {
        m_elaborator.set_elaboration_function(
            [&other, this](sca_basic_libraries_namespace::cos_connectivity_elaborator::elaboration_data_base& d)
            { this->elaborate_binding(other, static_cast<elaboration_data&>(d).size); });
        m_elaborator.bind(*this, other);

        if (this->size() != 0)
        {
            elaboration_data data(this->size());
            m_elaborator.elaborate(data);
        }
        else if (other.size() != 0)
        {
            elaboration_data data(other.size());
            m_elaborator.elaborate(data);
        }
        return this->end();
    }

    template <typename Other>
    iterator operator()(Other& other) { return bind(other); }

private:
    void init_default()
    {
        if (N > 0) init(static_cast<size_type>(N));
    }

    template <typename Creator>
    void create_elements(size_type n, Creator c)
    {
        // Elements belong to the vector's parent module, even when the vector
        // gets sized late (e.g. during size propagation at elaboration).
        sc_module* parent = dynamic_cast<sc_module*>(this->get_parent_object());
        if (parent)
        {
            sc_get_curr_simcontext()->hierarchy_push(parent);
            base_type::init(n, c);
            sc_get_curr_simcontext()->hierarchy_pop();
        }
        else
        {
            base_type::init(n, c);
        }
    }

    template <typename Other>
    void elaborate_binding(Other& other, std::size_t n)
    {
        if (other.size() == 0)
            other.init(n);
        else if (other.size() != n)
            report_size_mismatch(other.name(), other.size(), n);

        if (this->size() == 0)
        {
            create_elements(n, &base_type::create_element);
            elaboration_data data(this->size());
            m_elaborator.elaborate(data, *this);
            if (m_init_callback) m_init_callback(n);
        }
        else if (this->size() != n)
        {
            report_size_mismatch(this->name(), this->size(), n);
        }

        base_type::bind(other);
    }

    void report_size_mismatch(const char* nm, std::size_t actual, std::size_t expected)
    {
        std::ostringstream str;
        str << "sc_vector_n binding size mismatch: '" << nm << "' (" << actual
            << ") expected size: " << expected;
        SC_REPORT_ERROR("sc_vector_n", str.str().c_str());
    }

    sca_basic_libraries_namespace::cos_connectivity_elaborator m_elaborator;
    std::function<void(std::size_t)> m_init_callback;
};

} // namespace sc_core

#endif // COS_COMPAT_SC_VECTOR_N_H_
