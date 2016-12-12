/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file lte-enb.c
 * \brief Top-level threads for eNodeB
 * \author R. Knopp, F. Kaltenberger, Navid Nikaein
 * \date 2012
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
 * \note
 * \warning
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <sys/sysinfo.h>

#include "T.h"

#include "rt_wrapper.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "assertions.h"
#include "msc.h"

#include "PHY/types.h"

#include "PHY/defs.h"
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "../../ARCH/COMMON/common_lib.h"
#include "../../ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"

//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/vars.h"
#include "SCHED/vars.h"
#include "LAYER2/MAC/vars.h"

#include "../../SIMU/USER/init_lte.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/vars.h"
#include "LAYER2/MAC/proto.h"
#include "RRC/LITE/vars.h"
#include "PHY_INTERFACE/vars.h"

#ifdef SMBV
#include "PHY/TOOLS/smbv.h"
unsigned short config_frames[4] = {2,9,11,13};
#endif
#include "UTIL/LOG/log_extern.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "enb_config.h"
//#include "PHY/TOOLS/time_meas.h"

#ifndef OPENAIR2
#include "UTIL/OTG/otg_vars.h"
#endif

#if defined(ENABLE_ITTI)
# include "intertask_interface_init.h"
# include "create_tasks.h"
# if defined(ENABLE_USE_MME)
#   include "s1ap_eNB.h"
#ifdef PDCP_USE_NETLINK
#   include "SIMULATION/ETH_TRANSPORT/proto.h"
extern int netlink_init(void);
#endif
# endif
#endif

#ifdef XFORMS
#include "PHY/TOOLS/lte_phy_scope.h"
#include "stats.h"
#endif

// In lte-enb.c
extern int setup_eNB_buffers(PHY_VARS_eNB **phy_vars_eNB, openair0_config_t *openair0_cfg);
extern void init_eNB(eNB_func_t *, eNB_timing_t *,int,eth_params_t *,int);
extern void stop_eNB(int);
extern void kill_eNB_proc(void);

// In lte-ue.c
extern int setup_ue_buffers(PHY_VARS_UE **phy_vars_ue, openair0_config_t *openair0_cfg);
extern void fill_ue_band_info(void);
extern void init_UE(int);

#ifdef XFORMS
// current status is that every UE has a DL scope for a SINGLE eNB (eNB_id=0)
// at eNB 0, an UL scope for every UE
FD_lte_phy_scope_ue  *form_ue[NUMBER_OF_UE_MAX];
FD_lte_phy_scope_enb *form_enb[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
FD_stats_form                  *form_stats=NULL,*form_stats_l2=NULL;
char title[255];
unsigned char                   scope_enb_num_ue = 2;
#endif //XFORMS






pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.





#ifdef XFORMS
static pthread_t                forms_thread; //xforms
#endif

uint16_t runtime_phy_rx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]
uint16_t runtime_phy_tx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]


#if defined(ENABLE_ITTI)
volatile int             start_eNB = 0;
volatile int             start_UE = 0;
#endif
volatile int             oai_exit = 0;




static char              UE_flag=0;
unsigned int                    mmapped_dma=0;
int                             single_thread_flag=1;

static char                     threequarter_fs=0;

uint32_t                 downlink_frequency[MAX_NUM_CCs][4];
int32_t                  uplink_frequency_offset[MAX_NUM_CCs][4];


static char                    *conf_config_file_name = NULL;
#if defined(ENABLE_ITTI)
static char                    *itti_dump_file = NULL;
#endif

int UE_scan = 1;
int UE_scan_carrier = 0;
runmode_t mode = normal_txrx;

FILE *input_fd=NULL;


#if MAX_NUM_CCs == 1
rx_gain_t                rx_gain_mode[MAX_NUM_CCs][4] = {{max_gain,max_gain,max_gain,max_gain}};
double tx_gain[MAX_NUM_CCs][4] = {{20,0,0,0}};
double rx_gain[MAX_NUM_CCs][4] = {{110,0,0,0}};
#else
rx_gain_t                rx_gain_mode[MAX_NUM_CCs][4] = {{max_gain,max_gain,max_gain,max_gain},{max_gain,max_gain,max_gain,max_gain}};
double tx_gain[MAX_NUM_CCs][4] = {{20,0,0,0},{20,0,0,0}};
double rx_gain[MAX_NUM_CCs][4] = {{110,0,0,0},{20,0,0,0}};
#endif

double rx_gain_off = 0.0;

double sample_rate=30.72e6;
double bw = 10.0e6;

static int                      tx_max_power[MAX_NUM_CCs]; /* =  {0,0}*/;

char   rf_config_file[1024];

int chain_offset=0;
int phy_test = 0;
uint8_t usim_test = 0;


char ref[128] = "internal";
char channels[128] = "0";

int                      rx_input_level_dBm;
static int                      online_log_messages=0;
#ifdef XFORMS
extern int                      otg_enabled;
static char                     do_forms=0;
#else
int                             otg_enabled;
#endif
//int                             number_of_cards =   1;


static LTE_DL_FRAME_PARMS      *frame_parms[MAX_NUM_CCs];
eNB_func_t node_function[MAX_NUM_CCs];
eNB_timing_t node_timing[MAX_NUM_CCs];
int16_t   node_synch_ref[MAX_NUM_CCs];

uint32_t target_dl_mcs = 28; //maximum allowed mcs
uint32_t target_ul_mcs = 20;
uint32_t timing_advance = 0;
uint8_t exit_missed_slots=1;
uint64_t num_missed_slots=0; // counter for the number of missed slots


extern void reset_opp_meas(void);
extern void print_opp_meas(void);
//int transmission_mode=1;

int16_t           glog_level         = LOG_INFO;
int16_t           glog_verbosity     = LOG_MED;
int16_t           hw_log_level       = LOG_INFO;
int16_t           hw_log_verbosity   = LOG_MED;
int16_t           phy_log_level      = LOG_INFO;
int16_t           phy_log_verbosity  = LOG_MED;
int16_t           mac_log_level      = LOG_INFO;
int16_t           mac_log_verbosity  = LOG_MED;
int16_t           rlc_log_level      = LOG_INFO;
int16_t           rlc_log_verbosity  = LOG_MED;
int16_t           pdcp_log_level     = LOG_INFO;
int16_t           pdcp_log_verbosity = LOG_MED;
int16_t           rrc_log_level      = LOG_INFO;
int16_t           rrc_log_verbosity  = LOG_MED;
int16_t           opt_log_level      = LOG_INFO;
int16_t           opt_log_verbosity  = LOG_MED;

# if defined(ENABLE_USE_MME)
int16_t           gtpu_log_level     = LOG_DEBUG;
int16_t           gtpu_log_verbosity = LOG_MED;
int16_t           udp_log_level      = LOG_DEBUG;
int16_t           udp_log_verbosity  = LOG_MED;
#endif
#if defined (ENABLE_SECURITY)
int16_t           osa_log_level      = LOG_INFO;
int16_t           osa_log_verbosity  = LOG_MED;
#endif



char *rrh_UE_ip = "127.0.0.1";
int rrh_UE_port = 51000;


/* flag set by eNB conf file to specify if the radio head is local or remote (default option is local) */
uint8_t local_remote_radio = BBU_LOCAL_RADIO_HEAD;
/* struct for ethernet specific parameters given in eNB conf file */
eth_params_t *eth_params;

openair0_config_t openair0_cfg[MAX_CARDS];

double cpuf;

char uecap_xer[1024],uecap_xer_in=0;



/*---------------------BMC: timespec helpers -----------------------------*/

struct timespec min_diff_time = { .tv_sec = 0, .tv_nsec = 0 };
struct timespec max_diff_time = { .tv_sec = 0, .tv_nsec = 0 };

struct timespec clock_difftime(struct timespec start, struct timespec end)
{
  struct timespec temp;
  if ((end.tv_nsec-start.tv_nsec)<0) {
    temp.tv_sec = end.tv_sec-start.tv_sec-1;
    temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec-start.tv_sec;
    temp.tv_nsec = end.tv_nsec-start.tv_nsec;
  }
  return temp;
}

void print_difftimes(void)
{
#ifdef DEBUG
  printf("difftimes min = %lu ns ; max = %lu ns\n", min_diff_time.tv_nsec, max_diff_time.tv_nsec);
#else
  LOG_I(HW,"difftimes min = %lu ns ; max = %lu ns\n", min_diff_time.tv_nsec, max_diff_time.tv_nsec);
#endif
}

void update_difftimes(struct timespec start, struct timespec end)
{
  struct timespec diff_time = { .tv_sec = 0, .tv_nsec = 0 };
  int             changed = 0;
  diff_time = clock_difftime(start, end);
  if ((min_diff_time.tv_nsec == 0) || (diff_time.tv_nsec < min_diff_time.tv_nsec)) { min_diff_time.tv_nsec = diff_time.tv_nsec; changed = 1; }
  if ((max_diff_time.tv_nsec == 0) || (diff_time.tv_nsec > max_diff_time.tv_nsec)) { max_diff_time.tv_nsec = diff_time.tv_nsec; changed = 1; }
#if 1
  if (changed) print_difftimes();
#endif
}

/*------------------------------------------------------------------------*/

unsigned int build_rflocal(int txi, int txq, int rxi, int rxq)
{
  return (txi + (txq<<6) + (rxi<<12) + (rxq<<18));
}
unsigned int build_rfdc(int dcoff_i_rxfe, int dcoff_q_rxfe)
{
  return (dcoff_i_rxfe + (dcoff_q_rxfe<<8));
}

