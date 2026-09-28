# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
###############################################################################
#  gtm_tb - reusable test-bench building blocks for the GTM model
###############################################################################
add_library(gtm_tb STATIC
  src/cmdline.cpp
  src/gtm_config.cpp
  src/gtm_harness.cpp)
add_library(gtm::tb ALIAS gtm_tb)
target_include_directories(gtm_tb PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
# bv_combiner uses sc_spawn (header-only switch, no ABI impact)
target_compile_definitions(gtm_tb PUBLIC SC_INCLUDE_DYNAMIC_PROCESSES)
target_compile_options(gtm_tb PRIVATE -Wall -Wextra)
target_link_libraries(gtm_tb PUBLIC gtm::gtm)
