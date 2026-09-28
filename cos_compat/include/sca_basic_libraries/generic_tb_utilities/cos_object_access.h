// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - object search and trace-by-name utilities replacing the
// COSIDE sca_basic_libraries object access API:
//
//   get_matching_objects<T>("*pattern*")   all objects of type T whose full
//                                          hierarchical name matches the glob
//   GET_OBJECT<T>("*pattern*")             first match (error if none)
//   sc_object_trace(tf, "*pattern*" [, "display name"])
//                                          traces signals, ports and
//                                          registered variables by name
//
// Glob syntax: '*' matches any character sequence (including '.'), '?'
// matches exactly one character.
//
// ABI notes (must match the prebuilt GTM libraries, x86_64):
//   object_searcher_base          : vptr, 2 words                    (0x18 bytes)
//   object_searcher<T>            : + std::vector<T*>*               (0x20 bytes)
//   sca_obj_floc_trace            : std::string, int, std::ifstream  (0x230 bytes)
//   cos_trace_type_registrar_base : vptr, bool                       (0x10 bytes)
#ifndef COS_COMPAT_COS_OBJECT_ACCESS_H_
#define COS_COMPAT_COS_OBJECT_ACCESS_H_

#include <systemc>
#include <fstream>
#include <string>
#include <vector>

namespace sca_util { class sca_trace_file; }

namespace sca_basic_libraries_namespace
{

// ---------------------------------------------------------------------------
// Object search
// ---------------------------------------------------------------------------
class object_searcher_base
{
public:
    /** max_number_of_objects < 0: no limit */
    explicit object_searcher_base(long max_number_of_objects = -1);
    virtual ~object_searcher_base() {}

    /** Visits all objects whose full name matches the glob pattern. */
    void get_matching_objects(const std::string& pattern);

    /** Glob match of a full object name (exposed for tests). */
    static bool match(const char* pattern, const char* name);

protected:
    virtual bool check_type(sc_core::sc_object* obj) = 0;
    virtual void push_back(sc_core::sc_object* obj) = 0;

private:
    void search(const std::vector<sc_core::sc_object*>& objects, const std::string& pattern);

    long m_max_number_of_objects;
    long m_number_of_objects;
};

template <class T>
class object_searcher : public object_searcher_base
{
public:
    explicit object_searcher(std::vector<T*>& result, long max_number_of_objects = -1)
        : object_searcher_base(max_number_of_objects), m_result(&result) {}

protected:
    bool check_type(sc_core::sc_object* obj) override { return dynamic_cast<T*>(obj) != nullptr; }
    void push_back(sc_core::sc_object* obj) override { m_result->push_back(dynamic_cast<T*>(obj)); }

private:
    std::vector<T*>* m_result;
};

template <class T>
std::vector<T*> get_matching_objects(const std::string& pattern)
{
    std::vector<T*> result;
    object_searcher<T> searcher(result);
    searcher.get_matching_objects(pattern);
    return result;
}

// ---------------------------------------------------------------------------
// Source location of the last object access (for diagnostics)
// ---------------------------------------------------------------------------
namespace sca_obj_access_namespace
{
void set_file_location(const char* file, unsigned int line);
const char* get_filename();
int get_lineno();
} // namespace sca_obj_access_namespace

/** Returns the first object of type T matching the pattern (see GET_OBJECT). */
template <class T>
T* cos_compat_get_object(const std::string& pattern, bool report_error = true)
{
    std::vector<T*> objs;
    object_searcher<T> searcher(objs, 1);
    searcher.get_matching_objects(pattern);
    if (objs.empty())
    {
        if (report_error)
        {
            std::string msg = "no object of the requested type matches '" + pattern + "'";
            sc_core::sc_report_handler::report(sc_core::SC_ERROR, "GET_OBJECT", msg.c_str(),
                                               sca_obj_access_namespace::get_filename(),
                                               sca_obj_access_namespace::get_lineno());
        }
        return nullptr;
    }
    return objs.front();
}

/** Records the call site, then looks up an object (implementation of GET_OBJECT). */
struct cos_compat_object_locator
{
    cos_compat_object_locator(const char* file, int line)
    {
        sca_obj_access_namespace::set_file_location(file, static_cast<unsigned int>(line));
    }

