# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
###############################################################################
#  Example test benches. Each example writes its VCD file into its own folder
#  in the build tree; `ctest` runs the examples and checks their waveforms.
###############################################################################

set(GTM_EXAMPLES_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common")

# gtm_add_example(<name> SOURCES <files...>)
function(gtm_add_example name)
  cmake_parse_arguments(EX "" "" "SOURCES" ${ARGN})
  add_executable(${name} ${EX_SOURCES})
  target_link_libraries(${name} PRIVATE gtm::tb)
  target_compile_options(${name} PRIVATE -Wall)
  set_target_properties(${name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/${name}")
  # The GTM AXIM model loads its memory image from the working directory.
  configure_file("${GTM_EXAMPLES_COMMON_DIR}/axim.mem"
                 "${CMAKE_BINARY_DIR}/examples/${name}/axim.mem" COPYONLY)
endfunction()

add_subdirectory(atom_somp)
add_subdirectory(tom_pwm_irq)
