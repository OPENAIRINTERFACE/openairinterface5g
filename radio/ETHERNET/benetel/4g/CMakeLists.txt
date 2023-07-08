pkg_check_modules(libdpdk REQUIRED libdpdk=20.05.0)

add_library(benetel_4g MODULE
            benetel.c
            shared_buffers.c
            low.c
            low_dpdk.c
            dpdk_driver.c
)

target_compile_definitions(benetel_4g PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -I$ENV{RTE_SDK}/$ENV{RTE_TARGET}/include")

SET(DPDK_LIBS "-Wl,-rpath,$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib -Wl,--whole-archive -L$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib -ldpdk -Wl,--no-whole-archive")
target_include_directories(benetel_4g PRIVATE ${libdpdk_INCLUDE_DIRS})
target_link_libraries(benetel_4g PRIVATE ${libdpdk_LIBRARIES})
target_link_libraries(benetel_4g PRIVATE pthread dl rt m numa)
set_target_properties(benetel_4g PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_custom_command(TARGET benetel_4g POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink libbenetel_4g.so liboai_transpro.so
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
