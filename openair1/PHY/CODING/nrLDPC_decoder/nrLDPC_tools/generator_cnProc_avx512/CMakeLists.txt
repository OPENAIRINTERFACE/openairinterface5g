add_executable(cnProc_gen_avx512
               cnProc_gen_BG1_avx512.c
               cnProc_gen_BG2_avx512.c
               main.c)
target_compile_options(cnProc_gen_avx512 PRIVATE -W -Wall -mavx2)

#set(cnProc_avx512_headers
#    cnProc_avx512/nrLDPC_cnProc_BG1_R13_AVX512.h
#    cnProc_avx512/nrLDPC_cnProc_BG1_R23_AVX512.h
#    cnProc_avx512/nrLDPC_cnProc_BG1_R89_AVX512.h
#    cnProc_avx512/nrLDPC_cnProc_BG2_R13_AVX512.h
#    cnProc_avx512/nrLDPC_cnProc_BG2_R15_AVX512.h
#    cnProc_avx512/nrLDPC_cnProc_BG2_R23_AVX512.h)

add_custom_command(TARGET cnProc_gen_avx512 POST_BUILD
  #OUTPUT ${cnProc_avx512_headers}
  COMMAND ${CMAKE_COMMAND} -E make_directory cnProc_avx512
  COMMAND cnProc_gen_avx512 .
  DEPENDS cnProc_gen_avx512
  COMMENT "Generating LDPC cnProc header files for AVX512"
)

add_library(cnProc_gen_avx512_HEADERS INTERFACE)
target_include_directories(cnProc_gen_avx512_HEADERS INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(cnProc_gen_avx512_HEADERS cnProc_gen_avx512)
