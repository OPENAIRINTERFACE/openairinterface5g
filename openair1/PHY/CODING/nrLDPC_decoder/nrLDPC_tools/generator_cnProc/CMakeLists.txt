add_executable(cnProc_gen_avx2
               cnProc_gen_BG1_avx2.c
               cnProc_gen_BG2_avx2.c
               main.c)
target_compile_options(cnProc_gen_avx2 PRIVATE -W -Wall -mavx2)

#set(cnProc_headers
#    cnProc/rLDPC_cnProc_BG1_R13_AVX2.h
#    cnProc/rLDPC_cnProc_BG1_R23_AVX2.h
#    cnProc/rLDPC_cnProc_BG1_R89_AVX2.h
#    cnProc/rLDPC_cnProc_BG2_R13_AVX2.h
#    cnProc/rLDPC_cnProc_BG2_R15_AVX2.h
#    cnProc/rLDPC_cnProc_BG2_R23_AVX2.h)

add_custom_command(TARGET cnProc_gen_avx2 POST_BUILD
  #OUTPUT ${cnProc_headers}
  COMMAND ${CMAKE_COMMAND} -E make_directory cnProc
  COMMAND cnProc_gen_avx2 .
  DEPENDS cnProc_gen_avx2
  COMMENT "Generating LDPC cnProc header files for AVX2"
)

add_library(cnProc_gen_avx2_HEADERS INTERFACE)
target_include_directories(cnProc_gen_avx2_HEADERS INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(cnProc_gen_avx2_HEADERS cnProc_gen_avx2)
