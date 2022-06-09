#!/bin/bash
$(
cd $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools
rm -rf ldpc_gen_files
mkdir ldpc_gen_files
cd ldpc_gen_files
mkdir cnProc
mkdir cnProc_avx512
mkdir bnProc
mkdir bnProc_avx512
mkdir bnProcPc
mkdir bnProcPc_avx512
cd ../generator_bnProc; make; make clean; ./bnProc_gen_avx2
cd ../generator_cnProc; make; make clean; ./cnProc_gen_avx2
cd ../generator_bnProc_avx512; make; make clean; ./bnProc_gen_avx512
cd ../generator_cnProc_avx512; make; make clean; ./cnProc_gen_avx512
)
