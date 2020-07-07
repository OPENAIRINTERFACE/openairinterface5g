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


/* *************************************************************************************************

  USER GUIDE

  1 - CONFIGURE TEST SESSION 
  see TESTS PARAMETERS section below

  2 - COMPILATION CMD LINE (same as openair compilation)
    - NO AVX SUPPORT
  /usr/bin/cc            -msse4.1 -mssse3  -std=gnu99 -Wall -Wstrict-prototypes -fno-strict-aliasing -rdynamic -funroll-loops -Wno-packed-bitfield-compat -fPIC  -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_FCNTL_H=1 -DHAVE_ARPA_INET_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_SOCKET_H=1 -DHAVE_STRERROR=1 -DHAVE_SOCKET=1 -DHAVE_MEMSET=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_STDLIB_H=1 -DHAVE_MALLOC=1 -DHAVE_LIBSCTP -g -DMALLOC_CHECK_=3 -O2  -o lte-hwlat-test  lte-hwlat2.c -lrt -lpthread -lm -ldl
    - AVX2 Support
  /usr/bin/cc     -mavx2 -msse4.1 -mssse3  -std=gnu99 -Wall -Wstrict-prototypes -fno-strict-aliasing -rdynamic -funroll-loops -Wno-packed-bitfield-compat -fPIC  -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_FCNTL_H=1 -DHAVE_ARPA_INET_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_SOCKET_H=1 -DHAVE_STRERROR=1 -DHAVE_SOCKET=1 -DHAVE_MEMSET=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_STDLIB_H=1 -DHAVE_MALLOC=1 -DHAVE_LIBSCTP -g -DMALLOC_CHECK_=3 -O2  -o lte-hwlat-test  lte-hwlat2.c -lrt -lpthread -lm -ldl


  3 - RUN
  sudo cset shield --force --kthread on -c 1-3    // for 4 cores
  sudo cset shield --force --kthread on -c 1-7    // for 8 cores
  sudo cset shield ./lte-hwlat-test

  4 - remove cset shield
  sudo cset shield --reset

 ***************************************************************************************************/

/* *************************************************************************************************
 *  TESTS PARAMETERS
 */
#define     HWLAT_LOOP_CNT      1000000   /* measurment loop count for each thread*/
#define     HWLAT_TTI_SLEEP_US  250     /* usleep duration -> IQ capture simulation (in µ seconds) */

#define     RX_NB_TH            6

#define     CALIB_RT_INTRUMENTATION 0

/* Laurent Thpmas instrumentation -> see openair2/UTIL/LOG/log.h for full implementation
    -> This is a copy and paste implementation in this file for a self contained source */
#define     INSTRUMENTATION_LT_RDTSC              1
/* SYRTEM rdtsc instrumentation implementation (see below for more infaormation) */
#define     INSTRUMENTATION_SYR_RDTSC             2
/* SYRTEM instrumentation using clock_gettime MONOTONIC */
#define     INSTRUMENTATION_SYR_CLOCK_MONO        3
/* SYRTEM instrumentation using clock_gettime REALTIME */
#define     INSTRUMENTATION_SYR_CLOCK_REALTIME    4

#define     HWLAT_INSTRUMENTATION                 INSTRUMENTATION_LT_RDTSC


/* Statistics histogram output */
#define HISTOGRAM_MIN_VALUE     0
#define HISTOGRAM_MAX_VALUE     2000
#define HISTOGRAM_STEP          1
#define HISTOGRAM_SIZE          ( ( ( HISTOGRAM_MAX_VALUE - HISTOGRAM_MIN_VALUE ) / HISTOGRAM_STEP ) + 1 )



/* *************************************************************************************************/


/*
 *  INCLUDES
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <syscall.h>
#include <math.h>
#include <sched.h> 
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>


/* From rt_wrapper.h 
****************************************************************************************/
#define gettid() syscall(__NR_gettid) // for gettid

/* From common/utils/itti/assertions.h
****************************************************************************************/
# define display_backtrace()

#define _Assert_Exit_                           \
{                                               \
    fprintf(stderr, "\nExiting execution\n");   \
    display_backtrace();                        \
    fflush(stdout);                             \
    fflush(stderr);                             \
    exit(EXIT_FAILURE);                         \
}

