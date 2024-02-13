#
# Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The OpenAirInterface Software Alliance licenses this file to You under
# the OAI Public License, Version 1.1  (the "License"); you may not use this file
# except in compliance with the License.
# You may obtain a copy of the License at
# 
#      http://www.openairinterface.org/?page_id=698
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ------------------------------------------------------------------------------
# For more information about the OpenAirInterface (OAI) Software Alliance:
#      contact@openairinterface.org
# 

# FindXran
# -------
# 
# Finds the xran library. Note that the library number is as follows:
# - oran_bronze_release_v1.1 -> 2.1.1 (B = second letter)
# - oran_e_maintenance_release_v1.0 -> 5.1.0
# the version is currently hardcoded to 5.1.0
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

option(xran_LOCATION "directory of XRAN library" "")
if (NOT xran_LOCATION)
  message(FATAL_ERROR "xran_LOCATION required")
endif()
if (NOT EXISTS ${xran_LOCATION})
  message(FATAL_ERROR "no such directory: ${xran_LOCATION}")
endif()

find_path(xran_INCLUDE_DIR
  NAMES
    xran_common.h
    xran_compression.h
    xran_cp_api.h
    xran_ecpri_owd_measurements.h
    xran_fh_o_du.h
    xran_pkt.h
    xran_pkt_up.h
    xran_sync_api.h
  PATHS ${xran_LOCATION}
  PATH_SUFFIXES api
)
find_library(xran_LIBRARY
  NAMES xran
  PATHS ${xran_LOCATION}/build
)
if (NOT xran_LIBRARY)
  message(FATAL_ERROR "could not detect xran build artifacts at ${xran_LOCATION}/build")
endif()

set(xran_VERSION_FILE "${xran_LOCATION}/../app/src/common.h")
if(NOT EXISTS ${xran_VERSION_FILE})
  message(FATAL_ERROR "could not find xran version file at ${xran_VERSION_FILE}")
endif()

file(STRINGS ${xran_VERSION_FILE} xran_VERSION_LINE REGEX "^#define[ \t]+VERSIONX[ \t]+\"[a-z_.0-9]+\"$")
string(REGEX REPLACE "^#define[ \t]+VERSIONX[ \t]+\"([a-z_.0-9]+)\"$" "\\1" xran_VERSION_STRING "${xran_VERSION_LINE}")
message(STATUS "Found xran release ${xran_VERSION_STRING}")
set(xran_VERSION "NOTFOUND")
if (xran_VERSION_STRING STREQUAL "oran_e_maintenance_release_v1.0")
  set(xran_VERSION 5.1.0)
elseif (xran_VERSION_STRING STREQUAL "oran_e_maintenance_release_v1.1")
  set(xran_VERSION 5.1.1)
elseif (xran_VERSION_STRING STREQUAL "oran_e_maintenance_release_v1.2")
  set(xran_VERSION 5.1.2)
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
    INTERFACE_INCLUDE_DIRECTORIES "${xran_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
  xran_INCLUDE_DIR
  xran_LIBRARY
)
