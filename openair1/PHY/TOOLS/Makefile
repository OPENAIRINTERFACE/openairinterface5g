oai_dfts_sse4: oai_dfts.c
	gcc-7 -O3 -std=gnu99 -msse4.1 -o oai_dfts_sse4 oai_dfts.c time_meas.c  ../../SIMULATION/TOOLS/taus.c -I$$OPENAIR_HOME -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR_TARGETS/COMMON -I$$OPENAIR_TARGETS/ARCH/COMMON -I$$OPENAIR2_DIR -I$$OPENAIR2_DIR/COMMON -I$$OPENAIR_HOME/common/utils -I$$OPENAIR_HOME/common/utils/T -I$$OPENAIR_HOME/common/utils/msc -I$$OPENAIR_HOME/nfapi/open-nFAPI/nfapi/public_inc -DMR_MAIN -DNB_ANTENNAS_RX=1 -lm -lpthread # -DD256STATS #-DD64STATS

oai_dfts_avx2: oai_dfts.c
	gcc -O2 -std=gnu99 -mavx2 -g -ggdb -o oai_dfts_avx2 oai_dfts.c time_meas.c ../../SIMULATION/TOOLS/taus.c $$OPENAIR_HOME/common/utils/backtrace.c -I$$OPENAIR_HOME -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR_TARGETS/COMMON -I$$OPENAIR_TARGETS/ARCH/COMMON -I$$OPENAIR2_DIR -I$$OPENAIR2_DIR/COMMON -I$$OPENAIR_HOME/common/utils -I$$OPENAIR_HOME/common/utils/T -I$$OPENAIR_HOME/common/utils/msc -I$$OPENAIR_HOME/nfapi/open-nFAPI/nfapi/public_inc -DMR_MAIN -DNB_ANTENNAS_RX=1 -lm -lpthread # -DD256STATS #-DD64STATS

oai_dfts_avx2.s: oai_dfts.c
	gcc -O2 -std=gnu99 -mavx2 -S oai_dfts.c time_meas.c ../../SIMULATION/TOOLS/taus.c $$OPENAIR_HOME/common/utils/backtrace.c -I$$OPENAIR_HOME -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR_TARGETS/COMMON -I$$OPENAIR_TARGETS/ARCH/COMMON -I$$OPENAIR2_DIR -I$$OPENAIR2_DIR/COMMON -I$$OPENAIR_HOME/common/utils -I$$OPENAIR_HOME/common/utils/T -I$$OPENAIR_HOME/common/utils/msc -I$$OPENAIR_HOME/nfapi/open-nFAPI/nfapi/public_inc -DMR_MAIN -DNB_ANTENNAS_RX=1 -lm -lpthread # -DD256STATS #-DD64STATS


oai_dfts_sse4.s: oai_dfts.c
	gcc -O2 -std=gnu99 -msse4.1 -S oai_dfts.c time_meas.c ../../SIMULATION/TOOLS/taus.c $$OPENAIR_HOME/common/utils/backtrace.c -I$$OPENAIR_HOME -I$$OPENAIR1_DIR -I$$OPENAIR_TARGETS -I$$OPENAIR_TARGETS/COMMON -I$$OPENAIR_TARGETS/ARCH/COMMON -I$$OPENAIR2_DIR -I$$OPENAIR2_DIR/COMMON -I$$OPENAIR_HOME/common/utils -I$$OPENAIR_HOME/common/utils/T -I$$OPENAIR_HOME/common/utils/msc -I$$OPENAIR_HOME/nfapi/open-nFAPI/nfapi/public_inc -DMR_MAIN -DNB_ANTENNAS_RX=1 -lm -lpthread # -DD256STATS #-DD64STATS


dft_cycles_avx2: oai_dfts_avx2
	./oai_dfts_avx2 | egrep cycles