#define _Assert_(cOND, aCTION, fORMAT, aRGS...)             \
do {                                                        \
    if (!(cOND)) {                                          \
        fprintf(stderr, "\nAssertion ("#cOND") failed!\n"   \
                "In %s() %s:%d\n" fORMAT,                   \
                __FUNCTION__, __FILE__, __LINE__, ##aRGS);  \
        aCTION;                                             \
    }           \
} while(0)

#define AssertFatal(cOND, fORMAT, aRGS...)          _Assert_(cOND, _Assert_Exit_, fORMAT, ##aRGS)


/* From "openair1/PHY/TOOLS/time_meas.h"
****************************************************************************************/
double cpu_freq_GHz;


typedef struct {

  long long in;
  long long diff;
  long long diff_now;
  long long p_time; /*!< \brief absolute process duration */
  long long diff_square; /*!< \brief process duration square */
  long long max;
  int trials;
  int meas_flag;
} time_stats_t;

static inline unsigned long long rdtsc_oai(void) __attribute__((always_inline));
static inline unsigned long long rdtsc_oai(void)
{
  unsigned long long a, d;
  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  return (d<<32) | a;
}

double get_cpu_freq_GHz(void);

static inline void reset_meas(time_stats_t *ts) {

  ts->trials=0;
  ts->diff=0;
  ts->diff_now=0;
  ts->p_time=0;
  ts->diff_square=0;
  ts->max=0;
  ts->meas_flag=0;
  
}

double estimate_MHz_syr(void);
static __inline__ uint64_t pickCyclesStart(void);
static __inline__ uint64_t pickCyclesStop(void);


/* From "openair2/UTIL/LOG/log.h"
****************************************************************************************/
extern double cpuf;         
extern double cpu_mhz_syr;


static __inline__ uint64_t rdtsc(void) {
  uint64_t a, d;
  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  return (d<<32) | a;
}

typedef struct m {
    uint64_t iterations;
    uint64_t sum;
    uint64_t maxArray[11];
} Meas;

static inline void printMeas(char * txt, Meas *M, int period) {
    if (M->iterations%period == 0 ) {
        char txt2[512];
        sprintf(txt2,"%s avg=%" PRIu64 " iterations=%" PRIu64 " max=%" 
                PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 "\n",
                txt,
                M->sum/M->iterations,
                M->iterations,
                M->maxArray[1],M->maxArray[2], M->maxArray[3],M->maxArray[4], M->maxArray[5], 
                M->maxArray[6],M->maxArray[7], M->maxArray[8],M->maxArray[9],M->maxArray[10]);
// SYRTEM : just use printf do not include all LOG_X for this test
//#if DISABLE_LOG_X
        printf("%s",txt2);
//#else
//        LOG_W(PHY, "%s",txt2);
//#endif
    }
}

static inline int cmpint(const void* a, const void* b) {
    uint64_t* aa=(uint64_t*)a;
    uint64_t* bb=(uint64_t*)b;
    return (int)(*aa-*bb);
}

static inline uint64_t updateTimes(uint64_t start, Meas *M, int period, char * txt) {
    if (start!=0) {
        uint64_t end=rdtsc();
        long long diff=(end-start)/(cpuf*1000);
        M->maxArray[0]=diff;
        M->sum+=diff;
        M->iterations++;
        qsort(M->maxArray, 11, sizeof(uint64_t), cmpint);
//        printMeas(txt,M,period);    // SYRTEM : Printed only a the end of the measurment loop 
        return diff;
    }
    return 0;
}

static inline uint64_t updateTimes_syr(uint64_t start, Meas *M, int period, char * txt) {
    if (start!=0) {
//        uint64_t end=rdtsc();
        uint64_t end=pickCyclesStop();
        long long diff=(long long)((double)(end-start)/(cpu_mhz_syr));
//        long long diff=(end-start)/(cpuf*1000);
        M->maxArray[0]=diff;
        M->sum+=diff;
        M->iterations++;
        qsort(M->maxArray, 11, sizeof(uint64_t), cmpint);
//        printMeas(txt,M,period);    // SYRTEM : Printed only a the end of the measurment loop 
        return diff;
    }
    return 0;
}




#define initRefTimes(a) static __thread Meas a= {0}
#define pickTime(a) uint64_t a=rdtsc()
#define pickTime_syr(a) uint64_t a=pickCyclesStart()
#define readTime(a) a






/*
 *  DEFINES
 */

#define TIMESPEC_TO_DOUBLE_US( t )    ( ( (double)t.tv_sec * 1000000 ) + ( (double)t.tv_nsec / 1000 ) )

typedef struct histo_time {
  double      max;
  unsigned int  count;
} histo_time_t;

static void measure_time ( struct timespec *start, struct timespec *stop, uint8_t START, uint16_t PRINT_INTERVAL );
static struct timespec  get_timespec_diff( struct timespec *start, struct timespec *stop );
void  histogram_save_in_csv( histo_time_t *histo , char *file_prefix);
histo_time_t  *histogram_init( histo_time_t *histo );
void          histogram_store_value( histo_time_t *histo, double value );


#define FIFO_PRIORITY         40



#define true            1
#define false           0




/*
 *  STRUCTURES
 */


/* stub of UE_rxtx_proc full structure in openair1/PHY/defs.h  
*/
typedef struct  UE_rxtx_proc {
  int                 instance_cnt_rxtx;
  pthread_t           pthread_rxtx;
  pthread_cond_t      cond_rxtx;
  pthread_mutex_t     mutex_rxtx;


  int sub_frame_start;
  int sub_frame_step;
  unsigned long long gotIQs;


  unsigned long syr_rdtsc_rxtx_th_unlock_iteration;
  uint64_t      syr_rdtsc_ue_th_got_iq;
  double        syr_rdtsc_rxtx_th_unlock; 
  double        syr_rdtsc_rxtx_th_unlock_max;
  double        syr_rdtsc_rxtx_th_unlock_mean;
  double        syr_rdtsc_rxtx_th_unlock_min;
  histo_time_t  *syr_rdtsc_rxtx_th_unlock_histogram;

} UE_rxtx_proc_t;



/* this structure is used to pass both UE phy vars and
 * proc to the function UE_thread_rxn_txnp4
 */
struct rx_tx_thread_data {
  /* PHY_VARS_UE    *UE;  */  // UE phy vars not used for this test
  UE_rxtx_proc_t    *proc;    // We use a stub of rxtx_proc see definition above
};


// ODD / EVEN Scheduling
#if 0
typedef struct  threads_s {
    int         iq;
    int         odd;
    int         even;
} threads_t;
threads_t       threads = { -1, -1, -1 }; // Core number for each thread (iq=3, even=2, odd=1)
#endif

// SLOT 0 / SLOT 1 parallelization
typedef struct threads_s {
    int iq;
    int one;
    int two;
    int three;
    int four;
    int five;
    int six;
    int slot1_proc_one;
    int slot1_proc_two;
    int slot1_proc_three;
} threads_t;
threads_t threads= {7,6,5,4,3,2,1,-1,-1,-1};

/*
 *  FUNCTIONS DEFINITION
 */

void *UE_thread(void *arg);
void init_UE(int nb_inst);



/*
 *  GLOBALS VARIABLES
 */

volatile int      oai_exit  = 0;
int           th_count  = 0;
double cpuf;
double cpu_mhz_syr;







pthread_t       pthread_ue;
pthread_attr_t      attr_ue;

UE_rxtx_proc_t      **rxtx_proc;

struct timespec     even_start, even_stop;
struct timespec     odd_start, odd_stop;

histo_time_t      *th_wake_histogram;


/*
 *  FUNCTIONS
 */



void exit_fun(const char* s) {
    if ( s != NULL ) {
        printf("%s %s() Exiting OAI softmodem: %s\n",__FILE__, __FUNCTION__, s);
    }

    oai_exit = 1;
}




void init_thread(int sched_runtime, int sched_deadline, int sched_fifo, cpu_set_t *cpuset, char * name) {

#ifdef DEADLINE_SCHEDULER
    if (sched_runtime!=0) {
        struct sched_attr attr= {0};
        attr.size = sizeof(attr);
        attr.sched_policy = SCHED_DEADLINE;
        attr.sched_runtime  = sched_runtime;
        attr.sched_deadline = sched_deadline;
        attr.sched_period   = 0;
        AssertFatal(sched_setattr(0, &attr, 0) == 0,
                    "[SCHED] %s thread: sched_setattr failed %s \n", name, strerror(errno));
        LOG_I(HW,"[SCHED][eNB] %s deadline thread %lu started on CPU %d\n",
              name, (unsigned long)gettid(), sched_getcpu());
    }

#else
    if (CPU_COUNT(cpuset) > 0)
        AssertFatal( 0 == pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), cpuset), "");
    struct sched_param sp;
    sp.sched_priority = sched_fifo;
    AssertFatal(pthread_setschedparam(pthread_self(),SCHED_FIFO,&sp)==0,
                "Can't set thread priority, Are you root?\n");
    /* Check the actual affinity mask assigned to the thread */
    cpu_set_t *cset=CPU_ALLOC(CPU_SETSIZE);
    if (0 == pthread_getaffinity_np(pthread_self(), CPU_ALLOC_SIZE(CPU_SETSIZE), cset)) {
      char txt[512]={0};
      for (int j = 0; j < CPU_SETSIZE; j++)
        if (CPU_ISSET(j, cset))
    sprintf(txt+strlen(txt), " %d ", j);
      printf("CPU Affinity of thread %s is %s\n", name, txt);
    }
    CPU_FREE(cset);
