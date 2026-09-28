// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - object search and trace-by-name (see cos_object_access.h).
#include "sca_basic_libraries/generic_tb_utilities/cos_object_access.h"
#include "sca_basic_libraries/generic_tb_utilities/cos_sc_traceable_object.h"

#include <cstdint>
#include <utility>

namespace sca_basic_libraries_namespace
{

// object_searcher_base holds two 'long' counters: 32-bit on Windows (LLP64),
// 64-bit on Linux (LP64) - the prebuilt libraries follow the same rule.
static_assert(sizeof(object_searcher<sc_core::sc_object>) == (sizeof(long) == 4 ? 0x18 : 0x20),
              "object_searcher layout");
static_assert(sizeof(cos_trace_type_registrar_base) == 0x10, "cos_trace_type_registrar_base layout");
// std::string + int + std::ifstream; libstdc++'s filebuf is smaller on MinGW.
#if defined(__x86_64__) && defined(__GLIBCXX__) && defined(_WIN32)
static_assert(sizeof(sca_obj_floc_trace) == 0x200, "sca_obj_floc_trace layout");
#elif defined(__x86_64__) && defined(__GLIBCXX__)
static_assert(sizeof(sca_obj_floc_trace) == 0x230, "sca_obj_floc_trace layout");
#endif

// ---------------------------------------------------------------------------
// object_searcher_base
// ---------------------------------------------------------------------------
object_searcher_base::object_searcher_base(long max_number_of_objects)
    : m_max_number_of_objects(max_number_of_objects), m_number_of_objects(0)
{
}

bool object_searcher_base::match(const char* pattern, const char* name)
{
    // Iterative glob matching with single-star backtracking.
    const char* star = nullptr;
    const char* resume = nullptr;
    while (*name)
    {
        if (*pattern == '*')
        {
            star = pattern++;
            resume = name;
        }
        else if (*pattern == '?' || *pattern == *name)
        {
            ++pattern;
            ++name;
        }
        else if (star)
        {
            pattern = star + 1;
            name = ++resume;
        }
        else
        {
            return false;
        }
    }
    while (*pattern == '*') ++pattern;
    return *pattern == '\0';
}

void object_searcher_base::search(const std::vector<sc_core::sc_object*>& objects,
                                  const std::string& pattern)
{
    for (sc_core::sc_object* obj : objects)
    {
        if (m_max_number_of_objects >= 0 && m_number_of_objects >= m_max_number_of_objects) return;
        if (match(pattern.c_str(), obj->name()) && check_type(obj))
        {
            push_back(obj);
            ++m_number_of_objects;
        }
        search(obj->get_child_objects(), pattern);
    }
}

void object_searcher_base::get_matching_objects(const std::string& pattern)
{
    m_number_of_objects = 0;
    search(sc_core::sc_get_top_level_objects(), pattern);
}

// ---------------------------------------------------------------------------
// Diagnostics location
// ---------------------------------------------------------------------------
namespace sca_obj_access_namespace
{
namespace
{
std::string& location_file()
{
    static std::string file = "<unknown>";
    return file;
}
int location_line = 0;
} // namespace

void set_file_location(const char* file, unsigned int line)
{
    location_file() = file ? file : "<unknown>";
    location_line = static_cast<int>(line);
}

const char* get_filename() { return location_file().c_str(); }

int get_lineno() { return location_line; }
} // namespace sca_obj_access_namespace

// ---------------------------------------------------------------------------
// Channel type registry
// ---------------------------------------------------------------------------
namespace
{
std::vector<const cos_trace_type_registrar_base*>& user_registrars()
{
    static std::vector<const cos_trace_type_registrar_base*> registrars;
    return registrars;
}

template <class... T>
void add_registrars(std::vector<const cos_trace_type_registrar_base*>& list)
{
    const cos_trace_type_registrar_base* regs[] = {new cos_compat_signal_trace_registrar<T>()...};
    list.insert(list.end(), std::begin(regs), std::end(regs));
}

template <std::size_t... N>
void add_bv_registrars(std::vector<const cos_trace_type_registrar_base*>& list,
                       std::index_sequence<N...>)
{
    add_registrars<sc_dt::sc_bv<N + 1>...>(list);
    add_registrars<sc_dt::sc_lv<N + 1>...>(list);
}

/** Built-in support for the common signal types. */
const std::vector<const cos_trace_type_registrar_base*>& builtin_registrars()
{
    static std::vector<const cos_trace_type_registrar_base*> list;
    if (list.empty())
    {
        add_registrars<bool, sc_dt::sc_logic, char, signed char, unsigned char, short,
                       unsigned short, int, unsigned int, long, unsigned long, long long,
                       unsigned long long, float, double>(list);
        add_bv_registrars(list, std::make_index_sequence<64>());
        add_registrars<sc_dt::sc_bv<128>, sc_dt::sc_bv<256>, sc_dt::sc_bv<1024> >(list);
    }
    return list;
}

const cos_trace_type_registrar_base* find_registrar(const sc_core::sc_interface* intf)
{
    for (const cos_trace_type_registrar_base* reg : user_registrars())
        if (reg->is_type(intf)) return reg;
    for (const cos_trace_type_registrar_base* reg : builtin_registrars())
        if (reg->is_type(intf)) return reg;
    return nullptr;
}

void report_unsupported(const sc_core::sc_object& obj, const std::string& file, int line)
{
    std::string msg = std::string("cannot trace '") + obj.name() + "' (" + obj.kind() + "): unsupported type";
    sc_core::sc_report_handler::report(sc_core::SC_WARNING, "sc_object_trace", msg.c_str(), file.c_str(), line);
}

void trace_interface(sc_core::sc_trace_file* tf, const sc_core::sc_interface* intf, const std::string& name,
                     const sc_core::sc_object& obj, const std::string& file, int line)
{
    if (const cos_trace_type_registrar_base* reg = find_registrar(intf))
        reg->trace_sc(intf, tf, name);
    else
        report_unsupported(obj, file, line);
}

/**
 * Port traces requested during elaboration: the bound channel of a port is
 * only known after binding has completed, so they are resolved at the end of
 * elaboration (still early enough for sc_trace).
 */
class deferred_port_tracer : public sc_core::sc_module
{
public:
    struct request
    {
        sc_core::sc_trace_file* tf;
        sc_core::sc_port_base* port;
        std::string name;
        std::string file;
        int line;
    };

