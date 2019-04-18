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

/*! \file lte-uesoftmodem.c
 * \brief Top-level threads for eNodeB
 * \author R. Knopp, F. Kaltenberger, Navid Nikaein
 * \date 2012
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
 * \note
 * \warning
 */


#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>

#include "rt_wrapper.h"


#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "assertions.h"
#include "msc.h"

#include "PHY/types.h"

#include "PHY/defs_UE.h"
#include "common/ran_context.h"
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "../../ARCH/COMMON/common_lib.h"
#include "../../ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"

//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/phy_vars_ue.h"
#include "PHY/LTE_TRANSPORT/transport_vars.h"
#include "SCHED/sched_common_vars.h"
#include "PHY/MODULATION/modulation_vars.h"

#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_vars.h"
#include "LAYER2/MAC/mac_proto.h"
#include "RRC/LTE/rrc_vars.h"
#include "PHY_INTERFACE/phy_interface_vars.h"

#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"

#ifndef OPENAIR2
  #include "UTIL/OTG/otg_vars.h"
#endif

#if defined(ENABLE_ITTI)
  #include "create_tasks.h"
#endif

#include "system.h"

#ifdef XFORMS
  #include "PHY/TOOLS/lte_phy_scope.h"
  #include "stats.h"
#endif
#include "lte-softmodem.h"



/* temporary compilation wokaround (UE/eNB split */
uint16_t sf_ahead;
#ifdef XFORMS
  // current status is that every UE has a DL scope for a SINGLE eNB (eNB_id=0)
  // at eNB 0, an UL scope for every UE
  FD_lte_phy_scope_ue  *form_ue[NUMBER_OF_UE_MAX];
  FD_lte_phy_scope_enb *form_enb[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
  FD_stats_form                  *form_stats=NULL,*form_stats_l2=NULL;
  char title[255];
  unsigned char                   scope_enb_num_ue = 2;
  static pthread_t                forms_thread; //xforms
#endif //XFORMS

pthread_cond_t nfapi_sync_cond;
pthread_mutex_t nfapi_sync_mutex;
int nfapi_sync_var=-1; //!< protected by mutex \ref nfapi_sync_mutex


#ifdef UESIM_EXPANSION
  uint16_t inst_pdcp_list[NUMBER_OF_UE_MAX];
#endif
uint16_t sf_ahead=2;
int tddflag;
char *emul_iface;


pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.
int config_sync_var=-1;

uint16_t runtime_phy_rx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]
uint16_t runtime_phy_tx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]

volatile int             oai_exit = 0;

clock_source_t clock_source = internal;


unsigned int                    mmapped_dma=0;


uint32_t                 downlink_frequency[MAX_NUM_CCs][4];
int32_t                  uplink_frequency_offset[MAX_NUM_CCs][4];



int UE_scan = 1;
int UE_scan_carrier = 0;

int snr_dB=25;

runmode_t mode = normal_txrx;

FILE *input_fd=NULL;


#if MAX_NUM_CCs == 1
rx_gain_t                rx_gain_mode[MAX_NUM_CCs][4] = {{max_gain,max_gain,max_gain,max_gain}};
double tx_gain[MAX_NUM_CCs][4] = {{20,0,0,0}};
double rx_gain[MAX_NUM_CCs][4] = {{130,0,0,0}};
#else
rx_gain_t                rx_gain_mode[MAX_NUM_CCs][4] = {{max_gain,max_gain,max_gain,max_gain},{max_gain,max_gain,max_gain,max_gain}};
double tx_gain[MAX_NUM_CCs][4] = {{20,0,0,0},{20,0,0,0}};
double rx_gain[MAX_NUM_CCs][4] = {{130,0,0,0},{20,0,0,0}};
#endif

double rx_gain_off = 0.0;

double sample_rate=30.72e6;
double bw = 10.0e6;

static int                      tx_max_power[MAX_NUM_CCs]; /* =  {0,0}*/;


uint8_t dci_Format = 0;
uint8_t agregation_Level =0xFF;

uint8_t nb_antenna_tx = 1;
uint8_t nb_antenna_rx = 1;

char ref[128] = "internal";
char channels[128] = "0";

int                      rx_input_level_dBm;

#ifdef XFORMS
  extern int                      otg_enabled;
