# SPDX-License-Identifier: LicenseRef-CSSL-1.0

set(DPDK_TEST_ARGS "--no-huge" "--iova-mode=va" "--no-pci")

add_executable(test_fh_timer test_fh_timer.c)
target_link_libraries(test_fh_timer PRIVATE fh_timer PkgConfig::dpdk)
add_dependencies(tests test_fh_timer)
add_test(NAME test_fh_timer COMMAND test_fh_timer ${DPDK_TEST_ARGS} "--file-prefix=test_fh_timer")
set_property(TEST test_fh_timer PROPERTY LABELS fronthaul)

add_executable(test_fh_timer_jumps test_fh_timer_jumps.c)
target_link_libraries(test_fh_timer_jumps PRIVATE fh_timer PkgConfig::dpdk "-Wl,--wrap=clock_gettime")
add_dependencies(tests test_fh_timer_jumps)
add_test(NAME test_fh_timer_jumps COMMAND test_fh_timer_jumps ${DPDK_TEST_ARGS} "--file-prefix=test_fh_timer_jumps")
set_property(TEST test_fh_timer_jumps PROPERTY LABELS fronthaul)

add_executable(test_fh_timer_drift test_fh_timer_drift.c)
target_link_libraries(test_fh_timer_drift PRIVATE fh_timer PkgConfig::dpdk m)
add_dependencies(tests test_fh_timer_drift)
#add_test(NAME test_fh_timer_drift COMMAND test_fh_timer_drift ${DPDK_TEST_ARGS} "--file-prefix=test_fh_timer_drift")
#set_property(TEST test_fh_timer_drift PROPERTY LABELS fronthaul)

add_executable(test_fh_recv_drift test_fh_recv_drift.c)
target_link_libraries(test_fh_recv_drift PRIVATE fh_recv fh_timer PkgConfig::dpdk m)
add_dependencies(tests test_fh_recv_drift)
#add_test(NAME test_fh_recv_drift COMMAND test_fh_recv_drift ${DPDK_TEST_ARGS} "--file-prefix=test_fh_recv_drift" "--vdev=net_null0" "--vdev=net_null1")
#set_property(TEST test_fh_recv_drift PROPERTY LABELS fronthaul)

add_executable(test_fh_send test_fh_send.c)
target_link_libraries(test_fh_send PRIVATE fh_send PkgConfig::dpdk)
add_dependencies(tests test_fh_send)
add_test(NAME test_fh_send COMMAND test_fh_send ${DPDK_TEST_ARGS} "--file-prefix=test_fh_send" "--vdev=net_null0")
set_property(TEST test_fh_send PROPERTY LABELS fronthaul)

add_executable(test_fh_recv test_fh_recv.c)
target_link_libraries(test_fh_recv PRIVATE fh_recv PkgConfig::dpdk)
add_dependencies(tests test_fh_recv)
add_test(NAME test_fh_recv COMMAND test_fh_recv ${DPDK_TEST_ARGS} "--file-prefix=test_fh_recv" "--vdev=net_null0" "--vdev=net_null1")
set_property(TEST test_fh_recv PROPERTY LABELS fronthaul)
