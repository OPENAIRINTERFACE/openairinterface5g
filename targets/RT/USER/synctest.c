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

/*! \file lte-softmodem.c
* \brief main program to control HW and scheduling
* \author R. Knopp, F. Kaltenberger
* \date 2012
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/
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
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>

#include "rt_wrapper.h"

#include "PHY/types.h"
//#include "PHY/defs.h"
#include "openair0_lib.h"

#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface_init.h"
# include "timer.h"
# if defined(ENABLE_USE_MME)
#   include "s1ap_eNB.h"
#   include "sctp_eNB_task.h"
# endif
#endif

#ifdef XFORMS
#include "PHY/TOOLS/lte_phy_scope.h"
#include "stats.h"
// current status is that every UE has a DL scope for a SINGLE eNB (eNB_id=0)
// at eNB 0, an UL scope for every UE
FD_lte_phy_scope_ue  *form_ue[NUMBER_OF_UE_MAX];
FD_lte_phy_scope_enb *form_enb[NUMBER_OF_UE_MAX];
FD_stats_form *form_stats=NULL;
char title[255];
unsigned char scope_enb_num_ue = 1;
#endif //XFORMS

#ifdef EMOS
#include <gps.h>
#include <rtai_fifos.h>

//#define CHANSOUNDER_FIFO_SIZE 10485760 // 10 Mbytes FIFO
#define CHANSOUNDER_FIFO_SIZE 20971520  // 20 Mbytes FIFO
#define CHANSOUNDER_FIFO_MINOR 4               // minor of the FIFO device - this is /dev/rtf3
#define CHANSOUNDER_FIFO_DEV "/dev/rtf4"
#endif

#define TIMER_ONESHOT_MODE
#define FRAME_PERIOD 100000000ULL
#define DAQ_PERIOD 66667ULL
#define LTE_SLOTS_PER_FRAME  20
#define RESAMPLING_FACTOR 2
#define SAMPLES_PER_SLOT (15360/(1<<RESAMPLING_FACTOR))
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#ifdef RTAI
static SEM *mutex;
//static CND *cond;

static long int thread0;
static long int thread1;
//static long int sync_thread;
#else
#define OPENAIR_THREAD_STACK_SIZE    8192
#define OPENAIR_THREAD_PRIORITY        255
pthread_t thread0;
//pthread_t thread1;
pthread_attr_t attr_dlsch_threads;
struct sched_param sched_param_dlsch;
#endif

pthread_t  thread2; //xforms
pthread_t  thread3; //emos

/*
static int instance_cnt=-1; //0 means worker is busy, -1 means its free
int instance_cnt_ptr_kern,*instance_cnt_ptr_user;
int pci_interface_ptr_kern;
*/
//extern unsigned int bigphys_top;
//extern unsigned int mem_base;

int card = 0;
exmimo_config_t *p_exmimo_config;
exmimo_id_t     *p_exmimo_id;
volatile unsigned int *DAQ_MBOX;

int oai_exit = 0;

//int time_offset[4] = {-138,-138,-138,-138};
//int time_offset[4] = {-145,-145,-145,-145};
int time_offset[4] = {0,0,0,0};

int fs4_test=0;
char UE_flag=0;
uint8_t  eNB_id=0,UE_id=0;

uint32_t carrier_freq[4]= {1907600000,1907600000,1907600000,1907600000};

struct timing_info_t {
  //unsigned int frame, hw_slot, last_slot, next_slot;
  RTIME time_min, time_max, time_avg, time_last, time_now;
  //unsigned int mbox0, mbox1, mbox2, mbox_target;
  unsigned int n_samples;
} timing_info;

extern int16_t* sync_corr_ue0;
extern int16_t prach_ifft[4][1024*2];


int rx_input_level_dBm;
#ifdef XFORMS
extern int otg_enabled;
#else
int otg_enabled;
#endif
int number_of_cards = 1;

int mbox_bounds[20] = {8,16,24,30,38,46,54,60,68,76,84,90,98,106,114,120,128,136,144, 0}; ///boundaries of slots in terms ob mbox counter rounded up to even numbers
//int mbox_bounds[20] = {6,14,22,28,36,44,52,58,66,74,82,88,96,104,112,118,126,134,142, 148}; ///boundaries of slots in terms ob mbox counter rounded up to even numbers

int init_dlsch_threads(void);
void cleanup_dlsch_threads(void);
int32_t init_rx_pdsch_thread(void);
void cleanup_rx_pdsch_thread(void);
int init_ulsch_threads(void);
void cleanup_ulsch_threads(void);

//void setup_ue_buffers(PHY_VARS_UE *phy_vars_ue, LTE_DL_FRAME_PARMS *frame_parms, int carrier);
//void setup_eNB_buffers(PHY_VARS_eNB *phy_vars_eNB, LTE_DL_FRAME_PARMS *frame_parms, int carrier);
void test_config(int card, int ant, unsigned int rf_mode, int UE_flag);

unsigned int build_rflocal(int txi, int txq, int rxi, int rxq)
{
  return (txi + (txq<<6) + (rxi<<12) + (rxq<<18));
}
unsigned int build_rfdc(int dcoff_i_rxfe, int dcoff_q_rxfe)
{
  return (dcoff_i_rxfe + (dcoff_q_rxfe<<8));
}

void signal_handler(int sig)
{
  void *array[10];
  size_t size;

  if (sig==SIGSEGV) {
    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, 2);
    exit(-1);
  } else {
    oai_exit=1;
  }
}

void exit_fun(const char* s)
{
  void *array[10];
  size_t size;

  printf("Exiting: %s\n",s);

  oai_exit=1;
  //rt_sleep_ns(FRAME_PERIOD);

  //exit (-1);
}

