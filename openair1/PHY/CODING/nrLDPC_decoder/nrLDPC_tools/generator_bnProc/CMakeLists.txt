if (CROSS_COMPILE)
  find_package(bnProc_gen_128)
  find_package(bnProc_gen_avx2)
else()
  add_executable(bnProc_gen_avx2
                 bnProc_gen_BG1_avx2.c
                 bnProc_gen_BG2_avx2.c
                 bnProcPc_gen_BG1_avx2.c
                 bnProcPc_gen_BG2_avx2.c
                 main.c)
  add_executable(bnProc_gen_128
                 bnProc_gen_BG1_128.c
                 bnProc_gen_BG2_128.c
                 bnProcPc_gen_BG1_128.c
                 bnProcPc_gen_BG2_128.c
                 main128.c)
  	       
  target_compile_options(bnProc_gen_avx2 PRIVATE -W -Wall )
  
  target_compile_options(bnProc_gen_128 PRIVATE -W -Wall )
  export(TARGETS bnProc_gen_avx2 FILE
          "${CMAKE_BINARY_DIR}/bnProc_gen_avx2Config.cmake")
  export(TARGETS bnProc_gen_128 FILE
          "${CMAKE_BINARY_DIR}/bnProc_gen_128Config.cmake")
endif ()

set(bnProc_headers
  bnProc/nrLDPC_bnProc_BG1_R13_AVX2.h
  bnProc/nrLDPC_bnProc_BG1_R23_AVX2.h
  bnProc/nrLDPC_bnProc_BG1_R89_AVX2.h
  bnProc/nrLDPC_bnProc_BG2_R13_AVX2.h
  bnProc/nrLDPC_bnProc_BG2_R15_AVX2.h
  bnProc/nrLDPC_bnProc_BG2_R23_AVX2.h)

set(bnProcPc_headers
  bnProcPc/nrLDPC_bnProcPc_BG1_R13_AVX2.h
  bnProcPc/nrLDPC_bnProcPc_BG1_R23_AVX2.h
  bnProcPc/nrLDPC_bnProcPc_BG1_R89_AVX2.h
  bnProcPc/nrLDPC_bnProcPc_BG2_R13_AVX2.h
  bnProcPc/nrLDPC_bnProcPc_BG2_R15_AVX2.h
  bnProcPc/nrLDPC_bnProcPc_BG2_R23_AVX2.h)

set(bnProc128_headers
  bnProc/nrLDPC_bnProc_BG1_R13_128.h
  bnProc/nrLDPC_bnProc_BG1_R23_128.h
  bnProc/nrLDPC_bnProc_BG1_R89_128.h
  bnProc/nrLDPC_bnProc_BG2_R13_128.h
  bnProc/nrLDPC_bnProc_BG2_R15_128.h
  bnProc/nrLDPC_bnProc_BG2_R23_128.h)

set(bnProcPc128_headers
  bnProcPc/nrLDPC_bnProcPc_BG1_R13_128.h
  bnProcPc/nrLDPC_bnProcPc_BG1_R23_128.h
  bnProcPc/nrLDPC_bnProcPc_BG1_R89_128.h
  bnProcPc/nrLDPC_bnProcPc_BG2_R13_128.h
  bnProcPc/nrLDPC_bnProcPc_BG2_R15_128.h
  bnProcPc/nrLDPC_bnProcPc_BG2_R23_128.h)


add_custom_command(
  # TARGET bnProc_gen_avx2 POST_BUILD
  OUTPUT ${bnProc_headers} ${bnProcPc_headers}
  COMMAND ${CMAKE_COMMAND} -E make_directory bnProc
  COMMAND ${CMAKE_COMMAND} -E make_directory bnProcPc
  COMMAND bnProc_gen_avx2 .
  DEPENDS bnProc_gen_avx2
  COMMENT "Generating LDPC bnProc header files for AVX2"
)
add_custom_target(bnProc_gen_avx2_files DEPENDS ${bnProc_headers} ${bnProcPc_headers})

add_custom_command(
  # TARGET bnProc_gen_128 POST_BUILD
  OUTPUT ${bnProc128_headers} ${bnProcPc128_headers}
  COMMAND ${CMAKE_COMMAND} -E make_directory bnProc128
  COMMAND ${CMAKE_COMMAND} -E make_directory bnProcPc128
  COMMAND bnProc_gen_128 .
  DEPENDS bnProc_gen_128
  COMMENT "Generating LDPC bnProc header files for 128-bit SIMD"
)
add_custom_target(bnProc_gen_128_files DEPENDS ${bnProc128_headers} ${bnProcPc128_headers})

add_library(bnProc_gen_avx2_HEADERS INTERFACE)
target_include_directories(bnProc_gen_avx2_HEADERS INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(bnProc_gen_avx2_HEADERS bnProc_gen_avx2_files)

add_library(bnProc_gen_128_HEADERS INTERFACE)
target_include_directories(bnProc_gen_128_HEADERS INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(bnProc_gen_128_HEADERS bnProc_gen_128_files)

