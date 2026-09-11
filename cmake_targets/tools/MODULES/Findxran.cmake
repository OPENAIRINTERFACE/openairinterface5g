# SPDX-License-Identifier: LicenseRef-CSSL-1.0

# FindXran
# -------
# 
# Finds the xran library. Note that the library number is as follows:
# - oran_e_maintenance_release_v1.5 -> 5.1.6
# - oran_f_release_v1.3 -> 6.1.4
# - oran_k_release_v1.7 -> 11.1.7
#
# Required options
# ^^^^^^^^^^^^^^^^
#
# ``xran_LOCATION``
#   The location of the library.
# 
# Imported Targets
# ^^^^^^^^^^^^^^^^
# 
# This module provides the following imported targets, if found:
# 
# ``xran::xran``
#   The xran library
# 
# Result Variables
# ^^^^^^^^^^^^^^^^
# 
# This will define the following variables:
# 
# ``xran_FOUND``
#   True if the system has the xran library.
# ``xran_VERSION``
#   The version of the xran library which was found.
# ``xran_INCLUDE_DIRS``
#   Include directories needed to use xran.
# ``xran_LIBRARIES``
#   Libraries needed to link to xran.
# 
# Cache Variables
# ^^^^^^^^^^^^^^^
# 
# The following cache variables may also be set:
# 
# ``xran_INCLUDE_DIR``
#   The directory containing ``foo.h``.
# ``xran_LIBRARY``
#   The path to the xran library.

set(CACHE{xran_LOCATION} TYPE PATH HELP "directory of XRAN library" VALUE "")

find_path(xran_INCLUDE_DIR
  NAMES
    xran_compression.h
    xran_cp_api.h
    xran_ecpri_owd_measurements.h
    xran_fh_o_du.h
    xran_pkt.h
    xran_pkt_up.h
    xran_sync_api.h
  HINTS ${xran_LOCATION}
  PATH_SUFFIXES api
  NO_DEFAULT_PATH
)
find_library(xran_LIBRARY
  NAMES xran
  HINTS ${xran_LOCATION}
  PATH_SUFFIXES build api
  NO_DEFAULT_PATH
)

set(xran_VERSION_FILE "${xran_LOCATION}/../app/src/common.h")
if(EXISTS ${xran_VERSION_FILE})
  file(STRINGS ${xran_VERSION_FILE} xran_VERSION_LINE REGEX "^#define[ \t]+VERSIONX[ \t]+\"[a-z_.0-9]+\"$")
else()
  set(xran_VERSION_LINE "UNKNOWN")
endif()

string(REGEX REPLACE "^#define[ \t]+VERSIONX[ \t]+\"([a-z_.0-9]+)\"$" "\\1" xran_VERSION_STRING "${xran_VERSION_LINE}")
if (xran_VERSION_STRING MATCHES "^oran_e_maintenance_release_v")
  string(REGEX REPLACE "oran_e_maintenance_release_v([0-9]+).([0-9]+)" "5.\\1.\\2" xran_VERSION ${xran_VERSION_STRING})
elseif(xran_VERSION_STRING MATCHES "^oran_f_release_v")
  string(REGEX REPLACE "oran_f_release_v([0-9]+).([0-9]+)" "6.\\1.\\2" xran_VERSION ${xran_VERSION_STRING})
elseif(xran_VERSION_STRING MATCHES "^oran_k_release_v")
  string(REGEX REPLACE "oran_k_release_v([0-9]+).([0-9]+)" "11.\\1.\\2" xran_VERSION ${xran_VERSION_STRING})
elseif(xran_VERSION_STRING MATCHES "^oran_bronze_release_v")
  string(REGEX REPLACE "oran_bronze_release_v([0-9]+).([0-9]+)" "2.\\1.\\2" xran_VERSION ${xran_VERSION_STRING})
else()
  set(xran_VERSION "UNKNOWN")
endif()
unset(xran_VERSION_LINE)
unset(xran_VERSION_STRING)
unset(xran_VERSION_FILE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(xran
  FOUND_VAR xran_FOUND
  REQUIRED_VARS
    xran_LIBRARY
    xran_INCLUDE_DIR
  VERSION_VAR xran_VERSION
)

if(xran_FOUND)
  set(xran_LIBRARIES ${xran_LIBRARY})
  set(xran_INCLUDE_DIRS ${xran_INCLUDE_DIR})
endif()

if(xran_FOUND AND NOT TARGET xran::xran)
  add_library(xran::xran UNKNOWN IMPORTED)
  set_target_properties(xran::xran PROPERTIES
    IMPORTED_LOCATION "${xran_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${xran_INCLUDE_DIRS}"
  )
endif()

mark_as_advanced(
  xran_INCLUDE_DIR
  xran_LIBRARY
)
