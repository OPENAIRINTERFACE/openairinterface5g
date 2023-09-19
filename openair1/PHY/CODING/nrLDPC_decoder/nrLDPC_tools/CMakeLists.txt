add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/generator_bnProc        ldpc/generator_bnProc)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/generator_bnProc_avx512 ldpc/generator_bnProc_avx512)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/generator_cnProc        ldpc/generator_cnProc)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/generator_cnProc_avx512 ldpc/generator_cnProc_avx512)

# custom target to build all generators
add_custom_target(ldpc_generators)

# if (CROSS_COMPILE)
#   find_package(bnProc_gen_avx2)
#   find_package(bnProc_gen_128)
#   find_package(bnProc_gen_avx512)
#   find_package(cnProc_gen_avx2)
#   find_package(cnProc_gen_128)
#   find_package(cnProc_gen_avx512)
# endif()

add_dependencies(ldpc_generators
                 bnProc_gen_avx2_files
                 bnProc_gen_128_files
                 bnProc_gen_avx512_files
                 cnProc_gen_avx2_files
                 cnProc_gen_128_files
                 cnProc_gen_avx512_files)

add_library(ldpc_gen_HEADERS INTERFACE)
target_link_libraries(ldpc_gen_HEADERS INTERFACE
                      bnProc_gen_avx2_HEADERS
                      bnProc_gen_128_HEADERS
                      bnProc_gen_avx512_HEADERS
                      cnProc_gen_avx2_HEADERS
                      cnProc_gen_128_HEADERS
                      cnProc_gen_avx512_HEADERS)
