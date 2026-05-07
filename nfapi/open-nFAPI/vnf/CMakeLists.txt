# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_library(nfapi_vnf_common_lib
        src/vnf_common.c
        src/vnf_interface_common.c
        src/vnf_p7_common.c
        src/vnf_p7_interface_common.c)
target_link_libraries(nfapi_vnf_common_lib PUBLIC nfapi_lte_lib)
target_include_directories(nfapi_vnf_common_lib PUBLIC public_inc)
target_include_directories(nfapi_vnf_common_lib PUBLIC inc)

add_library(nfapi_lte_vnf_lib
        src/vnf_lte.c
        src/vnf_interface_lte.c
        src/vnf_p7_lte.c
        src/vnf_p7_interface_lte.c)
target_link_libraries(nfapi_lte_vnf_lib PUBLIC nfapi_vnf_common_lib nfapi_lte_lib)
target_link_libraries(nfapi_lte_vnf_lib PRIVATE asn1_lte_rrc_hdrs)

add_library(nfapi_nr_vnf_lib
        src/vnf_nr.c
        src/vnf_interface_nr.c
        src/vnf_p7_nr.c
        src/vnf_p7_interface_nr.c)
target_link_libraries(nfapi_nr_vnf_lib PRIVATE asn1_nr_rrc_hdrs nr_fapi_p5 nr_fapi_p7)
target_link_libraries(nfapi_nr_vnf_lib PUBLIC nfapi_vnf_common_lib nfapi_nr_lib nfapi_user_lib)
target_include_directories(nfapi_nr_vnf_lib PUBLIC public_inc)
target_include_directories(nfapi_nr_vnf_lib PUBLIC inc)
if(OAI_AERIAL)
    target_compile_definitions(nfapi_lte_vnf_lib PRIVATE ENABLE_AERIAL)
    target_compile_definitions(nfapi_nr_vnf_lib PRIVATE ENABLE_AERIAL)
    target_compile_definitions(nfapi_vnf_common_lib PRIVATE ENABLE_AERIAL)
endif()
if(OAI_WLS)
    target_compile_definitions(nfapi_lte_vnf_lib PRIVATE ENABLE_WLS)
    target_compile_definitions(nfapi_nr_vnf_lib PRIVATE ENABLE_WLS)
    target_compile_definitions(nfapi_vnf_common_lib PRIVATE ENABLE_WLS)
endif()
