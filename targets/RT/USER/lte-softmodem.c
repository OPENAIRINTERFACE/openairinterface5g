/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

   Contact Information
   OpenAirInterface Admin: openair_admin@eurecom.fr
   OpenAirInterface Tech : openair_tech@eurecom.fr
   OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file lte-softmodem.c
 * \brief main program to control HW and scheduling
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

#ifdef EMOS
#include <gps.h>
struct gps_fix_t dummy_gps_data;
#endif

#include "PHY/types.h"

#include "PHY/defs.h"
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#ifdef EXMIMO
#include "openair0_lib.h"
#else
#include "../../ARCH/COMMON/common_lib.h"
#endif

//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/vars.h"
#include "SCHED/vars.h"
#include "LAYER2/MAC/vars.h"

#include "../../SIMU/USER/init_lte.h"

#ifdef EMOS
#include "SCHED/phy_procedures_emos.h"
#endif

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
#endif
# endif
#endif

#ifdef XFORMS
#include "PHY/TOOLS/lte_phy_scope.h"
#include "stats.h"
#endif

#define FRAME_PERIOD    100000000ULL
#define DAQ_PERIOD      66667ULL

//#define DEBUG_THREADS 1

//#define USRP_DEBUG 1

struct timing_info_t {
  //unsigned int frame, hw_slot, last_slot, next_slot;
  RTIME time_min, time_max, time_avg, time_last, time_now;
  //unsigned int mbox0, mbox1, mbox2, mbox_target;
  unsigned int n_samples;
} timing_info;



openair0_config_t openair0_cfg[MAX_CARDS];

int32_t **rxdata;
int32_t **txdata;
int setup_ue_buffers(PHY_VARS_UE **phy_vars_ue, openair0_config_t *openair0_cfg, openair0_rf_map rf_map[MAX_NUM_CCs]);
int setup_eNB_buffers(PHY_VARS_eNB **phy_vars_eNB, openair0_config_t *openair0_cfg, openair0_rf_map rf_map[MAX_NUM_CCs]);

uint16_t runtime_phy_rx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]
uint16_t runtime_phy_tx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]
double cpuf;

void fill_ue_band_info(void);
#ifdef XFORMS
// current status is that every UE has a DL scope for a SINGLE eNB (eNB_id=0)
// at eNB 0, an UL scope for every UE
FD_lte_phy_scope_ue  *form_ue[NUMBER_OF_UE_MAX];
FD_lte_phy_scope_enb *form_enb[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
FD_stats_form                  *form_stats=NULL,*form_stats_l2=NULL;
char title[255];
unsigned char                   scope_enb_num_ue = 2;
#endif //XFORMS

#ifdef RTAI

static long                      main_eNB_thread;
static long                      main_ue_thread;

#else
pthread_t                       main_eNB_thread;
pthread_t                       main_ue_thread;

pthread_attr_t                  attr_dlsch_threads;
pthread_attr_t                  attr_UE_thread;

#ifndef DEADLINE_SCHEDULER
struct sched_param              sched_param_dlsch;
#endif
#endif

pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.




struct sched_param              sched_param_UE_thread;

pthread_attr_t                  attr_eNB_proc_tx[MAX_NUM_CCs][NUM_ENB_THREADS];
pthread_attr_t                  attr_eNB_proc_rx[MAX_NUM_CCs][NUM_ENB_THREADS];
#ifndef DEADLINE_SCHEDULER
struct sched_param              sched_param_eNB_proc_tx[MAX_NUM_CCs][NUM_ENB_THREADS];
struct sched_param              sched_param_eNB_proc_rx[MAX_NUM_CCs][NUM_ENB_THREADS];
#endif
#ifdef XFORMS
static pthread_t                forms_thread; //xforms
#endif
#ifdef EMOS
static pthread_t                thread3; //emos
static pthread_t                thread4; //GPS
#endif

openair0_device openair0;


/*
  static int instance_cnt=-1; //0 means worker is busy, -1 means its free
  int instance_cnt_ptr_kern,*instance_cnt_ptr_user;
  int pci_interface_ptr_kern;
*/
//extern unsigned int bigphys_top;
//extern unsigned int mem_base;

//int                             card = 0;


#if defined(ENABLE_ITTI)
static volatile int             start_eNB = 0;
static volatile int             start_UE = 0;
#endif
volatile int                    oai_exit = 0;

//static int                      time_offset[4] = {-138,-138,-138,-138};
//static int                      time_offset[4] = {-145,-145,-145,-145};
static int                      time_offset[4] = {0,0,0,0};


static char                     UE_flag=0;
//static uint8_t                  eNB_id=0,UE_id=0;

static char                     threequarter_fs=0;

uint32_t                 downlink_frequency[MAX_NUM_CCs][4];
int32_t                  uplink_frequency_offset[MAX_NUM_CCs][4];

openair0_rf_map rf_map[MAX_NUM_CCs];

static char                    *conf_config_file_name = NULL;
#if defined(ENABLE_ITTI)
static char                    *itti_dump_file = NULL;
#endif

int UE_scan = 1;
int UE_scan_carrier = 0;
runmode_t mode = normal_txrx;

FILE *input_fd=NULL;


#ifdef EXMIMO
#if MAX_NUM_CCs == 1
double tx_gain[MAX_NUM_CCs][4] = {{20,20,0,0}};
double rx_gain[MAX_NUM_CCs][4] = {{20,20,0,0}};
#else
double tx_gain[MAX_NUM_CCs][4] = {{20,20,0,0},{20,20,0,0}};
double rx_gain[MAX_NUM_CCs][4] = {{20,20,0,0},{20,20,0,0}};
#endif
// these are for EXMIMO2 target only
/*
  static unsigned int             rxg_max[4] =    {133,133,133,133};
  static unsigned int             rxg_med[4] =    {127,127,127,127};
  static unsigned int             rxg_byp[4] =    {120,120,120,120};
 */
// these are for EXMIMO2 card 39
unsigned int             rxg_max[4] =    {128,128,128,126};
unsigned int             rxg_med[4] =    {122,123,123,120};
unsigned int             rxg_byp[4] =    {116,117,116,116};
unsigned int             nf_max[4] =    {7,9,16,12};
unsigned int             nf_med[4] =    {12,13,22,17};
unsigned int             nf_byp[4] =    {15,20,29,23};
#if MAX_NUM_CCs == 1
rx_gain_t                rx_gain_mode[MAX_NUM_CCs][4] = {{max_gain,max_gain,max_gain,max_gain}};
#else
rx_gain_t                rx_gain_mode[MAX_NUM_CCs][4] = {{max_gain,max_gain,max_gain,max_gain},{max_gain,max_gain,max_gain,max_gain}};
#endif
#else
double tx_gain[MAX_NUM_CCs][4] = {{20,0,0,0}};
double rx_gain[MAX_NUM_CCs][4] = {{110,0,0,0}};
#endif

double sample_rate=30.72e6;
double bw = 10.0e6;

static int                      tx_max_power[MAX_NUM_CCs]; /* =  {0,0}*/;

char   rf_config_file[1024];

int chain_offset=0;
int phy_test = 0;

#ifndef EXMIMO
char ref[128] = "internal";
char channels[128] = "0";

#endif

int                      rx_input_level_dBm;
static int                      online_log_messages=0;
#ifdef XFORMS
extern int                      otg_enabled;
static char                     do_forms=0;
#else
int                             otg_enabled;
#endif
//int                             number_of_cards =   1;
#ifdef EXMIMO
static int                      mbox_bounds[20] =   {8,16,24,30,38,46,54,60,68,76,84,90,98,106,114,120,128,136,144, 0}; ///boundaries of slots in terms ob mbox counter rounded up to even numbers
//static int                      mbox_bounds[20] =   {6,14,22,28,36,44,52,58,66,74,82,88,96,104,112,118,126,134,142, 148}; ///boundaries of slots in terms ob mbox counter rounded up to even numbers
#endif

static LTE_DL_FRAME_PARMS      *frame_parms[MAX_NUM_CCs];

int multi_thread=1;
uint32_t target_dl_mcs = 28; //maximum allowed mcs
uint32_t target_ul_mcs = 20;
uint32_t timing_advance = 0;
uint8_t exit_missed_slots=1;
uint64_t num_missed_slots=0; // counter for the number of missed slots


time_stats_t softmodem_stats_mt; // main thread
time_stats_t softmodem_stats_hw; //  hw acquisition
time_stats_t softmodem_stats_tx_sf[10]; // total tx time
time_stats_t softmodem_stats_rx_sf[10]; // total rx time
void reset_opp_meas(void);
void print_opp_meas(void);
int transmission_mode=1;

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


#ifdef ETHERNET
char *rrh_UE_ip = "127.0.0.1";
int rrh_UE_port = 51000;
#endif

/* flag set by eNB conf file to specify if the radio head is local or remote (default option is local) */
uint8_t local_remote_radio = BBU_LOCAL_RADIO_HEAD;
/* struct for ethernet specific parameters given in eNB conf file */
eth_params_t *eth_params;

char uecap_xer[1024],uecap_xer_in=0;
extern void *UE_thread(void *arg);
extern void init_UE_threads(void);

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
  printf("  --ue-txgain set UE TX gain\n");
  printf("  --ue-scan_carrier set UE to scan around carrier\n");
  printf("  --loop-memory get softmodem (UE) to loop through memory instead of acquiring from HW\n");
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
  if (s != NULL) {
    printf("%s %s() Exiting OAI softmodem: %s\n",__FILE__, __FUNCTION__, s);
  }

  oai_exit = 1;

#if defined(ENABLE_ITTI)
  sleep(1); //allow lte-softmodem threads to exit first
  itti_terminate_tasks (TASK_UNKNOWN);
#endif

  //rt_sleep_ns(FRAME_PERIOD);

  //exit (-1);
}


#ifdef XFORMS

