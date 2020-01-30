all:
measurement_display thread-pool-test

measurement_display:
measurement_display.c thread-pool.h
gcc measurement_display.c -I $ {OPENAIR_DIR}/ -I $ {OPENAIR_DIR}/common/utils/  -I. $ {OPENAIR_DIR}/common/utils/backtrace.c -lpthread -D TEST_THREAD_POOL -I../LOG -I../../utils/T -o
measurement_display

thread-pool-test:
thread-pool.c thread-pool.h
gcc -g thread-pool.c -I $ {OPENAIR_DIR}/ -I $ {OPENAIR_DIR}/common/utils/  -I. $ {OPENAIR_DIR}/common/utils/backtrace.c -I $ {OPENAIR_DIR}/openair2/COMMON $ {OPENAIR_DIR}/common/utils/LOG/log.c $ {OPENAIR_DIR}/common/config/config_userapi.c
$ {OPENAIR_DIR}/common/config/config_load_configmodule.c $ {OPENAIR_DIR}/common/config/config_cmdline.c -lpthread -ldl -D TEST_THREAD_POOL -I../LOG -I../../utils/T -o thread-pool-test

