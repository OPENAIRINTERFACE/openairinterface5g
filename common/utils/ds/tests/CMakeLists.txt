add_executable(test_seq_arr test_seq_array.c)
target_link_libraries(test_seq_arr ds alg)
add_dependencies(tests test_seq_arr)
add_test(NAME test_seq_arr COMMAND test_seq_arr) # no options required