#ifdef XFORMS
void *scope_thread(void *arg)
{
  int16_t prach_corr[1024], i;
  char stats_buffer[16384];
  //FILE *UE_stats, *eNB_stats;
  int len=0;
  struct sched_param sched_param;

  sched_param.sched_priority = sched_get_priority_min(SCHED_FIFO)+1;
  sched_setscheduler(0, SCHED_FIFO,&sched_param);

  printf("Scope thread has priority %d\n",sched_param.sched_priority);

  /*
    if (UE_flag==1)
    UE_stats  = fopen("UE_stats.txt", "w");
    else
    eNB_stats = fopen("eNB_stats.txt", "w");
  */

  while (!oai_exit) {
    if (UE_flag==1) {
      len = dump_ue_stats (PHY_vars_UE_g[0], stats_buffer, 0, mode,rx_input_level_dBm);
      fl_set_object_label(form_stats->stats_text, stats_buffer);
      //rewind (UE_stats);
      //fwrite (stats_buffer, 1, len, UE_stats);

      phy_scope_UE(form_ue[UE_id],
                   PHY_vars_UE_g[UE_id],
                   eNB_id,
                   UE_id,7);

    } else {
      len = dump_eNB_stats (PHY_vars_eNB_g[0], stats_buffer, 0);
      fl_set_object_label(form_stats->stats_text, stats_buffer);

      //rewind (eNB_stats);
      //fwrite (stats_buffer, 1, len, eNB_stats);
      for(UE_id=0; UE_id<scope_enb_num_ue; UE_id++) {
        phy_scope_eNB(form_enb[UE_id],
                      PHY_vars_eNB_g[eNB_id],
                      UE_id);
      }

    }

    //printf("doing forms\n");
    usleep(100000);
  }

  //fclose (UE_stats);
  //fclose (eNB_stats);

  pthread_exit((void*)arg);
}
#endif

int dummy_tx_buffer[3840*4] __attribute__((aligned(16)));

#ifdef EMOS
#define NO_ESTIMATES_DISK 20 //No. of estimates that are aquired before dumped to disk
int channel_buffer_size =  SAMPLES_PER_SLOT*4; //one slot, 4 byte per sample


void *emos_thread (void *arg)
{
  char c;
  char *fifo2file_buffer, *fifo2file_ptr;

  int fifo, counter=0, bytes;
  long long unsigned int total_bytes=0;

  FILE  *dumpfile_id;
  char  dumpfile_name[1024];
  time_t starttime_tmp;
  struct tm starttime;

  time_t timer;
  struct tm *now;

  struct gps_data_t *gps_data = NULL;
  struct gps_fix_t dummy_gps_data;

  struct sched_param sched_param;
  int ret;

  sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO)-1;
  sched_setscheduler(0, SCHED_FIFO,&sched_param);

  printf("EMOS thread has priority %d\n",sched_param.sched_priority);

  timer = time(NULL);
  now = localtime(&timer);

  memset(&dummy_gps_data,1,sizeof(struct gps_fix_t));

#if GPSD_API_MAJOR_VERSION>=5
  ret = gps_open("127.0.0.1","2947",gps_data);

  if (ret!=0)
#else
  gps_data = gps_open("127.0.0.1","2947");

  if (gps_data == NULL)
#endif
  {
    printf("[EMOS] Could not open GPS\n");
    //exit(-1);
  }

#if GPSD_API_MAJOR_VERSION>=4
  else if (gps_stream(gps_data, WATCH_ENABLE,NULL) != 0)
#else
  else if (gps_query(gps_data, "w+x") != 0)
#endif
  {
    //sprintf(tmptxt,"Error sending command to GPS, gps_data = %x", gps_data);
    printf("[EMOS] Error sending command to GPS\n");
    //exit(-1);
  } else
    printf("[EMOS] Opened GPS, gps_data=%p\n", gps_data);

  /*
  if (UE_flag==0)
    channel_buffer_size = sizeof(fifo_dump_emos_eNB);
  else
    channel_buffer_size = sizeof(fifo_dump_emos_UE);
  */

  // allocate memory for NO_FRAMES_DISK channes estimations
  fifo2file_buffer = malloc(NO_ESTIMATES_DISK*channel_buffer_size);
  fifo2file_ptr = fifo2file_buffer;

  if (fifo2file_buffer == NULL) {
    printf("[EMOS] Cound not allocate memory for fifo2file_buffer\n");
    exit(EXIT_FAILURE);
  }

  if ((fifo = open(CHANSOUNDER_FIFO_DEV, O_RDONLY)) < 0) {
    fprintf(stderr, "[EMOS] Error opening the fifo\n");
    exit(EXIT_FAILURE);
  }


  time(&starttime_tmp);
  localtime_r(&starttime_tmp,&starttime);
  snprintf(dumpfile_name,1024,"/tmp/%s_data_%d%02d%02d_%02d%02d%02d.EMOS",
           (UE_flag==0) ? "eNB" : "UE",
           1900+starttime.tm_year, starttime.tm_mon+1, starttime.tm_mday, starttime.tm_hour, starttime.tm_min, starttime.tm_sec);

  dumpfile_id = fopen(dumpfile_name,"w");

  if (dumpfile_id == NULL) {
    fprintf(stderr, "[EMOS] Error opening dumpfile %s\n",dumpfile_name);
    exit(EXIT_FAILURE);
  }


  printf("[EMOS] starting dump, channel_buffer_size=%d ...\n",channel_buffer_size);

  while (!oai_exit) {
    bytes = rtf_read_timed(fifo, fifo2file_ptr, channel_buffer_size,100);

    if (bytes<=0)
      continue;

    /*
    if (UE_flag==0)
    printf("eNB: count %d, frame %d, read: %d bytes from the fifo\n",counter, ((fifo_dump_emos_eNB*)fifo2file_ptr)->frame_tx,bytes);
    else
    printf("UE: count %d, frame %d, read: %d bytes from the fifo\n",counter, ((fifo_dump_emos_UE*)fifo2file_ptr)->frame_rx,bytes);
    */

    fifo2file_ptr += channel_buffer_size;
    counter ++;
    total_bytes += bytes;

    if ((counter%NO_ESTIMATES_DISK)==0) {
      //reset stuff
      fifo2file_ptr = fifo2file_buffer;
      //counter = 0;

      //flush buffer to disk
      if (fwrite(fifo2file_buffer, sizeof(char), NO_ESTIMATES_DISK*channel_buffer_size, dumpfile_id) != NO_ESTIMATES_DISK*channel_buffer_size) {
        fprintf(stderr, "[EMOS] Error writing to dumpfile\n");
        exit(EXIT_FAILURE);
      }

      /*
      if (gps_data)
        {
          if (gps_poll(gps_data) != 0) {
      printf("[EMOS] problem polling data from gps\n");
          }
          else {
      printf("[EMOS] lat %g, lon %g\n",gps_data->fix.latitude,gps_data->fix.longitude);
          }
          if (fwrite(&(gps_data->fix), sizeof(char), sizeof(struct gps_fix_t), dumpfile_id) != sizeof(struct gps_fix_t))
      {
        printf("[EMOS] Error writing to dumpfile, stopping recording\n");
        exit(EXIT_FAILURE);
      }
        }
      else
        {
          printf("[EMOS] WARNING: No GPS data available, storing dummy packet\n");
          if (fwrite(&(dummy_gps_data), sizeof(char), sizeof(struct gps_fix_t), dumpfile_id) != sizeof(struct gps_fix_t))
      {
        printf("[EMOS] Error writing to dumpfile, stopping recording\n");
        exit(EXIT_FAILURE);
      }
        }
      */
    }

    if ((counter%2000)==0)
      printf("[EMOS] count %d (%d sec), total bytes wrote %llu\n", counter, counter/2000, total_bytes);
  }

  free(fifo2file_buffer);
  fclose(dumpfile_id);
  close(fifo);

  pthread_exit((void*) arg);

}
#endif

