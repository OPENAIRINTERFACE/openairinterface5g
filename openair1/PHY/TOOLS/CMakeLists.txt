# SPDX-License-Identifier: LicenseRef-CSSL-1.0

#!
#* \brief: build rules and checks for softscope shared libraries
#*/

add_boolean_option(ENABLE_UESCOPE OFF "Whether to build the lte uescope" OFF)
add_boolean_option(ENABLE_ENBSCOPE OFF "Whether to build the lte enbcope" OFF)
add_boolean_option(ENABLE_NRSCOPE OFF "Whether to build the 5G scope" OFF)
if(ENABLE_UESCOPE OR ENABLE_ENBSCOPE OR ENABLE_NRSCOPE)
  find_library(forms NAMES forms)
  if(NOT forms)
    message(FATAL_ERROR "required library forms not found for building scopes")
  else()
    message(STATUS "libforms library, required for scopes, found at ${forms}")
  endif()

  include_directories ("/usr/include/X11")
  add_library(xforms_common OBJECT
              lte_phy_scope.c
              ../../../executables/stats.c)
  target_link_libraries(xforms_common PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
  target_link_libraries(xforms_common PRIVATE log_headers)

  add_library(enbscope MODULE lte_enb_scope.c $<TARGET_OBJECTS:xforms_common>)
  target_link_libraries(enbscope PUBLIC forms PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs UTIL)

  add_library(uescope MODULE lte_ue_scope.c $<TARGET_OBJECTS:xforms_common>)
  target_link_libraries(uescope PUBLIC forms PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs UTIL)

  add_library(nrscope MODULE nr_phy_scope.c)
  target_link_libraries(nrscope PUBLIC forms PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs UTIL)

  # all libraries should be written to root build dir (default creates the same hierarchie under build as existing for sources)
  set_target_properties(enbscope uescope nrscope 
                        PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../../..
  )
endif()
if (ENABLE_TESTS)
  add_subdirectory(tests)
endif()

if (ENABLE_IMSCOPE OR ENABLE_IMSCOPE_RECORD)
  add_subdirectory(imscope)
endif()

add_library(PHY_TOOLS
  file_output.c
  dfts_load.c
  log2_approx.c
  cdot_prod.c
  signal_energy.c
  dB_routines.c
  sqrt.c
  get_sin_cos.c
  oai_arith_operations.c
)
target_link_libraries(PHY_TOOLS PRIVATE shlib_loader)
