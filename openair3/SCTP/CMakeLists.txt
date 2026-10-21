# SPDX-License-Identifier: LicenseRef-CSSL-1.0

find_package(sctp REQUIRED)
add_library(SCTP_CLIENT STATIC
  sctp_common.c
  sctp_eNB_task.c
  sctp_eNB_itti_messaging.c
)
target_include_directories(SCTP_CLIENT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SCTP_CLIENT PRIVATE sctp::sctp)
target_link_libraries(SCTP_CLIENT PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
