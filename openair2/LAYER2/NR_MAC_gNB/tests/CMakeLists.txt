# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_executable(test_nr_mac_gnb
  test_nr_mac_gnb.cpp
  test_nr_mac_gnb_common.c
  test_nr_other_sib_ssb_index.c
  test_nr_other_sib_ssb_index.cpp
  ${PHY_INTERFACE_DIR}/queue_t.c)
target_link_libraries(test_nr_mac_gnb PRIVATE L2_NR MAC_NR_COMMON SCHED_NR_LIB CONFIG_LIB UTIL ITTI physim_common time_management_core softmodem_common ${T_LIB} asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs GTest::gtest)
add_dependencies(tests test_nr_mac_gnb)
include(GoogleTest)
# one process per test, see test_nr_mac_gnb.cpp
gtest_discover_tests(test_nr_mac_gnb)