#endif

    // Lock memory from swapping. This is a process wide call (not constraint to this thread).
    mlockall(MCL_CURRENT | MCL_FUTURE);
    pthread_setname_np( pthread_self(), name );

// SYRTEM : Synchronization thread is not simulated -> just ignore this part of code
//    // LTS: this sync stuff should be wrong
//    printf("waiting for sync (%s)\n",name);
//    pthread_mutex_lock(&sync_mutex);
//    printf("Locked sync_mutex, waiting (%s)\n",name);
//    while (sync_var<0)
//        pthread_cond_wait(&sync_cond, &sync_mutex);
//    pthread_mutex_unlock(&sync_mutex);
    printf("started %s as PID: %ld\n",name, gettid());
}

void init_UE(int nb_inst)
{
  int inst;
  for (inst=0; inst < nb_inst; inst++) {
    //   UE->rfdevice.type      = NONE_DEV;
// SYRTEM : we use a stub of phy_var_ue
//    PHY_VARS_UE *UE = PHY_vars_UE_g[inst][0];
//    AssertFatal(0 == pthread_create(&UE->proc.pthread_ue, &UE->proc.attr_ue, UE_thread, (void*)UE), "");
    AssertFatal( 0 == pthread_create( &pthread_ue, &attr_ue, UE_thread, NULL ), "" );
  }

  printf("UE threads created by %ld\n", gettid());
}


// SYRTEM - UE synchronization thread is not used in this latency test program
//static void *UE_thread_synch(void *arg) {
//  ...
//}


// SYRTEM - Stub of UE_thread_rxn_txnp4
//    -> No DSP scheduled
//    -> Only thread initialization and thread wake up are kept 

