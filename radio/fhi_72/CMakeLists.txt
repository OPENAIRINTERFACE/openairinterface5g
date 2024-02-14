# use env var PKG_CONFIG_PATH to override paths to libdpdk.pc
pkg_check_modules(dpdk REQUIRED libdpdk)
pkg_check_modules(numa REQUIRED numa)

find_package(xran 5.1.2 EXACT REQUIRED) # E release -> 5

# Ignore xran-specific warning: we don't care/can't change the following warning, so suppress
# alignment 1 of 'struct XYZ' is less than 2
add_compile_options(-Wno-packed-not-aligned)

add_library(oran_fhlib_5g MODULE
            oran_isolate.c
            oaioran.c
            oran-config.c
            oran-init.c
)

set_target_properties(oran_fhlib_5g PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -march=native")
target_link_libraries(oran_fhlib_5g PRIVATE xran::xran)
target_link_libraries(oran_fhlib_5g PRIVATE ${dpdk_LINK_LIBRARIES})
target_include_directories(oran_fhlib_5g PRIVATE ${dpdk_INCLUDE_DIRS})
target_link_libraries(oran_fhlib_5g PRIVATE pthread dl rt m numa)
target_link_libraries(oran_fhlib_5g PRIVATE ${T_LIB})

# TODO: can be removed?
target_include_directories(oran_fhlib_5g PRIVATE ${xran_INCLUDE_DIRS}/../src/)

set_target_properties(oran_fhlib_5g PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_custom_command(TARGET oran_fhlib_5g POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink liboran_fhlib_5g.so liboai_transpro.so
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
