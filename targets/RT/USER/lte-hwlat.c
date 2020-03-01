/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <syscall.h>
#include <sys/sysinfo.h>

#include "assertions.h"
#include "PHY/types.h"

#include "PHY/defs.h"

#include <sys/time.h>
#define GET_TIME_INIT(num) struct timeval _timers[num]
#define GET_TIME_VAL(num) gettimeofday(&_timers[num], NULL)
#define TIME_VAL_TO_MS(num) (((double)_timers[num].tv_sec*1000.0) + ((double)_timers[num].tv_usec/1000.0))

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "../../ARCH/COMMON/common_lib.h"

#include "PHY/extern.h"
#include "SCHED/extern.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/proto.h"

#define 		SYRIQ_CHANNEL_TESTER  0
#define 		SYRIQ_CHANNEL_TESTER2 0
#define     SYRIQ_CHANNEL_DATA1   1

volatile int             oai_exit = 0;

openair0_config_t openair0_cfg[MAX_CARDS];

#if 0
#define NB_ANTENNAS_RX 4

#define DevAssert(cOND)                     _Assert_(cOND, _Assert_Exit_, "")
#define malloc16(x) memalign(16,x)

#ifdef 0
static inline void* malloc16_clear( size_t size )
{
#ifdef __AVX2__
  void* ptr = memalign(32, size);
#else
  void* ptr = memalign(16, size);
#endif
  if(ptr)
    memset( ptr, 0, size );
  return ptr;
}
#endif

#endif

typedef struct latency_stat {
    uint64_t    counter;

    uint64_t    stat250;
    uint64_t    stat500;
    uint64_t    stat600;
    uint64_t    stat700;
    uint64_t    stat800;

    uint64_t    stat1300;
    uint64_t    stat1500;
    uint64_t    stat2000;
    uint64_t    stat2500;
    uint64_t    stat3000;

    uint64_t    stat880;
    uint64_t    stat960;
    uint64_t    stat1040;
    uint64_t    stat1120;
    uint64_t    stat1200;
} latency_stat_t;


typedef struct	timing_stats {
	char			*name;
	double			min;
	double			max;
	double			total;
	unsigned int	count;
} timing_stats_t;


//static struct timespec	get_timespec_diff(
//							struct timespec *start,
//							struct timespec *stop )
//{
//	struct timespec		result;
//
//	if ( ( stop->tv_nsec - start->tv_nsec ) < 0 ) {
//		result.tv_sec = stop->tv_sec - start->tv_sec - 1;
//		result.tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
//	}
//	else {
//		result.tv_sec = stop->tv_sec - start->tv_sec;
//		result.tv_nsec = stop->tv_nsec - start->tv_nsec;
//	}
//
//	return result;
//}


//static void 			measure_time (
//							openair0_device	*rf_device,
//							struct timespec *start,
//							struct timespec *stop,
//							timing_stats_t *stats,
//							boolean_t START,
//							uint8_t PRINT_INTERVAL )
//{
//	if ( START ) {
//		clock_gettime( CLOCK_MONOTONIC, start );
//	}
//	else {
//		clock_gettime( CLOCK_MONOTONIC, stop );
//
//		struct timespec		diff;
//		double				current		= 0;
////		boolean_t			show_stats	= false;
//
//		diff 	= get_timespec_diff( start, stop );
//		current = (double)diff.tv_sec * 1000000 + (double)diff.tv_nsec / 1000;
//
//		if ( current > stats->max ) {
//			stats->max = current;
////			show_stats = true;
//		}
//		if ( stats->min == 0 || current < stats->min ) {
//			stats->min = current;
////			show_stats = true;
//		}
//		stats->total += current;
//
////		if ( show_stats ) {
////			rf_device.trx_get_stats_func( &rf_device );
////		}
//
////		if ( stats->count % PRINT_INTERVAL == 0 ) {
////			double			avg	= stats->total / ( stats->count + 1 );
////			printf( "[%s][%d] Current : %.2lf µs, Min : %.2lf µs, Max : %.2lf µs, Avg : %.2lf µs\n",
////					stats->count, stats->name, current, stats->min, stats->max, avg );
////		}
//
//		stats->count++;
//	}
//}
int32_t **rxdata;
int32_t **txdata;

