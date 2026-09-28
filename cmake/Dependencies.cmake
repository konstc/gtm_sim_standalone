# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
###############################################################################
#  Third-party kernels built as part of this project:
#    - SystemC 2.3.4 (Accellera)          -> target SystemC::systemc
#    - SystemC-AMS 2.3.4 PoC (COSEDA)     -> target SystemC::systemc-ams
#
#  SystemC is downloaded once into the build tree (point the standard
#  FETCHCONTENT_SOURCE_DIR_SYSTEMC cache variable to a local source tree to
#  build offline).
#
#  SystemC-AMS is a COSEDA download (like the GTM bundle): its unpacked source
#  tree is referenced through the environment variable SYSTEMC_AMS_DIR and
#  compiled in place (read-only; all outputs go to the build tree).
###############################################################################
include(FetchContent)

# FetchContent_Populate() + add_subdirectory(... EXCLUDE_FROM_ALL) is used to
# set cache options before adding SystemC and to keep its examples out of
# 'all'; CMake >= 3.30 deprecates this (CMP0169), the replacement needs 3.28.
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

set(GTM_SYSTEMC_URL
    "https://github.com/accellera-official/systemc/archive/refs/tags/2.3.4.tar.gz"
    CACHE STRING "SystemC 2.3.4 source archive (URL or local path)")
set(GTM_SYSTEMC_SHA256
    "bfb309485a8ad35a08ee78827d1647a451ec5455767b25136e74522a6f41e0ea"
    CACHE STRING "SHA256 of the SystemC archive (empty to skip the check)")

# The environment variable is the primary reference and always wins; the cache
# variable allows -DSYSTEMC_AMS_DIR=... when the environment is not set.
if(NOT "$ENV{SYSTEMC_AMS_DIR}" STREQUAL "")
  set(SYSTEMC_AMS_DIR "$ENV{SYSTEMC_AMS_DIR}" CACHE PATH
      "Unpacked SystemC-AMS 2.3.4 PoC source tree (COSEDA)" FORCE)
else()
  set(SYSTEMC_AMS_DIR "" CACHE PATH "Unpacked SystemC-AMS 2.3.4 PoC source tree (COSEDA)")
endif()

option(GTM_AMS_PARALLEL_TRACING
       "Build SystemC-AMS with parallel tracing (changes sca_trace_file_base layout)" OFF)

# ---------------------------------------------------------------------------
# SystemC
# ---------------------------------------------------------------------------
set(_sc_hash)
if(GTM_SYSTEMC_SHA256)
  set(_sc_hash URL_HASH SHA256=${GTM_SYSTEMC_SHA256})
endif()
FetchContent_Declare(systemc URL "${GTM_SYSTEMC_URL}" ${_sc_hash})

FetchContent_GetProperties(systemc)
if(NOT systemc_POPULATED)
  FetchContent_Populate(systemc)

  # SystemC declares these as cache variables (C++98 by default); seed the
  # cache first so its set(... CACHE ...) keeps our values.
  set(CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD} CACHE STRING "C++ standard" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

  # Needed by the COSIDE trace pre-processor (cos_sc_trace_preprocessor_base).
  set(ENABLE_PHASE_CALLBACKS ON CACHE BOOL "SystemC simulation phase callbacks" FORCE)
  set(ENABLE_PHASE_CALLBACKS_TRACING ON CACHE BOOL "" FORCE)
  set(DISABLE_COPYRIGHT_MESSAGE ON CACHE BOOL "" FORCE)

  add_subdirectory(${systemc_SOURCE_DIR} ${systemc_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if(MINGW)
  # On Windows the SystemC kernel uses Win32 fibers for its coroutines, but
  # SystemC 2.3.4's CMakeLists still adds the (unused, ELF-only) QuickThreads
  # assembly for x86_64 GCC, which does not assemble for COFF -> exclude it.
  set_source_files_properties("${systemc_SOURCE_DIR}/src/sysc/packages/qt/md/iX86_64.s"
    DIRECTORY "${systemc_SOURCE_DIR}/src" PROPERTIES HEADER_FILE_ONLY ON)
endif()

# SystemC's headers are third-party: silence their warnings in our code.
get_target_property(_sc_inc systemc INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(systemc PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_sc_inc}")

# ---------------------------------------------------------------------------
# SystemC-AMS
#   Its own CMakeLists only works as a top-level project (CMAKE_SOURCE_DIR,
#   $ENV{SYSTEMC_HOME}), so the library target is defined here.
# ---------------------------------------------------------------------------
if(NOT SYSTEMC_AMS_DIR OR NOT EXISTS "${SYSTEMC_AMS_DIR}/src/systemc-ams.h")
  message(FATAL_ERROR
    "SystemC-AMS sources not found (SYSTEMC_AMS_DIR='${SYSTEMC_AMS_DIR}').\n"
    "Download systemc-ams-2.3.4.tar.gz from "
    "https://www.coseda-tech.com/systemc-ams-proof-of-concept, unpack it and set "
    "the environment variable SYSTEMC_AMS_DIR to the unpacked folder (the one "
    "containing src/systemc-ams.h, see scripts/env.sh), or pass -DSYSTEMC_AMS_DIR=<path>.")
endif()

file(TO_CMAKE_PATH "${SYSTEMC_AMS_DIR}" SYSTEMC_AMS_DIR)
set(systemc_ams_SOURCE_DIR "${SYSTEMC_AMS_DIR}")
message(STATUS "SystemC-AMS: ${SYSTEMC_AMS_DIR}")

file(GLOB_RECURSE _ams_sources CONFIGURE_DEPENDS
  "${systemc_ams_SOURCE_DIR}/src/scams/*.cpp"
  "${systemc_ams_SOURCE_DIR}/src/scams/*.c")
add_library(systemc-ams STATIC ${_ams_sources})
add_library(SystemC::systemc-ams ALIAS systemc-ams)
target_include_directories(systemc-ams SYSTEM PUBLIC "${systemc_ams_SOURCE_DIR}/src")
target_compile_definitions(systemc-ams PRIVATE _USE_MATH_DEFINES)

if(NOT GTM_AMS_PARALLEL_TRACING)
  # Affects the layout of sca_trace_file_base -> must be visible to users too.
  target_compile_definitions(systemc-ams PUBLIC DISABLE_PARALLEL_TRACING)
endif()

target_compile_options(systemc-ams PRIVATE -w)
target_link_libraries(systemc-ams PUBLIC SystemC::systemc)
