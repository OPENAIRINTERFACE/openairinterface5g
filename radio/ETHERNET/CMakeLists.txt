add_library(oai_eth_transpro MODULE
            ethernet_lib.c
            eth_udp.c
            eth_raw.c
)
target_link_libraries(oai_eth_transpro PRIVATE ${T_LIB})
target_include_directories(oai_eth_transpro PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(oai_eth_transpro PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_custom_command(TARGET oai_eth_transpro POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink liboai_eth_transpro.so liboai_transpro.so
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(benetel)