#endif
//int                             number_of_cards =   1;


static LTE_DL_FRAME_PARMS      *frame_parms[MAX_NUM_CCs];

uint8_t exit_missed_slots=1;
uint64_t num_missed_slots=0; // counter for the number of missed slots

/* prototypes from function implemented in lte-ue.c, probably should be elsewhere in a include
   file */
extern void init_UE_stub_single_thread(int nb_inst,int eMBMS_active, int uecap_xer_in, char *emul_iface);

extern PHY_VARS_UE *init_ue_vars(LTE_DL_FRAME_PARMS *frame_parms,
                                 uint8_t UE_id,
                                 uint8_t abstraction_flag);

extern void get_uethreads_params(void);

int transmission_mode=1;


char *usrp_args=NULL;
char *usrp_clksrc=NULL;

THREAD_STRUCT thread_struct;
/* struct for ethernet specific parameters given in eNB conf file */
eth_params_t *eth_params;

openair0_config_t openair0_cfg[MAX_CARDS];

double cpuf;

extern char uecap_xer[1024];
char uecap_xer_in=0;

int oaisim_flag=0;


/* see file openair2/LAYER2/MAC/main.c for why abstraction_flag is needed
 * this is very hackish - find a proper solution
 */
uint8_t abstraction_flag=0;

/* forward declarations */
void set_default_frame_parms(LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs]);

/*---------------------BMC: timespec helpers -----------------------------*/

struct timespec min_diff_time = { .tv_sec = 0, .tv_nsec = 0 };
struct timespec max_diff_time = { .tv_sec = 0, .tv_nsec = 0 };

struct timespec clock_difftime(struct timespec start, struct timespec end) {
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

void print_difftimes(void) {
#ifdef DEBUG
  printf("difftimes min = %lu ns ; max = %lu ns\n", min_diff_time.tv_nsec, max_diff_time.tv_nsec);
#else
  LOG_I(HW,"difftimes min = %lu ns ; max = %lu ns\n", min_diff_time.tv_nsec, max_diff_time.tv_nsec);
#endif
}

void update_difftimes(struct timespec start, struct timespec end) {
  struct timespec diff_time = { .tv_sec = 0, .tv_nsec = 0 };
  int             changed = 0;
  diff_time = clock_difftime(start, end);

  if ((min_diff_time.tv_nsec == 0) || (diff_time.tv_nsec < min_diff_time.tv_nsec)) {
    min_diff_time.tv_nsec = diff_time.tv_nsec;
    changed = 1;
  }

  if ((max_diff_time.tv_nsec == 0) || (diff_time.tv_nsec > max_diff_time.tv_nsec)) {
    max_diff_time.tv_nsec = diff_time.tv_nsec;
    changed = 1;
  }

#if 1

  if (changed) print_difftimes();

#endif
}

/*------------------------------------------------------------------------*/

unsigned int build_rflocal(int txi, int txq, int rxi, int rxq) {
  return (txi + (txq<<6) + (rxi<<12) + (rxq<<18));
}
unsigned int build_rfdc(int dcoff_i_rxfe, int dcoff_q_rxfe) {
  return (dcoff_i_rxfe + (dcoff_q_rxfe<<8));
}



void signal_handler(int sig) {
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
    char msg[64];
    sprintf(msg,"Received linux signal %s...\n",strsignal(sig));
    exit_function(__FILE__, __FUNCTION__, __LINE__,msg);
  }
}


void exit_function(const char *file, const char *function, const int line, const char *s) {
  int CC_id;
  logClean();
  printf("%s:%d %s() Exiting OAI softmodem: %s\n",file,line, function, ((s==NULL)?"":s));
  oai_exit = 1;

  for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    if (PHY_vars_UE_g)
      if (PHY_vars_UE_g[0])
        if (PHY_vars_UE_g[0][CC_id])
          if (PHY_vars_UE_g[0][CC_id]->rfdevice.trx_end_func)
            PHY_vars_UE_g[0][CC_id]->rfdevice.trx_end_func(&PHY_vars_UE_g[0][CC_id]->rfdevice);
  }

  sleep(1); //allow lte-softmodem threads to exit first

  if(PHY_vars_UE_g != NULL )
    itti_terminate_tasks (TASK_UNKNOWN);

  exit(1);
}