static void *UE_thread_rxn_txnp4(void *arg){
    static __thread int UE_thread_rxtx_retval;
    struct rx_tx_thread_data *rtd = arg;
    UE_rxtx_proc_t *proc = rtd->proc;

    proc->instance_cnt_rxtx=-1;

    char threadname[256];
    sprintf(threadname,"UE_%d_proc_%d", 0, proc->sub_frame_start);
    
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    if ( (proc->sub_frame_start)%RX_NB_TH == 0 && threads.one != -1 )
        CPU_SET(threads.one, &cpuset);
    if ( (proc->sub_frame_start)%RX_NB_TH == 1 && threads.two != -1 )
        CPU_SET(threads.two, &cpuset);
    if ( (proc->sub_frame_start)%RX_NB_TH == 2 && threads.three != -1 )
        CPU_SET(threads.three, &cpuset);
    if ( (proc->sub_frame_start)%RX_NB_TH == 3 && threads.four != -1 )
        CPU_SET(threads.four, &cpuset);
    if ( (proc->sub_frame_start)%RX_NB_TH == 4 && threads.five != -1 )
        CPU_SET(threads.five, &cpuset);
    if ( (proc->sub_frame_start)%RX_NB_TH == 5 && threads.six != -1 )
        CPU_SET(threads.six, &cpuset);
            //CPU_SET(threads.three, &cpuset);
    init_thread(900000,1000000 , FIFO_PRIORITY-1, &cpuset,
                threadname);


    proc->syr_rdtsc_rxtx_th_unlock_iteration=0;
    proc->syr_rdtsc_rxtx_th_unlock_max=0;
    proc->syr_rdtsc_rxtx_th_unlock_mean=0;
    proc->syr_rdtsc_rxtx_th_unlock_min=1000;
    proc->syr_rdtsc_rxtx_th_unlock_histogram = NULL;
    proc->syr_rdtsc_rxtx_th_unlock_histogram = histogram_init (proc->syr_rdtsc_rxtx_th_unlock_histogram);

    while (!oai_exit) {
        if (pthread_mutex_lock(&proc->mutex_rxtx) != 0) {
          printf("[SCHED][UE] error locking mutex for UE RXTX\n" );
          exit_fun("nothing to add");
        }
        while (proc->instance_cnt_rxtx < 0) {
          // most of the time, the thread is waiting here
          pthread_cond_wait( &proc->cond_rxtx, &proc->mutex_rxtx );
        }
        if (pthread_mutex_unlock(&proc->mutex_rxtx) != 0) {
          printf("[SCHED][UE] error unlocking mutex for UE RXn_TXnp4\n" );
          exit_fun("nothing to add");
        }

    
        proc->syr_rdtsc_rxtx_th_unlock = (double)(pickCyclesStop()-proc->syr_rdtsc_ue_th_got_iq )/cpu_mhz_syr;
  
        proc->syr_rdtsc_rxtx_th_unlock_iteration++;

        proc->syr_rdtsc_rxtx_th_unlock_mean += proc->syr_rdtsc_rxtx_th_unlock;
        if ( proc->syr_rdtsc_rxtx_th_unlock_max < proc->syr_rdtsc_rxtx_th_unlock )
          proc->syr_rdtsc_rxtx_th_unlock_max = proc->syr_rdtsc_rxtx_th_unlock;
        if ( proc->syr_rdtsc_rxtx_th_unlock_min > proc->syr_rdtsc_rxtx_th_unlock )
          proc->syr_rdtsc_rxtx_th_unlock_min = proc->syr_rdtsc_rxtx_th_unlock;
        histogram_store_value( proc->syr_rdtsc_rxtx_th_unlock_histogram, proc->syr_rdtsc_rxtx_th_unlock );


        if (pthread_mutex_lock(&proc->mutex_rxtx) != 0) {
          printf("[SCHED][UE] error locking mutex for UE RXTX\n" );
          exit_fun("noting to add");
        }
        proc->instance_cnt_rxtx--;
        if (pthread_mutex_unlock(&proc->mutex_rxtx) != 0) {
          printf("[SCHED][UE] error unlocking mutex for UE RXTX\n" );
          exit_fun("noting to add");
        }
    }

// thread finished
    free(arg);
    return &UE_thread_rxtx_retval;
}




void *UE_thread(void *arg) {

  int i           = 0;
  int hw_loop_cnt = 0;

  int             nb_threads  = RX_NB_TH;
  char            threadname[128];
  cpu_set_t           cpuset;
  struct rx_tx_thread_data  *rtd;
  UE_rxtx_proc_t *proc;

  CPU_ZERO( &cpuset );

  if ( threads.iq != -1 )
    CPU_SET( threads.iq, &cpuset );
  init_thread( 100000, 500000, FIFO_PRIORITY, &cpuset, "HDW Threads" );

  sprintf( threadname, "Main UE %d", 0 );
  pthread_setname_np( pthread_self(), threadname );


//  init_UE_threads(UE)
  pthread_attr_init( &attr_ue );
  pthread_attr_setstacksize( &attr_ue, 8192 );//5*PTHREAD_STACK_MIN);

  for ( i = 0; i < nb_threads; i++ ) {

    printf("\n");

    rtd = calloc( 1, sizeof( struct rx_tx_thread_data ) );
    if ( rtd == NULL )
      abort();

    rtd->proc = rxtx_proc[i]; // &UE->proc.proc_rxtx[i];

    pthread_mutex_init( &rxtx_proc[i]->mutex_rxtx ,NULL ); // pthread_mutex_init( &UE->proc.proc_rxtx[i].mutex_rxtx,NULL );
    pthread_cond_init( &rxtx_proc[i]->cond_rxtx, NULL ); // pthread_cond_init(&UE->proc.proc_rxtx[i].cond_rxtx,NULL);
    rtd->proc->sub_frame_start=i;
    rtd->proc->sub_frame_step=nb_threads;
    printf("Init_UE_threads rtd %d proc %d nb_threads %d i %d\n",rtd->proc->sub_frame_start, rxtx_proc[i]->sub_frame_start,nb_threads, i);
    pthread_create( &rxtx_proc[i]->pthread_rxtx, NULL, UE_thread_rxn_txnp4, rtd ); // pthread_create( &UE->proc.proc_rxtx[i].pthread_rxtx, NULL, UE_thread_rxn_txnp4, rtd);

    usleep(1000);

  }
//  init_UE_threads(UE)



  int sub_frame=-1;
  while ( !oai_exit ) {
  
    sub_frame++;
    sub_frame %= 10;

    proc = rxtx_proc[hw_loop_cnt%RX_NB_TH];

    // Simulate IQ reception
    usleep( HWLAT_TTI_SLEEP_US );

    proc->syr_rdtsc_ue_th_got_iq = pickCyclesStart();

    AssertFatal(pthread_mutex_lock(&proc->mutex_rxtx) ==0,"");

    proc->instance_cnt_rxtx++;
    if ( proc->instance_cnt_rxtx == 0 ) {
      if ( pthread_cond_signal( &proc->cond_rxtx) != 0 ) {
      exit_fun( "nothing to add" );
      }
    } else {
      if ( proc->instance_cnt_rxtx > 2 )
      exit_fun( "instance_cnt_rxtx > 2" );
    }

    AssertFatal( pthread_cond_signal( &proc->cond_rxtx ) == 0 ,"" );
    AssertFatal(pthread_mutex_unlock(&proc->mutex_rxtx) ==0,"");


    /* Do not go indefinitely */
    hw_loop_cnt++;

    if ( hw_loop_cnt%1000 == 0 )
    {
      printf("\r%d/%d",hw_loop_cnt,HWLAT_LOOP_CNT*RX_NB_TH);
      fflush(stdout);
    } 

    if (hw_loop_cnt >= HWLAT_LOOP_CNT*RX_NB_TH)
    {
      printf("\n\n");
      for (i=0 ; i<RX_NB_TH ; i++)
      {
        proc = rxtx_proc[i];
        pthread_getname_np(proc->pthread_rxtx,threadname,128 );
        printf("RxTX Thread unlock latency on thread %s (it. %lu) (us) : max=%8.3f - mean=%8.3f - min=%8.3f\n",
                  threadname,
                  proc->syr_rdtsc_rxtx_th_unlock_iteration,
                  proc->syr_rdtsc_rxtx_th_unlock_max,
                  proc->syr_rdtsc_rxtx_th_unlock_mean/proc->syr_rdtsc_rxtx_th_unlock_iteration,
                  proc->syr_rdtsc_rxtx_th_unlock_min);

        histogram_save_in_csv(proc->syr_rdtsc_rxtx_th_unlock_histogram, threadname );

      }
      printf("\n\n");
      oai_exit = 1;
    }

  } // while !oai_exit

  oai_exit = 1;

  return NULL;
}