int setup_ue_buffers(PHY_VARS_UE **phy_vars_ue, openair0_config_t *openair0_cfg);

static inline void saif_meas(unsigned int frame_rx, unsigned int subframe_rx) {
    static latency_stat_t __thread latency_stat;
    static struct timespec __thread last= {0};
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    if ( last.tv_sec )  {
        uint64_t diffTime =  ((uint64_t)now.tv_sec *1000 *1000 *1000 + now.tv_nsec) -
                             ((uint64_t)last.tv_sec *1000 *1000 *1000 + last.tv_nsec);
        diffTime/=1000;
        latency_stat.counter++;

        if ( diffTime <= 800 ) {
          if (diffTime  < 250 )
            latency_stat.stat250++;
          else if (diffTime  < 500 )
            latency_stat.stat500++;
          else if (diffTime  < 600 )
            latency_stat.stat600++;
          else if (diffTime  < 700 )
            latency_stat.stat700++;
          else
            latency_stat.stat800++;
        }
        else if ( diffTime > 1200 ) {
            if (diffTime  < 1500 )
                latency_stat.stat1300++;
            else if ( diffTime < 2000 )
                latency_stat.stat1500++;
            else if ( diffTime < 2500 )
                latency_stat.stat2000++;
            else if ( diffTime < 3000 )
                latency_stat.stat2500++;
            else
                latency_stat.stat3000++;
        }
        else
          if (diffTime  <= 880 )
            latency_stat.stat880++;
          else if (diffTime  <= 960 )
            latency_stat.stat960++;
          else if (diffTime  <= 1040 )
            latency_stat.stat1040++;
          else if (diffTime  < 1120 )
            latency_stat.stat1120++;
          else
            latency_stat.stat1200++;


        if ( (diffTime>=1500) || ( !(frame_rx%1024) && subframe_rx == 0 ) ) {
            time_t current=time(NULL);
            printf("\n");
            printf("%.2f Period stats cnt=%7.7ld    0.. 250=%7.7ld  250.. 500=%7.7ld  500.. 600=%7.7ld  600.. 700=%7.7ld  700.. 800=%7.7ld - (frame_rx=%u) - %s",
                  now.tv_sec+(double)now.tv_nsec/1e9,
                  latency_stat.counter,
                  latency_stat.stat250, latency_stat.stat500,
                  latency_stat.stat600, latency_stat.stat700,
                  latency_stat.stat800,
                  frame_rx,
                  ctime(&current));
            printf("%.2f Period stats cnt=%7.7ld  800.. 880=%7.7ld  880.. 960=%7.7ld  960..1040=%7.7ld 1040..1120=%7.7ld 1120..1200=%7.7ld - (frame_rx=%u) - %s",
                  now.tv_sec+(double)now.tv_nsec/1e9,
                  latency_stat.counter,
                  latency_stat.stat880, latency_stat.stat960,
                  latency_stat.stat1040, latency_stat.stat1120,
                  latency_stat.stat1200,
                  frame_rx,
                  ctime(&current));
            printf("%.2f Period stats cnt=%7.7ld 1200..1300=%7.7ld 1300..1500=%7.7ld 1500..2000=%7.7ld 2000..2500=%7.7ld      >3000=%7.7ld - (frame_rx=%u) - %s",
                  now.tv_sec+(double)now.tv_nsec/1e9,
                  latency_stat.counter,
                  latency_stat.stat1300, latency_stat.stat1500,
                  latency_stat.stat2000, latency_stat.stat2500,
                  latency_stat.stat3000,
                  frame_rx,
                  ctime(&current));
            fflush(stdout);
        }
    }
    last=now;
}
/* End of Changed by SYRTEM */

