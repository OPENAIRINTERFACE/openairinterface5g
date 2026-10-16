add_library(MAC_NR_COMMON
            nr_mac_common.c
            nr_mac_common_tdd.c
            nr_compute_tbs_common.c
            ue_capability_handler.c
)
target_link_libraries(MAC_NR_COMMON PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

if(ENABLE_TESTS)
  add_subdirectory(tests)
endif()