#define CALIB_LOOP_CNT            1000000 //2000000000
#define CALIB_LOOP_REPORT_PERIOD  1000
#define CALIB_LOOP_UP_THRESHOLD   20        // 20 -> for NG Intel flat model    
#define CALIB_USLEEP              250     

int           main( void )
{

  int i;


#if CALIB_RT_INTRUMENTATION
  uint32_t    calib_loop_count;


  uint64_t      lt_overhead_cur;
  uint64_t      lt_overhead_min;
  uint64_t      lt_overhead_max;
  uint64_t      lt_overhead_mean;
  histo_time_t  *lt_overhead_histogram = NULL;

  uint64_t      lt_syr_overhead_cur;
  uint64_t      lt_syr_overhead_min;
  uint64_t      lt_syr_overhead_max;
  uint64_t      lt_syr_overhead_mean;
  histo_time_t  *lt_syr_overhead_histogram = NULL;

  double        syr_rdtsc_overhead_cur;
  double        syr_rdtsc_overhead_min;
  double        syr_rdtsc_overhead_max;
  double        syr_rdtsc_overhead_mean;
  histo_time_t  *syr_rdtsc_overhead_histogram = NULL;

  double        syr_clock_gettime_overhead_cur;
  double        syr_clock_gettime_overhead_min;
  double        syr_clock_gettime_overhead_max;
  double        syr_clock_gettime_overhead_mean;
  histo_time_t  *syr_clock_gettime_overhead_histogram = NULL;


  uint64_t  cycles_start;
  uint64_t  cycles_stop;

  uint64_t  cycles_diff;
  uint64_t  cycles_diff_max = 0;


  struct timespec start;
  struct timespec stop;
#endif

  cpuf        = get_cpu_freq_GHz();
  cpu_mhz_syr = estimate_MHz_syr();

  printf("\n\ncpuf %f - cpu_mhz_syr %f\n\n", cpuf, cpu_mhz_syr);

#if CALIB_RT_INTRUMENTATION

  /* ********************************************************************************************** */
  /*  CALIBRATION                                                                                   */
  /* ********************************************************************************************** */

  /* *************************************************************
    Laurent Thomas instrumentation overhead
  */
  printf("\nCalibrating OAI Realtime Instrumentation (Laurent Thomas imp.)\n");
  calib_loop_count = CALIB_LOOP_CNT;
  initRefTimes(lt_instru_calib);      // Laurent Thomas realtime instrumentation calibration measurments
  lt_overhead_min   = 1000;
  lt_overhead_max   = 0;
  lt_overhead_mean  = 0;
  lt_overhead_histogram = histogram_init( lt_overhead_histogram );
  while(calib_loop_count)
  {
    pickTime(lt_start);

//    asm volatile("");     // Nop instruction
    usleep(CALIB_USLEEP);

    lt_overhead_cur = updateTimes(  readTime(lt_start), 
                                    &lt_instru_calib, 
                                    CALIB_LOOP_CNT, 
                                    "Laurent Thomas realtime instrumentation calibration measurments");

    lt_overhead_mean += lt_overhead_cur;
    if ( lt_overhead_max < lt_overhead_cur )
      lt_overhead_max = lt_overhead_cur;
    if ( lt_overhead_min > lt_overhead_cur )
      lt_overhead_min = lt_overhead_cur;
    histogram_store_value( lt_overhead_histogram, (double)lt_overhead_cur );

    if ( calib_loop_count%CALIB_LOOP_REPORT_PERIOD == 0 )
    {
      printf("\r%d/%d",calib_loop_count,CALIB_LOOP_CNT);
      fflush(stdout);
    }

    calib_loop_count--;
  }
  lt_overhead_mean = lt_overhead_mean/CALIB_LOOP_CNT;

//  printMeas("\nLaurent Thomas realtime instrumentation calibration measurments",&lt_instru_calib,CALIB_LOOP_CNT);


  /* *************************************************************
    Laurent Thomas instrumentation overhead 
  */
  printf("\nCalibrating OAI Realtime Instrumentation - SYRTEM revisited\n");
  calib_loop_count = CALIB_LOOP_CNT;
  initRefTimes(lt_instru_calib_syr);      // Laurent Thomas realtime instrumentation calibration measurments
  lt_syr_overhead_min   = 1000;
  lt_syr_overhead_max   = 0;
  lt_syr_overhead_mean  = 0;
  lt_syr_overhead_histogram = histogram_init( lt_syr_overhead_histogram );
  while(calib_loop_count)
  {
    pickTime_syr(lt_start_syr);

//    asm volatile("");     // Nop instruction
    usleep(CALIB_USLEEP);

    lt_syr_overhead_cur = updateTimes_syr(readTime(lt_start_syr), &lt_instru_calib_syr, CALIB_LOOP_CNT, "Laurent Thomas realtime instrumentation calibration measurments (SYRTEM Update)");

    lt_syr_overhead_mean += lt_syr_overhead_cur;
    if ( lt_syr_overhead_max < lt_syr_overhead_cur )
      lt_syr_overhead_max = lt_syr_overhead_cur;
    if ( lt_syr_overhead_min > lt_syr_overhead_cur )
      lt_syr_overhead_min = lt_syr_overhead_cur;
    histogram_store_value( lt_syr_overhead_histogram, (double)lt_syr_overhead_cur );

    if ( calib_loop_count%CALIB_LOOP_REPORT_PERIOD == 0 )
    {
      printf("\r%d/%d",calib_loop_count,CALIB_LOOP_CNT);
      fflush(stdout);
    }

    calib_loop_count--;
  }
  lt_syr_overhead_mean = lt_syr_overhead_mean / CALIB_LOOP_CNT;

//  printMeas("\nLaurent Thomas realtime instrumentation calibration measurments (SYRTEM Update)",&lt_instru_calib_syr,CALIB_LOOP_CNT);



  /* *************************************************************
    SYRTEM RDTSC instrumentation overhead 
  */
  printf("\nCalibrating SYRTEM RDTSC Realtime Instrumentation\n");
  calib_loop_count = CALIB_LOOP_CNT;
  syr_rdtsc_overhead_min    = 1000;
  syr_rdtsc_overhead_max    = 0;
  syr_rdtsc_overhead_mean   = 0;
  syr_rdtsc_overhead_histogram  = histogram_init( syr_rdtsc_overhead_histogram );
  while(calib_loop_count)
  {

    cycles_start = pickCyclesStart();
    
//    asm volatile("");     // Nop instruction
    usleep(CALIB_USLEEP);
    
    cycles_stop = pickCyclesStop();

    cycles_diff = cycles_stop - cycles_start;
    if(cycles_diff_max < cycles_diff)
      cycles_diff_max = cycles_diff;

    syr_rdtsc_overhead_cur = (double)cycles_diff/cpu_mhz_syr;

    syr_rdtsc_overhead_mean += syr_rdtsc_overhead_cur;
    if ( syr_rdtsc_overhead_max < syr_rdtsc_overhead_cur )
      syr_rdtsc_overhead_max = syr_rdtsc_overhead_cur;
    if ( syr_rdtsc_overhead_min > syr_rdtsc_overhead_cur )
      syr_rdtsc_overhead_min = syr_rdtsc_overhead_cur;
    histogram_store_value( syr_rdtsc_overhead_histogram, (double)syr_rdtsc_overhead_cur );

    if ( calib_loop_count%CALIB_LOOP_REPORT_PERIOD == 0 )
    {
      printf("\r%d/%d",calib_loop_count,CALIB_LOOP_CNT);
      fflush(stdout);
    }

    calib_loop_count--;
  }
  syr_rdtsc_overhead_mean = syr_rdtsc_overhead_mean / CALIB_LOOP_CNT;

//  printf("\nSYRTEM RDTSV realtime instrumentation calibration : cycles_diff_max %ld - tsc_duration_max %f us\n ", cycles_diff_max, tsc_duration_max);



  /* *************************************************************
    SYRTEM Clock_gettime MONOTONIC instrumentation overhead
  */
  printf("\nCalibrating SYRTEM clock_gettime Realtime Instrumentation\n");
  calib_loop_count = CALIB_LOOP_CNT;
  syr_clock_gettime_overhead_min = 1000;
  syr_clock_gettime_overhead_max    = 0;
  syr_clock_gettime_overhead_mean   = 0;
  syr_clock_gettime_overhead_histogram  = histogram_init( syr_clock_gettime_overhead_histogram );
  while(calib_loop_count)
  {

    clock_gettime( CLOCK_MONOTONIC, &start );

//    asm volatile("");     // Nop instruction
    usleep(CALIB_USLEEP);

    clock_gettime( CLOCK_MONOTONIC, &stop );
    syr_clock_gettime_overhead_cur = TIMESPEC_TO_DOUBLE_US( get_timespec_diff( &start, &stop ) );

    syr_clock_gettime_overhead_mean += syr_clock_gettime_overhead_cur;
    if ( syr_clock_gettime_overhead_max < syr_clock_gettime_overhead_cur )
      syr_clock_gettime_overhead_max = syr_clock_gettime_overhead_cur;
    if ( syr_clock_gettime_overhead_min > syr_clock_gettime_overhead_cur )
      syr_clock_gettime_overhead_min = syr_clock_gettime_overhead_cur;
    histogram_store_value( syr_clock_gettime_overhead_histogram, (double)syr_clock_gettime_overhead_cur );

    if ( calib_loop_count%CALIB_LOOP_REPORT_PERIOD == 0 )
    {
      printf("\r%d/%d",calib_loop_count,CALIB_LOOP_CNT);
      fflush(stdout);
    }

    calib_loop_count--;
  }
  syr_clock_gettime_overhead_mean = syr_clock_gettime_overhead_mean / CALIB_LOOP_CNT;

//  printf("\nSYRTEM clock_gettime MONOTONIC calibration : clock_gettime duration_max %f us\n ", max);


  printf("\n");

  printf("OAI LT  RT profiling overhead (it. %d) (us)       : max=%8ld - mean=%8ld - min=%8ld\n",
          CALIB_LOOP_CNT,
          lt_overhead_max, lt_overhead_mean, lt_overhead_min);
  printf("OAI SYR RT profiling overhead (it. %d) (us)       : max=%8ld - mean=%8ld - min=%8ld\n",
          CALIB_LOOP_CNT,
          lt_syr_overhead_max, lt_syr_overhead_mean, lt_syr_overhead_min);
  printf("SYR RDTSC  profiling overhead (it. %d) (us)       : max=%8.3f - mean=%8.3f - min=%8.3f\n",
          CALIB_LOOP_CNT,
          syr_rdtsc_overhead_max, syr_rdtsc_overhead_mean, syr_rdtsc_overhead_min);
  printf("SYR clock_gettime profiling overhead (it. %d) (us): max=%8.3f - mean=%8.3f - min=%8.3f\n",
          CALIB_LOOP_CNT,
          syr_clock_gettime_overhead_max, syr_clock_gettime_overhead_mean, syr_clock_gettime_overhead_min);

  return 0;
#endif

  rxtx_proc     = calloc( RX_NB_TH, sizeof( UE_rxtx_proc_t* ) );
  for (i=0 ; i<RX_NB_TH; i++)
    rxtx_proc[i]    = calloc( 1, sizeof( UE_rxtx_proc_t ) );


  th_wake_histogram = histogram_init( th_wake_histogram );

  init_UE( 1 );

  while ( !oai_exit )
    sleep( 1 );

  return 0;
}