    template <class T>
    T* get_object(const std::string& pattern, bool report_error = true) const
    {
        return cos_compat_get_object<T>(pattern, report_error);
    }
};

// ---------------------------------------------------------------------------
// Tracing of channel types by interface (registrar registry)
// ---------------------------------------------------------------------------
class cos_trace_type_registrar_base
{
public:
    cos_trace_type_registrar_base() : m_registered(false) {}

    virtual void trace_sc(const sc_core::sc_interface* intf, sc_core::sc_trace_file* tf,
                          const std::string& nm) const = 0;
    virtual void trace_sca(const sc_core::sc_interface* intf, sca_util::sca_trace_file* tf,
                           const std::string& nm) const = 0;
    virtual bool is_type(const sc_core::sc_interface* intf) const = 0;
    virtual bool is_same(const cos_trace_type_registrar_base* other) const = 0;
    virtual ~cos_trace_type_registrar_base() {}

    /** True once the registry has taken ownership of this registrar. */
    bool is_registered() const { return m_registered; }

private:
    friend void cos_register_sc_object_trace_type(const cos_trace_type_registrar_base*);
    mutable bool m_registered;
};

/**
 * Registers a channel type for sc_object_trace(). If no equivalent registrar
 * is registered yet, the registry takes ownership (is_registered() == true);
 * otherwise the caller keeps ownership and must delete the registrar.
 */
void cos_register_sc_object_trace_type(const cos_trace_type_registrar_base* registrar);

/** sc_signal_in_if<T> tracer usable with cos_register_sc_object_trace_type(). */
template <class T>
class cos_compat_signal_trace_registrar : public cos_trace_type_registrar_base
{
public:
    void trace_sc(const sc_core::sc_interface* intf, sc_core::sc_trace_file* tf,
                  const std::string& nm) const override
    {
        if (auto sig = dynamic_cast<const sc_core::sc_signal_in_if<T>*>(intf))
            sc_core::sc_trace(tf, sig->read(), nm);
    }
    void trace_sca(const sc_core::sc_interface*, sca_util::sca_trace_file*,
                   const std::string&) const override {}
    bool is_type(const sc_core::sc_interface* intf) const override
    {
        return dynamic_cast<const sc_core::sc_signal_in_if<T>*>(intf) != nullptr;
    }
    bool is_same(const cos_trace_type_registrar_base* other) const override
    {
        return dynamic_cast<const cos_compat_signal_trace_registrar<T>*>(other) != nullptr;
    }
};

// ---------------------------------------------------------------------------
// Trace by name
// ---------------------------------------------------------------------------
class sca_obj_floc_trace
{
public:
    sca_obj_floc_trace(const char* file, int line);

    /** Traces all objects matching pattern; nm replaces the name for a single match. */
    void sc_object_trace_(sc_core::sc_trace_file* tf, const std::string& pattern, const std::string& nm);

    void sc_object_trace_(sc_core::sc_trace_file* tf, const std::string& pattern)
    {
        sc_object_trace_(tf, pattern, std::string());
    }

private:
    std::string m_file;
    int m_line;
    std::ifstream m_source;  // unused; part of the COSIDE object layout
};

} // namespace sca_basic_libraries_namespace

using sca_basic_libraries_namespace::get_matching_objects;

#define GET_OBJECT \
    ::sca_basic_libraries_namespace::cos_compat_object_locator(__FILE__, __LINE__).get_object

#define sc_object_trace(...) \
    ::sca_basic_libraries_namespace::sca_obj_floc_trace(__FILE__, __LINE__).sc_object_trace_(__VA_ARGS__)

#endif // COS_COMPAT_COS_OBJECT_ACCESS_H_