#ifdef XFORMS


void reset_stats(FL_OBJECT *button, long arg) {
  int i,j,k;
  PHY_VARS_eNB *phy_vars_eNB = RC.eNB[0][0];

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
      phy_vars_eNB->UE_stats[i].dlsch_sliding_cnt=0;
      phy_vars_eNB->UE_stats[i].dlsch_NAK_round0=0;
      phy_vars_eNB->UE_stats[i].dlsch_mcs_offset=0;
    }
  }
}

static void *scope_thread(void *arg) {
  char stats_buffer[16384];
# ifdef ENABLE_XFORMS_WRITE_STATS
  FILE *UE_stats, *eNB_stats;
# endif
  struct sched_param sched_param;
  sched_param.sched_priority = sched_get_priority_min(SCHED_FIFO)+1;
  sched_setscheduler(0, SCHED_FIFO,&sched_param);
  printf("Scope thread has priority %d\n",sched_param.sched_priority);
# ifdef ENABLE_XFORMS_WRITE_STATS
  UE_stats  = fopen("UE_stats.txt", "w");
#endif

  while (!oai_exit) {
    //      dump_ue_stats (PHY_vars_UE_g[0][0], &PHY_vars_UE_g[0][0]->proc.proc_rxtx[0],stats_buffer, 0, mode,rx_input_level_dBm);
    //fl_set_object_label(form_stats->stats_text, stats_buffer);
    fl_clear_browser(form_stats->stats_text);
    fl_add_browser_line(form_stats->stats_text, stats_buffer);
    phy_scope_UE(form_ue[0],
                 PHY_vars_UE_g[0][0],
                 0,
                 0,7);
    //  printf("%s",stats_buffer);
  }

# ifdef ENABLE_XFORMS_WRITE_STATS

  if (UE_stats) {
    rewind (UE_stats);
    fwrite (stats_buffer, 1, len, UE_stats);
    fclose (UE_stats);
  }

# endif
  pthread_exit((void *)arg);
}
#endif



extern int16_t dlsch_demod_shift;

