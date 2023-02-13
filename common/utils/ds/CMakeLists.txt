add_library(ds OBJECT
            byte_array.c
)

target_include_directories(ds PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
