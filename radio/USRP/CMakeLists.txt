#find_package(Boost REQUIRED)
find_package(UHD REQUIRED)

add_library(oai_usrpdevif MODULE usrp_lib.cpp)
#target_include_directories(oai_usrpdevif PRIVATE Boost::boost)
target_link_libraries(oai_usrpdevif uhd)
set_target_properties(oai_usrpdevif PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