void exit_fun(const char* s)
{

  if (s != NULL) {
    printf("%s %s() Exiting OAI softmodem: %s\n",__FILE__, __FUNCTION__, s);
  }

  oai_exit = 1;
 
}


void init_thread(int sched_runtime, int sched_deadline, int sched_fifo, char * name) {

#ifdef DEADLINE_SCHEDULER
    if (sched_runtime!=0) {
        struct sched_attr attr= {0};
        attr.size = sizeof(attr);
        // This creates a .5 ms fpga_recv_cnt reservation
        attr.sched_policy = SCHED_DEADLINE;
        attr.sched_runtime  = sched_runtime;
        attr.sched_deadline = sched_deadline;
        attr.sched_period   = 0;
        AssertFatal(sched_setattr(0, &attr, 0) == 0,
                    "[SCHED] main eNB thread: sched_setattr failed %s \n",perror(errno));
        LOG_I(HW,"[SCHED][eNB] eNB main deadline thread %lu started on CPU %d\n",
              (unsigned long)gettid(), sched_getcpu());
    }

#else
#ifdef CPU_AFFINITY
    if (get_nprocs() >2) {
        for (j = 1; j < get_nprocs(); j++)
            CPU_SET(j, &cpuset);
    }
    AssertFatal( 0 == pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset)==0,"");
#endif
    struct sched_param sp;
    sp.sched_priority = sched_fifo;
    AssertFatal(pthread_setschedparam(pthread_self(),SCHED_FIFO,&sp)==0,
                "Can't set thread priority, Are you root?\n");
#endif

    // Lock memory from swapping. This is a process wide call (not constraint to this thread).
    mlockall(MCL_CURRENT | MCL_FUTURE);
    pthread_setname_np( pthread_self(), name );

}


