set(si5351_src_files
	Si5351C.cpp
)
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} ${C_FLAGS_PROCESSOR} -std=c++11 -fPIC"
)


add_library(Si5351C STATIC ${si5351_src_files})
target_include_directories(Si5351C PUBLIC ../lms7002m ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Si5351C LMS7002M)
