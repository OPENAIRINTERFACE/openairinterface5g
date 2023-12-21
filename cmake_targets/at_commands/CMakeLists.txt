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
#############################################
if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)

#
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2 -std=gnu99 -Wall -Wstrict-prototypes -fno-strict-aliasing -rdynamic -funroll-loops")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -DMALLOC_CHECK_=3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -ggdb -DMALLOC_CHECK_=3 -O2")

include(FindPkgConfig)

pkg_search_module(OPENSSL openssl REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})


pkg_search_module(CRYPTO libcrypto REQUIRED)
include_directories(${CRYPTO_INCLUDE_DIRS})


set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

set (FIRMWARE_VERSION "No svn information")
add_definitions("-DFIRMWARE_VERSION=\"${FIRMWARE_VERSION}\"")

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
  ${OPENAIR3_DIR}/SECU/aes_128_ctr.c
  ${OPENAIR3_DIR}/SECU/aes_128_cbc_cmac.c
  ${OPENAIR3_DIR}/SECU/sha_256_hmac.c 
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
    ${OPENAIR_DIR}/common/utils/mem/memory.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/nas_log.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/nas_timer.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/OctetString.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/parser.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/TLVDecoder.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/TLVEncoder.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/socket.c
    ${OPENAIR_NAS_DIR}/COMMON/UTIL/stty.c
)

add_library(util ${util_SRC})
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

add_library(api_user ${api_user_SRC})
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

add_library(api_usim ${api_usim_SRC})
target_include_directories(api_usim PRIVATE ${OPENAIR_NAS_DIR}/UE/API/USIM)

################################################################################
# api_network LIB
################################################################################
set(api_network_SRC
    ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK/as_message.c
    ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK/nas_message.c
    ${OPENAIR_NAS_DIR}/COMMON/API/NETWORK/network_api.c
)

add_library(api_network ${api_network_SRC})
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

add_library(emm ${emm_SRC})

target_include_directories(emm PRIVATE 
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

add_library(esm ${esm_SRC})

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

add_library(ies ${ies_SRC})

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
                       ${OPENSSL_LIBRARIES} 
                       ${CMAKE_THREAD_LIBS_INIT} m )


                



