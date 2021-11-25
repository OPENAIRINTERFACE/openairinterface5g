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

cmake_minimum_required (VERSION 3.0)


# System packages that are required
# We use either the cmake buildin, in ubuntu are in: /usr/share/cmake*/Modules/
# or cmake provide a generic interface to pkg-config that widely used
###################################
include(FindPkgConfig)

pkg_search_module(LIBXML2 libxml-2.0 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIRS})

pkg_search_module(LIBXSLT libxslt REQUIRED)
include_directories(${LIBXSLT_INCLUDE_DIRS})

pkg_search_module(OPENSSL openssl REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})

pkg_search_module(CONFIG libconfig REQUIRED)
include_directories(${CONFIG_INCLUDE_DIRS})

pkg_search_module(CRYPTO libcrypto REQUIRED)
include_directories(${CRYPTO_INCLUDE_DIRS})

#uhd 4.0 and iris installs by default in /usr/local
include_directories("/usr/local/include/")
#use native cmake method as this package is not in pkg-config
if (${RF_BOARD} STREQUAL "OAI_USRP")
  find_package(Boost REQUIRED)
  include_directories(${LIBBOOST_INCLUDE_DIR})

elseif (${RF_BOARD} STREQUAL "OAI_IRIS")
    include_directories("${OPENAIR_TARGETS}/ARCH/IRIS/USERSPACE/LIB/")

    set(HW_SOURCE ${HW_SOURCE}
            ${OPENAIR_TARGETS}/ARCH/IRIS/USERSPACE/LIB/iris_lib.cpp)
    LINK_DIRECTORIES("/usr/local/lib")
    set(option_HW_lib "-lSoapySDR -rdynamic -ldl")

elseif (${RF_BOARD} STREQUAL "OAI_AW2SORI")
    include_directories("${OPENAIR_TARGETS}/ARCH/AW2SORI")
    set(HW_SOURCE ${HW_SOURCE}
	    ${OPENAIR_TARGETS}/ARCH/AW2SORI/ARCH/AW2SORI/oaiori.c)
    LINK_DIRECTORIES("/usr/local/lib")
    set(openair_HW_lib "-shared -fPIC -msse4 -g -ggdb -lori")

endif (${RF_BOARD} STREQUAL "OAI_USRP")

message("RU=${RU}")

pkg_search_module(OPENPGM openpgm-5.1 openpgm-5.2)
if(NOT ${OPENPGM_FOUND})
  message("PACKAGE openpgm-5.1 is required by binaries such as oaisim: will fail later if this target is built")
else()
  include_directories(${OPENPGM_INCLUDE_DIRS})
endif()

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

pkg_search_module(XPM xpm)
if(NOT ${XPM_FOUND})
  message("PACKAGE xpm not found: some targets will fail")
else()
  include_directories(${XPM_INCLUDE_DIRS})
endif()

# Atlas is required by some packages, but not found in pkg-config
# So, here are some hacks here. Hope this gets fixed in future!
if(EXISTS "/usr/include/atlas/cblas.h" OR EXISTS "/usr/include/cblas.h")
  include_directories("/usr/include/atlas")
  LINK_DIRECTORIES("/usr/lib/lapack")
  LINK_DIRECTORIES("/usr/lib64")
  LINK_DIRECTORIES("/usr/lib64/atlas") #Added because atlas libraries in CentOS 7 are here!

  if(EXISTS "/usr/lib64/libblas.so" OR EXISTS "/usr/lib/libblas.so") #Case for CentOS7
     list(APPEND ATLAS_LIBRARIES blas)

  else() # Case for Ubuntu
     list(APPEND ATLAS_LIBRARIES cblas)
  endif()

  if(EXISTS "/usr/lib/atlas/libtatlas.so" OR EXISTS "/usr/lib64/atlas/libtatlas.so") #Case for CentOS7
     list(APPEND ATLAS_LIBRARIES tatlas)
  else()
     list(APPEND ATLAS_LIBRARIES atlas) #Case for Ubuntu
  endif()

  list(APPEND ATLAS_LIBRARIES lapack)

# for ubuntu 17.10, directories are different
elseif(EXISTS "/usr/include/x86_64-linux-gnu/cblas.h")
  
  include_directories("/usr/include/x86_64-linux-gnu")
  LINK_DIRECTORIES("/usr/lib/x86_64-linux-gnu")
  list(APPEND ATLAS_LIBRARIES cblas)
  list(APPEND ATLAS_LIBRARIES atlas)
  list(APPEND ATLAS_LIBRARIES lapack)
  
else()
  message("No Blas/Atlas libs found, some targets will fail")
endif()

#########################################################
# Base directories, compatible with legacy OAI building #
#########################################################
set (OPENAIR_DIR     $ENV{OPENAIR_DIR})
if("${OPENAIR_DIR}" STREQUAL "")
  string(REGEX REPLACE "/cmake_targets.*$" "" OPENAIR_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

set (NFAPI_DIR       ${OPENAIR_DIR}/nfapi/open-nFAPI)
set (NFAPI_USER_DIR  ${OPENAIR_DIR}/nfapi/oai_integration)
set (OPENAIR1_DIR    ${OPENAIR_DIR}/openair1)
set (OPENAIR2_DIR    ${OPENAIR_DIR}/openair2)
set (OPENAIR3_DIR    ${OPENAIR_DIR}/openair3)
set (OPENAIR_TARGETS ${OPENAIR_DIR}/targets)
set (OPENAIR3_DIR    ${OPENAIR_DIR}/openair3)
set (OPENAIR_CMAKE   ${OPENAIR_DIR}/cmake_targets)
set (OPENAIR_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY})

project (OpenAirInterface)

##############################################
# Base CUDA setting
##############################################
find_package(CUDA)
if (CUDA_FOUND)
    message ("cuda include ${CUDA_INCLUDE_DIRS}")
    message ("cuda library ${CUDA_LIBRARY_DIRS}")

    add_definitions("-L/usr/local/cuda/lib64")

    SET(CUDA_NVCC_FLAGS
      "${CUDA_NVCC_FLAGS};-arch=sm_60;")

    # Disable warnings for CUDA
    SET(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS};-lpthread;-w;-O3;--default-stream;per-thread;-I/usr/local/cuda/inc;-L/usr/local/cuda/lib -lcutil;-rdc=true;-lcudadevrt")
    SET(CUDA_VERBOSE_BUILD ON)
    SET(CUDA_HOST_COMPILER "/usr/bin/g++")

    SET(CUDA_SEPARABLE_COMPILATION ON)

else (CUDA_FOUND)
   message ("No CUDA tool installed")
endif ()


###########################################
# macros to define options as there is numerous options in oai
################################################
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

macro(add_integer_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  add_definitions("-D${name}=${value}")
endmacro(add_integer_option)

macro(add_list1_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY STRINGS ${ARGN})
  if(NOT "${value}" STREQUAL "False")
    add_definitions("-D${name}=${value}")
  endif()
endmacro(add_list1_option)

macro(add_list2_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY STRINGS ${ARGN})
  if(NOT "${value}" STREQUAL "False")
    add_definitions("-D${value}=1")
  endif()
endmacro(add_list2_option)

macro(add_list_string_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY STRINGS ${ARGN})
  if(NOT "${value}" STREQUAL "False")
    add_definitions("-D${name}=\"${value}\"")
  endif()
endmacro(add_list_string_option)

# this function should produce the same value as the macro MAKE_VERSION defined in the C code (file types.h)
function(make_version VERSION_VALUE)
  math(EXPR RESULT "0")
  foreach (ARG ${ARGN})
    math(EXPR RESULT "${RESULT} * 16 + ${ARG}")
  endforeach()
  set(${VERSION_VALUE} "${RESULT}" PARENT_SCOPE)
endfunction()
####################################################
# compilation flags
#############################################

#set(CMAKE_BUILD_TYPE "Debug")
if (CMAKE_BUILD_TYPE STREQUAL "")
   set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()
message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
add_list_string_option(CMAKE_BUILD_TYPE "RelWithDebInfo" "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." Debug Release RelWithDebInfo MinSizeRel)

Message("Architecture is ${CMAKE_SYSTEM_PROCESSOR}")
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
  set(C_FLAGS_PROCESSOR "-gdwarf-2 -mfloat-abi=hard -mfpu=neon -lgcc -lrt")
else (CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
  if(EXISTS  "/proc/cpuinfo")
    file(STRINGS "/proc/cpuinfo" CPUINFO REGEX flags LIMIT_COUNT 1)
    if (CPUINFO MATCHES "avx2")
      set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -mavx2")
      set(COMPILATION_AVX2 "True")
    else()
      set(COMPILATION_AVX2 "False")
    endif()
    if (CPUINFO MATCHES "sse4_1")
      set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -msse4.1")
    endif()
    if (CPUINFO MATCHES "ssse3")
      set(C_FLAGS_PROCESSOR "${C_FLAGS_PROCESSOR} -mssse3")
    endif()
  else()
    Message("/proc/cpuinfo does not exit. We will use manual CPU flags")
  endif()
endif()

set(C_FLAGS_PROCESSOR " ${C_FLAGS_PROCESSOR} ${CFLAGS_PROCESSOR_USER}")

Message("C_FLAGS_PROCESSOR is ${C_FLAGS_PROCESSOR}")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
  if ( (NOT( C_FLAGS_PROCESSOR MATCHES "ssse3")) OR (NOT( C_FLAGS_PROCESSOR MATCHES "msse4.1")) )
    Message(FATAL_ERROR "For x86 Architecture, you must have following flags: -mssse3 -msse4.1. The current detected flags are: ${C_FLAGS_PROCESSOR}. You can pass the flags manually in build script, for example: ./build_oai --cflags_processor \"-mssse3 -msse4.1 -mavx2\" ")
  endif()
endif()

#
# add autotools definitions that were maybe used!

add_definitions("-DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_FCNTL_H=1 -DHAVE_ARPA_INET_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_SOCKET_H=1 -DHAVE_STRERROR=1 -DHAVE_SOCKET=1 -DHAVE_MEMSET=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_STDLIB_H=1 -DHAVE_MALLOC=1 -DHAVE_LIBSCTP")

set(commonOpts "-pipe -Wno-packed-bitfield-compat -fPIC -Wall -fno-strict-aliasing -rdynamic")

set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} ${C_FLAGS_PROCESSOR} ${commonOpts} -std=gnu99 -funroll-loops")
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} ${C_FLAGS_PROCESSOR}  ${commonOpts} -std=c++11")



if (SANITIZE_ADDRESS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-common")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-common")
endif ()

add_definitions("-DASN_DISABLE_OER_SUPPORT")

#########################
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -ggdb2 -Wl,-rpath -Wl,${CMAKE_CURRENT_BINARY_DIR}")
#########################
# set a flag for changes in the source code
# these changes are related to hardcoded path to include .h files
set(debugOpt "-ggdb2 -DMALLOC_CHECK_=3 -fno-delete-null-pointer-checks")
set(CMAKE_C_FLAGS_DEBUG "${debugOpt} -O0")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${debugOpt} -O2")
set(CMAKE_C_FLAGS_RELEASE "-O3")

set(GIT_BRANCH        "UNKNOWN")
set(GIT_COMMIT_HASH   "UNKNOWN")
set(GIT_COMMIT_DATE   "UNKNOWN")

find_package(Git)
if(GIT_FOUND)
  message("git found: ${GIT_EXECUTABLE}")
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
#add_definitions("-DEMIT_ASN_DEBUG=1")


# Debug related options
#########################################
add_boolean_option(ASN_DEBUG           False "ASN1 coder/decoder Debug")
add_boolean_option(EMIT_ASN_DEBUG      False "ASN1 coder/decoder Debug")
add_boolean_option(MSG_PRINT           False "print debug messages")
add_boolean_option(DISABLE_XER_PRINT   False "print XER Format")
add_boolean_option(XER_PRINT           False "print XER Format")
add_boolean_option(RRC_MSG_PRINT       False "print RRC messages")
add_boolean_option(PDCP_MSG_PRINT      False "print PDCP messages to /tmp/pdcp.log")
add_boolean_option(DEBUG_PDCP_PAYLOAD  False "print PDCP PDU to stdout")  # if true, make sure that global and PDCP log levels are trace
add_boolean_option(DEBUG_MAC_INTERFACE False "print MAC-RLC PDU exchange to stdout") # if true, make sure that global and PDCP log levels are trace
add_boolean_option(TRACE_RLC_PAYLOAD   False "print RLC PDU to stdout") # if true, make sure that global and PDCP log levels are trace
add_boolean_option(PRINT_STATS         False "This adds the possibility to see the status")
add_boolean_option(T_TRACER            True  "Activate the T tracer, a debugging/monitoring framework" )
add_boolean_option(UE_AUTOTEST_TRACE   False "Activate UE autotest specific logs")
add_boolean_option(UE_DEBUG_TRACE      False "Activate UE debug trace")
add_boolean_option(UE_TIMING_TRACE     False "Activate UE timing trace")
add_boolean_option(DEBUG_CONSOLE       False "disables stdout/stderr buffering")

set (OCP_ITTI ${OPENAIR_DIR}/common/utils/ocp_itti)
add_library(ITTI
  ${OCP_ITTI}/intertask_interface.cpp
  ${OPENAIR_DIR}/common/utils/backtrace.c
  )
add_dependencies(ITTI rrc_flag)

##################################################
# ASN.1 grammar C code generation & dependencies #
##################################################
# A difficulty: asn1c generates C code of a un-predictable list of files
# so, generate the c from asn1c once at cmake run time
# So, if someone modify the asn.1 source file in such as way that it will create
# (so creating new asn.1 objects instead of modifying the object attributes)
# New C code source file, cmake must be re-run (instead of re-running make only)
#############
set(asn1_generated_dir ${OPENAIR_BIN_DIR})

set(protoc_call "${OPENAIR_CMAKE}/tools/generate_protobuf")
set(protobuf_generated_dir ${OPENAIR_BIN_DIR})

# RRC
######
set (RRC_ASN1_VERSION "Rel15")
make_version(LTE_RRC_VERSION 15 6 0)
set (RRC_GRAMMAR ${OPENAIR2_DIR}/RRC/LTE/MESSAGES/asn1c/ASN1_files/lte-rrc-15.6.0.asn1)

add_definitions(-DLTE_RRC_VERSION=${LTE_RRC_VERSION})
set (RRC_FULL_DIR ${asn1_generated_dir}/RRC_${RRC_ASN1_VERSION})

# Warning: if you modify ASN.1 source file to generate new C files, cmake should be re-run instead of make
if (${RU} STREQUAL 0)
  execute_process(COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh  "LTE_" " " "${RRC_FULL_DIR}" "${RRC_GRAMMAR}"
                  RESULT_VARIABLE ret)
endif (${RU} STREQUAL 0)

if (NOT ${ret} STREQUAL 0)
  message(FATAL_ERROR "${ret}: error")