void reset_stats(FL_OBJECT *button, long arg)
{
  int i,j,k;
  PHY_VARS_eNB *phy_vars_eNB = PHY_vars_eNB_g[0][0];

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    for (k=0; k<8; k++) { //harq_processes
      for (j=0; j<phy_vars_eNB->dlsch_eNB[i][0]->Mlimit; j++) {
        phy_vars_eNB->eNB_UE_stats[i].dlsch_NAK[k][j]=0;
        phy_vars_eNB->eNB_UE_stats[i].dlsch_ACK[k][j]=0;
        phy_vars_eNB->eNB_UE_stats[i].dlsch_trials[k][j]=0;
      }

      phy_vars_eNB->eNB_UE_stats[i].dlsch_l2_errors[k]=0;
      phy_vars_eNB->eNB_UE_stats[i].ulsch_errors[k]=0;
      phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors=0;

      for (j=0; j<phy_vars_eNB->ulsch_eNB[i]->Mlimit; j++) {
        phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[k][j]=0;
        phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts_last[k][j]=0;
        phy_vars_eNB->eNB_UE_stats[i].ulsch_round_errors[k][j]=0;
        phy_vars_eNB->eNB_UE_stats[i].ulsch_round_fer[k][j]=0;
      }
    }

    phy_vars_eNB->eNB_UE_stats[i].dlsch_sliding_cnt=0;
    phy_vars_eNB->eNB_UE_stats[i].dlsch_NAK_round0=0;
    phy_vars_eNB->eNB_UE_stats[i].dlsch_mcs_offset=0;
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
      len = dump_ue_stats (PHY_vars_UE_g[0][0], stats_buffer, 0, mode,rx_input_level_dBm);
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
	  if ((PHY_vars_eNB_g[0][CC_id]->dlsch_eNB[UE_id][0]->rnti>0) && (ue_cnt<scope_enb_num_ue)) {
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

#ifdef EMOS
#define NO_ESTIMATES_DISK 100 //No. of estimates that are aquired before dumped to disk

void* gps_thread (void *arg)
{

  struct gps_data_t gps_data;
  struct gps_data_t *gps_data_ptr = &gps_data;
  struct sched_param sched_param;
  int ret;

  sched_param.sched_priority = sched_get_priority_min(SCHED_FIFO)+1;
  sched_setscheduler(0, SCHED_FIFO,&sched_param);

  printf("GPS thread has priority %d\n",sched_param.sched_priority);

  memset(&dummy_gps_data,0,sizeof(struct gps_fix_t));

#if GPSD_API_MAJOR_VERSION>=5
  ret = gps_open("127.0.0.1","2947",gps_data_ptr);

  if (ret!=0)
#else
  gps_data_ptr = gps_open("127.0.0.1","2947");

  if (gps_data_ptr == NULL)
#endif
  {
    printf("[EMOS] Could not open GPS\n");
    pthread_exit((void*)arg);
  }

#if GPSD_API_MAJOR_VERSION>=4
  else if (gps_stream(gps_data_ptr, WATCH_ENABLE,NULL) != 0)
#else
  else if (gps_query(gps_data_ptr, "w+x") != 0)
#endif
  {
    printf("[EMOS] Error sending command to GPS\n");
    pthread_exit((void*) arg);
  } else
    printf("[EMOS] Opened GPS, gps_data=%p\n", gps_data_ptr);


  while (!oai_exit) {
    printf("[EMOS] polling data from gps\n");
#if GPSD_API_MAJOR_VERSION>=5

    if (gps_waiting(gps_data_ptr,500)) {
      if (gps_read(gps_data_ptr) <= 0) {
#else

    if (gps_waiting(gps_data_ptr)) {
      if (gps_poll(gps_data_ptr) != 0) {
#endif
        printf("[EMOS] problem polling data from gps\n");
      } else {
        memcpy(&dummy_gps_data,&(gps_data_ptr->fix),sizeof(struct gps_fix_t));
        printf("[EMOS] lat %g, lon %g\n",gps_data_ptr->fix.latitude,gps_data_ptr->fix.longitude);
      }
    } //gps_waiting
    else {
      printf("[EMOS] WARNING: No GPS data available, storing dummy packet\n");
    }

    //rt_sleep_ns(1000000000LL);
    sleep(1);
  } //oai_exit

  pthread_exit((void*) arg);

}

void *emos_thread (void *arg)
{
  char c;
  char *fifo2file_buffer, *fifo2file_ptr;

  int fifo, counter=0, bytes;

  FILE  *dumpfile_id;
  char  dumpfile_name[1024];
  time_t starttime_tmp;
  struct tm starttime;

  int channel_buffer_size,ret;

  time_t timer;
  struct tm *now;

  struct sched_param sched_param;

  sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO)-1;
  sched_setscheduler(0, SCHED_FIFO,&sched_param);

  printf("EMOS thread has priority %d\n",sched_param.sched_priority);

  timer = time(NULL);
  now = localtime(&timer);

  if (UE_flag==0)
    channel_buffer_size = sizeof(fifo_dump_emos_eNB);
  else
    channel_buffer_size = sizeof(fifo_dump_emos_UE);

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


  printf("[EMOS] starting dump, channel_buffer_size=%d, fifo %d\n",channel_buffer_size,fifo);

  while (!oai_exit) {
    /*
    bytes = rtf_read_timed(fifo, fifo2file_ptr, channel_buffer_size,100);
    if (bytes==0)
    continue;
    */
    bytes = rtf_read_all_at_once(fifo, fifo2file_ptr, channel_buffer_size);

    if (bytes<=0) {
      usleep(100);
      continue;
    }

    if (bytes != channel_buffer_size) {
      printf("[EMOS] ERROR! Only got %d bytes instead of %d!\n",bytes,channel_buffer_size);
    }

    /*
    if (UE_flag==0)
    printf("eNB: count %d, frame %d, read: %d bytes from the fifo\n",counter, ((fifo_dump_emos_eNB*)fifo2file_ptr)->frame_tx,bytes);
    else
    printf("UE: count %d, frame %d, read: %d bytes from the fifo\n",counter, ((fifo_dump_emos_UE*)fifo2file_ptr)->frame_rx,bytes);
    */

    fifo2file_ptr += channel_buffer_size;
    counter ++;

    if (counter == NO_ESTIMATES_DISK) {
      //reset stuff
      fifo2file_ptr = fifo2file_buffer;
      counter = 0;

      //flush buffer to disk
      if (UE_flag==0)
        printf("[EMOS] eNB: count %d, frame %d, flushing buffer to disk\n",
               counter, ((fifo_dump_emos_eNB*)fifo2file_ptr)->frame_tx);
      else
        printf("[EMOS] UE: count %d, frame %d, flushing buffer to disk\n",
               counter, ((fifo_dump_emos_UE*)fifo2file_ptr)->frame_rx);


      if (fwrite(fifo2file_buffer, sizeof(char), NO_ESTIMATES_DISK*channel_buffer_size, dumpfile_id) != NO_ESTIMATES_DISK*channel_buffer_size) {
        fprintf(stderr, "[EMOS] Error writing to dumpfile\n");
        exit(EXIT_FAILURE);
      }

      if (fwrite(&dummy_gps_data, sizeof(char), sizeof(struct gps_fix_t), dumpfile_id) != sizeof(struct gps_fix_t)) {
        printf("[EMOS] Error writing to dumpfile, stopping recording\n");
        exit(EXIT_FAILURE);
      }
    }
  }

  free(fifo2file_buffer);
  fclose(dumpfile_id);
  close(fifo);

  pthread_exit((void*) arg);

}
#endif



#if defined(ENABLE_ITTI)
static void wait_system_ready (char *message, volatile int *start_flag)
{
  /* Wait for eNB application initialization to be complete (eNB registration to MME) */
  {
    static char *indicator[] = {".    ", "..   ", "...  ", ".... ", ".....",
                                " ....", "  ...", "   ..", "    .", "     "
                               };
    int i = 0;

    while ((!oai_exit) && (*start_flag == 0)) {
      LOG_N(EMU, message, indicator[i]);
      fflush(stdout);
      i = (i + 1) % (sizeof(indicator) / sizeof(indicator[0]));
      usleep(200000);
    }

    LOG_D(EMU,"\n");
  }
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


void do_OFDM_mod_rt(int subframe,PHY_VARS_eNB *phy_vars_eNB)
{

  unsigned int aa,slot_offset, slot_offset_F;
  int dummy_tx_b[7680*4] __attribute__((aligned(32)));
  int i, tx_offset;
  int slot_sizeF = (phy_vars_eNB->lte_frame_parms.ofdm_symbol_size)*
                   ((phy_vars_eNB->lte_frame_parms.Ncp==1) ? 6 : 7);
  int len;

  slot_offset_F = (subframe<<1)*slot_sizeF;

  slot_offset = subframe*phy_vars_eNB->lte_frame_parms.samples_per_tti;

  if ((subframe_select(&phy_vars_eNB->lte_frame_parms,subframe)==SF_DL)||
      ((subframe_select(&phy_vars_eNB->lte_frame_parms,subframe)==SF_S))) {
    //    LOG_D(HW,"Frame %d: Generating slot %d\n",frame,next_slot);


    for (aa=0; aa<phy_vars_eNB->lte_frame_parms.nb_antennas_tx; aa++) {
      if (phy_vars_eNB->lte_frame_parms.Ncp == EXTENDED) {
        PHY_ofdm_mod(&phy_vars_eNB->lte_eNB_common_vars.txdataF[0][aa][slot_offset_F],
                     dummy_tx_b,
                     phy_vars_eNB->lte_frame_parms.ofdm_symbol_size,
                     6,
                     phy_vars_eNB->lte_frame_parms.nb_prefix_samples,
                     CYCLIC_PREFIX);
        PHY_ofdm_mod(&phy_vars_eNB->lte_eNB_common_vars.txdataF[0][aa][slot_offset_F+slot_sizeF],
                     dummy_tx_b+(phy_vars_eNB->lte_frame_parms.samples_per_tti>>1),
                     phy_vars_eNB->lte_frame_parms.ofdm_symbol_size,
                     6,
                     phy_vars_eNB->lte_frame_parms.nb_prefix_samples,
                     CYCLIC_PREFIX);
      } else {
        normal_prefix_mod(&phy_vars_eNB->lte_eNB_common_vars.txdataF[0][aa][slot_offset_F],
                          dummy_tx_b,
                          7,
                          &(phy_vars_eNB->lte_frame_parms));
	// if S-subframe generate first slot only
	if (subframe_select(&phy_vars_eNB->lte_frame_parms,subframe) == SF_DL)
	  normal_prefix_mod(&phy_vars_eNB->lte_eNB_common_vars.txdataF[0][aa][slot_offset_F+slot_sizeF],
			    dummy_tx_b+(phy_vars_eNB->lte_frame_parms.samples_per_tti>>1),
			    7,
			    &(phy_vars_eNB->lte_frame_parms));
      }

      // if S-subframe generate first slot only
      if (subframe_select(&phy_vars_eNB->lte_frame_parms,subframe) == SF_S)
	len = phy_vars_eNB->lte_frame_parms.samples_per_tti>>1;
      else
	len = phy_vars_eNB->lte_frame_parms.samples_per_tti;
      /*
      for (i=0;i<len;i+=4) {
	dummy_tx_b[i] = 0x100;
	dummy_tx_b[i+1] = 0x01000000;
	dummy_tx_b[i+2] = 0xff00;
	dummy_tx_b[i+3] = 0xff000000;
	}*/
      for (i=0; i<len; i++) {
        tx_offset = (int)slot_offset+time_offset[aa]+i;

	
        if (tx_offset<0)
          tx_offset += LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->lte_frame_parms.samples_per_tti;

        if (tx_offset>=(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->lte_frame_parms.samples_per_tti))
          tx_offset -= LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->lte_frame_parms.samples_per_tti;

	((short*)&phy_vars_eNB->lte_eNB_common_vars.txdata[0][aa][tx_offset])[0] = ((short*)dummy_tx_b)[2*i]<<openair0_cfg[0].iq_txshift;
	
	((short*)&phy_vars_eNB->lte_eNB_common_vars.txdata[0][aa][tx_offset])[1] = ((short*)dummy_tx_b)[2*i+1]<<openair0_cfg[0].iq_txshift;
     }
     // if S-subframe switch to RX in second subframe
     if (subframe_select(&phy_vars_eNB->lte_frame_parms,subframe) == SF_S) {
       for (i=0; i<len; i++) {
	 phy_vars_eNB->lte_eNB_common_vars.txdata[0][aa][tx_offset++] = 0x00010001;
       }
     }

     if ((((phy_vars_eNB->lte_frame_parms.tdd_config==0) ||
	  (phy_vars_eNB->lte_frame_parms.tdd_config==1) ||
	  (phy_vars_eNB->lte_frame_parms.tdd_config==2) ||
	  (phy_vars_eNB->lte_frame_parms.tdd_config==6)) && 
	  (subframe==0)) || (subframe==5)) {
       // turn on tx switch N_TA_offset before
       //LOG_D(HW,"subframe %d, time to switch to tx (N_TA_offset %d, slot_offset %d) \n",subframe,phy_vars_eNB->N_TA_offset,slot_offset);
       for (i=0; i<phy_vars_eNB->N_TA_offset; i++) {
	 tx_offset = (int)slot_offset+time_offset[aa]+i-phy_vars_eNB->N_TA_offset/2;
	 if (tx_offset<0)
	   tx_offset += LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->lte_frame_parms.samples_per_tti;
	 
	 if (tx_offset>=(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->lte_frame_parms.samples_per_tti))
	   tx_offset -= LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->lte_frame_parms.samples_per_tti;
	 
	 phy_vars_eNB->lte_eNB_common_vars.txdata[0][aa][tx_offset] = 0x00000000;
       }
     }
    }
  }
}

/* mutex, cond and variable to serialize phy proc TX calls
 * (this mechanism may be relaxed in the future for better
 * performances)
 */
static struct {
  pthread_mutex_t  mutex_phy_proc_tx;
  pthread_cond_t   cond_phy_proc_tx;
  volatile uint8_t phy_proc_CC_id;
} sync_phy_proc[NUM_ENB_THREADS];

/*!
 * \brief The transmit thread of eNB.
 * \ref NUM_ENB_THREADS threads of this type are active at the same time.
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* eNB_thread_tx( void* param )
{
return 0;
  static int eNB_thread_tx_status[NUM_ENB_THREADS];

  eNB_proc_t *proc = (eNB_proc_t*)param;
  FILE  *tx_time_file = NULL;
  char tx_time_name[101];

  if (opp_enabled == 1) {
    snprintf(tx_time_name, 100,"/tmp/%s_tx_time_thread_sf_%d", "eNB", proc->subframe);
    tx_time_file = fopen(tx_time_name,"w");
  }
  // set default return value
  eNB_thread_tx_status[proc->subframe] = 0;

  MSC_START_USE();

#ifdef RTAI
  RT_TASK *task;
  char task_name[8];

  sprintf(task_name,"TXC%dS%d",proc->CC_id,proc->subframe);
  task = rt_task_init_schmod(nam2num(task_name), 0, 0, 0, SCHED_FIFO, 0xF);

  if (task == NULL) {
    LOG_E(PHY,"[SCHED][eNB] Problem starting eNB_proc_TX thread_index %d (%s)!!!!\n",proc->subframe,task_name);
    return 0;
  } else {
    LOG_I(PHY,"[SCHED][eNB] eNB TX thread CC %d SF %d started with id %p\n",
          proc->CC_id,
          proc->subframe,
          task);
  }

#else
#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;
  unsigned int flags = 0;
  uint64_t runtime  = 850000 ;  
  uint64_t deadline = 1   *  1000000 ; // each tx thread will finish within 1ms
  uint64_t period   = 1   * 10000000; // each tx thread has a period of 10ms from the starting point

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  = runtime;
  attr.sched_deadline = deadline;
  attr.sched_period   = period; 

  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] eNB tx thread: sched_setattr failed\n");
    return &eNB_thread_tx_status[proc->subframe];
  }

  LOG_I( HW, "[SCHED] eNB TX deadline thread %d(TID %ld) started on CPU %d\n", proc->subframe, gettid(), sched_getcpu() );
#else
  int policy, s, j;
  struct sched_param sparam;
  char cpu_affinity[1024];
  cpu_set_t cpuset;

  /* Set affinity mask to include CPUs 1 to MAX_CPUS */
  /* CPU 0 is reserved for UHD threads */
  /* CPU 1 is reserved for all TX threads */
  /* Enable CPU Affinity only if number of CPUs >2 */
  CPU_ZERO(&cpuset);

  #ifdef CPU_AFFINITY
  if (get_nprocs() > 2)
  {
    for (j = 1; j < get_nprocs(); j++)
        CPU_SET(j, &cpuset);
    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
      perror( "pthread_setaffinity_np");
      exit_fun("Error setting processor affinity");
    }
  }
  #endif

  /* Check the actual affinity mask assigned to the thread */

  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0)
  {
    perror( "pthread_getaffinity_np");
    exit_fun("Error getting processor affinity ");
  }
  memset(cpu_affinity,0,sizeof(cpu_affinity));
  for (j = 0; j < CPU_SETSIZE; j++)
     if (CPU_ISSET(j, &cpuset))
     {  
        char temp[1024];
        sprintf (temp, " CPU_%d", j);
        strcat(cpu_affinity, temp);
     }

  memset(&sparam, 0 , sizeof (sparam));
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO)-1;
  policy = SCHED_FIFO ; 
  
  s = pthread_setschedparam(pthread_self(), policy, &sparam);
  if (s != 0)
     {
     perror("pthread_setschedparam : ");
     exit_fun("Error setting thread priority");
     }
  s = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (s != 0)
   {
     perror("pthread_getschedparam : ");
     exit_fun("Error getting thread priority");

   }

 LOG_I( HW, "[SCHED][eNB] TX thread %d started on CPU %d TID %ld, sched_policy = %s , priority = %d, CPU Affinity=%s \n", proc->subframe, sched_getcpu(),gettid(),
                   (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                   (policy == SCHED_RR)    ? "SCHED_RR" :
                   (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                   "???",
                   sparam.sched_priority, cpu_affinity );


#endif

#endif

  mlockall(MCL_CURRENT | MCL_FUTURE);

#ifdef HARD_RT
  rt_make_hard_real_time();
#endif

  while (!oai_exit) {

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_TX0+(2*proc->subframe), 0 );

    if (pthread_mutex_lock(&proc->mutex_tx) != 0) {
      LOG_E( PHY, "[SCHED][eNB] error locking mutex for eNB TX proc %d\n", proc->subframe );
      exit_fun("nothing to add");
      break;
    }

    while (proc->instance_cnt_tx < 0) {
      // most of the time the thread is waiting here
      // proc->instance_cnt_tx is -1
      pthread_cond_wait( &proc->cond_tx, &proc->mutex_tx ); // this unlocks mutex_tx while waiting and then locks it again
    }

    if (pthread_mutex_unlock(&proc->mutex_tx) != 0) {
      LOG_E(PHY,"[SCHED][eNB] error unlocking mutex for eNB TX proc %d\n",proc->subframe);
      exit_fun("nothing to add");
      break;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_TX0+(2*proc->subframe), 1 );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX_ENB, proc->frame_tx );
    start_meas( &softmodem_stats_tx_sf[proc->subframe] );

    if (oai_exit) break;

    if (((PHY_vars_eNB_g[0][proc->CC_id]->lte_frame_parms.frame_type == TDD) &&
         ((subframe_select(&PHY_vars_eNB_g[0][proc->CC_id]->lte_frame_parms,proc->subframe_tx) == SF_DL) ||
          (subframe_select(&PHY_vars_eNB_g[0][proc->CC_id]->lte_frame_parms,proc->subframe_tx) == SF_S))) ||
        (PHY_vars_eNB_g[0][proc->CC_id]->lte_frame_parms.frame_type == FDD)) {
      /* run PHY TX procedures the one after the other for all CCs to avoid race conditions
       * (may be relaxed in the future for performance reasons)
       */
      if (pthread_mutex_lock(&sync_phy_proc[proc->subframe].mutex_phy_proc_tx) != 0) {
        LOG_E(PHY, "[SCHED][eNB] error locking PHY proc mutex for eNB TX proc %d\n", proc->subframe);
        exit_fun("nothing to add");
        break;
      }
      /* wait for our turn or oai_exit */
      while (sync_phy_proc[proc->subframe].phy_proc_CC_id != proc->CC_id && !oai_exit) {
        pthread_cond_wait(&sync_phy_proc[proc->subframe].cond_phy_proc_tx,
                          &sync_phy_proc[proc->subframe].mutex_phy_proc_tx);
      }

      if (pthread_mutex_unlock(&sync_phy_proc[proc->subframe].mutex_phy_proc_tx) != 0) {
        LOG_E(PHY, "[SCHED][eNB] error unlocking PHY proc mutex for eNB TX proc %d\n", proc->subframe);
        exit_fun("nothing to add");
      }

      if (oai_exit)
        break;

      phy_procedures_eNB_TX( proc->subframe, PHY_vars_eNB_g[0][proc->CC_id], 0, no_relay, NULL );

      /* we're done, let the next one proceed */
      if (pthread_mutex_lock(&sync_phy_proc[proc->subframe].mutex_phy_proc_tx) != 0) {
        LOG_E(PHY, "[SCHED][eNB] error locking PHY proc mutex for eNB TX proc %d\n", proc->subframe);
        exit_fun("nothing to add");
        break;
      }
      sync_phy_proc[proc->subframe].phy_proc_CC_id++;
      sync_phy_proc[proc->subframe].phy_proc_CC_id %= MAX_NUM_CCs;
      pthread_cond_broadcast(&sync_phy_proc[proc->subframe].cond_phy_proc_tx);
      if (pthread_mutex_unlock(&sync_phy_proc[proc->subframe].mutex_phy_proc_tx) != 0) {
        LOG_E(PHY, "[SCHED][eNB] error unlocking PHY proc mutex for eNB TX proc %d\n", proc->subframe);
        exit_fun("nothing to add");
        break;
      }
    }

    do_OFDM_mod_rt( proc->subframe_tx, PHY_vars_eNB_g[0][proc->CC_id] );
    /*
    short *txdata = (short*)&PHY_vars_eNB_g[0][proc->CC_id]->lte_eNB_common_vars.txdata[0][0][proc->subframe_tx*PHY_vars_eNB_g[0][proc->CC_id]->lte_frame_parms.samples_per_tti];
    int i;
    for (i=0;i<PHY_vars_eNB_g[0][proc->CC_id]->lte_frame_parms.samples_per_tti*2;i+=8) {
      txdata[i] = 2047;
      txdata[i+1] = 0;
      txdata[i+2] = 0;
      txdata[i+3] = 2047;
      txdata[i+4] = -2047;
      txdata[i+5] = 0;
      txdata[i+6] = 0;
      txdata[i+7] = -2047;
    }
    */
    if (pthread_mutex_lock(&proc->mutex_tx) != 0) {
      LOG_E( PHY, "[SCHED][eNB] error locking mutex for eNB TX proc %d\n", proc->subframe );
      exit_fun("nothing to add");
      break;
    }

    proc->instance_cnt_tx--;

    if (pthread_mutex_unlock(&proc->mutex_tx) != 0) {
      LOG_E( PHY, "[SCHED][eNB] error unlocking mutex for eNB TX proc %d\n", proc->subframe );
      exit_fun("nothing to add");
      break;
    }

    proc->frame_tx++;

    if (proc->frame_tx==1024)
      proc->frame_tx=0;
    stop_meas( &softmodem_stats_tx_sf[proc->subframe] );
#ifdef DEADLINE_SCHEDULER
    if (opp_enabled){
      if(softmodem_stats_tx_sf[proc->subframe].diff_now/(cpuf) > attr.sched_runtime){
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RUNTIME_TX_ENB, (softmodem_stats_tx_sf[proc->subframe].diff_now/cpuf - attr.sched_runtime)/1000000.0);
      }
    }
#endif 
    print_meas_now(&softmodem_stats_tx_sf[proc->subframe],"eNB_TX_SF",proc->subframe, tx_time_file);

  }



  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_TX0+(2*proc->subframe), 0 );