/* This is the main eNB thread. It gets woken up by the kernel driver using the RTAI message mechanism (rt_send and rt_receive). */
static void *eNB_thread(void *arg)
{
#ifdef RTAI
  RT_TASK *task;
  RTIME now;
#endif
  unsigned char slot=0,last_slot, next_slot;
  int hw_slot,frame=0;
  unsigned int msg1;
  unsigned int aa,slot_offset, slot_offset_F;
  int diff;
  int delay_cnt;
  RTIME time_in, time_diff;
  int mbox_target=0,mbox_current=0;
  int i,ret;
  int tx_offset;
  int bytes;

#ifdef RTAI
  task = rt_task_init_schmod(nam2num("TASK0"), 0, 0, 0, SCHED_FIFO, 0xF);
  LOG_D(HW,"Started eNB thread (id %p)\n",task);
#ifndef TIMER_ONESHOT_MODE
  now = rt_get_time();
  ret = rt_task_make_periodic(task, now, nano2count(500000LL));

  if (ret!=0)
    LOG_E(HW,"Problem with periodic timer\n");

#endif
#endif

#ifdef HARD_RT
  rt_make_hard_real_time();
#endif

  mlockall(MCL_CURRENT | MCL_FUTURE);

  timing_info.time_min = 100000000ULL;
  timing_info.time_max = 0;
  timing_info.time_avg = 0;
  timing_info.n_samples = 0;

  while (!oai_exit) {
    hw_slot = (((((volatile unsigned int *)DAQ_MBOX)[0]+1)%150)<<1)/15;
    //LOG_D(HW,"eNB frame %d, time %llu: slot %d, hw_slot %d (mbox %d)\n",frame,rt_get_time_ns(),slot,hw_slot,((unsigned int *)DAQ_MBOX)[0]);
    //this is the mbox counter where we should be
    //mbox_target = ((((slot+1)%20)*15+1)>>1)%150;
    mbox_target = mbox_bounds[slot];
    //this is the mbox counter where we are
    mbox_current = ((volatile unsigned int *)DAQ_MBOX)[0];

    //this is the time we need to sleep in order to synchronize with the hw (in multiples of DAQ_PERIOD)
    if ((mbox_current>=135) && (mbox_target<15)) //handle the frame wrap-arround
      diff = 150-mbox_current+mbox_target;
    else if ((mbox_current<15) && (mbox_target>=135))
      diff = -150+mbox_target-mbox_current;
    else
      diff = mbox_target - mbox_current;

    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SLOT_NUMBER_ENB, slot);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_ENB, frame);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *((volatile unsigned int *) openair0_exmimo_pci[card].rxcnt_ptr[0]));
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DIFF, diff);

    time_in = rt_get_time_ns();
    //LOG_D(HW,"eNB Frame %d delaycnt %d : hw_slot %d (%d), slot %d, (slot+1)*15=%d, diff %d, time %llu\n",frame,delay_cnt,hw_slot,((unsigned int *)DAQ_MBOX)[0],slot,(((slot+1)*15)>>1),diff,time_in);
    //LOG_D(HW,"eNB Frame %d, time %llu: sleeping for %llu (slot %d, hw_slot %d, diff %d, mbox %d, delay_cnt %d)\n", frame, time_in, diff*DAQ_PERIOD,slot,hw_slot,diff,((volatile unsigned int *)DAQ_MBOX)[0],delay_cnt);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RT_SLEEP,1);
#ifdef TIMER_ONESHOT_MODE
    //ret = rt_sleep_ns(DAQ_PERIOD * (slot%4==0?6:8));
    ret = rt_sleep_ns(500000);

    if (ret)
      LOG_D(HW,"eNB Frame %d, time %llu: rt_sleep_ns returned %d\n",frame, time_in);

#else
    rt_task_wait_period();
#endif
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RT_SLEEP,0);

    //hw_slot = (((((volatile unsigned int *)DAQ_MBOX)[0]+1)%150)<<1)/15;
    //LOG_D(HW,"eNB Frame %d : hw_slot %d, time %llu\n",frame,hw_slot,rt_get_time_ns());

    mbox_current = ((volatile unsigned int *)DAQ_MBOX)[0];

    if ((mbox_current>=135) && (mbox_target<15)) //handle the frame wrap-arround
      diff = 150-mbox_current+mbox_target;
    else if ((mbox_current<15) && (mbox_target>=135))
      diff = -150+mbox_target-mbox_current;
    else
      diff = mbox_target - mbox_current;


    last_slot = (slot)%LTE_SLOTS_PER_FRAME;

    if (last_slot <0)
      last_slot+=20;

    next_slot = (slot+3)%LTE_SLOTS_PER_FRAME;

    slot++;

    if (slot==20) {
      slot=0;
      frame++;
    }

    if (frame==1000)
      oai_exit=1;

