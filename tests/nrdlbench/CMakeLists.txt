# SPDX-License-Identifier: LicenseRef-CSSL-1.0

# nr_dlbench: standalone gNB DL-scheduler + PHY-TX micro-benchmark.
# Lives under tests/ (not openair1/SIMULATION/NR_PHY) because it drives the full
# MAC scheduler path, not just the PHY.
add_executable(nr_dlbench dlbench.c)
target_link_libraries(nr_dlbench PRIVATE
  -Wl,--start-group UTIL SIMU PHY_NR_COMMON PHY_NR SCHED_NR_LIB MAC_NR_COMMON CONFIG_LIB L2_NR -Wl,--end-group
  m pthread ${T_LIB} ITTI dl physim_common time_management_core
  softmodem_common
  )
target_link_libraries(nr_dlbench PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

if(ENABLE_TESTS)
  add_dependencies(tests nr_dlbench)
  # Functional smoke tests via a wrapper script (room to grow richer checks):
  # run a short benchmark in UM and AM (-A) modes and verify it completes.
  add_test(NAME nr_dlbench_um_functional_test
           COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/nr-dlbench-functional-test.sh $<TARGET_FILE:nr_dlbench> -A 0)
  add_test(NAME nr_dlbench_am_functional_test
           COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/nr-dlbench-functional-test.sh $<TARGET_FILE:nr_dlbench> -A 50)
endif()
