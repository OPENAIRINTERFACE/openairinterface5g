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

# Author: Lionel GAUTHIER

cmake_minimum_required(VERSION 2.8)
set(CMAKE_VERBOSE_MAKEFILE ON)


project(at_nas_ue)


ENABLE_LANGUAGE(C)
#Sends the -std=c99 flag to the gcc compiler
#add_definitions(-std=c99)

ADD_DEFINITIONS(-D_GNU_SOURCE)

set (OPENAIR_DIR     $ENV{OPENAIR_DIR})

if( NOT DEFINED OPENAIR_DIR)
  message(SEND_ERROR "OPENAIR_DIR not set: ${OPENAIR_DIR}")
endif( NOT DEFINED OPENAIR_DIR)

set(OPENAIR1_DIR          ${OPENAIR_DIR}/openair1)
set(OPENAIR2_DIR          ${OPENAIR_DIR}/openair2)
set(OPENAIR3_DIR          ${OPENAIR_DIR}/openair3)
set(OPENAIR3_DIR         ${OPENAIR_DIR}/openair3)
set(OPENAIR_TARGETS       ${OPENAIR_DIR}/targets)
set(OPENAIR_CMAKE_SCRIPTS ${OPENAIR_DIR}/cmake_targets/tools)
set(OPENAIR_BIN_DIR       ${OPENAIR_DIR}/targets/bin)
set(OPENAIR_NAS_DIR       ${OPENAIR_DIR}/openair3/NAS)


set(CMAKE_MODULE_PATH "${OPENAIR_DIR}/cmake_targets/tools/MODULES" "${CMAKE_MODULE_PATH}")

macro(add_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  add_definitions("-D${name}=${value}")
endmacro(add_option)

macro(add_boolean_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY TYPE BOOL)
  if (${value})
    add_definitions("-D${name}")
  endif (${value})
endmacro(add_boolean_option)

# compilation flags
# added CMAKER conditional compilation to not bother legacy building system
#############################################
if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)

#
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2 -std=gnu99 -Wall -Wstrict-prototypes -fno-strict-aliasing -rdynamic -funroll-loops -Wno-packed-bitfield-compat -DCMAKER")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -DMALLOC_CHECK_=3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -ggdb -DMALLOC_CHECK_=3 -O2")

##This is to detect nettle version changes between Ubuntu 14.04/16.04
#set ( nettle_cmd "nettle-hash" )
#set ( nettle_arg "-V" )
#execute_process(COMMAND  ${nettle_cmd} ${nettle_arg}   RESULT_VARIABLE rv OUTPUT_VARIABLE ov ERROR_VARIABLE ev)
#
#string(REGEX MATCH "[+-]?[0-9]+([.][0-9]+)?" nv ${ov})
#
#message("NETTLE_VERSION = ${nv}")
#
## we need to remove decimal as floating point arithematic does not work properly with C preprocessor
#STRING(REGEX REPLACE "[.]" "" nv ${nv})
#
#if ("${nv}" STREQUAL "")
#  message( FATAL_ERROR "The nettle version not detected properly. Try to run build_oai -I again" )
#endif()
#
#set (NETTLE_VERSION "${nv}")
#add_definitions("-DNETTLE_VERSION=${NETTLE_VERSION}")

include(FindPkgConfig)

pkg_search_module(NETTLE nettle)
if(NOT ${NETTLE_FOUND})
  message( FATAL_ERROR "PACKAGE nettle not found: some targets will fail. Run build_oai -I again!")
else()
  include_directories(${NETTLE_INCLUDE_DIRS})
endif()

message ("NETTLE VERSION_INSTALLED  = ${NETTLE_VERSION}")

string(REGEX REPLACE "([0-9]+).*" "\\1" NETTLE_VERSION_MAJOR ${NETTLE_VERSION})
string(REGEX REPLACE "[0-9]+\\.([0-9]+).*" "\\1" NETTLE_VERSION_MINOR ${NETTLE_VERSION})
message ("NETTLE_VERSION_MAJOR = ${NETTLE_VERSION_MAJOR}")
message ("NETTLE_VERSION_MINOR = ${NETTLE_VERSION_MINOR}")

if ("${NETTLE_VERSION_MAJOR}" STREQUAL "" OR "${NETTLE_VERSION_MINOR}" STREQUAL "")
  message( FATAL_ERROR "The nettle version not detected properly. Try to run build_oai -I again" )
