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

# Author: laurent THOMAS, Lionel GAUTHIER

cmake_minimum_required (VERSION 3.12)
project (OpenAirInterface LANGUAGES C CXX)

#########################################################
# Base directories, compatible with legacy OAI building #
#########################################################
set (OPENAIR_DIR     ${CMAKE_SOURCE_DIR})

include("cmake_targets/macros.cmake")

##############################
### CCache: reduce compilation time
##############################
#use ccache if available
option(CCACHE_ACTIVE "CCache" ON)
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND AND CCACHE_ACTIVE)
  if(${CMAKE_VERSION} VERSION_LESS "3.4.0") 
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    message(STATUS "Found ccache in ${CCACHE_FOUND}. Using ccache. CMake < 3.4")
  else()
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_FOUND}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_FOUND}")
    message(STATUS "Found ccache in ${CCACHE_FOUND}. Using ccache. CMake >= 3.4")
  endif()
else()
  message(STATUS "Ccache not found. Consider installing it for faster compilation. Command: sudo apt/dnf install ccache")
endif()

# System packages that are required
# We use either the cmake buildin, in ubuntu are in: /usr/share/cmake*/Modules/
# or cmake provide a generic interface to pkg-config that widely used
###################################
include(FindPkgConfig)

# Check if asn1c is installed, and check if it supports all options we need
# a user can provide ASN1C_EXEC to override an asn1c to use (e.g., parallel
# installation)
find_program(ASN1C_EXEC_PATH asn1c HINTS /opt/asn1c/bin)
set(ASN1C_EXEC ${ASN1C_EXEC_PATH} CACHE FILEPATH "path to asn1c executable")
if(NOT ASN1C_EXEC)
    message(FATAL_ERROR "No asn1c found!
You might want to re-run ./build_oai -I
Or provide a path to asn1c using
./build_oai ... --cmake-opt -DASN1C_EXEC=/path/to/asn1c
")
endif()
check_option(${ASN1C_EXEC} -gen-APER ASN1C_EXEC)
check_option(${ASN1C_EXEC} -no-gen-UPER ASN1C_EXEC)
check_option(${ASN1C_EXEC} -no-gen-JER ASN1C_EXEC)
check_option(${ASN1C_EXEC} -no-gen-BER ASN1C_EXEC)
check_option(${ASN1C_EXEC} -no-gen-OER ASN1C_EXEC)

#########################################################
# Base directories, compatible with legacy OAI building #
#########################################################
set (NFAPI_DIR       ${OPENAIR_DIR}/nfapi/open-nFAPI)
set (NFAPI_USER_DIR  ${OPENAIR_DIR}/nfapi/oai_integration)
set (OPENAIR1_DIR    ${OPENAIR_DIR}/openair1)
set (OPENAIR2_DIR    ${OPENAIR_DIR}/openair2)
set (OPENAIR3_DIR    ${OPENAIR_DIR}/openair3)
set (OPENAIR3_DIR    ${OPENAIR_DIR}/openair3)
set (OPENAIR_CMAKE   ${OPENAIR_DIR}/cmake_targets)
set (OPENAIR_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY})

project (OpenAirInterface)

####################################################
# compilation flags
#############################################

#set(CMAKE_BUILD_TYPE "Debug")
if (CMAKE_BUILD_TYPE STREQUAL "")
   set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()
message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
add_list_string_option(CMAKE_BUILD_TYPE "RelWithDebInfo" "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." Debug Release RelWithDebInfo MinSizeRel)

# in case /proc/cpuinfo exists we want to inspect available Intrinsics
# -so not to go always through SIMDE emulation
# -so to avoid AVX512 instructions generation by gcc   
if(EXISTS "/proc/cpuinfo" AND NOT CROSS_COMPILE)
  file(STRINGS "/proc/cpuinfo" CPUFLAGS REGEX flags LIMIT_COUNT 1)
else()
  message(WARNING "did not find /proc/cpuinfo -- not setting any x86-specific compilation variables")
endif()
 
eval_boolean(AUTODETECT_AVX512 DEFINED CPUFLAGS AND CPUFLAGS MATCHES "avx512")
add_boolean_option(AVX512 ${AUTODETECT_AVX512} "Whether AVX512 intrinsics is available on the host processor" ON)

eval_boolean(AUTODETECT_AVX2 DEFINED CPUFLAGS AND CPUFLAGS MATCHES "avx2")
add_boolean_option(AVX2 ${AUTODETECT_AVX2} "Whether AVX2 intrinsics is available on the host processor" ON)

message(STATUS "CPU architecture is ${CMAKE_SYSTEM_PROCESSOR}")
if (CROSS_COMPILE)
  message(STATUS "setting as aarch64")
  set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -gdwarf-2 -lgcc -lrt")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  # The following intrinsics are assumed to be available on any x86 system
  # (avx, f16c, fma, gnfi, mmx, pclmul, sse, sse2, sse3, xop)
  set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -DSIMDE_X86_AVX_NATIVE -DSIMDE_X86_AVX_NATIVE -DSIMDE_X86_F16C_NATIVE -DSIMDE_X86_FMA_NATIVE -DSIMDE_X86_GFNI_NATIVE -DSIMDE_X86_MMX_NATIVE -DSIMDE_X86_PCLMUL_NATIVE -DSIMDE_X86_SSE2_NATIVE -DSIMDE_X86_SSE3_NATIVE -DSIMDE_X86_SSE_NATIVE -DSIMDE_X86_XOP_HAVE_COM_ -DSIMDE_X86_XOP_NATIVE")
  message(STATUS "AVX512 intrinsics are ${AVX512}")
  if(${AVX512})
    set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -DSIMDE_X86_AVX512BW_NATIVE -DSIMDE_X86_AVX512F_NATIVE -DSIMDE_X86_AVX512VL_NATIVE -mavx512bw -march=skylake-avx512 -mtune=skylake-avx512")
  else()
    set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -mno-avx512f -march=native")
  endif()
  message(STATUS "AVX2 intrinsics are ${AVX2}")
  if(${AVX2})
    set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -DSIMDE_X86_AVX2_NATIVE -DSIMDE_X86_VPCLMULQDQ_NATIVE")
  endif()
  if (CPUINFO MATCHES "sse4_1")
    set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -DSIMDE_X86_SSE4_1_NATIVE")
  endif()
  if(CPUINFO MATCHES "sse4_2")
    set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -DSIMDE_X86_SSE4_2_NATIVE")
  endif()
  if(CPUINFO MATCHES "ssse3")
    set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -DSIMDE_X86_SSSE3_NATIVE")
  endif()
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
  set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -gdwarf-2 -mfloat-abi=hard -mfpu=neon -lgcc -lrt")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
  set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -gdwarf-2 -lgcc -lrt")
else()
  message(FATAL_ERROR "compile for CPU architecture ${CPUARCH}, CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if (NOT CROSS_COMPILE)
  set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -march=native")
else ()
  set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -march=armv8-a")
endif()

# add autotools definitions that were maybe used!
add_definitions("-DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_FCNTL_H=1 -DHAVE_ARPA_INET_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_SOCKET_H=1 -DHAVE_STRERROR=1 -DHAVE_SOCKET=1 -DHAVE_MEMSET=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_STDLIB_H=1 -DHAVE_MALLOC=1 -DHAVE_LIBSCTP")

set(commonOpts "-pipe -fPIC -Wall -fno-strict-aliasing")
# GNU C/C++ Compiler might throw many warnings without packed-bitfield-compat, see man page
# also, we need -rdynamic to incorporate all symbols in shared objects, again, see man page
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(commonOpts "${commonOpts} -Wno-packed-bitfield-compat -rdynamic")
endif()

set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} ${C_FLAGS_PROCESSOR} ${commonOpts} -std=gnu11 -funroll-loops")
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} ${C_FLAGS_PROCESSOR}  ${commonOpts} -std=c++11")


add_boolean_option(SANITIZE_ADDRESS False "enable the address sanitizer (ASan)" ON)
if (SANITIZE_ADDRESS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fno-common")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fno-common")
  # There seems to be some incompatibility with pthread_create and the RT scheduler, which
  # results in pthread_create hanging.
  #
  # When we switch from Ubuntu 16.04 to 18.04, we found that running with the address sanitizer,
  # the pthread_create function calls were not working. The inital thought was that we were
  # trying to create a thread that was not-blocking and would eventually crash the machine during
  # the run. After more debugging, we found that we would never even start the thread. We narrowed
  # down the first two instances of pthread_create in the gNB and NR UE to be sctp_eNB_task and
  # one_thread, respectively. We found that adding sleeps, and various other pauses to the threads
  # had not effect. From there, we found that if we add an abort(); prior to the thread loop, we
  # do not execute that. This indicated to us that the problem is not likely to be a non-blocking
  # thread, but perhaps and issue with pthread_create itself. From there we begain to research the
  # issue on the web. See: https://github.com/google/sanitizers/issues/1125
  #
  # Google searching indicates this appears to be a problem since at least 2018. This could be something
  # wrong in the pthread library, or something subtly wrong in this CMakeLists.txt. Use Ubuntu 20.04 instead.
endif ()

add_boolean_option(SANITIZE_THREAD False "enable the address sanitizer (TSan)" ON)
if (SANITIZE_THREAD)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -fno-common")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -fno-common")
  set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()

add_boolean_option(SANITIZE_UNDEFINED False "enable the undefined behavior sanitizer (UBSan)" ON)
if (SANITIZE_UNDEFINED)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -fno-sanitize-recover=all")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-sanitize-recover=all")
endif ()

add_boolean_option(SANITIZE_MEMORY False "enable the memory sanitizer (MSan, requires clang, incompatible with ASan/UBSan)" ON)
if(SANITIZE_MEMORY)
  if (SANITIZE_UNDEFINED OR SANITIZE_ADDRESS)
    message(FATAL_ERROR "memory sanitizer cannot coexist with address sanitizer or undefined behavior sanitizer, please disable either MSan or ASan and UBSan")
  endif()
  if (NOT CMAKE_C_COMPILER_ID STREQUAL "Clang" OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
    message(FATAL_ERROR "memory sanitizer requires clang, please set clang using CC=/usr/bin/clang CXX=/usr/bin/clang++ ./build_oai ...")
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer -fsanitize-recover=memory")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer -fsanitize-recover=memory")
endif()

add_definitions("-DASN_DISABLE_OER_SUPPORT -DHAVE_CONFIG_H -DHAVE_CONFIG_H_")

#########################
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -ggdb2 -Wl,-rpath -Wl,${CMAKE_CURRENT_BINARY_DIR}")
#########################
# set a flag for changes in the source code
# these changes are related to hardcoded path to include .h files
set(debugOpt "-ggdb2 -DMALLOC_CHECK_=3 -fno-delete-null-pointer-checks")
set(CMAKE_C_FLAGS_DEBUG "${debugOpt} -O0")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${debugOpt} -O2")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# Enable assert() for RelWithDebInfo builds
string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

set(GIT_BRANCH        "UNKNOWN")
set(GIT_COMMIT_HASH   "UNKNOWN")
set(GIT_COMMIT_DATE   "UNKNOWN")

