// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// gtm_tb - command line reader (see cmdline.h).
#include "gtm_tb/cmdline.h"

#include <cstring>
#include <iostream>

namespace gtm_tb
{

bool cmdline::parse(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")
        {
            print_usage(argv[0]);
            return false;
        }
        if (arg.compare(0, 2, "--") != 0)
        {
            std::cerr << "unexpected argument '" << arg << "' (see --help)" << std::endl;
            return false;
        }
        arg = arg.substr(2);
        const std::size_t eq = arg.find('=');
        const std::string name = arg.substr(0, eq);
        const std::string value = eq == std::string::npos ? std::string() : arg.substr(eq + 1);

        bool known = false;
        for (const option& opt : m_options)
        {
            if (opt.name != name) continue;
            known = true;
            if (!opt.set(value))
            {
                std::cerr << "invalid value '" << value << "' for --" << name << std::endl;
                return false;
            }
        }
        if (!known)
        {
            std::cerr << "unknown option '--" << name << "' (see --help)" << std::endl;
            return false;
        }
    }
    return true;
}

void cmdline::print_usage(const char* program) const
{
    std::cout << "usage: " << program << " [--option=value ...]\n";
    if (!m_description.empty()) std::cout << m_description << "\n";
    std::cout << "options:\n";
    for (const option& opt : m_options)
        std::cout << "  --" << opt.name << "  " << opt.help << " (default: " << opt.default_value << ")\n";
    std::cout << "  --help  this text" << std::endl;
}

} // namespace gtm_tb
