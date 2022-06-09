add_executable(bnProc_gen_avx2
               bnProc_gen_BG1_avx2.c
               bnProc_gen_BG2_avx2.c
               bnProcPc_gen_BG1_avx2.c
               bnProcPc_gen_BG2_avx2.c
               main.c)
target_compile_options(bnProc_gen_avx2 PRIVATE -W -Wall -mavx2)

#set(bnProc_headers
#    bnProc/nrLDPC_bnProc_BG1_R13_AVX2.h
#    bnProc/nrLDPC_bnProc_BG1_R23_AVX2.h
#    bnProc/nrLDPC_bnProc_BG1_R89_AVX2.h
#    bnProc/rLDPC_bnProc_BG2_R13_AVX2.h
#    bnProc/rLDPC_bnProc_BG2_R15_AVX2.h
#    bnProc/rLDPC_bnProc_BG2_R23_AVX2.h)
#
#set(bnProcPc_headers
#    bnProcPc/rLDPC_bnProcPc_BG1_R13_AVX2.h
#    bnProcPc/rLDPC_bnProcPc_BG1_R23_AVX2.h
#    bnProcPc/rLDPC_bnProcPc_BG1_R89_AVX2.h
#    bnProcPc/rLDPC_bnProcPc_BG2_R13_AVX2.h
#    bnProcPc/rLDPC_bnProcPc_BG2_R15_AVX2.h
#    bnProcPc/rLDPC_bnProcPc_BG2_R23_AVX2.h)

add_custom_command(TARGET bnProc_gen_avx2 POST_BUILD
  #OUTPUT ${bnProc_headers} ${bnProcPc_headers}
  COMMAND ${CMAKE_COMMAND} -E make_directory bnProc
  COMMAND ${CMAKE_COMMAND} -E make_directory bnProcPc
  COMMAND bnProc_gen_avx2 .
  DEPENDS bnProc_gen_avx2
  COMMENT "Generating LDPC bnProc header files for AVX2"
)

add_library(bnProc_gen_avx2_HEADERS INTERFACE)
target_include_directories(bnProc_gen_avx2_HEADERS INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(bnProc_gen_avx2_HEADERS bnProc_gen_avx2)
