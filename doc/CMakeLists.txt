add_boolean_option(GENERATE_DOXYGEN False "Generate source code doc using doxygen" OFF)

if(GENERATE_DOXYGEN)
  find_package(Doxygen REQUIRED)

  configure_file(Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
else()
  message(STATUS "No Doxygen documentation requested")
endif()