#if !defined(ENABLE_ITTI)
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
    printf("trying to exit gracefully...\n");
    oai_exit = 1;
  }
}
#endif
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KBLU  "\x1B[34m"
#define RESET "\033[0m"

void help (void) {
  printf (KGRN "Usage:\n");
  printf("  sudo -E lte-softmodem [options]\n");
  printf("  sudo -E ./lte-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.tm1.exmimo2.openEPC.conf -S -V -m 26 -t 16 -x 1 --ulsch-max-errors 100 -W\n\n");
  printf("Options:\n");
  printf("  --rf-config-file Configuration file for front-end (e.g. LMS7002M)\n");
  printf("  --ulsch-max-errors set the max ULSCH erros\n");
  printf("  --calib-ue-rx set UE RX calibration\n");
  printf("  --calib-ue-rx-med \n");
  printf("  --calib-ue-rxbyp\n");
  printf("  --debug-ue-prach run normal prach power ramping, but don't continue random-access\n");
  printf("  --calib-prach-tx run normal prach with maximum power, but don't continue random-access\n");
  printf("  --no-L2-connect bypass L2 and upper layers\n");
  printf("  --ue-rxgain set UE RX gain\n");
  printf("  --ue-rxgain-off external UE amplifier offset\n");
  printf("  --ue-txgain set UE TX gain\n");
  printf("  --ue-scan_carrier set UE to scan around carrier\n");
  printf("  --loop-memory get softmodem (UE) to loop through memory instead of acquiring from HW\n");
  printf("  --mmapped-dma sets flag for improved EXMIMO UE performance\n");  
  printf("  --usim-test use XOR autentication algo in case of test usim mode\n"); 
  printf("  --single-thread-disable. Disables single-thread mode in lte-softmodem\n"); 
  printf("  -C Set the downlink frequency for all component carriers\n");
  printf("  -d Enable soft scope and L1 and L2 stats (Xforms)\n");
  printf("  -F Calibrate the EXMIMO borad, available files: exmimo2_2arxg.lime exmimo2_2brxg.lime \n");
  printf("  -g Set the global log level, valide options: (9:trace, 8/7:debug, 6:info, 4:warn, 3:error)\n");
  printf("  -G Set the global log verbosity \n");
  printf("  -h provides this help message!\n");
  printf("  -K Generate ITTI analyzser logs (similar to wireshark logs but with more details)\n");
  printf("  -m Set the maximum downlink MCS\n");
  printf("  -O eNB configuration file (located in targets/PROJECTS/GENERIC-LTE-EPC/CONF\n");
  printf("  -q Enable processing timing measurement of lte softmodem on per subframe basis \n");
  printf("  -r Set the PRB, valid values: 6, 25, 50, 100  \n");    
  printf("  -S Skip the missed slots/subframes \n");    
  printf("  -t Set the maximum uplink MCS\n");
  printf("  -T Set hardware to TDD mode (default: FDD). Used only with -U (otherwise set in config file).\n");
  printf("  -U Set the lte softmodem as a UE\n");
  printf("  -W Enable L2 wireshark messages on localhost \n");
  printf("  -V Enable VCD (generated file will be located atopenair_dump_eNB.vcd, read it with target/RT/USER/eNB.gtkw\n");
  printf("  -x Set the transmission mode, valid options: 1 \n");
  printf("  -E Apply three-quarter of sampling frequency, 23.04 Msps to reduce the data rate on USB/PCIe transfers (only valid for 20 MHz)\n");
#if T_TRACER
  printf("  --T_port [port]    use given port\n");
  printf("  --T_nowait         don't wait for tracer, start immediately\n");
  printf("  --T_dont_fork      to ease debugging with gdb\n");
#endif
  printf(RESET);
  fflush(stdout);
}

void exit_fun(const char* s)
{
  int CC_id;

  if (s != NULL) {
    printf("%s %s() Exiting OAI softmodem: %s\n",__FILE__, __FUNCTION__, s);
  }

  oai_exit = 1;
  
  for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    if (UE_flag == 0) {
      if (PHY_vars_eNB_g[0][CC_id]->rfdevice.trx_end_func)
	PHY_vars_eNB_g[0][CC_id]->rfdevice.trx_end_func(&PHY_vars_eNB_g[0][CC_id]->rfdevice);
      if (PHY_vars_eNB_g[0][CC_id]->ifdevice.trx_end_func)
	PHY_vars_eNB_g[0][CC_id]->ifdevice.trx_end_func(&PHY_vars_eNB_g[0][CC_id]->ifdevice);  
    }
    else {
      if (PHY_vars_UE_g[0][CC_id]->rfdevice.trx_end_func)
	PHY_vars_UE_g[0][CC_id]->rfdevice.trx_end_func(&PHY_vars_UE_g[0][CC_id]->rfdevice);
    }
  }

#if defined(ENABLE_ITTI)
  sleep(1); //allow lte-softmodem threads to exit first
  itti_terminate_tasks (TASK_UNKNOWN);
#endif

}


#ifdef XFORMS

void reset_stats(FL_OBJECT *button, long arg)
{
  int i,j,k;
  PHY_VARS_eNB *phy_vars_eNB = PHY_vars_eNB_g[0][0];

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    for (k=0; k<8; k++) { //harq_processes
      for (j=0; j<phy_vars_eNB->dlsch[i][0]->Mlimit; j++) {
        phy_vars_eNB->UE_stats[i].dlsch_NAK[k][j]=0;
        phy_vars_eNB->UE_stats[i].dlsch_ACK[k][j]=0;
        phy_vars_eNB->UE_stats[i].dlsch_trials[k][j]=0;
      }

      phy_vars_eNB->UE_stats[i].dlsch_l2_errors[k]=0;
      phy_vars_eNB->UE_stats[i].ulsch_errors[k]=0;
      phy_vars_eNB->UE_stats[i].ulsch_consecutive_errors=0;

      for (j=0; j<phy_vars_eNB->ulsch[i]->Mlimit; j++) {
        phy_vars_eNB->UE_stats[i].ulsch_decoding_attempts[k][j]=0;
        phy_vars_eNB->UE_stats[i].ulsch_decoding_attempts_last[k][j]=0;
        phy_vars_eNB->UE_stats[i].ulsch_round_errors[k][j]=0;
        phy_vars_eNB->UE_stats[i].ulsch_round_fer[k][j]=0;
      }
    }

    phy_vars_eNB->UE_stats[i].dlsch_sliding_cnt=0;
    phy_vars_eNB->UE_stats[i].dlsch_NAK_round0=0;
    phy_vars_eNB->UE_stats[i].dlsch_mcs_offset=0;
  }
}

static void *scope_thread(void *arg)
{
  char stats_buffer[16384];
# ifdef ENABLE_XFORMS_WRITE_STATS
  FILE *UE_stats, *eNB_stats;
# endif
  int len = 0;
  struct sched_param sched_param;
  int UE_id, CC_id;
  int ue_cnt=0;

  sched_param.sched_priority = sched_get_priority_min(SCHED_FIFO)+1;
  sched_setscheduler(0, SCHED_FIFO,&sched_param);

  printf("Scope thread has priority %d\n",sched_param.sched_priority);

# ifdef ENABLE_XFORMS_WRITE_STATS

  if (UE_flag==1)
    UE_stats  = fopen("UE_stats.txt", "w");
  else
    eNB_stats = fopen("eNB_stats.txt", "w");

#endif

  while (!oai_exit) {
    if (UE_flag==1) {
      len = dump_ue_stats (PHY_vars_UE_g[0][0], &PHY_vars_UE_g[0][0]->proc.proc_rxtx[0],stats_buffer, 0, mode,rx_input_level_dBm);
      //fl_set_object_label(form_stats->stats_text, stats_buffer);
      fl_clear_browser(form_stats->stats_text);
      fl_add_browser_line(form_stats->stats_text, stats_buffer);

      phy_scope_UE(form_ue[0],
                   PHY_vars_UE_g[0][0],
                   0,
                   0,7);

    } else {
      if (PHY_vars_eNB_g[0][0]->mac_enabled==1) {
	len = dump_eNB_l2_stats (stats_buffer, 0);
	//fl_set_object_label(form_stats_l2->stats_text, stats_buffer);
	fl_clear_browser(form_stats_l2->stats_text);
	fl_add_browser_line(form_stats_l2->stats_text, stats_buffer);
      }
      len = dump_eNB_stats (PHY_vars_eNB_g[0][0], stats_buffer, 0);

      if (MAX_NUM_CCs>1)
        len += dump_eNB_stats (PHY_vars_eNB_g[0][1], &stats_buffer[len], 0);

      //fl_set_object_label(form_stats->stats_text, stats_buffer);
      fl_clear_browser(form_stats->stats_text);
      fl_add_browser_line(form_stats->stats_text, stats_buffer);

      ue_cnt=0;
      for(UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
	for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
	  //	  if ((PHY_vars_eNB_g[0][CC_id]->dlsch[UE_id][0]->rnti>0) && (ue_cnt<scope_enb_num_ue)) {
	  if ((ue_cnt<scope_enb_num_ue)) {
	    phy_scope_eNB(form_enb[CC_id][ue_cnt],
			  PHY_vars_eNB_g[0][CC_id],
			  UE_id);
	    ue_cnt++;
	  }
	}
      }

    }

    //printf("doing forms\n");
    //usleep(100000); // 100 ms
    sleep(1);
  }

  //  printf("%s",stats_buffer);