#if defined(ENABLE_ITTI)
    itti_update_lte_time(frame, slot);
#endif
  }

  LOG_D(HW,"eNB_thread: finished, ran %d times.\n",frame);

#ifdef HARD_RT
  rt_make_soft_real_time();
#endif

  // clean task
#ifdef RTAI
  rt_task_delete(task);
#endif
  LOG_D(HW,"Task deleted. returning\n");
  return 0;
}


int main(int argc, char **argv)
{

#ifdef RTAI
  RT_TASK *task;
  RTIME period;
#endif
  int i,j,aa;
  void *status;

  /*
  uint32_t rf_mode_max[4]     = {55759,55759,55759,55759};
  uint32_t rf_mode_med[4]     = {39375,39375,39375,39375};
  uint32_t rf_mode_byp[4]     = {22991,22991,22991,22991};
  */
  uint32_t my_rf_mode = RXEN + TXEN + TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM + DMAMODE_RX + DMAMODE_TX;
  uint32_t rf_mode_base = TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM;
  uint32_t rf_mode[4]     = {my_rf_mode,0,0,0};
  uint32_t rf_local[4]    = {8255000,8255000,8255000,8255000}; // UE zepto
  //{8254617, 8254617, 8254617, 8254617}; //eNB khalifa
  //{8255067,8254810,8257340,8257340}; // eNB PETRONAS

  uint32_t rf_vcocal[4]   = {910,910,910,910};
  uint32_t rf_vcocal_850[4] = {2015, 2015, 2015, 2015};
  uint32_t rf_rxdc[4]     = {32896,32896,32896,32896};
  uint32_t rxgain[4]      = {20,20,20,20};
  uint32_t txgain[4]      = {20,20,20,20};

  uint16_t Nid_cell = 0;
  uint8_t  cooperation_flag=0, transmission_mode=1, abstraction_flag=0;
  uint8_t beta_ACK=0,beta_RI=0,beta_CQI=2;

  int c;
  char do_forms=0;
  unsigned int fd;
  unsigned int tcxo = 114;

  int amp;
  uint8_t prach_fmt;
  int N_ZC;

  char rxg_fname[100];
  char txg_fname[100];
  char rflo_fname[100];
  char rfdc_fname[100];
  FILE *rxg_fd=NULL;
  FILE *txg_fd=NULL;
  FILE *rflo_fd=NULL;
  FILE *rfdc_fd=NULL;
  unsigned int rxg_max[4]= {133,133,133,133}, rxg_med[4]= {127,127,127,127}, rxg_byp[4]= {120,120,120,120};
  int tx_max_power=0;

  char line[1000];
  int l;
  int ret, ant;
  int ant_offset=0;

  int error_code;
  char *itti_dump_file = NULL;

  const struct option long_options[] = {
    {"calib-ue-rx", required_argument, NULL, 256},
    {"calib-ue-rx-med", required_argument, NULL, 257},
    {"calib-ue-rx-byp", required_argument, NULL, 258},
    {"debug-ue-prach", no_argument, NULL, 259},
    {"no-L2-connect", no_argument, NULL, 260},
    {NULL, 0, NULL, 0}
  };

  //mode = normal_txrx;


  while ((c = getopt_long (argc, argv, "C:K:O:ST:UdF:V",long_options,NULL)) != -1) {
    switch (c) {
    case 'V':
      ouput_vcd = 1;
      break;

    case 'd':
      do_forms=1;
      break;

    case 'U':
      UE_flag = 1;
      break;

    case 'C':
      carrier_freq[0] = atoi(optarg);
      carrier_freq[1] = atoi(optarg);
      carrier_freq[2] = atoi(optarg);
      carrier_freq[3] = atoi(optarg);
      break;

    case 'S':
      fs4_test=1;
      break;

    case 'T':
      tcxo=atoi(optarg);
      break;

    case 'K':
#if defined(ENABLE_ITTI)
      itti_dump_file = strdup(optarg);
#else
      printf("-K option is disabled when ENABLE_ITTI is not defined\n");
#endif
      break;

    case 'O':
#if defined(ENABLE_USE_MME)
      EPC_MODE_ENABLED = 1;

      if (optarg == NULL) { /* No IP address provided: use localhost */
        memcpy(&EPC_MODE_MME_ADDRESS[0], "127.0.0.1", 10);
      } else {
        uint8_t ip_length = strlen(optarg) + 1;
        memcpy(&EPC_MODE_MME_ADDRESS[0], optarg,
               ip_length > 16 ? 16 : ip_length);
      }

#else
      printf("You enabled mme mode without s1ap compiled...\n");
#endif
      break;

    case 'F':
      sprintf(rxg_fname,"%srxg.lime",optarg);
      rxg_fd = fopen(rxg_fname,"r");

      if (rxg_fd) {
        printf("Loading RX Gain parameters from %s\n",rxg_fname);
        l=0;

        while (fgets(line, sizeof(line), rxg_fd)) {
          if ((strlen(line)==0) || (*line == '#')) continue; //ignore empty or comment lines
          else {
            if (l==0) sscanf(line,"%d %d %d %d",&rxg_max[0],&rxg_max[1],&rxg_max[2],&rxg_max[3]);

            if (l==1) sscanf(line,"%d %d %d %d",&rxg_med[0],&rxg_med[1],&rxg_med[2],&rxg_med[3]);

            if (l==2) sscanf(line,"%d %d %d %d",&rxg_byp[0],&rxg_byp[1],&rxg_byp[2],&rxg_byp[3]);

            l++;
          }
        }
      } else
        printf("%s not found, running with defaults\n",rxg_fname);

      sprintf(txg_fname,"%stxg.lime",optarg);
      txg_fd = fopen(txg_fname,"r");

      if (txg_fd) {
        printf("Loading TX Gain parameters from %s\n",txg_fname);
        l=0;

        while (fgets(line, sizeof(line), txg_fd)) {
          if ((strlen(line)==0) || (*line == '#')) {
            continue; //ignore empty or comment lines
          } else {
            if (l==0) sscanf(line,"%d %d %d %d",&txgain[0],&txgain[1],&txgain[2],&txgain[3]);

            if (l==1) sscanf(line,"%d",&tx_max_power);

            l++;
          }
        }
      } else
        printf("%s not found, running with defaults\n",txg_fname);

      sprintf(rflo_fname,"%srflo.lime",optarg);
      rflo_fd = fopen(rflo_fname,"r");

      if (rflo_fd) {
        printf("Loading RF LO parameters from %s\n",rflo_fname);
        fscanf(rflo_fd,"%d %d %d %d",&rf_local[0],&rf_local[1],&rf_local[2],&rf_local[3]);
      } else
        printf("%s not found, running with defaults\n",rflo_fname);

      sprintf(rfdc_fname,"%srfdc.lime",optarg);
      rfdc_fd = fopen(rfdc_fname,"r");

      if (rfdc_fd) {
        printf("Loading RF DC parameters from %s\n",rfdc_fname);
        fscanf(rfdc_fd,"%d %d %d %d",&rf_rxdc[0],&rf_rxdc[1],&rf_rxdc[2],&rf_rxdc[3]);
      } else
        printf("%s not found, running with defaults\n",rfdc_fname);

      break;

      /*
      case 256:
      mode = rx_calib_ue;
      rx_input_level_dBm = atoi(optarg);
      printf("Running with UE calibration on (LNA max), input level %d dBm\n",rx_input_level_dBm);
      break;
      case 257:
      mode = rx_calib_ue_med;
      rx_input_level_dBm = atoi(optarg);
      printf("Running with UE calibration on (LNA med), input level %d dBm\n",rx_input_level_dBm);
      break;
      case 258:
      mode = rx_calib_ue_byp;
      rx_input_level_dBm = atoi(optarg);
      printf("Running with UE calibration on (LNA byp), input level %d dBm\n",rx_input_level_dBm);
      break;
      case 259:
      mode = debug_prach;
      break;
      case 260:
      mode = no_L2_connect;
      break;
      */
    default:
      break;
    }
  }

  if (UE_flag==1)
    printf("configuring for UE\n");
  else
    printf("configuring for eNB\n");

  //randominit (0);
  //set_taus_seed (0);

  // initialize the log (see log.h for details)
  logInit();

#if defined(ENABLE_ITTI)
  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info, messages_definition_xml, itti_dump_file);

