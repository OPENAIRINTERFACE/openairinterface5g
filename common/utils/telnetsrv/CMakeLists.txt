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
#! \file common/utils/telnetsrv/CMakelists.txt
#* \brief: build rules and checks for web server shared libraries
#* \author Francois TABURET
#* \date 2023
#* \version 1.0
#* \company NOKIA BellLabs France
#* \email: francois.taburet@nokia-bell-labs.com
#* \note
#* \warning
#*/
add_library(telnetsrv MODULE
            telnetsrv.c
            telnetsrv_proccmd.c
            telnetsrv_loader.c
            telnetsrv_measurements.c
)
target_link_libraries(telnetsrv PRIVATE history)
target_link_libraries(telnetsrv PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

message(STATUS "Add enb specific telnet functions in libtelnetsrv_enb.so")
add_library(telnetsrv_enb MODULE
            telnetsrv_enb_measurements.c
            telnetsrv_enb_phycmd.c
)
add_dependencies(telnetsrv telnetsrv_enb)
target_link_libraries(telnetsrv_enb PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

message(STATUS "No specific telnet functions for gnb")

message(STATUS "No specific telnet functions for 4Gue")

message(STATUS "Add 5Gue specific telnet functions in libtelnetsrv_5Gue.so")
add_library(telnetsrv_5Gue MODULE telnetsrv_5Gue_measurements.c)
add_dependencies(telnetsrv telnetsrv_5Gue)
target_link_libraries(telnetsrv_5Gue PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

message(STATUS "Add CI specific telnet functions in libtelnetsrv_ci.so")
add_library(telnetsrv_ci MODULE telnetsrv_ci.c)
target_link_libraries(telnetsrv_ci PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
add_dependencies(telnetsrv telnetsrv_ci)

message(STATUS "Add bearer specific telnet functions in libtelnetsrv_bearer.so")
add_library(telnetsrv_bearer MODULE telnetsrv_bearer.c)
target_link_libraries(telnetsrv_bearer PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
add_dependencies(telnetsrv telnetsrv_bearer)

# all libraries should be written to root build dir
set_target_properties(telnetsrv telnetsrv_enb telnetsrv_5Gue telnetsrv_ci telnetsrv_bearer
                      PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../../..
)

