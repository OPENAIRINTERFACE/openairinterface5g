add_library(ds OBJECT
            byte_array.c
            seq_arr.c
)

target_include_directories(ds PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(ENABLE_TESTS)
  add_subdirectory(tests)
endif()
