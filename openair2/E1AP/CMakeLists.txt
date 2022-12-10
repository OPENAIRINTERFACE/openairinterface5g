add_subdirectory(MESSAGES)

add_library(E1AP e1ap.c)
target_link_libraries(E1AP
                        PUBLIC asn1_e1ap
                        PRIVATE UTIL asn1_nr_rrc asn1_lte_rrc)
target_include_directories(E1AP PUBLIC ${CMAKE_CURRENT_DIR})