# ifdef ENABLE_XFORMS_WRITE_STATS

  if (UE_flag==1) {
    if (UE_stats) {
      rewind (UE_stats);
      fwrite (stats_buffer, 1, len, UE_stats);
      fclose (UE_stats);
    }
  } else {
    if (eNB_stats) {
      rewind (eNB_stats);
      fwrite (stats_buffer, 1, len, eNB_stats);
      fclose (eNB_stats);
    }
  }

# endif

  pthread_exit((void*)arg);
}
#endif




#if defined(ENABLE_ITTI)
void *l2l1_task(void *arg)
{
  MessageDef *message_p = NULL;
  int         result;

  itti_set_task_real_time(TASK_L2L1);
  itti_mark_task_ready(TASK_L2L1);

  if (UE_flag == 0) {
    /* Wait for the initialize message */
    printf("Wait for the ITTI initialize message\n");
    do {
      if (message_p != NULL) {
        result = itti_free (ITTI_MSG_ORIGIN_ID(message_p), message_p);
        AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
      }

      itti_receive_msg (TASK_L2L1, &message_p);

      switch (ITTI_MSG_ID(message_p)) {
      case INITIALIZE_MESSAGE:
        /* Start eNB thread */
        LOG_D(EMU, "L2L1 TASK received %s\n", ITTI_MSG_NAME(message_p));
        start_eNB = 1;
        break;

      case TERMINATE_MESSAGE:
        printf("received terminate message\n");
        oai_exit=1;
        itti_exit_task ();
        break;

      default:
        LOG_E(EMU, "Received unexpected message %s\n", ITTI_MSG_NAME(message_p));
        break;
      }
    } while (ITTI_MSG_ID(message_p) != INITIALIZE_MESSAGE);

    result = itti_free (ITTI_MSG_ORIGIN_ID(message_p), message_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  }

  do {
    // Wait for a message
    itti_receive_msg (TASK_L2L1, &message_p);

    switch (ITTI_MSG_ID(message_p)) {
    case TERMINATE_MESSAGE:
      oai_exit=1;
      itti_exit_task ();
      break;

    case ACTIVATE_MESSAGE:
      start_UE = 1;
      break;

    case DEACTIVATE_MESSAGE:
      start_UE = 0;
      break;

    case MESSAGE_TEST:
      LOG_I(EMU, "Received %s\n", ITTI_MSG_NAME(message_p));
      break;

    default:
      LOG_E(EMU, "Received unexpected message %s\n", ITTI_MSG_NAME(message_p));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(message_p), message_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while(!oai_exit);

  return NULL;
}
#endif




static void get_options (int argc, char **argv)
{
  int c;
  //  char                          line[1000];
  //  int                           l;
  int k,i;//,j,k;
#if defined(OAI_USRP) || defined(CPRIGW)
  int clock_src;
#endif
  int CC_id;



  const Enb_properties_array_t *enb_properties;

  enum long_option_e {
    LONG_OPTION_START = 0x100, /* Start after regular single char options */
    LONG_OPTION_RF_CONFIG_FILE,
    LONG_OPTION_ULSCH_MAX_CONSECUTIVE_ERRORS,
    LONG_OPTION_CALIB_UE_RX,
    LONG_OPTION_CALIB_UE_RX_MED,
    LONG_OPTION_CALIB_UE_RX_BYP,
    LONG_OPTION_DEBUG_UE_PRACH,
    LONG_OPTION_NO_L2_CONNECT,
    LONG_OPTION_CALIB_PRACH_TX,
    LONG_OPTION_RXGAIN,
	LONG_OPTION_RXGAINOFF,
    LONG_OPTION_TXGAIN,
    LONG_OPTION_SCANCARRIER,
    LONG_OPTION_MAXPOWER,
    LONG_OPTION_DUMP_FRAME,
    LONG_OPTION_LOOPMEMORY,
    LONG_OPTION_PHYTEST,
    LONG_OPTION_USIMTEST,
    LONG_OPTION_MMAPPED_DMA,
    LONG_OPTION_SINGLE_THREAD_DISABLE,
#if T_TRACER
    LONG_OPTION_T_PORT,
    LONG_OPTION_T_NOWAIT,
    LONG_OPTION_T_DONT_FORK,
#endif
  };

  static const struct option long_options[] = {
    {"rf-config-file",required_argument,  NULL, LONG_OPTION_RF_CONFIG_FILE},
    {"ulsch-max-errors",required_argument,  NULL, LONG_OPTION_ULSCH_MAX_CONSECUTIVE_ERRORS},
    {"calib-ue-rx",     required_argument,  NULL, LONG_OPTION_CALIB_UE_RX},
    {"calib-ue-rx-med", required_argument,  NULL, LONG_OPTION_CALIB_UE_RX_MED},
    {"calib-ue-rx-byp", required_argument,  NULL, LONG_OPTION_CALIB_UE_RX_BYP},
    {"debug-ue-prach",  no_argument,        NULL, LONG_OPTION_DEBUG_UE_PRACH},
    {"no-L2-connect",   no_argument,        NULL, LONG_OPTION_NO_L2_CONNECT},
    {"calib-prach-tx",   no_argument,        NULL, LONG_OPTION_CALIB_PRACH_TX},
    {"ue-rxgain",   required_argument,  NULL, LONG_OPTION_RXGAIN},
	{"ue-rxgain-off",   required_argument,  NULL, LONG_OPTION_RXGAINOFF},
    {"ue-txgain",   required_argument,  NULL, LONG_OPTION_TXGAIN},
    {"ue-scan-carrier",   no_argument,  NULL, LONG_OPTION_SCANCARRIER},
    {"ue-max-power",   required_argument,  NULL, LONG_OPTION_MAXPOWER},
    {"ue-dump-frame", no_argument, NULL, LONG_OPTION_DUMP_FRAME},
    {"loop-memory", required_argument, NULL, LONG_OPTION_LOOPMEMORY},
    {"phy-test", no_argument, NULL, LONG_OPTION_PHYTEST},
    {"usim-test", no_argument, NULL, LONG_OPTION_USIMTEST},
    {"mmapped-dma", no_argument, NULL, LONG_OPTION_MMAPPED_DMA},
    {"single-thread-disable", no_argument, NULL, LONG_OPTION_SINGLE_THREAD_DISABLE},
#if T_TRACER
    {"T_port",                 required_argument, 0, LONG_OPTION_T_PORT},
    {"T_nowait",               no_argument,       0, LONG_OPTION_T_NOWAIT},
    {"T_dont_fork",            no_argument,       0, LONG_OPTION_T_DONT_FORK},
#endif
    {NULL, 0, NULL, 0}
  };

  while ((c = getopt_long (argc, argv, "A:a:C:dEK:g:F:G:hqO:m:SUVRM:r:P:Ws:t:Tx:",long_options,NULL)) != -1) {
    switch (c) {
    case LONG_OPTION_RF_CONFIG_FILE:
      if ((strcmp("null", optarg) == 0) || (strcmp("NULL", optarg) == 0)) {
	printf("no configuration filename is provided\n");
      }
      else if (strlen(optarg)<=1024){
	strcpy(rf_config_file,optarg);
      }else {
	printf("Configuration filename is too long\n");
	exit(-1);   
      }
      break;
    case LONG_OPTION_MAXPOWER:
      tx_max_power[0]=atoi(optarg);
      for (CC_id=1;CC_id<MAX_NUM_CCs;CC_id++)
	tx_max_power[CC_id]=tx_max_power[0];
      break;
    case LONG_OPTION_ULSCH_MAX_CONSECUTIVE_ERRORS:
      ULSCH_max_consecutive_errors = atoi(optarg);
      printf("Set ULSCH_max_consecutive_errors = %d\n",ULSCH_max_consecutive_errors);
      break;

    case LONG_OPTION_CALIB_UE_RX:
      mode = rx_calib_ue;
      rx_input_level_dBm = atoi(optarg);
      printf("Running with UE calibration on (LNA max), input level %d dBm\n",rx_input_level_dBm);
      break;

    case LONG_OPTION_CALIB_UE_RX_MED:
      mode = rx_calib_ue_med;
      rx_input_level_dBm = atoi(optarg);
      printf("Running with UE calibration on (LNA med), input level %d dBm\n",rx_input_level_dBm);
      break;

    case LONG_OPTION_CALIB_UE_RX_BYP:
      mode = rx_calib_ue_byp;
      rx_input_level_dBm = atoi(optarg);
      printf("Running with UE calibration on (LNA byp), input level %d dBm\n",rx_input_level_dBm);
      break;

    case LONG_OPTION_DEBUG_UE_PRACH:
      mode = debug_prach;
      break;

    case LONG_OPTION_NO_L2_CONNECT:
      mode = no_L2_connect;
      break;

    case LONG_OPTION_CALIB_PRACH_TX:
      mode = calib_prach_tx;
      printf("Setting mode to calib_prach_tx (%d)\n",mode);
      break;

    case LONG_OPTION_RXGAIN:
      for (i=0; i<4; i++)
        rx_gain[0][i] = atof(optarg);

      break;

    case LONG_OPTION_RXGAINOFF:
      rx_gain_off = atof(optarg);
      break;

    case LONG_OPTION_TXGAIN:
      for (i=0; i<4; i++)
        tx_gain[0][i] = atof(optarg);

      break;

    case LONG_OPTION_SCANCARRIER:
      UE_scan_carrier=1;

      break;

    case LONG_OPTION_LOOPMEMORY:
      mode=loop_through_memory;
      input_fd = fopen(optarg,"r");
      AssertFatal(input_fd != NULL,"Please provide an input file\n");
      break;

    case LONG_OPTION_DUMP_FRAME:
      mode = rx_dump_frame;
      break;
      
    case LONG_OPTION_PHYTEST:
      phy_test = 1;
      break;

    case LONG_OPTION_USIMTEST:
        usim_test = 1;
      break;
    case LONG_OPTION_MMAPPED_DMA:
      mmapped_dma = 1;
      break;

    case LONG_OPTION_SINGLE_THREAD_DISABLE:
      single_thread_flag = 0;
      break;
              
#if T_TRACER
    case LONG_OPTION_T_PORT: {
      extern int T_port;
      if (optarg == NULL) abort();  /* should not happen */
      T_port = atoi(optarg);
      break;
    }

    case LONG_OPTION_T_NOWAIT: {
      extern int T_wait;
      T_wait = 0;
      break;
    }

    case LONG_OPTION_T_DONT_FORK: {
      extern int T_dont_fork;
      T_dont_fork = 1;
      break;
    }
#endif

    case 'A':
      timing_advance = atoi (optarg);
      break;

    case 'C':
      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        downlink_frequency[CC_id][0] = atof(optarg); // Use float to avoid issue with frequency over 2^31.
        downlink_frequency[CC_id][1] = downlink_frequency[CC_id][0];
        downlink_frequency[CC_id][2] = downlink_frequency[CC_id][0];
        downlink_frequency[CC_id][3] = downlink_frequency[CC_id][0];
        printf("Downlink for CC_id %d frequency set to %u\n", CC_id, downlink_frequency[CC_id][0]);
      }

      UE_scan=0;

      break;

    case 'a':
      chain_offset = atoi(optarg);
      break;

    case 'd':
#ifdef XFORMS
      do_forms=1;
      printf("Running with XFORMS!\n");
#endif
      break;
      
    case 'E':
      threequarter_fs=1;
      break;

    case 'K':
#if defined(ENABLE_ITTI)
      itti_dump_file = strdup(optarg);
#else
      printf("-K option is disabled when ENABLE_ITTI is not defined\n");
#endif
      break;

    case 'O':
      conf_config_file_name = optarg;
      break;

    case 'U':
      UE_flag = 1;
      break;

    case 'm':
      target_dl_mcs = atoi (optarg);
      break;

    case 't':
      target_ul_mcs = atoi (optarg);
      break;

    case 'W':
      opt_enabled=1;
      opt_type = OPT_WIRESHARK;
      strncpy(in_ip, "127.0.0.1", sizeof(in_ip));
      in_ip[sizeof(in_ip) - 1] = 0; // terminate string
      printf("Enabling OPT for wireshark for local interface");
      /*
	if (optarg == NULL){
	in_ip[0] =NULL;
	printf("Enabling OPT for wireshark for local interface");
	} else {
	strncpy(in_ip, optarg, sizeof(in_ip));
	in_ip[sizeof(in_ip) - 1] = 0; // terminate string
	printf("Enabling OPT for wireshark with %s \n",in_ip);
	}
      */
      break;

    case 'P':
      opt_type = OPT_PCAP;
      opt_enabled=1;

      if (optarg == NULL) {
        strncpy(in_path, "/tmp/oai_opt.pcap", sizeof(in_path));
        in_path[sizeof(in_path) - 1] = 0; // terminate string
        printf("Enabling OPT for PCAP with the following path /tmp/oai_opt.pcap");
      } else {
        strncpy(in_path, optarg, sizeof(in_path));
        in_path[sizeof(in_path) - 1] = 0; // terminate string
        printf("Enabling OPT for PCAP  with the following file %s \n",in_path);
      }

      break;

    case 'V':
      ouput_vcd = 1;
      break;

    case  'q':
      opp_enabled = 1;
      break;

    case  'R' :
      online_log_messages =1;
      break;

    case 'r':
      UE_scan = 0;

      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        switch(atoi(optarg)) {
        case 6:
          frame_parms[CC_id]->N_RB_DL=6;
          frame_parms[CC_id]->N_RB_UL=6;
          break;

        case 25:
          frame_parms[CC_id]->N_RB_DL=25;
          frame_parms[CC_id]->N_RB_UL=25;
          break;

        case 50:
          frame_parms[CC_id]->N_RB_DL=50;
          frame_parms[CC_id]->N_RB_UL=50;
          break;

        case 100:
          frame_parms[CC_id]->N_RB_DL=100;
          frame_parms[CC_id]->N_RB_UL=100;
          break;

        default:
          printf("Unknown N_RB_DL %d, switching to 25\n",atoi(optarg));
          break;
        }
      }

      break;

    case 's':
#if defined(OAI_USRP) || defined(CPRIGW)

      clock_src = atoi(optarg);

      if (clock_src == 0) {
        //  char ref[128] = "internal";
        //strncpy(uhd_ref, ref, strlen(ref)+1);
      } else if (clock_src == 1) {
        //char ref[128] = "external";
        //strncpy(uhd_ref, ref, strlen(ref)+1);
      }

#else
      printf("Note: -s not defined for ExpressMIMO2\n");
#endif
      break;

    case 'S':
      exit_missed_slots=0;
      printf("Skip exit for missed slots\n");
      break;

    case 'g':
      glog_level=atoi(optarg); // value between 1 - 9
      break;

    case 'F':
      break;

    case 'G':
      glog_verbosity=atoi(optarg);// value from 0, 0x5, 0x15, 0x35, 0x75
      break;

    case 'x':
      printf("Transmission mode should be set in config file now\n");
      exit(-1);
      /*
      transmission_mode = atoi(optarg);

      if (transmission_mode > 7) {
        printf("Transmission mode %d not supported for the moment\n",transmission_mode);
        exit(-1);
      }
      */
      break;

    case 'T':
      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) 
	frame_parms[CC_id]->frame_type = TDD;
      break;

    case 'h':
      help ();
      exit (-1);
       
    default:
      help ();
      exit (-1);
      break;
    }
  }

  if (UE_flag == 0)
    AssertFatal(conf_config_file_name != NULL,"Please provide a configuration file\n");

  if ((UE_flag == 0) && (conf_config_file_name != NULL)) {
    int i,j;

    NB_eNB_INST = 1;

    /* Read eNB configuration file */
    enb_properties = enb_config_init(conf_config_file_name);

    AssertFatal (NB_eNB_INST <= enb_properties->number,
                 "Number of eNB is greater than eNB defined in configuration file %s (%d/%d)!",
                 conf_config_file_name, NB_eNB_INST, enb_properties->number);

    /* Update some simulation parameters */
    for (i=0; i < enb_properties->number; i++) {
      AssertFatal (MAX_NUM_CCs == enb_properties->properties[i]->nb_cc,
                   "lte-softmodem compiled with MAX_NUM_CCs=%d, but only %d CCs configured for eNB %d!",
                   MAX_NUM_CCs, enb_properties->properties[i]->nb_cc, i);
      eth_params = (eth_params_t*)malloc(enb_properties->properties[i]->nb_rrh_gw * sizeof(eth_params_t));
      memset(eth_params, 0, enb_properties->properties[i]->nb_rrh_gw * sizeof(eth_params_t));

      for (j=0; j<enb_properties->properties[i]->nb_rrh_gw; j++) {
        	
        if (enb_properties->properties[i]->rrh_gw_config[j].active == 1 ) {
          local_remote_radio = BBU_REMOTE_RADIO_HEAD;
          (eth_params+j)->local_if_name             = enb_properties->properties[i]->rrh_gw_config[j].rrh_gw_if_name;
          (eth_params+j)->my_addr                   = enb_properties->properties[i]->rrh_gw_config[j].local_address;
          (eth_params+j)->my_port                   = enb_properties->properties[i]->rrh_gw_config[j].local_port;
          (eth_params+j)->remote_addr               = enb_properties->properties[i]->rrh_gw_config[j].remote_address;
          (eth_params+j)->remote_port               = enb_properties->properties[i]->rrh_gw_config[j].remote_port;
          
          if (enb_properties->properties[i]->rrh_gw_config[j].raw == 1) {
            (eth_params+j)->transp_preference       = ETH_RAW_MODE; 
          } else if (enb_properties->properties[i]->rrh_gw_config[j].rawif4p5 == 1) {
            (eth_params+j)->transp_preference       = ETH_RAW_IF4p5_MODE;             
          } else if (enb_properties->properties[i]->rrh_gw_config[j].udpif4p5 == 1) {
            (eth_params+j)->transp_preference       = ETH_UDP_IF4p5_MODE;             
          } else if (enb_properties->properties[i]->rrh_gw_config[j].rawif5_mobipass == 1) {
            (eth_params+j)->transp_preference       = ETH_RAW_IF5_MOBIPASS;             
          } else {
            (eth_params+j)->transp_preference       = ETH_UDP_MODE;	 
          }
          
          (eth_params+j)->iq_txshift                = enb_properties->properties[i]->rrh_gw_config[j].iq_txshift;
          (eth_params+j)->tx_sample_advance         = enb_properties->properties[i]->rrh_gw_config[j].tx_sample_advance;
          (eth_params+j)->tx_scheduling_advance     = enb_properties->properties[i]->rrh_gw_config[j].tx_scheduling_advance;
          if (enb_properties->properties[i]->rrh_gw_config[j].exmimo == 1) {
            (eth_params+j)->rf_preference          = EXMIMO_DEV;
          } else if (enb_properties->properties[i]->rrh_gw_config[j].usrp_b200 == 1) {
            (eth_params+j)->rf_preference          = USRP_B200_DEV;
          } else if (enb_properties->properties[i]->rrh_gw_config[j].usrp_x300 == 1) {
            (eth_params+j)->rf_preference          = USRP_X300_DEV;
          } else if (enb_properties->properties[i]->rrh_gw_config[j].bladerf == 1) {
            (eth_params+j)->rf_preference          = BLADERF_DEV;
          } else if (enb_properties->properties[i]->rrh_gw_config[j].lmssdr == 1) {
            //(eth_params+j)->rf_preference          = LMSSDR_DEV;
          } else {
            (eth_params+j)->rf_preference          = 0;
          } 
        } else {
          local_remote_radio = BBU_LOCAL_RADIO_HEAD; 
        }
	
      }

      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
	

        node_function[CC_id]  = enb_properties->properties[i]->cc_node_function[CC_id];
        node_timing[CC_id]    = enb_properties->properties[i]->cc_node_timing[CC_id];
        node_synch_ref[CC_id] = enb_properties->properties[i]->cc_node_synch_ref[CC_id];

        frame_parms[CC_id]->frame_type =       enb_properties->properties[i]->frame_type[CC_id];
        frame_parms[CC_id]->tdd_config =       enb_properties->properties[i]->tdd_config[CC_id];
        frame_parms[CC_id]->tdd_config_S =     enb_properties->properties[i]->tdd_config_s[CC_id];
        frame_parms[CC_id]->Ncp =              enb_properties->properties[i]->prefix_type[CC_id];

        //for (j=0; j < enb_properties->properties[i]->nb_cc; j++ ){
        frame_parms[CC_id]->Nid_cell            =  enb_properties->properties[i]->Nid_cell[CC_id];
        frame_parms[CC_id]->N_RB_DL             =  enb_properties->properties[i]->N_RB_DL[CC_id];
        frame_parms[CC_id]->N_RB_UL             =  enb_properties->properties[i]->N_RB_DL[CC_id];
        frame_parms[CC_id]->nb_antennas_tx      =  enb_properties->properties[i]->nb_antennas_tx[CC_id];
        frame_parms[CC_id]->nb_antennas_tx_eNB  =  enb_properties->properties[i]->nb_antenna_ports[CC_id];
        frame_parms[CC_id]->nb_antennas_rx      =  enb_properties->properties[i]->nb_antennas_rx[CC_id];

	frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.prach_ConfigIndex = enb_properties->properties[i]->prach_config_index[CC_id];
	frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.prach_FreqOffset  = enb_properties->properties[i]->prach_freq_offset[CC_id];

	frame_parms[CC_id]->mode1_flag         = (frame_parms[CC_id]->nb_antennas_tx_eNB == 1) ? 1 : 0;
	frame_parms[CC_id]->threequarter_fs    = threequarter_fs;

        //} // j
      }


      init_all_otg(0);
      g_otg->seed = 0;
      init_seeds(g_otg->seed);

      for (k=0; k<enb_properties->properties[i]->num_otg_elements; k++) {
        j=enb_properties->properties[i]->otg_ue_id[k]; // ue_id
        g_otg->application_idx[i][j] = 1;
        //g_otg->packet_gen_type=SUBSTRACT_STRING;
        g_otg->background[i][j][0] =enb_properties->properties[i]->otg_bg_traffic[k];
        g_otg->application_type[i][j][0] =enb_properties->properties[i]->otg_app_type[k];// BCBR; //MCBR, BCBR

        printf("[OTG] configuring traffic type %d for  eNB %d UE %d (Background traffic is %s)\n",
               g_otg->application_type[i][j][0], i, j,(g_otg->background[i][j][0]==1)?"Enabled":"Disabled");
      }

      init_predef_traffic(enb_properties->properties[i]->num_otg_elements, 1);


      glog_level                     = enb_properties->properties[i]->glog_level;
      glog_verbosity                 = enb_properties->properties[i]->glog_verbosity;
      hw_log_level                   = enb_properties->properties[i]->hw_log_level;
      hw_log_verbosity               = enb_properties->properties[i]->hw_log_verbosity ;
      phy_log_level                  = enb_properties->properties[i]->phy_log_level;
      phy_log_verbosity              = enb_properties->properties[i]->phy_log_verbosity;
      mac_log_level                  = enb_properties->properties[i]->mac_log_level;
      mac_log_verbosity              = enb_properties->properties[i]->mac_log_verbosity;
      rlc_log_level                  = enb_properties->properties[i]->rlc_log_level;
      rlc_log_verbosity              = enb_properties->properties[i]->rlc_log_verbosity;
      pdcp_log_level                 = enb_properties->properties[i]->pdcp_log_level;
      pdcp_log_verbosity             = enb_properties->properties[i]->pdcp_log_verbosity;
      rrc_log_level                  = enb_properties->properties[i]->rrc_log_level;
      rrc_log_verbosity              = enb_properties->properties[i]->rrc_log_verbosity;
# if defined(ENABLE_USE_MME)
      gtpu_log_level                 = enb_properties->properties[i]->gtpu_log_level;
      gtpu_log_verbosity             = enb_properties->properties[i]->gtpu_log_verbosity;
      udp_log_level                  = enb_properties->properties[i]->udp_log_level;
      udp_log_verbosity              = enb_properties->properties[i]->udp_log_verbosity;
#endif
#if defined (ENABLE_SECURITY)
      osa_log_level                  = enb_properties->properties[i]->osa_log_level;
      osa_log_verbosity              = enb_properties->properties[i]->osa_log_verbosity;
#endif

      // adjust the log
      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        for (k = 0 ; k < 4; k++) {
          downlink_frequency[CC_id][k]      =       enb_properties->properties[i]->downlink_frequency[CC_id];
          uplink_frequency_offset[CC_id][k] =  enb_properties->properties[i]->uplink_frequency_offset[CC_id];
          rx_gain[CC_id][k]                 =  (double)enb_properties->properties[i]->rx_gain[CC_id];
          tx_gain[CC_id][k]                 =  (double)enb_properties->properties[i]->tx_gain[CC_id];
        }

        printf("Downlink frequency/ uplink offset of CC_id %d set to %ju/%d\n", CC_id,
               enb_properties->properties[i]->downlink_frequency[CC_id],
               enb_properties->properties[i]->uplink_frequency_offset[CC_id]);
      } // CC_id
    }// i
  } else if (UE_flag == 1) {
    if (conf_config_file_name != NULL) {
      
      // Here the configuration file is the XER encoded UE capabilities
      // Read it in and store in asn1c data structures
      strcpy(uecap_xer,conf_config_file_name);
      uecap_xer_in=1;
    }
  }
}