static void get_options(void) {
  int CC_id;
  int tddflag;
  char *loopfile=NULL;
  int dumpframe;
  int timingadv;
  uint8_t nfapi_mode;
  int simL1flag ;
  set_default_frame_parms(frame_parms);
  CONFIG_SETRTFLAG(CONFIG_NOEXITONHELP);
  /* unknown parameters on command line will be checked in main
     after all init have been performed                         */
  get_common_options();
  get_uethreads_params();
  paramdef_t cmdline_uemodeparams[] =CMDLINE_UEMODEPARAMS_DESC;
  paramdef_t cmdline_ueparams[] =CMDLINE_UEPARAMS_DESC;
  config_process_cmdline( cmdline_uemodeparams,sizeof(cmdline_uemodeparams)/sizeof(paramdef_t),NULL);
  config_process_cmdline( cmdline_ueparams,sizeof(cmdline_ueparams)/sizeof(paramdef_t),NULL);
  nfapi_setmode(nfapi_mode);

  if (simL1flag)
    set_softmodem_optmask(SOFTMODEM_SIML1_BIT);

  if (loopfile != NULL) {
    printf("Input file for hardware emulation: %s",loopfile);
    mode=loop_through_memory;
    input_fd = fopen(loopfile,"r");
    AssertFatal(input_fd != NULL,"Please provide a valid input file\n");
  }

  get_softmodem_params()->hw_timing_advance = timingadv;

  if ( (cmdline_uemodeparams[CMDLINE_CALIBUERX_IDX].paramflags &  PARAMFLAG_PARAMSET) != 0) mode = rx_calib_ue;

  if ( (cmdline_uemodeparams[CMDLINE_CALIBUERXMED_IDX].paramflags &  PARAMFLAG_PARAMSET) != 0) mode = rx_calib_ue_med;

  if ( (cmdline_uemodeparams[CMDLINE_CALIBUERXBYP_IDX].paramflags &  PARAMFLAG_PARAMSET) != 0) mode = rx_calib_ue_byp;

  if (cmdline_uemodeparams[CMDLINE_DEBUGUEPRACH_IDX].uptr)
    if ( *(cmdline_uemodeparams[CMDLINE_DEBUGUEPRACH_IDX].uptr) > 0) mode = debug_prach;

  if (cmdline_uemodeparams[CMDLINE_NOL2CONNECT_IDX].uptr)
    if ( *(cmdline_uemodeparams[CMDLINE_NOL2CONNECT_IDX].uptr) > 0)  mode = no_L2_connect;

  if (cmdline_uemodeparams[CMDLINE_CALIBPRACHTX_IDX].uptr)
    if ( *(cmdline_uemodeparams[CMDLINE_CALIBPRACHTX_IDX].uptr) > 0) mode = calib_prach_tx;

  if (dumpframe  > 0)  mode = rx_dump_frame;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    frame_parms[CC_id]->dl_CarrierFreq = downlink_frequency[0][0];
  }

  UE_scan=0;

  if (tddflag > 0) {
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      frame_parms[CC_id]->frame_type = TDD;
      frame_parms[CC_id]->tdd_config = tddflag;
    }
  }

  if (frame_parms[0]->N_RB_DL !=0) {
    if ( frame_parms[0]->N_RB_DL < 6 ) {
      frame_parms[0]->N_RB_DL = 6;
      printf ( "%i: Invalid number of ressource blocks, adjusted to 6\n",frame_parms[0]->N_RB_DL);
    }

    if ( frame_parms[0]->N_RB_DL > 100 ) {
      frame_parms[0]->N_RB_DL = 100;
      printf ( "%i: Invalid number of ressource blocks, adjusted to 100\n",frame_parms[0]->N_RB_DL);
    }

    if ( frame_parms[0]->N_RB_DL > 50 && frame_parms[0]->N_RB_DL < 100 ) {
      frame_parms[0]->N_RB_DL = 50;
      printf ( "%i: Invalid number of ressource blocks, adjusted to 50\n",frame_parms[0]->N_RB_DL);
    }

    if ( frame_parms[0]->N_RB_DL > 25 && frame_parms[0]->N_RB_DL < 50 ) {
      frame_parms[0]->N_RB_DL = 25;
      printf ( "%i: Invalid number of ressource blocks, adjusted to 25\n",frame_parms[0]->N_RB_DL);
    }

    UE_scan = 0;
    frame_parms[0]->N_RB_UL=frame_parms[0]->N_RB_DL;

    for (CC_id=1; CC_id<MAX_NUM_CCs; CC_id++) {
      frame_parms[CC_id]->N_RB_DL=frame_parms[0]->N_RB_DL;
      frame_parms[CC_id]->N_RB_UL=frame_parms[0]->N_RB_UL;
    }
  }

  for (CC_id=1; CC_id<MAX_NUM_CCs; CC_id++) {
    rx_gain[0][CC_id] = rx_gain[0][0];
    tx_gain[0][CC_id] = tx_gain[0][0];
  }
}


void set_default_frame_parms(LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs]) {
  int CC_id;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    frame_parms[CC_id] = (LTE_DL_FRAME_PARMS *) malloc(sizeof(LTE_DL_FRAME_PARMS));
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
    frame_parms[CC_id]->nb_antenna_ports_eNB  = 1;
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
    downlink_frequency[CC_id][0] = DEFAULT_DLF; // Use float to avoid issue with frequency over 2^31.
    downlink_frequency[CC_id][1] = downlink_frequency[CC_id][0];
    downlink_frequency[CC_id][2] = downlink_frequency[CC_id][0];
    downlink_frequency[CC_id][3] = downlink_frequency[CC_id][0];
    frame_parms[CC_id]->dl_CarrierFreq=downlink_frequency[CC_id][0];
  }
}