endif (NOT ${ret} STREQUAL 0)
file(GLOB rrc_source ${RRC_FULL_DIR}/*.c)
add_custom_target (
  rrc_flag ALL
  ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh "LTE_" " " "${RRC_FULL_DIR}" "${RRC_GRAMMAR}"
  DEPENDS ${RRC_GRAMMAR}
  )

set_source_files_properties(${rrc_source} PROPERTIES COMPILE_FLAGS -w) # suppress warnings from generated code
add_library(RRC_LIB ${rrc_source}
    ${OPENAIR2_DIR}/RRC/LTE/MESSAGES/asn1_msg.c
    ${OPENAIR2_DIR}/RRC/LTE/MESSAGES/asn1_msg_NB_IoT.c)
add_dependencies(RRC_LIB rrc_flag)
include_directories ("${RRC_FULL_DIR}")


#NR RRC
#######
set (NR_RRC_ASN1_VERSION "NR_Rel16" )
make_version(NR_RRC_VERSION 16 4 1)
set (NR_RRC_GRAMMAR ${OPENAIR2_DIR}/RRC/NR/MESSAGES/asn1c/ASN1_files/nr-rrc-16.4.1.asn1)
add_definitions(-DNR_RRC_VERSION=${NR_RRC_VERSION})
set (NR_RRC_FULL_DIR ${asn1_generated_dir}/RRC_${NR_RRC_ASN1_VERSION})

# Warning: if you modify ASN.1 source file to generate new C files, cmake should be re-run instead of make
  execute_process(
    COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh "NR_" "-findirect-choice" "${NR_RRC_FULL_DIR}" "${NR_RRC_GRAMMAR}"
    RESULT_VARIABLE ret)
if (NOT ${ret} STREQUAL 0)
  message(FATAL_ERROR "${ret}: error")
endif ()
file(GLOB nr_rrc_source ${NR_RRC_FULL_DIR}/*.c)
file(GLOB nr_rrc_h ${NR_RRC_FULL_DIR}/*.h)

add_custom_target (
  nr_rrc_flag ALL
  ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh  "NR_" "-findirect-choice" "${NR_RRC_FULL_DIR}" "${NR_RRC_GRAMMAR}"
  DEPENDS ${NR_RRC_GRAMMAR}
  )

add_library(NR_RRC_LIB ${nr_rrc_h} ${nr_rrc_source}
    ${OPENAIR2_DIR}/RRC/NR/MESSAGES/asn1_msg.c
    )
add_dependencies(NR_RRC_LIB nr_rrc_flag)
include_directories ("${NR_RRC_FULL_DIR}")

# S1AP
# Same limitation as described in RRC: unknown generated file list
# so we generate it at cmake time
##############
set (S1AP_RELEASE R15)

set(S1AP_DIR ${OPENAIR3_DIR}/S1AP)
make_version(S1AP_VERSION 15 6 0)
set(S1AP_ASN_FILES "s1ap-15.6.0.asn1")

add_definitions(-DS1AP_VERSION=${S1AP_VERSION})
set(S1AP_ASN_DIR ${S1AP_DIR}/MESSAGES/ASN1/${S1AP_RELEASE})
set(S1AP_C_DIR ${asn1_generated_dir}/S1AP_${S1AP_RELEASE})

# Warning: if you modify ASN.1 source file to generate new C files, cmake should be re-run instead of make
if (${RU} STREQUAL 0)
  execute_process(
    COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh  "S1AP_" -fno-include-deps "${S1AP_C_DIR}" "${S1AP_ASN_DIR}/${S1AP_ASN_FILES}"
    RESULT_VARIABLE ret)
  if (NOT ${ret} STREQUAL 0)
    message(FATAL_ERROR "${ret}: error")
  endif (NOT ${ret} STREQUAL 0)
  file(GLOB S1AP_source ${S1AP_C_DIR}/*.c)
endif (${RU} STREQUAL 0)

  add_custom_target (
    s1ap_flag ALL
    ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh  "S1AP_" -fno-include-deps "${S1AP_C_DIR}" "${S1AP_ASN_DIR}/${S1AP_ASN_FILES}"
    DEPENDS  "${S1AP_ASN_DIR}/${S1AP_ASN_FILES}"
  )

add_library(S1AP_LIB
  ${S1AP_source}
  ${S1AP_DIR}/s1ap_common.c
  )
add_dependencies(S1AP_LIB rrc_flag s1ap_flag)

include_directories ("${S1AP_C_DIR}")
include_directories ("${S1AP_DIR}")

add_library(S1AP_ENB
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
add_dependencies(S1AP_ENB rrc_flag s1ap_flag)


# NGAP
# Same limitation as described in RRC: unknown generated file list
# so we generate it at cmake time
##############
set (NGAP_RELEASE R15)

set(NGAP_DIR ${OPENAIR3_DIR}/NGAP)
make_version(NGAP_VERSION 15 8 0)
set(NGAP_ASN_FILES "ngap-15.8.0.asn1")

add_definitions(-DNGAP_VERSION=${NGAP_VERSION})
set(NGAP_ASN_DIR ${NGAP_DIR}/MESSAGES/ASN1/ASN1_files)
set(NGAP_C_DIR ${asn1_generated_dir}/NGAP_${NGAP_RELEASE})

# Warning: if you modify ASN.1 source file to generate new C files, cmake should be re-run instead of make
execute_process(
  COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh "NGAP_" "-fno-include-deps -findirect-choice" "${NGAP_C_DIR}" "${NGAP_ASN_DIR}/${NGAP_ASN_FILES}"
  RESULT_VARIABLE ret)
if (NOT ${ret} STREQUAL 0)
  message(FATAL_ERROR "${ret}: error")
endif (NOT ${ret} STREQUAL 0)
file(GLOB NGAP_source ${NGAP_C_DIR}/*.c)

add_custom_target (
  ngap_flag ALL
  ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh  "NGAP_" "-fno-include-deps -findirect-choice" "${NGAP_C_DIR}" "${NGAP_ASN_DIR}/${NGAP_ASN_FILES}"
  DEPENDS  "${NGAP_ASN_DIR}/${NGAP_ASN_FILES}"
)

add_library(NGAP_LIB
  ${NGAP_source}
#  ${NGAP_DIR}/ngap_common.c
  )
add_dependencies(NGAP_LIB rrc_flag ngap_flag)

include_directories ("${NGAP_C_DIR}")
include_directories ("${NGAP_DIR}")

add_library(NGAP_GNB
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
add_dependencies(NGAP_GNB rrc_flag ngap_flag)

#M2AP
# Same limitation as described in RRC/S1AP: unknown generated file list
# so we generate it at cmake time
##############
add_list1_option(M2AP_RELEASE R14 "M2AP ASN.1 grammar version" R14)

set(M2AP_DIR ${OPENAIR2_DIR}/M2AP)
if (${M2AP_RELEASE} STREQUAL "R8")
  make_version(M2AP_VERSION 8 9 0)
  set(M2AP_ASN_FILES m2ap-8.9.0.asn1)
elseif (${M2AP_RELEASE} STREQUAL "R11")
  make_version(M2AP_VERSION 11 9 0)
  set(M2AP_ASN_FILES m2ap-11.9.0.asn1)
elseif (${M2AP_RELEASE} STREQUAL "R12")
  make_version(M2AP_VERSION 12 9 0)
  set(M2AP_ASN_FILES m2ap-12.9.0.asn1)
elseif (${M2AP_RELEASE} STREQUAL "R14")
  make_version(M2AP_VERSION 14 0 0)
  set(M2AP_ASN_FILES m2ap-14.0.0.asn1)
elseif (${M2AP_RELEASE} STREQUAL "R15")
  make_version(M2AP_VERSION 15 1 0)
  set(M2AP_ASN_FILES m2ap-15.1.0.asn1)
endif(${M2AP_RELEASE} STREQUAL "R8")
add_definitions(-DM2AP_VERSION=${M2AP_VERSION})
set(M2AP_ASN_DIR ${M2AP_DIR}/MESSAGES/ASN1/${M2AP_RELEASE})
set(M2AP_C_DIR ${asn1_generated_dir}/M2AP_${M2AP_RELEASE})

# Warning: if you modify ASN.1 source file to generate new C files, cmake should be re-run instead of make
if (${RU} STREQUAL 0)
  execute_process(
    COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh  "M2AP_" "-fno-include-deps -DEMIT_ASN_DEBUG=1" "${M2AP_C_DIR}" "${M2AP_ASN_DIR}/${M2AP_ASN_FILES}"
    RESULT_VARIABLE ret)
  if (NOT ${ret} STREQUAL 0)
    message(FATAL_ERROR "${ret}: error")
  endif (NOT ${ret} STREQUAL 0)
endif (${RU} STREQUAL 0)

file(GLOB M2AP_source ${M2AP_C_DIR}/*.c)

add_custom_target (
  m2_flag ALL
  COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh  "M2AP_" "-fno-include-deps -DEMIT_ASN_DEBUG=1" "${M2AP_C_DIR}" "${M2AP_ASN_DIR}/${M2AP_ASN_FILES}"
  DEPENDS ${M2AP_ASN_DIR}/${M2AP_ASN_FILES}
  )

add_library(M2AP_LIB
  ${M2AP_source}
  ${M2AP_DIR}/m2ap_common.c
  )
add_dependencies(M2AP_LIB rrc_flag m2_flag)

include_directories ("${M2AP_C_DIR}")
include_directories ("${M2AP_DIR}")

add_library(M2AP_ENB
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
add_dependencies(M2AP_ENB rrc_flag m2_flag)

#M3AP
# Same limitation as described in RRC/S1AP: unknown generated file list
# so we generate it at cmake time
##############
add_list1_option(M3AP_RELEASE R14 "M3AP ASN.1 grammar version" R14)

set(M3AP_DIR ${OPENAIR3_DIR}/M3AP)
if (${M3AP_RELEASE} STREQUAL "R8")
  make_version(M3AP_VERSION 8 9 0)
  set(M3AP_ASN_FILES m3ap-8.9.0.asn1)
elseif (${M3AP_RELEASE} STREQUAL "R11")
  make_version(M3AP_VERSION 11 9 0)
  set(M3AP_ASN_FILES m3ap-11.9.0.asn1)
elseif (${M3AP_RELEASE} STREQUAL "R12")
  make_version(M3AP_VERSION 12 9 0)
  set(M3AP_ASN_FILES m3ap-12.9.0.asn1)
elseif (${M3AP_RELEASE} STREQUAL "R14")
  make_version(M3AP_VERSION 14 0 0)
  set(M3AP_ASN_FILES m3ap-14.0.0.asn1)
elseif (${M3AP_RELEASE} STREQUAL "R15")
  make_version(M3AP_VERSION 15 1 0)
  set(M3AP_ASN_FILES m3ap-15.1.0.asn1)
endif(${M3AP_RELEASE} STREQUAL "R8")
add_definitions(-DM3AP_VERSION=${M3AP_VERSION})
set(M3AP_ASN_DIR ${M3AP_DIR}/MESSAGES/ASN1/${M3AP_RELEASE})
set(M3AP_C_DIR ${asn1_generated_dir}/M3AP_${M3AP_RELEASE})

# Warning: if you modify ASN.1 source file to generate new C files, cmake should be re-run instead of make
if (${RU} STREQUAL 0)
  execute_process(
    COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh  "M3AP_" -fno-include-deps "${M3AP_C_DIR}" "${M3AP_ASN_DIR}/${M3AP_ASN_FILES}"
    RESULT_VARIABLE ret)
  if (NOT ${ret} STREQUAL 0)
    message(FATAL_ERROR "${ret}: error")
  endif (NOT ${ret} STREQUAL 0)
endif (${RU} STREQUAL 0)

file(GLOB M3AP_source ${M3AP_C_DIR}/*.c)

add_custom_target (
  m3_flag ALL
  COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh  "M3AP_" -fno-include-deps "${M3AP_C_DIR}" "${M3AP_ASN_DIR}/${M3AP_ASN_FILES}"
  DEPENDS ${M3AP_ASN_DIR}/${M3AP_ASN_FILES}
  )

add_library(M3AP_LIB
  ${M3AP_source}
  ${M3AP_DIR}/m3ap_common.c
  )
add_dependencies(M3AP_LIB rrc_flag m3_flag)

include_directories ("${M3AP_C_DIR}")
include_directories ("${M3AP_DIR}")

add_library(M3AP_ENB
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
add_dependencies(M3AP_ENB rrc_flag m3_flag)

#X2AP
# Same limitation as described in RRC/S1AP: unknown generated file list
# so we generate it at cmake time
##############
set (X2AP_RELEASE R15)

set(X2AP_DIR ${OPENAIR2_DIR}/X2AP)
make_version(X2AP_VERSION 15 6 0)
set(X2AP_ASN_FILES x2ap-15.6.0.asn1)

add_definitions(-DX2AP_VERSION=${X2AP_VERSION})
set(X2AP_ASN_DIR ${X2AP_DIR}/MESSAGES/ASN1/${X2AP_RELEASE})
set(X2AP_C_DIR ${asn1_generated_dir}/X2AP_${X2AP_RELEASE})
# Warning: if you modify ASN.1 source file to generate new C files, cmake should be re-run instead of make
if (${RU} STREQUAL 0)
  execute_process(
    COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh "X2AP_" -fno-include-deps "${X2AP_C_DIR}" "${X2AP_ASN_DIR}/${X2AP_ASN_FILES}"
    RESULT_VARIABLE ret)
  if (NOT ${ret} STREQUAL 0)
    message(FATAL_ERROR "${ret}: error")
  endif (NOT ${ret} STREQUAL 0)
endif (${RU} STREQUAL 0)

file(GLOB X2AP_source ${X2AP_C_DIR}/*.c)

add_custom_target (
  x2_flag ALL
  COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh "X2AP_" -fno-include-deps "${X2AP_C_DIR}" "${X2AP_ASN_DIR}/${X2AP_ASN_FILES}"
  DEPENDS ${X2AP_ASN_DIR}/${X2AP_ASN_FILES}
  )

add_library(X2AP_LIB
  ${X2AP_source}
  ${X2AP_DIR}/x2ap_common.c
  )
add_dependencies(X2AP_LIB rrc_flag x2_flag)

include_directories ("${X2AP_C_DIR}")
include_directories ("${X2AP_DIR}")

add_library(X2AP_ENB
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
add_dependencies(X2AP_ENB X2AP_LIB rrc_flag x2_flag)


# F1AP
##############
set (F1AP_RELEASE R16)
add_list1_option(F1AP_RELEASE R16 "F1AP ASN.1 grammar version" R16)
set(F1AP_DIR ${OPENAIR2_DIR}/F1AP)
if (${F1AP_RELEASE} STREQUAL "R16")
  make_version(F1AP_VERSION 16 3 1)
  set (ASN1RELDIR R16.3.1)
endif(${F1AP_RELEASE} STREQUAL "R16")
add_definitions(-DF1AP_VERSION=${F1AP_VERSION})
set(F1AP_ASN_DIR ${F1AP_DIR}/MESSAGES/ASN1/${ASN1RELDIR})
set(F1AP_ASN_FILES
  ${F1AP_ASN_DIR}/F1AP-CommonDataTypes.asn
  ${F1AP_ASN_DIR}/F1AP-Constants.asn
  ${F1AP_ASN_DIR}/F1AP-PDU-Descriptions.asn
  ${F1AP_ASN_DIR}/F1AP-PDU-Contents.asn
  ${F1AP_ASN_DIR}/F1AP-IEs.asn
  ${F1AP_ASN_DIR}/F1AP-Containers.asn
  )

if (${RU} STREQUAL 0)
  set(F1AP_ASN_GENERATED_C_DIR ${asn1_generated_dir}/F1AP_${ASN1RELDIR})
  message("calling ASN1C_PREFIX=F1AP_ asn1c -gen-PER -no-gen-OER -fcompound-names -no-gen-example -findirect-choice -fno-include-deps -D ${F1AP_ASN_GENERATED_C_DIR} ${F1AP_ASN_FILES}")
  execute_process(
    COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh "F1AP_" "-findirect-choice -fno-include-deps" ${F1AP_ASN_GENERATED_C_DIR} ${F1AP_ASN_FILES}
    RESULT_VARIABLE ret
    )
  
  if (NOT ${ret} STREQUAL 0)
    message(FATAL_ERROR "asn1c: error")
  endif (NOT ${ret} STREQUAL 0)
  
  add_custom_target (
    f1_flag ALL
    COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh "F1AP_" "-findirect-choice -fno-include-deps" ${F1AP_ASN_GENERATED_C_DIR} ${F1AP_ASN_FILES}
    DEPENDS ${F1AP_ASN_FILES}
    )
  file(GLOB F1AP_ASN_GENERATED_C_FILES ${F1AP_ASN_GENERATED_C_DIR}/*.c)
  add_library(F1AP_LIB
    ${F1AP_ASN_GENERATED_C_FILES}
    )
  add_dependencies (F1AP_LIB  f1_flag)
  
  include_directories ("${F1AP_ASN_GENERATED_C_DIR}")
  include_directories ("${F1AP_DIR}")
  
  file(GLOB F1AP_C_FILES ${F1AP_DIR}/*.c)
  add_library(F1AP ${F1AP_C_FILES} )
  
endif (${RU} STREQUAL 0)

# Hardware dependant options
###################################
add_list1_option(NB_ANTENNAS_RX "4" "Number of antennas in reception" "1" "2" "4")
add_list1_option(NB_ANTENNAS_TX "4" "Number of antennas in transmission" "1" "2" "4")

add_list2_option(RF_BOARD "EXMIMO" "RF head type" "None" "OAI_USRP" "OAI_BLADERF" "OAI_LMSSDR" "OAI_SIMU")

add_list2_option(TRANSP_PRO "None" "Transport protocol type" "None" "ETHERNET")
#NOKIA config enhancement
set (CONFIG_ROOTDIR
  ${OPENAIR_DIR}/common/config
  )
set (CONFIG_SOURCES
  ${CONFIG_ROOTDIR}/config_load_configmodule.c
  ${CONFIG_ROOTDIR}/config_userapi.c
  ${CONFIG_ROOTDIR}/config_cmdline.c
  )
set (CONFIG_LIBCONFIG_SOURCES
  ${CONFIG_ROOTDIR}/libconfig/config_libconfig.c
  )
add_library(CONFIG_LIB ${CONFIG_SOURCES})
add_library(params_libconfig MODULE ${CONFIG_LIBCONFIG_SOURCES} )
target_link_libraries(params_libconfig config)
# shared library loader
set (SHLIB_LOADER_SOURCES
  ${OPENAIR_DIR}/common/utils/load_module_shlib.c
)
# include RF devices / transport protocols library modules
######################################################################

include_directories("${OPENAIR_TARGETS}/ARCH/USRP/USERSPACE/LIB/")
set(HWLIB_USRP_SOURCE
  ${OPENAIR_TARGETS}/ARCH/USRP/USERSPACE/LIB/usrp_lib.cpp
  )
add_library(oai_usrpdevif MODULE ${HWLIB_USRP_SOURCE} )
target_link_libraries(oai_usrpdevif uhd)

include_directories("${OPENAIR_TARGETS}/ARCH/BLADERF/USERSPACE/LIB/")
set(HWLIB_BLADERF_SOURCE
  ${OPENAIR_TARGETS}/ARCH/BLADERF/USERSPACE/LIB/bladerf_lib.c
  )
add_library(oai_bladerfdevif MODULE ${HWLIB_BLADERF_SOURCE} )
target_link_libraries(oai_bladerfdevif bladeRF)

include_directories("${OPENAIR_TARGETS}/ARCH/LMSSDR/USERSPACE/LIB/")

set(HWLIB_LMSSDR_SOURCE
  ${OPENAIR_TARGETS}/ARCH/LMSSDR/USERSPACE/LIB/lms_lib.cpp
  )
add_library(oai_lmssdrdevif MODULE ${HWLIB_LMSSDR_SOURCE} )
target_include_directories(oai_lmssdrdevif PRIVATE /usr/local/include/lime)
target_link_libraries(oai_lmssdrdevif LimeSuite )


include_directories("${OPENAIR_TARGETS}/ARCH/ETHERNET/USERSPACE/LIB/")
set(TPLIB_ETHERNET_SOURCE
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.c
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/USERSPACE/LIB/eth_udp.c
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/USERSPACE/LIB/eth_raw.c
  )
add_library(oai_eth_transpro MODULE ${TPLIB_ETHERNET_SOURCE} )

include_directories("${OPENAIR_TARGETS}/ARCH/AW2SORI/")
link_directories("/usr/local/lib")
set(HWLIB_AW2SORI_SOURCE
  ${OPENAIR_TARGETS}/ARCH/AW2SORI/oaiori.c
  )
add_library(aw2sori_transpro MODULE ${HWLIB_AW2SORI_SOURCE})
target_compile_options(aw2sori_transpro PRIVATE -shared -fPIC -msse4 -g -ggdb -DLITE_COMPILATION)
target_link_libraries(aw2sori_transpro libori.so)

include_directories("${OPENAIR_TARGETS}/ARCH/IRIS/USERSPACE/LIB/")
set(option_HWIRISLIB_lib "-l SoapySDR")
set(HWLIB_IRIS_SOURCE
  ${OPENAIR_TARGETS}/ARCH/IRIS/USERSPACE/LIB/iris_lib.cpp
  )
add_library(oai_irisdevif MODULE ${HWLIB_IRIS_SOURCE})
target_include_directories(oai_irisdevif PRIVATE /usr/local/lib/SoapySDR/modules0.7/)
target_link_libraries(oai_irisdevif SoapySDR)

# TCP bridge libraries
######################################################################

# this one is for internal use at Eurecom and is not documented
set(HWLIB_TCP_BRIDGE_SOURCE
  ${OPENAIR_TARGETS}/ARCH/tcp_bridge/tcp_bridge.c
  )
add_library(tcp_bridge MODULE ${HWLIB_TCP_BRIDGE_SOURCE} )

#get_target_property(tcp_bridge_cflags tcp_bridge COMPILE_FLAGS)
#set_target_properties(tcp_bridge PROPERTIES COMPILE_FLAGS "${tcp_bridge_cflags} -fvisibility=hidden")
set_target_properties(tcp_bridge PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")

# this one is to connect OAI eNB and OAI UE in the basic simulator
# see targets/ARCH/tcp_bridge/README.tcp_bridge_oai for usage
set(HWLIB_TCP_BRIDGE_OAI_SOURCE
  ${OPENAIR_TARGETS}/ARCH/tcp_bridge/tcp_bridge_oai.c
  )
add_library(tcp_bridge_oai MODULE ${HWLIB_TCP_BRIDGE_OAI_SOURCE} )
set_target_properties(tcp_bridge_oai PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")

# Benetel 4G library
######################################################################


include_directories ("/usr/include/dpdk")

set(HWLIB_BENETEL_4G_SOURCE
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/benetel/4g/benetel.c
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/benetel/4g/shared_buffers.c
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/benetel/4g/low.c
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/benetel/4g/low_dpdk.c
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/benetel/4g/dpdk_driver.c
  )
add_library(benetel_4g MODULE ${HWLIB_BENETEL_4G_SOURCE} )

set_target_properties(benetel_4g PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -march=native -I$ENV{RTE_SDK}/$ENV{RTE_TARGET}/include")

SET(DPDK_LIBS "-Wl,-rpath,$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib -Wl,--whole-archive -L$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib -ldpdk -Wl,--no-whole-archive")
TARGET_LINK_LIBRARIES(benetel_4g ${DPDK_LIBS})
TARGET_LINK_LIBRARIES(benetel_4g pthread dl rt m numa)

# Benetel 5G library
######################################################################

set(HWLIB_BENETEL_5G_SOURCE
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/benetel/5g/benetel.c
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/benetel/5g/shared_buffers.c
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/benetel/5g/low.c
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/benetel/5g/low_dpdk.c
  ${OPENAIR_TARGETS}/ARCH/ETHERNET/benetel/5g/dpdk_driver.c
  )
add_library(benetel_5g MODULE ${HWLIB_BENETEL_5G_SOURCE} )

set_target_properties(benetel_5g PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -march=native -I$ENV{RTE_SDK}/$ENV{RTE_TARGET}/include")

SET(DPDK_LIBS "-Wl,-rpath,$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib -Wl,--whole-archive -L$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib -ldpdk -Wl,--no-whole-archive")
TARGET_LINK_LIBRARIES(benetel_5g ${DPDK_LIBS})
TARGET_LINK_LIBRARIES(benetel_5g pthread dl rt m numa)

##########################################################

include_directories ("${OPENAIR_TARGETS}/ARCH/COMMON")

Message("DEADLINE_SCHEDULER flag  is ${DEADLINE_SCHEDULER}")
Message("CPU_Affinity flag is ${CPU_AFFINITY}")

##############################################################
#    ???!!! TO BE DOCUMENTED OPTIONS !!!???
##############################################################

add_boolean_option(OAI_NW_DRIVER_TYPE_ETHERNET False "????")
add_boolean_option(DEADLINE_SCHEDULER False "Use the Linux scheduler SCHED_DEADLINE: kernel >= 3.14")
add_boolean_option(CPU_AFFINITY False "Enable CPU Affinity of threads (only valid without deadline scheduler). It is enabled only with >2 CPUs")
add_boolean_option(NAS_NETLINK False "useless ??? Must be True to compile nasmesh driver without rtai ????")
add_boolean_option(OAI_NW_DRIVER_USE_NETLINK True "????")

add_boolean_option(MESSAGE_CHART_GENERATOR False         "For generating sequence diagrams")
add_boolean_option(MESSAGE_CHART_GENERATOR_RLC_MAC False "trace RLC-MAC exchanges in sequence diagrams")
add_boolean_option(MESSAGE_CHART_GENERATOR_PHY     False "trace some PHY exchanges in sequence diagrams")

add_boolean_option(UE_EXPANSION             False         "enable UE_EXPANSION with max 256 UE")
add_boolean_option(PHY_TX_THREAD            False         "enable UE_EXPANSION with max 256 UE")
add_boolean_option(PRE_SCD_THREAD           False         "enable UE_EXPANSION with max 256 UE")
add_boolean_option(UESIM_EXPANSION          False         "enable UESIM_EXPANSION with max 256 UE")
add_boolean_option(ITTI_SIM                 False         "enable itti simulator")

########################
# Include order
##########################
add_boolean_option(ENB_MODE True "Swap the include directories between openair2 and openair3" )

##########################
# SCHEDULING/REAL-TIME/PERF options
##########################
add_boolean_option(ENABLE_USE_CPU_EXECUTION_TIME False "Add data in vcd traces: disable it if perf issues")
add_boolean_option(ENABLE_VCD              False  "always true now, time measurements of proc calls and var displays")
add_boolean_option(ENABLE_VCD_FIFO         False  "time measurements of proc calls and var displays sent to FIFO (one more thread)")
add_boolean_option(LINUX                   False "used in weird memcpy() in pdcp.c ???")
add_boolean_option(LINUX_LIST              False "used only in lists.c: either use OAI implementation of lists or Linux one (should be True, but it is False")

##########################
# PHY options
##########################
add_boolean_option(DRIVER2013              True "only relevant for EXMIMO")
add_boolean_option(EXMIMO_IOT              True "????")
add_boolean_option(LOCALIZATION            False "???")
add_integer_option(MAX_NUM_CCs             1     "????")
add_boolean_option(PHYSIM                  False  "for L1 simulators (dlsim, ulsim, ...)")
add_boolean_option(SMBV                    False "Rohde&Schwarz SMBV100A vector signal generator")
add_boolean_option(DEBUG_PHY               False "Enable PHY layer debugging options")
add_boolean_option(DEBUG_PHY_PROC          False "Enable debugging of PHY layer procedures")
add_boolean_option(DEBUG_DLSCH             False "Enable debugging of DLSCH physical layer channel")
add_boolean_option(MEX                     False "Enabling compilation with mex")

##########################
# NAS LAYER OPTIONS
##########################
add_boolean_option(ENABLE_NAS_UE_LOGGING   True  "????")
add_boolean_option(NAS_BUILT_IN_UE         True  "UE NAS layer present in this executable")
add_boolean_option(NAS_UE                  True  "NAS UE INSTANCE (<> NAS_MME)")


##########################
# ACCESS STRATUM LAYER2 OPTIONS
##########################
add_boolean_option(JUMBO_FRAME             True  "ENABLE LARGE SDU in ACCESS STRATUM (larger than common MTU)")

##########################
# RLC LAYER OPTIONS
##########################
add_boolean_option(TRACE_RLC_PAYLOAD       False "Fatal assert in this case")
add_boolean_option(RLC_STOP_ON_LOST_PDU    False "Fatal assert in this case")

add_boolean_option(TRACE_RLC_MUTEX         True  "TRACE for RLC, possible problem in thread scheduling")
add_boolean_option(TRACE_RLC_AM_BO         False "TRACE for RLC AM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_AM_FREE_SDU   False "TRACE for RLC AM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_AM_HOLE       False "TRACE for RLC AM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_AM_PDU        False "TRACE for RLC AM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_AM_RESEGMENT  False "TRACE for RLC AM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_AM_RX         False "TRACE for RLC AM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_AM_RX_DECODE  False "TRACE for RLC AM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_AM_TX         False "TRACE for RLC AM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_AM_TX_STATUS  False "TRACE for RLC AM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_AM_STATUS_CREATION   False "TRACE for RLC AM, TO BE CHANGED IN A MORE GENERAL FLAG")

add_boolean_option(STOP_ON_IP_TRAFFIC_OVERLOAD      False "")
add_boolean_option(TRACE_RLC_UM_DAR        False "TRACE for RLC UM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_UM_DISPLAY_ASCII_DATA  False "TRACE for RLC UM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_UM_PDU        False "TRACE for RLC UM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_UM_RX         False "TRACE for RLC UM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_UM_SEGMENT    False "TRACE for RLC UM, TO BE CHANGED IN A MORE GENERAL FLAG")
add_boolean_option(TRACE_RLC_UM_TX_STATUS  False "TRACE for RLC UM, TO BE CHANGED IN A MORE GENERAL FLAG")


##########################
# PDCP LAYER OPTIONS
##########################
#add_boolean_option(LINK_ENB_PDCP_TO_IP_DRIVER  False "For eNB, PDCP communicate with a IP driver")
#add_boolean_option(LINK_ENB_PDCP_TO_GTPV1U     True  "For eNB, PDCP communicate with GTP-U protocol (eNB<->S-GW)")

##########################
# RRC LAYER OPTIONS
##########################
add_boolean_option(RRC_DEFAULT_RAB_IS_AM       True   "set the RLC mode to AM for the default bearer, otherwise it is UM.")


##########################
# S1AP LAYER OPTIONS
##########################
# none

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
if(ENB_MODE)
  include_directories("${OPENAIR2_DIR}/COMMON")
  include_directories("${OPENAIR2_DIR}/UTIL")
  include_directories("${OPENAIR2_DIR}/UTIL/LOG")
  include_directories("${OPENAIR3_DIR}/COMMON")
  include_directories("${OPENAIR3_DIR}/UTILS")
else()
  include_directories("${OPENAIR3_DIR}/COMMON")
  include_directories("${OPENAIR3_DIR}/UTILS")
  include_directories("${OPENAIR2_DIR}/COMMON")
  include_directories("${OPENAIR2_DIR}/UTIL")
  include_directories("${OPENAIR2_DIR}/UTIL/LOG")
endif()
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
include_directories("${OPENAIR2_DIR}/LAYER2/RLC/AM_v9.3.0")
include_directories("${OPENAIR2_DIR}/LAYER2/RLC/UM_v9.3.0")
include_directories("${OPENAIR2_DIR}/LAYER2/RLC/TM_v9.3.0")
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
include_directories("${OPENAIR3_DIR}/UDP")
include_directories("${OPENAIR3_DIR}/GTPV1-U")
include_directories("${OPENAIR3_DIR}/M3AP")
include_directories("${OPENAIR3_DIR}/MME_APP")
include_directories("${OPENAIR_DIR}/targets/COMMON")
include_directories("${OPENAIR_DIR}/targets/ARCH/COMMON")
include_directories("${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/PHY")
include_directories("${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/MAC")
include_directories("${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/RRC")
include_directories("${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/PDCP")
include_directories("${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/S1AP")
include_directories("${OPENAIR2_DIR}/UTIL/OSA")
include_directories("${OPENAIR2_DIR}/UTIL/LFDS/liblfds6.1.1/liblfds611/inc")
include_directories("${OPENAIR2_DIR}/UTIL/LFDS/liblfds7.0.0/liblfds700/inc")
include_directories("${OPENAIR2_DIR}/LAYER2/PROTO_AGENT")
include_directories("${OPENAIR2_DIR}/UTIL/MEM")
include_directories("${OPENAIR2_DIR}/UTIL/LISTS")
include_directories("${OPENAIR2_DIR}/UTIL/FIFO")
include_directories("${OPENAIR2_DIR}/UTIL/OCG")
include_directories("${OPENAIR2_DIR}/UTIL/MATH")
include_directories("${OPENAIR2_DIR}/UTIL/TIMER")
include_directories("${OPENAIR2_DIR}/UTIL/OMG")
include_directories("${OPENAIR2_DIR}/UTIL/OTG")
include_directories("${OPENAIR2_DIR}/UTIL/CLI")
include_directories("${OPENAIR2_DIR}/UTIL/OPT")
include_directories("${OPENAIR2_DIR}/UTIL/OMV")
include_directories("${OPENAIR2_DIR}/RRC/LTE/MESSAGES")
include_directories("${OPENAIR3_DIR}/GTPV1-U/nw-gtpv1u/shared")
include_directories("${OPENAIR3_DIR}/GTPV1-U/nw-gtpv1u/include")
include_directories("${OPENAIR_DIR}")

# Utilities Library
################
# set the version of protobuf messages, V3 not supported yet
add_list1_option(FLPT_VERSION V2 "FLPT MSG  protobuf  grammar version" V2 V3)

if (${FLPT_VERSION} STREQUAL "V2")
  set (FLPTDIR V2)
elseif (${FLPT_VERSION} STREQUAL "V3")
  set (FLPTDIR V3)
endif(${FLPT_VERSION} STREQUAL "V2")

set(FLPT_MSG_DIR ${OPENAIR2_DIR}/ENB_APP/MESSAGES/${FLPTDIR} )
set(FLPT_MSG_FILES
  ${FLPT_MSG_DIR}/header.proto
  ${FLPT_MSG_DIR}/flexran.proto
  ${FLPT_MSG_DIR}/stats_common.proto
  ${FLPT_MSG_DIR}/stats_messages.proto
  ${FLPT_MSG_DIR}/time_common.proto
  ${FLPT_MSG_DIR}/controller_commands.proto
  ${FLPT_MSG_DIR}/mac_primitives.proto
  ${FLPT_MSG_DIR}/config_messages.proto
  ${FLPT_MSG_DIR}/config_common.proto
  ${FLPT_MSG_DIR}/control_delegation.proto
  )

set(FLPT_C_DIR ${protobuf_generated_dir}/FLPT_${FLPTDIR})
#message("calling protoc_call=${protoc_call} FLPT_C_DIR=${FLPT_C_DIR} FLPT_MSG_FILES=${FLPT_MSG_FILES}")
execute_process(COMMAND ${protoc_call} ${FLPT_C_DIR} ${FLPT_MSG_DIR} ${FLPT_MSG_FILES})
file(GLOB FLPT_source ${FLPT_C_DIR}/*.c)
set(FLPT_OAI_generated
  ${FLPT_C_DIR}/header.pb-c.c
  ${FLPT_C_DIR}/flexran.pb-c.c
  ${FLPT_C_DIR}/stats_common.pb-c.c
  ${FLPT_C_DIR}/stats_messages.pb-c.c
  ${FLPT_C_DIR}/time_common.pb-c.c
  ${FLPT_C_DIR}/controller_commands.pb-c.c
  ${FLPT_C_DIR}/mac_primitives.pb-c.c
  ${FLPT_C_DIR}/config_messages.pb-c.c
  ${FLPT_C_DIR}/config_common.pb-c.c
  ${FLPT_C_DIR}/control_delegation.pb-c.c
  )

file(GLOB flpt_h ${FLPT_C_DIR}/*.h)
set(flpt_h ${flpt_h} )

add_library(FLPT_MSG
  ${FLPT_OAI_generated}
  ${FLPT_source}
  )
set(FLPT_MSG_LIB FLPT_MSG)
#message("prpt c dir is : ${FLPT_C_DIR}")
include_directories (${FLPT_C_DIR})

add_library(ASYNC_IF
  ${OPENAIR2_DIR}/UTIL/ASYNC_IF/socket_link.c
  ${OPENAIR2_DIR}/UTIL/ASYNC_IF/link_manager.c
  ${OPENAIR2_DIR}/UTIL/ASYNC_IF/message_queue.c
  ${OPENAIR2_DIR}/UTIL/ASYNC_IF/ringbuffer_queue.c
  )
set(ASYNC_IF_LIB ASYNC_IF)
include_directories(${OPENAIR2_DIR}/UTIL/ASYNC_IF)

add_library(FLEXRAN_AGENT
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_handler.c
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_common.c
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_ran_api.c
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_timer.c
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_common_internal.c
  ${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/PHY/flexran_agent_phy.c
  ${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/MAC/flexran_agent_mac.c
  ${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/RRC/flexran_agent_rrc.c
  ${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/RRC/flexran_agent_rrc_internal.c
  ${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/PDCP/flexran_agent_pdcp.c
  ${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/S1AP/flexran_agent_s1ap.c
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent.c
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_task_manager.c
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_net_comm.c
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_async.c
  ${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/MAC/flexran_agent_mac_internal.c
  ${OPENAIR2_DIR}/ENB_APP/CONTROL_MODULES/MAC/flexran_agent_mac_slice_verification.c
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_app.c
  )
add_dependencies(FLEXRAN_AGENT rrc_flag)
set(FLEXRAN_AGENT_LIB FLEXRAN_AGENT)
add_library(flapp_sample SHARED
  ${OPENAIR2_DIR}/ENB_APP/flexran_apps/sample.c
)
set_target_properties(flapp_sample PROPERTIES C_VISIBILITY_PRESET hidden)
add_library(flapp_imsi SHARED
  ${OPENAIR2_DIR}/ENB_APP/flexran_apps/imsi.c
)
set_target_properties(flapp_imsi PROPERTIES C_VISIBILITY_PRESET hidden)
add_custom_target(flapp_all DEPENDS
  flapp_sample
  flapp_imsi
)
#include_directories(${OPENAIR2_DIR}/ENB_APP)

set(PROTOBUF_LIB "protobuf-c")

FIND_PATH(LIBYAML_INCLUDE_DIR NAMES yaml.h)
FIND_LIBRARY(LIBYAML_LIBRARIES NAMES yaml libyaml)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Yaml DEFAULT_MSG LIBYAML_LIBRARIES LIBYAML_INCLUDE_DIR)
MARK_AS_ADVANCED(LIBYAML_INCLUDE_DIR LIBYAML_LIBRARIES)

#set(PROTOBUF_LIB "protobuf") #for Cpp

# set the version of protobuf messages, V3 not supported yet
add_list1_option(FSPT_VERSION V2 "FSPT MSG  protobuf  grammar version" V2 V3)

if (${FSPT_VERSION} STREQUAL "V2")
  set (FSPTDIR V2)
elseif (${FSPT_VERSION} STREQUAL "V3")
  set (FSPTDIR V3)
endif(${FSPT_VERSION} STREQUAL "V2")

set(FSPT_MSG_DIR ${OPENAIR_DIR}/targets/COMMON/MESSAGES/${FSPTDIR} )
set(FSPT_MSG_FILES
  ${FSPT_MSG_DIR}/flexsplit.proto
  )

set(FSPT_C_DIR ${protobuf_generated_dir}/FSPT_${FSPTDIR})
message("calling protoc_call=${protoc_call} FSPT_C_DIR=${FSPT_C_DIR} FSPT_MSG_DIR=${FSPT_MSG_DIR} FSPT_MSG_FILES=${FSPT_MSG_FILES}")
execute_process(COMMAND ${protoc_call} ${FSPT_C_DIR} ${FSPT_MSG_DIR} ${FSPT_MSG_FILES})
file(GLOB FSPT_source ${FSPT_C_DIR}/*.c)
set(FSPT_OAI_generated
  ${FSPT_C_DIR}/flexsplit.pb-c.c
  )

file(GLOB fspt_h ${FSPT_C_DIR}/*.h)
set(fspt_h ${fspt_h} )

add_library(FSPT_MSG
  ${FSPT_OAI_generated}
  ${FSPT_source}
  )
set(FSPT_MSG_LIB FSPT_MSG)
message("fspt c dir is : ${FSPT_C_DIR}")
include_directories (${FSPT_C_DIR})

#  add_library(ASYNC_IF
#    ${OPENAIR2_DIR}/UTIL/ASYNC_IF/socket_link.c
#    ${OPENAIR2_DIR}/UTIL/ASYNC_IF/link_manager.c
#    ${OPENAIR2_DIR}/UTIL/ASYNC_IF/message_queue.c
#    ${OPENAIR2_DIR}/UTIL/ASYNC_IF/ringbuffer_queue.c
#    )
#  set(ASYNC_IF_LIB ASYNC_IF)
#  include_directories(${OPENAIR2_DIR}/UTIL/ASYNC_IF)

add_library(PROTO_AGENT
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_handler.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_common.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_net_comm.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_async.c
  )
set(PROTO_AGENT_LIB PROTO_AGENT)
include_directories(${OPENAIR2_DIR}/LAYER2/PROTO_AGENT)



set(PROTOBUF_LIB "protobuf-c")

#set(PROTOBUF_LIB "protobuf") #for Cpp

add_library(HASHTABLE
  ${OPENAIR_DIR}/common/utils/hashtable/hashtable.c
  ${OPENAIR_DIR}/common/utils/hashtable/obj_hashtable.c
)
include_directories(${OPENAIR_DIR}/common/utils/hashtable)

add_library(msc MODULE ${OPENAIR_DIR}/common/utils/msc/msc.c )
target_link_libraries (msc LFDS)

include_directories(${OPENAIR_DIR}/common/utils/msc)

set(UTIL_SRC
  #  ${OPENAIR2_DIR}/UTIL/CLI/cli.c
  #  ${OPENAIR2_DIR}/UTIL/CLI/cli_cmd.c
  #  ${OPENAIR2_DIR}/UTIL/CLI/cli_server.c
  ${OPENAIR2_DIR}/UTIL/FIFO/pad_list.c
  ${OPENAIR2_DIR}/UTIL/LISTS/list.c
  ${OPENAIR2_DIR}/UTIL/LISTS/list2.c
  ${OPENAIR_DIR}/common/utils/LOG/log.c
  ${OPENAIR_DIR}/common/utils/LOG/vcd_signal_dumper.c
  ${OPENAIR2_DIR}/UTIL/MATH/oml.c
  #  ${OPENAIR2_DIR}/UTIL/MEM/mem_block.c
  #  ${OPENAIR2_DIR}/UTIL/OCG/OCG.c
  #  ${OPENAIR2_DIR}/UTIL/OCG/OCG_create_dir.c
  #  ${OPENAIR2_DIR}/UTIL/OCG/OCG_detect_file.c
  #  ${OPENAIR2_DIR}/UTIL/OCG/OCG_generate_report.c
  #  ${OPENAIR2_DIR}/UTIL/OCG/OCG_parse_filename.c
  #  ${OPENAIR2_DIR}/UTIL/OCG/OCG_parse_XML.c
  #  ${OPENAIR2_DIR}/UTIL/OCG/OCG_save_XML.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/common.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/grid.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/job.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/mobility_parser.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/omg.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/omg_hashtable.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/rwalk.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/rwp.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/static.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/steadystaterwp.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/trace.c
  #  ${OPENAIR2_DIR}/UTIL/OMG/trace_hashtable.c
  ${OPENAIR2_DIR}/UTIL/OPT/probe.c
  #  ${OPENAIR2_DIR}/UTIL/OTG/otg_tx.c
  #  ${OPENAIR2_DIR}/UTIL/OTG/otg.c
  #  ${OPENAIR2_DIR}/UTIL/OTG/otg_kpi.c
  #  ${OPENAIR2_DIR}/UTIL/OTG/otg_models.c
  #  ${OPENAIR2_DIR}/UTIL/OTG/otg_form.c
  #  ${OPENAIR2_DIR}/UTIL/OTG/otg_rx.c
  )
add_library(UTIL ${UTIL_SRC})
add_dependencies(UTIL rrc_flag)

#set(OMG_SUMO_SRC
#  ${OPENAIR2_DIR}/UTIL/OMG/client_traci_OMG.c
#  ${OPENAIR2_DIR}/UTIL/OMG/id_manager.c
#  ${OPENAIR2_DIR}/UTIL/OMG/sumo.c
#  ${OPENAIR2_DIR}/UTIL/OMG/socket_traci_OMG.c
#  ${OPENAIR2_DIR}/UTIL/OMG/storage_traci_OMG.c
#  )
#add_library(OMG_SUMO ${OMG_SUMO_SRC})

set(SECU_OSA_SRC
  ${OPENAIR2_DIR}/UTIL/OSA/osa_key_deriver.c
  ${OPENAIR2_DIR}/UTIL/OSA/osa_rijndael.c
  ${OPENAIR2_DIR}/UTIL/OSA/osa_snow3g.c
  ${OPENAIR2_DIR}/UTIL/OSA/osa_stream_eea.c
  ${OPENAIR2_DIR}/UTIL/OSA/osa_stream_eia.c
  )
add_library(SECU_OSA ${SECU_OSA_SRC})
target_link_libraries(SECU_OSA ${NETTLE_LIBRARIES})

set(SECU_CN_SRC
  ${OPENAIR3_DIR}/SECU/kdf.c
  ${OPENAIR3_DIR}/SECU/rijndael.c
  ${OPENAIR3_DIR}/SECU/snow3g.c
  ${OPENAIR3_DIR}/SECU/key_nas_deriver.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eea1.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eia1.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eea2.c
  ${OPENAIR3_DIR}/SECU/nas_stream_eia2.c
  )
add_library(SECU_CN ${SECU_CN_SRC})

target_link_libraries(SECU_CN ${NETTLE_LIBRARIES})

# Physical Channel Procedures Scheduling
################################"
set(SCHED_SRC
  ${OPENAIR1_DIR}/SCHED/fapi_l1.c
  ${OPENAIR1_DIR}/SCHED/phy_procedures_lte_eNb.c
  ${OPENAIR1_DIR}/SCHED/phy_procedures_lte_common.c
)
add_library(SCHED_LIB ${SCHED_SRC})
add_dependencies(SCHED_LIB rrc_flag)

set(SCHED_NR_SRC
  ${OPENAIR1_DIR}/SCHED_NR/fapi_nr_l1.c
  ${OPENAIR1_DIR}/SCHED_NR/phy_procedures_nr_common.c
  ${OPENAIR1_DIR}/SCHED_NR/phy_procedures_nr_gNB.c
  ${OPENAIR1_DIR}/SCHED_NR/nr_prach_procedures.c
  ${OPENAIR1_DIR}/SCHED_NR/phy_frame_config_nr.c
)
add_library(SCHED_NR_LIB ${SCHED_NR_SRC})

set(SCHED_SRC_RU
  ${OPENAIR1_DIR}/SCHED/ru_procedures.c
  ${OPENAIR1_DIR}/SCHED_NR/nr_ru_procedures.c
  ${OPENAIR1_DIR}/SCHED/prach_procedures.c
)
add_library(SCHED_RU_LIB ${SCHED_SRC_RU})
add_dependencies(SCHED_RU_LIB rrc_flag)

set(SCHED_SRC_UE
  ${OPENAIR1_DIR}/SCHED_UE/phy_procedures_lte_ue.c
  ${OPENAIR1_DIR}/SCHED/phy_procedures_lte_common.c
  ${OPENAIR1_DIR}/SCHED_UE/pucch_pc.c
  ${OPENAIR1_DIR}/SCHED_UE/pusch_pc.c
  ${OPENAIR1_DIR}/SCHED_UE/srs_pc.c
)
add_library(SCHED_UE_LIB ${SCHED_SRC_UE})
add_dependencies(SCHED_UE_LIB rrc_flag)

set(SCHED_SRC_NR_UE
  ${OPENAIR1_DIR}/SCHED_NR_UE/phy_procedures_nr_ue.c
  ${OPENAIR1_DIR}/SCHED_NR/phy_procedures_nr_common.c
  ${OPENAIR1_DIR}/SCHED_NR_UE/fapi_nr_ue_l1.c
  ${OPENAIR1_DIR}/SCHED_NR_UE/phy_frame_config_nr_ue.c
  ${OPENAIR1_DIR}/SCHED_NR_UE/harq_nr.c
  ${OPENAIR1_DIR}/SCHED_NR_UE/pucch_uci_ue_nr.c
)
add_library(SCHED_NR_UE_LIB ${SCHED_SRC_NR_UE})


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

include_directories(${NFAPI_DIR}/pnf/public_inc)
include_directories(${NFAPI_DIR}/pnf/inc)

set(NFAPI_VNF_SRC
  ${NFAPI_DIR}/vnf/src/vnf.c
  ${NFAPI_DIR}/vnf/src/vnf_interface.c
  ${NFAPI_DIR}/vnf/src/vnf_p7.c
  ${NFAPI_DIR}/vnf/src/vnf_p7_interface.c
)
add_library(NFAPI_VNF_LIB ${NFAPI_VNF_SRC})

include_directories(${NFAPI_DIR}/vnf/public_inc)
include_directories(${NFAPI_DIR}/vnf/inc)

# nFAPI user defined code
#############################
set(NFAPI_USER_SRC
  ${NFAPI_USER_DIR}/nfapi.c
  ${NFAPI_USER_DIR}/nfapi_pnf.c
  ${NFAPI_USER_DIR}/nfapi_vnf.c
)
add_library(NFAPI_USER_LIB ${NFAPI_USER_SRC})
add_dependencies(NFAPI_USER_LIB rrc_flag)
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

set(PHY_LDPC_OPTIM_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.c
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim.c
)
set(PHY_LDPC_OPTIM8SEG_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.c
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim8seg.c
)
set(PHY_LDPC_OPTIM8SEGMULTI_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.c
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim8segmulti.c
)
set(PHY_LDPC_CUDA_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder_LYC/nrLDPC_decoder_LYC.cu
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim8segmulti.c
)

set(PHY_LDPC_CL_SRC
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder_CL.c
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim8segmulti.c
)

set(PHY_NR_CODINGIF
  ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_load.c;
)

add_library(ldpc_orig MODULE ${PHY_LDPC_ORIG_SRC} )
add_library(ldpc_optim MODULE ${PHY_LDPC_OPTIM_SRC} )
add_library(ldpc_optim8seg MODULE ${PHY_LDPC_OPTIM8SEG_SRC} )
add_library(ldpc_cl MODULE ${PHY_LDPC_CL_SRC} )
target_link_libraries(ldpc_cl OpenCL)

if (CUDA_FOUND)
  cuda_add_library(ldpc_cuda MODULE ${PHY_LDPC_CUDA_SRC} )
  set_target_properties(ldpc_cuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
#  CUDA_ADD_CUFFT_TO_TARGET(ldpc_cuda)
endif (CUDA_FOUND)

add_library(ldpc MODULE ${PHY_LDPC_OPTIM8SEGMULTI_SRC} )


add_library(coding MODULE ${PHY_TURBOSRC} )

add_library(dfts MODULE ${OPENAIR1_DIR}/PHY/TOOLS/oai_dfts.c )


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
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/lte_gold_generic.c
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
  ${OPENAIR1_DIR}/PHY/TOOLS/time_meas.c
  ${OPENAIR1_DIR}/PHY/TOOLS/lut.c
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
  ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/if5_tools.c
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
  )

  set(PHY_NR_SRC
  ${OPENAIR1_DIR}/PHY/INIT/nr_init.c
  ${OPENAIR1_DIR}/PHY/INIT/nr_parms.c
  ${OPENAIR1_DIR}/PHY/MODULATION/nr_modulation.c
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
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_csi_rs.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_gold.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/scrambling_luts.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_gen_mod_table.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/dmrs_nr.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/ptrs_nr.c
  ${OPENAIR1_DIR}/PHY/NR_UE_ESTIMATION/filt16a_32.c
  ${OPENAIR1_DIR}/PHY/NR_ESTIMATION/nr_ul_channel_estimation.c
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
  ${OPENAIR1_DIR}/PHY/TOOLS/time_meas.c
  ${OPENAIR1_DIR}/PHY/TOOLS/lut.c
  ${PHY_POLARSRC}
  ${PHY_SMALLBLOCKSRC}
  ${PHY_NR_CODINGIF}
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/pucch_rx.c
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
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/srs_modulation_nr.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/dci_nr.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/dci_tools_nr.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/pucch_nr.c
  ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_uci_tools_common.c
  ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/nr_ulsch_ue.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/ul_ref_seq_nr.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_dmrs_rx.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/dmrs_nr.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/ptrs_nr.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_gold_ue.c
  ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_gen_mod_table.c
  ${OPENAIR1_DIR}/PHY/NR_UE_ESTIMATION/filt16a_32.c
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
  ${OPENAIR1_DIR}/PHY/TOOLS/time_meas.c
  ${OPENAIR1_DIR}/PHY/TOOLS/lut.c
  ${OPENAIR1_DIR}/PHY/INIT/nr_init_ue.c
  #  ${OPENAIR1_DIR}/SIMULATION/NR_UE_PHY/unit_tests/src/pucch_uci_test.c
  ${PHY_POLARSRC}
  ${PHY_SMALLBLOCKSRC}
  ${PHY_NR_CODINGIF}
  )


if (${SMBV})
  set(PHY_SRC "${PHY_SRC} ${OPENAIR1_DIR}/PHY/TOOLS/smbv.c")
endif  (${SMBV})

if (${COMPILATION_AVX2} STREQUAL "True")
  #set(PHY_SRC ${PHY_SRC} ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dlsch_llr_computation_avx2.c)
  set(PHY_SRC_UE ${PHY_SRC_UE} ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dlsch_llr_computation_avx2.c)
endif ()

if (${COMPILATION_AVX2} STREQUAL "True")
  set(PHY_NR_UE_SRC ${PHY_NR_UE_SRC} ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dlsch_llr_computation_avx2.c)
endif ()

add_library(PHY_COMMON ${PHY_SRC_COMMON})
add_dependencies(PHY_COMMON rrc_flag)
add_dependencies(PHY_COMMON dfts)
add_library(PHY ${PHY_SRC})

add_dependencies(PHY rrc_flag)
add_library(PHY_UE ${PHY_SRC_UE})
add_dependencies(PHY_UE rrc_flag)
add_library(PHY_NR_COMMON ${PHY_NR_SRC_COMMON})
add_library(PHY_NR ${PHY_NR_SRC})
add_library(PHY_NR_UE ${PHY_NR_UE_SRC})

add_library(PHY_RU ${PHY_SRC_RU})
add_dependencies(PHY_RU rrc_flag)

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
  ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/dlsch_llr_computation_avx2.c
  ${OPENAIR1_DIR}/PHY/TOOLS/signal_energy.c
  ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_ue_measurements.c
  ${OPENAIR_DIR}/common/utils/LOG/log.c
  ${OPENAIR_DIR}/common/utils/T/T.c
  ${OPENAIR_DIR}/common/utils/T/local_tracer.c
  )
add_library(PHY_MEX ${PHY_MEX_UE} ${CONFIG_LIB})

#Layer 2 library
#####################
set(MAC_DIR ${OPENAIR2_DIR}/LAYER2/MAC)
set(NR_GNB_MAC_DIR ${OPENAIR2_DIR}/LAYER2/NR_MAC_gNB)
set(NR_UE_MAC_DIR ${OPENAIR2_DIR}/LAYER2/NR_MAC_UE)
set(PHY_INTERFACE_DIR ${OPENAIR2_DIR}/PHY_INTERFACE)
set(NR_PHY_INTERFACE_DIR ${OPENAIR2_DIR}/NR_PHY_INTERFACE)
set(NR_UE_PHY_INTERFACE_DIR ${OPENAIR2_DIR}/NR_UE_PHY_INTERFACE)
set(RLC_DIR ${OPENAIR2_DIR}/LAYER2/RLC)
set(RLC_UM_DIR ${OPENAIR2_DIR}/LAYER2/RLC/UM_v9.3.0)
set(RLC_AM_DIR ${OPENAIR2_DIR}/LAYER2/RLC/AM_v9.3.0)
set(RLC_TM_DIR ${OPENAIR2_DIR}/LAYER2/RLC/TM_v9.3.0)
set(RRC_DIR ${OPENAIR2_DIR}/RRC/LTE)
set(NR_RRC_DIR ${OPENAIR2_DIR}/RRC/NR)
set(NR_UE_RRC_DIR ${OPENAIR2_DIR}/RRC/NR_UE)
set(PDCP_DIR  ${OPENAIR2_DIR}/LAYER2/PDCP_v10.1.0)

set(RLC_V1
  ${RLC_AM_DIR}/rlc_am.c
  ${RLC_AM_DIR}/rlc_am_init.c
  ${RLC_AM_DIR}/rlc_am_timer_poll_retransmit.c
  ${RLC_AM_DIR}/rlc_am_timer_reordering.c
  ${RLC_AM_DIR}/rlc_am_timer_status_prohibit.c
  ${RLC_AM_DIR}/rlc_am_segment.c
  ${RLC_AM_DIR}/rlc_am_segments_holes.c
  ${RLC_AM_DIR}/rlc_am_in_sdu.c
  ${RLC_AM_DIR}/rlc_am_receiver.c
  ${RLC_AM_DIR}/rlc_am_retransmit.c
  ${RLC_AM_DIR}/rlc_am_windows.c
  ${RLC_AM_DIR}/rlc_am_rx_list.c
  ${RLC_AM_DIR}/rlc_am_reassembly.c
  ${RLC_AM_DIR}/rlc_am_status_report.c
  ${RLC_TM_DIR}/rlc_tm.c
  ${RLC_TM_DIR}/rlc_tm_init.c
  ${RLC_UM_DIR}/rlc_um.c
  ${RLC_UM_DIR}/rlc_um_fsm.c
  ${RLC_UM_DIR}/rlc_um_control_primitives.c
  ${RLC_UM_DIR}/rlc_um_segment.c
  ${RLC_UM_DIR}/rlc_um_reassembly.c
  ${RLC_UM_DIR}/rlc_um_receiver.c
  ${RLC_UM_DIR}/rlc_um_dar.c
  ${RLC_DIR}/rlc_mac.c
  ${RLC_DIR}/rlc.c
  ${RLC_DIR}/rlc_rrc.c
  ${RLC_DIR}/rlc_mpls.c
  )

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

set(NR_RLC_SRC
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/asn1_utils.c
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/nr_rlc_entity.c
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/nr_rlc_entity_am.c
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/nr_rlc_entity_tm.c
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/nr_rlc_entity_um.c
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/nr_rlc_oai_api.c
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/nr_rlc_pdu.c
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/nr_rlc_sdu.c
  ${OPENAIR2_DIR}/LAYER2/nr_rlc/nr_rlc_ue_manager.c
  )

set(NR_PDCP_SRC
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_oai_api.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_ue_manager.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_entity.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_sdu.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_timer_thread.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_security_nea2.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_integrity_nia2.c
  ${OPENAIR2_DIR}/LAYER2/nr_pdcp/asn1_utils.c
  )

set(NR_SDAP_SRC
  ${OPENAIR2_DIR}/SDAP/nr_sdap/nr_sdap_gnb.c
  )
  
set(L2_SRC
  ${PDCP_DIR}/pdcp.c
  ${PDCP_DIR}/pdcp_fifo.c
  ${PDCP_DIR}/pdcp_sequence_manager.c
  ${PDCP_DIR}/pdcp_primitives.c
  ${PDCP_DIR}/pdcp_util.c
  ${PDCP_DIR}/pdcp_security.c
  ${PDCP_DIR}/pdcp_netlink.c
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
  ${NR_RLC_SRC}
  ${NR_PDCP_SRC}
  ${NR_SDAP_SRC}
  ${NR_RRC_DIR}/rrc_gNB.c
  ${NR_RRC_DIR}/nr_rrc_common.c
  ${NR_RRC_DIR}/L2_nr_interface.c
  ${NR_RRC_DIR}/nr_rrc_config.c
  ${NR_RRC_DIR}/rrc_gNB_nsa.c
  ${NR_RRC_DIR}/rrc_gNB_internode.c
  ${NR_RRC_DIR}/rrc_gNB_reconfig.c
  ${NR_RRC_DIR}/rrc_gNB_UE_context.c
  ${NR_RRC_DIR}/rrc_gNB_NGAP.c
  )

set(L2_SRC_UE
  ${PDCP_DIR}/pdcp.c
  ${PDCP_DIR}/pdcp_fifo.c
  ${PDCP_DIR}/pdcp_sequence_manager.c
  ${PDCP_DIR}/pdcp_primitives.c
  ${PDCP_DIR}/pdcp_util.c
  ${PDCP_DIR}/pdcp_security.c
  ${PDCP_DIR}/pdcp_netlink.c
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
  ${NR_RLC_SRC}
  ${NR_PDCP_SRC}
  ${NR_SDAP_SRC}
  ${NR_UE_RRC_DIR}/L2_interface_ue.c
  ${NR_UE_RRC_DIR}/main_ue.c
  ${NR_UE_RRC_DIR}/rrc_UE.c
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
  ${NR_GNB_MAC_DIR}/main.c
  ${NR_GNB_MAC_DIR}/config.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_bch.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_dlsch.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_ulsch.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_primitives.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_phytest.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_uci.c
  ${NR_GNB_MAC_DIR}/gNB_scheduler_RA.c
 )


set (MAC_SRC_UE
  ${PHY_INTERFACE_DIR}/phy_stub_UE.c
  ${MAC_DIR}/main_ue.c
  ${MAC_DIR}/ue_procedures.c
  ${MAC_DIR}/ra_procedures.c
  ${MAC_DIR}/l1_helpers.c
  ${MAC_DIR}/rar_tools_ue.c
  ${MAC_DIR}/config_ue.c
 )

set (MAC_NR_SRC_UE
  ${NR_UE_PHY_INTERFACE_DIR}/NR_IF_Module.c
  ${NR_UE_MAC_DIR}/config_ue.c
  ${NR_UE_MAC_DIR}/mac_vars.c
  ${NR_UE_MAC_DIR}/main_ue_nr.c
  ${NR_UE_MAC_DIR}/nr_ue_procedures.c
  ${NR_UE_MAC_DIR}/nr_ue_scheduler.c
  ${NR_UE_MAC_DIR}/nr_ue_dci_configuration.c
  ${NR_UE_MAC_DIR}/nr_l1_helpers.c
  ${NR_UE_MAC_DIR}/nr_ra_procedures.c
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
  ${OPENAIR2_DIR}/GNB_APP/gnb_config.c
  )

set (MCE_APP_SRC
  ${OPENAIR2_DIR}/MCE_APP/mce_app.c
  ${OPENAIR2_DIR}/MCE_APP/mce_config.c
  )
set (MISC_NFAPI_LTE
 ${OPENAIR1_DIR}/SCHED/nfapi_lte_dummy.c
 )

add_library(MISC_NFAPI_LTE_LIB
  ${MISC_NFAPI_LTE}
 )

set (MISC_NFAPI_NR
 ${OPENAIR1_DIR}/SCHED/nfapi_nr_dummy.c
 )

add_library(MISC_NFAPI_NR_LIB
  ${MISC_NFAPI_NR}
 )
add_library(L2
  ${L2_SRC}
  ${MAC_SRC}
  ${ENB_APP_SRC}
  ${MCE_APP_SRC}
  )
add_dependencies(L2 rrc_flag s1ap_flag x2_flag m2_flag m3_flag)

add_library(MAC_NR
  ${MAC_NR_SRC}
  )

add_library(MAC_UE_NR
  ${MAC_NR_SRC_UE}
  )

add_library(L2_LTE
  ${L2_LTE_SRC}
  )

add_library(L2_NR
  ${L2_NR_SRC}
  ${MAC_NR_SRC}
  ${GNB_APP_SRC}
  )
target_compile_definitions(L2_NR PUBLIC NEW_GTPU)

add_library(L2_LTE_NR
  ${L2_RRC_SRC}
  ${MAC_SRC}
  ${ENB_APP_SRC}
  ${MCE_APP_SRC}
)
target_compile_definitions(L2_LTE_NR PUBLIC NEW_GTPU)

add_dependencies(L2_NR rrc_flag nr_rrc_flag s1ap_flag x2_flag)

add_library(L2_UE
  ${L2_SRC_UE}
  ${MAC_SRC_UE}
)

add_library(L2_UE_LTE_NR
  ${L2_RRC_SRC_UE}
  ${MAC_SRC_UE}
)

if (NOT ${NOS1})
target_compile_definitions(L2_UE PUBLIC -DPDCP_USE_NETLINK)
endif()

add_dependencies(L2_UE rrc_flag s1ap_flag x2_flag)

add_library( NR_L2_UE ${NR_L2_SRC_UE} ${MAC_NR_SRC_UE} )

add_library( MAC_NR_COMMON ${OPENAIR2_DIR}/LAYER2/NR_MAC_COMMON/nr_mac_common.c ${OPENAIR2_DIR}/LAYER2/NR_MAC_COMMON/nr_compute_tbs_common.c)

include_directories("${OPENAIR2_DIR}/NR_UE_PHY_INTERFACE")
include_directories("${OPENAIR2_DIR}/LAYER2")
include_directories("${OPENAIR1_DIR}/SCHED_NR_UE")
#include_directories("${NFAPI_USER_DIR}"")

# L3 Libs
##########################

# CN libs
##########################

add_library(CN_UTILS
  ${OPENAIR3_DIR}/UTILS/conversions.c
  ${OPENAIR3_DIR}/UTILS/enum_string.c
  ${OPENAIR3_DIR}/UTILS/mcc_mnc_itu.c
  )

set(GTPV1U_DIR ${OPENAIR3_DIR}/GTPV1-U)

add_library (GTPV1U_OCP
  ${NR_RRC_DIR}/rrc_gNB_GTPV1U.c
  ${RRC_DIR}/rrc_eNB_GTPV1U.c
  ${OPENAIR3_DIR}/ocp-gtpu/gtp_itf.cpp
  )
target_compile_definitions(GTPV1U_OCP PUBLIC NEW_GTPU)
add_dependencies(GTPV1U_OCP rrc_flag)
include_directories(${OPENAIR3_DIR}/ocp-gtp)

#NR case
set (NR_GTPV1U_SRC
  ${NR_RRC_DIR}/rrc_gNB_GTPV1U.c
  ${RRC_DIR}/rrc_eNB_GTPV1U.c
  ${GTPV1U_DIR}/gtpv1u_eNB.c
  ${GTPV1U_DIR}/gtpv1u_gNB.c
  ${GTPV1U_DIR}/nw-gtpv1u/src/NwGtpv1uTunnelEndPoint.c
  ${GTPV1U_DIR}/nw-gtpv1u/src/NwGtpv1uTrxn.c
  ${GTPV1U_DIR}/nw-gtpv1u/src/NwGtpv1uMsg.c
  ${GTPV1U_DIR}/nw-gtpv1u/src/NwGtpv1u.c
  ${GTPV1U_DIR}/gtpv1u_teid_pool.c
  )
add_library(NR_GTPV1U ${NR_GTPV1U_SRC})
add_dependencies(NR_GTPV1U rrc_flag)

set (MME_APP_SRC
  ${OPENAIR3_DIR}/MME_APP/mme_app.c
  ${OPENAIR3_DIR}/MME_APP/mme_config.c
)
add_library(MME_APP ${MME_APP_SRC})
add_dependencies(MME_APP rrc_flag s1ap_flag x2_flag m2_flag m3_flag)

set(SCTP_SRC
  ${OPENAIR3_DIR}/SCTP/sctp_common.c
  ${OPENAIR3_DIR}/SCTP/sctp_eNB_task.c
  ${OPENAIR3_DIR}/SCTP/sctp_eNB_itti_messaging.c
)
add_library(SCTP_CLIENT ${SCTP_SRC})
add_dependencies(SCTP_CLIENT rrc_flag)

add_library(UDP ${OPENAIR3_DIR}/UDP/udp_eNB_task.c)
add_dependencies(UDP rrc_flag)

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
  ${NAS_SRC}COMMON/UTIL/memory.c
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
  )
add_dependencies(LIB_NAS_SIMUE rrc_flag)
set(NAS_SIM_LIB LIB_NAS_SIMUE)

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
  )
add_dependencies(LIB_NAS_UE rrc_flag)
set(NAS_UE_LIB LIB_NAS_UE)


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

# shared library loader
set (SHLIB_LOADER_SOURCES
  ${OPENAIR_DIR}/common/utils/load_module_shlib.c
  )

add_library(LIB_5GNAS_GNB
  ${NAS_SRC}/COMMON/nr_common.c
  ${NAS_SRC}/gNB/network_process_nas.c
  ${NAS_SRC}/NR_UE/ue_process_nas.c
  ${OPENAIR3_DIR}//UICC/usim_interface.c
  )
target_link_libraries(LIB_5GNAS_GNB SECU_CN ${CRYPTO_LIBRARIES})

# Make lfds as a own source code (even if it is a outside library)
# For better intergration with compilation flags & structure of cmake
###################################################################
set(lfds ${OPENAIR2_DIR}/UTIL/LFDS/liblfds6.1.1/liblfds611/src/)
file(GLOB lfds_queue ${lfds}/lfds611_queue/*.c)
file(GLOB lfds_ring ${lfds}/lfds611_ringbuffer/*.c)
file(GLOB lfds_slist ${lfds}/lfds611_slist/*.c)
file(GLOB lfds_stack ${lfds}/lfds611_stack/*.c)
file(GLOB lfds_freelist ${lfds}/lfds611_freelist/*.c)

include_directories(${lfds})
add_library(LFDS
  ${lfds_queue} ${lfds_ring} ${lfds_slist} ${lfds_stack} ${lfds_freelist}
  ${lfds}/lfds611_liblfds/lfds611_liblfds_abstraction_test_helpers.c
  ${lfds}/lfds611_liblfds/lfds611_liblfds_aligned_free.c
  ${lfds}/lfds611_liblfds/lfds611_liblfds_aligned_malloc.c
  ${lfds}/lfds611_abstraction/lfds611_abstraction_free.c
  ${lfds}/lfds611_abstraction/lfds611_abstraction_malloc.c
)

set(lfds7 ${OPENAIR2_DIR}/UTIL/LFDS/liblfds7.0.0/liblfds700/src/)
file(GLOB lfds7_queue ${lfds7}/lfds700_queue/*.c)
file(GLOB lfds7_ring ${lfds7}/lfds700_ringbuffer/*.c)
file(GLOB lfds7_qbss ${lfds7}/lfds700_queue_bounded_singleconsumer_singleproducer/*.c)
file(GLOB lfds7_stack ${lfds7}/lfds700_stack/*.c)
file(GLOB lfds7_freelist ${lfds7}/lfds700_freelist/*.c)
file(GLOB lfds7_btree ${lfds7}/lfds700_btree_addonly_unbalanced/*.c)
file(GLOB lfds7_hash ${lfds7}/lfds700_hash_addonly/*.c)
file(GLOB lfds7_ordered_list ${lfds7}/lfds700_list_addonly_ordered_singlylinked/*.c)
file(GLOB lfds7_unordered_list ${lfds7}/lfds700_list_addonly_singlylinked_unordered/*.c)
file(GLOB lfds7_misc ${lfds7}/lfds700_misc/*.c)

include_directories(${lfds7})
add_library(LFDS7
  ${lfds7_queue} ${lfds7_ring} ${lfds7_qbss} ${lfds7_stack} ${lfds7_freelist} ${lfds7_btree} ${lfds7_hash} ${lfds7_ordered_list} ${lfds7_unordered_list} ${lfds7_misc}
)

add_library(SIMU_COMMON
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/random_channel.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/rangen_double.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/phase_noise.c
  )

# Simulation library
##########################
set (SIMUSRC
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/taus.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/multipath_channel.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/multipath_tv_channel.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/abstraction.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/channel_sim.c
  ${OPENAIR1_DIR}/SIMULATION/RF/rf.c
  ${OPENAIR1_DIR}/SIMULATION/RF/dac.c
  ${OPENAIR1_DIR}/SIMULATION/RF/adc.c
  #${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  )

# Simulation library
##########################
add_library(SIMU SHARED ${SIMUSRC} )

add_library(SIMU_ETH
${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/multicast_link.c
${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/socket.c
${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/bypass_session_layer.c
#${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/emu_transport.c
)

include_directories("${NFAPI_DIR}/nfapi/public_inc")
include_directories("${NFAPI_DIR}/common/public_inc")
include_directories("${NFAPI_DIR}/pnf/public_inc")
include_directories("${NFAPI_DIR}/nfapi/inc")
include_directories("${NFAPI_DIR}/sim_common/inc")
include_directories("${NFAPI_DIR}/pnf_sim/inc")

list(APPEND ATLAS_LIBRARIES lapack lapacke)

include_directories ("/usr/include/X11")
set(XFORMS_SOURCE
  ${OPENAIR1_DIR}/PHY/TOOLS/lte_phy_scope.c
  )
set(XFORMS_ENB_SOURCE
  ${OPENAIR1_DIR}/PHY/TOOLS/lte_enb_scope.c
  )
set(XFORMS_UE_SOURCE
  ${OPENAIR1_DIR}/PHY/TOOLS/lte_ue_scope.c
  )
set(XFORMS_SOURCE_NR
  ${OPENAIR1_DIR}/PHY/TOOLS/nr_phy_scope.c
  )
set(XFORMS_SOURCE_SOFTMODEM
  ${OPENAIR_TARGETS}/RT/USER/stats.c
  )
set(XFORMSINTERFACE_SOURCE
  ${OPENAIR1_DIR}/PHY/TOOLS/phy_scope_interface.c
  )
set(XFORMS_LIBRARIES "forms")

add_library(enbscope MODULE ${XFORMS_SOURCE} ${XFORMS_SOURCE_SOFTMODEM} ${XFORMS_ENB_SOURCE})
add_library(uescope MODULE ${XFORMS_SOURCE} ${XFORMS_SOURCE_SOFTMODEM} ${XFORMS_UE_SOURCE})
target_link_libraries(enbscope ${XFORMS_LIBRARIES})
target_link_libraries(uescope ${XFORMS_LIBRARIES})

add_library(nrscope MODULE ${XFORMS_SOURCE_NR})
target_link_libraries(nrscope ${XFORMS_LIBRARIES})


add_library(rfsimulator MODULE
  ${OPENAIR_TARGETS}/ARCH/rfsimulator/simulator.c
  ${OPENAIR_TARGETS}/ARCH/rfsimulator/apply_channelmod.c
  ${OPENAIR_TARGETS}/ARCH/rfsimulator/new_channel_sim.c
  ${OPENAIR1_DIR}/PHY/TOOLS/signal_energy.c
	)
target_link_libraries(rfsimulator SIMU_COMMON ${ATLAS_LIBRARIES})

add_library(oai_iqplayer MODULE
	${OPENAIR_TARGETS}/ARCH/iqplayer/iqplayer_lib.c
	)
set(CMAKE_MODULE_PATH "${OPENAIR_DIR}/cmake_targets/tools/MODULES" "${CMAKE_MODULE_PATH}")

#include T directory even if the T is off because T macros are in the code
#no matter what
include_directories("${OPENAIR_DIR}/common/utils/T")

if (${T_TRACER})
  set(T_SOURCE
      ${OPENAIR_DIR}/common/utils/T/T_IDs.h
      ${OPENAIR_DIR}/common/utils/T/T.c
      ${OPENAIR_DIR}/common/utils/T/local_tracer.c)
  set(T_LIB "rt")
endif (${T_TRACER})

#Some files in the T directory are generated.
#This rule and the following deal with it.
add_custom_command (
  OUTPUT ${OPENAIR_DIR}/common/utils/T/T_IDs.h
  COMMAND make check_vcd
  WORKING_DIRECTORY ${OPENAIR_DIR}/common/utils/T
  DEPENDS ${OPENAIR_DIR}/common/utils/T/T_messages.txt
          ${OPENAIR_DIR}/common/utils/LOG/vcd_signal_dumper.c
          ${OPENAIR_DIR}/common/utils/LOG/vcd_signal_dumper.h
  )

execute_process (
  COMMAND make check_vcd
  WORKING_DIRECTORY ${OPENAIR_DIR}/common/utils/T
)
#This rule is specifically needed to generate T files
#before anything else in a project that uses the T.
#See below, there are some 'add_dependencies' showing that.
#Basically we create a custom target and we make other
#targets depend on it. That forces cmake to generate
#T files before anything else.
add_custom_target (
  generate_T
  DEPENDS ${OPENAIR_DIR}/common/utils/T/T_IDs.h
)

# Hack on a test of asn1c version (already dirty)
add_definitions(-DASN1_MINIMUM_VERSION=924)

#################################
# add executables for operation
#################################
add_library(minimal_lib
  ${OPENAIR_DIR}/common/utils/backtrace.c
  ${OPENAIR_DIR}/common/utils/LOG/log.c
  ${OPENAIR_DIR}/common/utils/minimal_stub.c
  ${T_SOURCE}
  )
target_link_libraries(minimal_lib pthread dl ${T_LIB} ${CONFIG_LIB})

add_executable(nfapi_test
  ${OPENAIR_DIR}/openair2/NR_PHY_INTERFACE/nfapi_5g_test.c
  )

add_executable(replay_node
  ${OPENAIR_TARGETS}/ARCH/rfsimulator/stored_node.c
  )
target_link_libraries (replay_node minimal_lib)

add_executable(measurement_display
  ${OPENAIR_DIR}/common/utils/threadPool/measurement_display.c)
target_link_libraries (measurement_display minimal_lib)

add_executable(test5Gnas
  ${OPENAIR_DIR}/openair3/TEST/test5Gnas.c
)
target_link_libraries (test5Gnas LIB_5GNAS_GNB CONFIG_LIB minimal_lib )


# lte-softmodem is both eNB and UE implementation
###################################################

add_executable(lte-softmodem
  ${OPENAIR_TARGETS}/RT/USER/rt_wrapper.c
  ${OPENAIR_TARGETS}/RT/USER/lte-enb.c
  ${OPENAIR_TARGETS}/RT/USER/lte-ru.c
  ${OPENAIR_TARGETS}/RT/USER/ru_control.c
  ${OPENAIR_TARGETS}/RT/USER/lte-softmodem.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR2_DIR}/ENB_APP/NB_IoT_interface.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/taus.c
  ${OPENAIR_TARGETS}/COMMON/create_tasks.c
  ${OPENAIR_TARGETS}/COMMON/create_tasks_mbms.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/common_lib.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR2_DIR}/F1AP/dummy_enb.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/multicast_link.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/socket.c
  ${OPENAIR3_DIR}/NAS/UE/nas_ue_task.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/lte/ue_power.c
  ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
  ${XFORMSINTERFACE_SOURCE}
  ${T_SOURCE}
  ${CONFIG_SOURCES}
  ${SHLIB_LOADER_SOURCES}
  )
add_dependencies(lte-softmodem rrc_flag s1ap_flag x2_flag oai_iqplayer)

target_link_libraries (lte-softmodem
  -Wl,--start-group
  RRC_LIB NR_RRC_LIB S1AP_LIB S1AP_ENB F1AP_LIB F1AP M2AP_LIB M2AP_ENB X2AP_LIB X2AP_ENB M3AP_LIB M3AP_ENB NR_GTPV1U SECU_CN SECU_OSA UTIL HASHTABLE SCTP_CLIENT MME_APP UDP SCHED_LIB SCHED_RU_LIB
  PHY_COMMON PHY PHY_RU LFDS L2 L2_LTE NFAPI_COMMON_LIB NFAPI_LIB NFAPI_VNF_LIB NFAPI_PNF_LIB NFAPI_USER_LIB MISC_NFAPI_LTE_LIB LFDS7
  ${MSC_LIB} ${RAL_LIB} ${NAS_UE_LIB} ITTI ${FLPT_MSG_LIB} ${ASYNC_IF_LIB} ${FLEXRAN_AGENT_LIB} ${FSPT_MSG_LIB} ${PROTO_AGENT_LIB}
  -Wl,--end-group z dl)

target_link_libraries (lte-softmodem ${LIBXML2_LIBRARIES})
target_link_libraries (lte-softmodem pthread m ${CONFIG_LIB} rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} sctp ${PROTOBUF_LIB}  ${CMAKE_DL_LIBS} ${LIBYAML_LIBRARIES})
target_link_libraries (lte-softmodem ${LIB_LMS_LIBRARIES})
target_link_libraries (lte-softmodem ${T_LIB})


add_executable(ocp-enb
  ${OPENAIR_DIR}/executables/main-ocp.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR_DIR}/executables/main-fs6.c
  ${OPENAIR_DIR}/executables/transport_split.c
  ${OPENAIR2_DIR}/ENB_APP/NB_IoT_interface.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/taus.c
  ${OPENAIR_TARGETS}/COMMON/create_tasks.c
  ${OPENAIR_TARGETS}/COMMON/create_tasks_mbms.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/common_lib.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/multicast_link.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/socket.c
  ${OPENAIR3_DIR}/NAS/UE/nas_ue_task.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/lte/ue_power.c
  ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
  ${XFORMSINTERFACE_SOURCE}
  ${T_SOURCE}
  ${CONFIG_SOURCES}
  ${SHLIB_LOADER_SOURCES}
  )
add_dependencies(ocp-enb rrc_flag s1ap_flag x2_flag oai_iqplayer coding params_libconfig rfsimulator)

target_link_libraries (ocp-enb
  -Wl,--start-group

  RRC_LIB NR_RRC_LIB S1AP_LIB S1AP_ENB F1AP_LIB F1AP M2AP_LIB M2AP_ENB X2AP_LIB X2AP_ENB M3AP_LIB M3AP_ENB NR_GTPV1U SECU_CN SECU_OSA UTIL HASHTABLE SCTP_CLIENT MME_APP UDP SCHED_LIB SCHED_RU_LIB
  PHY_COMMON PHY PHY_RU LFDS L2 L2_LTE NFAPI_COMMON_LIB NFAPI_LIB MISC_NFAPI_LTE_LIB NFAPI_VNF_LIB NFAPI_PNF_LIB NFAPI_USER_LIB LFDS7 SIMU_COMMON
  ${MSC_LIB} ${RAL_LIB} ${NAS_UE_LIB} ITTI ${FLPT_MSG_LIB} ${ASYNC_IF_LIB} ${FLEXRAN_AGENT_LIB} ${FSPT_MSG_LIB} ${PROTO_AGENT_LIB}
  -Wl,--end-group z dl)
target_link_libraries (ocp-enb ${LIBXML2_LIBRARIES} pthread m ${CONFIG_LIBRARIES} rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} sctp ${PROTOBUF_LIB}  ${CMAKE_DL_LIBS} ${LIBYAML_LIBRARIES} ${LIB_LMS_LIBRARIES} ${T_LIB})

add_executable(cu_test
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/cu_test.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_handler.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_common.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_net_comm.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_async.c
  ${T_SOURCE}
)
target_link_libraries(cu_test
  ${FLPT_MSG_LIB} ${ASYNC_IF_LIB} ${FSPT_MSG_LIB} ${PROTOBUF_LIB}
  ${PROTO_AGENT_LIB} pthread UTIL ${T_LIB} dl ITTI
)

add_executable(du_test
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/du_test.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_handler.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_common.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_net_comm.c
  ${OPENAIR2_DIR}/LAYER2/PROTO_AGENT/proto_agent_async.c
  ${T_SOURCE}
)
target_link_libraries(du_test
  ${FLPT_MSG_LIB} ${ASYNC_IF_LIB} ${FSPT_MSG_LIB} ${PROTOBUF_LIB}
  ${PROTO_AGENT_LIB} pthread UTIL ${T_LIB} dl ITTI
)

add_executable(oairu
  ${OPENAIR_TARGETS}/RT/USER/lte-ru.c
  ${OPENAIR_TARGETS}/RT/USER/ru_control.c
  ${OPENAIR_TARGETS}/RT/USER/rt_wrapper.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/common_lib.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/record_player.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR_DIR}/openair1/SCHED/phy_procedures_lte_common.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${OPENAIR_DIR}/executables/main_ru.c
  ${T_SOURCE}
  ${OPENAIR_DIR}/common/utils/LOG/log.c
  ${CONFIG_SOURCES}
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
  ${SHLIB_LOADER_SOURCES}
)
target_link_libraries (oairu
  -Wl,--start-group
  SCHED_RU_LIB
  PHY_COMMON PHY_RU
  -Wl,--end-group z dl)

target_link_libraries (oairu pthread m ${CONFIG_LIB} rt ${CMAKE_DL_LIBS} ${T_LIB})


# lte-uesoftmodem is  UE implementation
#######################################

add_executable(lte-uesoftmodem
  ${OPENAIR_TARGETS}/RT/USER/rt_wrapper.c
  ${OPENAIR_TARGETS}/RT/USER/lte-ue.c
  ${OPENAIR_TARGETS}/RT/USER/lte-uesoftmodem.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR_TARGETS}/COMMON/create_tasks_ue.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/common_lib.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/multicast_link.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/socket.c
  ${OPENAIR3_DIR}/NAS/UE/nas_ue_task.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/lte/ue_power.c
  ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
  ${XFORMSINTERFACE_SOURCE}
  ${T_SOURCE}
  ${CONFIG_SOURCES}
  ${SHLIB_LOADER_SOURCES}
  )

add_dependencies(lte-uesoftmodem rrc_flag s1ap_flag x2_flag)

if (NOT ${NOS1})
	target_compile_definitions(lte-uesoftmodem PRIVATE -DPDCP_USE_NETLINK)
endif()
target_link_libraries (lte-uesoftmodem
  -Wl,--start-group
  RRC_LIB NR_RRC_LIB S1AP_LIB S1AP_ENB X2AP_LIB X2AP_ENB M2AP_LIB M2AP_ENB M3AP_LIB M3AP_ENB F1AP F1AP_LIB

  NR_GTPV1U SECU_CN SECU_OSA UTIL HASHTABLE SCTP_CLIENT MME_APP UDP SCHED_RU_LIB SCHED_UE_LIB PHY_COMMON
  PHY_UE PHY_RU LFDS L2_UE L2_LTE LFDS7 SIMU_COMMON SIMU NFAPI_COMMON_LIB NFAPI_LIB NFAPI_PNF_LIB NFAPI_USER_LIB MISC_NFAPI_LTE_LIB
  ${MSC_LIB} ${RAL_LIB} ${NAS_UE_LIB} ITTI ${FLPT_MSG_LIB} ${ASYNC_IF_LIB} ${ATLAS_LIBRARIES}
  -Wl,--end-group z dl)

target_link_libraries (lte-uesoftmodem ${LIBXML2_LIBRARIES})
target_link_libraries (lte-uesoftmodem pthread m ${CONFIG_LIB} rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} sctp ${PROTOBUF_LIB}  ${CMAKE_DL_LIBS} ${LIBYAML_LIBRARIES} ${ATLAS_LIBRARIES})
target_link_libraries (lte-uesoftmodem ${LIB_LMS_LIBRARIES})
target_link_libraries (lte-uesoftmodem ${T_LIB})

# nr-softmodem
###################################################

add_executable(nr-softmodem
  ${rrc_h}
  ${nr_rrc_h}
  ${s1ap_h}
#  ${OPENAIR_BIN_DIR}/messages_xml.h
  ${OPENAIR_DIR}/executables/nr-gnb.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${OPENAIR_DIR}/executables/nr-ru.c
  ${OPENAIR_DIR}/executables/nr-softmodem.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/taus.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/common_lib.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/lte/ue_power.c
  ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${XFORMSINTERFACE_SOURCE}
  ${T_SOURCE}
  ${CONFIG_SOURCES}
  ${SHLIB_LOADER_SOURCES}
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_ran_api_to_fix.c
  )

target_compile_definitions(nr-softmodem PUBLIC NEW_GTPU)
target_link_libraries (nr-softmodem
  -Wl,--start-group
  UTIL HASHTABLE SCTP_CLIENT UDP SCHED_LIB SCHED_RU_LIB SCHED_NR_LIB PHY_NR PHY PHY_COMMON PHY_NR_COMMON PHY_RU LFDS GTPV1U_OCP SECU_CN SECU_OSA
  ITTI ${FLPT_MSG_LIB} ${ASYNC_IF_LIB} ${FLEXRAN_AGENT_LIB} LFDS7 ${MSC_LIB} ${RAL_LIB} ${NAS_UE_LIB} RRC_LIB NR_RRC_LIB
  NGAP_LIB NGAP_GNB S1AP_LIB S1AP_ENB L2_LTE_NR L2_NR MAC_NR_COMMON NFAPI_COMMON_LIB NFAPI_LIB NFAPI_VNF_LIB NFAPI_PNF_LIB NFAPI_USER_LIB
  X2AP_LIB X2AP_ENB F1AP_LIB F1AP M2AP_LIB M2AP_ENB M3AP_LIB M3AP_ENB ${PROTO_AGENT_LIB} ${FSPT_MSG_LIB}
  -Wl,--end-group z dl)

target_link_libraries (nr-softmodem ${LIBXML2_LIBRARIES})
target_link_libraries (nr-softmodem pthread m ${CONFIG_LIB} rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} sctp  ${XFORMS_LIBRARIES} ${PROTOBUF_LIB}  ${CMAKE_DL_LIBS} ${LIBYAML_LIBRARIES} ${ATLAS_LIBRARIES})
target_link_libraries (nr-softmodem ${LIB_LMS_LIBRARIES})
target_link_libraries (nr-softmodem ${T_LIB})

add_dependencies( nr-softmodem ldpc_orig ldpc_optim ldpc_optim8seg ldpc )

add_executable(ocp-gnb
  ${rrc_h}
  ${nr_rrc_h}
  ${s1ap_h}
#  ${OPENAIR_BIN_DIR}/messages_xml.h
  ${OPENAIR_DIR}/executables/ocp-gnb.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/taus.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/common_lib.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${XFORMS_SOURCE_NR}
  ${T_SOURCE}
  ${CONFIG_SOURCES}
  ${SHLIB_LOADER_SOURCES}
  ${XFORMSINTERFACE_SOURCE}
  )

target_link_libraries (ocp-gnb
  -Wl,--start-group
  UTIL HASHTABLE SCTP_CLIENT UDP SCHED_LIB SCHED_RU_LIB SCHED_NR_LIB PHY_NR PHY PHY_COMMON PHY_NR_COMMON PHY_RU LFDS GTPV1U_OCP SECU_CN SECU_OSA
  ITTI ${FLPT_MSG_LIB} ${ASYNC_IF_LIB} LFDS7 ${MSC_LIB} ${RAL_LIB} ${NAS_UE_LIB} RRC_LIB NR_RRC_LIB
  NGAP_LIB NGAP_GNB S1AP_LIB S1AP_ENB L2_LTE_NR L2_NR MAC_NR_COMMON NFAPI_COMMON_LIB NFAPI_LIB NFAPI_VNF_LIB NFAPI_PNF_LIB NFAPI_USER_LIB
  X2AP_LIB X2AP_ENB F1AP_LIB F1AP M2AP_LIB M2AP_ENB M3AP_LIB M3AP_ENB SIMU_COMMON
  -Wl,--end-group z dl)

target_link_libraries (ocp-gnb ${LIBXML2_LIBRARIES} )
target_link_libraries (ocp-gnb pthread m ${CONFIG_LIB} rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} sctp  ${XFORMS_LIBRARIES} ${PROTOBUF_LIB}  ${CMAKE_DL_LIBS} ${LIBYAML_LIBRARIES} ${ATLAS_LIBRARIES})
target_link_libraries (ocp-gnb ${LIB_LMS_LIBRARIES})
target_link_libraries (ocp-gnb ${T_LIB})
add_dependencies(ocp-gnb ldpc_orig ldpc_optim ldpc_optim8seg ldpc params_libconfig rfsimulator oai_usrpdevif rfsimulator nrscope)


# nr-uesoftmodem is  UE implementation
#######################################

add_executable(nr-uesoftmodem
  ${rrc_h}
  ${s1ap_h}
#  ${OPENAIR_BIN_DIR}/messages_xml.h
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${OPENAIR_DIR}/executables/nr-uesoftmodem.c
  ${OPENAIR_DIR}/executables/nr-ue.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/taus.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/common_lib.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/LAYER2/NR_MAC_COMMON/nr_mac_common.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${OPENAIR3_DIR}/NAS/UE/nas_ue_task.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${XFORMSINTERFACE_SOURCE}
  ${T_SOURCE}
  ${UTIL_SRC}
  ${CONFIG_SOURCES}
  ${SHLIB_LOADER_SOURCES}
  )

target_link_libraries (nr-uesoftmodem
  -Wl,--start-group
  RRC_LIB NR_RRC_LIB NGAP_LIB NGAP_GNB SECU_CN SECU_OSA UTIL HASHTABLE SCTP_CLIENT UDP SCHED_RU_LIB SCHED_UE_LIB SCHED_NR_UE_LIB
  PHY_COMMON PHY_NR_COMMON PHY_UE PHY_NR_UE PHY_RU LFDS NR_L2_UE L2_UE_LTE_NR MAC_NR_COMMON NFAPI_COMMON_LIB NFAPI_LIB NFAPI_PNF_LIB
  NFAPI_USER_LIB MISC_NFAPI_NR_LIB S1AP_LIB S1AP_ENB
  ${MSC_LIB} ${RAL_LIB} ${NAS_UE_LIB} ITTI ${FLPT_MSG_LIB} ${ASYNC_IF_LIB} LFDS7 ${ATLAS_LIBRARIES}
  NFAPI_USER_LIB S1AP_LIB S1AP_ENB
  ${MSC_LIB} ${RAL_LIB} ${NAS_UE_LIB} ITTI ${FLPT_MSG_LIB} ${ASYNC_IF_LIB} LFDS7 ${ATLAS_LIBRARIES} LIB_5GNAS_GNB LIB_NAS_SIMUE ${NAS_SIM_LIB}
  -Wl,--end-group z dl)

target_link_libraries (nr-uesoftmodem ${LIBXML2_LIBRARIES})
target_link_libraries (nr-uesoftmodem pthread m ${CONFIG_LIB} rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} sctp  ${XFORMS_LIBRARIES} ${PROTOBUF_LIB}  ${CMAKE_DL_LIBS} ${LIBYAML_LIBRARIES} ${ATLAS_LIBRARIES})
target_link_libraries (nr-uesoftmodem ${LIB_LMS_LIBRARIES})
target_link_libraries (nr-uesoftmodem ${T_LIB})

add_dependencies( nr-uesoftmodem ldpc_orig ldpc_optim ldpc_optim8seg ldpc )
if (CUDA_FOUND)
   add_dependencies( nr-uesoftmodem ldpc_cuda)
   add_dependencies( nr-softmodem ldpc_cuda)
   add_dependencies( ocp-gnb ldpc_cuda)
endif (CUDA_FOUND)
###################################"
# Addexecutables for tests
####################################


# Unitary tests for each piece of L1: example, mbmssim is MBMS L1 simulator
#####################################

#special case for dlim TM4, which uses its own version of phy_scope code
add_executable(dlsim_tm4
  ${OPENAIR1_DIR}/SIMULATION/LTE_PHY/dlsim_tm4.c
  ${OPENAIR1_DIR}/PHY/TOOLS/lte_phy_scope_tm4.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${T_SOURCE}
  )
target_link_libraries (dlsim_tm4
  -Wl,--start-group SIMU_COMMON SIMU UTIL SCHED_LIB SCHED_RU_LIB PHY LFDS ITTI -Wl,--end-group
  pthread m rt ${CONFIG_LIB} ${ATLAS_LIBRARIES} ${T_LIB}
  )

add_executable(rftest
  ${OPENAIR_DIR}/openair1/PHY/TOOLS/calibration_test.c
  ${OPENAIR_DIR}/openair1/PHY/TOOLS/calibration_scope.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/common_lib.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${CONFIG_SOURCES}
  ${SHLIB_LOADER_SOURCES}
)
target_link_libraries(rftest minimal_lib PHY_NR_COMMON pthread dl m forms ${T_LIB} )

add_executable(polartest
  ${OPENAIR1_DIR}/PHY/CODING/TESTBENCH/polartest.c
  ${OPENAIR_DIR}/common/utils/backtrace.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${T_SOURCE}
  ${SHLIB_LOADER_SOURCES}
  )
target_link_libraries(polartest
  -Wl,--start-group UTIL SIMU_COMMON  SIMU PHY_COMMON PHY_NR PHY_NR_COMMON PHY_NR_UE CONFIG_LIB -Wl,--end-group
  m pthread ${ATLAS_LIBRARIES} dl
  )

add_executable(smallblocktest
  ${OPENAIR1_DIR}/PHY/CODING/TESTBENCH/smallblocktest.c
  ${OPENAIR_DIR}/common/utils/backtrace.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${T_SOURCE}
  ${SHLIB_LOADER_SOURCES}
)


target_link_libraries(smallblocktest
  -Wl,--start-group UTIL SIMU_COMMON SIMU PHY_NR PHY_COMMON PHY_NR_COMMON CONFIG_LIB -Wl,--end-group
  m pthread ${ATLAS_LIBRARIES} dl
  )


add_executable(ldpctest
   ${PHY_NR_CODINGIF}
   ${OPENAIR1_DIR}/PHY/CODING/TESTBENCH/ldpctest.c
   ${T_SOURCE}
   ${SHLIB_LOADER_SOURCES}
   )

add_dependencies( ldpctest ldpc_orig ldpc_optim ldpc_optim8seg ldpc )
if (CUDA_FOUND)
   add_dependencies( ldpctest ldpc_cuda)
endif (CUDA_FOUND)
target_link_libraries(ldpctest
  -Wl,--start-group UTIL SIMU_COMMON  SIMU PHY_NR PHY_COMMON PHY_NR_COMMON CONFIG_LIB -Wl,--end-group
  m pthread ${ATLAS_LIBRARIES} dl
  )

add_executable(nr_dlschsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/dlschsim.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${UTIL_SRC}
  ${T_SOURCE}
  ${SHLIB_LOADER_SOURCES}
  )
target_link_libraries(nr_dlschsim
  -Wl,--start-group UTIL SIMU_COMMON SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB CONFIG_LIB MAC_NR_COMMON -Wl,--end-group
  m pthread ${ATLAS_LIBRARIES} ${T_LIB} ITTI dl
  )

add_executable(nr_pbchsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/pbchsim.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${UTIL_SRC}
  ${T_SOURCE}
  ${SHLIB_LOADER_SOURCES}
  )
target_link_libraries(nr_pbchsim
  -Wl,--start-group UTIL SIMU_COMMON SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB CONFIG_LIB MAC_NR_COMMON -Wl,--end-group
  m pthread ${ATLAS_LIBRARIES} ${T_LIB} ITTI dl
  )

#PUCCH ---> Prashanth
add_executable(nr_pucchsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/pucchsim.c
  ${OPENAIR_DIR}/common/utils/backtrace.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${UTIL_SRC}
  ${T_SOURCE}
  ${SHLIB_LOADER_SOURCES}
  )
target_link_libraries(nr_pucchsim
  -Wl,--start-group UTIL SIMU_COMMON SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB CONFIG_LIB MAC_NR_COMMON -Wl,--end-group
  m pthread ${ATLAS_LIBRARIES} ${T_LIB} ITTI dl
  )

add_executable(nr_dlsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/dlsim.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${NFAPI_USER_DIR}/nfapi.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${UTIL_SRC}
  ${T_SOURCE}
  ${SHLIB_LOADER_SOURCES}
  )
target_link_libraries(nr_dlsim
  -Wl,--start-group UTIL SIMU_COMMON SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB SCHED_NR_UE_LIB MAC_NR MAC_UE_NR MAC_NR_COMMON RRC_LIB NR_RRC_LIB CONFIG_LIB L2_LTE_NR L2_NR HASHTABLE X2AP_ENB X2AP_LIB SECU_CN NGAP_GNB -Wl,--end-group
  m pthread ${ATLAS_LIBRARIES} ${T_LIB} ITTI ${OPENSSL_LIBRARIES} dl
  )
target_compile_definitions(nr_dlsim PUBLIC -DPHYSICAL_SIMULATOR)

add_executable(nr_prachsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/prachsim.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR1_DIR}/SCHED_NR/phy_procedures_nr_common.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${NFAPI_USER_DIR}/nfapi.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${UTIL_SRC}
  ${T_SOURCE}
  ${SHLIB_LOADER_SOURCES})
target_link_libraries(nr_prachsim
  -Wl,--start-group UTIL SIMU_COMMON SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_RU PHY_NR_UE MAC_NR_COMMON SCHED_NR_LIB SCHED_NR_UE_LIB MAC_NR MAC_UE_NR MAC_NR_COMMON RRC_LIB NR_RRC_LIB CONFIG_LIB L2_LTE_NR L2_NR HASHTABLE X2AP_ENB X2AP_LIB SECU_CN NGAP_GNB -Wl,--end-group
  m pthread ${ATLAS_LIBRARIES} ${T_LIB} ITTI ${OPENSSL_LIBRARIES} dl)

add_executable(nr_ulschsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/ulschsim.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${UTIL_SRC}
  ${T_SOURCE}
  ${SHLIB_LOADER_SOURCES}
  )
target_link_libraries(nr_ulschsim
  -Wl,--start-group UTIL SIMU_COMMON SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB CONFIG_LIB MAC_NR_COMMON -Wl,--end-group
  m pthread ${ATLAS_LIBRARIES} ${T_LIB} ITTI dl
  )

add_executable(nr_ulsim
  ${OPENAIR1_DIR}/SIMULATION/NR_PHY/ulsim.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${NFAPI_USER_DIR}/nfapi.c
  ${UTIL_SRC}
  ${T_SOURCE}
  ${SHLIB_LOADER_SOURCES}
  )
target_link_libraries(nr_ulsim
  -Wl,--start-group UTIL SIMU_COMMON SIMU PHY_COMMON PHY_NR_COMMON PHY_NR PHY_NR_UE SCHED_NR_LIB SCHED_NR_UE_LIB MAC_NR MAC_UE_NR MAC_NR_COMMON RRC_LIB NR_RRC_LIB CONFIG_LIB L2_LTE_NR L2_NR HASHTABLE X2AP_ENB X2AP_LIB SECU_CN NGAP_GNB -Wl,--end-group
  m pthread ${ATLAS_LIBRARIES} ${T_LIB} ITTI ${OPENSSL_LIBRARIES} dl
  )
target_compile_definitions(nr_ulsim PUBLIC -DPHYSICAL_SIMULATOR)

foreach(myExe dlsim dlsim_tm7 ulsim pbchsim scansim mbmssim pdcchsim pucchsim prachsim syncsim)

  add_executable(${myExe}
    ${OPENAIR1_DIR}/SIMULATION/LTE_PHY/${myExe}.c
    ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
    ${OPENAIR_DIR}/common/utils/backtrace.c
    ${OPENAIR_DIR}/common/utils/system.c
    ${OPENAIR_DIR}/common/utils/utils.c
    ${OPENAIR_DIR}/common/utils/lte/ue_power.c
    ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
    ${OPENAIR1_DIR}/PHY/INIT/lte_param_init.c
    ${XFORMS_SOURCE}
    ${T_SOURCE}
    ${CONFIG_SOURCES}
    ${SHLIB_LOADER_SOURCES}
    ${NFAPI_USER_DIR}/nfapi.c
    )
  target_link_libraries (${myExe}
    -Wl,--start-group SIMU_COMMON SIMU UTIL SCHED_LIB SCHED_RU_LIB SCHED_UE_LIB PHY_COMMON PHY_NR_COMMON PHY PHY_UE PHY_RU LFDS ITTI LFDS7 -Wl,--end-group
    pthread m rt ${CONFIG_LIB} ${ATLAS_LIBRARIES}  ${XFORMS_LIBRARIES} ${T_LIB} dl
    )

endforeach(myExe)

add_executable(test_epc_generate_scenario
  ${OPENAIR3_DIR}/TEST/EPC_TEST/generate_scenario.c
  ${OPENAIR3_DIR}/TEST/EPC_TEST/generate_scenario.h
  ${OPENAIR2_DIR}/ENB_APP/enb_config.h
  ${OPENAIR2_DIR}/COMMON/commonDef.h
  ${OPENAIR2_DIR}/COMMON/messages_def.h
  ${OPENAIR3_DIR}/S1AP/s1ap_eNB_defs.h
  )
target_link_libraries (test_epc_generate_scenario
  -Wl,--start-group RRC_LIB S1AP_LIB S1AP_ENB X2AP_LIB X2AP_ENB M2AP_LIB M2AP_ENB M3AP_LIB M3AP_ENB F1AP_LIB F1AP NR_GTPV1U LIB_NAS_UE SECU_CN UTIL HASHTABLE SCTP_CLIENT MME_APP UDP SCHED_LIB PHY LFDS ITTI ${MSC_LIB} L2 -Wl,--end-group pthread m rt crypt sctp ${LIBXML2_LIBRARIES} ${LIBXSLT_LIBRARIES} ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${CONFIG_LIB}
  )

add_executable(test_epc_play_scenario
  ${OPENAIR3_DIR}/TEST/EPC_TEST/play_scenario.c
  ${OPENAIR3_DIR}/TEST/EPC_TEST/play_scenario_decode.c
  ${OPENAIR3_DIR}/TEST/EPC_TEST/play_scenario_display.c
  ${OPENAIR3_DIR}/TEST/EPC_TEST/play_scenario_fsm.c
  ${OPENAIR3_DIR}/TEST/EPC_TEST/play_scenario_parse.c
  ${OPENAIR3_DIR}/TEST/EPC_TEST/play_scenario_s1ap.c
  ${OPENAIR3_DIR}/TEST/EPC_TEST/play_scenario_s1ap_compare_ie.c
  ${OPENAIR3_DIR}/TEST/EPC_TEST/play_scenario_s1ap_eNB_defs.h
  ${OPENAIR3_DIR}/TEST/EPC_TEST/play_scenario_sctp.c
  ${OPENAIR3_DIR}/TEST/EPC_TEST/play_scenario.h
  ${OPENAIR2_DIR}/COMMON/commonDef.h
  ${OPENAIR2_DIR}/COMMON/messages_def.h
  )
target_include_directories(test_epc_play_scenario PUBLIC /usr/local/share/asn1c)
target_link_libraries (test_epc_play_scenario
  -Wl,--start-group RRC_LIB S1AP_LIB X2AP_LIB X2AP_ENB M3AP_LIB M3AP_ENB F1AP_LIB F1AP NR_GTPV1U LIB_NAS_UE SECU_CN UTIL HASHTABLE SCTP_CLIENT MME_APP UDP SCHED_LIB PHY_NR_COMMON PHY_COMMON PHY PHY_UE LFDS ITTI ${MSC_LIB} -Wl,--end-group pthread m rt crypt sctp ${LIBXML2_LIBRARIES} ${LIBXSLT_LIBRARIES} ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${CONFIG_LIB}
  )


#unitary tests for Core NEtwork pieces
#################################
foreach(myExe s1ap
    secu_knas_encrypt_eia1
    secu_kenb
    aes128_ctr_encrypt
    aes128_ctr_decrypt
    secu_knas_encrypt_eea2
    secu_knas secu_knas_encrypt_eea1
    kdf
    aes128_cmac_encrypt
    secu_knas_encrypt_eia2)
  add_executable(test_${myExe}
    ${OPENAIR3_DIR}/TEST/test_${myExe}.c
    )
  target_link_libraries (test_${myExe}
    -Wl,--start-group SECU_CN UTIL LFDS -Wl,--end-group m rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${CONFIG_LIB}
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
        nr-uesoftmodem dlsim dlsim_tm4 dlsim_tm7 nr-ittisim
        ulsim pbchsim scansim mbmssim pdcchsim pucchsim prachsim
        syncsim nr_ulsim nr_dlsim nr_dlschsim nr_pbchsim nr_pucchsim
        nr_ulschsim ldpctest polartest smallblocktest cu_test du_test
        #all "add_library" definitions
        ITTI RRC_LIB NR_RRC_LIB S1AP_LIB S1AP_ENB X2AP_LIB X2AP_ENB M2AP_LIB M2AP_ENB M3AP_LIB M3AP_ENB F1AP_LIB F1AP
        params_libconfig oai_exmimodevif oai_usrpdevif oai_bladerfdevif oai_lmssdrdevif oai_iqplayer
        oai_eth_transpro oai_mobipass tcp_bridge tcp_bridge_oai
        coding FLPT_MSG ASYNC_IF FLEXRAN_AGENT HASHTABLE MSC UTIL OMG_SUMO
        SECU_OSA SECU_CN SCHED_LIB SCHED_NR_LIB SCHED_RU_LIB SCHED_UE_LIB SCHED_NR_UE_LIB default_sched remote_sched RAL
        NFAPI_COMMON_LIB NFAPI_LIB NFAPI_PNF_LIB NFAPI_VNF_LIB NFAPI_USER_LIB
        PHY_COMMON PHY PHY_UE PHY_NR PHY_NR_COMMON PHY_NR_UE PHY_RU PHY_MEX
        L2 L2_LTE L2_NR L2_LTE_NR L2_UE NR_L2_UE L2_UE_LTE_NR MAC_NR_COMMON MAC_NR MAC_UE_NR NGAP_GNB
        CN_UTILS NR_GTPV1U GTPV1U_OCP SCTP_CLIENT MME_APP UDP LIB_NAS_UE NB_IoT LFDS LFDS7 SIMU_COMMON SIMU SIMU_ETH OPENAIR0_LIB
        ldpc_orig ldpc_optim ldpc_optim8seg ldpc PROTO_AGENT dfts)
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
    COMMAND make -j2 -C ${module_build_path} M=${OPENAIR_BIN_DIR}/${name}
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
  ${OPENAIR2_DIR}/NETWORK_DRIVER/MESH/constant.h
  ${OPENAIR2_DIR}/NETWORK_DRIVER/MESH/ioctl.h
  ${OPENAIR2_DIR}/NETWORK_DRIVER/MESH/rrc_nas_primitives.h
  ${OPENAIR2_DIR}/NETWORK_DRIVER/MESH/RB_TOOL/rb_tool.c
)
target_include_directories(rb_tool PRIVATE ${OPENAIR2_DIR}/NETWORK_DRIVER/MESH/)

# nr-ittisim
###################################################

add_executable(nr-ittisim
  ${rrc_h}
  ${nr_rrc_h}
  ${OPENAIR2_DIR}/GNB_APP/gnb_app.c
  ${OPENAIR2_DIR}/GNB_APP/gnb_config.c
  ${OPENAIR_DIR}/executables/nr-gnb.c
  ${OPENAIR_DIR}/executables/nr-ru.c
  ${OPENAIR2_DIR}/SIMULATION/NR_RRC/itti_sim.c
  ${OPENAIR_DIR}/executables/softmodem-common.c
  ${OPENAIR1_DIR}/SIMULATION/TOOLS/taus.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/common_lib.c
  ${OPENAIR_TARGETS}/ARCH/COMMON/record_player.c
  ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
  ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
  ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
  ${OPENAIR3_DIR}/NAS/UE/nas_ue_task.c
  ${OPENAIR_DIR}/common/utils/utils.c
  ${OPENAIR_DIR}/common/utils/system.c
  ${OPENAIR_DIR}/common/utils/nr/nr_common.c
  ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
  ${OPENAIR_DIR}/common/utils/lte/ue_power.c
  ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
  ${OPENAIR2_DIR}/ENB_APP/flexran_agent_ran_api_to_fix.c
  ${XFORMSINTERFACE_SOURCE}
  ${T_SOURCE}
  ${CONFIG_SOURCES}
  ${SHLIB_LOADER_SOURCES}
  )
target_compile_definitions(nr-ittisim PUBLIC NEW_GTPU)

target_link_libraries (nr-ittisim
  -Wl,--start-group
  UTIL HASHTABLE SCTP_CLIENT UDP SCHED_LIB SCHED_RU_LIB SCHED_NR_LIB PHY_NR PHY PHY_COMMON PHY_NR_COMMON PHY_RU LFDS GTPV1U_OCP SECU_CN SECU_OSA
  ITTI ${FLPT_MSG_LIB} ${ASYNC_IF_LIB} ${FLEXRAN_AGENT_LIB} LFDS7 ${MSC_LIB} ${RAL_LIB} ${NAS_SIM_LIB} RRC_LIB NR_RRC_LIB
  NGAP_LIB NGAP_GNB S1AP_LIB S1AP_ENB L2_LTE_NR L2_NR MAC_NR_COMMON NFAPI_COMMON_LIB NFAPI_LIB NFAPI_VNF_LIB NFAPI_PNF_LIB NFAPI_USER_LIB
  X2AP_LIB X2AP_ENB F1AP_LIB F1AP M2AP_LIB M2AP_ENB M3AP_LIB M3AP_ENB ${PROTO_AGENT_LIB} ${FSPT_MSG_LIB}
  PHY_NR_UE SCHED_NR_UE_LIB NR_L2_UE
  -Wl,--end-group z dl)

target_link_libraries (nr-ittisim ${LIBXML2_LIBRARIES})
target_link_libraries (nr-ittisim pthread m ${CONFIG_LIB} rt crypt ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${NETTLE_LIBRARIES} sctp  ${XFORMS_LIBRARIES} ${PROTOBUF_LIB}  ${CMAKE_DL_LIBS} ${LIBYAML_LIBRARIES} ${ATLAS_LIBRARIES})
target_link_libraries (nr-ittisim ${LIB_LMS_LIBRARIES})
target_link_libraries (nr-ittisim ${T_LIB})

add_dependencies( nr-ittisim ldpc_orig ldpc_optim ldpc_optim8seg ldpc )

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


foreach(file IN ITEMS ${OCT_FILES})
  string(REGEX REPLACE "oct *$" "cc" src ${file})
  add_custom_command(
    OUTPUT ${file}
    DEPENDS ${OCT_DIR}/${src} OPENAIR0_LIB
    COMMAND mkoctfile
    ARGS ${OCT_FLAGS} ${OCT_INCL} ${OCT_LIBS}
    ARGS -o ${file} ${OCT_DIR}/${src}
    COMMENT "Generating ${file}"
    VERBATIM
  )
endforeach(file)

ADD_CUSTOM_TARGET(oarf
   DEPENDS ${OCT_FILES}
)

include (${OPENAIR_DIR}/common/utils/telnetsrv/telnetsrv_CMakeLists.txt)