#ifdef HARD_RT
  rt_make_soft_real_time();
#endif

#ifdef DEBUG_THREADS
  printf( "Exiting eNB thread TX %d\n", proc->subframe );
#endif
  // clean task
#ifdef RTAI
  rt_task_delete(task);
#endif

  eNB_thread_tx_status[proc->subframe] = 0;
  return &eNB_thread_tx_status[proc->subframe];
}


/*!
 * \brief The receive thread of eNB.
 * \ref NUM_ENB_THREADS threads of this type are active at the same time.
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* eNB_thread_rx( void* param )
{
return 0;
  static int eNB_thread_rx_status[NUM_ENB_THREADS];

  eNB_proc_t *proc = (eNB_proc_t*)param;

  FILE  *rx_time_file = NULL;
  char rx_time_name[101];
  //int i;

  if (opp_enabled == 1){
    snprintf(rx_time_name, 100,"/tmp/%s_rx_time_thread_sf_%d", "eNB", proc->subframe);
    rx_time_file = fopen(rx_time_name,"w");
  }
  // set default return value
  eNB_thread_rx_status[proc->subframe] = 0;

  MSC_START_USE();

#ifdef RTAI
  RT_TASK *task;
  char task_name[8];

  sprintf(task_name,"RXC%1dS%1d",proc->CC_id,proc->subframe);
  task = rt_task_init_schmod(nam2num(task_name), 0, 0, 0, SCHED_FIFO, 0xF);

  if (task==NULL) {
    LOG_E(PHY,"[SCHED][eNB] Problem starting eNB_proc_RX thread_index %d (%s)!!!!\n",proc->subframe,task_name);
    return 0;
  } else {
    LOG_I(PHY,"[SCHED][eNB] eNB RX thread CC_id %d SF %d started with id %p\n", /*  on CPU %d*/
          proc->CC_id,
          proc->subframe,
          task); /*,rtai_cpuid()*/
  }

#else
#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;
  unsigned int flags = 0;
  uint64_t runtime  = 870000 ;
  uint64_t deadline = 1   *  1000000;
  uint64_t period   = 1   * 10000000; // each rx thread has a period of 10ms from the starting point
 
  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime  = runtime;
  attr.sched_deadline = deadline;
  attr.sched_period   = period; 

  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] eNB RX sched_setattr failed\n");
    return &eNB_thread_rx_status[proc->subframe];
  }

  LOG_I( HW, "[SCHED] eNB RX deadline thread %d(TID %ld) started on CPU %d\n", proc->subframe, gettid(), sched_getcpu() );
#else
    int policy, s, j;
  struct sched_param sparam;
  char cpu_affinity[1024];
  cpu_set_t cpuset;

  /* Set affinity mask to include CPUs 1 to MAX_CPUS */
  /* CPU 0 is reserved for UHD */
  /* CPU 1 is reserved for all TX threads */
  /* CPU 2..MAX_CPUS is reserved for all RX threads */
  /* Set CPU Affinity only if number of CPUs >2 */
  CPU_ZERO(&cpuset);
  #ifdef CPU_AFFINITY
  if (get_nprocs() >2)
  {
    for (j = 1; j < get_nprocs(); j++)
       CPU_SET(j, &cpuset);
  
    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
      perror( "pthread_setaffinity_np");  
      exit_fun (" Error setting processor affinity :");
    }
  }
  #endif
  /* Check the actual affinity mask assigned to the thread */

  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0)
  {
     perror ("pthread_getaffinity_np");
     exit_fun (" Error getting processor affinity :");
  }
  memset(cpu_affinity,0, sizeof(cpu_affinity));

  for (j = 0; j < CPU_SETSIZE; j++)
     if (CPU_ISSET(j, &cpuset))
     {  
        char temp[1024];
        sprintf (temp, " CPU_%d", j);
        strcat(cpu_affinity, temp);
     }


  memset(&sparam, 0 , sizeof (sparam)); 
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO)-1;

  policy = SCHED_FIFO ; 
  s = pthread_setschedparam(pthread_self(), policy, &sparam);
  if (s != 0)
     {
     perror("pthread_setschedparam : ");
     exit_fun("Error setting thread priority");
     }

  memset(&sparam, 0 , sizeof (sparam));

  s = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (s != 0)
   {
     perror("pthread_getschedparam");
     exit_fun("Error getting thread priority");
   }


LOG_I( HW, "[SCHED][eNB] RX thread %d started on CPU %d TID %ld, sched_policy = %s, priority = %d, CPU Affinity = %s\n", proc->subframe, sched_getcpu(),gettid(),
       (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
       (policy == SCHED_RR)    ? "SCHED_RR" :
       (policy == SCHED_OTHER) ? "SCHED_OTHER" :
        "???",
        sparam.sched_priority, cpu_affinity);


#endif
#endif // RTAI

  mlockall(MCL_CURRENT | MCL_FUTURE);

#ifdef HARD_RT
  rt_make_hard_real_time();
#endif

  while (!oai_exit) {

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RX0+(2*proc->subframe), 0 );

    if (pthread_mutex_lock(&proc->mutex_rx) != 0) {
      LOG_E( PHY, "[SCHED][eNB] error locking mutex for eNB RX proc %d\n", proc->subframe );
      exit_fun( "error locking mutex" );
      break;
    }

    while (proc->instance_cnt_rx < 0) {
      // most of the time the thread is waiting here
      // proc->instance_cnt_rx is -1
      pthread_cond_wait( &proc->cond_rx, &proc->mutex_rx ); // this unlocks mutex_rx while waiting and then locks it again
    }

    if (pthread_mutex_unlock(&proc->mutex_rx) != 0) {
      LOG_E( PHY, "[SCHED][eNB] error unlocking mutex for eNB RX proc %d\n", proc->subframe );
      exit_fun( "error unlocking mutex" );
      break;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RX0+(2*proc->subframe), 1 );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX_ENB, proc->frame_rx );
    start_meas( &softmodem_stats_rx_sf[proc->subframe] );

    if (oai_exit) break;

    if ((((PHY_vars_eNB_g[0][proc->CC_id]->lte_frame_parms.frame_type == TDD )&&(subframe_select(&PHY_vars_eNB_g[0][proc->CC_id]->lte_frame_parms,proc->subframe_rx)==SF_UL)) ||
         (PHY_vars_eNB_g[0][proc->CC_id]->lte_frame_parms.frame_type == FDD))) {

      phy_procedures_eNB_RX( proc->subframe, PHY_vars_eNB_g[0][proc->CC_id], 0, no_relay );
    }

    if ((subframe_select(&PHY_vars_eNB_g[0][proc->CC_id]->lte_frame_parms,proc->subframe_rx) == SF_S)) {
      phy_procedures_eNB_S_RX( proc->subframe, PHY_vars_eNB_g[0][proc->CC_id], 0, no_relay );
    }

    if (pthread_mutex_lock(&proc->mutex_rx) != 0) {
      LOG_E( PHY, "[SCHED][eNB] error locking mutex for eNB RX proc %d\n", proc->subframe );
      exit_fun( "error locking mutex" );
      break;
    }

    proc->instance_cnt_rx--;

    if (pthread_mutex_unlock(&proc->mutex_rx) != 0) {
      LOG_E( PHY, "[SCHED][eNB] error unlocking mutex for eNB RX proc %d\n", proc->subframe );
      exit_fun( "error unlocking mutex" );
      break;
    }

    proc->frame_rx++;

    if (proc->frame_rx==1024)
      proc->frame_rx=0;

    stop_meas( &softmodem_stats_rx_sf[proc->subframe] );
#ifdef DEADLINE_SCHEDULER
    if (opp_enabled){
      if(softmodem_stats_rx_sf[proc->subframe].diff_now/(cpuf) > attr.sched_runtime){
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RUNTIME_RX_ENB, (softmodem_stats_rx_sf[proc->subframe].diff_now/cpuf - attr.sched_runtime)/1000000.0);
      }
    }
#endif  
    print_meas_now(&softmodem_stats_rx_sf[proc->subframe],"eNB_RX_SF",proc->subframe, rx_time_file);
  }

  //stop_meas( &softmodem_stats_rx_sf[proc->subframe] );
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RX0+(2*proc->subframe), 0 );

#ifdef HARD_RT
  rt_make_soft_real_time();
#endif

#ifdef DEBUG_THREADS
  printf( "Exiting eNB thread RX %d\n", proc->subframe );
#endif
  // clean task
#ifdef RTAI
  rt_task_delete(task);
#endif

  eNB_thread_rx_status[proc->subframe] = 0;
  return &eNB_thread_rx_status[proc->subframe];
}




