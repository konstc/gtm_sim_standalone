// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// gtm_tb - stimulus sources for GTM test benches.
#ifndef GTM_TB_SOURCES_H_
#define GTM_TB_SOURCES_H_

#include <systemc>
#include <utility>
#include <vector>

namespace gtm_tb
{

/** Drives a constant value (e.g. the GTM clock period in ps). */
template <class T>
class constant_source : public sc_core::sc_module
{
public:
    sc_core::sc_out<T> out;

    constant_source(sc_core::sc_module_name nm, const T& value) : sc_core::sc_module(nm), out("out")
    {
        out.initialize(value);
    }
};

/**
 * Piece-wise constant source: starts with init_value and switches to the
 * given values at the given absolute times. With repeat == true the curve
 * restarts after the last point (period = time of the last point).
 */
template <class T>
class pwc_source : public sc_core::sc_module
{
public:
    typedef std::pair<sc_core::sc_time, T> point;

    sc_core::sc_out<T> out;

    pwc_source(sc_core::sc_module_name nm, const T& init_value, std::vector<point> curve,
               bool repeat = false)
        : sc_core::sc_module(nm), out("out"), m_init(init_value), m_curve(std::move(curve)),
          m_repeat(repeat)
    {
        out.initialize(init_value);
        SC_HAS_PROCESS(pwc_source);
        SC_THREAD(run);
    }

private:
    void run()
    {
        sc_core::sc_time offset = sc_core::SC_ZERO_TIME;
        do
        {
            for (const point& p : m_curve)
            {
                const sc_core::sc_time t = offset + p.first;
                if (t > sc_core::sc_time_stamp()) wait(t - sc_core::sc_time_stamp());
                out.write(p.second);
            }
            if (m_curve.empty() || m_curve.back().first == sc_core::SC_ZERO_TIME) return;
            offset += m_curve.back().first;
            if (m_repeat) out.write(m_init);
        } while (m_repeat);
    }

    T m_init;
    std::vector<point> m_curve;
    bool m_repeat;
};

} // namespace gtm_tb

#endif // GTM_TB_SOURCES_H_
