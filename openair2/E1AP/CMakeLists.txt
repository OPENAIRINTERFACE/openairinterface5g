add_subdirectory(MESSAGES)

add_library(e1ap e1ap.c e1ap_common.c)
target_link_libraries(e1ap
                      PUBLIC asn1_e1ap f1ap
                      PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs asn1_f1ap UTIL e1_if)
target_include_directories(e1ap PUBLIC ${CMAKE_CURRENT_DIR})
