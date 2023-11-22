add_boolean_option(OAI_AERIAL OFF "Activate OAI's AERIAL driver" OFF)
if (OAI_AERIAL)
    message(STATUS "Building with support for Aerial")

    target_sources(
            nr-softmodem
            PRIVATE
            ${OPENAIR_DIR}/nfapi/oai_integration/aerial/fapi_nvIPC.c
            ${OPENAIR_DIR}/nfapi/oai_integration/aerial/fapi_vnf_p7.c
            ${OPENAIR_DIR}/nfapi/oai_integration/aerial/fapi_vnf_p5.c )

    find_library(NVIPC_LIB nvipc REQUIRED)
    if (NOT NVIPC_LIB)
        message(FATAL_ERROR "-- nvipc library not found")
    else ()
        message(STATUS "  nvipc library found in ${NVIPC_LIB}")
        #add_library(nvipc   STATIC  SHARED IMPORTED GLOBAL)
    endif ()

    find_library(NVLOG_LIB nvlog REQUIRED)
    if (NOT NVLOG_LIB)
        message(FATAL_ERROR "-- nvlog library not found")
    else ()
        message(STATUS "  nvlog library found in ${NVLOG_LIB}")
    endif ()

    add_compile_definitions(ENABLE_L2_SLT_RSP)
    add_compile_definitions(FAPI_BYTE_ORDERING_BIG_ENDIAN)
    add_compile_definitions(ENABLE_AERIAL)
    target_link_libraries(nr-softmodem PRIVATE  "${NVLOG_LIB}" "${NVIPC_LIB}")
else()
    message(STATUS "No Support for Aerial")
endif()