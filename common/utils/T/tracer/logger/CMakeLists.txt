add_library(tracer_logger STATIC
            logger.c textlog.c framelog.c ttilog.c timelog.c ticklog.c iqlog.c
            iqdotlog.c ticked_ttilog.c throughputlog.c)
target_link_libraries(tracer_logger PRIVATE pthread tracer_filter)
target_include_directories(tracer_logger PRIVATE ..)
