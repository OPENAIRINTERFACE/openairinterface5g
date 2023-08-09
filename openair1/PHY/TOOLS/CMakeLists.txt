#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */
#! \file openair1/PHY/TOOLS/CMakelists.txt
#* \brief: build rules and checks for softscope shared libraries
#* \author Francois TABURET
#* \date 2023
#* \version 1.0
#* \company NOKIA BellLabs France
#* \email: francois.taburet@nokia-bell-labs.com
#* \note
#* \warning
#*/

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

add_library(enbscope MODULE lte_enb_scope.c $<TARGET_OBJECTS:xforms_common>)
target_link_libraries(enbscope PUBLIC forms PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

add_library(uescope MODULE lte_ue_scope.c $<TARGET_OBJECTS:xforms_common>)
target_link_libraries(uescope PUBLIC forms PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

add_library(nrscope MODULE nr_phy_scope.c)
target_link_libraries(nrscope PUBLIC forms PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

# all libraries should be written to root build dir (default creates the same hierarchie under build as existing for sources)
set_target_properties(enbscope uescope nrscope 
                      PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../../..
)