endif()

add_definitions("-DNETTLE_VERSION_MAJOR=${NETTLE_VERSION_MAJOR}")
add_definitions("-DNETTLE_VERSION_MINOR=${NETTLE_VERSION_MINOR}")


pkg_search_module(OPENSSL openssl REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})


pkg_search_module(CRYPTO libcrypto REQUIRED)
include_directories(${CRYPTO_INCLUDE_DIRS})


set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

set (FIRMWARE_VERSION "No svn information")
add_definitions("-DFIRMWARE_VERSION=\"${FIRMWARE_VERSION}\"")

add_boolean_option(LINUX   True  "????")
add_boolean_option(ENB_MODE   True  "????") # to avoid backtrace

##########################
# NAS LAYER OPTIONS
##########################
add_boolean_option(ENABLE_NAS_UE_LOGGING   True  "????")
add_boolean_option(NAS_BUILT_IN_EPC        False "MME NAS layer not present in this executable")
add_boolean_option(NAS_BUILT_IN_UE         False  "UE NAS layer present in this executable")
add_boolean_option(NAS_UE                  True  "NAS UE INSTANCE (<> NAS_MME)")
add_boolean_option(NAS_MME                 False "NAS_UE and NAS_MME are incompatible options")

################################################################################
# SECU LIB
################################################################################
set(secu_cn_SRC
  ${OPENAIR3_DIR}/SECU/kdf.c
  ${OPENAIR3_DIR}/SECU/rijndael.c
  ${OPENAIR3_DIR}/SECU/snow3g.c
  ${OPENAIR3_DIR}/SECU/key_nas_deriver.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eea1.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eia1.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eea2.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eia2.c
  )
add_library(secu_cn ${secu_cn_SRC})
target_include_directories(secu_cn PRIVATE 
                            ${OPENAIR_DIR}/common/utils/itti
                            ${OPENAIR_DIR}/openair2/COMMON
                            ${OPENAIR3_DIR}/UTILS
)

################################################################################
# UTIL LIB
################################################################################
set(util_SRC
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/device.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/memory.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/nas_log.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/nas_timer.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/OctetString.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/parser.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/TLVDecoder.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/TLVEncoder.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/socket.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/stty.c
)
set(util_HDR
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/device.h
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/memory.h
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/nas_log.h
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/nas_timer.h
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/OctetString.h
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/parser.h
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/TLVDecoder.h
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/TLVEncoder.h
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/socket.h
)

add_library(util ${util_SRC} ${util_HDR})
target_include_directories(util PRIVATE ${OPENAIR_NAS_DIR}/COMMON/UTIL
                                        ${OPENAIR_NAS_DIR}/COMMON
                                        )


################################################################################
# api_user LIB
################################################################################
set(api_user_SRC
    ${OPENAIR_NAS_DIR}/UE/API/USER/at_command.c
    ${OPENAIR_NAS_DIR}/UE/API/USER/at_error.c
    ${OPENAIR_NAS_DIR}/UE/API/USER/at_response.c
    ${OPENAIR_NAS_DIR}/UE/API/USER/user_api.c
    ${OPENAIR_NAS_DIR}/UE/API/USER/user_indication.c
)

set(api_user_HDR
    ${OPENAIR_NAS_DIR}/UE/API/USER/at_command.h
    ${OPENAIR_NAS_DIR}/UE/API/USER/at_error.h
    ${OPENAIR_NAS_DIR}/UE/API/USER/at_response.h
    ${OPENAIR_NAS_DIR}/UE/API/USER/user_api.h
    ${OPENAIR_NAS_DIR}/UE/API/USER/user_indication.h
)

add_library(api_user ${api_user_SRC} ${api_user_HDR})
target_include_directories(api_user PRIVATE ${OPENAIR_NAS_DIR}/UE/API/USER
                                            ${OPENAIR_NAS_DIR}/COMMON
                                            ${OPENAIR_NAS_DIR}/UE
                                            )

################################################################################
# api_usim LIB
################################################################################
set(api_usim_SRC
    ${OPENAIR_NAS_DIR}/UE/API/USIM/aka_functions.c
    ${OPENAIR_NAS_DIR}/UE/API/USIM/usim_api.c
)

