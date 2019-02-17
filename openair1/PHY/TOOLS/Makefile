lte_dfts_sse4: lte_dfts.c
	gcc-7 -O3 -std=gnu99 -msse4.1 -o lte_dfts_sse4 lte_dfts.c time_meas.c  ../../SIMULATION/TOOLS/taus.c -I$$OPENAIR_HOME -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR2_DIR -I$$OPENAIR2_DIR/COMMON -I$$OPENAIR_HOME/common/utils -I$$OPENAIR_HOME/common/utils/T -DMR_MAIN -DNB_ANTENNAS_RX=1 -lm -lpthread # -DD256STATS #-DD64STATS

lte_dfts_avx2: lte_dfts.c
	gcc -O2 -std=gnu99 -mavx2 -g -ggdb -o lte_dfts_avx2 lte_dfts.c time_meas.c ../../SIMULATION/TOOLS/taus.c -I$$OPENAIR_HOME -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR2_DIR -I$$OPENAIR2_DIR/COMMON -I$$OPENAIR_HOME/common/utils -DMR_MAIN -DNB_ANTENNAS_RX=1 -lm -lpthread # -DD256STATS #-DD64STATS

lte_dfts_avx2.s: lte_dfts.c
	gcc -O2 -std=gnu99 -mavx2 -S lte_dfts.c time_meas.c ../../SIMULATION/TOOLS/taus.c -I$$OPENAIR_HOME -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR2_DIR -I$$OPENAIR2_DIR/COMMON -I$$OPENAIR_HOME/common/utils -DMR_MAIN -DNB_ANTENNAS_RX=1 -lm -lpthread # -DD256STATS #-DD64STATS

lte_dfts_sse4.s: lte_dfts.c
	gcc -O2 -std=gnu99 -msse4.1 -S lte_dfts.c time_meas.c ../../SIMULATION/TOOLS/taus.c -I$$OPENAIR_HOME -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR2_DIR -I$$OPENAIR2_DIR/COMMON -I$$OPENAIR_HOME/common/utils -DMR_MAIN -DNB_ANTENNAS_RX=1 -lm # -DD256STATS #-DD64STATS

dft_cycles_avx2: lte_dfts_avx2
	./lte_dfts_avx2 | egrep cycles
