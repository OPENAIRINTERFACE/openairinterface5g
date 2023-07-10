find_library(ori NAMES "libori.so" REQUIRED)
if(NOT ori)
  message(FATAL_ERROR "cannot locate libori.so, required to build aw2sori_transpro")
endif()
# force that oai_eth_transpro is built as well, as aw2sori_transpro requires it
if(NOT OAI_ETHERNET)
  message(FATAL_ERROR "aw2sori_transpro requires oai_eth_transpro, please enable OAI_ETHERNET (-DOAI_ETHERNET=ON for cmake, or ./build_oai -t Ethernet)")
endif()

add_library(aw2sori_transpro MODULE oaiori.c)
target_compile_options(aw2sori_transpro PRIVATE -shared -fPIC -msse4 -g -ggdb -DLITE_COMPILATION)
target_link_libraries(aw2sori_transpro PRIVATE ori oai_eth_transpro)
set_target_properties(aw2sori_transpro PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_custom_command(TARGET aw2sori_transpro POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink libaw2sori_transpro.so libthirdparty_transpro.so
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