set(api_usim_HDR
    ${OPENAIR_NAS_DIR}/UE/API/USIM/aka_functions.h
    ${OPENAIR_NAS_DIR}/UE/API/USIM/usim_api.h

)

add_library(api_usim ${api_usim_SRC} ${api_usim_HDR})
target_include_directories(api_usim PRIVATE ${OPENAIR_NAS_DIR}/UE/API/USIM)

################################################################################
# api_network LIB
################################################################################
set(api_network_SRC
    ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK/as_message.c
    ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK/nas_message.c
    ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK/network_api.c
)
set(api_network_HDR
    ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK/as_message.h
    ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK/nas_message.h
    ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK/network_api.h
)

add_library(api_network ${api_network_SRC} ${api_network_HDR})
target_include_directories(api_network PRIVATE ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK
                                               ${OPENAIR_NAS_DIR}/COMMON
                                               )


################################################################################
# emm LIB
################################################################################
set(emm_SRC
    ${OPENAIR_NAS_DIR}/UE/EMM/Attach.c
    ${OPENAIR_NAS_DIR}/UE/EMM/Authentication.c
    ${OPENAIR_NAS_DIR}/UE/EMM/Detach.c
    ${OPENAIR_NAS_DIR}/UE/EMM/emm_main.c
    ${OPENAIR_NAS_DIR}/UE/EMM/EmmStatusHdl.c
    ${OPENAIR_NAS_DIR}/UE/EMM/Identification.c
    ${OPENAIR_NAS_DIR}/UE/EMM/IdleMode.c
    ${OPENAIR_NAS_DIR}/UE/EMM/LowerLayer.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SecurityModeControl.c
    ${OPENAIR_NAS_DIR}/UE/EMM/ServiceRequestHdl.c
    ${OPENAIR_NAS_DIR}/UE/EMM/TrackingAreaUpdate.c

    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/emm_msg.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachAccept.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachComplete.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachReject.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachAccept.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachComplete.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachReject.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AuthenticationFailure.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AuthenticationReject.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AuthenticationRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AuthenticationResponse.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/CsServiceNotification.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/DetachAccept.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/DetachRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/DownlinkNasTransport.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/EmmInformation.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/emm_msg.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/EmmStatus.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/ExtendedServiceRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/GutiReallocationCommand.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/GutiReallocationComplete.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/IdentityRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/IdentityResponse.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/SecurityModeCommand.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/SecurityModeComplete.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/SecurityModeReject.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/ServiceReject.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/ServiceRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/TrackingAreaUpdateAccept.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/TrackingAreaUpdateComplete.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/TrackingAreaUpdateReject.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/TrackingAreaUpdateRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/UplinkNasTransport.c
    
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_as.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmDeregisteredAttachNeeded.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmDeregisteredAttemptingToAttach.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmDeregistered.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmDeregisteredInitiated.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmDeregisteredLimitedService.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmDeregisteredNoCellAvailable.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmDeregisteredNoImsi.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmDeregisteredNormalService.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmDeregisteredPlmnSearch.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_esm.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_fsm.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmNull.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_recv.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_reg.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmRegisteredAttemptingToUpdate.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmRegistered.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmRegisteredImsiDetachInitiated.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmRegisteredInitiated.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmRegisteredLimitedService.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmRegisteredNoCellAvailable.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmRegisteredNormalService.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmRegisteredPlmnSearch.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmRegisteredUpdateNeeded.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_sap.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_send.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmServiceRequestInitiated.c
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/EmmTrackingAreaUpdatingInitiated.c
    
)

