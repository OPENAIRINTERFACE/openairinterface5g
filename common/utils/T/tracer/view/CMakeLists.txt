add_library(tracer_view STATIC
            stdout.c textlist.c xy.c tti.c time.c ticktime.c scrolltti.c)
target_link_libraries(tracer_view PRIVATE pthread)
target_include_directories(tracer_view PRIVATE ../logger)
target_include_directories(tracer_view PUBLIC ..)
