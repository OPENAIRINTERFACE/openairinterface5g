lte_dfts_sse4: lte_dfts.c
	gcc -O2 -msse4.1 -g -ggdb -o lte_dfts_sse4 lte_dfts.c time_meas.c file_output.c ../../SIMULATION/TOOLS/taus.c -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR2_DIR/COMMON -DMR_MAIN -DNB_ANTENNAS_RX=1 # -DD256STATS #-DD64STATS

lte_dfts_avx2: lte_dfts.c
	gcc -O2 -mavx2 -g -ggdb -o lte_dfts_avx2 lte_dfts.c time_meas.c file_output.c ../../SIMULATION/TOOLS/taus.c -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR2_DIR/COMMON -DMR_MAIN -DNB_ANTENNAS_RX=1 # -DD256STATS #-DD64STATS

lte_dfts_avx2.s: lte_dfts.c
	gcc -O2 -mavx2 -S lte_dfts.c time_meas.c file_output.c ../../SIMULATION/TOOLS/taus.c -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR2_DIR/COMMON -DMR_MAIN -DNB_ANTENNAS_RX=1 # -DD256STATS #-DD64STATS

lte_dfts_sse4.s: lte_dfts.c
	gcc -O2 -msse4.1 -S lte_dfts.c time_meas.c file_output.c ../../SIMULATION/TOOLS/taus.c -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR2_DIR/COMMON -DMR_MAIN -DNB_ANTENNAS_RX=1 # -DD256STATS #-DD64STATS

dft_cycles_avx2: lte_dfts_avx2
	./lte_dfts_avx2 | egrep cycles
