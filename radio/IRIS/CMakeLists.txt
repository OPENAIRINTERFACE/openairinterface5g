find_package(SoapySDR REQUIRED)

add_library(oai_irisdevif MODULE iris_lib.cpp)
target_link_libraries(oai_irisdevif PRIVATE SoapySDR)
set_target_properties(oai_irisdevif PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
