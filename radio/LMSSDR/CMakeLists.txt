find_package(LimeSuite REQUIRED)

add_library(oai_lmssdrdevif MODULE lms_lib.cpp)
target_link_libraries(oai_lmssdrdevif LimeSuite )
set_target_properties(oai_lmssdrdevif PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
