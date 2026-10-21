# SPDX-License-Identifier: LicenseRef-CSSL-1.0

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)
add_executable(test_xran_pkt test_xran_pkt.c)
target_link_libraries(test_xran_pkt PRIVATE xran_pkt PkgConfig::dpdk)
target_compile_options(test_xran_pkt PRIVATE ${dpdk_CFLAGS})
target_include_directories(test_xran_pkt PRIVATE ../)
add_dependencies(tests test_xran_pkt)
add_test(NAME test_xran_pkt COMMAND test_xran_pkt --no-pci --no-huge)
set_property(TEST test_xran_pkt PROPERTY LABELS fronthaul)

add_executable(xran_pcap_dump xran_pcap_dump.c)
target_link_libraries(xran_pcap_dump PRIVATE xran_pkt PkgConfig::dpdk ${PCAP_LIBRARIES})
target_compile_options(xran_pcap_dump PRIVATE ${dpdk_CFLAGS} -march=native)
target_include_directories(xran_pcap_dump PRIVATE ../ ${PCAP_INCLUDE_DIRS})
add_dependencies(tests xran_pcap_dump)

# Force xran_pcap_dump to use real captured fronthaul trace 
# Similar approach to test_oru_pcap_1/frag in ../../oru/tests, and
add_test(NAME test_xran_pcap_dump_real_capture
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../oru/tests/run_oru_pcap_test.sh
                 https://github.com/bpodrygajlo/openairinterface5g/releases/download/fhi72-pcaps/capture-oai-odu-4x4-273prb-truncated.pcap.xz
                 $<TARGET_FILE:xran_pcap_dump>)
set_property(TEST test_xran_pcap_dump_real_capture PROPERTY LABELS fronthaul)

add_executable(test_xran_pkt_bfw test_xran_pkt_bfw.c)
target_link_libraries(test_xran_pkt_bfw PRIVATE xran_pkt)
target_include_directories(test_xran_pkt_bfw PRIVATE ../)
add_dependencies(tests test_xran_pkt_bfw)
add_test(NAME test_xran_pkt_bfw COMMAND test_xran_pkt_bfw)
set_property(TEST test_xran_pkt_bfw PROPERTY LABELS fronthaul)
