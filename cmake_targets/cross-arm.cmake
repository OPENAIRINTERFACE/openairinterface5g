set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   /usr/bin/aarch64-linux-gnu-gcc-9)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++-9)

set(CROSS_COMPILE 1)
set(bnProc_gen_128_DIR    ${CMAKE_CURRENT_BINARY_DIR}/${NATIVE_DIR}) # /../build)
set(bnProc_gen_avx2_DIR   ${CMAKE_CURRENT_BINARY_DIR}/${NATIVE_DIR}) # /../build)
set(bnProc_gen_avx512_DIR ${CMAKE_CURRENT_BINARY_DIR}/${NATIVE_DIR}) # /../build)
set(cnProc_gen_128_DIR    ${CMAKE_CURRENT_BINARY_DIR}/${NATIVE_DIR}) # /../build)
set(cnProc_gen_avx2_DIR   ${CMAKE_CURRENT_BINARY_DIR}/${NATIVE_DIR}) # /../build)
set(cnProc_gen_avx512_DIR ${CMAKE_CURRENT_BINARY_DIR}/${NATIVE_DIR}) # /../build)
set(genids_DIR            ${CMAKE_CURRENT_BINARY_DIR}/${NATIVE_DIR}) # /../build)
set(_check_vcd_DIR        ${CMAKE_CURRENT_BINARY_DIR}/${NATIVE_DIR}) # /../build)

