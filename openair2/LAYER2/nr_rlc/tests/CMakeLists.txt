
foreach(TESTNUM RANGE 1 17)
  add_executable(nr_rlc_test_${TESTNUM} test.c)
  target_link_libraries(nr_rlc_test_${TESTNUM} PRIVATE nr_rlc_core pthread UTIL ${T_LIB} minimal_lib)
  target_compile_definitions(nr_rlc_test_${TESTNUM} PRIVATE TEST="test${TESTNUM}.h")
  #target_include_directories(nr_rlc_test_${TESTNUM} PRIVATE LOG)
  #target_compile_options(nr_rlc_test_${TESTNUM} PRIVATE -g --coverage -Wall)
  add_dependencies(tests nr_rlc_test_${TESTNUM})
endforeach()

add_test(NAME nr_rlc_tests
  COMMAND exec_nr_rlc_test.sh ${CMAKE_CURRENT_BINARY_DIR}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

