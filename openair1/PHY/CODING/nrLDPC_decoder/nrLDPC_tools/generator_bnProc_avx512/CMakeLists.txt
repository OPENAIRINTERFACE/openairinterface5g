if (CROSS_COMPILE)
  find_package(bnProc_gen_avx512)
else()
  add_executable(bnProc_gen_avx512
                 bnProc_gen_BG1_avx512.c
                 bnProc_gen_BG2_avx512.c
                 bnProcPc_gen_BG1_avx512.c
                 bnProcPc_gen_BG2_avx512.c
                 main.c)
  target_compile_options(bnProc_gen_avx512 PRIVATE -W -Wall )
  export(TARGETS bnProc_gen_avx512 FILE
        "${CMAKE_BINARY_DIR}/bnProc_gen_avx512Config.cmake")
endif ()

set(bnProc_avx512_headers
    bnProc_avx512/nrLDPC_bnProc_BG1_R13_AVX512.h
    bnProc_avx512/nrLDPC_bnProc_BG1_R23_AVX512.h
    bnProc_avx512/nrLDPC_bnProc_BG1_R89_AVX512.h
    bnProc_avx512/nrLDPC_bnProc_BG2_R13_AVX512.h
    bnProc_avx512/nrLDPC_bnProc_BG2_R15_AVX512.h
    bnProc_avx512/nrLDPC_bnProc_BG2_R23_AVX512.h)

set(bnProcPc_avx512_headers
    bnProcPc_avx512/nrLDPC_bnProcPc_BG1_R13_AVX512.h
    bnProcPc_avx512/nrLDPC_bnProcPc_BG1_R23_AVX512.h
    bnProcPc_avx512/nrLDPC_bnProcPc_BG1_R89_AVX512.h
    bnProcPc_avx512/nrLDPC_bnProcPc_BG2_R13_AVX512.h
    bnProcPc_avx512/nrLDPC_bnProcPc_BG2_R15_AVX512.h
    bnProcPc_avx512/nrLDPC_bnProcPc_BG2_R23_AVX512.h)

add_custom_command(
  # TARGET bnProc_gen_avx512 POST_BUILD
  OUTPUT ${bnProc_avx512_headers} ${bnProcPc_avx512_headers}
  COMMAND ${CMAKE_COMMAND} -E make_directory bnProc_avx512
  COMMAND ${CMAKE_COMMAND} -E make_directory bnProcPc_avx512
  COMMAND bnProc_gen_avx512 .
  DEPENDS bnProc_gen_avx512
  COMMENT "Generating LDPC bnProc header files for AVX512"
)
add_custom_target(bnProc_gen_avx512_files DEPENDS ${bnProc_avx512_headers} ${bnProcPc_avx512_headers})

add_library(bnProc_gen_avx512_HEADERS INTERFACE)
target_include_directories(bnProc_gen_avx512_HEADERS INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(bnProc_gen_avx512_HEADERS bnProc_gen_avx512_files)