#if T_TRACER
int T_wait = 1;       /* by default we wait for the tracer */
int T_port = 2021;    /* default port to listen to to wait for the tracer */
int T_dont_fork = 0;  /* default is to fork, see 'T_init' to understand */
#endif

void set_default_frame_parms(LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs]);
void set_default_frame_parms(LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs]) {

  int CC_id;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    frame_parms[CC_id] = (LTE_DL_FRAME_PARMS*) malloc(sizeof(LTE_DL_FRAME_PARMS));
    /* Set some default values that may be overwritten while reading options */
    frame_parms[CC_id]->frame_type          = FDD;
    frame_parms[CC_id]->tdd_config          = 3;
    frame_parms[CC_id]->tdd_config_S        = 0;
    frame_parms[CC_id]->N_RB_DL             = 100;
    frame_parms[CC_id]->N_RB_UL             = 100;
    frame_parms[CC_id]->Ncp                 = NORMAL;
    frame_parms[CC_id]->Ncp_UL              = NORMAL;
    frame_parms[CC_id]->Nid_cell            = 0;
    frame_parms[CC_id]->num_MBSFN_config    = 0;
    frame_parms[CC_id]->nb_antennas_tx_eNB  = 1;
    frame_parms[CC_id]->nb_antennas_tx      = 1;
    frame_parms[CC_id]->nb_antennas_rx      = 1;

    frame_parms[CC_id]->nushift             = 0;

    frame_parms[CC_id]->phich_config_common.phich_resource = oneSixth;
    frame_parms[CC_id]->phich_config_common.phich_duration = normal;
    // UL RS Config
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift = 0;//n_DMRS1 set to 0
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled = 0;
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = 0;
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH = 0;

    frame_parms[CC_id]->prach_config_common.rootSequenceIndex=22;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig=1;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.prach_ConfigIndex=0;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.highSpeedFlag=0;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.prach_FreqOffset=0;

    downlink_frequency[CC_id][0] = 2680000000; // Use float to avoid issue with frequency over 2^31.
    downlink_frequency[CC_id][1] = downlink_frequency[CC_id][0];
    downlink_frequency[CC_id][2] = downlink_frequency[CC_id][0];
    downlink_frequency[CC_id][3] = downlink_frequency[CC_id][0];
    //printf("Downlink for CC_id %d frequency set to %u\n", CC_id, downlink_frequency[CC_id][0]);

  }

}