void init_openair0(LTE_DL_FRAME_PARMS *frame_parms,int rxgain) {
  int card;
  int i;

  for (card=0; card<MAX_CARDS; card++) {
    openair0_cfg[card].mmapped_dma=mmapped_dma;
    openair0_cfg[card].configFilename = NULL;

    if(frame_parms->N_RB_DL == 100) {
      if (frame_parms->threequarter_fs) {
        openair0_cfg[card].sample_rate=23.04e6;
        openair0_cfg[card].samples_per_frame = 230400;
        openair0_cfg[card].tx_bw = 10e6;
        openair0_cfg[card].rx_bw = 10e6;
      } else {
        openair0_cfg[card].sample_rate=30.72e6;
        openair0_cfg[card].samples_per_frame = 307200;
        openair0_cfg[card].tx_bw = 10e6;
        openair0_cfg[card].rx_bw = 10e6;
      }
    } else if(frame_parms->N_RB_DL == 50) {
      openair0_cfg[card].sample_rate=15.36e6;
      openair0_cfg[card].samples_per_frame = 153600;
      openair0_cfg[card].tx_bw = 5e6;
      openair0_cfg[card].rx_bw = 5e6;
    } else if (frame_parms->N_RB_DL == 25) {
      openair0_cfg[card].sample_rate=7.68e6;
      openair0_cfg[card].samples_per_frame = 76800;
      openair0_cfg[card].tx_bw = 2.5e6;
      openair0_cfg[card].rx_bw = 2.5e6;
    } else if (frame_parms->N_RB_DL == 6) {
      openair0_cfg[card].sample_rate=1.92e6;
      openair0_cfg[card].samples_per_frame = 19200;
      openair0_cfg[card].tx_bw = 1.5e6;
      openair0_cfg[card].rx_bw = 1.5e6;
    }

    if (frame_parms->frame_type==TDD)
      openair0_cfg[card].duplex_mode = duplex_mode_TDD;
    else //FDD
      openair0_cfg[card].duplex_mode = duplex_mode_FDD;

    openair0_cfg[card].Mod_id = 0;
    openair0_cfg[card].num_rb_dl=frame_parms->N_RB_DL;
    openair0_cfg[card].clock_source = clock_source;
    openair0_cfg[card].tx_num_channels=min(2,frame_parms->nb_antennas_tx);
    openair0_cfg[card].rx_num_channels=min(2,frame_parms->nb_antennas_rx);

    for (i=0; i<4; i++) {
      if (i<openair0_cfg[card].tx_num_channels)
        openair0_cfg[card].tx_freq[i] = downlink_frequency[0][i]+uplink_frequency_offset[0][i];
      else
        openair0_cfg[card].tx_freq[i]=0.0;

      if (i<openair0_cfg[card].rx_num_channels)
        openair0_cfg[card].rx_freq[i] = downlink_frequency[0][i];
      else
        openair0_cfg[card].rx_freq[i]=0.0;

      openair0_cfg[card].autocal[i] = 1;
      openair0_cfg[card].tx_gain[i] = tx_gain[0][i];
      openair0_cfg[card].rx_gain[i] = rxgain - rx_gain_off;
      openair0_cfg[card].configFilename = get_softmodem_params()->rf_config_file;
      printf("Card %d, channel %d, Setting tx_gain %f, rx_gain %f, tx_freq %f, rx_freq %f\n",
             card,i, openair0_cfg[card].tx_gain[i],
             openair0_cfg[card].rx_gain[i],
             openair0_cfg[card].tx_freq[i],
             openair0_cfg[card].rx_freq[i]);
    }

    if (usrp_args) openair0_cfg[card].sdr_addrs = usrp_args;

    if (usrp_clksrc) {
      if (strcmp(usrp_clksrc, "internal") == 0) {
        openair0_cfg[card].clock_source = internal;
        LOG_D(PHY, "USRP clock source set as internal\n");
      } else if (strcmp(usrp_clksrc, "external") == 0) {
        openair0_cfg[card].clock_source = external;
        LOG_D(PHY, "USRP clock source set as external\n");
      } else if (strcmp(usrp_clksrc, "gpsdo") == 0) {
        openair0_cfg[card].clock_source = gpsdo;
        LOG_D(PHY, "USRP clock source set as gpsdo\n");
      } else {
        openair0_cfg[card].clock_source = internal;
        LOG_I(PHY, "USRP clock source unknown ('%s'). defaulting to internal\n", usrp_clksrc);
      }
    } else {
      openair0_cfg[card].clock_source = internal;
      LOG_I(PHY, "USRP clock source not specified. defaulting to internal\n");
    }
  }
}



/* helper function to terminate a certain ITTI task
 */
void terminate_task(task_id_t task_id, module_id_t mod_id) {
  LOG_I(ENB_APP, "sending TERMINATE_MESSAGE to task %s (%d)\n", itti_get_task_name(task_id), task_id);
  MessageDef *msg;
  msg = itti_alloc_new_message (ENB_APP, TERMINATE_MESSAGE);
  itti_send_msg_to_task (task_id, ENB_MODULE_ID_TO_INSTANCE(mod_id), msg);
}