# if defined(ENABLE_USE_MME)

  if (itti_create_task(TASK_SCTP, sctp_eNB_task, NULL) < 0) {
    LOG_E(EMU, "Create task failed");
    LOG_D(EMU, "Initializing SCTP task interface: FAILED\n");
    return -1;
  }

  if (itti_create_task(TASK_S1AP, s1ap_eNB_task, NULL) < 0) {
    LOG_E(EMU, "Create task failed");
    LOG_D(EMU, "Initializing S1AP task interface: FAILED\n");
    return -1;
  }

# endif

  if (itti_create_task(TASK_L2L1, l2l1_task, NULL) < 0) {
    LOG_E(EMU, "Create task failed");
    LOG_D(EMU, "Initializing L2L1 task interface: FAILED\n");
    return -1;
  }

  // Handle signals until all tasks are terminated
  //   itti_wait_tasks_end();
#endif

  if (ouput_vcd) {
    if (UE_flag==1)
      VCD_SIGNAL_DUMPER_INIT("/tmp/openair_dump_UE.vcd");
    else
      VCD_SIGNAL_DUMPER_INIT("/tmp/openair_dump_eNB.vcd");
  }

#ifdef PDCP_USE_NETLINK
  netlink_init();
#endif

  // to make a graceful exit when ctrl-c is pressed
  signal(SIGSEGV, signal_handler);
  signal(SIGINT, signal_handler);

#ifndef RTAI
  check_clock();
#endif

  g_log->log_component[HW].level = LOG_DEBUG;
  g_log->log_component[HW].flag  = LOG_HIGH;
#ifdef OPENAIR2
  g_log->log_component[PHY].level = LOG_INFO;
#else
  g_log->log_component[PHY].level = LOG_INFO;
