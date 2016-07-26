cmake_minimum_required(VERSION 2.8)

project(NAS_SIM_TOOLS)


ENABLE_LANGUAGE(C)
#Sends the -std=c99 flag to the gcc compiler
add_definitions(-std=c99)
add_definitions(-DNAS_UE)

set(OPENAIR_DIR     $ENV{OPENAIR_DIR})
set(OPENAIR1_DIR    $ENV{OPENAIR_DIR}/openair1)
set(OPENAIR2_DIR    $ENV{OPENAIR_DIR}/openair2)
set(OPENAIR3_DIR    $ENV{OPENAIR_DIR}/openair3)
set(OPENAIR3_DIR   $ENV{OPENAIR_DIR}/openair3)
set(OPENAIR_TARGETS $ENV{OPENAIR_DIR}/targets)

#set(EXECUTABLE_OUTPUT_PATH ${OPENAIR_DIR}/targets/bin)


# Add .h files for dependancies
set(usim_SRC
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/usim_data.c
    ${OPENAIR_DIR}/openair3/NAS/UE/API/USIM/usim_api.c
    ${OPENAIR_DIR}/openair3/NAS/UE/API/USIM/aka_functions.c
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/memory.c
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/nas_log.c
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/OctetString.c
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/TLVEncoder.c
    ${OPENAIR_DIR}/common/utils/utils.c
)
set(usim_HDR
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/network.h
    ${OPENAIR_DIR}/openair3/NAS/UE/API/USIM/usim_api.h
    ${OPENAIR_DIR}/openair3/NAS/UE/API/USIM/aka_functions.h
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/memory.h
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/nas_log.h
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/OctetString.h
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/TLVEncoder.h
    ${OPENAIR_DIR}/common/utils/utils.h
)
include_directories(
    ${OPENAIR_DIR}/common/utils
    ${OPENAIR_DIR}/openair3/NAS/UE
    ${OPENAIR_DIR}/openair3/NAS/COMMON
    ${OPENAIR_DIR}/openair3/NAS/UE/API/USIM
    ${OPENAIR_DIR}/openair3/NAS/UE/EMM/
    ${OPENAIR_DIR}/openair3/NAS/UE/ESM/
    ${OPENAIR_DIR}/openair3/NAS/COMMON/IES/
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL
)
ADD_EXECUTABLE(usim  ${usim_SRC} ${usim_HDR})




set(nvram_SRC
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/ue_data.c
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/memory.c
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/nas_log.c
)

set(nvram_HDR
    ${OPENAIR_DIR}/openair3/NAS/UE/EMM/emmData.h
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/memory.h
    ${OPENAIR_DIR}/openair3/NAS/COMMON/userDef.h
)

ADD_EXECUTABLE(nvram  ${nvram_SRC} ${nvram_HDR})

#install (TARGETS usim  DESTINATION ${EXECUTABLE_OUTPUT_PATH})
#install (TARGETS nvram DESTINATION ${EXECUTABLE_OUTPUT_PATH})

#install(CODE "EXECUTE_PROCESS (COMMAND ${OPENAIR_TARGETS}/bin/nvram --gen WORKING_DIRECTORY ${OPENAIR_TARGETS}/bin)")
#install(CODE "EXECUTE_PROCESS (COMMAND ${OPENAIR_TARGETS}/bin/usim  --gen WORKING_DIRECTORY ${OPENAIR_TARGETS}/bin)")
