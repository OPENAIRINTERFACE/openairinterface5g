# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_executable(test_tpool_vs_actors test_tpool_vs_actors.c)
target_link_libraries(test_tpool_vs_actors PRIVATE thread-pool pthread LOG minimal_lib actor)
add_test(NAME test_tpool_vs_actors COMMAND ./test_tpool_vs_actors)
add_dependencies(tests test_tpool_vs_actors)

add_executable(test_fsn test_fsn.cpp)
add_dependencies(tests test_fsn)
target_link_libraries(test_fsn PRIVATE utils GTest::gtest)
add_test(NAME test_fsn COMMAND ./test_fsn)

add_executable(test_utils test_utils.cpp)
add_dependencies(tests test_utils)
target_link_libraries(test_utils PRIVATE utils GTest::gtest)
add_test(NAME test_utils COMMAND ./test_utils)

add_executable(test_bits test_bits.c)
add_dependencies(tests test_bits)
add_test(NAME test_bits COMMAND ./test_bits)