static inline void wait_nfapi_init(char *thread_name) {
  printf( "waiting for NFAPI PNF connection and population of global structure (%s)\n",thread_name);
  pthread_mutex_lock( &nfapi_sync_mutex );

  while (nfapi_sync_var<0)
    pthread_cond_wait( &nfapi_sync_cond, &nfapi_sync_mutex );

  pthread_mutex_unlock(&nfapi_sync_mutex);
  printf( "NFAPI: got sync (%s)\n", thread_name);
}

int stop_L1L2(module_id_t enb_id) {
  return 0;
}


int restart_L1L2(module_id_t enb_id) {
  return 0;
}

void init_pdcp(void) {
  uint32_t pdcp_initmask = (!IS_SOFTMODEM_NOS1) ? LINK_ENB_PDCP_TO_GTPV1U_BIT : (LINK_ENB_PDCP_TO_GTPV1U_BIT | PDCP_USE_NETLINK_BIT | LINK_ENB_PDCP_TO_IP_DRIVER_BIT);

  if (IS_SOFTMODEM_BASICSIM || IS_SOFTMODEM_RFSIM || (nfapi_getmode()==NFAPI_UE_STUB_PNF)) {
    pdcp_initmask = pdcp_initmask | UE_NAS_USE_TUN_BIT;
  }

  if (IS_SOFTMODEM_NOKRNMOD)
    pdcp_initmask = pdcp_initmask | UE_NAS_USE_TUN_BIT;

  pdcp_module_init(pdcp_initmask);
  pdcp_set_rlc_data_req_func((send_rlc_data_req_func_t) rlc_data_req);
  pdcp_set_pdcp_data_ind_func((pdcp_data_ind_func_t) pdcp_data_ind);
}

int main( int argc, char **argv ) {
#if defined (XFORMS)
  void *status;
#endif
  int CC_id;
  uint8_t  abstraction_flag=0;
#ifdef UESIM_EXPANSION
  memset(inst_pdcp_list, 0, sizeof(inst_pdcp_list));
#endif
  // Default value for the number of UEs. It will hold,
  // if not changed from the command line option --num-ues
  NB_UE_INST=1;
  NB_THREAD_INST=1;
#if defined (XFORMS)
  int ret;
#endif
  configmodule_interface_t *config_mod;
  start_background_system();
  config_mod = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY);

  if (config_mod == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }

  mode = normal_txrx;
  memset(&openair0_cfg[0],0,sizeof(openair0_config_t)*MAX_CARDS);
  set_latency_target();
  logInit();
  printf("Reading in command-line options\n");

  for (int i=0; i<MAX_NUM_CCs; i++) tx_max_power[i]=23;

  get_options ();

  if (is_nos1exec(argv[0]) )
    set_softmodem_optmask(SOFTMODEM_NOS1_BIT);

  EPC_MODE_ENABLED = !IS_SOFTMODEM_NOS1;
  printf("Running with %d UE instances\n",NB_UE_INST);

  if (NB_UE_INST > 1 && (!IS_SOFTMODEM_SIML1)  && NFAPI_MODE!=NFAPI_UE_STUB_PNF) {
    printf("Running with more than 1 UE instance and simL1 is not active, this will result in undefined behaviour for now, exiting.\n");
    abort();
  }

  // Checking option of nums_ue_thread.
  if(NB_THREAD_INST < 1) {
    printf("Running with 0 UE rxtx thread, exiting.\n");
    abort();
  }

  // Checking option's relation between nums_ue_thread and num-ues
  if(NB_UE_INST <NB_THREAD_INST ) {
    printf("Number of UEs < number of UE rxtx threads, exiting.\n");
    abort();
  }

#if T_TRACER
  T_Config_Init();