void init_openair0(void);

void init_openair0() {

  int card;
  int i;

  for (card=0; card<MAX_CARDS; card++) {

    openair0_cfg[card].mmapped_dma=mmapped_dma;
    openair0_cfg[card].configFilename = NULL;

    if(frame_parms[0]->N_RB_DL == 100) {
      if (frame_parms[0]->threequarter_fs) {
	openair0_cfg[card].sample_rate=23.04e6;
	openair0_cfg[card].samples_per_frame = 230400; 
	openair0_cfg[card].tx_bw = 10e6;
	openair0_cfg[card].rx_bw = 10e6;
      }
      else {
	openair0_cfg[card].sample_rate=30.72e6;
	openair0_cfg[card].samples_per_frame = 307200; 
	openair0_cfg[card].tx_bw = 10e6;
	openair0_cfg[card].rx_bw = 10e6;
      }
    } else if(frame_parms[0]->N_RB_DL == 50) {
      openair0_cfg[card].sample_rate=15.36e6;
      openair0_cfg[card].samples_per_frame = 153600;
      openair0_cfg[card].tx_bw = 5e6;
      openair0_cfg[card].rx_bw = 5e6;
    } else if (frame_parms[0]->N_RB_DL == 25) {
      openair0_cfg[card].sample_rate=7.68e6;
      openair0_cfg[card].samples_per_frame = 76800;
      openair0_cfg[card].tx_bw = 2.5e6;
      openair0_cfg[card].rx_bw = 2.5e6;
    } else if (frame_parms[0]->N_RB_DL == 6) {
      openair0_cfg[card].sample_rate=1.92e6;
      openair0_cfg[card].samples_per_frame = 19200;
      openair0_cfg[card].tx_bw = 1.5e6;
      openair0_cfg[card].rx_bw = 1.5e6;
    }

    if (frame_parms[0]->frame_type==TDD)
      openair0_cfg[card].duplex_mode = duplex_mode_TDD;
    else //FDD
      openair0_cfg[card].duplex_mode = duplex_mode_FDD;

    
    if (local_remote_radio == BBU_REMOTE_RADIO_HEAD) {      
      openair0_cfg[card].remote_addr    = (eth_params+card)->remote_addr;
      openair0_cfg[card].remote_port    = (eth_params+card)->remote_port;
      openair0_cfg[card].my_addr        = (eth_params+card)->my_addr;
      openair0_cfg[card].my_port        = (eth_params+card)->my_port;    
    } 
    
    printf("HW: Configuring card %d, nb_antennas_tx/rx %d/%d\n",card,
           ((UE_flag==0) ? PHY_vars_eNB_g[0][0]->frame_parms.nb_antennas_tx : PHY_vars_UE_g[0][0]->frame_parms.nb_antennas_tx),
           ((UE_flag==0) ? PHY_vars_eNB_g[0][0]->frame_parms.nb_antennas_rx : PHY_vars_UE_g[0][0]->frame_parms.nb_antennas_rx));
    openair0_cfg[card].Mod_id = 0;

    if (UE_flag) {
      printf("ETHERNET: Configuring UE ETH for %s:%d\n",rrh_UE_ip,rrh_UE_port);
      openair0_cfg[card].remote_addr   = &rrh_UE_ip[0];
      openair0_cfg[card].remote_port = rrh_UE_port;
    } 

    openair0_cfg[card].num_rb_dl=frame_parms[0]->N_RB_DL;


    openair0_cfg[card].tx_num_channels=min(2,((UE_flag==0) ? PHY_vars_eNB_g[0][0]->frame_parms.nb_antennas_tx : PHY_vars_UE_g[0][0]->frame_parms.nb_antennas_tx));
    openair0_cfg[card].rx_num_channels=min(2,((UE_flag==0) ? PHY_vars_eNB_g[0][0]->frame_parms.nb_antennas_rx : PHY_vars_UE_g[0][0]->frame_parms.nb_antennas_rx));

    for (i=0; i<4; i++) {

      if (i<openair0_cfg[card].tx_num_channels)
	openair0_cfg[card].tx_freq[i] = (UE_flag==0) ? downlink_frequency[0][i] : downlink_frequency[0][i]+uplink_frequency_offset[0][i];
      else
	openair0_cfg[card].tx_freq[i]=0.0;

      if (i<openair0_cfg[card].rx_num_channels)
	openair0_cfg[card].rx_freq[i] = (UE_flag==0) ? downlink_frequency[0][i] + uplink_frequency_offset[0][i] : downlink_frequency[0][i];
      else
	openair0_cfg[card].rx_freq[i]=0.0;

      openair0_cfg[card].autocal[i] = 1;
      openair0_cfg[card].tx_gain[i] = tx_gain[0][i];
      if (UE_flag == 0) {
	openair0_cfg[card].rx_gain[i] = PHY_vars_eNB_g[0][0]->rx_total_gain_dB;
      }
      else {
	openair0_cfg[card].rx_gain[i] = PHY_vars_UE_g[0][0]->rx_total_gain_dB - rx_gain_off;
      }

      printf("Card %d, channel %d, Setting tx_gain %f, rx_gain %f, tx_freq %f, rx_freq %f\n",
             card,i, openair0_cfg[card].tx_gain[i],
             openair0_cfg[card].rx_gain[i],
             openair0_cfg[card].tx_freq[i],
             openair0_cfg[card].rx_freq[i]);
    }
  }
}

