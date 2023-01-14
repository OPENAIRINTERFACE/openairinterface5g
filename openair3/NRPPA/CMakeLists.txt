add_subdirectory(MESSAGES)

add_executable(test_nrppa ${OPENAIR3_DIR}/NRPPA/test_nrppa.c )
target_link_libraries(test_nrppa asn1_nrppa)
  
