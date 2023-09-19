if (CROSS_COMPILE)
  find_package(cnProc_gen_128)
  find_package(cnProc_gen_avx2)
else()
  add_executable(cnProc_gen_avx2
                 cnProc_gen_BG1_avx2.c
                 cnProc_gen_BG2_avx2.c
                 main.c)
  add_executable(cnProc_gen_128
                 cnProc_gen_BG1_128.c
                 cnProc_gen_BG2_128.c
                 main128.c)
  target_compile_options(cnProc_gen_avx2 PRIVATE -W -Wall )
  target_compile_options(cnProc_gen_128 PRIVATE -W -Wall )
  export(TARGETS cnProc_gen_avx2 FILE
          "${CMAKE_BINARY_DIR}/cnProc_gen_avx2Config.cmake")
  export(TARGETS cnProc_gen_128 FILE
          "${CMAKE_BINARY_DIR}/cnProc_gen_128Config.cmake")
endif()  

set(cnProc_headers
    cnProc/nrLDPC_cnProc_BG1_R13_AVX2.h
    cnProc/nrLDPC_cnProc_BG1_R23_AVX2.h
    cnProc/nrLDPC_cnProc_BG1_R89_AVX2.h
    cnProc/nrLDPC_cnProc_BG2_R13_AVX2.h
    cnProc/nrLDPC_cnProc_BG2_R15_AVX2.h
    cnProc/nrLDPC_cnProc_BG2_R23_AVX2.h)

set(cnProc128_headers
    cnProc/nrLDPC_cnProc_BG1_R13_128.h
    cnProc/nrLDPC_cnProc_BG1_R23_128.h
    cnProc/nrLDPC_cnProc_BG1_R89_128.h
    cnProc/nrLDPC_cnProc_BG2_R13_128.h
    cnProc/nrLDPC_cnProc_BG2_R15_128.h
    cnProc/nrLDPC_cnProc_BG2_R23_128.h)

add_custom_command(
  # TARGET cnProc_gen_avx2 POST_BUILD
  OUTPUT ${cnProc_headers}
  COMMAND ${CMAKE_COMMAND} -E make_directory cnProc
  COMMAND cnProc_gen_avx2 .
  DEPENDS cnProc_gen_avx2
  COMMENT "Generating LDPC cnProc header files for AVX2"
)
add_custom_target(cnProc_gen_avx2_files DEPENDS ${cnProc_headers})

add_custom_command(
  # TARGET cnProc_gen_128 POST_BUILD
  OUTPUT ${cnProc128_headers}
  COMMAND ${CMAKE_COMMAND} -E make_directory cnProc128
  COMMAND cnProc_gen_128 .
  DEPENDS cnProc_gen_128
  COMMENT "Generating LDPC cnProc header files for 128-bit SIMD"
)
add_custom_target(cnProc_gen_128_files DEPENDS ${cnProc128_headers})

add_library(cnProc_gen_avx2_HEADERS INTERFACE)
target_include_directories(cnProc_gen_avx2_HEADERS INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(cnProc_gen_avx2_HEADERS cnProc_gen_avx2_files)

add_library(cnProc_gen_128_HEADERS INTERFACE)
target_include_directories(cnProc_gen_128_HEADERS INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(cnProc_gen_128_HEADERS cnProc_gen_128_files)

