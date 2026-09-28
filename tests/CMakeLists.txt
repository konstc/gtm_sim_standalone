# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
###############################################################################
#  Tests
#    cos_compat.unit      - compatibility layer without the GTM model
#    atom_somp.*          - example simulations + waveform checks
###############################################################################
find_package(Python3 COMPONENTS Interpreter)

add_executable(test_cos_compat test_cos_compat.cpp)
target_link_libraries(test_cos_compat PRIVATE cos_compat)
# The variable-handle template lives in a bundle header.
target_include_directories(test_cos_compat SYSTEM PRIVATE ${GTM_BUNDLE_INCLUDE_DIRS})
add_test(NAME cos_compat.unit COMMAND test_cos_compat
         WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

if(NOT Python3_Interpreter_FOUND)
  message(WARNING "python3 not found: ABI and waveform checks are not added to the tests")
  return()
endif()

# ---------------------------------------------------------------------------
# ABI checks against the prebuilt libraries (docs/ABI_NOTES.md)
#   abi.layout  - class layouts seen through our headers vs. the libraries
#   abi.vtables - SystemC/TLM vtable layouts vs. those inside the libraries
# ---------------------------------------------------------------------------
add_executable(abi_layout_probe "${PROJECT_SOURCE_DIR}/tools/abi/layout_probe.cpp")
target_link_libraries(abi_layout_probe PRIVATE gtm::gtm)
target_compile_options(abi_layout_probe PRIVATE -Wno-invalid-offsetof)
add_test(NAME abi.layout
         COMMAND ${Python3_EXECUTABLE} "${PROJECT_SOURCE_DIR}/tools/abi/check_layout.py"
                 $<TARGET_FILE:abi_layout_probe> ${GTM_BUNDLE_ARCH})

add_library(abi_vtable_probe OBJECT "${PROJECT_SOURCE_DIR}/tools/abi/sc_vtable_probe.cpp")
target_link_libraries(abi_vtable_probe PRIVATE SystemC::systemc)
get_filename_component(_gtm_tool_dir "${CMAKE_OBJDUMP}" DIRECTORY)
add_test(NAME abi.vtables
         COMMAND ${Python3_EXECUTABLE} "${PROJECT_SOURCE_DIR}/tools/abi/vtable_compare.py"
                 --tool-dir "${_gtm_tool_dir}"
                 $<TARGET_FILE:gtm::bosch_gtm> $<TARGET_FILE:gtm::cos_gtm_lib>
                 -- $<TARGET_OBJECTS:abi_vtable_probe>
         COMMAND_EXPAND_LISTS)

# gtm_example_test(<name> <example target> <vcd> ARGS <sim args...> CHECK <check command...>)
function(gtm_example_test name example vcd)
  cmake_parse_arguments(T "" "" "ARGS;CHECK" ${ARGN})
  set(dir "${CMAKE_BINARY_DIR}/examples/${example}")
  add_test(NAME ${name}.run COMMAND ${example} --vcd=${vcd} ${T_ARGS} WORKING_DIRECTORY "${dir}")
  set_tests_properties(${name}.run PROPERTIES FIXTURES_SETUP ${name} TIMEOUT 1800)
  add_test(NAME ${name}.check COMMAND ${T_CHECK} WORKING_DIRECTORY "${dir}")
  set_tests_properties(${name}.check PROPERTIES FIXTURES_REQUIRED ${name})
endfunction()

set(ATOM_CHECK "${PROJECT_SOURCE_DIR}/examples/atom_somp/check_atom_somp.py")
gtm_example_test(atom_somp.lab0 atom_somp atom_somp_lab0
  CHECK ${Python3_EXECUTABLE} ${ATOM_CHECK} atom_somp_lab0.vcd)
gtm_example_test(atom_somp.lab1 atom_somp atom_somp_lab1 ARGS --lab1
  CHECK ${Python3_EXECUTABLE} ${ATOM_CHECK} atom_somp_lab1.vcd --lab1)

gtm_example_test(tom_pwm_irq tom_pwm_irq tom_pwm_irq
  CHECK ${Python3_EXECUTABLE} "${PROJECT_SOURCE_DIR}/examples/tom_pwm_irq/check_tom_pwm_irq.py" tom_pwm_irq.vcd)

# Signal coverage against the COSIDE waveform layout of the original example.
set(ATOM_WAVE "${GTM_BUNDLE_DIR}/cos_gtm_example/atom_somp/atom_somp_cnt_out.wave")
if(EXISTS "${ATOM_WAVE}")
  gtm_example_test(atom_somp.cosim_signals atom_somp atom_somp_secondary ARGS --secondary_variables
    CHECK ${Python3_EXECUTABLE} "${PROJECT_SOURCE_DIR}/tools/wave_signals.py" "${ATOM_WAVE}"
          atom_somp_secondary.vcd --allow-missing "*.atom0.tim_ext_capture_sig(*)")
endif()