#endif
  //randominit (0);
  set_taus_seed (0);
  cpuf=get_cpu_freq_GHz();
  pthread_cond_init(&sync_cond,NULL);
  pthread_mutex_init(&sync_mutex, NULL);
  printf("ITTI init\n");
  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info);

  // initialize mscgen log after ITTI
  if (get_softmodem_params()->start_msc) {
    load_module_shlib("msc",NULL,0,&msc_interface);
  }

  MSC_INIT(MSC_E_UTRAN, THREAD_MAX+TASK_MAX);
  init_opt();

  init_pdcp();

  //TTN for D2D
  printf ("RRC control socket\n");
  rrc_control_socket_init();
  printf ("PDCP PC5S socket\n");
  pdcp_pc5_socket_init();
  // to make a graceful exit when ctrl-c is pressed
  signal(SIGSEGV, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGABRT, signal_handler);
  check_clock();
#ifndef PACKAGE_VERSION
#  define PACKAGE_VERSION "UNKNOWN-EXPERIMENTAL"
#endif
  LOG_I(HW, "Version: %s\n", PACKAGE_VERSION);

  // init the parameters
  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    frame_parms[CC_id]->nb_antennas_tx     = nb_antenna_tx;
    frame_parms[CC_id]->nb_antennas_rx     = nb_antenna_rx;
    frame_parms[CC_id]->nb_antenna_ports_eNB = 1; //initial value overwritten by initial sync later
  }

  NB_INST=1;

  if(NFAPI_MODE==NFAPI_UE_STUB_PNF) {
    PHY_vars_UE_g = malloc(sizeof(PHY_VARS_UE **)*NB_UE_INST);

    for (int i=0; i<NB_UE_INST; i++) {
      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        PHY_vars_UE_g[i] = malloc(sizeof(PHY_VARS_UE *)*MAX_NUM_CCs);
        PHY_vars_UE_g[i][CC_id] = init_ue_vars(frame_parms[CC_id], i,abstraction_flag);

        if (get_softmodem_params()->phy_test==1)
          PHY_vars_UE_g[i][CC_id]->mac_enabled = 0;
        else
          PHY_vars_UE_g[i][CC_id]->mac_enabled = 1;
      }
    }
  } else init_openair0(frame_parms[0],(int)rx_gain[0][0]);

  if (IS_SOFTMODEM_SIML1 ) {
    RCConfig_sim();
  }

  cpuf=get_cpu_freq_GHz();
#ifndef DEADLINE_SCHEDULER
  printf("NO deadline scheduler\n");
  /* Currently we set affinity for UHD to CPU 0 for eNB/UE and only if number of CPUS >2 */
  cpu_set_t cpuset;
  int s;
  char cpu_affinity[1024];
  CPU_ZERO(&cpuset);
#ifdef CPU_AFFINITY

  if (get_nprocs() > 2) {
    CPU_SET(0, &cpuset);
    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    if (s != 0) {
      perror( "pthread_setaffinity_np");
      exit_fun("Error setting processor affinity");
    }

    LOG_I(HW, "Setting the affinity of main function to CPU 0, for device library to use CPU 0 only!\n");
  }

#endif
  /* Check the actual affinity mask assigned to the thread */
  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

  if (s != 0) {
    perror( "pthread_getaffinity_np");
    exit_fun("Error getting processor affinity ");
  }

  memset(cpu_affinity, 0, sizeof(cpu_affinity));

  for (int j = 0; j < CPU_SETSIZE; j++) {
    if (CPU_ISSET(j, &cpuset)) {
      char temp[1024];
      sprintf(temp, " CPU_%d ", j);
      strcat(cpu_affinity, temp);
    }
  }

  LOG_I(HW, "CPU Affinity of main() function is... %s\n", cpu_affinity);
