# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
###############################################################################
#  cos_compat - independent implementation of the COSIDE runtime interface
#  (sca_basic_libraries) that the prebuilt GTM libraries depend on.
#  See docs/ABI_NOTES.md for the interface requirements.
###############################################################################

add_library(cos_compat STATIC
  src/cos_connectivity_elaborator.cpp
  src/cos_enum_tracer.cpp
  src/cos_gtm_aei.cpp
  src/cos_misc_utilities.cpp
  src/cos_object_access.cpp
  src/cos_variable_handle.cpp)
target_include_directories(cos_compat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
# cos_variable_handle.cpp implements a class declared in a bundle header.
target_include_directories(cos_compat SYSTEM PRIVATE ${GTM_BUNDLE_INCLUDE_DIRS})
target_compile_definitions(cos_compat PRIVATE ${GTM_BUNDLE_DEFINITIONS})
target_compile_options(cos_compat PRIVATE -Wall -Wextra)
target_link_libraries(cos_compat PUBLIC SystemC::systemc-ams)

# ---------------------------------------------------------------------------
# gtm::gtm - everything a test bench needs to instantiate the GTM model.
# The bundle archives and cos_compat reference each other -> link group.
# ---------------------------------------------------------------------------
add_library(gtm_gtm INTERFACE)
add_library(gtm::gtm ALIAS gtm_gtm)

# The prebuilt libraries read uninitialized heap members (see
# src/cos_zero_init_new.cpp); zero-initialized allocations make simulations
# deterministic. Compiled into each executable so it is always linked.
option(GTM_ZERO_INIT_HEAP "Zero-initialize all heap allocations (works around uninitialized members in the GTM libraries)" ON)
if(GTM_ZERO_INIT_HEAP)
  target_sources(gtm_gtm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/cos_zero_init_new.cpp")
endif()
target_include_directories(gtm_gtm SYSTEM INTERFACE ${GTM_BUNDLE_INCLUDE_DIRS})
target_compile_definitions(gtm_gtm INTERFACE ${GTM_BUNDLE_DEFINITIONS})
target_link_libraries(gtm_gtm INTERFACE
  "$<LINK_GROUP:RESCAN,gtm::cos_gtm_lib,gtm::bosch_gtm,cos_compat>"
  SystemC::systemc-ams
  ${CMAKE_DL_LIBS})