void init_eNB_proc(void)
{
  int i;
  int CC_id;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    for (i=0; i<NUM_ENB_THREADS; i++) {
      // set the stack size
     

#ifndef DEADLINE_SCHEDULER 
      /*  
       pthread_attr_init( &attr_eNB_proc_tx[CC_id][i] );
       if (pthread_attr_setstacksize( &attr_eNB_proc_tx[CC_id][i], 64 *PTHREAD_STACK_MIN ) != 0)
        perror("[ENB_PROC_TX] setting thread stack size failed\n");
      
      pthread_attr_init( &attr_eNB_proc_rx[CC_id][i] );
      if (pthread_attr_setstacksize( &attr_eNB_proc_rx[CC_id][i], 64 * PTHREAD_STACK_MIN ) != 0)
        perror("[ENB_PROC_RX] setting thread stack size failed\n");
      */
      // set the kernel scheduling policy and priority
      sched_param_eNB_proc_tx[CC_id][i].sched_priority = sched_get_priority_max(SCHED_FIFO)-1; //OPENAIR_THREAD_PRIORITY;
      pthread_attr_setschedparam  (&attr_eNB_proc_tx[CC_id][i], &sched_param_eNB_proc_tx[CC_id][i]);
      pthread_attr_setschedpolicy (&attr_eNB_proc_tx[CC_id][i], SCHED_FIFO);
      sched_param_eNB_proc_rx[CC_id][i].sched_priority = sched_get_priority_max(SCHED_FIFO)-1; //OPENAIR_THREAD_PRIORITY;
      pthread_attr_setschedparam  (&attr_eNB_proc_rx[CC_id][i], &sched_param_eNB_proc_rx[CC_id][i]);
      pthread_attr_setschedpolicy (&attr_eNB_proc_rx[CC_id][i], SCHED_FIFO);
      printf("Setting OS scheduler to SCHED_FIFO for eNB [cc%d][thread%d] \n",CC_id, i);
#endif
      PHY_vars_eNB_g[0][CC_id]->proc[i].instance_cnt_tx = -1;
      PHY_vars_eNB_g[0][CC_id]->proc[i].instance_cnt_rx = -1;
      PHY_vars_eNB_g[0][CC_id]->proc[i].subframe = i;
      PHY_vars_eNB_g[0][CC_id]->proc[i].CC_id = CC_id;
      pthread_mutex_init( &PHY_vars_eNB_g[0][CC_id]->proc[i].mutex_tx, NULL);
      pthread_mutex_init( &PHY_vars_eNB_g[0][CC_id]->proc[i].mutex_rx, NULL);
      pthread_cond_init( &PHY_vars_eNB_g[0][CC_id]->proc[i].cond_tx, NULL);
      pthread_cond_init( &PHY_vars_eNB_g[0][CC_id]->proc[i].cond_rx, NULL);
#ifndef DEADLINE_SCHEDULER
      pthread_create( &PHY_vars_eNB_g[0][CC_id]->proc[i].pthread_tx, &attr_eNB_proc_tx[CC_id][i], eNB_thread_tx, &PHY_vars_eNB_g[0][CC_id]->proc[i] );
      pthread_create( &PHY_vars_eNB_g[0][CC_id]->proc[i].pthread_rx, &attr_eNB_proc_rx[CC_id][i], eNB_thread_rx, &PHY_vars_eNB_g[0][CC_id]->proc[i] );
#else 
      pthread_create( &PHY_vars_eNB_g[0][CC_id]->proc[i].pthread_tx, NULL, eNB_thread_tx, &PHY_vars_eNB_g[0][CC_id]->proc[i] );
      pthread_create( &PHY_vars_eNB_g[0][CC_id]->proc[i].pthread_rx, NULL, eNB_thread_rx, &PHY_vars_eNB_g[0][CC_id]->proc[i] );
#endif
      char name[16];
      snprintf( name, sizeof(name), "TX %d", i );
      pthread_setname_np( PHY_vars_eNB_g[0][CC_id]->proc[i].pthread_tx, name );
      snprintf( name, sizeof(name), "RX %d", i );
      pthread_setname_np( PHY_vars_eNB_g[0][CC_id]->proc[i].pthread_rx, name );
      PHY_vars_eNB_g[0][CC_id]->proc[i].frame_tx = 0;
      PHY_vars_eNB_g[0][CC_id]->proc[i].frame_rx = 0;
#ifdef EXMIMO
      PHY_vars_eNB_g[0][CC_id]->proc[i].subframe_rx = (i+9)%10;
      PHY_vars_eNB_g[0][CC_id]->proc[i].subframe_tx = (i+1)%10;
#else
      PHY_vars_eNB_g[0][CC_id]->proc[i].subframe_rx = i;
      PHY_vars_eNB_g[0][CC_id]->proc[i].subframe_tx = (i+2)%10;
#endif
    }


#ifdef EXMIMO
    // TX processes subframe + 1, RX subframe -1
    // Note this inialization is because the first process awoken for frame 0 is number 1 and so processes 9 and 0 have to start with frame 1

    //PHY_vars_eNB_g[0][CC_id]->proc[0].frame_rx = 1023;
    PHY_vars_eNB_g[0][CC_id]->proc[9].frame_tx = 1;
    PHY_vars_eNB_g[0][CC_id]->proc[0].frame_tx = 1;
#else
    // TX processes subframe +2, RX subframe
    // Note this inialization is because the first process awoken for frame 0 is number 1 and so processes 8,9 and 0 have to start with frame 1
    //    PHY_vars_eNB_g[0][CC_id]->proc[7].frame_tx = 1;
    PHY_vars_eNB_g[0][CC_id]->proc[8].frame_tx = 1;
    PHY_vars_eNB_g[0][CC_id]->proc[9].frame_tx = 1;
    //    PHY_vars_eNB_g[0][CC_id]->proc[0].frame_tx = 1;
#endif
  }

  /* setup PHY proc TX sync mechanism */
  for (i=0; i<NUM_ENB_THREADS; i++) {
    pthread_mutex_init(&sync_phy_proc[i].mutex_phy_proc_tx, NULL);
    pthread_cond_init(&sync_phy_proc[i].cond_phy_proc_tx, NULL);
    sync_phy_proc[i].phy_proc_CC_id = 0;
  }
}

/*!
 * \brief Terminate eNB TX and RX threads.
 */
void kill_eNB_proc(void)
{
  int *status;

  for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++)
    for (int i=0; i<NUM_ENB_THREADS; i++) {

#ifdef DEBUG_THREADS
      printf( "Killing TX CC_id %d thread %d\n", CC_id, i );
#endif

      PHY_vars_eNB_g[0][CC_id]->proc[i].instance_cnt_tx = 0; // FIXME data race!
      pthread_cond_signal( &PHY_vars_eNB_g[0][CC_id]->proc[i].cond_tx );
      pthread_cond_broadcast(&sync_phy_proc[i].cond_phy_proc_tx);

#ifdef DEBUG_THREADS
      printf( "Joining eNB TX CC_id %d thread %d...\n", CC_id, i );
#endif
      int result = pthread_join( PHY_vars_eNB_g[0][CC_id]->proc[i].pthread_tx, (void**)&status );

#ifdef DEBUG_THREADS

      if (result != 0) {
        printf( "Error joining thread.\n" );
      } else {
        if (status) {
          printf( "status %d\n", *status );
        } else {
          printf( "The thread was killed. No status available.\n" );
        }
      }

#else
      UNUSED(result)
#endif

#ifdef DEBUG_THREADS
      printf( "Killing RX CC_id %d thread %d\n", CC_id, i );
#endif

      PHY_vars_eNB_g[0][CC_id]->proc[i].instance_cnt_rx = 0; // FIXME data race!
      pthread_cond_signal( &PHY_vars_eNB_g[0][CC_id]->proc[i].cond_rx );

#ifdef DEBUG_THREADS
      printf( "Joining eNB RX CC_id %d thread %d...\n", CC_id, i );
#endif
      result = pthread_join( PHY_vars_eNB_g[0][CC_id]->proc[i].pthread_rx, (void**)&status );

#ifdef DEBUG_THREADS

      if (result != 0) {
        printf( "Error joining thread.\n" );
      } else {
        if (status) {
          printf( "status %d\n", *status );
        } else {
          printf( "The thread was killed. No status available.\n" );
        }
      }

#else
      UNUSED(result)
#endif

      pthread_mutex_destroy( &PHY_vars_eNB_g[0][CC_id]->proc[i].mutex_tx );
      pthread_mutex_destroy( &PHY_vars_eNB_g[0][CC_id]->proc[i].mutex_rx );
      pthread_cond_destroy( &PHY_vars_eNB_g[0][CC_id]->proc[i].cond_tx );
      pthread_cond_destroy( &PHY_vars_eNB_g[0][CC_id]->proc[i].cond_rx );
    }
}






/*!
 * \brief This is the main eNB thread.
 * \param arg unused
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* eNB_thread( void* arg )
{
return 0;
  UNUSED(arg);
  static int eNB_thread_status;

  unsigned char slot;
#ifdef EXMIMO
  slot=0;
  RTIME time_in;
  volatile unsigned int *DAQ_MBOX = openair0_daq_cnt();
  int mbox_target=0,mbox_current=0;
  int hw_slot,delay_cnt;
  int diff;
  int ret;
  int first_run=1;
#else
  // the USRP implementation operates on subframes, not slots
  // one subframe consists of one even and one odd slot
  slot = 1;
  int spp;
  int tx_launched = 0;
  int card=0;

  void *rxp[2]; // FIXME hard coded array size; indexed by lte_frame_parms.nb_antennas_rx
  void *txp[2]; // FIXME hard coded array size; indexed by lte_frame_parms.nb_antennas_tx

  int hw_subframe = 0; // 0..NUM_ENB_THREADS-1 => 0..9
  
  unsigned int rx_pos = 0;
  unsigned int tx_pos = 0;
#endif
  int CC_id=0;	
  struct timespec trx_time0, trx_time1, trx_time2;

  /* avoid gcc warnings */
  (void)trx_time0;
  (void)trx_time1;
  (void)trx_time2;

#ifdef RTAI
  RT_TASK* task = rt_task_init_schmod(nam2num("eNBmain"), 0, 0, 0, SCHED_FIFO, 0xF);
#else
#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;
  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  /* This creates a .2 ms  reservation */
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime  = (0.3 * 100) * 10000;
  attr.sched_deadline = (0.9 * 100) * 10000;
  attr.sched_period   = 1 * 1000000;

  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] main eNB thread: sched_setattr failed\n");
    exit_fun("Nothing to add");
  } else {
    LOG_I(HW,"[SCHED][eNB] eNB main deadline thread %ld started on CPU %d\n",
          gettid(),sched_getcpu());
  }

#else

  int policy, s, j;
  struct sched_param sparam;
  char cpu_affinity[1024];
  cpu_set_t cpuset;

  /* Set affinity mask to include CPUs 1 to MAX_CPUS */
  /* CPU 0 is reserved for UHD threads */
  CPU_ZERO(&cpuset);
  #ifdef CPU_AFFINITY
  if (get_nprocs() >2)
  {
    for (j = 1; j < get_nprocs(); j++)
      CPU_SET(j, &cpuset);

    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
      perror( "pthread_setaffinity_np");
      exit_fun("Error setting processor affinity");
    }
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
  for (j = 0; j < CPU_SETSIZE; j++)
  if (CPU_ISSET(j, &cpuset))
  {  
     char temp[1024];
     sprintf(temp, " CPU_%d ", j);    
     strcat(cpu_affinity, temp);
  }

  memset(&sparam, 0 , sizeof (sparam));
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
  policy = SCHED_FIFO ; 
  
  s = pthread_setschedparam(pthread_self(), policy, &sparam);
  if (s != 0)
     {
     perror("pthread_setschedparam : ");
     exit_fun("Error setting thread priority");
     }
  s = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (s != 0)
   {
     perror("pthread_getschedparam : ");
     exit_fun("Error getting thread priority");

   }


  LOG_I( HW, "[SCHED][eNB] Started eNB main thread on CPU %d TID %ld , sched_policy = %s, priority = %d, CPU Affinity = %s \n", (int)sched_getcpu(), gettid(),
                   (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                   (policy == SCHED_RR)    ? "SCHED_RR" :
                   (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                   "???",
                   (int) sparam.sched_priority, cpu_affinity);


#endif
#endif

  // stop early, if an exit is requested
  // FIXME really neccessary?
  if (oai_exit)
    goto eNB_thread_cleanup;

#ifdef RTAI
  LOG_I( HW, "[SCHED][eNB] Started eNB main thread (id %p)\n", task );
#endif

#ifdef HARD_RT
  rt_make_hard_real_time();
#endif

  printf("eNB_thread: mlockall in ...\n");
  mlockall(MCL_CURRENT | MCL_FUTURE);
  printf("eNB_thread: mlockall out ...\n");

  timing_info.time_min = 100000000ULL;
  timing_info.time_max = 0;
  timing_info.time_avg = 0;
  timing_info.n_samples = 0;

  printf( "waiting for sync (eNB_thread)\n" );
  pthread_mutex_lock( &sync_mutex );

  while (sync_var<0)
    pthread_cond_wait( &sync_cond, &sync_mutex );

  pthread_mutex_unlock(&sync_mutex);

  printf( "got sync (eNB_thread)\n" );

  int frame = 0;

#ifndef EXMIMO
  spp        = openair0_cfg[0].samples_per_packet;
  tx_pos     = openair0_cfg[0].tx_scheduling_advance;
#endif

#if defined(ENABLE_ITTI)
  wait_system_ready ("Waiting for eNB application to be ready %s\r", &start_eNB);
#endif 

  while (!oai_exit) {
    start_meas( &softmodem_stats_mt );

#ifdef EXMIMO
    hw_slot = (((((volatile unsigned int *)DAQ_MBOX)[0]+1)%150)<<1)/15;
    //        LOG_D(HW,"eNB frame %d, time %llu: slot %d, hw_slot %d (mbox %d)\n",frame,rt_get_time_ns(),slot,hw_slot,((unsigned int *)DAQ_MBOX)[0]);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_HW_SUBFRAME, hw_slot>>1);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_HW_FRAME, frame);
    //this is the mbox counter where we should be
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

    //when we start the aquisition we want to start with slot 0, so we rather wait for the hardware than to advance the slot number (a positive diff will cause the programm to go into the second if clause rather than the first)
    if (first_run==1) {
      first_run=0;

      if (diff<0)
        diff = diff + 150;

      LOG_I(HW,"eNB Frame %d, time %llu: slot %d, hw_slot %d, diff %d\n",frame, rt_get_time_ns(), slot, hw_slot, diff);
    }

    if (((slot%2==0) && (diff < (-14))) || ((slot%2==1) && (diff < (-7)))) {

      // at the eNB, even slots have double as much time since most of the processing is done here and almost nothing in odd slots
      LOG_W(HW,"eNB Frame %d, time %llu: missed slot %d, proceeding with next one (slot %d, hw_slot %d, mbox_current %d, mbox_target %d, diff %d)\n",
	    frame, rt_get_time_ns(), num_missed_slots, slot, hw_slot, mbox_current,mbox_target, diff);
      
      if (exit_missed_slots==1) {
        stop_meas(&softmodem_stats_mt);
        exit_fun("[HW][eNB] missed slot");
      } else {
        num_missed_slots++;
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_MISSED_SLOTS_ENB,num_missed_slots );
      }

      if ((slot&1) == 1) {
	for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++){
	  if (PHY_vars_eNB_g[0][CC_id]->proc[((slot>>1)+1)%10].frame_rx==1023)
	    PHY_vars_eNB_g[0][CC_id]->proc[((slot>>1)+1)%10].frame_rx=0;
	  else 
	    PHY_vars_eNB_g[0][CC_id]->proc[((slot>>1)+1)%10].frame_rx += 1;
	  
	  if (PHY_vars_eNB_g[0][CC_id]->proc[((slot>>1)+1)%10].frame_tx==1023)
	    PHY_vars_eNB_g[0][CC_id]->proc[((slot>>1)+1)%10].frame_tx=0;
	  else 
	    PHY_vars_eNB_g[0][CC_id]->proc[((slot>>1)+1)%10].frame_tx += 1;
	}
      }
      
     slot++;
     if (slot == 20) {
       frame++;
       slot = 0;
     }

    
    }

    if (diff>8)
      LOG_D(HW,"eNB Frame %d, time %llu: skipped slot, waiting for hw to catch up (slot %d, hw_slot %d, mbox_current %d, mbox_target %d, diff %d)\n",frame, rt_get_time_ns(), slot, hw_slot, mbox_current,
            mbox_target, diff);

    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *DAQ_MBOX);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DIFF, diff);

    delay_cnt = 0;

    while ((diff>0) && (!oai_exit)) {
      time_in = rt_get_time_ns();
      //LOG_D(HW,"eNB Frame %d delaycnt %d : hw_slot %d (%d), slot %d, (slot+1)*15=%d, diff %d, time %llu\n",frame,delay_cnt,hw_slot,((unsigned int *)DAQ_MBOX)[0],slot,(((slot+1)*15)>>1),diff,time_in);
      //LOG_D(HW,"eNB Frame %d, time %llu: sleeping for %llu (slot %d, hw_slot %d, diff %d, mbox %d, delay_cnt %d)\n", frame, time_in, diff*DAQ_PERIOD,slot,hw_slot,diff,((volatile unsigned int *)DAQ_MBOX)[0],delay_cnt);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RT_SLEEP,1);
      ret = rt_sleep_ns(diff*DAQ_PERIOD);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RT_SLEEP,0);
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *DAQ_MBOX);

      if (ret)
        LOG_D(HW,"eNB Frame %d, time %llu: rt_sleep_ns returned %d\n",frame, time_in);

      hw_slot = (((((volatile unsigned int *)DAQ_MBOX)[0]+1)%150)<<1)/15;
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_HW_SUBFRAME, hw_slot>>1);
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_HW_FRAME, frame);
//LOG_D(HW,"eNB Frame %d : hw_slot %d, time %llu\n",frame,hw_slot,rt_get_time_ns());
      delay_cnt++;

      if (delay_cnt == 10) {
        stop_meas(&softmodem_stats_mt);
        LOG_D(HW,"eNB Frame %d: HW stopped ... \n",frame);
        exit_fun("[HW][eNB] HW stopped");
      }

      mbox_current = ((volatile unsigned int *)DAQ_MBOX)[0];

      if ((mbox_current>=135) && (mbox_target<15)) //handle the frame wrap-arround
        diff = 150-mbox_current+mbox_target;
      else if ((mbox_current<15) && (mbox_target>=135))
        diff = -150+mbox_target-mbox_current;
      else
        diff = mbox_target - mbox_current;
    }

