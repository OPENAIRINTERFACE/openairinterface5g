all: measurement_display thread-pool-test

measurement_display: measurement_display.c thread-pool.h
	gcc measurement_display.c -I /data/openairinterface5g.nr/common/utils/  -I. /data/openairinterface5g.nr/common/utils/backtrace.c -lpthread -D TEST_THREAD_POOL -I../LOG -I../../utils/T -o measurement_display

thread-pool-test: thread-pool.c thread-pool.h
	gcc -g thread-pool.c -I /data/openairinterface5g.nr/common/utils/  -I. /data/openairinterface5g.nr/common/utils/backtrace.c -lpthread -D TEST_THREAD_POOL -I../LOG -I../../utils/T -o thread-pool-test

