add_library(nr_rlc_core OBJECT
            nr_rlc_entity.c
            nr_rlc_entity_am.c
            nr_rlc_entity_tm.c
            nr_rlc_entity_um.c
            nr_rlc_pdu.c
            nr_rlc_sdu.c)
target_include_directories(nr_rlc_core PRIVATE ../../..)
target_link_libraries(nr_rlc_core PRIVATE ${T_LIB})
add_library(nr_rlc STATIC
            asn1_utils.c
            nr_rlc_oai_api.c
            nr_rlc_ue_manager.c
)
# L2_NR: for PDCP
target_link_libraries(nr_rlc PUBLIC pthread f1ap nr_rlc_core UTIL ${T_LIB})
target_include_directories(nr_rlc PRIVATE ../../..)
target_link_libraries(nr_rlc PRIVATE asn1_lte_rrc asn1_nr_rrc)

if(ENABLE_TESTS)
  add_subdirectory(tests)
endif()
