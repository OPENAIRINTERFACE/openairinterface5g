thread-pool-test: thread-pool.c thread-pool.h
	gcc -g -O3 thread-pool.c -I ${OPENAIR_DIR}/nfapi/open-nFAPI/nfapi/public_inc -I ${OPENAIR_DIR}/ -I ${OPENAIR_DIR}/common/utils/  -I. ${OPENAIR_DIR}/common/utils/backtrace.c -I ${OPENAIR_DIR}/openair2/COMMON ${OPENAIR_DIR}/common/utils/system.c ${OPENAIR_DIR}/common/utils/LOG/log.c ${OPENAIR_DIR}/common/config/config_userapi.c ${OPENAIR_DIR}/common/config/config_load_configmodule.c ${OPENAIR_DIR}/common/config/config_cmdline.c -lpthread -ldl -D TEST_THREAD_POOL -DMAX_NUM_CCs=1 -I../LOG -I../../utils/T -o thread-pool-test

