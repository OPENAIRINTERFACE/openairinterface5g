pkg_check_modules(libdpdk REQUIRED libdpdk=20.05.0)

add_library(benetel_5g MODULE
            benetel.c
            shared_buffers.c
            low.c
            low_dpdk.c
            dpdk_driver.c
)

SET(DPDK_LIBS "-Wl,-rpath,$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib -Wl,--whole-archive -L$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib -ldpdk -Wl,--no-whole-archive")
target_include_directories(benetel_5g PRIVATE ${libdpdk_INCLUDE_DIRS})
target_link_libraries(benetel_5g PRIVATE ${libdpdk_LIBRARIES})
target_link_libraries(benetel_5g PRIVATE pthread dl rt m numa)
set_target_properties(benetel_5g PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -I$ENV{RTE_SDK}/$ENV{RTE_TARGET}/include")
set_target_properties(benetel_5g PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_custom_command(TARGET benetel_5g POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink libbenetel_5g.so liboai_transpro.so
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
