add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/generator_bnProc        ldpc/generator_bnProc)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/generator_bnProc_avx512 ldpc/generator_bnProc_avx512)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/generator_cnProc        ldpc/generator_cnProc)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/generator_cnProc_avx512 ldpc/generator_cnProc_avx512)

# custom target to build all generators
add_custom_target(ldpc_generators)
add_dependencies(ldpc_generators
                 bnProc_gen_avx2
                 bnProc_gen_avx512
                 cnProc_gen_avx2
                 cnProc_gen_avx512)

add_library(ldpc_gen_HEADERS INTERFACE)
target_link_libraries(ldpc_gen_HEADERS INTERFACE
                      bnProc_gen_avx2_HEADERS
                      bnProc_gen_avx512_HEADERS
                      cnProc_gen_avx2_HEADERS
                      cnProc_gen_avx512_HEADERS)
