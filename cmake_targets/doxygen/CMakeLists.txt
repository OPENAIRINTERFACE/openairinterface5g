cmake_minimum_required(VERSION 2.8)
include(${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt)
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${OPENAIR_TARGETS}/DOCS/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)
