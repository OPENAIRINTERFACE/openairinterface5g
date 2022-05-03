rm -rf $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/ldpc_gen_files
mkdir $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/ldpc_gen_files
mkdir $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/ldpc_gen_files/cnProc
mkdir $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/ldpc_gen_files/cnProc_avx512
mkdir $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/ldpc_gen_files/bnProc
mkdir $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/ldpc_gen_files/bnProc_avx512
mkdir $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/ldpc_gen_files/bnProcPc
mkdir $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/ldpc_gen_files/bnProcPc_avx512
cd $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/generator_bnProc; make;./bnProc_gen_avx2
cd $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/generator_cnProc; make;./cnProc_gen_avx2
cd $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/generator_bnProc_avx512;make;./bnProc_gen_avx512
cd $OPENAIR_HOME/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/generator_cnProc_avx512;make;./cnProc_gen_avx512
