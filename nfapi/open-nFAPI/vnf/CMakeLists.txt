# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_library(nfapi_vnf_lib
        src/vnf_common.c
        src/vnf_lte.c
        src/vnf_nr.c
        src/vnf_interface_common.c
        src/vnf_interface_lte.c
        src/vnf_interface_nr.c
        src/vnf_p7_common.c
        src/vnf_p7_lte.c
        src/vnf_p7_nr.c
        src/vnf_p7_interface_common.c
        src/vnf_p7_interface_lte.c
        src/vnf_p7_interface_nr.c)
target_link_libraries(nfapi_vnf_lib PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)
target_link_libraries(nfapi_vnf_lib PRIVATE nr_fapi_p5 nr_fapi_p7)
target_link_libraries(nfapi_vnf_lib PUBLIC nfapi_nr_lib nfapi_lte_lib nfapi_user_lib)

if(OAI_AERIAL)
    target_compile_definitions(nfapi_vnf_lib PRIVATE ENABLE_AERIAL)
endif()
target_include_directories(nfapi_vnf_lib PUBLIC public_inc)
target_include_directories(nfapi_vnf_lib PUBLIC inc)
if(OAI_WLS)
    target_compile_definitions(nfapi_vnf_lib PRIVATE ENABLE_WLS)
endif()