int main( int argc, char **argv )
{
  int i,aa;
#if defined (XFORMS)
  void *status;
#endif

  int CC_id;
  uint8_t  abstraction_flag=0;
  uint8_t beta_ACK=0,beta_RI=0,beta_CQI=2;

#if defined (XFORMS)
  int ret;
#endif

#ifdef DEBUG_CONSOLE
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
#endif

  PHY_VARS_UE *UE[MAX_NUM_CCs];

  mode = normal_txrx;
  memset(&openair0_cfg[0],0,sizeof(openair0_config_t)*MAX_CARDS);

  memset(tx_max_power,0,sizeof(int)*MAX_NUM_CCs);
  set_latency_target();

  // set default parameters
  set_default_frame_parms(frame_parms);

  // initialize logging
  logInit();

  // get options and fill parameters from configuration file
  get_options (argc, argv); //Command-line options, enb_properties


 

  
#if T_TRACER
  T_init(T_port, T_wait, T_dont_fork);
#endif

  // initialize the log (see log.h for details)
  set_glog(glog_level, glog_verbosity);

  //randominit (0);
  set_taus_seed (0);

  if (UE_flag==1) {
    printf("configuring for UE\n");

    set_comp_log(HW,      LOG_DEBUG,  LOG_HIGH, 1);
    set_comp_log(PHY,     LOG_DEBUG,   LOG_HIGH, 1);
    set_comp_log(MAC,     LOG_INFO,   LOG_HIGH, 1);
    set_comp_log(RLC,     LOG_INFO,   LOG_HIGH, 1);
    set_comp_log(PDCP,    LOG_INFO,   LOG_HIGH, 1);
    set_comp_log(OTG,     LOG_INFO,   LOG_HIGH, 1);
    set_comp_log(RRC,     LOG_INFO,   LOG_HIGH, 1);
#if defined(ENABLE_ITTI)
    set_comp_log(EMU,     LOG_INFO,   LOG_MED, 1);
# if defined(ENABLE_USE_MME)
    set_comp_log(NAS,     LOG_INFO,   LOG_HIGH, 1);
# endif
#endif
  } else {
    printf("configuring for eNB\n");

    set_comp_log(HW,      hw_log_level, hw_log_verbosity, 1);
    set_comp_log(PHY,     phy_log_level,   phy_log_verbosity, 1);
    if (opt_enabled == 1 )
      set_comp_log(OPT,   opt_log_level,      opt_log_verbosity, 1);
    set_comp_log(MAC,     mac_log_level,  mac_log_verbosity, 1);
    set_comp_log(RLC,     rlc_log_level,   rlc_log_verbosity, 1);
    set_comp_log(PDCP,    pdcp_log_level,  pdcp_log_verbosity, 1);
    set_comp_log(RRC,     rrc_log_level,  rrc_log_verbosity, 1);
#if defined(ENABLE_ITTI)
    set_comp_log(EMU,     LOG_INFO,   LOG_MED, 1);
# if defined(ENABLE_USE_MME)
    set_comp_log(UDP_,    udp_log_level,   udp_log_verbosity, 1);
    set_comp_log(GTPU,    gtpu_log_level,   gtpu_log_verbosity, 1);
    set_comp_log(S1AP,    LOG_DEBUG,   LOG_HIGH, 1);
    set_comp_log(SCTP,    LOG_INFO,   LOG_HIGH, 1);
# endif
#if defined(ENABLE_SECURITY)
    set_comp_log(OSA,    osa_log_level,   osa_log_verbosity, 1);
#endif
#endif
#ifdef LOCALIZATION
    set_comp_log(LOCALIZE, LOG_DEBUG, LOG_LOW, 1);
    set_component_filelog(LOCALIZE);
#endif
    set_comp_log(ENB_APP, LOG_INFO, LOG_HIGH, 1);
    set_comp_log(OTG,     LOG_INFO,   LOG_HIGH, 1);

    if (online_log_messages == 1) {
      set_component_filelog(RRC);
      set_component_filelog(PDCP);
    }
  }

  if (ouput_vcd) {
    if (UE_flag==1)
      VCD_SIGNAL_DUMPER_INIT("/tmp/openair_dump_UE.vcd");
    else
      VCD_SIGNAL_DUMPER_INIT("/tmp/openair_dump_eNB.vcd");
  }

  if (opp_enabled ==1){
    reset_opp_meas();
  }
  cpuf=get_cpu_freq_GHz();

#if defined(ENABLE_ITTI)

  if (UE_flag == 1) {
    log_set_instance_type (LOG_INSTANCE_UE);
  } else {
    log_set_instance_type (LOG_INSTANCE_ENB);
  }

  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info, messages_definition_xml, itti_dump_file);
 
  // initialize mscgen log after ITTI
  MSC_INIT(MSC_E_UTRAN, THREAD_MAX+TASK_MAX);