double estimate_MHz_syr(void)
{
    //copied blantantly from http://www.cs.helsinki.fi/linux/linux-kernel/2001-37/0256.html
    /*
    * $Id: MHz.c,v 1.4 2001/05/21 18:58:01 davej Exp $
    * This file is part of x86info.
    * (C) 2001 Dave Jones.
    *
    * Licensed under the terms of the GNU GPL License version 2.
    *
    * Estimate CPU MHz routine by Andrea Arcangeli <andrea@suse.de>
    * Small changes by David Sterba <sterd9am@ss1000.ms.mff.cuni.cz>
    *
    */

    struct timespec start;
    struct timespec stop;

    unsigned long long int cycles[2];   /* must be 64 bit */
    double microseconds;  /* total time taken */


    /* get this function in cached memory */
    cycles[0] = rdtsc ();
    clock_gettime( CLOCK_REALTIME, &start );

    /* we don't trust that this is any specific length of time */
    usleep (100000);

    cycles[1] = rdtsc ();
    clock_gettime( CLOCK_REALTIME, &stop );
    microseconds = TIMESPEC_TO_DOUBLE_US( get_timespec_diff( &start, &stop ) );

    unsigned long long int elapsed = 0;
    if (cycles[1] < cycles[0])
    {
        //printf("c0 = %llu   c1 = %llu",cycles[0],cycles[1]);
        elapsed = UINT32_MAX - cycles[0];
        elapsed = elapsed + cycles[1];
        //printf("c0 = %llu  c1 = %llu max = %llu elapsed=%llu\n",cycles[0], cycles[1], UINT32_MAX,elapsed);
    }
    else
    {
        elapsed = cycles[1] - cycles[0];
        // printf("\nc0 = %llu  c1 = %llu elapsed=%llu\n",cycles[0], cycles[1],elapsed);
    }

    double mhz = elapsed / microseconds;


    //printf("%llg MHz processor (estimate).  diff cycles=%llu  microseconds=%llu \n", mhz, elapsed, microseconds);
    //printf("%g  elapsed %llu  microseconds %llu\n",mhz, elapsed, microseconds);
    return (mhz);
}



