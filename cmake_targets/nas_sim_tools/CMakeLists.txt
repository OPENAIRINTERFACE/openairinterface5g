cmake_minimum_required(VERSION 2.8)

project(NAS_SIM_TOOLS)

include(FindPkgConfig)
pkg_search_module(CONFIG libconfig REQUIRED)
include_directories(${CONFIG_INCLUDE_DIRS})
add_definitions(-std=gnu99)

ENABLE_LANGUAGE(C)

set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} ${C_FLAGS_PROCESSOR} -Werror -Wall -Wstrict-prototypes -Wno-packed-bitfield-compat -g")

set(OPENAIR_DIR     $ENV{OPENAIR_DIR})
set(OPENAIR3_DIR    $ENV{OPENAIR_DIR}/openair3)

set(CONF2UEDATA_LIB_SRC
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/conf_emm.c
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/conf_user_data.c
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/conf_usim.c
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/conf_network.c
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/conf_user_plmn.c
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/conf_parser.c
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/fs.c
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/display.c
    ${OPENAIR_DIR}/openair3/NAS/UE/API/USIM/usim_api.c
    ${OPENAIR_DIR}/openair3/NAS/UE/API/USIM/aka_functions.c
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/memory.c
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/OctetString.c
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL/TLVEncoder.c
    ${OPENAIR_DIR}/common/utils/utils.c
)

set(conf2uedata_HDR
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/conf2uedata.h
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/conf_emm.h
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
    ${OPENAIR_DIR}/openair3/NAS/UE/API/USER
    ${OPENAIR_DIR}/openair3/NAS/UE/API/USIM
    ${OPENAIR_DIR}/openair3/NAS/UE/EMM/
    ${OPENAIR_DIR}/openair3/NAS/UE/ESM/
    ${OPENAIR_DIR}/openair3/NAS/COMMON/IES/
    ${OPENAIR_DIR}/openair3/NAS/COMMON/UTIL
)

# conf2uedata binary

set(conf2uedata_SRC
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/conf2uedata.c
	${CONF2UEDATA_LIB_SRC}
)

add_executable(conf2uedata  ${conf2uedata_SRC} ${conf2uedata_HDR} )
target_link_libraries(conf2uedata ${CONFIG_LIBRARIES})

# usim binary

set(usim_SRC
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/usim.c
	${CONF2UEDATA_LIB_SRC}
)


add_executable(usim  ${usim_SRC} ${conf2uedata_HDR} )
target_link_libraries(usim ${CONFIG_LIBRARIES})

# nvram binary

set(nvram_SRC
    ${OPENAIR_DIR}/openair3/NAS/TOOLS/nvram.c
	${CONF2UEDATA_LIB_SRC}
)


add_executable(nvram  ${nvram_SRC} ${conf2uedata_HDR} )
target_link_libraries(nvram ${CONFIG_LIBRARIES})