#endif
 
  if (opt_type != OPT_NONE) {
    radio_type_t radio_type;

    if (frame_parms[0]->frame_type == FDD)
      radio_type = RADIO_TYPE_FDD;
    else
      radio_type = RADIO_TYPE_TDD;

    if (init_opt(in_path, in_ip, NULL, radio_type) == -1)
      LOG_E(OPT,"failed to run OPT \n");
  }

#ifdef PDCP_USE_NETLINK
  netlink_init();
#if defined(PDCP_USE_NETLINK_QUEUES)
  pdcp_netlink_init();
#endif
#endif

#if !defined(ENABLE_ITTI)
  // to make a graceful exit when ctrl-c is pressed
  signal(SIGSEGV, signal_handler);
  signal(SIGINT, signal_handler);
#endif


  check_clock();

  // init the parameters
  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

    if (UE_flag==1) {
      frame_parms[CC_id]->nb_antennas_tx     = 1;
      frame_parms[CC_id]->nb_antennas_rx     = 1;
      frame_parms[CC_id]->nb_antennas_tx_eNB = 1; //initial value overwritten by initial sync later
    }

    init_ul_hopping(frame_parms[CC_id]);
    init_frame_parms(frame_parms[CC_id],1);
    //   phy_init_top(frame_parms[CC_id]);
    phy_init_lte_top(frame_parms[CC_id]);
  }


  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    //init prach for openair1 test

    // prach_fmt = get_prach_fmt(frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex, frame_parms->frame_type);
    // N_ZC = (prach_fmt <4)?839:139;
  }

  if (UE_flag==1) {
    NB_UE_INST=1;
    NB_INST=1;

    PHY_vars_UE_g = malloc(sizeof(PHY_VARS_UE**));
    PHY_vars_UE_g[0] = malloc(sizeof(PHY_VARS_UE*)*MAX_NUM_CCs);

    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

      PHY_vars_UE_g[0][CC_id] = init_lte_UE(frame_parms[CC_id], 0,abstraction_flag);
      UE[CC_id] = PHY_vars_UE_g[0][CC_id];
      printf("PHY_vars_UE_g[0][%d] = %p\n",CC_id,UE[CC_id]);

      if (phy_test==1)
	UE[CC_id]->mac_enabled = 0;
      else 
	UE[CC_id]->mac_enabled = 1;

      if (UE[CC_id]->mac_enabled == 0) {  //set default UL parameters for testing mode
	for (i=0; i<NUMBER_OF_CONNECTED_eNB_MAX; i++) {
	  UE[CC_id]->pusch_config_dedicated[i].betaOffset_ACK_Index = beta_ACK;
	  UE[CC_id]->pusch_config_dedicated[i].betaOffset_RI_Index  = beta_RI;
	  UE[CC_id]->pusch_config_dedicated[i].betaOffset_CQI_Index = beta_CQI;
	  
	  UE[CC_id]->scheduling_request_config[i].sr_PUCCH_ResourceIndex = 0;
	  UE[CC_id]->scheduling_request_config[i].sr_ConfigIndex = 7+(0%3);
	  UE[CC_id]->scheduling_request_config[i].dsr_TransMax = sr_n4;
	}
      }

      UE[CC_id]->UE_scan = UE_scan;
      UE[CC_id]->UE_scan_carrier = UE_scan_carrier;
      UE[CC_id]->mode    = mode;
      printf("UE[%d]->mode = %d\n",CC_id,mode);

      compute_prach_seq(&UE[CC_id]->frame_parms.prach_config_common,
                        UE[CC_id]->frame_parms.frame_type,
                        UE[CC_id]->X_u);

      if (UE[CC_id]->mac_enabled == 1) 
	UE[CC_id]->pdcch_vars[0]->crnti = 0x1234;
      else
	UE[CC_id]->pdcch_vars[0]->crnti = 0x1235;

      UE[CC_id]->rx_total_gain_dB =  (int)rx_gain[CC_id][0] + rx_gain_off;
      UE[CC_id]->tx_power_max_dBm = tx_max_power[CC_id];
      
      if (frame_parms[CC_id]->frame_type==FDD) {
	UE[CC_id]->N_TA_offset = 0;
      }
      else {
	if (frame_parms[CC_id]->N_RB_DL == 100)
	  UE[CC_id]->N_TA_offset = 624;
	else if (frame_parms[CC_id]->N_RB_DL == 50)
	  UE[CC_id]->N_TA_offset = 624/2;
	else if (frame_parms[CC_id]->N_RB_DL == 25)
	  UE[CC_id]->N_TA_offset = 624/4;
      }

    }

    //  printf("tx_max_power = %d -> amp %d\n",tx_max_power,get_tx_amp(tx_max_poHwer,tx_max_power));
  } else {
    //this is eNB
    PHY_vars_eNB_g = malloc(sizeof(PHY_VARS_eNB**));
    PHY_vars_eNB_g[0] = malloc(sizeof(PHY_VARS_eNB*));

    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      PHY_vars_eNB_g[0][CC_id] = init_lte_eNB(frame_parms[CC_id],0,frame_parms[CC_id]->Nid_cell,abstraction_flag);
      PHY_vars_eNB_g[0][CC_id]->CC_id = CC_id;

      PHY_vars_eNB_g[0][CC_id]->ue_dl_rb_alloc=0x1fff;
      PHY_vars_eNB_g[0][CC_id]->target_ue_dl_mcs=target_dl_mcs;
      PHY_vars_eNB_g[0][CC_id]->ue_ul_nb_rb=6;
      PHY_vars_eNB_g[0][CC_id]->target_ue_ul_mcs=target_ul_mcs;

      if (phy_test==1) PHY_vars_eNB_g[0][CC_id]->mac_enabled = 0;
      else PHY_vars_eNB_g[0][CC_id]->mac_enabled = 1;

      if (PHY_vars_eNB_g[0][CC_id]->mac_enabled == 0) { //set default parameters for testing mode
	for (i=0; i<NUMBER_OF_UE_MAX; i++) {
	  PHY_vars_eNB_g[0][CC_id]->pusch_config_dedicated[i].betaOffset_ACK_Index = beta_ACK;
	  PHY_vars_eNB_g[0][CC_id]->pusch_config_dedicated[i].betaOffset_RI_Index  = beta_RI;
	  PHY_vars_eNB_g[0][CC_id]->pusch_config_dedicated[i].betaOffset_CQI_Index = beta_CQI;
	  
	  PHY_vars_eNB_g[0][CC_id]->scheduling_request_config[i].sr_PUCCH_ResourceIndex = i;
	  PHY_vars_eNB_g[0][CC_id]->scheduling_request_config[i].sr_ConfigIndex = 7+(i%3);
	  PHY_vars_eNB_g[0][CC_id]->scheduling_request_config[i].dsr_TransMax = sr_n4;
	}
      }

      compute_prach_seq(&PHY_vars_eNB_g[0][CC_id]->frame_parms.prach_config_common,
                        PHY_vars_eNB_g[0][CC_id]->frame_parms.frame_type,
                        PHY_vars_eNB_g[0][CC_id]->X_u);

      PHY_vars_eNB_g[0][CC_id]->rx_total_gain_dB = (int)rx_gain[CC_id][0];

      if (frame_parms[CC_id]->frame_type==FDD) {
	PHY_vars_eNB_g[0][CC_id]->N_TA_offset = 0;
      }
      else {
	if (frame_parms[CC_id]->N_RB_DL == 100)
	  PHY_vars_eNB_g[0][CC_id]->N_TA_offset = 624;
	else if (frame_parms[CC_id]->N_RB_DL == 50)
	  PHY_vars_eNB_g[0][CC_id]->N_TA_offset = 624/2;
	else if (frame_parms[CC_id]->N_RB_DL == 25)
	  PHY_vars_eNB_g[0][CC_id]->N_TA_offset = 624/4;
      }
    }


    NB_eNB_INST=1;
    NB_INST=1;

  }

  fill_modeled_runtime_table(runtime_phy_rx,runtime_phy_tx);
  cpuf=get_cpu_freq_GHz();


  dump_frame_parms(frame_parms[0]);

  init_openair0();



#ifndef DEADLINE_SCHEDULER

  /* Currently we set affinity for UHD to CPU 0 for eNB/UE and only if number of CPUS >2 */
  
  cpu_set_t cpuset;
  int s;
  char cpu_affinity[1024];
  CPU_ZERO(&cpuset);
#ifdef CPU_AFFINITY
  if (get_nprocs() > 2)
    {
      CPU_SET(0, &cpuset);
      s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
      if (s != 0)
	{
	  perror( "pthread_setaffinity_np");
	  exit_fun("Error setting processor affinity");
	}
      LOG_I(HW, "Setting the affinity of main function to CPU 0, for device library to use CPU 0 only!\n");
    }
#endif

  /* Check the actual affinity mask assigned to the thread */
  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0)
    {
      perror( "pthread_getaffinity_np");
      exit_fun("Error getting processor affinity ");
    }
  memset(cpu_affinity, 0 , sizeof(cpu_affinity));
  for (int j = 0; j < CPU_SETSIZE; j++)
    {
      if (CPU_ISSET(j, &cpuset))
	{  
	  char temp[1024];
	  sprintf(temp, " CPU_%d ", j);    
	  strcat(cpu_affinity, temp);
	}
    }
  LOG_I(HW, "CPU Affinity of main() function is... %s\n", cpu_affinity);
