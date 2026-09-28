// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// gtm_tb - bit-level connection helpers.
//
// The GTM ports are bit vectors (sc_bv<N>) carrying one channel per bit.
// COSIDE connects individual bits with sc_slice(); plain SystemC needs small
// adapter modules instead:
//
//   bit_extract<N>  : sc_bv<N> signal bit -> bool signal
//   bv_combiner<N>  : several bool / sc_bv<M> drivers -> one sc_bv<N> signal
#ifndef GTM_TB_BIT_ADAPTERS_H_
#define GTM_TB_BIT_ADAPTERS_H_

#include <systemc>
#include <functional>
#include <vector>

namespace gtm_tb
{

template <int N>
class bit_extract : public sc_core::sc_module
{
public:
    sc_core::sc_in<sc_dt::sc_bv<N> > in;
    sc_core::sc_out<bool> out;

    bit_extract(sc_core::sc_module_name nm, int bit) : sc_core::sc_module(nm), in("in"), out("out"), m_bit(bit)
    {
        sc_assert(bit >= 0 && bit < N);
        SC_HAS_PROCESS(bit_extract);
        SC_METHOD(update);
        sensitive << in;
    }

private:
    void update() { out.write(in.read()[m_bit].to_bool()); }

    int m_bit;
};

template <int N>
class bv_combiner : public sc_core::sc_module
{
public:
    sc_core::sc_out<sc_dt::sc_bv<N> > out;

    explicit bv_combiner(sc_core::sc_module_name nm, const sc_dt::sc_bv<N>& default_value = sc_dt::sc_bv<N>())
        : sc_core::sc_module(nm), out("out"), m_default(default_value)
    {
    }

    /** Drives bit 'bit' of the output from a bool signal. */
    void connect_bit(const sc_core::sc_signal_in_if<bool>& src, int bit)
    {
        sc_assert(bit >= 0 && bit < N);
        m_events.push_back(&src.value_changed_event());
        m_drivers.push_back([&src, bit](sc_dt::sc_bv<N>& v) { v[bit] = src.read(); });
    }

    /** Drives bits [lsb, lsb+M) of the output from an sc_bv<M> signal. */
    template <int M>
    void connect_range(const sc_core::sc_signal_in_if<sc_dt::sc_bv<M> >& src, int lsb)
    {
        sc_assert(lsb >= 0 && lsb + M <= N);
        m_events.push_back(&src.value_changed_event());
        m_drivers.push_back([&src, lsb](sc_dt::sc_bv<N>& v) { v.range(lsb + M - 1, lsb) = src.read(); });
    }

private:
    void before_end_of_elaboration() override
    {
        sc_core::sc_spawn_options opt;
        opt.spawn_method();
        for (const sc_core::sc_event* ev : m_events) opt.set_sensitivity(ev);
        sc_core::sc_spawn([this]() { update(); }, "update", &opt);
    }

    void update()
    {
        sc_dt::sc_bv<N> v = m_default;
        for (auto& drive : m_drivers) drive(v);
        out.write(v);
    }

    sc_dt::sc_bv<N> m_default;
    std::vector<const sc_core::sc_event*> m_events;
    std::vector<std::function<void(sc_dt::sc_bv<N>&)> > m_drivers;
};

} // namespace gtm_tb

#endif // GTM_TB_BIT_ADAPTERS_H_