#else  // EXMIMO
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_HW_SUBFRAME, hw_subframe );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_HW_FRAME, frame );
    tx_launched = 0;

    while (rx_pos < ((1+hw_subframe)*PHY_vars_eNB_g[0][0]->lte_frame_parms.samples_per_tti)) {

      unsigned int rxs;
#ifndef USRP_DEBUG
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TXCNT, tx_pos );
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RXCNT, rx_pos );

      clock_gettime( CLOCK_MONOTONIC, &trx_time0 );

       start_meas( &softmodem_stats_hw );

      openair0_timestamp timestamp;
      int i=0;
      // prepare rx buffer pointers
      for (i=0; i<PHY_vars_eNB_g[0][0]->lte_frame_parms.nb_antennas_rx; i++)
        rxp[i] = (void*)&rxdata[i][rx_pos];
	// check if nsymb_read == spp
	// map antenna port i to the cc_id. Now use the 1:1 mapping
	rxs = openair0.trx_read_func(&openair0,
				     &timestamp,
				     rxp,
				     spp,
				     PHY_vars_eNB_g[0][0]->lte_frame_parms.nb_antennas_rx);
      
      stop_meas( &softmodem_stats_hw );
      if (frame > 50) { 
	  clock_gettime( CLOCK_MONOTONIC, &trx_time1 );
      }

      if (frame > 20){ 
	if (rxs != spp)
	  exit_fun( "problem receiving samples" );
      }
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );

      // Transmit TX buffer based on timestamp from RX
    
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
      // prepare tx buffer pointers
      for (i=0; i<PHY_vars_eNB_g[0][0]->lte_frame_parms.nb_antennas_tx; i++)
	txp[i] = (void*)&txdata[i][tx_pos];
      // if symb_written < spp ==> error 
      if (frame > 50) {
	openair0.trx_write_func(&openair0,
				(timestamp+(openair0_cfg[card].tx_scheduling_advance)-openair0_cfg[card].tx_sample_advance),
				txp,
				spp,
				PHY_vars_eNB_g[0][0]->lte_frame_parms.nb_antennas_tx,
				1);
      }
      
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, timestamp&0xffffffff );
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, (timestamp+(openair0_cfg[card].tx_scheduling_advance)-openair0_cfg[card].tx_sample_advance)&0xffffffff );

      stop_meas( &softmodem_stats_mt );
      if (frame > 50) { 
	  clock_gettime( CLOCK_MONOTONIC, &trx_time2 );
	  //update_difftimes(trx_time1, trx_time2);
      }


      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE,0);
#else
      // USRP_DEBUG is active
      rt_sleep_ns(1000000);
#endif
      /* FT configurable tx lauch delay (in slots): txlaunch_wait, txlaunch_wait_slotcount is device specific and 
	 set in the corresponding library (with txlaunch_wait=1 and txlaunch_wait_slotcount=1 the check is as it previously was) */
      /* old check:
	 if ((frame>50) &&
	 (tx_launched == 0) &&
	 (rx_pos >= (((2*hw_subframe)+1)*PHY_vars_eNB_g[0][0]->lte_frame_parms.samples_per_tti>>1))) {*/
      if ( (frame>50) && (tx_launched == 0) &&
	   ((openair0_cfg[card].txlaunch_wait == 0) ||
	    ((openair0_cfg[card].txlaunch_wait == 1) &&
	     (rx_pos >= (((2*hw_subframe)+openair0_cfg[card].txlaunch_wait_slotcount)*PHY_vars_eNB_g[0][0]->lte_frame_parms.samples_per_tti>>1))))) { 
	
        tx_launched = 1;

        for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
          if (pthread_mutex_lock(&PHY_vars_eNB_g[0][CC_id]->proc[hw_subframe].mutex_tx) != 0) {
            LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB TX thread %d (IC %d)\n", hw_subframe, PHY_vars_eNB_g[0][CC_id]->proc[hw_subframe].instance_cnt_tx );
            exit_fun( "error locking mutex_tx" );
            break;
          }

          int cnt_tx = ++PHY_vars_eNB_g[0][CC_id]->proc[hw_subframe].instance_cnt_tx;

          pthread_mutex_unlock( &PHY_vars_eNB_g[0][CC_id]->proc[hw_subframe].mutex_tx );

          if (cnt_tx == 0) {
            // the thread was presumably waiting where it should and can now be woken up
            if (pthread_cond_signal(&PHY_vars_eNB_g[0][CC_id]->proc[hw_subframe].cond_tx) != 0) {
              LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB TX thread %d\n", hw_subframe );
              exit_fun( "ERROR pthread_cond_signal" );
              break;
            }
          } else {
            LOG_W( PHY,"[eNB] Frame %d, eNB TX thread %d busy!! (rx_cnt %u, cnt_tx %i)\n", PHY_vars_eNB_g[0][CC_id]->proc[hw_subframe].frame_tx, hw_subframe, rx_pos, cnt_tx );
            exit_fun( "TX thread busy" );
            break;
          }
        }
      }

      rx_pos += spp;
      tx_pos += spp;

      if (tx_pos >= openair0_cfg[card].samples_per_frame)
        tx_pos -= openair0_cfg[card].samples_per_frame;
    }

    if (rx_pos >= openair0_cfg[card].samples_per_frame)
      rx_pos -= openair0_cfg[card].samples_per_frame;


#endif // USRP

    if (oai_exit) break;

    timing_info.time_last = timing_info.time_now;
    timing_info.time_now = rt_get_time_ns();

    if (timing_info.n_samples>0) {
      RTIME time_diff = timing_info.time_now - timing_info.time_last;

      if (time_diff < timing_info.time_min)
        timing_info.time_min = time_diff;

      if (time_diff > timing_info.time_max)
        timing_info.time_max = time_diff;

      timing_info.time_avg += time_diff;
    }

    timing_info.n_samples++;

    if ((slot&1) == 1) {
      // odd slot
#ifdef EXMIMO
      int sf = ((slot>>1)+1)%10;
#else
      int sf = hw_subframe;
#endif
      /* TODO: is it the right place for master tick? */
      T(T_ENB_MASTER_TICK, T_INT(0), T_INT(frame % 1024), T_INT(sf));
      if (frame>50) {
	for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
#ifdef EXMIMO
	  
	  if (pthread_mutex_lock(&PHY_vars_eNB_g[0][CC_id]->proc[sf].mutex_tx) != 0) {
	    LOG_E(PHY,"[eNB] ERROR pthread_mutex_lock for eNB TX thread %d (IC %d)\n",sf,PHY_vars_eNB_g[0][CC_id]->proc[sf].instance_cnt_tx);
	  } else {
	    //          LOG_I(PHY,"[eNB] Waking up eNB process %d (IC %d)\n",sf,PHY_vars_eNB_g[0][CC_id]->proc[sf].instance_cnt);
	    PHY_vars_eNB_g[0][CC_id]->proc[sf].instance_cnt_tx++;
	    pthread_mutex_unlock(&PHY_vars_eNB_g[0][CC_id]->proc[sf].mutex_tx);
	    
	    if (PHY_vars_eNB_g[0][CC_id]->proc[sf].instance_cnt_tx == 0) {
	      if (pthread_cond_signal(&PHY_vars_eNB_g[0][CC_id]->proc[sf].cond_tx) != 0) {
		LOG_E(PHY,"[eNB] ERROR pthread_cond_signal for eNB TX thread %d\n",sf);
		exit_fun("nothing to add");
	      }
	    } else {
	      LOG_W(PHY,"[eNB] Frame %d, eNB TX thread %d busy!!\n",PHY_vars_eNB_g[0][CC_id]->proc[sf].frame_tx,sf);
	      exit_fun("nothing to add");
	    }
	  }
	  
#endif

	  if (pthread_mutex_lock(&PHY_vars_eNB_g[0][CC_id]->proc[sf].mutex_rx) != 0) {
	    LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB RX thread %d (IC %d)\n", sf, PHY_vars_eNB_g[0][CC_id]->proc[sf].instance_cnt_rx );
	    exit_fun( "error locking mutex_rx" );
	    break;
	  }
	  
	  int cnt_rx = ++PHY_vars_eNB_g[0][CC_id]->proc[sf].instance_cnt_rx;
	  
	  pthread_mutex_unlock( &PHY_vars_eNB_g[0][CC_id]->proc[sf].mutex_rx );
	  
	  if (cnt_rx == 0) {
	    // the thread was presumably waiting where it should and can now be woken up
	    if (pthread_cond_signal(&PHY_vars_eNB_g[0][CC_id]->proc[sf].cond_rx) != 0) {
	      LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB RX thread %d\n", sf );
	      exit_fun( "ERROR pthread_cond_signal" );
	      break;
	    }
	  } else {
	    LOG_W( PHY, "[eNB] Frame %d, eNB RX thread %d busy!! instance_cnt %d CC_id %d\n", PHY_vars_eNB_g[0][CC_id]->proc[sf].frame_rx, sf, PHY_vars_eNB_g[0][CC_id]->proc[sf].instance_cnt_rx, CC_id );
	    exit_fun( "RX thread busy" );
	    break;
	  }
	}
      }
    }

#ifdef EXMIMO
    slot++;

    if (slot == 20) {
      frame++;
      slot = 0;
    }

#else
    hw_subframe++;
    slot += 2;

    if (hw_subframe == NUM_ENB_THREADS) {
      // the radio frame is complete, start over
      hw_subframe = 0;
      frame++;
      slot = 1;
    }

#endif

#if defined(ENABLE_ITTI)
    itti_update_lte_time( frame, slot );
#endif
  }

eNB_thread_cleanup:
#ifdef DEBUG_THREADS
  printf( "eNB_thread: finished, ran %d times.\n", frame );
#endif

#ifdef HARD_RT
  rt_make_soft_real_time();
#endif

#ifdef DEBUG_THREADS
  printf( "Exiting eNB_thread ..." );
#endif
  // clean task
#ifdef RTAI
  rt_task_delete(task);
#endif

  eNB_thread_status = 0;

  // print_difftimes();

  return &eNB_thread_status;
}




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
#ifdef EXMIMO
  char rxg_fname[256], line[1000];
  FILE *rxg_fd;
  int l;
#endif




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
    LONG_OPTION_TXGAIN,
    LONG_OPTION_SCANCARRIER,
    LONG_OPTION_MAXPOWER,
    LONG_OPTION_DUMP_FRAME,
    LONG_OPTION_LOOPMEMORY,
    LONG_OPTION_PHYTEST,

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
    {"ue-txgain",   required_argument,  NULL, LONG_OPTION_TXGAIN},
    {"ue-scan-carrier",   no_argument,  NULL, LONG_OPTION_SCANCARRIER},
    {"ue-max-power",   required_argument,  NULL, LONG_OPTION_MAXPOWER},
    {"ue-dump-frame", no_argument, NULL, LONG_OPTION_DUMP_FRAME},
    {"loop-memory", required_argument, NULL, LONG_OPTION_LOOPMEMORY},
    {"phy-test", no_argument, NULL, LONG_OPTION_PHYTEST},

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
      break;

    case LONG_OPTION_RXGAIN:
      for (i=0; i<4; i++)
        rx_gain[0][i] = atof(optarg);

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
#ifdef EXMIMO
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