    static deferred_port_tracer& instance()
    {
        static deferred_port_tracer* tracer =
            new deferred_port_tracer(sc_core::sc_gen_unique_name("cos_compat_deferred_trace"));
        return *tracer;
    }

    void add(const request& r) { m_requests.push_back(r); }

private:
    explicit deferred_port_tracer(sc_core::sc_module_name nm) : sc_core::sc_module(nm) {}

    void end_of_elaboration() override
    {
        for (const request& r : m_requests)
        {
            if (const sc_core::sc_interface* intf = r.port->get_interface())
                trace_interface(r.tf, intf, r.name, *r.port, r.file, r.line);
            else
                report_unsupported(*r.port, r.file, r.line);
        }
        m_requests.clear();
    }

    std::vector<request> m_requests;
};
} // namespace

void cos_register_sc_object_trace_type(const cos_trace_type_registrar_base* registrar)
{
    if (!registrar) return;
    for (const cos_trace_type_registrar_base* reg : user_registrars())
        if (reg->is_same(registrar)) return;  // duplicate: stays owned by the caller
    registrar->m_registered = true;
    user_registrars().push_back(registrar);
}

// ---------------------------------------------------------------------------
// sca_obj_floc_trace
// ---------------------------------------------------------------------------
sca_obj_floc_trace::sca_obj_floc_trace(const char* file, int line)
    : m_file(file ? file : ""), m_line(line)
{
    sca_obj_access_namespace::set_file_location(file, static_cast<unsigned int>(line));
}

void sca_obj_floc_trace::sc_object_trace_(sc_core::sc_trace_file* tf, const std::string& pattern,
                                          const std::string& nm)
{
    if (!tf) return;

    std::vector<sc_core::sc_object*> objs = get_matching_objects<sc_core::sc_object>(pattern);
    if (objs.empty())
    {
        std::string msg = "no object matches '" + pattern + "'";
        sc_core::sc_report_handler::report(sc_core::SC_WARNING, "sc_object_trace", msg.c_str(),
                                           m_file.c_str(), m_line);
        return;
    }

    for (sc_core::sc_object* obj : objs)
    {
        const std::string trace_name = (objs.size() == 1 && !nm.empty()) ? nm : std::string(obj->name());

        if (auto traceable = dynamic_cast<const cos_sc_traceable_object*>(obj))
        {
            traceable->trace_sc(tf, trace_name);
            continue;
        }

        if (auto intf = dynamic_cast<const sc_core::sc_interface*>(obj))
        {
            trace_interface(tf, intf, trace_name, *obj, m_file, m_line);
            continue;
        }

        if (auto port = dynamic_cast<sc_core::sc_port_base*>(obj))
        {
            if (const sc_core::sc_interface* intf = port->get_interface())
                trace_interface(tf, intf, trace_name, *obj, m_file, m_line);
            else
                deferred_port_tracer::instance().add({tf, port, trace_name, m_file, m_line});
            continue;
        }

        report_unsupported(*obj, m_file, m_line);
    }
}

// ---------------------------------------------------------------------------
// Misc.
// ---------------------------------------------------------------------------
namespace
{
/** Grants access to the protected sc_prim_channel::request_update(). */
struct prim_channel_access : sc_core::sc_prim_channel
{
    static void request(sc_core::sc_prim_channel& ch)
    {
        void (sc_core::sc_prim_channel::*fn)() = &prim_channel_access::request_update;
        (ch.*fn)();
    }
};
} // namespace

void request_channel_update(sc_core::sc_prim_channel& channel)
{
    prim_channel_access::request(channel);
}

} // namespace sca_basic_libraries_namespace
