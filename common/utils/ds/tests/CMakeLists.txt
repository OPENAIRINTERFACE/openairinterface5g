# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_executable(test_seq_arr test_seq_array.c)
target_link_libraries(test_seq_arr ds alg)
add_dependencies(tests test_seq_arr)
add_test(NAME test_seq_arr COMMAND test_seq_arr) # no options required

add_executable(test_hashtable test_hashtable.cpp)
add_dependencies(tests test_hashtable)
target_link_libraries(test_hashtable PRIVATE ds GTest::gtest)
add_test(NAME test_hashtable COMMAND ./test_hashtable)

add_executable(test_spsc_q test_spsc_q.cpp)
target_link_libraries(test_spsc_q ds GTest::gtest)
add_dependencies(tests test_spsc_q)
add_test(NAME test_spsc_q COMMAND test_spsc_q)
add_executable(test_spsc_q_perf test_spsc_q_perf.cpp)
target_link_libraries(test_spsc_q_perf PRIVATE ds benchmark::benchmark)

add_executable(test_work_q test_work_q.cpp)
target_link_libraries(test_work_q ds GTest::gtest)
add_dependencies(tests test_work_q)
add_test(NAME test_work_q COMMAND test_work_q)

add_executable(test_seqlock test_seqlock.c)
target_link_libraries(test_seqlock PRIVATE pthread)
add_dependencies(tests test_seqlock)
add_test(NAME test_seqlock COMMAND test_seqlock)

add_executable(test_queue_t test_queue_t.c)
target_link_libraries(test_queue_t PRIVATE ds pthread)
add_dependencies(tests test_queue_t)
add_test(NAME test_queue_t COMMAND test_queue_t)
