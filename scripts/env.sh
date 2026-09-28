# SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
# SPDX-License-Identifier: Apache-2.0
# Source this file:  . scripts/env.sh
# Checks the environment variables that point the build at the external
# (read-only) inputs:
#   GTM_BUNDLE_DIR      Bosch GTM SystemC model bundle (unpacked folder)
#   SYSTEMC_AMS_DIR     SystemC-AMS proof-of-concept (unpacked source tree)
# Set them once, e.g. in ~/.bashrc:
#   export GTM_BUNDLE_DIR=/path/to/coside-<version>-Bosch_GTM-bundle-<date>
#   export SYSTEMC_AMS_DIR=/path/to/systemc-ams-<version>
_gtm_env_ok=1
if [ ! -d "${GTM_BUNDLE_DIR:-}/cos_gtm_lib" ]; then
    echo "GTM_BUNDLE_DIR='${GTM_BUNDLE_DIR:-}' is not the GTM bundle folder (cos_gtm_lib/ not found)" >&2
    _gtm_env_ok=0
fi
if [ ! -f "${SYSTEMC_AMS_DIR:-}/src/systemc-ams.h" ]; then
    echo "SYSTEMC_AMS_DIR='${SYSTEMC_AMS_DIR:-}' is not a SystemC-AMS source tree (src/systemc-ams.h not found)" >&2
    _gtm_env_ok=0
fi
echo "GTM_BUNDLE_DIR=${GTM_BUNDLE_DIR:-}"
echo "SYSTEMC_AMS_DIR=${SYSTEMC_AMS_DIR:-}"
[ "$_gtm_env_ok" = 1 ]