/** TSC Timestamp references

 - http://oliveryang.net/2015/09/pitfalls-of-TSC-usage/#312-tsc-sync-behaviors-on-smp-system
 - https://www.intel.com/content/www/us/en/embedded/training/ia-32-ia-64-benchmark-code-execution-paper.html
 - https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-3b-part-2-manual.pdf

 - http://blog.tinola.com/?e=54
 - https://stackoverflow.com/questions/24392392/getting-tsc-frequency-from-proc-cpuinfo !! warning on turbo
 - https://gist.github.com/nfarring/1624742
 - https://www.lmax.com/blog/staff-blogs/2015/10/25/time-stamp-counters/
 - https://github.com/cyring/CoreFreq
 - https://patchwork.kernel.org/patch/9438133/ --> TSC_ADJUST - to be checked
    -> https://software.intel.com/en-us/forums/software-tuning-performance-optimization-platform-monitoring/topic/388964
 - https://www.google.fr/url?sa=t&rct=j&q=&esrc=s&source=web&cd=11&ved=0ahUKEwjwpJbem5DVAhWBDpoKHXv0Dt04ChAWCCIwAA&url=https%3A%2F%2Fsigarra.up.pt%2Ffeup%2Fpt%2Fpub_geral.show_file%3Fpi_gdoc_id%3D809041&usg=AFQjCNFHTMjmjWMG6QSVnLDflEoU0RavKw&cad=rja
 - https://unix4lyfe.org/benchmarking/
 - https://stackoverflow.com/questions/27693145/rdtscp-versus-rdtsc-cpuid
 - https://github.com/dterei/tsc/blob/master/tsc.h
 - https://software.intel.com/en-us/forums/intel-isa-extensions/topic/280440
 - https://stackoverflow.com/questions/19941588/wrong-clock-cycle-measurements-with-rdtsc



*/






static __inline__ uint64_t pickCyclesStart(void) {
    unsigned  cycles_low, cycles_high;
    asm volatile(   "CPUID\n\t" // serialize
                    "RDTSC\n\t" // read clock
                    "MOV %%edx, %0\n\t"
                    "MOV %%eax, %1\n\t"
                    : "=r" (cycles_high), "=r" (cycles_low)
                    :: "%rax", "%rbx", "%rcx", "%rdx" );
return ((uint64_t) cycles_high << 32) | cycles_low;
}

static __inline__ uint64_t pickCyclesStop(void) {
  uint32_t eax, edx;

  __asm__ __volatile__("rdtscp"
    : "=a" (eax), "=d" (edx)
    :
    : "%ecx", "memory");

return (((uint64_t)edx << 32) | eax);
}