set(emm_HDR
    ${OPENAIR_NAS_DIR}/UE/EMM/emmData.h
    ${OPENAIR_NAS_DIR}/UE/EMM/emm_main.h
    ${OPENAIR_NAS_DIR}/UE/EMM/emm_proc.h
    ${OPENAIR_NAS_DIR}/UE/EMM/IdleMode.h
    ${OPENAIR_NAS_DIR}/UE/EMM/LowerLayer.h
    
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachAccept.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachComplete.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachReject.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AttachRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AuthenticationFailure.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AuthenticationReject.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AuthenticationRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/AuthenticationResponse.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/CsServiceNotification.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/DetachAccept.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/DetachRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/DownlinkNasTransport.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/emm_cause.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/EmmInformation.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/emm_msgDef.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/emm_msg.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/EmmStatus.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/ExtendedServiceRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/GutiReallocationCommand.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/GutiReallocationComplete.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/IdentityRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/IdentityResponse.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/NASSecurityModeCommand.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/NASSecurityModeComplete.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/SecurityModeReject.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/ServiceReject.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/ServiceRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/TrackingAreaUpdateAccept.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/TrackingAreaUpdateComplete.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/TrackingAreaUpdateReject.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/TrackingAreaUpdateRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG/UplinkNasTransport.h
    
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_asDef.h
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_as.h
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_esmDef.h
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_esm.h
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_fsm.h
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_recv.h
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_regDef.h
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_reg.h
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_sap.h
    ${OPENAIR_NAS_DIR}/UE/EMM/SAP/emm_send.h

)

add_library(emm ${emm_SRC} ${emm_HDR})

target_include_directories(emm PRIVATE 
                     ${OPENAIR_DIR}/common/utils/msc 
                     ${OPENAIR_DIR}/common/utils
                     ${OPENAIR_DIR}/openair2/COMMON
                     ${OPENAIR_NAS_DIR}/UE
                     ${OPENAIR_NAS_DIR}/UE/API/USIM 
                     ${OPENAIR_NAS_DIR}/UE/EMM
                     ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG
                     ${OPENAIR_NAS_DIR}/UE/EMM/SAP
                     ${OPENAIR_NAS_DIR}/COMMON/IES
)


################################################################################
# esm LIB
################################################################################
set(esm_SRC
    ${OPENAIR_NAS_DIR}/UE/ESM/DedicatedEpsBearerContextActivation.c
    ${OPENAIR_NAS_DIR}/UE/ESM/DefaultEpsBearerContextActivation.c
    ${OPENAIR_NAS_DIR}/UE/ESM/EpsBearerContextDeactivation.c
    ${OPENAIR_NAS_DIR}/UE/ESM/esm_ebr.c
    ${OPENAIR_NAS_DIR}/UE/ESM/esm_ebr_context.c
    ${OPENAIR_NAS_DIR}/UE/ESM/esm_ip.c
    ${OPENAIR_NAS_DIR}/UE/ESM/esm_main.c
    ${OPENAIR_NAS_DIR}/UE/ESM/esm_pt.c
    ${OPENAIR_NAS_DIR}/UE/ESM/EsmStatusHdl.c
    ${OPENAIR_NAS_DIR}/UE/ESM/PdnConnectivity.c
    ${OPENAIR_NAS_DIR}/UE/ESM/PdnDisconnect.c
    
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextAccept.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextReject.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDefaultEpsBearerContextAccept.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDefaultEpsBearerContextReject.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDefaultEpsBearerContextRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/BearerResourceAllocationReject.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/BearerResourceAllocationRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/BearerResourceModificationReject.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/BearerResourceModificationRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/DeactivateEpsBearerContextAccept.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/DeactivateEpsBearerContextRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/EsmInformationRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/EsmInformationResponse.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/esm_msg.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/EsmStatus.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ModifyEpsBearerContextAccept.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ModifyEpsBearerContextReject.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ModifyEpsBearerContextRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/PdnConnectivityReject.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/PdnConnectivityRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/PdnDisconnectReject.c
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/PdnDisconnectRequest.c
    
    ${OPENAIR_NAS_DIR}/UE/ESM/SAP/esm_recv.c
    ${OPENAIR_NAS_DIR}/UE/ESM/SAP/esm_sap.c
    ${OPENAIR_NAS_DIR}/UE/ESM/SAP/esm_send.c
)

set(esm_HDR
    ${OPENAIR_TARGETS}/COMMON/openairinterface5g_limits.h
    ${OPENAIR_NAS_DIR}/UE/ESM/esm_main.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextAccept.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextReject.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDefaultEpsBearerContextAccept.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDefaultEpsBearerContextReject.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ActivateDefaultEpsBearerContextRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/BearerResourceAllocationReject.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/BearerResourceAllocationRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/BearerResourceModificationReject.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/BearerResourceModificationRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/DeactivateEpsBearerContextAccept.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/DeactivateEpsBearerContextRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/esm_cause.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/EsmInformationRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/EsmInformationResponse.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/esm_msgDef.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/esm_msg.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/EsmStatus.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ModifyEpsBearerContextAccept.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ModifyEpsBearerContextReject.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/ModifyEpsBearerContextRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/PdnConnectivityReject.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/PdnConnectivityRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/PdnDisconnectReject.h
    ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG/PdnDisconnectRequest.h
    ${OPENAIR_NAS_DIR}/UE/ESM/SAP/esm_recv.h
    ${OPENAIR_NAS_DIR}/UE/ESM/SAP/esm_sap.h
    ${OPENAIR_NAS_DIR}/UE/ESM/SAP/esm_send.h
    ${OPENAIR_DIR}/common/utils/utils.h
)