#endif
      break;

    case 'G':
      glog_verbosity=atoi(optarg);// value from 0, 0x5, 0x15, 0x35, 0x75
      break;

    case 'x':
      transmission_mode = atoi(optarg);

      if (transmission_mode > 7) {
        printf("Transmission mode %d not supported for the moment\n",transmission_mode);
        exit(-1);
      }
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

      for (j=0; j<enb_properties->properties[i]->nb_rrh_gw; j++) {
	
	if (enb_properties->properties[i]->rrh_gw_config[j].active == 1 ) {
	  local_remote_radio = BBU_REMOTE_RADIO_HEAD;
	  eth_params = (eth_params_t*)malloc(sizeof(eth_params_t));
	  memset(eth_params, 0, sizeof(eth_params_t));
	  
	  eth_params->local_if_name             = enb_properties->properties[i]->rrh_gw_if_name;
	  eth_params->my_addr                   = enb_properties->properties[i]->rrh_gw_config[j].local_address;
	  eth_params->my_port                   = enb_properties->properties[i]->rrh_gw_config[j].local_port;
	  eth_params->remote_addr               = enb_properties->properties[i]->rrh_gw_config[j].remote_address;
	  eth_params->remote_port               = enb_properties->properties[i]->rrh_gw_config[j].remote_port;
	  eth_params->transp_preference         = enb_properties->properties[i]->rrh_gw_config[j].raw;	 
	  eth_params->iq_txshift                = enb_properties->properties[i]->rrh_gw_config[j].iq_txshift;
	  eth_params->tx_sample_advance         = enb_properties->properties[i]->rrh_gw_config[j].tx_sample_advance;
	  eth_params->tx_scheduling_advance     = enb_properties->properties[i]->rrh_gw_config[j].tx_scheduling_advance;
	  if (enb_properties->properties[i]->rrh_gw_config[j].exmimo == 1) {
	     eth_params->rf_preference          = EXMIMO_DEV;
	  } else if (enb_properties->properties[i]->rrh_gw_config[j].usrp_b200 == 1) {
	    eth_params->rf_preference          = USRP_B200_DEV;
	  } else if (enb_properties->properties[i]->rrh_gw_config[j].usrp_x300 == 1) {
	   eth_params->rf_preference          = USRP_X300_DEV;
	  } else if (enb_properties->properties[i]->rrh_gw_config[j].bladerf == 1) {
	    eth_params->rf_preference          = BLADERF_DEV;
	  } else if (enb_properties->properties[i]->rrh_gw_config[j].lmssdr == 1) {
	    //eth_params->rf_preference          = LMSSDR_DEV;
	  } else {
	    eth_params->rf_preference          = 0;
	  } 
	} else {
	  local_remote_radio = BBU_LOCAL_RADIO_HEAD; 
	}
	
      }

      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        frame_parms[CC_id]->frame_type =       enb_properties->properties[i]->frame_type[CC_id];
        frame_parms[CC_id]->tdd_config =       enb_properties->properties[i]->tdd_config[CC_id];
        frame_parms[CC_id]->tdd_config_S =     enb_properties->properties[i]->tdd_config_s[CC_id];
        frame_parms[CC_id]->Ncp =              enb_properties->properties[i]->prefix_type[CC_id];

        //for (j=0; j < enb_properties->properties[i]->nb_cc; j++ ){
        frame_parms[CC_id]->Nid_cell            =  enb_properties->properties[i]->Nid_cell[CC_id];
        frame_parms[CC_id]->N_RB_DL             =  enb_properties->properties[i]->N_RB_DL[CC_id];
        frame_parms[CC_id]->N_RB_UL             =  enb_properties->properties[i]->N_RB_DL[CC_id];
        frame_parms[CC_id]->nb_antennas_tx      =  enb_properties->properties[i]->nb_antennas_tx[CC_id];
        frame_parms[CC_id]->nb_antennas_tx_eNB  =  enb_properties->properties[i]->nb_antennas_tx[CC_id];
        frame_parms[CC_id]->nb_antennas_rx      =  enb_properties->properties[i]->nb_antennas_rx[CC_id];
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

void *processing_thread(void *_)
{
  PHY_VARS_eNB *phy = PHY_vars_eNB_g[0][0];
  int sched_frame = 0;
  int sched_subframe = 0;
  openair0_timestamp timestamp;
  int spp;
  int rxs;
  void *rxp[2];
  void *txp[2];

  spp = phy->lte_frame_parms.samples_per_tti; //openair0_cfg[0].samples_per_packet;

loop:
  T(T_ENB_MASTER_TICK, T_INT(0), T_INT(sched_frame), T_INT(sched_subframe));

  /* received subframe n */
  rxp[0] = &rxdata[0][spp * sched_subframe];
  rxs = openair0.trx_read_func(&openair0,
                               &timestamp,
                               rxp,
                               spp,
                               1);
  if (rxs != spp) printf("WARN: received %d samples out of %d wanted\n", rxs, spp);

  /* transmit subframe n+3 */
  txp[0] = &txdata[0][spp * ((sched_subframe + 3) % 10)];
  openair0.trx_write_func(&openair0,
                          timestamp + spp * 3 -openair0_cfg[0].tx_sample_advance,
                          txp,
                          spp,
                          1,
                          1);

  /* process received subframe n */
  phy->proc[sched_subframe].subframe = sched_subframe;
  phy->proc[sched_subframe].frame_rx = sched_frame;
  phy->proc[sched_subframe].subframe_rx = sched_subframe;
  phy_procedures_eNB_RX(sched_subframe, PHY_vars_eNB_g[0][0], 0, no_relay);

  /* generate subframe n+4 */
  phy->proc[sched_subframe].subframe = (sched_subframe+4) % 10;
  phy->proc[sched_subframe].frame_tx = sched_frame + ((sched_subframe >= 6) ? 1 : 0);
  phy->proc[sched_subframe].subframe_tx = (sched_subframe+4) % 10;
  phy_procedures_eNB_TX(sched_subframe, PHY_vars_eNB_g[0][0], 0, no_relay, NULL);
  do_OFDM_mod_rt(phy->proc[sched_subframe].subframe_tx, PHY_vars_eNB_g[0][0]);

  sched_subframe++;
  if (sched_subframe == 10) {
    sched_subframe = 0;
    sched_frame++;
    sched_frame %= 1024;
  }
  if (!oai_exit) goto loop;

  return 0;
}

static void new_thread(void *(*f)(void *), void *data)
{
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att))
    { fprintf(stderr, "pthread_attr_init err\n"); exit(1); }
  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED))
    { fprintf(stderr, "pthread_attr_setdetachstate err\n"); exit(1); }
  if (pthread_attr_setstacksize(&att, 10000000))
    { fprintf(stderr, "pthread_attr_setstacksize err\n"); exit(1); }
  if (pthread_create(&t, &att, f, data))
    { fprintf(stderr, "pthread_create err\n"); exit(1); }
  if (pthread_attr_destroy(&att))
    { fprintf(stderr, "pthread_attr_destroy err\n"); exit(1); }
}

void new_threading(void)
{
  new_thread(processing_thread, 0);
}

int main( int argc, char **argv )
{
  int i,aa,card=0;
#if defined (XFORMS) || defined (EMOS) || defined (EXMIMO)
  void *status;
#endif

  int CC_id;
  uint16_t Nid_cell = 0;
  uint8_t  cooperation_flag=0,  abstraction_flag=0;
  uint8_t beta_ACK=0,beta_RI=0,beta_CQI=2;

#ifdef ENABLE_TCXO
  unsigned int tcxo = 114;
#endif

#if defined (XFORMS)
  int ret;
#endif
#if defined (EMOS) || (! defined (RTAI))
  int error_code;
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
    frame_parms[CC_id]->Nid_cell            = Nid_cell;
    frame_parms[CC_id]->num_MBSFN_config    = 0;
    frame_parms[CC_id]->nb_antennas_tx_eNB  = 1;
    frame_parms[CC_id]->nb_antennas_tx      = 1;
    frame_parms[CC_id]->nb_antennas_rx      = 1;
  }

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    downlink_frequency[CC_id][0] = 2680000000; // Use float to avoid issue with frequency over 2^31.
    downlink_frequency[CC_id][1] = downlink_frequency[CC_id][0];
    downlink_frequency[CC_id][2] = downlink_frequency[CC_id][0];
    downlink_frequency[CC_id][3] = downlink_frequency[CC_id][0];
    //printf("Downlink for CC_id %d frequency set to %u\n", CC_id, downlink_frequency[CC_id][0]);
  }
  logInit();
 
  rf_config_file[0]='\0';
  get_options (argc, argv); //Command-line options
  if (rf_config_file[0] == '\0')
    openair0_cfg[0].configFilename = NULL;
  else
    openair0_cfg[0].configFilename = rf_config_file;
  
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

#ifndef RTAI
  check_clock();
#endif

  // init the parameters
  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    frame_parms[CC_id]->nushift            = 0;

    if (UE_flag==0) {

    } else {
      //UE_flag==1
      frame_parms[CC_id]->nb_antennas_tx     = 1;
      frame_parms[CC_id]->nb_antennas_rx     = 1;
      frame_parms[CC_id]->nb_antennas_tx_eNB = (transmission_mode == 1) ? 1 : 2; //initial value overwritten by initial sync later
    }

    frame_parms[CC_id]->mode1_flag         = (transmission_mode == 1) ? 1 : 0;
    frame_parms[CC_id]->phich_config_common.phich_resource = oneSixth;
    frame_parms[CC_id]->phich_config_common.phich_duration = normal;
    // UL RS Config
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift = 0;//n_DMRS1 set to 0
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled = 0;
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = 0;
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH = 0;
    frame_parms[CC_id]->threequarter_fs = threequarter_fs;
    init_ul_hopping(frame_parms[CC_id]);
    init_frame_parms(frame_parms[CC_id],1);
    //   phy_init_top(frame_parms[CC_id]);
    phy_init_lte_top(frame_parms[CC_id]);
  }


  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    //init prach for openair1 test
    frame_parms[CC_id]->prach_config_common.rootSequenceIndex=22;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig=1;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.prach_ConfigIndex=0;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.highSpeedFlag=0;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.prach_FreqOffset=0;
    // prach_fmt = get_prach_fmt(frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex, frame_parms->frame_type);
    // N_ZC = (prach_fmt <4)?839:139;
  }

  if (UE_flag==1) {
    NB_UE_INST=1;
    NB_INST=1;

    PHY_vars_UE_g = malloc(sizeof(PHY_VARS_UE**));
    PHY_vars_UE_g[0] = malloc(sizeof(PHY_VARS_UE*)*MAX_NUM_CCs);

    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

      PHY_vars_UE_g[0][CC_id] = init_lte_UE(frame_parms[CC_id], 0,abstraction_flag,transmission_mode);
      UE[CC_id] = PHY_vars_UE_g[0][CC_id];
      printf("PHY_vars_UE_g[0][%d] = %p\n",CC_id,UE[CC_id]);

      if (phy_test==1)
	UE[CC_id]->mac_enabled = 0;
      else 
	UE[CC_id]->mac_enabled = 1;

      if (UE[CC_id]->mac_enabled == 0) {
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

      compute_prach_seq(&UE[CC_id]->lte_frame_parms.prach_config_common,
                        UE[CC_id]->lte_frame_parms.frame_type,
                        UE[CC_id]->X_u);

      if (UE[CC_id]->mac_enabled == 1) 
	UE[CC_id]->lte_ue_pdcch_vars[0]->crnti = 0x1234;
      else
	UE[CC_id]->lte_ue_pdcch_vars[0]->crnti = 0x1235;

#ifdef EXMIMO
      for (i=0; i<4; i++) {
        UE[CC_id]->rx_gain_max[i] = rxg_max[i];
        UE[CC_id]->rx_gain_med[i] = rxg_med[i];
        UE[CC_id]->rx_gain_byp[i] = rxg_byp[i];
      }

      if ((UE[0]->mode == normal_txrx) ||
          (UE[0]->mode == rx_calib_ue) ||
          (UE[0]->mode == no_L2_connect) ||
          (UE[0]->mode == debug_prach)) {
        for (i=0; i<4; i++)
          rx_gain_mode[CC_id][i] = max_gain;

        UE[CC_id]->rx_total_gain_dB =  UE[CC_id]->rx_gain_max[0] + (int)rx_gain[CC_id][0] - 30; //-30 because it was calibrated with a 30dB gain
      } else if ((mode == rx_calib_ue_med)) {
        for (i=0; i<4; i++)
          rx_gain_mode[CC_id][i] =  med_gain;

        UE[CC_id]->rx_total_gain_dB =  UE[CC_id]->rx_gain_med[0]  + (int)rx_gain[CC_id][0] - 30; //-30 because it was calibrated with a 30dB gain;
      } else if ((mode == rx_calib_ue_byp)) {
        for (i=0; i<4; i++)
          rx_gain_mode[CC_id][i] =  byp_gain;

        UE[CC_id]->rx_total_gain_dB =  UE[CC_id]->rx_gain_byp[0]  + (int)rx_gain[CC_id][0] - 30; //-30 because it was calibrated with a 30dB gain;
      }

#else
      UE[CC_id]->rx_total_gain_dB =  (int)rx_gain[CC_id][0];
#endif

      UE[CC_id]->tx_power_max_dBm = tx_max_power[CC_id];


#ifdef EXMIMO
      //N_TA_offset
      if (UE[CC_id]->lte_frame_parms.frame_type == TDD) {
        if (UE[CC_id]->lte_frame_parms.N_RB_DL == 100)
          UE[CC_id]->N_TA_offset = 624;
        else if (UE[CC_id]->lte_frame_parms.N_RB_DL == 50)
          UE[CC_id]->N_TA_offset = 624/2;
        else if (UE[CC_id]->lte_frame_parms.N_RB_DL == 25)
          UE[CC_id]->N_TA_offset = 624/4;
      } else {
        UE[CC_id]->N_TA_offset = 0;
      }
#else
      //already taken care of in lte-softmodem
      UE[CC_id]->N_TA_offset = 0;
#endif
    }

    openair_daq_vars.manual_timing_advance = 0;
    openair_daq_vars.rx_gain_mode = DAQ_AGC_ON;
    openair_daq_vars.auto_freq_correction = 0;
    openair_daq_vars.use_ia_receiver = 0;



    //  printf("tx_max_power = %d -> amp %d\n",tx_max_power,get_tx_amp(tx_max_power,tx_max_power));
  } else {
    //this is eNB
    PHY_vars_eNB_g = malloc(sizeof(PHY_VARS_eNB**));
    PHY_vars_eNB_g[0] = malloc(sizeof(PHY_VARS_eNB*));

    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      PHY_vars_eNB_g[0][CC_id] = init_lte_eNB(frame_parms[CC_id],0,frame_parms[CC_id]->Nid_cell,cooperation_flag,transmission_mode,abstraction_flag);
      PHY_vars_eNB_g[0][CC_id]->CC_id = CC_id;

      if (phy_test==1)
	PHY_vars_eNB_g[0][CC_id]->mac_enabled = 0;
      else 
	PHY_vars_eNB_g[0][CC_id]->mac_enabled = 1;

      if (PHY_vars_eNB_g[0][CC_id]->mac_enabled == 0) {
	for (i=0; i<NUMBER_OF_UE_MAX; i++) {
	  PHY_vars_eNB_g[0][CC_id]->pusch_config_dedicated[i].betaOffset_ACK_Index = beta_ACK;
	  PHY_vars_eNB_g[0][CC_id]->pusch_config_dedicated[i].betaOffset_RI_Index  = beta_RI;
	  PHY_vars_eNB_g[0][CC_id]->pusch_config_dedicated[i].betaOffset_CQI_Index = beta_CQI;
	  
	  PHY_vars_eNB_g[0][CC_id]->scheduling_request_config[i].sr_PUCCH_ResourceIndex = i;
	  PHY_vars_eNB_g[0][CC_id]->scheduling_request_config[i].sr_ConfigIndex = 7+(i%3);
	  PHY_vars_eNB_g[0][CC_id]->scheduling_request_config[i].dsr_TransMax = sr_n4;
	}
      }

      compute_prach_seq(&PHY_vars_eNB_g[0][CC_id]->lte_frame_parms.prach_config_common,
                        PHY_vars_eNB_g[0][CC_id]->lte_frame_parms.frame_type,
                        PHY_vars_eNB_g[0][CC_id]->X_u);

#ifndef EXMIMO

      PHY_vars_eNB_g[0][CC_id]->rx_total_gain_eNB_dB = (int)rx_gain[CC_id][0];

#else
      PHY_vars_eNB_g[0][CC_id]->rx_total_gain_eNB_dB =  rxg_max[0] + (int)rx_gain[CC_id][0] - 30; //was measured at rxgain=30;

      printf("Setting RX total gain to %d\n",PHY_vars_eNB_g[0][CC_id]->rx_total_gain_eNB_dB);

      // set eNB to max gain
      for (i=0; i<4; i++)
        rx_gain_mode[CC_id][i] = max_gain;

#endif

#ifdef EXMIMO

      //N_TA_offset
      if (PHY_vars_eNB_g[0][CC_id]->lte_frame_parms.frame_type == TDD) {
        if (PHY_vars_eNB_g[0][CC_id]->lte_frame_parms.N_RB_DL == 100)
          PHY_vars_eNB_g[0][CC_id]->N_TA_offset = 624;
        else if (PHY_vars_eNB_g[0][CC_id]->lte_frame_parms.N_RB_DL == 50)
          PHY_vars_eNB_g[0][CC_id]->N_TA_offset = 624/2;
        else if (PHY_vars_eNB_g[0][CC_id]->lte_frame_parms.N_RB_DL == 25)
          PHY_vars_eNB_g[0][CC_id]->N_TA_offset = 624/4;
      } else {
        PHY_vars_eNB_g[0][CC_id]->N_TA_offset = 0;
      }

#else
      //already taken care of in lte-softmodem.c
      PHY_vars_eNB_g[0][CC_id]->N_TA_offset = 0;
#endif

    }


    NB_eNB_INST=1;
    NB_INST=1;

    openair_daq_vars.ue_dl_rb_alloc=0x1fff;
    openair_daq_vars.target_ue_dl_mcs=target_dl_mcs;
    openair_daq_vars.ue_ul_nb_rb=6;
    openair_daq_vars.target_ue_ul_mcs=target_ul_mcs;

  }
