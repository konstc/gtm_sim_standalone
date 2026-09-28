// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// Prints the layout (as "name = 0x...") of the classes whose layout the
// prebuilt GTM libraries depend on, as seen through our headers. The values
// are compared with the values the libraries expect by
// tools/abi/check_layout.py.
#include <cstddef>
#include <cstdio>
#include <fstream>

#include "cos_gtm_lib/gtm_configurations/cos_gtm_c1.h"
#include "cos_utilities/cos_sc_variable_handle.h"
#include "sca_basic_libraries/coside_utilities.h"

#define PRINT(name, value) std::printf("%-44s = 0x%zx\n", name, static_cast<std::size_t>(value))

int main()
{
    using namespace sca_basic_libraries_namespace;

    PRINT("sizeof(cos_gtm_c1)", sizeof(cos_gtm_c1));
    PRINT("offsetof(cos_gtm_c1, c)", offsetof(cos_gtm_c1, c));
    PRINT("sizeof(sc_vector_n<sc_in<bool>>)", sizeof(sc_core::sc_vector_n<sc_core::sc_in<bool> >));
    PRINT("sizeof(cos_connectivity_elaborator)", sizeof(cos_connectivity_elaborator));
    PRINT("sizeof(cos_sc_variable_handle<int>)", sizeof(cos_sc_variable_handle<int>));
    PRINT("sizeof(cos_sc_enum_tracer_base)", sizeof(cos_sc_enum_tracer_base));
    PRINT("sizeof(object_searcher<sc_object>)", sizeof(object_searcher<sc_core::sc_object>));
    PRINT("sizeof(cos_trace_type_registrar_base)", sizeof(cos_trace_type_registrar_base));
    PRINT("sizeof(sca_obj_floc_trace)", sizeof(sca_obj_floc_trace));
    PRINT("sizeof(std::ifstream)", sizeof(std::ifstream));
    PRINT("sizeof(std::filebuf)", sizeof(std::filebuf));
    return 0;
}
