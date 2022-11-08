
find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
  configure_file(Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
else()
  add_custom_target(doc
    message(FATAL_ERROR "required to generate doxygen documentation but doxygen not installed")
    )
endif(DOXYGEN_FOUND)