#endif
  
  openair0_cfg[0].log_level = glog_level;

  


  int eMBMS_active=0;
  if (node_function[0] <= NGFI_RAU_IF4p5) { // don't initialize L2 for RRU
    LOG_I(PHY,"Intializing L2\n");
    mac_xface = malloc(sizeof(MAC_xface));  
    l2_init(frame_parms[0],eMBMS_active,(uecap_xer_in==1)?uecap_xer:NULL,
	    0,// cba_group_active
	    0); // HO flag
    mac_xface->macphy_exit = &exit_fun;
  }
  else if (node_function[0] == NGFI_RRU_IF4p5) { // Initialize PRACH in this case

  }



#if defined(ENABLE_ITTI)

  if ((UE_flag == 1)||
      (node_function[0]<NGFI_RAU_IF4p5))
    // don't create if node doesn't connect to RRC/S1/GTP
    if (create_tasks(UE_flag ? 0 : 1, UE_flag ? 1 : 0) < 0) {
      printf("cannot create ITTI tasks\n");
      exit(-1); // need a softer mode
    }

  printf("ITTI tasks created\n");
#endif

  if (phy_test==0) {
    if (UE_flag==1) {
      printf("Filling UE band info\n");
      fill_ue_band_info();
      mac_xface->dl_phy_sync_success (0, 0, 0, 1);
    } else if (node_function[0]>NGFI_RRU_IF4p5)
      mac_xface->mrbch_phy_sync_failure (0, 0, 0);
  }



  mlockall(MCL_CURRENT | MCL_FUTURE);

  pthread_cond_init(&sync_cond,NULL);
  pthread_mutex_init(&sync_mutex, NULL);

#ifdef XFORMS
  int UE_id;

  if (do_forms==1) {
    fl_initialize (&argc, argv, NULL, 0, 0);

    if (UE_flag==0) {
      form_stats_l2 = create_form_stats_form();
      fl_show_form (form_stats_l2->stats_form, FL_PLACE_HOTSPOT, FL_FULLBORDER, "l2 stats");
      form_stats = create_form_stats_form();
      fl_show_form (form_stats->stats_form, FL_PLACE_HOTSPOT, FL_FULLBORDER, "stats");

      for(UE_id=0; UE_id<scope_enb_num_ue; UE_id++) {
	for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
	  form_enb[CC_id][UE_id] = create_lte_phy_scope_enb();
	  sprintf (title, "LTE UL SCOPE eNB for CC_id %d, UE %d",CC_id,UE_id);
	  fl_show_form (form_enb[CC_id][UE_id]->lte_phy_scope_enb, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);

	  if (otg_enabled) {
	    fl_set_button(form_enb[CC_id][UE_id]->button_0,1);
	    fl_set_object_label(form_enb[CC_id][UE_id]->button_0,"DL Traffic ON");
	  } else {
	    fl_set_button(form_enb[CC_id][UE_id]->button_0,0);
	    fl_set_object_label(form_enb[CC_id][UE_id]->button_0,"DL Traffic OFF");
	  }
	} // CC_id
      } // UE_id
    } else {
      form_stats = create_form_stats_form();
      fl_show_form (form_stats->stats_form, FL_PLACE_HOTSPOT, FL_FULLBORDER, "stats");
      UE_id = 0;
      form_ue[UE_id] = create_lte_phy_scope_ue();
      sprintf (title, "LTE DL SCOPE UE");
      fl_show_form (form_ue[UE_id]->lte_phy_scope_ue, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);

      /*
	if (openair_daq_vars.use_ia_receiver) {
        fl_set_button(form_ue[UE_id]->button_0,1);
        fl_set_object_label(form_ue[UE_id]->button_0, "IA Receiver ON");
	} else {
        fl_set_button(form_ue[UE_id]->button_0,0);
        fl_set_object_label(form_ue[UE_id]->button_0, "IA Receiver OFF");
	}*/
      fl_set_button(form_ue[UE_id]->button_0,0);
      fl_set_object_label(form_ue[UE_id]->button_0, "IA Receiver OFF");
    }

    ret = pthread_create(&forms_thread, NULL, scope_thread, NULL);

    if (ret == 0)
      pthread_setname_np( forms_thread, "xforms" );

    printf("Scope thread created, ret=%d\n",ret);
  }

#endif

  rt_sleep_ns(10*100000000ULL);



  // start the main thread
  if (UE_flag == 1) {
    init_UE(1);
    number_of_cards = 1;
    
    for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      PHY_vars_UE_g[0][CC_id]->rf_map.card=0;
      PHY_vars_UE_g[0][CC_id]->rf_map.chain=CC_id+chain_offset;
    }
  }
  else { 
    init_eNB(node_function,node_timing,1,eth_params,single_thread_flag);
    // Sleep to allow all threads to setup
    
    number_of_cards = 1;
    
    for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      PHY_vars_eNB_g[0][CC_id]->rf_map.card=0;
      PHY_vars_eNB_g[0][CC_id]->rf_map.chain=CC_id+chain_offset;
    }
  }

  // connect the TX/RX buffers
  if (UE_flag==1) {

    for (CC_id=0;CC_id<MAX_NUM_CCs; CC_id++) {

    
#ifdef OAI_USRP
      UE[CC_id]->hw_timing_advance = timing_advance;
#else
      UE[CC_id]->hw_timing_advance = 160;
#endif
    }
    if (setup_ue_buffers(UE,&openair0_cfg[0])!=0) {
      printf("Error setting up eNB buffer\n");
      exit(-1);
    }



    if (input_fd) {
      printf("Reading in from file to antenna buffer %d\n",0);
      if (fread(UE[0]->common_vars.rxdata[0],
	        sizeof(int32_t),
	        frame_parms[0]->samples_per_tti*10,
	        input_fd) != frame_parms[0]->samples_per_tti*10)
        printf("error reading from file\n");
    }
    //p_exmimo_config->framing.tdd_config = TXRXSWITCH_TESTRX;
  } else {





    printf("Setting eNB buffer to all-RX\n");

    // Set LSBs for antenna switch (ExpressMIMO)
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      PHY_vars_eNB_g[0][CC_id]->hw_timing_advance = 0;
      for (i=0; i<frame_parms[CC_id]->samples_per_tti*10; i++)
        for (aa=0; aa<frame_parms[CC_id]->nb_antennas_tx; aa++)
          PHY_vars_eNB_g[0][CC_id]->common_vars.txdata[0][aa][i] = 0x00010001;
    }
  }
  sleep(3);


  printf("Sending sync to all threads\n");

  pthread_mutex_lock(&sync_mutex);
  sync_var=0;
  pthread_cond_broadcast(&sync_cond);
  pthread_mutex_unlock(&sync_mutex);

  // wait for end of program
  printf("TYPE <CTRL-C> TO TERMINATE\n");
  //getchar();

#if defined(ENABLE_ITTI)
  printf("Entering ITTI signals handler\n");
  itti_wait_tasks_end();
  oai_exit=1;
#else

  while (oai_exit==0)
    rt_sleep_ns(100000000ULL);

#endif

  // stop threads
#ifdef XFORMS
  printf("waiting for XFORMS thread\n");

  if (do_forms==1) {
    pthread_join(forms_thread,&status);
    fl_hide_form(form_stats->stats_form);
    fl_free_form(form_stats->stats_form);

    if (UE_flag==1) {
      fl_hide_form(form_ue[0]->lte_phy_scope_ue);
      fl_free_form(form_ue[0]->lte_phy_scope_ue);
    } else {
      fl_hide_form(form_stats_l2->stats_form);
      fl_free_form(form_stats_l2->stats_form);

      for(UE_id=0; UE_id<scope_enb_num_ue; UE_id++) {
	for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
	  fl_hide_form(form_enb[CC_id][UE_id]->lte_phy_scope_enb);
	  fl_free_form(form_enb[CC_id][UE_id]->lte_phy_scope_enb);
	}
      }
    }
  }

#endif

  printf("stopping MODEM threads\n");

  // cleanup
  if (UE_flag == 1) {
  } else {
    stop_eNB(1);
  }


  pthread_cond_destroy(&sync_cond);
  pthread_mutex_destroy(&sync_mutex);


  // *** Handle per CC_id openair0
  if (UE_flag==1) {
    if (PHY_vars_UE_g[0][0]->rfdevice.trx_end_func)
      PHY_vars_UE_g[0][0]->rfdevice.trx_end_func(&PHY_vars_UE_g[0][0]->rfdevice);
  }
  else {
    for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      if (PHY_vars_eNB_g[0][CC_id]->rfdevice.trx_end_func)
	PHY_vars_eNB_g[0][CC_id]->rfdevice.trx_end_func(&PHY_vars_eNB_g[0][CC_id]->rfdevice);  
      if (PHY_vars_eNB_g[0][CC_id]->ifdevice.trx_end_func)
	PHY_vars_eNB_g[0][CC_id]->ifdevice.trx_end_func(&PHY_vars_eNB_g[0][CC_id]->ifdevice);  
    }
  }
  if (ouput_vcd)
    VCD_SIGNAL_DUMPER_CLOSE();

  if (opt_enabled == 1)
    terminate_opt();

  logClean();

  return 0;
}
