add_library(alg OBJECT
            find.c
            foreach.c
)

target_include_directories(alg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