#endif
  g_log->log_component[PHY].flag  = LOG_HIGH;
  g_log->log_component[MAC].level = LOG_INFO;
  g_log->log_component[MAC].flag  = LOG_HIGH;
  g_log->log_component[RLC].level = LOG_INFO;
  g_log->log_component[RLC].flag  = LOG_HIGH;
  g_log->log_component[PDCP].level = LOG_INFO;
  g_log->log_component[PDCP].flag  = LOG_HIGH;
  g_log->log_component[OTG].level = LOG_INFO;
  g_log->log_component[OTG].flag  = LOG_HIGH;
  g_log->log_component[RRC].level = LOG_INFO;
  g_log->log_component[RRC].flag  = LOG_HIGH;


  // Initialize card
  ret = openair0_open();

  if ( ret != 0 ) {
    if (ret == -1)
      printf("Error opening /dev/openair0");

    if (ret == -2)
      printf("Error mapping bigshm");

    if (ret == -3)
      printf("Error mapping RX or TX buffer");

    return(ret);
  }

  printf ("Detected %d number of cards, %d number of antennas.\n", openair0_num_detected_cards, openair0_num_antennas[card]);

  p_exmimo_config = openair0_exmimo_pci[card].exmimo_config_ptr;
  p_exmimo_id     = openair0_exmimo_pci[card].exmimo_id_ptr;

  printf("Card %d: ExpressMIMO %d, HW Rev %d, SW Rev 0x%d\n", card, p_exmimo_id->board_exmimoversion, p_exmimo_id->board_hwrev, p_exmimo_id->board_swrev);

  if (p_exmimo_id->board_swrev>=BOARD_SWREV_CNTL2)
    p_exmimo_config->framing.eNB_flag   = 0;
  else
    p_exmimo_config->framing.eNB_flag   = !UE_flag;

  p_exmimo_config->framing.tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_LSB;

  for (ant=0; ant<4; ant++)
    p_exmimo_config->framing.resampling_factor[ant] = RESAMPLING_FACTOR;

  /*
  for (ant=0;ant<max(frame_parms->nb_antennas_tx,frame_parms->nb_antennas_rx);ant++)
    p_exmimo_config->rf.rf_mode[ant] = rf_mode_base;
  for (ant=0;ant<frame_parms->nb_antennas_tx;ant++)
    p_exmimo_config->rf.rf_mode[ant] += (TXEN + DMAMODE_TX);
  for (ant=0;ant<frame_parms->nb_antennas_rx;ant++)
    p_exmimo_config->rf.rf_mode[ant] += (RXEN + DMAMODE_RX);
  for (ant=max(frame_parms->nb_antennas_tx,frame_parms->nb_antennas_rx);ant<4;ant++) {
    p_exmimo_config->rf.rf_mode[ant] = 0;
    carrier_freq[ant] = 0; //this turns off all other LIMEs
  }
  */

  ant_offset = 0;

  for (ant=0; ant<4; ant++) {
    if (ant==ant_offset) {
      //if (1) {
      p_exmimo_config->rf.rf_mode[ant] = rf_mode_base;
      //p_exmimo_config->rf.rf_mode[ant] += (TXEN + DMAMODE_TX);
      p_exmimo_config->rf.rf_mode[ant] += (RXEN + DMAMODE_RX);
    } else {
      p_exmimo_config->rf.rf_mode[ant] = 0;
      carrier_freq[ant] = 0; //this turns off all other LIMEs
    }
  }

  for (ant = 0; ant<4; ant++) {
    p_exmimo_config->rf.do_autocal[ant] = 1;
    p_exmimo_config->rf.rf_freq_rx[ant] = carrier_freq[ant];
    p_exmimo_config->rf.rf_freq_tx[ant] = carrier_freq[ant];
    p_exmimo_config->rf.rx_gain[ant][0] = rxgain[ant];
    p_exmimo_config->rf.tx_gain[ant][0] = txgain[ant];

    p_exmimo_config->rf.rf_local[ant]   = rf_local[ant];
    p_exmimo_config->rf.rf_rxdc[ant]    = rf_rxdc[ant];

    if ((carrier_freq[ant] >= 850000000) && (carrier_freq[ant] <= 865000000)) {
      p_exmimo_config->rf.rf_vcocal[ant]  = rf_vcocal_850[ant];
      p_exmimo_config->rf.rffe_band_mode[ant] = DD_TDD;
    } else if ((carrier_freq[ant] >= 1900000000) && (carrier_freq[ant] <= 2000000000)) {
      p_exmimo_config->rf.rf_vcocal[ant]  = rf_vcocal[ant];
      p_exmimo_config->rf.rffe_band_mode[ant] = B19G_TDD;
    } else {
      p_exmimo_config->rf.rf_vcocal[ant]  = rf_vcocal[ant];
      p_exmimo_config->rf.rffe_band_mode[ant] = 0;
    }

    p_exmimo_config->rf.rffe_gain_txlow[ant] = 31;
    p_exmimo_config->rf.rffe_gain_txhigh[ant] = 31;
    p_exmimo_config->rf.rffe_gain_rxfinal[ant] = 52;
    p_exmimo_config->rf.rffe_gain_rxlow[ant] = 31;
  }


  number_of_cards = openair0_num_detected_cards;
  /*
  if (p_exmimo_id->board_exmimoversion==1) //ExpressMIMO1
    openair_daq_vars.timing_advance = 138;
  else //ExpressMIMO2
    openair_daq_vars.timing_advance = 0;
  */

  openair0_dump_config(card);

  printf("EXMIMO_CONFIG: rf_mode 0x %x %x %x %x, [0]: TXRXEn %d, TXLPFEn %d, TXLPF %d, RXLPFEn %d, RXLPF %d, RFBB %d, LNA %d, LNAGain %d, RXLPFMode %d, SWITCH %d, rf_rxdc %d, rf_local %d, rf_vcocal %d\n",
         p_exmimo_config->rf.rf_mode[0],
         p_exmimo_config->rf.rf_mode[1],
         p_exmimo_config->rf.rf_mode[2],
         p_exmimo_config->rf.rf_mode[3],
         (p_exmimo_config->rf.rf_mode[0]&3),  // RXen+TXen
         (p_exmimo_config->rf.rf_mode[0]&4)>>2,         //TXLPFen
         (p_exmimo_config->rf.rf_mode[0]&TXLPFMASK)>>3, //TXLPF
         (p_exmimo_config->rf.rf_mode[0]&128)>>7,      //RXLPFen
         (p_exmimo_config->rf.rf_mode[0]&RXLPFMASK)>>8, //TXLPF
         (p_exmimo_config->rf.rf_mode[0]&RFBBMASK)>>16, // RFBB mode
         (p_exmimo_config->rf.rf_mode[0]&LNAMASK)>>12, // RFBB mode
         (p_exmimo_config->rf.rf_mode[0]&LNAGAINMASK)>>14, // RFBB mode
         (p_exmimo_config->rf.rf_mode[0]&RXLPFMODEMASK)>>19, // RXLPF mode
         (p_exmimo_config->framing.tdd_config&TXRXSWITCH_MASK)>>1, // Switch mode
         p_exmimo_config->rf.rf_rxdc[0],
         p_exmimo_config->rf.rf_local[0],
         p_exmimo_config->rf.rf_vcocal[0]);

  for (ant=0; ant<4; ant++)
    p_exmimo_config->rf.do_autocal[ant] = 0;

#ifdef EMOS
  error_code = rtf_create(CHANSOUNDER_FIFO_MINOR,CHANSOUNDER_FIFO_SIZE);

  if (error_code==0)
    printf("[OPENAIR][SCHED][INIT] Created EMOS FIFO %d\n",CHANSOUNDER_FIFO_MINOR);
  else if (error_code==ENODEV)
    printf("[OPENAIR][SCHED][INIT] Problem: EMOS FIFO %d is greater than or equal to RTF_NO\n",CHANSOUNDER_FIFO_MINOR);
  else if (error_code==ENOMEM)
    printf("[OPENAIR][SCHED][INIT] Problem: cannot allocate memory for EMOS FIFO %d\n",CHANSOUNDER_FIFO_MINOR);
  else
    printf("[OPENAIR][SCHED][INIT] Problem creating EMOS FIFO %d, error_code %d\n",CHANSOUNDER_FIFO_MINOR,error_code);

