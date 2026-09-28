// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// gtm_tb - minimal command line reader for test benches.
//
//   options.add("sim_time", sim_time, "simulation time in seconds");
//   options.parse(argc, argv);     // accepts --sim_time=1e-5, --help
#ifndef GTM_TB_CMDLINE_H_
#define GTM_TB_CMDLINE_H_

#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace gtm_tb
{

class cmdline
{
public:
    explicit cmdline(std::string description = std::string()) : m_description(std::move(description)) {}

    template <class T>
    void add(const std::string& name, T& value, const std::string& help)
    {
        std::ostringstream def;
        def << value;
        m_options.push_back({name, help, def.str(), [&value](const std::string& s) {
                                 std::istringstream is(s);
                                 is >> value;
                                 return !is.fail();
                             }});
    }

    void add(const std::string& name, std::string& value, const std::string& help)
    {
        m_options.push_back({name, help, value, [&value](const std::string& s) {
                                 value = s;
                                 return true;
                             }});
    }

    void add(const std::string& name, bool& value, const std::string& help)
    {
        m_options.push_back({name, help, value ? "1" : "0", [&value](const std::string& s) {
                                 value = s.empty() || s == "1" || s == "true" || s == "yes" || s == "on";
                                 return true;
                             }});
    }

    /** Returns false if the program should exit (--help or an error). */
    bool parse(int argc, char* argv[]);

    void print_usage(const char* program) const;

private:
    struct option
    {
        std::string name, help, default_value;
        std::function<bool(const std::string&)> set;
    };

    std::string m_description;
    std::vector<option> m_options;
};

} // namespace gtm_tb

#endif // GTM_TB_CMDLINE_H_
