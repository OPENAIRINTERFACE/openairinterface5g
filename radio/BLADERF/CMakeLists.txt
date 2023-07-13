pkg_check_modules(bladeRF REQUIRED libbladeRF)
add_library(oai_bladerfdevif MODULE bladerf_lib.c)
target_link_libraries(oai_bladerfdevif PRIVATE bladeRF)
target_link_libraries(oai_bladerfdevif PRIVATE UTIL)
set_target_properties(oai_bladerfdevif PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_custom_command(TARGET oai_bladerfdevif POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink liboai_bladerfdevif.so liboai_device.so
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
