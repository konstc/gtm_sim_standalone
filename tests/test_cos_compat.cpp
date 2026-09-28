// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// Unit test of the COSIDE compatibility layer (cos_compat), independent of
// the GTM model: lazily sized vector binding, object search, tracing.
#include <systemc>
#include <cstdlib>
#include <iostream>

#include "cos_utilities/cos_sc_variable_handle.h"
#include "sca_basic_libraries/coside_utilities.h"

using namespace sc_core;

static int failures = 0;
#define EXPECT(cond)                                                                           \
    do                                                                                         \
    {                                                                                          \
        if (!(cond))                                                                           \
        {                                                                                      \
            std::cerr << __FILE__ << ":" << __LINE__ << ": expectation failed: " #cond "\n"; \
            ++failures;                                                                        \
        }                                                                                      \
    } while (0)

// Leaf with a fixed-size port vector (like the GTM sub-modules).
struct leaf : sc_module
{
    sc_vector_n<sc_in<bool>, 3> in{"in"};
    SC_CTOR(leaf) {}
};

// Intermediate level with a port vector of unknown size.
struct middle : sc_module
{
    sc_vector_n<sc_in<bool> > in{"in"};
    leaf l{"l"};
    bool leaf_first;
    middle(sc_module_name nm, bool lf) : sc_module(nm), leaf_first(lf)
    {
        if (leaf_first) l.in.bind(in);  // size known on the leaf side first
    }
    void bind_leaf() { l.in.bind(in); }
};

struct top : sc_module
{
    sc_vector_n<sc_signal<bool> > sigs_a{"sigs_a"};
    sc_vector_n<sc_signal<bool> > sigs_b{"sigs_b"};
    middle m_a{"m_a", true};
    middle m_b{"m_b", false};
    int counter = 0;
    COS_SC_VARIABLE_HANDLE(counter);

    SC_CTOR(top)
    {
        m_a.in.bind(sigs_a);  // leaf already sized middle -> propagates to sigs_a
        m_b.in.bind(sigs_b);  // nothing sized yet: pending
        m_b.bind_leaf();      // now the size flows leaf -> middle -> sigs_b
        SC_THREAD(run);
    }

    void run()
    {
        for (int i = 0; i < 3; ++i)
        {
            sigs_a[i].write(i % 2 == 0);
            sigs_b[i].write(i % 2 == 1);
            ++counter;
        }
        wait(1, SC_NS);
    }
};

enum class color { red, green };

static std::string color_name(const color& c) { return c == color::red ? "red" : "green"; }

int sc_main(int, char*[])
{
    using sca_basic_libraries_namespace::object_searcher_base;

    // --- glob matching ----------------------------------------------------------
    EXPECT(object_searcher_base::match("*", "a.b.c"));
    EXPECT(object_searcher_base::match("*.b.*", "a.b.c"));
    EXPECT(object_searcher_base::match("a.?.c", "a.b.c"));
    EXPECT(!object_searcher_base::match("*.x", "a.b.c"));
    EXPECT(object_searcher_base::match("*i_cos_gtm*.*i_cos_irq_ctrl.irqs", "t.i_cos_gtm.x.i_cos_irq_ctrl.irqs"));

    top t("t");

    // --- lazily sized vector binding ----------------------------------------------
    EXPECT(t.sigs_a.size() == 3);
    EXPECT(t.sigs_b.size() == 3);
    EXPECT(t.m_a.in.size() == 3);
    EXPECT(t.m_b.in.size() == 3);
    EXPECT(std::string(t.m_b.in[2].name()) == "t.m_b.in_2");

    // --- object search ----------------------------------------------------------------
    EXPECT(get_matching_objects<sc_signal<bool> >("t.sigs_*").size() == 6);
    EXPECT(get_matching_objects<sc_in<bool> >("*.l.in_*").size() == 6);
    auto* s = GET_OBJECT<sc_signal<bool> >("*sigs_b_1");
    EXPECT(s == &t.sigs_b[1]);
    EXPECT((GET_OBJECT<sc_module>("*does_not_exist*", false)) == nullptr);

    // --- tracing by name ------------------------------------------------------------
    sc_trace_file* tf = sc_create_vcd_trace_file("test_cos_compat");
    sc_object_trace(tf, "t.sigs_a_*");                // signals, own names
    sc_object_trace(tf, "*.m_b.l.in_0", "leaf_in0");  // port, display name
    sc_object_trace(tf, "t.counter", "counter");      // registered variable
    color c = color::red;
    sca_basic_libraries_namespace::cos_compat_enum_tracer<color> ct(tf, c, "color", 8, color_name);

    sc_start(2, SC_NS);
    EXPECT(t.m_a.l.in[0].read() == true);
    EXPECT(t.m_b.l.in[1].read() == true);
    EXPECT(t.counter == 3);
    sc_close_vcd_trace_file(tf);

    EXPECT(sc_report_handler::get_count(SC_ERROR) == 0);
    EXPECT(sc_report_handler::get_count(SC_WARNING) == 0);

    std::cout << (failures ? "FAILED" : "PASSED") << " (" << failures << " failures)" << std::endl;
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