find_package(Git)
if(GIT_FOUND)
  # Get the current working branch
  execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Get the latest abbreviated commit hash of the working branch
  execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Get the latest commit date of the working branch
  execute_process(
    COMMAND git log -1 --format=%cd
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

# Below is a hard-coded info
set (FIRMWARE_VERSION "No svn information")
add_definitions("-DFIRMWARE_VERSION=\"${FIRMWARE_VERSION}\"")
add_definitions("-DPACKAGE_VERSION=\"Branch: ${GIT_BRANCH} Abrev. Hash: ${GIT_COMMIT_HASH} Date: ${GIT_COMMIT_DATE}\"")
add_definitions("-DPACKAGE_BUGREPORT=\"openair4g-devel@lists.eurecom.fr\"")

# Debug related options
#########################################
add_boolean_option(DEBUG_ASN1 False "Enable ASN1 debug logs" OFF)
# asn1c skeletons have hardcoded this flag to make customized debug logs
# OAI uses this feature to re-use OAI LOG_I(ASN1, ...)
# see common/utils/config.h
add_boolean_option(TRACE_ASN1C_ENC_DEC OFF "Enable ASN1 encoder/decoder debug traces via OAI logging system" ON)
add_boolean_option(T_TRACER            True  "Activate the T tracer, a debugging/monitoring framework" ON)
add_boolean_option(UE_AUTOTEST_TRACE   False "Activate UE autotest specific logs" ON)
add_boolean_option(UE_DEBUG_TRACE      False "Activate UE debug trace" ON)
add_boolean_option(UE_TIMING_TRACE     False "Activate UE timing trace" ON)

set (OCP_ITTI ${OPENAIR_DIR}/common/utils/ocp_itti)
add_library(ITTI ${OCP_ITTI}/intertask_interface.cpp)
target_link_libraries(ITTI PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

# asn1c skeletons need this
add_definitions(-DHAVE_NETINET_IN_H)

#########################
##### E2 AGENT
#########################
set(E2_AGENT "OFF" CACHE STRING "O-RAN-compliant E2 Agent")
set_property(CACHE E2_AGENT PROPERTY STRINGS "ON" "OFF")

set(E2AP_VERSION "E2AP_V2" CACHE STRING "E2AP version")
set_property(CACHE E2AP_VERSION PROPERTY STRINGS "E2AP_V1" "E2AP_V2" "E2AP_V3")
message(STATUS "Selected E2AP_VERSION: ${E2AP_VERSION}")

set(KPM_VERSION "KPM_V2_03" CACHE STRING "The KPM SM version to use")
set_property(CACHE KPM_VERSION PROPERTY STRINGS "KPM_V2_03" "KPM_V3_00")
message(STATUS "Selected KPM Version: ${KPM_VERSION}")


##################################################
# ASN.1 grammar C code generation & dependencies #
##################################################

# RRC
######
add_library(lte_rrc
    ${OPENAIR2_DIR}/RRC/LTE/MESSAGES/asn1_msg.c
    ${OPENAIR2_DIR}/RRC/LTE/MESSAGES/asn1_msg_NB_IoT.c)
target_link_libraries(lte_rrc PUBLIC asn1_lte_rrc)
target_link_libraries(lte_rrc PRIVATE nr_rrc)


#NR RRC
#######
add_library(nr_rrc ${OPENAIR2_DIR}/RRC/NR/MESSAGES/asn1_msg.c)
target_link_libraries(nr_rrc PUBLIC asn1_nr_rrc asn1_lte_rrc)

# S1AP
##############
set(S1AP_DIR ${OPENAIR3_DIR}/S1AP)
include_directories ("${S1AP_DIR}")
add_library(s1ap
  ${S1AP_DIR}/s1ap_common.c
  ${S1AP_DIR}/s1ap_eNB.c
  ${S1AP_DIR}/s1ap_eNB_context_management_procedures.c
  ${S1AP_DIR}/s1ap_eNB_decoder.c
  ${S1AP_DIR}/s1ap_eNB_encoder.c
  ${S1AP_DIR}/s1ap_eNB_handlers.c
  ${S1AP_DIR}/s1ap_eNB_itti_messaging.c
  ${S1AP_DIR}/s1ap_eNB_management_procedures.c
  ${S1AP_DIR}/s1ap_eNB_nas_procedures.c
  ${S1AP_DIR}/s1ap_eNB_nnsf.c
  ${S1AP_DIR}/s1ap_eNB_overload.c
  ${S1AP_DIR}/s1ap_eNB_trace.c
  ${S1AP_DIR}/s1ap_eNB_ue_context.c
  )
target_link_libraries(s1ap PUBLIC asn1_s1ap)
target_link_libraries(s1ap PRIVATE nr_rrc asn1_nr_rrc_hdrs lte_rrc)

# NGAP
##############
set(NGAP_DIR ${OPENAIR3_DIR}/NGAP)
include_directories ("${NGAP_DIR}")
add_library(ngap
  ${NGAP_DIR}/ngap_gNB.c
  ${NGAP_DIR}/ngap_gNB_context_management_procedures.c
  ${NGAP_DIR}/ngap_gNB_decoder.c
  ${NGAP_DIR}/ngap_gNB_encoder.c
  ${NGAP_DIR}/ngap_gNB_handlers.c
  ${NGAP_DIR}/ngap_gNB_itti_messaging.c
  ${NGAP_DIR}/ngap_gNB_management_procedures.c
  ${NGAP_DIR}/ngap_gNB_nas_procedures.c
  ${NGAP_DIR}/ngap_gNB_nnsf.c
  ${NGAP_DIR}/ngap_gNB_overload.c
  ${NGAP_DIR}/ngap_gNB_trace.c
  ${NGAP_DIR}/ngap_gNB_ue_context.c
  )
target_link_libraries(ngap PUBLIC asn1_ngap)
target_link_libraries(ngap PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

#M2AP
##############
set(M2AP_DIR ${OPENAIR2_DIR}/M2AP)
add_library(m2ap
  ${M2AP_DIR}/m2ap_common.c
  ${M2AP_DIR}/m2ap_eNB.c
  ${M2AP_DIR}/m2ap_MCE.c
  ${M2AP_DIR}/m2ap_decoder.c
  ${M2AP_DIR}/m2ap_encoder.c
  ${M2AP_DIR}/m2ap_MCE_handler.c
  ${M2AP_DIR}/m2ap_eNB_handler.c
  ${M2AP_DIR}/m2ap_itti_messaging.c
  ${M2AP_DIR}/m2ap_eNB_management_procedures.c
  ${M2AP_DIR}/m2ap_eNB_generate_messages.c
  ${M2AP_DIR}/m2ap_MCE_management_procedures.c
  ${M2AP_DIR}/m2ap_MCE_generate_messages.c
  ${M2AP_DIR}/m2ap_ids.c
  ${M2AP_DIR}/m2ap_timers.c
  ${M2AP_DIR}/m2ap_MCE_interface_management.c
  ${M2AP_DIR}/m2ap_eNB_interface_management.c
 )
target_link_libraries(m2ap PUBLIC asn1_m2ap)
target_link_libraries(m2ap PRIVATE lte_rrc nr_rrc)
target_include_directories(m2ap PRIVATE "${M2AP_DIR}")

#M3AP
##############
set(M3AP_DIR ${OPENAIR3_DIR}/M3AP)
add_library(m3ap
  ${M3AP_DIR}/m3ap_common.c
  ${M3AP_DIR}/m3ap_decoder.c
  ${M3AP_DIR}/m3ap_encoder.c
  ${M3AP_DIR}/m3ap_MCE_handler.c
  ${M3AP_DIR}/m3ap_MME_handler.c
  ${M3AP_DIR}/m3ap_MME.c
  ${M3AP_DIR}/m3ap_MME_management_procedures.c
  ${M3AP_DIR}/m3ap_MME_interface_management.c
  ${M3AP_DIR}/m3ap_MCE.c
  ${M3AP_DIR}/m3ap_MCE_management_procedures.c
  ${M3AP_DIR}/m3ap_MCE_interface_management.c
  ${M3AP_DIR}/m3ap_itti_messaging.c
  ${M3AP_DIR}/m3ap_ids.c
  ${M3AP_DIR}/m3ap_timers.c
)
target_link_libraries(m3ap PUBLIC asn1_m3ap)
target_link_libraries(m3ap PRIVATE m2ap)
target_link_libraries(m3ap PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)
target_include_directories(m3ap PUBLIC "${M3AP_DIR}")

#X2AP
##############
set(X2AP_DIR ${OPENAIR2_DIR}/X2AP)
include_directories ("${X2AP_DIR}")
add_library(x2ap
  ${X2AP_DIR}/x2ap_common.c
  ${X2AP_DIR}/x2ap_eNB.c
  ${X2AP_DIR}/x2ap_eNB_decoder.c
  ${X2AP_DIR}/x2ap_eNB_encoder.c
  ${X2AP_DIR}/x2ap_eNB_handler.c
  ${X2AP_DIR}/x2ap_eNB_itti_messaging.c
  ${X2AP_DIR}/x2ap_eNB_management_procedures.c
  ${X2AP_DIR}/x2ap_eNB_generate_messages.c
  ${X2AP_DIR}/x2ap_ids.c
  ${X2AP_DIR}/x2ap_timers.c
 )
target_link_libraries(x2ap PUBLIC asn1_x2ap)
target_link_libraries(x2ap PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

# F1AP
##############
set(F1AP_DIR ${OPENAIR2_DIR}/F1AP)
add_library(f1ap
            ${F1AP_DIR}/f1ap_common.c
            ${F1AP_DIR}/f1ap_ids.c
            ${F1AP_DIR}/f1ap_cu_interface_management.c
            ${F1AP_DIR}/f1ap_cu_paging.c
            ${F1AP_DIR}/f1ap_cu_rrc_message_transfer.c
            ${F1AP_DIR}/f1ap_cu_task.c
            ${F1AP_DIR}/f1ap_cu_ue_context_management.c
            ${F1AP_DIR}/f1ap_cu_warning_message_transmission.c
            ${F1AP_DIR}/f1ap_du_interface_management.c
            ${F1AP_DIR}/f1ap_du_paging.c
            ${F1AP_DIR}/f1ap_du_rrc_message_transfer.c
            ${F1AP_DIR}/f1ap_du_system_information.c
            ${F1AP_DIR}/f1ap_du_task.c
            ${F1AP_DIR}/f1ap_du_ue_context_management.c
            ${F1AP_DIR}/f1ap_du_warning_message_transmission.c
            ${F1AP_DIR}/f1ap_encoder.c
            ${F1AP_DIR}/f1ap_handlers.c
            ${F1AP_DIR}/f1ap_itti_messaging.c)
target_include_directories(f1ap PUBLIC F1AP_DIR)
target_link_libraries(f1ap PUBLIC asn1_f1ap)
target_link_libraries(f1ap PRIVATE ngap nr_rrc HASHTABLE)

# LPP
##############
# target asn1_lpp in openair3/LPP/MESSAGES/CMakeLists.txt used below

# Hardware dependant options
###################################
add_list1_option(NB_ANTENNAS_RX "4" "Number of antennas in reception" "1" "2" "4")
add_list1_option(NB_ANTENNAS_TX "4" "Number of antennas in transmission" "1" "2" "4")

set (CONFIG_ROOTDIR ${OPENAIR_DIR}/common/config)
add_library(config_internals OBJECT
  ${CONFIG_ROOTDIR}/config_common.c
)

add_library(CONFIG_LIB
  ${CONFIG_ROOTDIR}/config_load_configmodule.c
  ${CONFIG_ROOTDIR}/config_userapi.c
  ${CONFIG_ROOTDIR}/config_cmdline.c
)
target_link_libraries(CONFIG_LIB PRIVATE config_internals dl UTIL)

# CONFIG_LIB is a wrapper around a particular config implementation, default is
# libconfig in params_libconfig below
pkg_check_modules(libconfig REQUIRED libconfig)
add_library(params_libconfig MODULE ${CONFIG_ROOTDIR}/libconfig/config_libconfig.c)
target_include_directories(params_libconfig PRIVATE ${libconfig_INCLUDE_DIRS})
target_link_libraries(params_libconfig PRIVATE config_internals ${libconfig_LIBRARIES})

# shared library loader
add_library(shlib_loader OBJECT common/utils/load_module_shlib.c)
target_link_libraries(shlib_loader PRIVATE CONFIG_LIB)

##########################################################

# LDPC offload library - AMD T2 Accelerator Card
##########################################################

add_boolean_option(ENABLE_LDPC_T2 OFF "Build support for LDPC Offload to T2 library" OFF)
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib/x86_64-linux-gnu/pkgconfig")
if (ENABLE_LDPC_T2)
  pkg_check_modules(LIBDPDK_T2 REQUIRED libdpdk=20.11.9)
  find_library(PMD_T2 NAMES rte_baseband_accl_ldpc HINTS ${LIBDPDK_T2_LIBRARY_DIRS})
  if (NOT PMD_T2)
    message(FATAL_ERROR "could not find poll-mode driver for AccelerComm T2 LDPC Offload (rte_baseband_accl_ldpc.so)")
  endif()
  message(STATUS "T2 build: use ${PMD_T2}")
  add_library(ldpc_t2 MODULE ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder_offload.c)
  set_target_properties(ldpc_t2 PROPERTIES COMPILE_FLAGS "-DALLOW_EXPERIMENTAL_API")
  target_link_libraries(ldpc_t2 ${LIBDPDK_T2_LDFLAGS} ${PMD_T2})
endif()

##########################################################

include_directories ("${OPENAIR_DIR}/radio/COMMON")

##############################################################
#    ???!!! TO BE DOCUMENTED OPTIONS !!!???
##############################################################

add_boolean_option(OAI_NW_DRIVER_TYPE_ETHERNET False "????" ON)
add_boolean_option(OAI_NW_DRIVER_USE_NETLINK True "????" ON)

add_boolean_option(UE_EXPANSION             False         "enable UE_EXPANSION with max 256 UE" ON)
add_boolean_option(PHY_TX_THREAD            False         "enable UE_EXPANSION with max 256 UE" ON)
add_boolean_option(PRE_SCD_THREAD           False         "enable UE_EXPANSION with max 256 UE" ON)
add_boolean_option(UESIM_EXPANSION          False         "enable UESIM_EXPANSION with max 256 UE" ON)

##########################
# SCHEDULING/REAL-TIME/PERF options
##########################
add_boolean_option(ENABLE_USE_CPU_EXECUTION_TIME False "Add data in vcd traces: disable it if perf issues" ON)
add_boolean_option(ENABLE_VCD              False  "always true now, time measurements of proc calls and var displays" ON)
add_boolean_option(ENABLE_VCD_FIFO         False  "time measurements of proc calls and var displays sent to FIFO (one more thread)" ON)

##########################
# PHY options
##########################
add_boolean_option(LOCALIZATION            False "???" ON)
add_integer_option(MAX_NUM_CCs             1     "????" ON)
add_boolean_option(SMBV                    False "Rohde&Schwarz SMBV100A vector signal generator" ON)
add_boolean_option(DEBUG_PHY               False "Enable PHY layer debugging options" ON)
add_boolean_option(DEBUG_PHY_PROC          False "Enable debugging of PHY layer procedures" ON)
add_boolean_option(MEX                     False "Enabling compilation with mex" ON)

##########################
# NAS LAYER OPTIONS
##########################
add_boolean_option(ENABLE_NAS_UE_LOGGING   True  "????" ON)
add_boolean_option(NAS_BUILT_IN_UE         True  "UE NAS layer present in this executable" ON)
add_boolean_option(NAS_UE                  True  "NAS UE INSTANCE (<> NAS_MME)" ON)


##########################
# RRC LAYER OPTIONS
##########################
add_boolean_option(RRC_DEFAULT_RAB_IS_AM       True   "set the LTE RLC mode to AM for the default bearer, otherwise it is UM." ON)

# add the binary tree to the search path for include files
#######################################################
# We will find ConfigOAI.h after generation in target directory
include_directories("${OPENAIR_BIN_DIR}")
# add directories to find all include files
# the internal rule is to use generic names such as defs.h
# but to make it uniq name as adding the relative path in the include directtive
# example: #include "RRC/LTE/rrc_defs.h"
#find_path (include_dirs_all *.h ${OPENAIR_DIR})
#find_path (include_dirs_all *.h PATHS /usr/include NO_CMAKE_PATH)
#include_directories("${include_dirs_all}")

# Legacy exact order

include_directories("${OPENAIR_DIR}/executables")
include_directories("${OPENAIR2_DIR}/COMMON")
include_directories("${OPENAIR2_DIR}/UTIL")
include_directories("${OPENAIR2_DIR}/UTIL/LOG")
include_directories("${OPENAIR3_DIR}/COMMON")
include_directories("${OPENAIR3_DIR}/UTILS")
include_directories("${NFAPI_DIR}/nfapi/public_inc")
include_directories("${NFAPI_DIR}/common/public_inc")
include_directories("${NFAPI_DIR}/pnf/public_inc")
include_directories("${NFAPI_DIR}/nfapi/inc")
include_directories("${NFAPI_DIR}/sim_common/inc")
include_directories("${NFAPI_DIR}/pnf_sim/inc")
include_directories("${OPENAIR1_DIR}")
include_directories("${OPENAIR2_DIR}")
include_directories("${OPENAIR3_DIR}/NAS/TOOLS")
include_directories("${OPENAIR2_DIR}/ENB_APP")
include_directories("${OPENAIR2_DIR}/GNB_APP")
include_directories("${OPENAIR2_DIR}/MCE_APP")
include_directories("${OPENAIR2_DIR}/LAYER2/RLC")
include_directories("${OPENAIR2_DIR}/LAYER2/PDCP_v10.1.0")
include_directories("${OPENAIR2_DIR}/RRC/LTE/MESSAGES")
include_directories("${OPENAIR2_DIR}/RRC/LTE")
include_directories("${OPENAIR_DIR}/common/utils")
include_directories("${OPENAIR_DIR}/common/utils/collection")
include_directories("${OPENAIR_DIR}/common/utils/ocp_itti")
include_directories("${OPENAIR3_DIR}/NAS/COMMON")
include_directories("${OPENAIR3_DIR}/NAS/COMMON/API/NETWORK")
include_directories("${OPENAIR3_DIR}/NAS/COMMON/EMM/MSG")
include_directories("${OPENAIR3_DIR}/NAS/COMMON/ESM/MSG")
include_directories("${OPENAIR3_DIR}/NAS/UE/ESM")
include_directories("${OPENAIR3_DIR}/NAS/UE/EMM")
include_directories("${OPENAIR3_DIR}/NAS/UE/API/USER")
include_directories("${OPENAIR3_DIR}/NAS/COMMON/IES")
include_directories("${OPENAIR3_DIR}/NAS/COMMON/UTIL")
include_directories("${OPENAIR3_DIR}/SECU")
include_directories("${OPENAIR3_DIR}/SCTP")
include_directories("${OPENAIR3_DIR}/S1AP")
include_directories("${OPENAIR2_DIR}/X2AP")
include_directories("${OPENAIR2_DIR}/M2AP")
include_directories("${OPENAIR2_DIR}/F1AP")
include_directories("${OPENAIR3_DIR}/ocp-gtpu")
include_directories("${OPENAIR3_DIR}/M3AP")
include_directories("${OPENAIR3_DIR}/MME_APP")
include_directories("${OPENAIR_DIR}/radio/COMMON")
include_directories("${OPENAIR2_DIR}/UTIL/OSA")
include_directories("${OPENAIR2_DIR}/UTIL/MEM")
include_directories("${OPENAIR2_DIR}/UTIL/LISTS")
include_directories("${OPENAIR2_DIR}/UTIL/FIFO")
include_directories("${OPENAIR2_DIR}/UTIL/MATH")
include_directories("${OPENAIR2_DIR}/UTIL/TIMER")
include_directories("${OPENAIR2_DIR}/UTIL/OTG")
include_directories("${OPENAIR2_DIR}/UTIL/OPT")
include_directories("${OPENAIR_DIR}")

###########
# Utilities
###############

# include T directory even if the T is off because T macros are in the code
# no matter what. Note: sone files are generated, so we have both source and
# binary directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common/utils/T
                    ${CMAKE_CURRENT_BINARY_DIR}/common/utils/T)

# set the T_LIB name to T if the target exists, otherwise leave empty
set(T_LIB $<TARGET_NAME_IF_EXISTS:T>)

add_library(HASHTABLE
  ${OPENAIR_DIR}/common/utils/hashtable/hashtable.c
  ${OPENAIR_DIR}/common/utils/hashtable/obj_hashtable.c
)
include_directories(${OPENAIR_DIR}/common/utils/hashtable)

add_library(UTIL
  ${OPENAIR_DIR}/common/utils/LOG/log.c
  ${OPENAIR_DIR}/common/utils/LOG/vcd_signal_dumper.c
  ${OPENAIR2_DIR}/UTIL/MATH/oml.c
  ${OPENAIR2_DIR}/UTIL/OPT/probe.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/time_meas.c
  ${OPENAIR_DIR}/common/utils/time_stat.c
)
target_link_libraries(UTIL PUBLIC ${T_LIB} pthread)

set(SECURITY_SRC
  ${OPENAIR3_DIR}/SECU/secu_defs.c
  ${OPENAIR3_DIR}/SECU/kdf.c
  ${OPENAIR3_DIR}/SECU/aes_128_ctr.c
  ${OPENAIR3_DIR}/SECU/aes_128_cbc_cmac.c
  ${OPENAIR3_DIR}/SECU/aes_128_ecb.c
  ${OPENAIR3_DIR}/SECU/sha_256_hmac.c
  ${OPENAIR3_DIR}/SECU/rijndael.c
  ${OPENAIR3_DIR}/SECU/snow3g.c
  ${OPENAIR3_DIR}/SECU/key_nas_deriver.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eea0.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eea1.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eia1.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eea2.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eia2.c
  )

pkg_check_modules(OpenSSL openssl REQUIRED)
add_library(SECURITY ${SECURITY_SRC} $<TARGET_OBJECTS:ds>)
target_include_directories(SECURITY PRIVATE ${OPENAIR_DIR}/common/utils/ds/)
target_link_libraries(SECURITY PRIVATE ${OpenSSL_LIBRARIES})

# Physical Channel Procedures Scheduling
################################"
set(SCHED_SRC
  ${OPENAIR1_DIR}/SCHED/fapi_l1.c
  ${OPENAIR1_DIR}/SCHED/phy_procedures_lte_eNb.c
  ${OPENAIR1_DIR}/SCHED/phy_procedures_lte_common.c
)
add_library(SCHED_LIB ${SCHED_SRC})
target_link_libraries(SCHED_LIB PRIVATE asn1_lte_rrc_hdrs)

set(SCHED_NR_SRC
  ${OPENAIR1_DIR}/SCHED_NR/fapi_nr_l1.c
  ${OPENAIR1_DIR}/SCHED_NR/phy_procedures_nr_gNB.c
  ${OPENAIR1_DIR}/SCHED_NR/nr_prach_procedures.c
  ${OPENAIR1_DIR}/SCHED_NR/phy_frame_config_nr.c
)
add_library(SCHED_NR_LIB ${SCHED_NR_SRC})
target_link_libraries(SCHED_NR_LIB PRIVATE asn1_nr_rrc_hdrs)

set(SCHED_SRC_RU
  ${OPENAIR1_DIR}/SCHED/ru_procedures.c
  ${OPENAIR1_DIR}/SCHED_NR/nr_ru_procedures.c
  ${OPENAIR1_DIR}/SCHED/prach_procedures.c
)
add_library(SCHED_RU_LIB ${SCHED_SRC_RU})
target_link_libraries(SCHED_RU_LIB PRIVATE asn1_lte_rrc_hdrs)

set(SCHED_SRC_UE
  ${OPENAIR1_DIR}/SCHED_UE/phy_procedures_lte_ue.c
  ${OPENAIR1_DIR}/SCHED/phy_procedures_lte_common.c
  ${OPENAIR1_DIR}/SCHED_UE/pucch_pc.c
  ${OPENAIR1_DIR}/SCHED_UE/pusch_pc.c
  ${OPENAIR1_DIR}/SCHED_UE/srs_pc.c
)
add_library(SCHED_UE_LIB ${SCHED_SRC_UE})
target_link_libraries(SCHED_UE_LIB PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

set(SCHED_SRC_NR_UE
  ${OPENAIR1_DIR}/SCHED_NR_UE/phy_procedures_nr_ue.c
  ${OPENAIR1_DIR}/SCHED_NR_UE/fapi_nr_ue_l1.c
  ${OPENAIR1_DIR}/SCHED_NR_UE/phy_frame_config_nr_ue.c
  ${OPENAIR1_DIR}/SCHED_NR_UE/harq_nr.c
  ${OPENAIR1_DIR}/SCHED_NR_UE/pucch_uci_ue_nr.c
)
add_library(SCHED_NR_UE_LIB ${SCHED_SRC_NR_UE})
target_link_libraries(SCHED_NR_UE_LIB PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)


# nFAPI
#################################
set(NFAPI_COMMON_SRC
  ${NFAPI_DIR}/common/src/debug.c
  )
add_library(NFAPI_COMMON_LIB ${NFAPI_COMMON_SRC})

include_directories(${NFAPI_DIR}/common/public_inc)

set(NFAPI_SRC
  ${NFAPI_DIR}/nfapi/src/nfapi.c
  ${NFAPI_DIR}/nfapi/src/nfapi_p4.c
  ${NFAPI_DIR}/nfapi/src/nfapi_p5.c
  ${NFAPI_DIR}/nfapi/src/nfapi_p7.c
)
add_library(NFAPI_LIB ${NFAPI_SRC})

include_directories(${NFAPI_DIR}/nfapi/public_inc)
include_directories(${NFAPI_DIR}/nfapi/inc)

set(NFAPI_PNF_SRC
  ${NFAPI_DIR}/pnf/src/pnf.c
  ${NFAPI_DIR}/pnf/src/pnf_interface.c
  ${NFAPI_DIR}/pnf/src/pnf_p7.c
  ${NFAPI_DIR}/pnf/src/pnf_p7_interface.c
)
add_library(NFAPI_PNF_LIB ${NFAPI_PNF_SRC})
target_link_libraries(NFAPI_PNF_LIB PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

include_directories(${NFAPI_DIR}/pnf/public_inc)
include_directories(${NFAPI_DIR}/pnf/inc)

set(NFAPI_VNF_SRC
  ${NFAPI_DIR}/vnf/src/vnf.c
  ${NFAPI_DIR}/vnf/src/vnf_interface.c
  ${NFAPI_DIR}/vnf/src/vnf_p7.c
  ${NFAPI_DIR}/vnf/src/vnf_p7_interface.c
)
add_library(NFAPI_VNF_LIB ${NFAPI_VNF_SRC})
target_link_libraries(NFAPI_VNF_LIB PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

include_directories(${NFAPI_DIR}/vnf/public_inc)
include_directories(${NFAPI_DIR}/vnf/inc)

# nFAPI user defined code
#############################
set(NFAPI_USER_SRC
  ${NFAPI_USER_DIR}/nfapi.c
  ${NFAPI_USER_DIR}/nfapi_pnf.c
  ${NFAPI_USER_DIR}/nfapi_vnf.c
  ${NFAPI_USER_DIR}/gnb_ind_vars.c
)
add_library(NFAPI_USER_LIB ${NFAPI_USER_SRC})
target_link_libraries(NFAPI_USER_LIB PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)
include_directories(${NFAPI_USER_DIR})

# Layer 1
#############################
set(PHY_TURBOSRC
  ${OPENAIR1_DIR}/PHY/CODING/3gpplte_sse.c
  ${OPENAIR1_DIR}/PHY/CODING/3gpplte.c
  ${OPENAIR1_DIR}/PHY/CODING/3gpplte_turbo_decoder_sse_8bit.c
  ${OPENAIR1_DIR}/PHY/CODING/3gpplte_turbo_decoder_sse_16bit.c
  ${OPENAIR1_DIR}/PHY/CODING/3gpplte_turbo_decoder_avx2_16bit.c
  ${OPENAIR1_DIR}/PHY/CODING/3gpplte_turbo_decoder.c
)
set(PHY_POLARSRC
  ${OPENAIR1_DIR}/PHY/CODING/nr_polar_init.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_bitwise_operations.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_crc_byte.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_crc.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_decoder.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_decoding_tools.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_encoder.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_interleaving_pattern.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_kernal_operation.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_kronecker_power_matrices.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_matrix_and_array.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_procedures.c
  ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_sequence_pattern.c
)
set(PHY_SMALLBLOCKSRC
  ${OPENAIR1_DIR}/PHY/CODING/nrSmallBlock/encodeSmallBlock.c
  ${OPENAIR1_DIR}/PHY/CODING/nrSmallBlock/decodeSmallBlock.c
)
set(PHY_TURBOIF
  ${OPENAIR1_DIR}/PHY/CODING/coding_load.c
)

set(PHY_LDPC_ORIG_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.c
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder.c
)
add_library(ldpc_orig MODULE ${PHY_LDPC_ORIG_SRC} )
target_link_libraries(ldpc_orig PRIVATE ldpc_gen_HEADERS)

set(PHY_LDPC_OPTIM_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.c
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim.c
  )
add_library(ldpc_optim MODULE ${PHY_LDPC_OPTIM_SRC} )
target_link_libraries(ldpc_optim PRIVATE ldpc_gen_HEADERS)

set(PHY_LDPC_OPTIM8SEG_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.c
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim8seg.c
)
add_library(ldpc_optim8seg MODULE ${PHY_LDPC_OPTIM8SEG_SRC} )
target_link_libraries(ldpc_optim8seg PRIVATE ldpc_gen_HEADERS)

set(PHY_LDPC_OPTIM8SEGMULTI_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.c
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim8segmulti.c
  )
add_library(ldpc MODULE ${PHY_LDPC_OPTIM8SEGMULTI_SRC} )
target_link_libraries(ldpc PRIVATE ldpc_gen_HEADERS)

set(PHY_LDPC_CUDA_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder_LYC/nrLDPC_decoder_LYC.cu
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim8segmulti.c
)

set(PHY_LDPC_CL_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder_CL.c
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim8segmulti.c
)
add_custom_target( nrLDPC_decoder_kernels_CL
                   COMMAND gcc ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder_CL.c -dD -DNRLDPC_KERNEL_SOURCE -E -o ${CMAKE_CURRENT_BINARY_DIR}/nrLDPC_decoder_kernels_CL.clc
                   SOURCES ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder_CL.c
)
add_library(ldpc_cl MODULE ${PHY_LDPC_CL_SRC} )
target_link_libraries(ldpc_cl OpenCL)
add_dependencies(ldpc_cl nrLDPC_decoder_kernels_CL)

set(PHY_NR_CODINGIF
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_load.c
)

##############################################
# Base CUDA setting
##############################################

add_boolean_option(ENABLE_LDPC_CUDA OFF "Build support for CUDA" OFF)
if (ENABLE_LDPC_CUDA)
  find_package(CUDA REQUIRED)
  SET(CUDA_NVCC_FLAG "${CUDA_NVCC_FLAGS};-arch=sm_60;")
  SET(CUDA_VERBOSE_BUILD ON)
  cuda_add_library(ldpc_cuda MODULE ${PHY_LDPC_CUDA_SRC})
  set_target_properties(ldpc_cuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
  if (NOT CUDA_FOUND)
    message(FATAL_ERROR "no CUDA found")
  endif()
endif()

add_library(coding MODULE ${PHY_TURBOSRC} )

add_library(dfts MODULE ${OPENAIR1_DIR}/PHY/TOOLS/oai_dfts.c ${OPENAIR1_DIR}/PHY/TOOLS/oai_dfts_neon.c)


set(PHY_SRC_COMMON
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dci_tools_common.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/lte_mcs.c
#  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/slss.c
#  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/sldch.c
#  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/slsch.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/get_pmi.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/group_hopping.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/phich_common.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pcfich_common.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pmch_common.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/power_control.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/prach_common.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pucch_common.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dlsch_scrambling.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/srs_modulation.c
  ${OPENAIR1_DIR}/PHY/MODULATION/ofdm_mod.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_sync_time.c
  ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_dl_cell_spec.c
  ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_dl_uespec.c
  ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_gold.c
  ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_gold_mbsfn.c
  ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_dl_mbsfn.c
  ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_ul_ref.c
  ${OPENAIR1_DIR}/PHY/CODING/lte_segmentation.c
  ${OPENAIR1_DIR}/PHY/CODING/nr_segmentation.c
  ${OPENAIR1_DIR}/PHY/CODING/nr_rate_matching.c
  ${OPENAIR1_DIR}/PHY/CODING/ccoding_byte.c
  ${OPENAIR1_DIR}/PHY/CODING/ccoding_byte_lte.c
  ${OPENAIR1_DIR}/PHY/CODING/3gpplte_sse.c
  ${OPENAIR1_DIR}/PHY/CODING/crc_byte.c
  ${PHY_TURBOIF}
  ${OPENAIR1_DIR}/PHY/CODING/lte_rate_matching.c
  ${OPENAIR1_DIR}/PHY/CODING/viterbi.c
  ${OPENAIR1_DIR}/PHY/CODING/viterbi_lte.c
  ${OPENAIR1_DIR}/PHY/INIT/init_top.c
  ${OPENAIR1_DIR}/PHY/INIT/lte_parms.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cadd_vv.c
  ${OPENAIR1_DIR}/PHY/TOOLS/dfts_load.c
  ${OPENAIR1_DIR}/PHY/TOOLS/log2_approx.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cmult_sv.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cmult_vv.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cdot_prod.c
  ${OPENAIR1_DIR}/PHY/TOOLS/signal_energy.c
  ${OPENAIR1_DIR}/PHY/TOOLS/dB_routines.c
  ${OPENAIR1_DIR}/PHY/TOOLS/sqrt.c
  ${OPENAIR1_DIR}/PHY/TOOLS/lut.c
  ${OPENAIR1_DIR}/PHY/TOOLS/simde_operations.c
  )

set(PHY_SRC
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pss.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/sss.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/sss_gen.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pilots.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pilots_mbsfn.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dlsch_coding.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dlsch_modulation.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dci_tools.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pbch.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dci.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/edci.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/phich.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pcfich.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pucch.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pmch.c
#  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/ulsch_demodulation.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/ulsch_decoding.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/rar_tools.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/uci_tools.c
#  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/freq_equalization.c
#  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_adjust_sync_eNB.c
#  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_ul_channel_estimation.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_eNB_measurements.c
  ${OPENAIR1_DIR}/PHY/INIT/lte_init.c
  )

set(PHY_SRC_RU
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/if4_tools.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/drs_modulation.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/ulsch_demodulation.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_ul_channel_estimation.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_adjust_sync_eNB.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/freq_equalization.c
  ${OPENAIR1_DIR}/PHY/MODULATION/slot_fep_ul.c
  ${OPENAIR1_DIR}/PHY/MODULATION/slot_fep_nr.c
  ${OPENAIR1_DIR}/PHY/MODULATION/ul_7_5_kHz.c
  ${OPENAIR1_DIR}/PHY/MODULATION/gen_75KHz.cpp
  ${OPENAIR1_DIR}/PHY/MODULATION/beamforming.c
  ${OPENAIR1_DIR}/PHY/MODULATION/compute_bf_weights.c
  ${OPENAIR1_DIR}/PHY/INIT/lte_init_ru.c
  ${OPENAIR1_DIR}/PHY/INIT/nr_init_ru.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/prach.c

  )

set(PHY_SRC_UE
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/sss_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/sss_gen.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dlsch_demodulation.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dlsch_llr_computation.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/linear_preprocessing_rec.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dlsch_decoding.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dci_tools_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/uci_tools_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/pbch_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dci_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/phich_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/pcfich_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/pucch_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/prach_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/pmch_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/pch_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/slss.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/sldch.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/slsch.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/drs_modulation.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/ulsch_modulation.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/ulsch_coding.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/rar_tools_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/initial_sync.c
  ${OPENAIR1_DIR}/PHY/MODULATION/slot_fep.c
  ${OPENAIR1_DIR}/PHY/MODULATION/slot_fep_mbsfn.c
  ${OPENAIR1_DIR}/PHY/MODULATION/ul_7_5_kHz_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_sync_time.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_sync_timefreq.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_adjust_sync_ue.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_dl_channel_estimation.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_dl_bf_channel_estimation.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_dl_mbsfn_channel_estimation.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_est_freq_offset.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_ue_measurements.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/adjust_gain.c
  ${OPENAIR1_DIR}/PHY/INIT/lte_init_ue.c
  )

  set(PHY_NR_SRC_COMMON
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_prach_common.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_csi_rs.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_scrambling.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/scrambling_luts.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/srs_modulation_nr.c
  )

  set(PHY_NR_SRC
  ${OPENAIR1_DIR}/PHY/INIT/nr_init.c
  ${OPENAIR1_DIR}/PHY/INIT/nr_parms.c
  ${OPENAIR1_DIR}/PHY/MODULATION/nr_modulation.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_prs.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_pss.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_sss.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_pbch.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_dci.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_dci_tools.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_dlsch.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_dlsch_tools.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_dlsch_coding.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_ulsch_decoding.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_ulsch.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_tbs_tools.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_sch_dmrs.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_prach.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_ulsch_llr_computation.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_ulsch_demodulation.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/ul_ref_seq_nr.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_dmrs_rx.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_gold.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_gen_mod_table.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/dmrs_nr.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/ptrs_nr.c
  ${OPENAIR1_DIR}/PHY/NR_ESTIMATION/nr_ul_channel_estimation.c
  ${OPENAIR1_DIR}/PHY/NR_ESTIMATION/nr_freq_equalization.c
  ${OPENAIR1_DIR}/PHY/NR_ESTIMATION/nr_measurements_gNB.c
  ${OPENAIR1_DIR}/PHY/TOOLS/file_output.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cadd_vv.c
  #${OPENAIR1_DIR}/PHY/TOOLS/lte_dfts.c
  ${OPENAIR1_DIR}/PHY/TOOLS/log2_approx.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cmult_sv.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cmult_vv.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cdot_prod.c
  ${OPENAIR1_DIR}/PHY/TOOLS/signal_energy.c
  ${OPENAIR1_DIR}/PHY/TOOLS/dB_routines.c
  ${OPENAIR1_DIR}/PHY/TOOLS/sqrt.c
  ${OPENAIR1_DIR}/PHY/TOOLS/lut.c
  ${OPENAIR1_DIR}/PHY/TOOLS/simde_operations.c
  ${PHY_POLARSRC}
  ${PHY_SMALLBLOCKSRC}
  ${PHY_NR_CODINGIF}
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/pucch_rx.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/srs_rx.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_uci_tools_common.c
  )
  set(PHY_NR_UE_SRC
  ${OPENAIR1_DIR}/PHY/INIT/nr_parms.c
  ${OPENAIR1_DIR}/PHY/MODULATION/nr_modulation.c
  ${OPENAIR1_DIR}/PHY/MODULATION/slot_fep_nr.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/pss_nr.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/sss_nr.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/cic_filter_nr.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/nr_initial_sync.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/nr_ue_rf_helpers.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/nr_pbch.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/nr_dlsch_demodulation.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/nr_ulsch_coding.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/nr_dlsch_decoding.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/nr_dlsch_llr_computation.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_tbs_tools.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_prach_common.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_sch_dmrs.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/nr_prach.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/dci_nr.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/pucch_nr.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/csi_rx.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_uci_tools_common.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/nr_ulsch_ue.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/ul_ref_seq_nr.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_dmrs_rx.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/dmrs_nr.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/ptrs_nr.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_gold_ue.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_gen_mod_table.c
  ${OPENAIR1_DIR}/PHY/NR_UE_ESTIMATION/nr_dl_channel_estimation.c
  ${OPENAIR1_DIR}/PHY/NR_UE_ESTIMATION/nr_adjust_synch_ue.c
  ${OPENAIR1_DIR}/PHY/NR_UE_ESTIMATION/nr_ue_measurements.c
  ${OPENAIR1_DIR}/PHY/NR_UE_ESTIMATION/nr_adjust_gain.c
  ${OPENAIR1_DIR}/PHY/TOOLS/file_output.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cadd_vv.c
 # ${OPENAIR1_DIR}/PHY/TOOLS/lte_dfts.c
  ${OPENAIR1_DIR}/PHY/TOOLS/log2_approx.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cmult_sv.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cmult_vv.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cdot_prod.c
  ${OPENAIR1_DIR}/PHY/TOOLS/signal_energy.c
  ${OPENAIR1_DIR}/PHY/TOOLS/dB_routines.c
  ${OPENAIR1_DIR}/PHY/TOOLS/sqrt.c
  ${OPENAIR1_DIR}/PHY/TOOLS/lut.c
  ${OPENAIR1_DIR}/PHY/TOOLS/simde_operations.c
  ${OPENAIR1_DIR}/PHY/INIT/nr_init_ue.c
  #  ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/pucch_uci_test.c
  ${PHY_POLARSRC}
  ${PHY_SMALLBLOCKSRC}
  ${PHY_NR_CODINGIF}
  )


if (${SMBV})
  set(PHY_SRC "${PHY_SRC} ${OPENAIR1_DIR}/PHY/TOOLS/smbv.c")
endif  (${SMBV})

set(PHY_SRC_UE ${PHY_SRC_UE} ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dlsch_llr_computation_avx2.c)

add_library(PHY_COMMON ${PHY_SRC_COMMON})
target_link_libraries(PHY_COMMON PRIVATE shlib_loader)
add_dependencies(PHY_COMMON dfts)
target_link_libraries(PHY_COMMON PRIVATE asn1_lte_rrc_hdrs)

add_library(PHY ${PHY_SRC})
target_link_libraries(PHY PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

pkg_check_modules(blas REQUIRED blas)
pkg_check_modules(lapacke REQUIRED lapacke)

add_library(PHY_UE ${PHY_SRC_UE})
target_link_libraries(PHY_UE PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)
# RHEL needs also cblas, but Ubuntu does not have it. So `cblas_LIBRARIES` will
# be empty for Ubuntu, a no-op
pkg_check_modules(cblas cblas)
target_link_libraries(PHY_UE PRIVATE ${blas_LIBRARIES} ${cblas_LIBRARIES} ${lapacke_LIBRARIES})
target_include_directories(PHY_UE PRIVATE ${blas_INCLUDE_DIRS} ${lapacke_INCLUDE_DIRS})

add_library(PHY_NR_COMMON ${PHY_NR_SRC_COMMON})

add_library(PHY_NR ${PHY_NR_SRC})

add_library(PHY_NR_UE ${PHY_NR_UE_SRC})
target_link_libraries(PHY_NR_UE PRIVATE asn1_nr_rrc_hdrs)

add_library(PHY_RU ${PHY_SRC_RU})
target_link_libraries(PHY_RU PRIVATE asn1_lte_rrc_hdrs)

#Library for mex functions
#########################3
set(PHY_MEX_UE
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/linear_preprocessing_rec.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dlsch_llr_computation.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dlsch_demodulation.c
  ${OPENAIR1_DIR}/PHY/TOOLS/log2_approx.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/lte_mcs.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/get_pmi.c
  ${OPENAIR1_DIR}/PHY/TOOLS/dB_routines.c
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pmch_common.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cadd_vv.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cmult_sv.c
  ${OPENAIR1_DIR}/PHY/TOOLS/cmult_vv.c
  ${OPENAIR1_DIR}/PHY/TOOLS/signal_energy.c
  ${OPENAIR1_DIR}/PHY/TOOLS/simde_operations.c
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dlsch_llr_computation_avx2.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_ue_measurements.c
  ${OPENAIR_DIR}/common/utils/LOG/log.c
  )
add_library(PHY_MEX ${PHY_MEX_UE})
target_link_libraries(PHY_MEX PRIVATE asn1_lte_rrc_hdrs)

#Layer 2 library
#####################
set(MAC_DIR ${OPENAIR2_DIR}/LAYER2/MAC)
set(NR_GNB_MAC_DIR ${OPENAIR2_DIR}/LAYER2/NR_MAC_gNB)
set(NR_UE_MAC_DIR ${OPENAIR2_DIR}/LAYER2/NR_MAC_UE)
set(PHY_INTERFACE_DIR ${OPENAIR2_DIR}/PHY_INTERFACE)
set(NR_PHY_INTERFACE_DIR ${OPENAIR2_DIR}/NR_PHY_INTERFACE)
set(NR_UE_PHY_INTERFACE_DIR ${OPENAIR2_DIR}/NR_UE_PHY_INTERFACE)
set(RRC_DIR ${OPENAIR2_DIR}/RRC/LTE)
set(NR_RRC_DIR ${OPENAIR2_DIR}/RRC/NR)
set(NR_UE_RRC_DIR ${OPENAIR2_DIR}/RRC/NR_UE)
set(PDCP_DIR  ${OPENAIR2_DIR}/LAYER2/PDCP_v10.1.0)

set(RLC_V2
  ${OPENAIR2_DIR}/LAYER2/rlc_v2/rlc_oai_api.c
  ${OPENAIR2_DIR}/LAYER2/rlc_v2/asn1_utils.c
  ${OPENAIR2_DIR}/LAYER2/rlc_v2/rlc_ue_manager.c
  ${OPENAIR2_DIR}/LAYER2/rlc_v2/rlc_entity.c
  ${OPENAIR2_DIR}/LAYER2/rlc_v2/rlc_entity_am.c
  ${OPENAIR2_DIR}/LAYER2/rlc_v2/rlc_entity_um.c
  ${OPENAIR2_DIR}/LAYER2/rlc_v2/rlc_pdu.c
  ${OPENAIR2_DIR}/LAYER2/rlc_v2/rlc_sdu.c
  )

set(NR_PDCP_SRC
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_oai_api.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_ue_manager.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_entity.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_sdu.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_timer_thread.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_security_nea2.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_integrity_nia2.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_integrity_nia1.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/asn1_utils.c
  openair2/LAYER2/nr_pdcp/cucp_cuup_handler.c
  openair2/LAYER2/nr_pdcp/cuup_cucp_if.c
  openair2/LAYER2/nr_pdcp/cuup_cucp_direct.c
  openair2/LAYER2/nr_pdcp/cuup_cucp_e1ap.c
  )

set(NR_SDAP_SRC
  ${OPENAIR2_DIR}/SDAP/nr_sdap/nr_sdap.c
  ${OPENAIR2_DIR}/SDAP/nr_sdap/nr_sdap_entity.c
  )
  
set(L2_SRC
  ${PDCP_DIR}/pdcp.c
  ${PDCP_DIR}/pdcp_fifo.c
  ${PDCP_DIR}/pdcp_sequence_manager.c
  ${PDCP_DIR}/pdcp_primitives.c
  ${PDCP_DIR}/pdcp_util.c
  ${PDCP_DIR}/pdcp_security.c
  ${OPENAIR2_DIR}/LAYER2/openair2_proc.c
#  ${RRC_DIR}/rrc_UE.c
  ${RRC_DIR}/rrc_eNB.c
  ${RRC_DIR}/rrc_eNB_endc.c
  ${RRC_DIR}/rrc_eNB_S1AP.c
  ${RRC_DIR}/rrc_eNB_M2AP.c
  ${RRC_DIR}/rrc_eNB_UE_context.c
  ${NR_RRC_DIR}/rrc_gNB_UE_context.c
  ${RRC_DIR}/rrc_common.c
  ${RRC_DIR}/L2_interface.c
  ${RRC_DIR}/L2_interface_common.c
  ${RRC_DIR}/L2_interface_ue.c
  )

set(L2_RRC_SRC
  ${OPENAIR2_DIR}/LAYER2/openair2_proc.c
  #  ${RRC_DIR}/rrc_UE.c
  ${RRC_DIR}/rrc_eNB.c
  ${RRC_DIR}/rrc_eNB_endc.c
  ${RRC_DIR}/rrc_eNB_S1AP.c
  ${RRC_DIR}/rrc_eNB_M2AP.c
  ${RRC_DIR}/rrc_eNB_UE_context.c
  ${RRC_DIR}/rrc_common.c
  ${RRC_DIR}/L2_interface.c
  ${RRC_DIR}/L2_interface_common.c
  ${RRC_DIR}/L2_interface_ue.c
  )

set(L2_LTE_SRC
  ${RLC_V2}
  )

set(L2_NR_SRC
  ${NR_PDCP_SRC}
  ${NR_SDAP_SRC}
  ${NR_RRC_DIR}/rrc_gNB.c
  ${NR_RRC_DIR}/nr_rrc_common.c
  ${NR_RRC_DIR}/mac_rrc_dl_direct.c
  ${NR_RRC_DIR}/mac_rrc_dl_f1ap.c
  ${NR_RRC_DIR}/nr_rrc_config.c
  ${NR_RRC_DIR}/rrc_gNB_nsa.c
  ${NR_RRC_DIR}/rrc_gNB_internode.c
  ${NR_RRC_DIR}/rrc_gNB_reconfig.c
  ${NR_RRC_DIR}/rrc_gNB_UE_context.c
  ${NR_RRC_DIR}/rrc_gNB_NGAP.c
  ${NR_RRC_DIR}/rrc_gNB_radio_bearers.c
  ${NR_RRC_DIR}/rrc_gNB_cuup.c
  ${NR_RRC_DIR}/rrc_gNB_du.c
  )

set(L2_SRC_UE
  ${PDCP_DIR}/pdcp.c
  ${PDCP_DIR}/pdcp_fifo.c
  ${PDCP_DIR}/pdcp_sequence_manager.c
  ${PDCP_DIR}/pdcp_primitives.c
  ${PDCP_DIR}/pdcp_util.c
  ${PDCP_DIR}/pdcp_security.c
  ${RRC_DIR}/rrc_UE.c
  ${RRC_DIR}/rrc_common.c
  ${RRC_DIR}/L2_interface_common.c
  ${RRC_DIR}/L2_interface_ue.c
  )

set(L2_RRC_SRC_UE
  ${RRC_DIR}/rrc_UE.c
  ${RRC_DIR}/rrc_common.c
  ${RRC_DIR}/L2_interface_common.c
  ${RRC_DIR}/L2_interface_ue.c
  )

set(NR_L2_SRC_UE
  ${NR_PDCP_SRC}
  ${NR_SDAP_SRC}
  ${NR_UE_RRC_DIR}/L2_interface_ue.c
  ${NR_UE_RRC_DIR}/main_ue.c
  ${NR_RRC_DIR}/nr_rrc_config.c
  ${NR_UE_RRC_DIR}/rrc_UE.c
  ${NR_UE_RRC_DIR}/rrc_nsa.c
  ${NR_UE_RRC_DIR}/rrc_timers_and_constants.c
  ${NR_UE_RRC_DIR}/rrc_sl_preconfig.c
  )

set (MAC_SRC
  #${PHY_INTERFACE_DIR}/phy_stub_UE.c
  ${PHY_INTERFACE_DIR}/IF_Module.c
  ${MAC_DIR}/main.c
  #${MAC_DIR}/main_ue.c
  #${MAC_DIR}/ue_procedures.c
  #${MAC_DIR}/ra_procedures.c
  ${MAC_DIR}/l1_helpers.c
  ${MAC_DIR}/rar_tools.c
  #${MAC_DIR}/rar_tools_ue.c
  ${MAC_DIR}/eNB_scheduler.c
  ${MAC_DIR}/eNB_scheduler_dlsch.c
  ${MAC_DIR}/eNB_scheduler_ulsch.c
  ${MAC_DIR}/eNB_scheduler_mch.c
  ${MAC_DIR}/eNB_scheduler_bch.c
  ${MAC_DIR}/eNB_scheduler_primitives.c
  ${MAC_DIR}/eNB_scheduler_RA.c
  ${MAC_DIR}/eNB_scheduler_fairRR.c
  ${MAC_DIR}/eNB_scheduler_phytest.c
  ${MAC_DIR}/pre_processor.c
  ${MAC_DIR}/slicing/slicing.c
  ${MAC_DIR}/config.c
  ${MAC_DIR}/config_ue.c
 )

set (MAC_NR_SRC
  ${NR_PHY_INTERFACE_DIR}/NR_IF_Module.c
  ${NR_PHY_INTERFACE_DIR}/nr_sched_response.c
  ${NR_GNB_MAC_DIR}/main.c
  ${NR_GNB_MAC_DIR}/config.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_bch.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_dlsch.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_ulsch.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_primitives.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_phytest.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_uci.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_srs.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_RA.c
  ${NR_GNB_MAC_DIR}/mac_rrc_dl_handler.c
  ${NR_GNB_MAC_DIR}/mac_rrc_ul_direct.c
  ${NR_GNB_MAC_DIR}/mac_rrc_ul_f1ap.c
 )


set (MAC_SRC_UE
  ${PHY_INTERFACE_DIR}/phy_stub_UE.c
  ${PHY_INTERFACE_DIR}/queue_t.c
  ${MAC_DIR}/main_ue.c
  ${MAC_DIR}/ue_procedures.c
  ${MAC_DIR}/ra_procedures.c
  ${MAC_DIR}/l1_helpers.c
  ${MAC_DIR}/rar_tools_ue.c
  ${MAC_DIR}/config_ue.c
 )

set (MAC_NR_SRC_UE
  ${NR_UE_PHY_INTERFACE_DIR}/NR_IF_Module.c
  ${NR_UE_PHY_INTERFACE_DIR}/NR_Packet_Drop.c
  ${NR_UE_MAC_DIR}/config_ue.c
  ${NR_UE_MAC_DIR}/config_ue_sl.c
  ${NR_UE_MAC_DIR}/mac_vars.c
  ${NR_UE_MAC_DIR}/main_ue_nr.c
  ${NR_UE_MAC_DIR}/nr_ue_procedures.c
  ${NR_UE_MAC_DIR}/nr_ue_procedures_sl.c
  ${NR_UE_MAC_DIR}/nr_ue_scheduler.c
  ${NR_UE_MAC_DIR}/nr_ue_dci_configuration.c
  ${NR_UE_MAC_DIR}/nr_ra_procedures.c
  ${NR_UE_MAC_DIR}/nr_ue_power_procedures.c
)

set (ENB_APP_SRC
  ${OPENAIR2_DIR}/ENB_APP/enb_app.c
  ${OPENAIR2_DIR}/ENB_APP/enb_config.c
  ${OPENAIR2_DIR}/ENB_APP/enb_config_SL.c
  ${OPENAIR2_DIR}/ENB_APP/enb_config_eMTC.c
  ${OPENAIR2_DIR}/ENB_APP/RRC_config_tools.c
  )

set (GNB_APP_SRC
  ${OPENAIR2_DIR}/GNB_APP/gnb_app.c
  ${OPENAIR2_DIR}/E1AP/e1ap_setup.c
  ${OPENAIR2_DIR}/GNB_APP/gnb_config.c
  )

set (MCE_APP_SRC
  ${OPENAIR2_DIR}/MCE_APP/mce_app.c
  ${OPENAIR2_DIR}/MCE_APP/mce_config.c
  )
set (MISC_NFAPI_LTE
 ${OPENAIR1_DIR}/SCHED/nfapi_lte_dummy.c
 )

add_library(MISC_NFAPI_LTE_LIB ${MISC_NFAPI_LTE})

set (MISC_NFAPI_NR
 ${OPENAIR1_DIR}/SCHED/nfapi_nr_dummy.c
 )

add_library(MISC_NFAPI_NR_LIB ${MISC_NFAPI_NR})

add_library(L2
  ${L2_SRC}
  ${MAC_SRC}
  ${ENB_APP_SRC}
  ${MCE_APP_SRC}
  )
target_link_libraries(L2 PRIVATE x2ap s1ap lte_rrc m2ap)
target_link_libraries(L2 PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

if(E2_AGENT)
  target_link_libraries(L2 PUBLIC e2_agent e2_agent_arg e2_ran_func_du_cucp_cuup)
  target_compile_definitions(L2 PRIVATE E2_AGENT)
  target_compile_definitions(L2 PRIVATE ${E2AP_VERSION} ${KPM_VERSION})
endif()


add_library(MAC_UE_NR ${MAC_NR_SRC_UE})
target_link_libraries(MAC_UE_NR PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

add_library(L2_LTE ${L2_LTE_SRC})
target_link_libraries(L2_LTE PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

add_library(L2_NR
  ${L2_NR_SRC}
  ${MAC_NR_SRC}
  ${GNB_APP_SRC}
  )

add_library(e1_if
  ${NR_RRC_DIR}/cucp_cuup_direct.c
  ${NR_RRC_DIR}/cucp_cuup_e1ap.c
  )

target_link_libraries(e1_if PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs asn1_f1ap SECURITY ${OPENSSL_LIBRARIES} e1ap GTPV1U)

target_link_libraries(L2_NR PRIVATE f1ap x2ap s1ap ngap nr_rrc e1ap nr_rlc)
if(E2_AGENT)
  target_link_libraries(L2_NR PUBLIC e2_agent e2_agent_arg e2_ran_func_du_cucp_cuup)
  target_compile_definitions(L2_NR PRIVATE ${E2AP_VERSION} ${KPM_VERSION})
  target_compile_definitions(L2_NR PRIVATE E2_AGENT)
endif()

add_library(L2_LTE_NR
  ${L2_RRC_SRC}
  ${MAC_SRC}
  ${ENB_APP_SRC}
  ${MCE_APP_SRC}
)

target_link_libraries(L2_LTE_NR PRIVATE f1ap s1ap nr_rrc)

add_library(L2_UE
  ${L2_SRC_UE}
  ${MAC_SRC_UE}
)
target_link_libraries(L2_UE PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

add_library(L2_UE_LTE_NR
  ${L2_RRC_SRC_UE}
  ${MAC_SRC_UE}
)
target_link_libraries(L2_UE_LTE_NR PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

if (NOT ${NOS1})
target_compile_definitions(L2_UE PUBLIC -DPDCP_USE_NETLINK)
endif()

target_link_libraries(L2_UE PRIVATE asn1_lte_rrc_hdrs)

add_library( NR_L2_UE ${NR_L2_SRC_UE} ${MAC_NR_SRC_UE} )
target_link_libraries(NR_L2_UE PRIVATE f1ap nr_rlc)
target_link_libraries(NR_L2_UE PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

add_library(MAC_NR_COMMON
            ${OPENAIR2_DIR}/LAYER2/NR_MAC_COMMON/nr_mac_common.c
            ${OPENAIR2_DIR}/LAYER2/NR_MAC_COMMON/nr_compute_tbs_common.c
)
target_link_libraries(MAC_NR_COMMON PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

include_directories("${OPENAIR2_DIR}/NR_UE_PHY_INTERFACE")
include_directories("${OPENAIR2_DIR}/LAYER2")
include_directories("${OPENAIR1_DIR}/SCHED_NR_UE")
#include_directories("${NFAPI_USER_DIR}"")

# L3 Libs
##########################

# CN libs
##########################

add_library (GTPV1U
  ${NR_RRC_DIR}/rrc_gNB_GTPV1U.c
  ${RRC_DIR}/rrc_eNB_GTPV1U.c
  ${OPENAIR3_DIR}/ocp-gtpu/gtp_itf.cpp
  )
target_link_libraries(GTPV1U PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
target_link_libraries(GTPV1U PRIVATE SIMU)
include_directories(${OPENAIR3_DIR}/ocp-gtp)

set (MME_APP_SRC
  ${OPENAIR3_DIR}/MME_APP/mme_app.c
  ${OPENAIR3_DIR}/MME_APP/mme_config.c
)
add_library(MME_APP ${MME_APP_SRC})
target_link_libraries(MME_APP PRIVATE m2ap m3ap)
target_link_libraries(MME_APP PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

set(SCTP_SRC
  ${OPENAIR3_DIR}/SCTP/sctp_common.c
  ${OPENAIR3_DIR}/SCTP/sctp_eNB_task.c
  ${OPENAIR3_DIR}/SCTP/sctp_eNB_itti_messaging.c
)
add_library(SCTP_CLIENT ${SCTP_SRC})
target_link_libraries(SCTP_CLIENT PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

set(NAS_SRC ${OPENAIR3_DIR}/NAS/)
set(libnas_api_OBJS
  ${NAS_SRC}COMMON/API/NETWORK/as_message.c
  ${NAS_SRC}COMMON/API/NETWORK/nas_message.c
  ${NAS_SRC}COMMON/API/NETWORK/network_api.c
  )

set(libnas_emm_msg_OBJS
  ${NAS_SRC}COMMON/EMM/MSG/AttachAccept.c
  ${NAS_SRC}COMMON/EMM/MSG/AttachComplete.c
  ${NAS_SRC}COMMON/EMM/MSG/AttachReject.c
  ${NAS_SRC}COMMON/EMM/MSG/AttachRequest.c
  ${NAS_SRC}COMMON/EMM/MSG/AuthenticationFailure.c
  ${NAS_SRC}COMMON/EMM/MSG/AuthenticationReject.c
  ${NAS_SRC}COMMON/EMM/MSG/AuthenticationRequest.c
  ${NAS_SRC}COMMON/EMM/MSG/AuthenticationResponse.c
  ${NAS_SRC}COMMON/EMM/MSG/CsServiceNotification.c
  ${NAS_SRC}COMMON/EMM/MSG/DetachAccept.c
  ${NAS_SRC}COMMON/EMM/MSG/DetachRequest.c
  ${NAS_SRC}COMMON/EMM/MSG/DownlinkNasTransport.c
  ${NAS_SRC}COMMON/EMM/MSG/EmmInformation.c
  ${NAS_SRC}COMMON/EMM/MSG/emm_msg.c
  ${NAS_SRC}COMMON/EMM/MSG/EmmStatus.c
  ${NAS_SRC}COMMON/EMM/MSG/ExtendedServiceRequest.c
  ${NAS_SRC}COMMON/EMM/MSG/GutiReallocationCommand.c
  ${NAS_SRC}COMMON/EMM/MSG/GutiReallocationComplete.c
  ${NAS_SRC}COMMON/EMM/MSG/IdentityRequest.c
  ${NAS_SRC}COMMON/EMM/MSG/IdentityResponse.c
  ${NAS_SRC}COMMON/EMM/MSG/SecurityModeCommand.c
  ${NAS_SRC}COMMON/EMM/MSG/SecurityModeComplete.c
  ${NAS_SRC}COMMON/EMM/MSG/SecurityModeReject.c
  ${NAS_SRC}COMMON/EMM/MSG/ServiceReject.c
  ${NAS_SRC}COMMON/EMM/MSG/ServiceRequest.c
  ${NAS_SRC}COMMON/EMM/MSG/TrackingAreaUpdateAccept.c
  ${NAS_SRC}COMMON/EMM/MSG/TrackingAreaUpdateComplete.c
  ${NAS_SRC}COMMON/EMM/MSG/TrackingAreaUpdateReject.c
  ${NAS_SRC}COMMON/EMM/MSG/TrackingAreaUpdateRequest.c
  ${NAS_SRC}COMMON/EMM/MSG/UplinkNasTransport.c
)

set(libnas_esm_msg_OBJS
  ${NAS_SRC}COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextAccept.c
  ${NAS_SRC}COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextReject.c
  ${NAS_SRC}COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextRequest.c
  ${NAS_SRC}COMMON/ESM/MSG/ActivateDefaultEpsBearerContextAccept.c
  ${NAS_SRC}COMMON/ESM/MSG/ActivateDefaultEpsBearerContextReject.c
  ${NAS_SRC}COMMON/ESM/MSG/ActivateDefaultEpsBearerContextRequest.c
  ${NAS_SRC}COMMON/ESM/MSG/BearerResourceAllocationReject.c
  ${NAS_SRC}COMMON/ESM/MSG/BearerResourceAllocationRequest.c
  ${NAS_SRC}COMMON/ESM/MSG/BearerResourceModificationReject.c
  ${NAS_SRC}COMMON/ESM/MSG/BearerResourceModificationRequest.c
  ${NAS_SRC}COMMON/ESM/MSG/DeactivateEpsBearerContextAccept.c
  ${NAS_SRC}COMMON/ESM/MSG/DeactivateEpsBearerContextRequest.c
  ${NAS_SRC}COMMON/ESM/MSG/EsmInformationRequest.c
  ${NAS_SRC}COMMON/ESM/MSG/EsmInformationResponse.c
  ${NAS_SRC}COMMON/ESM/MSG/esm_msg.c
  ${NAS_SRC}COMMON/ESM/MSG/EsmStatus.c
  ${NAS_SRC}COMMON/ESM/MSG/ModifyEpsBearerContextAccept.c
  ${NAS_SRC}COMMON/ESM/MSG/ModifyEpsBearerContextReject.c
  ${NAS_SRC}COMMON/ESM/MSG/ModifyEpsBearerContextRequest.c
  ${NAS_SRC}COMMON/ESM/MSG/PdnConnectivityReject.c
  ${NAS_SRC}COMMON/ESM/MSG/PdnConnectivityRequest.c
  ${NAS_SRC}COMMON/ESM/MSG/PdnDisconnectReject.c
  ${NAS_SRC}COMMON/ESM/MSG/PdnDisconnectRequest.c
)

set(libnas_ies_OBJS
  ${NAS_SRC}COMMON/IES/AccessPointName.c
  ${NAS_SRC}COMMON/IES/AdditionalUpdateResult.c
  ${NAS_SRC}COMMON/IES/AdditionalUpdateType.c
  ${NAS_SRC}COMMON/IES/ApnAggregateMaximumBitRate.c
  ${NAS_SRC}COMMON/IES/AuthenticationFailureParameter.c
  ${NAS_SRC}COMMON/IES/AuthenticationParameterAutn.c
  ${NAS_SRC}COMMON/IES/AuthenticationParameterRand.c
  ${NAS_SRC}COMMON/IES/AuthenticationResponseParameter.c
  ${NAS_SRC}COMMON/IES/CipheringKeySequenceNumber.c
  ${NAS_SRC}COMMON/IES/Cli.c
  ${NAS_SRC}COMMON/IES/CsfbResponse.c
  ${NAS_SRC}COMMON/IES/DaylightSavingTime.c
  ${NAS_SRC}COMMON/IES/DetachType.c
  ${NAS_SRC}COMMON/IES/DrxParameter.c
  ${NAS_SRC}COMMON/IES/EmergencyNumberList.c
  ${NAS_SRC}COMMON/IES/EmmCause.c
  ${NAS_SRC}COMMON/IES/EpsAttachResult.c
  ${NAS_SRC}COMMON/IES/EpsAttachType.c
  ${NAS_SRC}COMMON/IES/EpsBearerContextStatus.c
  ${NAS_SRC}COMMON/IES/EpsBearerIdentity.c
  ${NAS_SRC}COMMON/IES/EpsMobileIdentity.c
  ${NAS_SRC}COMMON/IES/EpsNetworkFeatureSupport.c
  ${NAS_SRC}COMMON/IES/EpsQualityOfService.c
  ${NAS_SRC}COMMON/IES/EpsUpdateResult.c
  ${NAS_SRC}COMMON/IES/EpsUpdateType.c
  ${NAS_SRC}COMMON/IES/EsmCause.c
  ${NAS_SRC}COMMON/IES/EsmInformationTransferFlag.c
  ${NAS_SRC}COMMON/IES/EsmMessageContainer.c
  ${NAS_SRC}COMMON/IES/GprsTimer.c
  ${NAS_SRC}COMMON/IES/GutiType.c
  ${NAS_SRC}COMMON/IES/IdentityType2.c
  ${NAS_SRC}COMMON/IES/ImeisvRequest.c
  ${NAS_SRC}COMMON/IES/KsiAndSequenceNumber.c
  ${NAS_SRC}COMMON/IES/LcsClientIdentity.c
  ${NAS_SRC}COMMON/IES/LcsIndicator.c
  ${NAS_SRC}COMMON/IES/LinkedEpsBearerIdentity.c
  ${NAS_SRC}COMMON/IES/LlcServiceAccessPointIdentifier.c
  ${NAS_SRC}COMMON/IES/LocationAreaIdentification.c
  ${NAS_SRC}COMMON/IES/MessageType.c
  ${NAS_SRC}COMMON/IES/MobileIdentity.c
  ${NAS_SRC}COMMON/IES/MobileStationClassmark2.c
  ${NAS_SRC}COMMON/IES/MobileStationClassmark3.c
  ${NAS_SRC}COMMON/IES/MsNetworkCapability.c
  ${NAS_SRC}COMMON/IES/MsNetworkFeatureSupport.c
  ${NAS_SRC}COMMON/IES/NasKeySetIdentifier.c
  ${NAS_SRC}COMMON/IES/NasMessageContainer.c
  ${NAS_SRC}COMMON/IES/NasRequestType.c
  ${NAS_SRC}COMMON/IES/NasSecurityAlgorithms.c
  ${NAS_SRC}COMMON/IES/NetworkName.c
  ${NAS_SRC}COMMON/IES/Nonce.c
  ${NAS_SRC}COMMON/IES/PacketFlowIdentifier.c
  ${NAS_SRC}COMMON/IES/PagingIdentity.c
  ${NAS_SRC}COMMON/IES/PdnAddress.c
  ${NAS_SRC}COMMON/IES/PdnType.c
  ${NAS_SRC}COMMON/IES/PlmnList.c
  ${NAS_SRC}COMMON/IES/ProcedureTransactionIdentity.c
  ${NAS_SRC}COMMON/IES/ProtocolConfigurationOptions.c
  ${NAS_SRC}COMMON/IES/ProtocolDiscriminator.c
  ${NAS_SRC}COMMON/IES/PTmsiSignature.c
  ${NAS_SRC}COMMON/IES/QualityOfService.c
  ${NAS_SRC}COMMON/IES/RadioPriority.c
  ${NAS_SRC}COMMON/IES/SecurityHeaderType.c
  ${NAS_SRC}COMMON/IES/ServiceType.c
  ${NAS_SRC}COMMON/IES/ShortMac.c
  ${NAS_SRC}COMMON/IES/SsCode.c
  ${NAS_SRC}COMMON/IES/SupportedCodecList.c
  ${NAS_SRC}COMMON/IES/TimeZoneAndTime.c
  ${NAS_SRC}COMMON/IES/TimeZone.c
  ${NAS_SRC}COMMON/IES/TmsiStatus.c
  ${NAS_SRC}COMMON/IES/TrackingAreaIdentity.c
  ${NAS_SRC}COMMON/IES/TrackingAreaIdentityList.c
  ${NAS_SRC}COMMON/IES/TrafficFlowAggregateDescription.c
  ${NAS_SRC}COMMON/IES/TrafficFlowTemplate.c
  ${NAS_SRC}COMMON/IES/TransactionIdentifier.c
  ${NAS_SRC}COMMON/IES/UeNetworkCapability.c
  ${NAS_SRC}COMMON/IES/UeRadioCapabilityInformationUpdateNeeded.c
  ${NAS_SRC}COMMON/IES/UeSecurityCapability.c
  ${NAS_SRC}COMMON/IES/VoiceDomainPreferenceAndUeUsageSetting.c
)

set (libnas_utils_OBJS
  ${NAS_SRC}COMMON/UTIL/device.c
  common/utils/mem/memory.c
  ${NAS_SRC}COMMON/UTIL/nas_timer.c
  ${NAS_SRC}COMMON/UTIL/socket.c
  ${NAS_SRC}COMMON/UTIL/stty.c
  ${NAS_SRC}COMMON/UTIL/TLVEncoder.c
  ${NAS_SRC}COMMON/UTIL/TLVDecoder.c
  ${NAS_SRC}COMMON/UTIL/OctetString.c
)

set(libnas_ue_api_OBJS
  ${NAS_SRC}UE/API/USER/at_command.c
  ${NAS_SRC}UE/API/USER/at_error.c
  ${NAS_SRC}UE/API/USER/at_response.c
  ${NAS_SRC}UE/API/USER/user_api.c
  ${NAS_SRC}UE/API/USER/user_indication.c
  ${NAS_SRC}UE/API/USIM/aka_functions.c
  ${NAS_SRC}UE/API/USIM/usim_api.c
  )
set(libnas_ue_emm_OBJS
  ${NAS_SRC}UE/EMM/Attach.c
  ${NAS_SRC}UE/EMM/Authentication.c
  ${NAS_SRC}UE/EMM/Detach.c
  ${NAS_SRC}UE/EMM/emm_main.c
  ${NAS_SRC}UE/EMM/EmmStatusHdl.c
  ${NAS_SRC}UE/EMM/Identification.c
  ${NAS_SRC}UE/EMM/IdleMode.c
  ${NAS_SRC}UE/EMM/LowerLayer.c
  ${NAS_SRC}UE/EMM/SecurityModeControl.c
  ${NAS_SRC}UE/EMM/ServiceRequestHdl.c
  ${NAS_SRC}UE/EMM/TrackingAreaUpdate.c
  )
set(libnas_ue_emm_sap_OBJS
  ${NAS_SRC}UE/EMM/SAP/emm_as.c
  ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredAttachNeeded.c
  ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredAttemptingToAttach.c
  ${NAS_SRC}UE/EMM/SAP/EmmDeregistered.c
  ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredInitiated.c
  ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredLimitedService.c
  ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredNoCellAvailable.c
  ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredNoImsi.c
  ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredNormalService.c
  ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredPlmnSearch.c
  ${NAS_SRC}UE/EMM/SAP/emm_esm.c
  ${NAS_SRC}UE/EMM/SAP/emm_fsm.c
  ${NAS_SRC}UE/EMM/SAP/EmmNull.c
  ${NAS_SRC}UE/EMM/SAP/emm_recv.c
  ${NAS_SRC}UE/EMM/SAP/emm_reg.c
  ${NAS_SRC}UE/EMM/SAP/EmmRegisteredAttemptingToUpdate.c
  ${NAS_SRC}UE/EMM/SAP/EmmRegistered.c
  ${NAS_SRC}UE/EMM/SAP/EmmRegisteredImsiDetachInitiated.c
  ${NAS_SRC}UE/EMM/SAP/EmmRegisteredInitiated.c
  ${NAS_SRC}UE/EMM/SAP/EmmRegisteredLimitedService.c
  ${NAS_SRC}UE/EMM/SAP/EmmRegisteredNoCellAvailable.c
  ${NAS_SRC}UE/EMM/SAP/EmmRegisteredNormalService.c
  ${NAS_SRC}UE/EMM/SAP/EmmRegisteredPlmnSearch.c
  ${NAS_SRC}UE/EMM/SAP/EmmRegisteredUpdateNeeded.c
  ${NAS_SRC}UE/EMM/SAP/emm_sap.c
  ${NAS_SRC}UE/EMM/SAP/emm_send.c
  ${NAS_SRC}UE/EMM/SAP/EmmServiceRequestInitiated.c
  ${NAS_SRC}UE/EMM/SAP/EmmTrackingAreaUpdatingInitiated.c
  )
set (libnas_ue_esm_OBJS
  ${NAS_SRC}UE/ESM/DedicatedEpsBearerContextActivation.c
  ${NAS_SRC}UE/ESM/DefaultEpsBearerContextActivation.c
  ${NAS_SRC}UE/ESM/EpsBearerContextDeactivation.c
  ${NAS_SRC}UE/ESM/esm_ebr.c
  ${NAS_SRC}UE/ESM/esm_ebr_context.c
  ${NAS_SRC}UE/ESM/esm_ip.c
  ${NAS_SRC}UE/ESM/esm_main.c
  ${NAS_SRC}UE/ESM/esm_pt.c
  ${NAS_SRC}UE/ESM/EsmStatusHdl.c
  ${NAS_SRC}UE/ESM/PdnConnectivity.c
  ${NAS_SRC}UE/ESM/PdnDisconnect.c
  )
set(libnas_ue_esm_sap_OBJS
  ${NAS_SRC}UE/ESM/SAP/esm_recv.c
  ${NAS_SRC}UE/ESM/SAP/esm_send.c
  ${NAS_SRC}UE/ESM/SAP/esm_sap.c
  )

set(libnrnas_emm_msg_OBJS
  ${NAS_SRC}COMMON/EMM/MSG/RegistrationRequest.c
  ${NAS_SRC}COMMON/EMM/MSG/RegistrationAccept.c
  ${NAS_SRC}COMMON/EMM/MSG/FGSIdentityResponse.c
  ${NAS_SRC}COMMON/EMM/MSG/FGSAuthenticationResponse.c
  ${NAS_SRC}COMMON/EMM/MSG/FGSNASSecurityModeComplete.c
  ${NAS_SRC}COMMON/EMM/MSG/RegistrationComplete.c
  ${NAS_SRC}COMMON/EMM/MSG/FGSUplinkNasTransport.c
  ${NAS_SRC}COMMON/ESM/MSG/PduSessionEstablishRequest.c
  ${NAS_SRC}COMMON/ESM/MSG/PduSessionEstablishmentAccept.c
  ${NAS_SRC}COMMON/EMM/MSG/FGSDeregistrationRequestUEOriginating.c
  )

set(libnrnas_ies_OBJS
  ${NAS_SRC}COMMON/IES/ExtendedProtocolDiscriminator.c
  ${NAS_SRC}COMMON/IES/FGSMobileIdentity.c
  ${NAS_SRC}COMMON/IES/FGSRegistrationType.c
  ${NAS_SRC}COMMON/IES/SpareHalfOctet.c
  ${NAS_SRC}COMMON/IES/FGSRegistrationResult.c
  ${NAS_SRC}COMMON/IES/FGMMCapability.c
  ${NAS_SRC}COMMON/IES/NrUESecurityCapability.c
  ${NAS_SRC}COMMON/IES/FGCNasMessageContainer.c
  ${NAS_SRC}COMMON/IES/SORTransparentContainer.c
  )

add_library(LIB_NAS_SIMUE
  ${NAS_SRC}UE/nas_itti_messaging.c
  ${NAS_SRC}UE/nas_network.c
  ${NAS_SRC}UE/nas_parser.c
  ${NAS_SRC}UE/nas_proc.c
  ${NAS_SRC}UE/nas_user.c
  ${NAS_SRC}NR_UE/nr_nas_msg_sim.c
  ${libnas_api_OBJS}
  ${libnas_ue_api_OBJS}
  ${libnas_emm_msg_OBJS}
  ${libnas_esm_msg_OBJS}
  ${libnas_ies_OBJS}
  ${libnas_utils_OBJS}
  ${libnas_ue_emm_OBJS}
  ${libnas_ue_emm_sap_OBJS}
  ${libnas_ue_esm_OBJS}
  ${libnas_ue_esm_sap_OBJS}
  ${libnrnas_emm_msg_OBJS}
  ${libnrnas_ies_OBJS}
  $<TARGET_OBJECTS:ds>
  )
target_include_directories(LIB_NAS_SIMUE PRIVATE ${OPENAIR_DIR}/common/utils/ds/)
target_link_libraries(LIB_NAS_SIMUE PRIVATE lte_rrc)



set(NAS_SIM_LIB LIB_NAS_SIMUE)
target_link_libraries(LIB_NAS_SIMUE PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

add_library(LIB_NAS_UE
  ${NAS_SRC}UE/nas_itti_messaging.c
  ${NAS_SRC}UE/nas_network.c
  ${NAS_SRC}UE/nas_parser.c
  ${NAS_SRC}UE/nas_proc.c
  ${NAS_SRC}UE/nas_user.c
  ${libnas_api_OBJS}
  ${libnas_ue_api_OBJS}
  ${libnas_emm_msg_OBJS}
  ${libnas_esm_msg_OBJS}
  ${libnas_ies_OBJS}
  ${libnas_utils_OBJS}
  ${libnas_ue_emm_OBJS}
  ${libnas_ue_emm_sap_OBJS}
  ${libnas_ue_esm_OBJS}
  ${libnas_ue_esm_sap_OBJS}
  $<TARGET_OBJECTS:ds>
  )
target_include_directories(LIB_NAS_UE PRIVATE ${OPENAIR_DIR}/common/utils/ds/)
set(NAS_UE_LIB LIB_NAS_UE)
target_link_libraries(LIB_NAS_UE PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

include_directories(${NAS_SRC}NR_UE)
include_directories(${NAS_SRC}UE)
include_directories(${NAS_SRC}UE/API/USER)
include_directories(${NAS_SRC}UE/API/USIM)
include_directories(${NAS_SRC}UE/EMM)
include_directories(${NAS_SRC}UE/EMM/SAP)
include_directories(${NAS_SRC}UE/ESM)
include_directories(${NAS_SRC}UE/ESM/SAP)

# nbiot
add_definitions("-DNUMBER_OF_UE_MAX_NB_IoT=16")
set (NBIOT_SOURCES
  ${OPENAIR2_DIR}/ENB_APP/NB_IoT_config.c
  )
add_library(NB_IoT MODULE ${NBIOT_SOURCES} )

add_library(LIB_5GNAS_GNB
  ${NAS_SRC}/COMMON/nr_common.c
  ${NAS_SRC}/gNB/network_process_nas.c
  ${NAS_SRC}/NR_UE/ue_process_nas.c
  ${OPENAIR3_DIR}//UICC/usim_interface.c
  )

target_include_directories(LIB_5GNAS_GNB PRIVATE ${OPENAIR_DIR}/common/utils/ds/)
target_link_libraries(LIB_5GNAS_GNB PRIVATE SECURITY)
target_link_libraries(LIB_5GNAS_GNB PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

# Simulation library
##########################
set (SIMUSRC
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/taus.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/multipath_channel.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/multipath_tv_channel.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/abstraction.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/channel_sim.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/random_channel.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/rangen_double.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/phase_noise.c
  ${OPENAIR1_DIR}/SIMULATION/RF/rf.c
  ${OPENAIR1_DIR}/SIMULATION/RF/dac.c
  ${OPENAIR1_DIR}/SIMULATION/RF/adc.c
)
add_library(SIMU STATIC ${SIMUSRC} )
target_include_directories(SIMU PUBLIC ${OPENAIR1_DIR}/SIMULATION/TOOLS ${OPENAIR1_DIR}/SIMULATION/RF)

# Qt-based scope
add_boolean_option(ENABLE_NRQTSCOPE OFF "Build the Qt-Scope" OFF)
if (ENABLE_NRQTSCOPE)
  find_package(Qt5 REQUIRED COMPONENTS Widgets Charts)
  message ("Qt5 Widgets and Charts found for nrqtscope")
  set(QTSCOPE_SOURCE_NR
    ${OPENAIR1_DIR}/PHY/TOOLS/nr_phy_qt_scope.cpp
    ${OPENAIR1_DIR}/PHY/TOOLS/phy_scope_interface.c)
  # Creates rules for calling the Meta-Object Compiler (moc) on the given source files
  qt5_wrap_cpp(QTSCOPE_SOURCE_NR ${OPENAIR1_DIR}/PHY/TOOLS/nr_phy_qt_scope.h)
  add_library(nrqtscope MODULE ${QTSCOPE_SOURCE_NR})
  target_link_libraries(nrqtscope PRIVATE Qt5::Widgets Qt5::Charts)
  target_link_libraries(nrqtscope PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
endif()

add_library(SIMU_ETH STATIC
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/multicast_link.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/socket.c
)
target_link_libraries(SIMU_ETH PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

include_directories("${NFAPI_DIR}/nfapi/public_inc")
include_directories("${NFAPI_DIR}/common/public_inc")
include_directories("${NFAPI_DIR}/pnf/public_inc")
include_directories("${NFAPI_DIR}/nfapi/inc")
include_directories("${NFAPI_DIR}/sim_common/inc")
include_directories("${NFAPI_DIR}/pnf_sim/inc")

add_library(oai_iqplayer MODULE
	${OPENAIR_DIR}/radio/iqplayer/iqplayer_lib.c
	)
set(CMAKE_MODULE_PATH "${OPENAIR_DIR}/cmake_targets/tools/MODULES" "${CMAKE_MODULE_PATH}")

#################################
# add executables for operation
#################################
add_library(minimal_lib
  ${OPENAIR_DIR}/common/utils/LOG/log.c
  ${OPENAIR_DIR}/common/utils/minimal_stub.c
  )
target_link_libraries(minimal_lib pthread dl ${T_LIB})

add_executable(nfapi_test
  ${OPENAIR_DIR}/openair2/NR_PHY_INTERFACE/nfapi_5g_test.c
  )

add_executable(measurement_display
  ${OPENAIR_DIR}/common/utils/threadPool/measurement_display.c)
target_link_libraries (measurement_display minimal_lib)

add_executable(test5Gnas
  ${OPENAIR_DIR}/openair3/TEST/test5Gnas.c
)
target_link_libraries (test5Gnas LIB_5GNAS_GNB minimal_lib )


# lte-softmodem is both eNB and UE implementation
###################################################

add_executable(lte-softmodem
  ${OPENAIR_DIR}/executables/lte-enb.c
  ${OPENAIR_DIR}/executables/lte-ru.c
  ${OPENAIR_DIR}/executables/ru_control.c
  ${OPENAIR_DIR}/executables/lte-softmodem.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR2_DIR}/ENB_APP/NB_IoT_interface.c
  ${OPENAIR_DIR}/executables/create_tasks.c
  ${OPENAIR_DIR}/executables/create_tasks_mbms.c
  ${OPENAIR_DIR}/radio/COMMON/common_lib.c
  ${OPENAIR_DIR}/radio/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR3_DIR}/NAS/UE/nas_ue_task.c
  ${PHY_INTERFACE_DIR}/queue_t.c
  ${OPENAIR_DIR}/common/utils/lte/ue_power.c
  ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
  ${OPENAIR1_DIR}/PHY/TOOLS/phy_scope_interface.c
  )
add_dependencies(lte-softmodem oai_iqplayer)

target_link_libraries(lte-softmodem PRIVATE
  -Wl,--start-group
  lte_rrc nr_rrc s1ap m2ap x2ap m3ap GTPV1U SECURITY UTIL HASHTABLE SCTP_CLIENT MME_APP SCHED_LIB SCHED_RU_LIB
  PHY_COMMON PHY PHY_RU  L2 L2_LTE NFAPI_COMMON_LIB NFAPI_LIB NFAPI_VNF_LIB NFAPI_PNF_LIB NFAPI_USER_LIB MISC_NFAPI_LTE_LIB
  ${NAS_UE_LIB} ITTI SIMU SIMU_ETH shlib_loader
  -Wl,--end-group z dl)

target_link_libraries(lte-softmodem PRIVATE pthread m CONFIG_LIB rt sctp)
target_link_libraries(lte-softmodem PRIVATE ${T_LIB})
target_link_libraries(lte-softmodem PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

if(E2_AGENT)
  target_compile_definitions(lte-softmodem PRIVATE E2_AGENT)
  target_compile_definitions(lte-softmodem PRIVATE ${E2AP_VERSION} ${KPM_VERSION})
endif()

add_executable(oairu
  ${OPENAIR_DIR}/executables/lte-ru.c
  ${OPENAIR_DIR}/executables/ru_control.c
  ${OPENAIR_DIR}/radio/COMMON/common_lib.c
  ${OPENAIR_DIR}/radio/COMMON/record_player.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR_DIR}/openair1/SCHED/phy_procedures_lte_common.c
  ${OPENAIR_DIR}/executables/main_ru.c
  ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
)
target_link_libraries(oairu PRIVATE
  -Wl,--start-group
  SCHED_RU_LIB PHY_COMMON PHY_RU UTIL shlib_loader
  -Wl,--end-group z dl)

target_link_libraries(oairu PRIVATE pthread m CONFIG_LIB rt ${T_LIB})
target_link_libraries(oairu PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

# force the generation of ASN.1 so that we don't need to wait during the build
target_link_libraries(lte-softmodem PRIVATE
                      asn1_lte_rrc asn1_s1ap asn1_m2ap asn1_m3ap asn1_x2ap)


# lte-uesoftmodem is  UE implementation
#######################################

add_executable(lte-uesoftmodem
  ${OPENAIR_DIR}/executables/lte-ue.c
  ${OPENAIR_DIR}/executables/lte-uesoftmodem.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR_DIR}/executables/create_tasks_ue.c
  ${OPENAIR_DIR}/radio/COMMON/common_lib.c
  ${OPENAIR_DIR}/radio/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR3_DIR}/NAS/UE/nas_ue_task.c
  ${OPENAIR_DIR}/common/utils/lte/ue_power.c
  ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
  ${OPENAIR1_DIR}/PHY/TOOLS/phy_scope_interface.c
  )

if (NOT ${NOS1})
	target_compile_definitions(lte-uesoftmodem PRIVATE -DPDCP_USE_NETLINK)
endif()
target_link_libraries(lte-uesoftmodem PRIVATE
  -Wl,--start-group
  lte_rrc nr_rrc s1ap x2ap m2ap m3ap
  SECURITY UTIL HASHTABLE SCTP_CLIENT MME_APP SCHED_RU_LIB SCHED_UE_LIB PHY_COMMON
  PHY_UE PHY_RU L2_UE L2_LTE SIMU SIMU_ETH NFAPI_COMMON_LIB NFAPI_LIB NFAPI_PNF_LIB NFAPI_USER_LIB MISC_NFAPI_LTE_LIB
  ${NAS_UE_LIB} ITTI shlib_loader
  -Wl,--end-group z dl)

target_link_libraries(lte-uesoftmodem PRIVATE pthread m CONFIG_LIB rt sctp)
target_link_libraries(lte-uesoftmodem PRIVATE ${T_LIB})
target_link_libraries(lte-uesoftmodem PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

# force the generation of ASN.1 so that we don't need to wait during the build
target_link_libraries(lte-uesoftmodem PRIVATE
                      asn1_lte_rrc asn1_s1ap asn1_m2ap asn1_m3ap asn1_x2ap)

# nr-softmodem
###################################################

add_executable(nr-softmodem
  ${rrc_h}
  ${nr_rrc_h}
  ${s1ap_h}
  ${OPENAIR_DIR}/executables/nr-gnb.c
  ${OPENAIR_DIR}/executables/nr-ru.c
  ${OPENAIR_DIR}/executables/nr-softmodem.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR_DIR}/radio/COMMON/common_lib.c
  ${OPENAIR_DIR}/radio/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR_DIR}/common/utils/lte/ue_power.c
  ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${PHY_INTERFACE_DIR}/queue_t.c
  ${OPENAIR1_DIR}/PHY/TOOLS/phy_scope_interface.c
  )

target_link_libraries(nr-softmodem PRIVATE
  -Wl,--start-group
  UTIL HASHTABLE SCTP_CLIENT SCHED_LIB SCHED_RU_LIB SCHED_NR_LIB PHY_NR PHY PHY_COMMON PHY_NR_COMMON PHY_RU GTPV1U SECURITY
  ITTI ${NAS_UE_LIB} lte_rrc nr_rrc
  ngap s1ap L2_LTE_NR L2_NR MAC_NR_COMMON NFAPI_COMMON_LIB NFAPI_LIB NFAPI_VNF_LIB NFAPI_PNF_LIB NFAPI_USER_LIB SIMU SIMU_ETH
  x2ap f1ap m2ap m3ap e1ap shlib_loader
  -Wl,--end-group z dl)

target_link_libraries(nr-softmodem PRIVATE pthread m CONFIG_LIB rt sctp)
target_link_libraries(nr-softmodem PRIVATE ${T_LIB})
target_link_libraries(nr-softmodem PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
target_link_libraries(nr-softmodem PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
if(E2_AGENT)
  target_compile_definitions(nr-softmodem PRIVATE ${E2AP_VERSION} ${KPM_VERSION})
endif()


add_dependencies(nr-softmodem ldpc_orig ldpc_optim ldpc_optim8seg ldpc)

if (ENABLE_LDPC_T2)
  add_dependencies(nr-softmodem ldpc_t2)
endif()

# force the generation of ASN.1 so that we don't need to wait during the build
target_link_libraries(nr-softmodem PRIVATE
  asn1_lte_rrc asn1_nr_rrc asn1_s1ap asn1_ngap asn1_m2ap asn1_m3ap asn1_x2ap asn1_f1ap asn1_lpp)
if(E2_AGENT)
  target_compile_definitions(nr-softmodem PRIVATE E2_AGENT)
endif()


add_executable(nr-cuup
  executables/nr-cuup.c
  executables/softmodem-common.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${NR_RRC_DIR}/rrc_gNB_UE_context.c
  ${OPENAIR2_DIR}/E1AP/e1ap_setup.c
  ${NR_PDCP_SRC}
  ${NR_SDAP_SRC}
  )

target_link_libraries(nr-cuup PRIVATE
                      CONFIG_LIB ITTI SCTP_CLIENT
                      GTPV1U e1ap f1ap SIMU_ETH
                      z sctp dl pthread shlib_loader ${T_LIB})
target_link_libraries(nr-cuup PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)
if(E2_AGENT)
  target_link_libraries(nr-cuup PRIVATE e2_agent e2_agent_arg e2_ran_func_cuup)
  target_compile_definitions(nr-cuup PRIVATE ${E2AP_VERSION} ${KPM_VERSION} E2_AGENT)
endif()
                     
# nr-uesoftmodem is  UE implementation
#######################################

add_executable(nr-uesoftmodem
  ${rrc_h}
  ${s1ap_h}
  ${OPENAIR_DIR}/executables/nr-uesoftmodem.c
  ${OPENAIR_DIR}/executables/nr-ue.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR_DIR}/radio/COMMON/common_lib.c
  ${OPENAIR_DIR}/radio/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/LAYER2/NR_MAC_COMMON/nr_mac_common.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR3_DIR}/NAS/UE/nas_ue_task.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR1_DIR}/PHY/TOOLS/phy_scope_interface.c
  )

target_link_libraries(nr-uesoftmodem PRIVATE
  -Wl,--start-group
  nr_rrc SECURITY UTIL HASHTABLE SCHED_RU_LIB SCHED_NR_UE_LIB
  PHY_COMMON PHY_NR_COMMON PHY_NR_UE NR_L2_UE L2_UE_LTE_NR MAC_NR_COMMON NFAPI_COMMON_LIB NFAPI_LIB NFAPI_PNF_LIB
  NFAPI_USER_LIB MISC_NFAPI_NR_LIB
  ITTI LIB_5GNAS_GNB LIB_NAS_SIMUE ${NAS_SIM_LIB} SIMU SIMU_ETH shlib_loader
  -Wl,--end-group z dl)

target_link_libraries(nr-uesoftmodem PRIVATE pthread m CONFIG_LIB rt)
target_link_libraries(nr-uesoftmodem PRIVATE ${T_LIB})
target_link_libraries(nr-uesoftmodem PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

add_dependencies( nr-uesoftmodem ldpc_orig ldpc_optim ldpc_optim8seg ldpc )
if (ENABLE_LDPC_CUDA)
   add_dependencies(nr-uesoftmodem ldpc_cuda)
   add_dependencies(nr-softmodem ldpc_cuda)
endif()

# force the generation of ASN.1 so that we don't need to wait during the build
target_link_libraries(nr-uesoftmodem PRIVATE
                      asn1_lte_rrc asn1_nr_rrc asn1_s1ap asn1_ngap asn1_m2ap asn1_m3ap asn1_x2ap asn1_f1ap asn1_lpp)

###################################"
# Addexecutables for tests
####################################


# Unitary tests for each piece of L1: example, mbmssim is MBMS L1 simulator
#####################################

#special case for dlim TM4, which uses its own version of phy_scope code
add_executable(dlsim_tm4
  ${OPENAIR1_DIR}/SIMULATION/LTE_PHY/dlsim_tm4.c
  ${OPENAIR1_DIR}/PHY/TOOLS/lte_phy_scope_tm4.c
  )
target_link_libraries (dlsim_tm4
  -Wl,--start-group SIMU UTIL SCHED_LIB SCHED_RU_LIB PHY ITTI -Wl,--end-group
  pthread m rt CONFIG_LIB ${T_LIB}
  )

add_executable(rftest
  ${OPENAIR_DIR}/openair1/PHY/TOOLS/calibration_test.c
  ${OPENAIR_DIR}/openair1/PHY/TOOLS/calibration_scope.c
  ${OPENAIR_DIR}/radio/COMMON/common_lib.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
)
target_link_libraries(rftest PRIVATE minimal_lib PHY_NR_COMMON UTIL forms pthread dl m ${T_LIB} shlib_loader)
target_link_libraries(rftest PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

add_executable(polartest
  ${OPENAIR1_DIR}/PHY/CODING/TESTBENCH/polartest.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  )
target_link_libraries(polartest PRIVATE
  -Wl,--start-group UTIL SIMU PHY_COMMON PHY_NR PHY_NR_COMMON PHY_NR_UE CONFIG_LIB -Wl,--end-group
  m pthread dl ${T_LIB}
  )

add_executable(smallblocktest
  ${OPENAIR1_DIR}/PHY/CODING/TESTBENCH/smallblocktest.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
)

target_link_libraries(smallblocktest PRIVATE
  -Wl,--start-group UTIL SIMU PHY_NR PHY_COMMON PHY_NR_COMMON -Wl,--end-group
  m pthread dl shlib_loader ${T_LIB}
)


add_executable(ldpctest
   ${PHY_NR_CODINGIF}
   ${OPENAIR1_DIR}/PHY/CODING/TESTBENCH/ldpctest.c
   )

add_dependencies(ldpctest ldpc_orig ldpc_optim ldpc_optim8seg ldpc)
if (ENABLE_LDPC_CUDA)
  add_dependencies(ldpctest ldpc_cuda)
endif()
target_link_libraries(ldpctest PRIVATE
  -Wl,--start-group UTIL SIMU PHY_COMMON PHY_NR_COMMON -Wl,--end-group
  m pthread dl shlib_loader ${T_LIB}
  )

add_executable(nr_dlschsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/dlschsim.c
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/nr_dummy_functions.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  )
target_link_libraries(nr_dlschsim PRIVATE
  -Wl,--start-group UTIL SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB CONFIG_LIB MAC_NR_COMMON -Wl,--end-group
  m pthread ${T_LIB} ITTI dl shlib_loader
  )
target_link_libraries(nr_dlschsim PRIVATE asn1_nr_rrc_hdrs)

add_executable(nr_pbchsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/pbchsim.c
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/nr_dummy_functions.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  )
target_link_libraries(nr_pbchsim PRIVATE
  -Wl,--start-group UTIL SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB CONFIG_LIB MAC_NR_COMMON -Wl,--end-group
  m pthread ${T_LIB} ITTI dl shlib_loader
)
target_link_libraries(nr_pbchsim PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)


#PUCCH ---> Prashanth
add_executable(nr_pucchsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/pucchsim.c
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/nr_dummy_functions.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  )
target_link_libraries(nr_pucchsim PRIVATE
  -Wl,--start-group UTIL SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB CONFIG_LIB MAC_NR_COMMON -Wl,--end-group
  m pthread ${T_LIB} ITTI dl shlib_loader
)
target_link_libraries(nr_pucchsim PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

add_executable(nr_dlsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/dlsim.c
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/nr_dummy_functions.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${NR_UE_RRC_DIR}/rrc_nsa.c
  ${NFAPI_USER_DIR}/nfapi.c
  ${NFAPI_USER_DIR}/gnb_ind_vars.c
  ${PHY_INTERFACE_DIR}/queue_t.c
  )
target_link_libraries(nr_dlsim PRIVATE
  -Wl,--start-group UTIL SIMU SIMU_ETH PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB SCHED_NR_UE_LIB MAC_UE_NR MAC_NR_COMMON nr_rrc CONFIG_LIB L2_NR HASHTABLE x2ap SECURITY ngap -lz -Wl,--end-group
  m pthread ${T_LIB} ITTI dl shlib_loader
  )
target_link_libraries(nr_dlsim PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

add_executable(nr_prachsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/prachsim.c
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/nr_dummy_functions.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
)
target_link_libraries(nr_prachsim PRIVATE
  -Wl,--start-group UTIL SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_RU PHY_NR_UE MAC_NR_COMMON SCHED_NR_LIB CONFIG_LIB -lz -Wl,--end-group
  m pthread ${T_LIB} ITTI dl shlib_loader)
target_link_libraries(nr_prachsim PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

add_executable(nr_ulschsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/ulschsim.c
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/nr_dummy_functions.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${PHY_INTERFACE_DIR}/queue_t.c
  )
target_link_libraries(nr_ulschsim PRIVATE
  -Wl,--start-group UTIL SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB CONFIG_LIB MAC_NR_COMMON -Wl,--end-group
  m pthread ${T_LIB} ITTI dl shlib_loader
)
target_link_libraries(nr_ulschsim PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

add_executable(nr_ulsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/ulsim.c
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/nr_dummy_functions.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${NR_UE_RRC_DIR}/rrc_nsa.c
  ${NFAPI_USER_DIR}/nfapi.c
  ${NFAPI_USER_DIR}/gnb_ind_vars.c
  ${PHY_INTERFACE_DIR}/queue_t.c
  )

if (ENABLE_LDPC_T2)
  add_dependencies(nr_ulsim ldpc_t2)
endif()

target_link_libraries(nr_ulsim PRIVATE
  -Wl,--start-group UTIL SIMU SIMU_ETH PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB SCHED_NR_UE_LIB MAC_UE_NR MAC_NR_COMMON nr_rrc CONFIG_LIB L2_NR HASHTABLE x2ap SECURITY ngap -lz -Wl,--end-group
  m pthread ${T_LIB} ITTI dl shlib_loader
  )
target_link_libraries(nr_ulsim PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

foreach(myExe dlsim dlsim_tm7 ulsim pbchsim scansim mbmssim pdcchsim pucchsim prachsim syncsim)

  add_executable(${myExe}
    ${OPENAIR1_DIR}/SIMULATION/LTE_PHY/${myExe}.c
    ${OPENAIR_DIR}/common/utils/lte/ue_power.c
    ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
    ${OPENAIR1_DIR}/PHY/INIT/lte_param_init.c
    ${NFAPI_USER_DIR}/nfapi.c
    )
  target_link_libraries (${myExe} PRIVATE
    -Wl,--start-group SIMU UTIL SCHED_LIB SCHED_RU_LIB SCHED_UE_LIB PHY_COMMON PHY_NR_COMMON PHY PHY_UE PHY_RU ITTI -Wl,--end-group
    pthread m rt CONFIG_LIB ${T_LIB} dl shlib_loader
    )
  target_link_libraries(${myExe} PRIVATE asn1_lte_rrc_hdrs)

endforeach(myExe)

#unitary tests for Core NEtwork pieces
#################################
foreach(myExe s1ap
    secu_knas_encrypt_eia1
    secu_kenb
    aes128_ctr
    secu_knas_encrypt_eea2
    secu_knas secu_knas_encrypt_eea1
    kdf
    aes128_cmac_encrypt
    secu_knas_encrypt_eia2)
  add_executable(test_${myExe}
    ${OPENAIR3_DIR}/TEST/test_${myExe}.c
    )
  target_link_libraries (test_${myExe}
    -Wl,--start-group SECURITY UTIL -Wl,--end-group m rt CONFIG_LIB
    )
endforeach(myExe)

# to be added
#../targets/TEST/PDCP/test_pdcp.c
#../targets/TEST/PDCP/with_rlc/test_pdcp_rlc.c

#ensure that the T header files are generated before targets depending on them
if (${T_TRACER})
  foreach(i
        #all "add_executable" definitions (except tests, rb_tool, updatefw)
        lte-softmodem lte-uesoftmodem nr-softmodem
        nr-uesoftmodem dlsim dlsim_tm4 dlsim_tm7
        ulsim pbchsim scansim mbmssim pdcchsim pucchsim prachsim
        syncsim nr_ulsim nr_dlsim nr_dlschsim nr_pbchsim nr_pucchsim
        nr_ulschsim ldpctest polartest smallblocktest cu_test du_test
        #all "add_library" definitions
        ITTI lte_rrc nr_rrc s1ap x2ap m2ap m3ap f1ap
        params_libconfig oai_usrpdevif oai_bladerfdevif oai_lmssdrdevif oai_iqplayer
        oai_eth_transpro oai_mobipass coding HASHTABLE UTIL OMG_SUMO
        SECURITY SCHED_LIB SCHED_NR_LIB SCHED_RU_LIB SCHED_UE_LIB SCHED_NR_UE_LIB default_sched remote_sched RAL
        NFAPI_COMMON_LIB NFAPI_LIB NFAPI_PNF_LIB NFAPI_VNF_LIB NFAPI_USER_LIB
        MISC_NFAPI_LTE_LIB MISC_NFAPI_NR_LIB
        PHY_COMMON PHY PHY_UE PHY_NR PHY_NR_COMMON PHY_NR_UE PHY_RU PHY_MEX
        L2 L2_LTE L2_NR L2_LTE_NR L2_UE NR_L2_UE L2_UE_LTE_NR MAC_NR_COMMON MAC_UE_NR ngap
        CN_UTILS GTPV1U SCTP_CLIENT MME_APP LIB_NAS_UE NB_IoT SIMU SIMU_ETH OPENAIR0_LIB
        ldpc_orig ldpc_optim ldpc_optim8seg ldpc_t2 ldpc_cl ldpc_cuda ldpc dfts config_internals)
    if (TARGET ${i})
      add_dependencies(${i} generate_T)
    endif()
  endforeach(i)
endif (${T_TRACER})

##################################################
# Generated specific cases is not regular code
###############################################

################
# Kernel modules
###############
# Set compiler options for kernel modules
# we need to get out cmake to use the regular Linux Kernel process
# this is documented as https://www.kernel.org/doc/Documentation/kbuild/modules.txt
######################################

# retrieve the compiler options to send it to gccxml
get_directory_property(DirDefs COMPILE_DEFINITIONS )
foreach( d ${DirDefs} )
  set(module_cc_opt "${module_cc_opt} -D${d}")
endforeach()
get_directory_property( DirDefs INCLUDE_DIRECTORIES )
foreach( d ${DirDefs} )
  set(module_cc_opt "${module_cc_opt} -I${d}")
endforeach()

EXECUTE_PROCESS(COMMAND uname -r
  OUTPUT_VARIABLE os_release
  OUTPUT_STRIP_TRAILING_WHITESPACE)
SET(module_build_path /lib/modules/${os_release}/build)

function(make_driver name dir)
  file(MAKE_DIRECTORY ${OPENAIR_BIN_DIR}/${name})
  foreach(f  IN  ITEMS ${ARGN})
    list(APPEND src_path_list ${dir}/${f})
    string(REGEX REPLACE "c *$" "o" obj ${f})
    set(objs "${objs} ${obj}")
  endforeach()
  CONFIGURE_FILE(${OPENAIR_CMAKE}/tools/Kbuild.cmake ${OPENAIR_BIN_DIR}/${name}/Kbuild)
  add_custom_command(OUTPUT ${name}.ko
    COMMAND make -C ${module_build_path} M=${OPENAIR_BIN_DIR}/${name}
    WORKING_DIRECTORY ${OPENAIR_BIN_DIR}/${name}
    COMMENT "building ${module}.ko"
    VERBATIM
    SOURCES  ${src_path_list}
    )
  add_custom_target(${name} DEPENDS ${name}.ko)
endfunction(make_driver name dir src)

# nashmesh module
################
list(APPEND nasmesh_src device.c common.c ioctl.c classifier.c tool.c mesh.c)
set(module_cc_opt "${module_cc_opt} -DNAS_NETLINK -DPDCP_USE_NETLINK")
# legacy Makefile was using NAS_NETLINK flag, but other drivers the hereafter flag
# so, this cmake use OAI_NW_DRIVER_USE_NETLINK everywhere
if (OAI_NW_DRIVER_USE_NETLINK)
  list(APPEND nasmesh_src netlink.c)
endif()
make_driver(nasmesh  ${OPENAIR2_DIR}/NETWORK_DRIVER/MESH ${nasmesh_src})

# user space tool for configuring MESH IP driver
################
add_executable(rb_tool
  ${OPENAIR2_DIR}/NETWORK_DRIVER/MESH/RB_TOOL/rb_tool.c
)
target_include_directories(rb_tool PRIVATE ${OPENAIR2_DIR}/NETWORK_DRIVER/MESH/)

# ???
####################
list(APPEND oai_nw_drv_src device.c common.c ioctl.c classifier.c tool.c)
if(OAI_NW_DRIVER_USE_NETLINK)
  list(APPEND oai_nw_drv_src netlink.c)
endif()
make_driver(oai_nw_drv ${OPENAIR2_DIR}/NETWORK_DRIVER/LTE ${oai_nw_drv_src})


# ue_ip: purpose ???
###############
list(APPEND ue_ip_src device.c common.c)
if(OAI_NW_DRIVER_USE_NETLINK)
  list(APPEND ue_ip_src netlink.c)
endif()
make_driver(ue_ip ${OPENAIR2_DIR}/NETWORK_DRIVER/UE_IP ${ue_ip_src})

include(${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/CMakeLists.txt)

set(ENABLE_TESTS OFF CACHE STRING "Activate build of tests")
set_property(CACHE ENABLE_TESTS PROPERTY TYPE BOOL)
if(ENABLE_TESTS)
  # activates globally adding tests such that they can be run using ctest
  enable_testing()
  # meta-target: each test is supposed to add_dependencies(tests ${NEWTEST})
  # then, it is possible to build and execute all tests using "ninja tests && ctest"
  add_custom_target(tests)
endif()

add_subdirectory(common)
add_subdirectory(doc)
add_subdirectory(openair1)
add_subdirectory(openair2)
add_subdirectory(openair3)
add_subdirectory(radio)