#endif

  if (create_tasks_ue(NB_UE_INST) < 0) {
    printf("cannot create ITTI tasks\n");
    exit(-1); // need a softer mode
  }

  if (NFAPI_MODE==NFAPI_UE_STUB_PNF) { // UE-STUB-PNF
    UE_config_stub_pnf();
  }

  printf("ITTI tasks created\n");
  mlockall(MCL_CURRENT | MCL_FUTURE);
  rt_sleep_ns(10*100000000ULL);
  int eMBMS_active = 0;

  if (NFAPI_MODE==NFAPI_UE_STUB_PNF) { // UE-STUB-PNF
    config_sync_var=0;
    wait_nfapi_init("main?");
    //Panos: Temporarily we will be using single set of threads for multiple UEs.
    //init_UE_stub(1,eMBMS_active,uecap_xer_in,emul_iface);
    init_UE_stub_single_thread(NB_UE_INST,eMBMS_active,uecap_xer_in,emul_iface);
  } else {
    init_UE(NB_UE_INST,eMBMS_active,uecap_xer_in,0,get_softmodem_params()->phy_test,UE_scan,UE_scan_carrier,mode,(int)rx_gain[0][0],tx_max_power[0],
            frame_parms[0]);
  }

  if (get_softmodem_params()->phy_test==0) {
    printf("Filling UE band info\n");
    fill_ue_band_info();
    dl_phy_sync_success (0, 0, 0, 1);
  }

  if (NFAPI_MODE != NFAPI_UE_STUB_PNF) {
    number_of_cards = 1;

    for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      PHY_vars_UE_g[0][CC_id]->rf_map.card=0;
      PHY_vars_UE_g[0][CC_id]->rf_map.chain=CC_id+(get_softmodem_params()->chain_offset);
    }
  }

  if (input_fd) {
    printf("Reading in from file to antenna buffer %d\n",0);

    if (fread(PHY_vars_UE_g[0][0]->common_vars.rxdata[0],
              sizeof(int32_t),
              frame_parms[0]->samples_per_tti*10,
              input_fd) != frame_parms[0]->samples_per_tti*10)
      printf("error reading from file\n");
  }

  //p_exmimo_config->framing.tdd_config = TXRXSWITCH_TESTRX;

  if (IS_SOFTMODEM_SIML1 )  {
    init_ocm((double)snr_dB,0);
    PHY_vars_UE_g[0][0]->no_timing_correction = 1;
  }

#ifdef XFORMS
  int UE_id;
  printf("XFORMS\n");

  if (get_softmodem_params()->do_forms==1) {
    fl_initialize (&argc, argv, NULL, 0, 0);
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
    ret = pthread_create(&forms_thread, NULL, scope_thread, NULL);

    if (ret == 0)
      pthread_setname_np( forms_thread, "xforms" );

    printf("Scope thread created, ret=%d\n",ret);
  }

#endif
  config_check_unknown_cmdlineopt(CONFIG_CHECKALLSECTIONS);
  printf("Sending sync to all threads (%p,%p,%p)\n",&sync_var,&sync_cond,&sync_mutex);
  pthread_mutex_lock(&sync_mutex);
  sync_var=0;
  pthread_cond_broadcast(&sync_cond);
  pthread_mutex_unlock(&sync_mutex);
  printf("sync sent\n");
  /*
    printf("About to call end_configmodule() from %s() %s:%d\n", __FUNCTION__, __FILE__, __LINE__);
    end_configmodule();
    printf("Called end_configmodule() from %s() %s:%d\n", __FUNCTION__, __FILE__, __LINE__);
  */
  // wait for end of program
  printf("TYPE <CTRL-C> TO TERMINATE\n");
  //getchar();
#if defined(ENABLE_ITTI)
  printf("Entering ITTI signals handler\n");
  itti_wait_tasks_end();
  printf("Returned from ITTI signal handler\n");
  oai_exit=1;
  printf("oai_exit=%d\n",oai_exit);
#else

  while (oai_exit==0)
    rt_sleep_ns(100000000ULL);

  printf("Terminating application - oai_exit=%d\n",oai_exit);
#endif
  // stop threads
#ifdef XFORMS
  printf("waiting for XFORMS thread\n");

  if (get_softmodem_params()->do_forms==1) {
    pthread_join(forms_thread,&status);
    fl_hide_form(form_stats->stats_form);
    fl_free_form(form_stats->stats_form);
    fl_hide_form(form_ue[0]->lte_phy_scope_ue);
    fl_free_form(form_ue[0]->lte_phy_scope_ue);
  }

#endif
  printf("stopping MODEM threads\n");
  pthread_cond_destroy(&sync_cond);
  pthread_mutex_destroy(&sync_mutex);

  //  pthread_mutex_destroy(&ue_pf_po_mutex);

  // *** Handle per CC_id openair0
  if (PHY_vars_UE_g[0][0]->rfdevice.trx_end_func)
    PHY_vars_UE_g[0][0]->rfdevice.trx_end_func(&PHY_vars_UE_g[0][0]->rfdevice);

  terminate_opt();
  logClean();
  printf("Bye.\n");
  return 0;
}