/**************************************************************************************
    EXTRACT from openair1/PHY/TOOLS/time_meas.c
***************************************************************************************/
double get_cpu_freq_GHz(void) {

  time_stats_t ts = {0};
  reset_meas(&ts);
  ts.trials++;
  ts.in = rdtsc_oai();
  sleep(1);
  ts.diff = (rdtsc_oai()-ts.in);
  cpu_freq_GHz = (double)ts.diff/1000000000;
  printf("CPU Freq is %f \n", cpu_freq_GHz);
  return cpu_freq_GHz; 
}



/**************************************************************************************
    SYRTEM STATISTICS HELPER
***************************************************************************************/



histo_time_t *histogram_init(histo_time_t *histo )
{
  int     i   = 0;
  double  max_val = HISTOGRAM_MIN_VALUE + HISTOGRAM_STEP;

  histo = calloc( HISTOGRAM_SIZE, sizeof( histo_time_t ) );
  if ( histo == NULL ) {
    printf( "ERROR allocating histogram structure !\n" );
    return NULL;
  }

  for ( i = 0; i < HISTOGRAM_SIZE; i++ ) {
    histo[i].max  = max_val;
    max_val     += HISTOGRAM_STEP;
  }

  return histo;
}


void  histogram_store_value( histo_time_t *histo, double value )
{
  int         i   = 0;

  for ( i = 0; i < HISTOGRAM_SIZE; i++ ) {
    if ( histo[i].max >= value || ( i + 1 ) == HISTOGRAM_SIZE ) {
      histo[i].count++;
      break;
    }
  }
}


void  histogram_save_in_csv( histo_time_t *histo , char *file_sufix)
{
  char        *csv_filename;
  char        month[3], day[3], hour[3], min[3], sec[3];
  time_t      curr_time;
  struct tm   *datetime;

  curr_time = time( NULL );
  datetime = localtime( &curr_time );

  csv_filename = calloc( sizeof( char ), 64 );
  if ( csv_filename == NULL ) {
    return;
  }

  memset( csv_filename, 0x00, 64 );
  memset( month, 0x00, 3 );
  memset( day, 0x00, 3 );
  memset( hour, 0x00, 3 );
  memset( min, 0x00, 3 );
  memset( sec, 0x00, 3 );

/*  Month.  */
  if ( datetime->tm_mon < 9 )
    snprintf( month, 3, "0%d", datetime->tm_mon + 1 );
  else
    snprintf( month, 3, "%d", datetime->tm_mon + 1 );

/*  Day.  */
  if ( datetime->tm_mday < 10 )
    snprintf( day, 3, "0%d", datetime->tm_mday );
  else
    snprintf( day, 3, "%d", datetime->tm_mday );

/*  Hour.  */
  if ( datetime->tm_hour < 10 )
    snprintf( hour, 3, "0%d", datetime->tm_hour );
  else
    snprintf( hour, 3, "%d", datetime->tm_hour );

/*  Minute.  */
  if ( datetime->tm_min < 10 )
    snprintf( min, 3, "0%d", datetime->tm_min );
  else
    snprintf( min, 3, "%d", datetime->tm_min );

/*  Second.  */
  if ( datetime->tm_sec < 10 )
    snprintf( sec, 3, "0%d", datetime->tm_sec );
  else
    snprintf( sec, 3, "%d", datetime->tm_sec );

  snprintf( csv_filename, 63, "ADRV9371_ZC706_HISTO_UEtoRXTX_%d%s%s%s%s%s_%s.csv",
      datetime->tm_year + 1900, month, day, hour, min, sec, file_sufix );

  FILE        *fp;
  int         i   = 0;
  int         min_val = 0;

  fp  = fopen( csv_filename, "w+" );

  fprintf( fp, "range;count\n" );

  for ( i = 0; i < HISTOGRAM_SIZE; i++ ) {
    if ( i + 1 == HISTOGRAM_SIZE )
      fprintf( fp, "%d+;%u\n", min_val, histo[i].count );
    else
      fprintf( fp, "%d-%.0f;%u\n", min_val, histo[i].max, histo[i].count );
    min_val = histo[i].max;
  }
  free(csv_filename);
  fclose( fp );
}



static struct timespec  get_timespec_diff(
              struct timespec *start,
              struct timespec *stop )
{
  struct timespec   result;

  if ( ( stop->tv_nsec - start->tv_nsec ) < 0 ) {
    result.tv_sec = stop->tv_sec - start->tv_sec - 1;
    result.tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
  }
  else {
    result.tv_sec = stop->tv_sec - start->tv_sec;
    result.tv_nsec = stop->tv_nsec - start->tv_nsec;
  }

  return result;
}


static void       measure_time (
              struct timespec *start,
              struct timespec *stop,
              uint8_t START,
              uint16_t PRINT_INTERVAL )
{
  static double   max   = 0;
  static double   min   = 0;
  static double     total = 0;
  static int      count = 0;

  if ( START ) {
    clock_gettime( CLOCK_REALTIME, start );
  }
  else {
    clock_gettime( CLOCK_REALTIME, stop );

    struct timespec   diff;
    double        current   = 0;

    diff  = get_timespec_diff( start, stop );
//    current = ( (double)diff.tv_sec * 1000000 ) + ( (double)diff.tv_nsec / 1000 );
    current = TIMESPEC_TO_DOUBLE_US( diff );

    if ( current > max ) {
      max = current;
    }
    if ( min == 0 || current < min ) {
      min = current;
    }
    total += current;

    histogram_store_value( th_wake_histogram, current );

    if ( count % PRINT_INTERVAL == 0 && count != 0 ) {
      double      avg = total / ( count + 1 );
      printf( "[%d] Current : %.2lf µs, Min : %.2lf µs, Max : %.2lf µs, Moy : %.2lf µs\n",
          count, current, min, max, avg );
    }
    count++;
  }
}










    


  