#endif

  mlockall(MCL_CURRENT | MCL_FUTURE);

#ifdef RTAI
  // make main thread LXRT soft realtime
  task = rt_task_init_schmod(nam2num("MYTASK"), 9, 0, 0, SCHED_FIFO, 0xF);

  // start realtime timer and scheduler
#ifdef TIMER_ONESHOT_MODE
  rt_set_oneshot_mode();
  start_rt_timer(0);
  printf("started RTAI timer inoneshot mode\n");
#else
  rt_set_periodic_mode();
  period = start_rt_timer(nano2count(500000));
  printf("started RTAI timer with period %llu ns\n",count2nano(period));
#endif

  printf("Init mutex\n");
  //mutex = rt_get_adr(nam2num("MUTEX"));
  mutex = rt_sem_init(nam2num("MUTEX"), 1);

  if (mutex==0) {
    printf("Error init mutex\n");
    exit(-1);
  } else
    printf("mutex=%p\n",mutex);

#endif

  DAQ_MBOX = (volatile unsigned int *) openair0_exmimo_pci[card].rxcnt_ptr[0];

  // this starts the DMA transfers
  if (UE_flag!=1)
    openair0_start_rt_acquisition(card);


#ifdef XFORMS

  if (do_forms==1) {
    fl_initialize (&argc, argv, NULL, 0, 0);
    form_stats = create_form_stats_form();

    if (UE_flag==1) {
      form_ue[UE_id] = create_lte_phy_scope_ue();
      sprintf (title, "LTE DL SCOPE UE");
      fl_show_form (form_ue[UE_id]->lte_phy_scope_ue, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
    } else {
      for(UE_id=0; UE_id<scope_enb_num_ue; UE_id++) {
        form_enb[UE_id] = create_lte_phy_scope_enb();
        sprintf (title, "UE%d LTE UL SCOPE eNB",UE_id+1);
        fl_show_form (form_enb[UE_id]->lte_phy_scope_enb, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
      }
    }

    fl_show_form (form_stats->stats_form, FL_PLACE_HOTSPOT, FL_FULLBORDER, "stats");

    if (UE_flag==0) {
      for (UE_id=0; UE_id<scope_enb_num_ue; UE_id++) {
        if (otg_enabled) {
          fl_set_button(form_enb[UE_id]->button_0,1);
          fl_set_object_label(form_enb[UE_id]->button_0,"DL Traffic ON");
        } else {
          fl_set_button(form_enb[UE_id]->button_0,0);
          fl_set_object_label(form_enb[UE_id]->button_0,"DL Traffic OFF");
        }
      }
    } else {
      if (openair_daq_vars.use_ia_receiver) {
        fl_set_button(form_ue[UE_id]->button_0,1);
        fl_set_object_label(form_ue[UE_id]->button_0, "IA Receiver ON");
      } else {
        fl_set_button(form_ue[UE_id]->button_0,0);
        fl_set_object_label(form_ue[UE_id]->button_0, "IA Receiver OFF");
      }
    }

    ret = pthread_create(&thread2, NULL, scope_thread, NULL);
    printf("Scope thread created, ret=%d\n",ret);
  }

#endif

#ifdef EMOS
  ret = pthread_create(&thread3, NULL, emos_thread, NULL);
  printf("EMOS thread created, ret=%d\n",ret);
#endif

  rt_sleep_ns(10*FRAME_PERIOD);

#ifndef RTAI
  pthread_attr_init (&attr_dlsch_threads);
  pthread_attr_setstacksize(&attr_dlsch_threads,OPENAIR_THREAD_STACK_SIZE);
  //attr_dlsch_threads.priority = 1;
  sched_param_dlsch.sched_priority = sched_get_priority_max(SCHED_FIFO); //OPENAIR_THREAD_PRIORITY;
  pthread_attr_setschedparam  (&attr_dlsch_threads, &sched_param_dlsch);
  pthread_attr_setschedpolicy (&attr_dlsch_threads, SCHED_FIFO);
#endif

  // start the main thread
  if (UE_flag == 1) {
    /*
    #ifdef RTAI
    thread1 = rt_thread_create(UE_thread, NULL, 100000000);
    #else
    error_code = pthread_create(&thread1, &attr_dlsch_threads, UE_thread, NULL);
    if (error_code!= 0) {
      LOG_D(HW,"[lte-softmodem.c] Could not allocate UE_thread, error %d\n",error_code);
      return(error_code);
    }
    else {
      LOG_D(HW,"[lte-softmodem.c] Allocate UE_thread successful\n");
    }
    #endif
    #ifdef DLSCH_THREAD
    init_rx_pdsch_thread();
    rt_sleep_ns(FRAME_PERIOD/10);
    init_dlsch_threads();
    #endif
    printf("UE threads created\n");
    */
  } else {
#ifdef RTAI
    thread0 = rt_thread_create(eNB_thread, NULL, 100000000);
#else
    error_code = pthread_create(&thread0, &attr_dlsch_threads, eNB_thread, NULL);

    if (error_code!= 0) {
      LOG_D(HW,"[lte-softmodem.c] Could not allocate eNB_thread, error %d\n",error_code);
      return(error_code);
    } else {
      LOG_D(HW,"[lte-softmodem.c] Allocate eNB_thread successful\n");
    }

#endif
#ifdef ULSCH_THREAD
    init_ulsch_threads();
#endif
    printf("eNB threads created\n");
  }


  // wait for end of program
  printf("TYPE <CTRL-C> TO TERMINATE\n");

  //getchar();
  while (oai_exit==0)
    rt_sleep_ns(FRAME_PERIOD);

  // stop threads
#ifdef XFORMS
  printf("waiting for XFORMS thread\n");

  if (do_forms==1) {
    pthread_join(thread2,&status);
    fl_hide_form(form_stats->stats_form);
    fl_free_form(form_stats->stats_form);

    if (UE_flag==1) {
      fl_hide_form(form_ue[UE_id]->lte_phy_scope_ue);
      fl_free_form(form_ue[UE_id]->lte_phy_scope_ue);
    } else {
      for(UE_id=0; UE_id<scope_enb_num_ue; UE_id++) {
        fl_hide_form(form_enb[UE_id]->lte_phy_scope_enb);
        fl_free_form(form_enb[UE_id]->lte_phy_scope_enb);
      }
    }
  }

#endif

  printf("stopping MODEM threads\n");

  // cleanup
  if (UE_flag == 1) {
    /*
    #ifdef RTAI
    rt_thread_join(thread1);
    #else
    pthread_join(thread1,&status);
    #endif
    #ifdef DLSCH_THREAD
    cleanup_dlsch_threads();
    cleanup_rx_pdsch_thread();
    #endif
    */
  } else {
#ifdef RTAI
    rt_thread_join(thread0);
#else
    pthread_join(thread0,&status);
#endif
#ifdef ULSCH_THREAD
    cleanup_ulsch_threads();
#endif
  }

#ifdef OPENAIR2
  //cleanup_pdcp_thread();
#endif

#ifdef RTAI
  stop_rt_timer();
#endif

  printf("stopping card\n");
  openair0_stop(card);
  printf("closing openair0_lib\n");
  openair0_close();

#ifdef EMOS
  printf("waiting for EMOS thread\n");
  pthread_cancel(thread3);
  pthread_join(thread3,&status);
#endif

#ifdef EMOS
  error_code = rtf_destroy(CHANSOUNDER_FIFO_MINOR);
  printf("[OPENAIR][SCHED][CLEANUP] EMOS FIFO closed, error_code %d\n", error_code);
#endif

  if (ouput_vcd)
    VCD_SIGNAL_DUMPER_CLOSE();

  logClean();

  return 0;
}

void test_config(int card, int ant, unsigned int rf_mode, int UE_flag)
{
  p_exmimo_config->framing.eNB_flag   = !UE_flag;
  p_exmimo_config->framing.tdd_config = 0;
  p_exmimo_config->framing.resampling_factor[ant] = 2;

  p_exmimo_config->rf.rf_freq_rx[ant] = 1907600000;
  p_exmimo_config->rf.rf_freq_tx[ant] = 1907600000;;
  p_exmimo_config->rf.rx_gain[ant][0] = 20;
  p_exmimo_config->rf.tx_gain[ant][0] = 10;
  p_exmimo_config->rf.rf_mode[ant] = rf_mode;

  p_exmimo_config->rf.rf_local[ant] = build_rflocal(20,25,26,04);
  p_exmimo_config->rf.rf_rxdc[ant] = build_rfdc(128, 128);
  p_exmimo_config->rf.rf_vcocal[ant] = (0xE<<6) + 0xE;
}

/*
void setup_ue_buffers(PHY_VARS_UE *phy_vars_ue, LTE_DL_FRAME_PARMS *frame_parms, int carrier) {

  int i;
  if (phy_vars_ue) {

    if ((frame_parms->nb_antennas_rx>1) && (carrier>0)) {
      printf("RX antennas > 1 and carrier > 0 not possible\n");
      exit(-1);
    }

    if ((frame_parms->nb_antennas_tx>1) && (carrier>0)) {
      printf("TX antennas > 1 and carrier > 0 not possible\n");
      exit(-1);
    }

    // replace RX signal buffers with mmaped HW versions
    for (i=0;i<frame_parms->nb_antennas_rx;i++) {
      free(phy_vars_ue->lte_ue_common_vars.rxdata[i]);
      phy_vars_ue->lte_ue_common_vars.rxdata[i] = (int32_t*) openair0_exmimo_pci[card].adc_head[i+carrier];


      printf("rxdata[%d] @ %p\n",i,phy_vars_ue->lte_ue_common_vars.rxdata[i]);
    }
    for (i=0;i<frame_parms->nb_antennas_tx;i++) {
      free(phy_vars_ue->lte_ue_common_vars.txdata[i]);
      phy_vars_ue->lte_ue_common_vars.txdata[i] = (int32_t*) openair0_exmimo_pci[card].dac_head[i+carrier];

      printf("txdata[%d] @ %p\n",i,phy_vars_ue->lte_ue_common_vars.txdata[i]);
    }
  }
}

void setup_eNB_buffers(PHY_VARS_eNB *phy_vars_eNB, LTE_DL_FRAME_PARMS *frame_parms, int carrier) {

  int i,j;

  if (phy_vars_eNB) {
    if ((frame_parms->nb_antennas_rx>1) && (carrier>0)) {
      printf("RX antennas > 1 and carrier > 0 not possible\n");
      exit(-1);
    }

    if ((frame_parms->nb_antennas_tx>1) && (carrier>0)) {
      printf("TX antennas > 1 and carrier > 0 not possible\n");
      exit(-1);
    }

    // replace RX signal buffers with mmaped HW versions
    for (i=0;i<frame_parms->nb_antennas_rx;i++) {
        free(phy_vars_eNB->lte_eNB_common_vars.rxdata[0][i]);
        phy_vars_eNB->lte_eNB_common_vars.rxdata[0][i] = (int32_t*) openair0_exmimo_pci[card].adc_head[i+carrier];

        printf("rxdata[%d] @ %p\n",i,phy_vars_eNB->lte_eNB_common_vars.rxdata[0][i]);
        for (j=0;j<16;j++) {
            printf("rxbuffer %d: %x\n",j,phy_vars_eNB->lte_eNB_common_vars.rxdata[0][i][j]);
            phy_vars_eNB->lte_eNB_common_vars.rxdata[0][i][j] = 16-j;
        }
    }
    for (i=0;i<frame_parms->nb_antennas_tx;i++) {
        free(phy_vars_eNB->lte_eNB_common_vars.txdata[0][i]);
        phy_vars_eNB->lte_eNB_common_vars.txdata[0][i] = (int32_t*) openair0_exmimo_pci[card].dac_head[i+carrier];

        printf("txdata[%d] @ %p\n",i,phy_vars_eNB->lte_eNB_common_vars.txdata[0][i]);
        for (j=0;j<16;j++) {
            printf("txbuffer %d: %x\n",j,phy_vars_eNB->lte_eNB_common_vars.txdata[0][i][j]);
            phy_vars_eNB->lte_eNB_common_vars.txdata[0][i][j] = 16-j;
  }
    }
  }
}
*/
