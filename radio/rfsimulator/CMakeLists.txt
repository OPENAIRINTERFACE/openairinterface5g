add_library(rfsimulator MODULE
            simulator.c
            apply_channelmod.c
            ../../openair1/PHY/TOOLS/signal_energy.c
)
target_link_libraries(rfsimulator PRIVATE SIMU)
set_target_properties(rfsimulator PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(replay_node stored_node.c)
target_link_libraries (replay_node minimal_lib)
