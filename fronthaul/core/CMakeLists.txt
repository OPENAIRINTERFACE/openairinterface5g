# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_library(fh_timer fh_timer.c)
target_include_directories(fh_timer PUBLIC ./)
target_link_libraries(fh_timer PRIVATE PkgConfig::dpdk m pthread)
target_link_libraries(fh_timer PUBLIC UTIL)

add_library(fh_send fh_send.c)
target_include_directories(fh_send PUBLIC ./)
target_link_libraries(fh_send PUBLIC fh_timer UTIL)
target_link_libraries(fh_send PRIVATE PkgConfig::dpdk)

add_library(fh_recv fh_recv.c)
target_include_directories(fh_recv PUBLIC ./)
target_link_libraries(fh_recv PRIVATE PkgConfig::dpdk)
target_link_libraries(fh_recv PUBLIC fh_timer UTIL)

if (ENABLE_TESTS)
    add_subdirectory(tests)
endif()

add_library(fh_core INTERFACE)
target_link_libraries(fh_core INTERFACE fh_timer fh_send fh_recv)
target_include_directories(fh_core INTERFACE ./)
