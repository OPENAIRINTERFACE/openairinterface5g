# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_subdirectory(5GS)

add_library(nr_ue_tun STATIC nr_ue_tun.c)
target_include_directories(nr_ue_tun PUBLIC .)
target_link_libraries(nr_ue_tun PUBLIC utils)

add_library(nr_nas STATIC nr_nas_msg.c)
target_link_libraries(nr_nas PUBLIC asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)
target_link_libraries(nr_nas PUBLIC
    fgs_lib
    fgs_5gmm_lib
    fgs_5gsm_lib
    fgs_5gmm_ies_lib
    nas_common_ies_lib
    nas_utils
    ds
    UTIL
    SECURITY
    nr_ue_tun
)

target_include_directories(nr_nas PUBLIC . 5GS/)
