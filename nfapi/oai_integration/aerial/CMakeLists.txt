# SPDX-License-Identifier: LicenseRef-CSSL-1.0

if (OAI_AERIAL)
    message(STATUS "Building with support for Aerial")
    add_library(aerial_lib
            fapi_nvIPC.c
            fapi_vnf_p7.c
    )

    find_library(NVIPC_LIB nvipc REQUIRED)
    if (NOT NVIPC_LIB)
        message(FATAL_ERROR "-- nvipc library not found")
    else ()
        message(STATUS "  nvipc library found in ${NVIPC_LIB}")
    endif ()

    find_library(NVLOG_LIB nvlog REQUIRED)
    if (NOT NVLOG_LIB)
        message(FATAL_ERROR "-- nvlog library not found")
    else ()
        message(STATUS "  nvlog library found in ${NVLOG_LIB}")
    endif ()
    target_compile_definitions(aerial_lib PUBLIC ENABLE_L2_SLT_RSP)
    target_link_libraries(aerial_lib PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)
    target_link_libraries(aerial_lib PRIVATE nr_fapi_p5 nr_fapi_p7)
    target_link_libraries(aerial_lib PRIVATE nfapi_nr_vnf_lib)
    target_link_libraries(aerial_lib PRIVATE "${NVLOG_LIB}" "${NVIPC_LIB}")
else ()
    message(STATUS "No Support for Aerial")
endif ()