#ifndef RTAI
  fill_modeled_runtime_table(runtime_phy_rx,runtime_phy_tx);
  cpuf=get_cpu_freq_GHz();
#endif 

  dump_frame_parms(frame_parms[0]);

  for (card=0; card<MAX_CARDS; card++) {

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
      openair0_cfg[card].remote_addr    = eth_params->remote_addr;
      openair0_cfg[card].remote_port    = eth_params->remote_port;
      openair0_cfg[card].my_addr        = eth_params->my_addr;
      openair0_cfg[card].my_port        = eth_params->my_port;    
    }

    printf("HW: Configuring card %d, nb_antennas_tx/rx %d/%d\n",card,
           ((UE_flag==0) ? PHY_vars_eNB_g[0][0]->lte_frame_parms.nb_antennas_tx : PHY_vars_UE_g[0][0]->lte_frame_parms.nb_antennas_tx),
           ((UE_flag==0) ? PHY_vars_eNB_g[0][0]->lte_frame_parms.nb_antennas_rx : PHY_vars_UE_g[0][0]->lte_frame_parms.nb_antennas_rx));
    openair0_cfg[card].Mod_id = 0;
#ifdef ETHERNET

    if (UE_flag) {
      printf("ETHERNET: Configuring UE ETH for %s:%d\n",rrh_UE_ip,rrh_UE_port);
      openair0_cfg[card].remote_addr   = &rrh_UE_ip[0];
      openair0_cfg[card].remote_port = rrh_UE_port;
    } 

    openair0_cfg[card].num_rb_dl=frame_parms[0]->N_RB_DL;
#endif

    // in the case of the USRP, the following variables need to be initialized before the init
    // since the USRP only supports one CC (for the moment), we initialize all the cards with first CC.
    // in the case of EXMIMO2, these values are overwirtten in the function setup_eNB/UE_buffer
#ifndef EXMIMO
    openair0_cfg[card].tx_num_channels=min(2,((UE_flag==0) ? PHY_vars_eNB_g[0][0]->lte_frame_parms.nb_antennas_tx : PHY_vars_UE_g[0][0]->lte_frame_parms.nb_antennas_tx));
    openair0_cfg[card].rx_num_channels=min(2,((UE_flag==0) ? PHY_vars_eNB_g[0][0]->lte_frame_parms.nb_antennas_rx : PHY_vars_UE_g[0][0]->lte_frame_parms.nb_antennas_rx));

    for (i=0; i<4; i++) {

      openair0_cfg[card].tx_freq[i] = (UE_flag==0) ? downlink_frequency[0][i] : downlink_frequency[0][i]+uplink_frequency_offset[0][i];
      openair0_cfg[card].rx_freq[i] = (UE_flag==0) ? downlink_frequency[0][i] + uplink_frequency_offset[0][i] : downlink_frequency[0][i];
      printf("Card %d, channel %d, Setting tx_gain %f, rx_gain %f, tx_freq %f, rx_freq %f\n",
             card,i, openair0_cfg[card].tx_gain[i],
             openair0_cfg[card].rx_gain[i],
             openair0_cfg[card].tx_freq[i],
             openair0_cfg[card].rx_freq[i]);
      
      openair0_cfg[card].autocal[i] = 1;
      openair0_cfg[card].tx_gain[i] = tx_gain[0][i];
      if (UE_flag == 0) {
	openair0_cfg[card].rx_gain[i] = PHY_vars_eNB_g[0][0]->rx_total_gain_eNB_dB;
      }
      else {
	openair0_cfg[card].rx_gain[i] = PHY_vars_UE_g[0][0]->rx_total_gain_dB;
      }

#if 0  // UHD 3.8     
      switch(frame_parms[0]->N_RB_DL) {
      case 6:
        openair0_cfg[card].rx_gain[i] -= 12;
        break;

      case 25:
        openair0_cfg[card].rx_gain[i] -= 6;
        break;

      case 50:
        openair0_cfg[card].rx_gain[i] -= 3;
        break;

      case 100:
        openair0_cfg[card].rx_gain[i] -= 0;
        break;

      default:
        break;
      }
#endif      

    }

#endif
  }

#ifndef DEADLINE_SCHEDULER

  /* Currently we set affinity for UHD to CPU 0 for eNB/UE and only if number of CPUS >2 */
  
  cpu_set_t cpuset;
  int s;
  char cpu_affinity[1024];
  CPU_ZERO(&cpuset);
#if 0
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

  /* device host type is set*/
  openair0.host_type = BBU_HOST;
  /* device type is initialized NONE_DEV (no RF device) when the RF device will be initiated device type will be set */
  openair0.type = NONE_DEV;
  /* transport type is initialized NONE_TP (no transport protocol) when the transport protocol will be initiated transport protocol type will be set */
  openair0.transp_type = NONE_TP;
  openair0_cfg[0].log_level = glog_level;

  int returns=-1;
  /* BBU can have either a local or a remote radio head */  
  if (local_remote_radio == BBU_LOCAL_RADIO_HEAD) { //local radio head active  - load library of radio head and initiate it
    if (mode!=loop_through_memory) {
      returns=openair0_device_load(&openair0, &openair0_cfg[0]);
      printf("openair0_device_init returns %d\n",returns);
      if (returns<0) {
	printf("Exiting, cannot initialize device\n");
	exit(-1);
      }
    }
    else if (mode==loop_through_memory) {    
    }
  }  else { //remote radio head active - load library of transport protocol and initiate it 
    if (mode!=loop_through_memory) {
      returns=openair0_transport_load(&openair0, &openair0_cfg[0], eth_params);
      printf("openair0_transport_init returns %d\n",returns);
      if (returns<0) { 
	printf("Exiting, cannot initialize transport protocol\n");
	exit(-1);
      }
    }
    else if (mode==loop_through_memory) {    
    }
  }   
  
  printf("Done\n");

  mac_xface = malloc(sizeof(MAC_xface));

  int eMBMS_active=0;
  
  l2_init(frame_parms[0],eMBMS_active,(uecap_xer_in==1)?uecap_xer:NULL,
	  0,// cba_group_active
	  0); // HO flag
  
  mac_xface->macphy_exit = &exit_fun;

#if defined(ENABLE_ITTI)

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
    } else
      mac_xface->mrbch_phy_sync_failure (0, 0, 0);
  }

  /* #ifdef OPENAIR2
  //if (otg_enabled) {
  init_all_otg(0);
  g_otg->seed = 0;
  init_seeds(g_otg->seed);
  g_otg->num_nodes = 2;
  for (i=0; i<g_otg->num_nodes; i++){
  for (j=0; j<g_otg->num_nodes; j++){
  g_otg->application_idx[i][j] = 1;
  //g_otg->packet_gen_type=SUBSTRACT_STRING;
  g_otg->aggregation_level[i][j][0]=1;
  g_otg->application_type[i][j][0] = BCBR; //MCBR, BCBR
  }
  }
  init_predef_traffic(UE_flag ? 1 : 0, UE_flag ? 0 : 1);
  //  }
  #endif */

#ifdef EXMIMO
  number_of_cards = openair0_num_detected_cards;
#else
  number_of_cards = 1;
#endif



  for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    rf_map[CC_id].card=0;
    rf_map[CC_id].chain=CC_id+chain_offset;
  }

  // connect the TX/RX buffers
  if (UE_flag==1) {
#ifdef OAI_USRP
    openair_daq_vars.timing_advance = timing_advance;
#else
    openair_daq_vars.timing_advance = 160;
#endif
    if (setup_ue_buffers(UE,&openair0_cfg[0],rf_map)!=0) {
      printf("Error setting up eNB buffer\n");
      exit(-1);
    }

    printf("Setting UE buffer to all-RX\n");

    // Set LSBs for antenna switch (ExpressMIMO)
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      for (i=0; i<frame_parms[CC_id]->samples_per_tti*10; i++)
        for (aa=0; aa<frame_parms[CC_id]->nb_antennas_tx; aa++)
          UE[CC_id]->lte_ue_common_vars.txdata[aa][i] = 0x00010001;
    }

    if (input_fd) {
      printf("Reading in from file to antenna buffer %d\n",0);
      if (fread(UE[0]->lte_ue_common_vars.rxdata[0],
	        sizeof(int32_t),
	        frame_parms[0]->samples_per_tti*10,
	        input_fd) != frame_parms[0]->samples_per_tti*10)
        printf("error reading from file\n");
    }
    //p_exmimo_config->framing.tdd_config = TXRXSWITCH_TESTRX;
  } else {
    openair_daq_vars.timing_advance = 0;

    if (setup_eNB_buffers(PHY_vars_eNB_g[0],&openair0_cfg[0],rf_map)!=0) {
      printf("Error setting up eNB buffer\n");
      exit(-1);
    }

    printf("Setting eNB buffer to all-RX\n");

    // Set LSBs for antenna switch (ExpressMIMO)
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      for (i=0; i<frame_parms[CC_id]->samples_per_tti*10; i++)
        for (aa=0; aa<frame_parms[CC_id]->nb_antennas_tx; aa++)
          PHY_vars_eNB_g[0][CC_id]->lte_eNB_common_vars.txdata[0][aa][i] = 0x00010001;
    }
  }

#ifdef EXMIMO
  openair0_config(&openair0_cfg[0],UE_flag);
#endif

  /*
      for (ant=0;ant<4;ant++)
      p_exmimo_config->rf.do_autocal[ant] = 0;
  */

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
  /* task = */ rt_task_init_schmod(nam2num("MAIN"), 9, 0, 0, SCHED_FIFO, 0xF);

  // start realtime timer and scheduler
  //rt_set_oneshot_mode();
  rt_set_periodic_mode();
  start_rt_timer(0);
#endif

  pthread_cond_init(&sync_cond,NULL);
  pthread_mutex_init(&sync_mutex, NULL);

  // this starts the DMA transfers
#ifdef EXMIMO

  if (UE_flag!=1)
    for (card=0; card<openair0_num_detected_cards; card++)
      openair0_start_rt_acquisition(card);

