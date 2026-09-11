# SPDX-License-Identifier: LicenseRef-CSSL-1.0

message(STATUS "Building with support for WLS")
add_library(wls_integration_lib
        wls_pnf.c
        wls_vnf.c
        wls_common.c
)

find_library(WLS_LIB wls REQUIRED)
message(STATUS "  wls library found in ${WLS_LIB}")

find_library(hugetlbfs_LIB hugetlbfs REQUIRED)
message(STATUS "  hugetlbfs library found in ${hugetlbfs_LIB}")

pkg_check_modules(dpdk REQUIRED libdpdk)
if(dpdk_VERSION VERSION_LESS 22.11)
    message(FATAL_ERROR "We require min DPDK version 22.11, but found ${dpdk_VERSION}")
endif()
target_link_libraries(wls_integration_lib PRIVATE ${dpdk_LINK_LIBRARIES})
target_include_directories(wls_integration_lib PRIVATE ${dpdk_INCLUDE_DIRS})
target_include_directories(wls_integration_lib PRIVATE include/)

target_compile_definitions(wls_integration_lib PUBLIC ENABLE_WLS)
target_link_libraries(wls_integration_lib PRIVATE nfapi_nr_lib)
target_link_libraries(wls_integration_lib PRIVATE nr_fapi_p5 nr_fapi_p7)
target_link_libraries(wls_integration_lib PRIVATE nfapi_lte_vnf_lib nfapi_nr_vnf_lib nfapi_pnf_lib)
target_link_libraries(wls_integration_lib PRIVATE "${WLS_LIB}" "${hugetlbfs_LIB}")
