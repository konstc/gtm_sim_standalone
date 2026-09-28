# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
###############################################################################
#  gtm_sim_standalone - standalone build of the Bosch GTM virtual model
#  (COSIDE bundle) on top of the Accellera SystemC / SystemC-AMS kernels.
#
#  Super-build: SystemC 2.3.4 + SystemC-AMS 2.3.4 + COSIDE compatibility
#  layer (cos_compat) + test-bench library + examples.
#
#  The COSEDA downloads are never modified; they are referenced via the
#  environment (or cache) variables
#    GTM_BUNDLE_DIR      COSIDE Bosch GTM bundle
#    SYSTEMC_AMS_DIR     SystemC-AMS 2.3.4 PoC (unpacked source tree)
###############################################################################
cmake_minimum_required(VERSION 3.24)   # $<LINK_GROUP:RESCAN,...>

# Prefer the ABI-matched GCC 9 when available (bundle libs: GCC 9.4.0).
if(NOT DEFINED CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CXX})
  find_program(_GTM_GXX9 NAMES g++-9)
  find_program(_GTM_GCC9 NAMES gcc-9)
  if(_GTM_GXX9 AND _GTM_GCC9)
    set(CMAKE_CXX_COMPILER "${_GTM_GXX9}" CACHE FILEPATH "C++ compiler")
    set(CMAKE_C_COMPILER   "${_GTM_GCC9}" CACHE FILEPATH "C compiler")
  endif()
endif()

project(gtm_sim_standalone VERSION 0.1 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# The bundle libraries were compiled against SystemC 2.3.4 with
# SC_CPLUSPLUS=201103L (symbol sc_api_version_2_3_4_cxx201103L). Every
# translation unit - SystemC itself included - must agree on this value.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(GTM_SC_CPLUSPLUS 201103L)
add_compile_definitions(SC_CPLUSPLUS=${GTM_SC_CPLUSPLUS})

set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
set(BUILD_SHARED_LIBS OFF)

if(MINGW)
  # The GTM/SystemC headers produce objects with more than 2^15 sections.
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-Wa,-mbig-obj>")
  # Self-contained executables: no toolchain DLLs needed at run time.
  add_link_options(-static)
endif()

include(Dependencies)   # SystemC + SystemC-AMS targets
include(GtmBundle)      # imported GTM libraries from the COSIDE bundle

enable_testing()

add_subdirectory(cos_compat)
add_subdirectory(tb_lib)
add_subdirectory(examples)
add_subdirectory(tests)