int main(void)
{
	int                 ret;
	uint64_t			i;
	openair0_device     rf_device;
	openair0_timestamp	timestamp       = 0;

	unsigned int        sub_frame       = 0;
	unsigned int        frame_rx        = 0;

	unsigned int        nb_antennas_tx  = 1;
	unsigned int        nb_antennas_rx  = 1;


	openair0_cfg[0].mmapped_dma         = 0;
	openair0_cfg[0].configFilename      = NULL;
	openair0_cfg[0].duplex_mode         = duplex_mode_FDD;

	uint32_t			**sendbuff		= NULL;
	uint32_t			**recvbuff		= NULL;
	uint32_t			expected_value	= 0;
	uint32_t			received_value	= 0;
	int					nsamp 			= 0; // 1 ms
	int					antenna_id		= 1;
	int 				c 				= 0;
	int 				numIter			= 0;
	int 				fpga_loop		= 0;
#if SYRIQ_CHANNEL_DATA1
	uint64_t			first_ts		= 0;
	uint8_t				is_first_ts		= 0;
	uint64_t			trx_read_cnt	= 0;
	uint32_t			j				= 0;
	uint32_t			err_cnt			= 0;
#else
	int         		failure			= 0;
#endif
#if 0 // 5MHz BW
  unsigned int 			nb_sample_per_tti	= 7680;
  openair0_cfg[0].sample_rate				= 7.68e6;
  openair0_cfg[0].samples_per_frame			= nb_sample_per_tti*10;
  openair0_cfg[0].rx_bw						= 2.5e6;
  openair0_cfg[0].tx_bw						= 2.5e6;
  openair0_cfg[0].num_rb_dl					= 25;
#endif

#if 0 // 10MHz BW
  unsigned int  nb_sample_per_tti   = 15360;
  openair0_cfg[0].sample_rate       = 15.36e6;
  openair0_cfg[0].samples_per_frame = nb_sample_per_tti*10;
  openair0_cfg[0].rx_bw             = 5.0e6;
  openair0_cfg[0].tx_bw             = 5.0e6;
  openair0_cfg[0].num_rb_dl         = 50;
#endif

#if 1 // 20MHz BW
  unsigned int  nb_sample_per_tti   = 30720;
  openair0_cfg[0].sample_rate       = 30.72e6;
  openair0_cfg[0].samples_per_frame = nb_sample_per_tti*10;
  openair0_cfg[0].rx_bw             = 10.0e6;
  openair0_cfg[0].tx_bw             = 10.0e6;
  openair0_cfg[0].num_rb_dl         = 100;
#endif

  const char *openair_dir = getenv("OPENAIR_DIR");
  const char *ini_file    = "/usr/local/etc/syriq/ue.band7.tm1.PRB100.NR20.dat";

  int   readBlockSize;
  void* rxp[nb_antennas_rx];
  void* txp[nb_antennas_tx];

//  int32_t rxdata[1][nb_sample_per_tti*10+2048];
#if 0
  int32_t 				**rxdata;
#endif
//  init_thread(100000, 500000, sched_get_priority_max(SCHED_FIFO),"main UE");

	printf("LTE HARDWARE Latency debug utility \n");
  printf("INIT data buffers \n");

#if 0
  rxdata    = (int32_t**)memalign(32, nb_antennas_rx*sizeof(int32_t*) );
  rxdata[0] = (int32_t*)memalign(32,(nb_sample_per_tti*10+2048)*sizeof(int32_t));
#else
  rxdata = (int32_t**)malloc16( nb_antennas_rx*sizeof(int32_t*) );
  txdata = (int32_t**)malloc16( nb_antennas_tx*sizeof(int32_t*) );
  rxdata[0] = (int32_t*)malloc16_clear( 307200*sizeof(int32_t) );
  txdata[0] = (int32_t*)malloc16_clear( 307200*sizeof(int32_t) );
#endif

	printf("INIT done\n");

	memset(&rf_device, 0, sizeof(openair0_device));

	rf_device.host_type = BBU_HOST;
	openair0_cfg[0].duplex_mode = duplex_mode_FDD;
	openair0_cfg[0].rx_num_channels  = 1;
	openair0_cfg[0].tx_num_channels  = 1;

	// configure channel 0
	for (i=0; i<openair0_cfg[0].rx_num_channels; i++)
  {
		printf("configure channel %d \n",i);
		openair0_cfg[0].autocal[i] = 1;
		openair0_cfg[0].rx_freq[i] = 2680000000;
		openair0_cfg[0].tx_freq[i] = 2560000000;
		openair0_cfg[0].rx_gain[i] = 61;
		openair0_cfg[0].tx_gain[i] = 90;
	}

	if (openair_dir)
	{
		openair0_cfg[0].configFilename = malloc(strlen(openair_dir) + strlen(ini_file) + 2);
		sprintf(openair0_cfg[0].configFilename, "%s/%s", openair_dir, ini_file);
	}
//	printf("openair0_cfg[0].configFilename:%s\n", openair0_cfg[0].configFilename);

	ret = openair0_device_load( &rf_device, &openair0_cfg[0] );
	if (ret != 0){
		exit_fun("Error loading device library");
    exit(-1);
	}

/*	Runtime test
 *	30 720 000 samples (4 bytes) for 1 sec (30 720 for 1 ms)
 */
	puts( "* Starting the device" );

//	TIMING
//	struct timespec		start, stop; // tx_start, tx_stop;
//	timing_stats_t		rx_stats, tx_stats;

//	rx_stats.name		= strdup( "RX" );
//	tx_stats.name		= strdup( "TX" );
//	TIMING

	nsamp		= 307200; // 1 ms
	sendbuff	= memalign( 128, nb_antennas_rx * sizeof( uint32_t * ) );
	sendbuff[0]	= memalign( 128, nsamp * sizeof( uint32_t ) );
	recvbuff	= memalign( 128, nb_antennas_rx * sizeof( uint32_t * ) );
	recvbuff[0]	= memalign( 128, nsamp * sizeof( uint32_t ) );

//	Create dummy buffer
	for ( c = 0; c < nsamp; c++ ) {
//		sendbuff[0][c] = (c+1);
		sendbuff[0][c] = (c+1)*16;
//		sendbuff[0][c] = (nsamp-c);
		recvbuff[0][c] = 0;
	}

	GET_TIME_INIT(3);

	if ( rf_device.trx_start_func( &rf_device ) < 0 ) {
		printf( "  device could not be started !\n" );
		return -1;
	}

	// read 30720

	// write 30720 ts+2*30720

	// if tsread >= 2-307200 -> check expected value + status (RxoVer TxUnder)

	rxp[0]			= (void*)&rxdata[0][0];
	txp[0]			= (void*)&txdata[0][0];
	timestamp		= 0;
	nsamp			= 30720;	// 30720  => 1ms buffer
	trx_read_cnt	= 100000;		// 1 loop => 1ms (10sec=10000; 15min = 900000)
	antenna_id		= 1;
	int	diff		= 0;
	int compare_start	= 0;

	uint32_t	looploop=0;
	uint64_t	error_cnt = 0;
	uint64_t	error_ts_start;

	for (i = 0; i < trx_read_cnt; i++)
	{

//		printf("\n\n");


//		ret = rf_device.trx_read_func( &rf_device, &timestamp, &(rxp[0][(i*30720)%307200]), nsamp, antenna_id );
//		ret = rf_device.trx_read_func( &rf_device, &timestamp, recvbuff[0][(i*30720)%307200], nsamp, antenna_id );
		rxp[0]			= (void*)&rxdata[0][(i*30720)%307200];
		GET_TIME_VAL(0);
		ret = rf_device.trx_read_func( &rf_device, &timestamp, rxp, nsamp, antenna_id );
		GET_TIME_VAL(1);
		
		if (ret != nsamp)
		{
			printf("Error: nsamp received (%d) != nsamp required (%d)\n", ret, nsamp);
			fflush(stdout);
			return (-1);
		}
		if (!is_first_ts)
		{
			first_ts 	= timestamp;
			is_first_ts	= 1;
		}
	
//		printf("%d - trx_read_func ret=%d - ts = %d - @=0x%08lx\n", i, ret, timestamp, rxp[0]  );

		txp[0]			= (void*)&(txdata[0][ ((i*30720)%307200+2*30720)%307200 ]);	
//		printf("    i=%d @txp[0][0] = 0x%016lx\n", i, &(((uint32_t *)txp[0])[0]) );
//		printf("    i=%d @txp[0][1] = 0x%016lx\n", i, &(((uint32_t *)txp[0])[1]) );

		for ( c = 0; c < nsamp; c++ )
		{
			(((uint32_t *)txp[0])[c]) = ( (((timestamp+c+2*30720)<<4)&0x0000FFF0) + (((timestamp+c+2*30720)<<8)&0xFFF00000) );
		}

//		printf("    txp[0][%d] = 0x%x\n",0,((uint32_t *)(txp[0]))[0]);
//		printf("    txp[0][%d] = 0x%x\n",1,((uint32_t *)(txp[0]))[1]);
//		printf("    ...\n");
//		printf("    txp[0][%d] = 0x%x\n",nsamp-2,((uint32_t *)(txp[0]))[nsamp-2]);
//		printf("    txp[0][%d] = 0x%x\n",nsamp-1,((uint32_t *)(txp[0]))[nsamp-1]);


		ret = rf_device.trx_write_func( &rf_device, (timestamp+2*30720), txp, nsamp, antenna_id, false );
		if (ret != nsamp)
		{
			printf("Error: nsamp sent (%d) != nsamp required (%d)\n", ret, nsamp);
			fflush(stdout);
			return (-1);
		}

//		printf("%d - trx_write_func ret=%d - ts = %d - @=0x%08lx\n", i, ret, (timestamp+2*30720), txp[0]  );


		if (timestamp >= (first_ts + 2 * 307200))
		{			

			// check Rx Overflow
			
			// check Tx Underflow

			// check Expected Value
			for ( c = 0; c < nsamp; c++ )
			{

// LOOPBACK
#if 1
				expected_value	= ((timestamp + c)&0xFFFFFF);
				received_value	= ((uint32_t *)(rxp[0]))[c];
				received_value	= ((received_value)&0xFFF) + ((received_value>>4)&0xFFF000);

				if (compare_start == 0)
				{
					compare_start	= 1;
					diff			= expected_value - received_value;
				}
				received_value = (received_value + diff)&0xFFFFFF;

#if 1

				if (expected_value != received_value)
				{
					if(!error_cnt)
						error_ts_start = (timestamp + c);
					else
					{
						if (looploop < 32)
							printf("%d - %d != %d - ts %d - raw 0x%x - diff %d\n",
								looploop, 
								expected_value, received_value, 
								(timestamp + c), ((uint32_t *)(rxp[0]))[c],
								diff);
						looploop++;
					}

					error_cnt++;

					if(!(error_cnt%102400))
						printf(" -> error detected     : cnt=%d - start=%ld - stop=... diff=%d\n",
									error_cnt,
									error_ts_start,
									diff);

				}
				else
				{
					if(error_cnt)
					{
						printf(" -> error detected     : cnt=%d - start=%ld - stop=%ld\n\n",
									error_cnt,
									error_ts_start,
									(timestamp + c) );
						looploop=0;
					}
					error_cnt=0;
				}
#endif
#endif

// DEBUG mode 0
#if 0
				received_value	= ((uint32_t *)(rxp[0]))[c];
				received_value	= ((received_value)&0xFFF) + ((received_value>>4)&0xFFF000);
				received_value	= (received_value&0xFFFFFF);
				if (compare_start == 0)
				{
					compare_start	= 1;
					expected_value 	= received_value;
				}
				else
				{
					expected_value++;
					expected_value	= (expected_value&0xFFFFFF);
				}

				if (expected_value != received_value)
				{
					if(!error_cnt)
						error_ts_start = (timestamp + c);
					error_cnt++;
				}
				else
				{
					if(error_cnt)
					{
						printf(" -> error detected     : cnt=%d - start=%ld - stop=%ld\n\n",
									error_cnt,
									error_ts_start,
									(timestamp + c) );
					}
					error_cnt=0;
				}



//				if (expected_value != received_value)
//				{
//					printf("%d -> %d != %d (ts+c=%ld)(raw=0x%08x)\n", 
//							looploop, 
//							expected_value, received_value,
//							(timestamp + c),
//							((uint32_t *)(rxp[0]))[c] );
//
//					if (received_value)
//						expected_value 	= received_value;
//				}
#endif

			}
		}
	}

	printf("HwLat Application returns !!!\n");
	fflush(stdout);
	sleep(1);


	printf("\n");
	rf_device.trx_end_func( &rf_device );

	sleep(1);
	free(sendbuff[0]);
	free(sendbuff);
	free(recvbuff[0]);
	free(recvbuff);

	exit(0);



	return(0);



//	puts( "* Frequency modification test" );
//	rf_device.trx_set_freq_func( &rf_device, &openair0_cfg[0], 0 );

//	puts( "* Gain modification test" );
//	rf_device.trx_set_gains_func( &rf_device, &openair0_cfg[0] );	//	NOT working (cf. initialization)

//#if LTE_UE
//  sleep(1);
	GET_TIME_VAL(0);
	for (i = 0; i < trx_read_cnt; i++)
	{
    printf("\n");
		ret = rf_device.trx_read_func( &rf_device, &timestamp, rxp, nsamp, antenna_id );
    printf("* timestamp=%ld\n", timestamp);
#if 1
		for (j = 0; j < nsamp; j++)
		{
			if ( ((uint32_t *)rxp[0])[j] != ((expected_value + j + timestamp)%307200) )
			{
				err_cnt++;
				printf("rxp[%06d]=0x%08x (expected 0x%08lx)\n", j, ((uint32_t *)rxp[0])[j], ((expected_value + j + timestamp)%307200) );
			}
			if (err_cnt >= 128)
			{
				printf("Error: more than 128 expected value failed !\n");
				i = (trx_read_cnt - 1);
				j = nsamp;
				break;
			}
		}
		//expected_value	= (expected_value + nsamp)%307200;
    expected_value  = (expected_value)%307200;
#endif
	}
	GET_TIME_VAL(1);
//#endif

/* ********** ********** */
/*  SYRIQ_CHANNEL_DATA1  */
/* ********** ********** */
#if SYRIQ_CHANNEL_DATA1

//  rf_device.trx_get_stats_func( &rf_device );

  rf_device.trx_end_func( &rf_device );

  printf("\n* rf_device.trx_read_func(%d) x %d: %.6lf s\n\n", nsamp, i, ((TIME_VAL_TO_MS(1) - TIME_VAL_TO_MS(0)))/1000.0 );

//  rf_device.trx_get_stats_func( &rf_device );

#if 0
  for (i = 0; i < nsamp; i++)
  {
//    if ( ((uint32_t *)rxp[0])[i] != i)
//    {
      err_cnt++;
      printf("rxp[%06d]=0x%08x (expected 0x%08x)\n", i, ((uint32_t *)rxp[0])[i], i);
//    }
//    if (err_cnt > 256)
//    {
//      i = 307200;
//      break;
//    }
  }
#endif
//  for (i = 1024-32; i < 1024+32; i++)
//  {
////    if ( ((uint32_t *)rxp[0])[i] != i)
////    {
//      err_cnt++;
//      printf("rxp[%06d]=0x%08x (expected 0x%08x)\n", i, ((uint32_t *)rxp[0])[i], i);
////    }
//    if (err_cnt > 256)
//    {
//      i = 307200;
//      break;
//    }
//  }
	printf("SYRIQ CHANNEL DATA1 done !!!\n");
  sleep(1);
	return(0);
#endif
/* ********** ********** */
/*  SYRIQ_CHANNEL_DATA1  */
/* ********** ********** */



	timestamp	= 0;
	ret = rf_device.trx_write_func( &rf_device, timestamp, (void**)sendbuff, nsamp, antenna_id, false );
	printf("* rf_device.trx_write_func returns %d\n", ret);

	sleep(1);

	nsamp	= 30720;
	numIter		= 100000;
	for ( c = 0; c < numIter; c++ )
	{
		fpga_loop ++;
		if ( !(fpga_loop % 1000) )
		{
			printf("\rtest loop %d / %d", fpga_loop, numIter);
			fflush(stdout);
		}

//		printf("* TEST : %08d\n", (c+1));
//		measure_time( &rf_device, &start, &stop, &tx_stats, true, 10 );
//		ret = rf_device.trx_write_func( &rf_device, timestamp, (void**)sendbuff, nsamp, antenna_id, false );
//    		printf("* rf_device.trx_write_func returns %d\n", ret);
//		measure_time( &rf_device, &start, &stop, &tx_stats, false, 10 );

//		sleep(1);

//		measure_time( &rf_device, &start, &stop, &tx_stats, true, 10 );
		ret = rf_device.trx_read_func( &rf_device, &timestamp, (void**)recvbuff, nsamp, antenna_id );
//		measure_time( &rf_device, &start, &stop, &tx_stats, false, 10 );

		// Check the data
		if (ret > 0)
		{
/* ********** ********** */
/*  SYRIQ_CHANNEL_TESTER */
/* ********** ********** */
#if SYRIQ_CHANNEL_TESTER
      failure = 0;
			for (i = 0; i < ret; i++)
			{
				if ( ((i%1024) == 0) || ((i%1024) == 1) || ((i%1024) == 2) || ((i%1024) == 3) )
				{
					if ( (recvbuff[0])[i] != (1020 + ((i%1024)+1)) )
					{
						printf("* ERROR (buff[0])[%d]: %d, expected %d\n", i, (uint32_t)(recvbuff[0])[i], ((i%1024)+1) );
						failure = 1;
					}
				}
				else if ( (recvbuff[0])[i] != ((i%1024)+1) )
				{
					printf("* ERROR (buff[0])[%d]: %d, expected %d\n", i, (uint32_t)(recvbuff[0])[i], ((i%1024)+1) );
					failure = 1;
				}
				else
				{
					printf("* DONE  (buff[0])[%d]: %d, expected %d\n", i, (uint32_t)(recvbuff[0])[i], ((i%1024)+1) );
				}
				if(failure)
					break;
			}
			if (failure)
			 	printf("* ERROR recv %08d checked FAILURE ret=%d\n", (c+1), ret);
			else
      			{
//			 	printf("* DONE  recv %08d checked SUCCESSFULLY ret=%d\n", (c+1), ret);
      			}
#endif
/* ********** ********** */
/*  SYRIQ_CHANNEL_TESTER2*/
/* ********** ********** */
#if SYRIQ_CHANNEL_TESTER2
//			printf("* ret=%d timestamp=%ld (%ld)\n", ret, timestamp, (timestamp%307200));
      failure = 0;
			for (i = 0; i < ret; i++)
			{
//				printf("* (recvbuff[0])[%d]: %d\n", i, (uint32_t)(recvbuff[0])[i]);
//				printf("* timestamp+(i/1024+1)*1024 - 3 + i%1024: %d\n", (timestamp-nsamp+(i/1024+1)*1024 - 3 + i%1024)%307200 );
				expected_value	= ((((i%1024)+1)/*>>4*/)&0x00000FFF);
				if ( ((i%1024) == 0) || ((i%1024) == 1) || ((i%1024) == 2) || ((i%1024) == 3) )
				{
					expected_value	= ((((timestamp-nsamp+(i/1024+1)*1024 - 3 + i%1024))%307200));
					if ( (recvbuff[0])[i] !=  expected_value)
					{
						if ( (expected_value == 0) && ((recvbuff[0])[i] != 4915200) )
						{
							printf("* ERROR (buff[0])[%d]: %d, expected %d\n", i, (uint32_t)(recvbuff[0])[i], expected_value );
							failure = 1;
						}
						if ( (expected_value == 0) && ((recvbuff[0])[i] == 4915200) )
						{
//							printf("* DONE loop in circular buffer\n");
						}
					}
				}
				else if ( (recvbuff[0])[i] != expected_value )
				{
					printf("* ERROR (buff[0])[%d]: %d, expected %d\n", i, (uint32_t)(recvbuff[0])[i], ((i%1024)+1) );
					failure = 1;
				}
				else
				{
//					printf("* DONE  (buff[0])[%d]: %d, expected %d\n", i, (uint32_t)(recvbuff[0])[i], ((i%1024)+1) );
				}
//				if(failure)
//					break;
			}
			if (failure)
			 	printf("* ERROR recv %08d checked FAILURE ret=%d\n", (c+1), ret);
			else
      			{
//			 	printf("* DONE  recv %08d checked SUCCESSFULLY ret=%d\n", (c+1), ret);
      			}
#endif
#if LOOPBACK

#endif
		}
		else
		{
			printf("* ERROR rf_device.trx_read_func returns %d\n", ret);
		}

	}
	printf("\n");
	rf_device.trx_end_func( &rf_device );

	sleep(1);
	free(sendbuff[0]);
	free(sendbuff);
	free(recvbuff[0]);
	free(recvbuff);

	exit(0);

//	END IS HERE !

	rf_device.trx_set_freq_func(&rf_device,&openair0_cfg[0],0);

	if (rf_device.trx_start_func(&rf_device) != 0 ) {
		printf("Could not start the device\n");
		oai_exit=1;
	}

	while(!oai_exit){

	rxp[0] = (void*)&rxdata[0][sub_frame*nb_sample_per_tti];

	readBlockSize = rf_device.trx_read_func(&rf_device,
											&timestamp,
											rxp,
											nb_sample_per_tti,
											0);

    if ( readBlockSize != nb_sample_per_tti )
      oai_exit = 1;

    sub_frame++;
    sub_frame%=10;

    if( sub_frame == 0)
      frame_rx++;

    saif_meas(frame_rx, sub_frame);
  }

  return(0);
}
