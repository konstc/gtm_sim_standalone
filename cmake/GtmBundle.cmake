# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
###############################################################################
#  Imported targets for the prebuilt GTM model shipped in the COSIDE bundle
#  (coside-*-Bosch_GTM-bundle-*). The bundle is used read-only.
#
#    gtm::bosch_gtm    - Bosch GTM reference model   (libbosch_gtm.a)
#    gtm::cos_gtm_lib  - COSEDA wrapper / netlist    (libcos_gtm_lib.a)
#
#  The umbrella target gtm::gtm (libraries + include paths + defines +
#  COSIDE runtime replacement + SystemC) is defined in cos_compat/.
###############################################################################

# The environment variable is the primary reference and always wins; the cache
# variable allows -DGTM_BUNDLE_DIR=... when the environment is not set.
if(NOT "$ENV{GTM_BUNDLE_DIR}" STREQUAL "")
  set(GTM_BUNDLE_DIR "$ENV{GTM_BUNDLE_DIR}" CACHE PATH
      "Root of the COSIDE Bosch GTM bundle (contains bosch_gtm/ and cos_gtm_lib/)" FORCE)
else()
  set(GTM_BUNDLE_DIR "" CACHE PATH
      "Root of the COSIDE Bosch GTM bundle (contains bosch_gtm/ and cos_gtm_lib/)")
endif()

if(NOT GTM_BUNDLE_DIR OR NOT EXISTS "${GTM_BUNDLE_DIR}/cos_gtm_lib")
  message(FATAL_ERROR
    "GTM bundle not found (GTM_BUNDLE_DIR='${GTM_BUNDLE_DIR}').\n"
    "Set the environment variable GTM_BUNDLE_DIR (see scripts/env.sh) or pass "
    "-DGTM_BUNDLE_DIR=<path>.")
endif()
file(TO_CMAKE_PATH "${GTM_BUNDLE_DIR}" GTM_BUNDLE_DIR)

set(GTM_BUNDLE_CONFIG DEBUG CACHE STRING "Bundle library configuration folder")

if(WIN32 AND MINGW)
  set(_gtm_arch mingw-w64)
elseif(UNIX AND CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_gtm_arch linux64)
else()
  message(FATAL_ERROR "No GTM bundle libraries for this platform (need linux64 or mingw-w64).")
endif()
set(GTM_BUNDLE_ARCH ${_gtm_arch} CACHE INTERNAL "")

set(_bosch "${GTM_BUNDLE_DIR}/bosch_gtm/${GTM_BUNDLE_CONFIG}")
set(_cos   "${GTM_BUNDLE_DIR}/cos_gtm_lib/${GTM_BUNDLE_CONFIG}")

foreach(_lib IN ITEMS "${_bosch}/lib-${_gtm_arch}/libbosch_gtm.a"
                      "${_cos}/lib-${_gtm_arch}/libcos_gtm_lib.a")
  if(NOT EXISTS "${_lib}")
    message(FATAL_ERROR "Missing bundle library: ${_lib}")
  endif()
endforeach()

add_library(gtm::bosch_gtm STATIC IMPORTED GLOBAL)
set_target_properties(gtm::bosch_gtm PROPERTIES
  IMPORTED_LOCATION "${_bosch}/lib-${_gtm_arch}/libbosch_gtm.a")

add_library(gtm::cos_gtm_lib STATIC IMPORTED GLOBAL)
set_target_properties(gtm::cos_gtm_lib PROPERTIES
  IMPORTED_LOCATION "${_cos}/lib-${_gtm_arch}/libcos_gtm_lib.a")

# Include paths / defines as used by the COSIDE example project
# (.cproject + _config_data).
set(GTM_BUNDLE_INCLUDE_DIRS
  "${_bosch}/include"
  "${_cos}/include"
  "${GTM_BUNDLE_DIR}/cos_gtm_lib")
set(GTM_BUNDLE_DEFINITIONS
  GAL_GTM_GEN=4
  COSIDE=1
  GAL_MSG
  GTM_BASE_ADDR=0xE0000000)

message(STATUS "GTM bundle: ${GTM_BUNDLE_DIR} (${GTM_BUNDLE_CONFIG}/${_gtm_arch})")