add_library(esm ${esm_SRC} ${esm_HDR})

target_include_directories(esm PRIVATE 
                     ${OPENAIR_DIR}/common/utils
                     ${OPENAIR_NAS_DIR}/UE
                     ${OPENAIR_NAS_DIR}/UE/API/USER 
                     ${OPENAIR_NAS_DIR}/UE/ESM
                     ${OPENAIR_TARGETS}/COMMON
                     ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG
                     ${OPENAIR_NAS_DIR}/UE/ESM/SAP
                     ${OPENAIR_NAS_DIR}/COMMON/IES
)

################################################################################
# ies LIB
################################################################################
set(ies_SRC
    ${OPENAIR_NAS_DIR}/COMMON/IES/AccessPointName.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/AdditionalUpdateResult.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/AdditionalUpdateType.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/ApnAggregateMaximumBitRate.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/AuthenticationFailureParameter.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/AuthenticationParameterAutn.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/AuthenticationParameterRand.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/AuthenticationResponseParameter.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/CipheringKeySequenceNumber.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/Cli.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/CsfbResponse.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/DaylightSavingTime.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/DetachType.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/DrxParameter.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EmergencyNumberList.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EmmCause.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsAttachResult.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsAttachType.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsBearerContextStatus.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsBearerIdentity.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsMobileIdentity.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsNetworkFeatureSupport.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsQualityOfService.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsUpdateResult.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsUpdateType.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EsmCause.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EsmInformationTransferFlag.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/EsmMessageContainer.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/GprsTimer.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/GutiType.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/IdentityType2.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/ImeisvRequest.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/KsiAndSequenceNumber.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/LcsClientIdentity.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/LcsIndicator.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/LinkedEpsBearerIdentity.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/LlcServiceAccessPointIdentifier.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/LocationAreaIdentification.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/MessageType.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/MobileIdentity.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/MobileStationClassmark2.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/MobileStationClassmark3.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/MsNetworkCapability.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/MsNetworkFeatureSupport.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/NasKeySetIdentifier.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/NasMessageContainer.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/NasRequestType.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/NasSecurityAlgorithms.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/NetworkName.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/Nonce.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/PacketFlowIdentifier.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/PagingIdentity.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/PdnAddress.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/PdnType.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/PlmnList.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/ProcedureTransactionIdentity.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/ProtocolConfigurationOptions.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/ProtocolDiscriminator.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/PTmsiSignature.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/QualityOfService.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/RadioPriority.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/SecurityHeaderType.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/ServiceType.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/ShortMac.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/SsCode.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/SupportedCodecList.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/TimeZoneAndTime.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/TimeZone.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/TmsiStatus.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/TrackingAreaIdentity.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/TrackingAreaIdentityList.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/TrafficFlowAggregateDescription.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/TrafficFlowTemplate.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/TransactionIdentifier.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/UeNetworkCapability.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/UeRadioCapabilityInformationUpdateNeeded.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/UeSecurityCapability.c
    ${OPENAIR_NAS_DIR}/COMMON/IES/VoiceDomainPreferenceAndUeUsageSetting.c
)
set(ies_HDR
    ${OPENAIR_NAS_DIR}/COMMON/IES/AccessPointName.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/AdditionalUpdateResult.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/AdditionalUpdateType.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/ApnAggregateMaximumBitRate.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/AuthenticationFailureParameter.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/AuthenticationParameterAutn.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/AuthenticationParameterRand.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/AuthenticationResponseParameter.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/CipheringKeySequenceNumber.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/Cli.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/CsfbResponse.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/DaylightSavingTime.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/DetachType.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/DrxParameter.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EmergencyNumberList.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EmmCause.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsAttachResult.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsAttachType.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsBearerContextStatus.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsBearerIdentity.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsMobileIdentity.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsNetworkFeatureSupport.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsQualityOfService.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsUpdateResult.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EpsUpdateType.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EsmCause.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EsmInformationTransferFlag.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/EsmMessageContainer.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/GprsTimer.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/GutiType.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/IdentityType2.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/ImeisvRequest.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/KsiAndSequenceNumber.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/LcsClientIdentity.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/LcsIndicator.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/LinkedEpsBearerIdentity.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/LlcServiceAccessPointIdentifier.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/LocationAreaIdentification.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/MessageType.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/MobileIdentity.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/MobileStationClassmark2.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/MobileStationClassmark3.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/MsNetworkCapability.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/MsNetworkFeatureSupport.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/NasKeySetIdentifier.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/NasMessageContainer.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/NasRequestType.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/NasSecurityAlgorithms.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/NetworkName.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/Nonce.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/PacketFlowIdentifier.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/PagingIdentity.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/PdnAddress.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/PdnType.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/PlmnList.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/ProcedureTransactionIdentity.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/ProtocolConfigurationOptions.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/ProtocolDiscriminator.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/PTmsiSignature.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/QualityOfService.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/RadioPriority.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/SecurityHeaderType.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/ServiceType.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/ShortMac.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/SsCode.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/SupportedCodecList.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/TimeZoneAndTime.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/TimeZone.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/TmsiStatus.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/TrackingAreaIdentity.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/TrackingAreaIdentityList.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/TrafficFlowAggregateDescription.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/TrafficFlowTemplate.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/TransactionIdentifier.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/UeNetworkCapability.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/UeRadioCapabilityInformationUpdateNeeded.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/UeSecurityCapability.h
    ${OPENAIR_NAS_DIR}/COMMON/IES/VoiceDomainPreferenceAndUeUsageSetting.h

)