#endif

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
	}
      }
    } else {
      form_stats = create_form_stats_form();
      fl_show_form (form_stats->stats_form, FL_PLACE_HOTSPOT, FL_FULLBORDER, "stats");
      UE_id = 0;
      form_ue[UE_id] = create_lte_phy_scope_ue();
      sprintf (title, "LTE DL SCOPE UE");
      fl_show_form (form_ue[UE_id]->lte_phy_scope_ue, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);

      if (openair_daq_vars.use_ia_receiver) {
        fl_set_button(form_ue[UE_id]->button_0,1);
        fl_set_object_label(form_ue[UE_id]->button_0, "IA Receiver ON");
      } else {
        fl_set_button(form_ue[UE_id]->button_0,0);
        fl_set_object_label(form_ue[UE_id]->button_0, "IA Receiver OFF");
      }
    }

    ret = pthread_create(&forms_thread, NULL, scope_thread, NULL);

    if (ret == 0)
      pthread_setname_np( forms_thread, "xforms" );

    printf("Scope thread created, ret=%d\n",ret);
  }

#endif

#ifdef EMOS
  ret = pthread_create(&thread3, NULL, emos_thread, NULL);
  printf("EMOS thread created, ret=%d\n",ret);
  ret = pthread_create(&thread4, NULL, gps_thread, NULL);
  printf("GPS thread created, ret=%d\n",ret);
#endif

  rt_sleep_ns(10*FRAME_PERIOD);

#ifndef RTAI
  pthread_attr_init (&attr_dlsch_threads);
  pthread_attr_setstacksize(&attr_dlsch_threads,4*PTHREAD_STACK_MIN);

  pthread_attr_init (&attr_UE_thread);
  pthread_attr_setstacksize(&attr_UE_thread,8192);//5*PTHREAD_STACK_MIN);

#ifndef DEADLINE_SCHEDULER
#if 0
  sched_param_UE_thread.sched_priority = sched_get_priority_max(SCHED_FIFO);
  pthread_attr_setschedparam(&attr_UE_thread,&sched_param_UE_thread);
  sched_param_dlsch.sched_priority = sched_get_priority_max(SCHED_FIFO); //OPENAIR_THREAD_PRIORITY;
  pthread_attr_setschedparam  (&attr_dlsch_threads, &sched_param_dlsch);
  pthread_attr_setschedpolicy (&attr_dlsch_threads, SCHED_FIFO);
  printf("Setting eNB_thread FIFO scheduling policy with priority %d \n", sched_param_dlsch.sched_priority);
#endif
#endif

#endif

  // start the main thread
  if (UE_flag == 1) {
    printf("Intializing UE Threads ...\n");
    init_UE_threads();
#ifdef DLSCH_THREAD
    init_rx_pdsch_thread();
    rt_sleep_ns(FRAME_PERIOD/10);
    init_dlsch_threads();
#endif

    sleep(1);
#ifdef RTAI
    main_ue_thread = rt_thread_create(UE_thread, NULL, 100000000);
#else
    error_code = pthread_create(&main_ue_thread, &attr_UE_thread, UE_thread, NULL);

    if (error_code!= 0) {
      LOG_D(HW,"[lte-softmodem.c] Could not allocate UE_thread, error %d\n",error_code);
      return(error_code);
    } else {
      LOG_D( HW, "[lte-softmodem.c] Allocate UE_thread successful\n" );
      pthread_setname_np( main_ue_thread, "main UE" );
    }

#endif
    printf("UE threads created\n");
#ifdef USE_MME

    while (start_UE == 0) {
      sleep(1);
    }

#endif



  } else {
    if (multi_thread>0) {
      init_eNB_proc();
      sleep(1);
      LOG_D(HW,"[lte-softmodem.c] eNB threads created\n");
    }

    printf("Creating main eNB_thread \n");
#ifdef RTAI
    main_eNB_thread = rt_thread_create(eNB_thread, NULL, PTHREAD_STACK_MIN);
#else
    error_code = pthread_create( &main_eNB_thread, &attr_dlsch_threads, eNB_thread, NULL );

    if (error_code!= 0) {
      LOG_D(HW,"[lte-softmodem.c] Could not allocate eNB_thread, error %d\n",error_code);
      return(error_code);
    } else {
      LOG_D( HW, "[lte-softmodem.c] Allocate eNB_thread successful\n" );
      pthread_setname_np( main_eNB_thread, "main eNB" );
    }

#endif
  }

  new_threading();

  // Sleep to allow all threads to setup
  sleep(1);



#ifndef EXMIMO

#ifndef USRP_DEBUG
  if (mode!=loop_through_memory)
    if (openair0.trx_start_func(&openair0) != 0 ) 
      LOG_E(HW,"Could not start the device\n");

#endif

#endif

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
    rt_sleep_ns(FRAME_PERIOD);

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
#ifdef EXMIMO
#ifdef RTAI
    rt_thread_join(main_ue_thread);
#else
    pthread_join(main_ue_thread,&status);
#endif
#ifdef DLSCH_THREAD
    cleanup_dlsch_threads();
    cleanup_rx_pdsch_thread();
#endif
#endif
  } else {
#ifdef DEBUG_THREADS
    printf("Joining eNB_thread ...");
#endif
#ifdef RTAI
    rt_thread_join(main_eNB_thread);
#else
    int *eNB_thread_status_p;
    int result = pthread_join( main_eNB_thread, (void**)&eNB_thread_status_p );
#ifdef DEBUG_THREADS

    if (result != 0) {
      printf( "\nError joining main_eNB_thread.\n" );
    } else {
      if (eNB_thread_status_p) {
        printf( "status %d\n", *eNB_thread_status_p );
      } else {
        printf( "The thread was killed. No status available.\n");
      }
    }

#else
    UNUSED(result);
#endif // DEBUG_THREADS
#endif // RTAI

    if (multi_thread>0) {
      printf("Killing eNB processing threads\n");
      kill_eNB_proc();
    }
  }


#ifdef RTAI
  stop_rt_timer();
#endif
  pthread_cond_destroy(&sync_cond);
  pthread_mutex_destroy(&sync_mutex);


#ifdef EXMIMO
  printf("stopping card\n");
  openair0_stop(0);
  printf("closing openair0_lib\n");
  openair0_close();
#else
  openair0.trx_end_func(&openair0);
#endif

#ifdef EMOS
  printf("waiting for EMOS thread\n");
  pthread_cancel(thread3);
  pthread_join(thread3,&status);
  printf("waiting for GPS thread\n");
  pthread_cancel(thread4);
  pthread_join(thread4,&status);
#endif

#ifdef EMOS
  error_code = rtf_destroy(CHANSOUNDER_FIFO_MINOR);
  printf("[OPENAIR][SCHED][CLEANUP] EMOS FIFO closed, error_code %d\n", error_code);
#endif

  if (ouput_vcd)
    VCD_SIGNAL_DUMPER_CLOSE();

  if (opt_enabled == 1)
    terminate_opt();

  logClean();

  return 0;
}


/* this function maps the phy_vars_eNB tx and rx buffers to the available rf chains.
   Each rf chain is is addressed by the card number and the chain on the card. The
   rf_map specifies for each CC, on which rf chain the mapping should start. Multiple
   antennas are mapped to successive RF chains on the same card. */
int setup_eNB_buffers(PHY_VARS_eNB **phy_vars_eNB, openair0_config_t *openair0_cfg, openair0_rf_map rf_map[MAX_NUM_CCs])
{

  int i, CC_id;
#ifndef EXMIMO
  uint16_t N_TA_offset = 0;
#else
  int j;
#endif
  LTE_DL_FRAME_PARMS *frame_parms;


  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    if (phy_vars_eNB[CC_id]) {
      frame_parms = &(phy_vars_eNB[CC_id]->lte_frame_parms);
      printf("setup_eNB_buffers: frame_parms = %p\n",frame_parms);
    } else {
      printf("phy_vars_eNB[%d] not initialized\n", CC_id);
      return(-1);
    }

#ifndef EXMIMO

    if (frame_parms->frame_type == TDD) {
      if (frame_parms->N_RB_DL == 100)
        N_TA_offset = 624;
      else if (frame_parms->N_RB_DL == 50)
        N_TA_offset = 624/2;
      else if (frame_parms->N_RB_DL == 25)
        N_TA_offset = 624/4;
    }

#endif

    // replace RX signal buffers with mmaped HW versions
#ifdef EXMIMO
    openair0_cfg[CC_id].tx_num_channels = 0;
    openair0_cfg[CC_id].rx_num_channels = 0;

    for (i=0; i<frame_parms->nb_antennas_rx; i++) {
      printf("Mapping eNB CC_id %d, rx_ant %d, freq %u on card %d, chain %d\n",CC_id,i,downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i],rf_map[CC_id].card,rf_map[CC_id].chain+i);
      free(phy_vars_eNB[CC_id]->lte_eNB_common_vars.rxdata[0][i]);
      phy_vars_eNB[CC_id]->lte_eNB_common_vars.rxdata[0][i] = (int32_t*) openair0_exmimo_pci[rf_map[CC_id].card].adc_head[rf_map[CC_id].chain+i];

      if (openair0_cfg[rf_map[CC_id].card].rx_freq[rf_map[CC_id].chain+i]) {
        printf("Error with rf_map! A channel has already been allocated!\n");
        return(-1);
      } else {
        openair0_cfg[rf_map[CC_id].card].rx_freq[rf_map[CC_id].chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
        openair0_cfg[rf_map[CC_id].card].rx_gain[rf_map[CC_id].chain+i] = rx_gain[CC_id][i];
        openair0_cfg[rf_map[CC_id].card].rx_num_channels++;
      }

      printf("rxdata[%d] @ %p\n",i,phy_vars_eNB[CC_id]->lte_eNB_common_vars.rxdata[0][i]);

      for (j=0; j<16; j++) {
        printf("rxbuffer %d: %x\n",j,phy_vars_eNB[CC_id]->lte_eNB_common_vars.rxdata[0][i][j]);
        phy_vars_eNB[CC_id]->lte_eNB_common_vars.rxdata[0][i][j] = 16-j;
      }
    }

    for (i=0; i<frame_parms->nb_antennas_tx; i++) {
      printf("Mapping eNB CC_id %d, tx_ant %d, freq %u on card %d, chain %d\n",CC_id,i,downlink_frequency[CC_id][i],rf_map[CC_id].card,rf_map[CC_id].chain+i);
      free(phy_vars_eNB[CC_id]->lte_eNB_common_vars.txdata[0][i]);
      phy_vars_eNB[CC_id]->lte_eNB_common_vars.txdata[0][i] = (int32_t*) openair0_exmimo_pci[rf_map[CC_id].card].dac_head[rf_map[CC_id].chain+i];

      if (openair0_cfg[rf_map[CC_id].card].tx_freq[rf_map[CC_id].chain+i]) {
        printf("Error with rf_map! A channel has already been allocated!\n");
        return(-1);
      } else {
        openair0_cfg[rf_map[CC_id].card].tx_freq[rf_map[CC_id].chain+i] = downlink_frequency[CC_id][i];
        openair0_cfg[rf_map[CC_id].card].tx_gain[rf_map[CC_id].chain+i] = tx_gain[CC_id][i];
        openair0_cfg[rf_map[CC_id].card].tx_num_channels++;
      }

      printf("txdata[%d] @ %p\n",i,phy_vars_eNB[CC_id]->lte_eNB_common_vars.txdata[0][i]);

      for (j=0; j<16; j++) {
        printf("txbuffer %d: %x\n",j,phy_vars_eNB[CC_id]->lte_eNB_common_vars.txdata[0][i][j]);
        phy_vars_eNB[CC_id]->lte_eNB_common_vars.txdata[0][i][j] = 16-j;
      }
    }

#else // not EXMIMO
    rxdata = (int32_t**)malloc16(frame_parms->nb_antennas_rx*sizeof(int32_t*));
    txdata = (int32_t**)malloc16(frame_parms->nb_antennas_tx*sizeof(int32_t*));

    for (i=0; i<frame_parms->nb_antennas_rx; i++) {
      free(phy_vars_eNB[CC_id]->lte_eNB_common_vars.rxdata[0][i]);
      rxdata[i] = (int32_t*)(32 + malloc16(32+openair0_cfg[rf_map[CC_id].card].samples_per_frame*sizeof(int32_t))); // FIXME broken memory allocation
      phy_vars_eNB[CC_id]->lte_eNB_common_vars.rxdata[0][i] = rxdata[i]-N_TA_offset; // N_TA offset for TDD         FIXME! N_TA_offset > 16 => access of unallocated memory
      memset(rxdata[i], 0, openair0_cfg[rf_map[CC_id].card].samples_per_frame*sizeof(int32_t));
      printf("rxdata[%d] @ %p (%p) (N_TA_OFFSET %d)\n", i, phy_vars_eNB[CC_id]->lte_eNB_common_vars.rxdata[0][i],rxdata[i],N_TA_offset);
      
    }

    for (i=0; i<frame_parms->nb_antennas_tx; i++) {
      free(phy_vars_eNB[CC_id]->lte_eNB_common_vars.txdata[0][i]);
      txdata[i] = (int32_t*)(32 + malloc16(32 + openair0_cfg[rf_map[CC_id].card].samples_per_frame*sizeof(int32_t))); // FIXME broken memory allocation
      phy_vars_eNB[CC_id]->lte_eNB_common_vars.txdata[0][i] = txdata[i];
      memset(txdata[i],0, openair0_cfg[rf_map[CC_id].card].samples_per_frame*sizeof(int32_t));
      printf("txdata[%d] @ %p\n", i, phy_vars_eNB[CC_id]->lte_eNB_common_vars.txdata[0][i]);

    }

#endif
  }

  return(0);
}

void reset_opp_meas(void) {
  int sfn;
  reset_meas(&softmodem_stats_mt);
  reset_meas(&softmodem_stats_hw);
  
  for (sfn=0; sfn < 10; sfn++) {
    reset_meas(&softmodem_stats_tx_sf[sfn]);
    reset_meas(&softmodem_stats_rx_sf[sfn]);
  }
}

void print_opp_meas(void) {

  int sfn=0;
  print_meas(&softmodem_stats_mt, "Main ENB Thread", NULL, NULL);
  print_meas(&softmodem_stats_hw, "HW Acquisation", NULL, NULL);
  
  for (sfn=0; sfn < 10; sfn++) {
    print_meas(&softmodem_stats_tx_sf[sfn],"[eNB][total_phy_proc_tx]",NULL, NULL);
    print_meas(&softmodem_stats_rx_sf[sfn],"[eNB][total_phy_proc_rx]",NULL,NULL);
  }
}