add_library(ies ${ies_SRC} ${ies_HDR})

target_include_directories(ies PRIVATE 
                     ${OPENAIR_NAS_DIR}/COMMON/IES 
)



################################################################################
# EXECUTABLE at_nas_ue
################################################################################
include_directories( 
                     ${OPENAIR_TARGETS}/COMMON
                     ${OPENAIR_NAS_DIR}/UE
                     ${OPENAIR_DIR}/common/utils
                     ${OPENAIR_DIR}/common/utils/msc 
                     ${OPENAIR3_DIR}/COMMON 
                     ${OPENAIR3_DIR}/SECU 
                     ${OPENAIR_NAS_DIR}/src 
                     ${OPENAIR_NAS_DIR}/COMMON 
                     ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK 
                     ${OPENAIR_NAS_DIR}/UE/API/USER 
                     ${OPENAIR_NAS_DIR}/UE/EMM 
                     ${OPENAIR_NAS_DIR}/COMMON/EMM/MSG 
                     ${OPENAIR_NAS_DIR}/UE/EMM/SAP 
                     ${OPENAIR_NAS_DIR}/UE/ESM 
                     ${OPENAIR_NAS_DIR}/UE/ESM/SAP 
                     ${OPENAIR_NAS_DIR}/COMMON/ESM/MSG 
                     ${OPENAIR_NAS_DIR}/COMMON/IES 
                     ${OPENAIR_NAS_DIR}/COMMON/UTIL )
                     
ADD_EXECUTABLE(at_nas_ue  ${OPENAIR_NAS_DIR}/UE/UEprocess.c 
                          ${OPENAIR_NAS_DIR}/UE/nas_network.c  
                          ${OPENAIR_NAS_DIR}/UE/nas_parser.c
                          ${OPENAIR_NAS_DIR}/UE/nas_proc.c  
                          ${OPENAIR_NAS_DIR}/UE/nas_user.c  
                          ${OPENAIR_DIR}/common/utils/utils.c
                          ${OPENAIR_DIR}/common/utils/system.c
                          )
                          
target_link_libraries (at_nas_ue 
                       -Wl,-whole-archive 
                       api_network api_user api_usim emm esm ies util secu_cn
                       -Wl,-no-whole-archive
                       ${NETTLE_LIBRARIES} 
                       ${OPENSSL_LIBRARIES} 
                       ${CMAKE_THREAD_LIBS_INIT} m )


                



