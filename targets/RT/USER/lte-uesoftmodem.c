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
 * \email: {knopp, florian.kaltenberger, navid.nikaein}@eurecom.fr
 * \note
 * \warning
 */


#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>

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
#include "PHY_INTERFACE/phy_stub_UE.h"
#include "PHY/TOOLS/phy_scope_interface.h"
#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"

#include "create_tasks.h"
#include "system.h"

#include "lte-softmodem.h"
#include "executables/softmodem-common.h"

/* temporary compilation wokaround (UE/eNB split */


pthread_cond_t nfapi_sync_cond;
pthread_mutex_t nfapi_sync_mutex;
int nfapi_sync_var=-1; //!< protected by mutex \ref nfapi_sync_mutex


uint16_t sf_ahead=4;
int tddflag;
char *emul_iface;


pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.
int config_sync_var=-1;

uint16_t runtime_phy_rx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]
uint16_t runtime_phy_tx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]

volatile int             oai_exit = 0;

unsigned int                    mmapped_dma=0;


uint64_t                 downlink_frequency[MAX_NUM_CCs][4];
int32_t                  uplink_frequency_offset[MAX_NUM_CCs][4];



int UE_scan = 1;
int UE_scan_carrier = 0;

runmode_t mode = normal_txrx;

FILE *input_fd=NULL;
int otg_enabled=0;

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



static LTE_DL_FRAME_PARMS      *frame_parms[MAX_NUM_CCs];

uint64_t num_missed_slots=0; // counter for the number of missed slots

// prototypes from function implemented in lte-ue.c, probably should be elsewhere in a include file.
extern void init_UE_stub_single_thread(int nb_inst,int eMBMS_active, int uecap_xer_in, char *emul_iface);
extern PHY_VARS_UE *init_ue_vars(LTE_DL_FRAME_PARMS *frame_parms, uint8_t UE_id, uint8_t abstraction_flag);
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
//threads_t threads= {-1,-1,-1,-1,-1,-1,-1,-1};

/* see file openair2/LAYER2/MAC/main.c for why abstraction_flag is needed
 * this is very hackish - find a proper solution
 */
uint8_t abstraction_flag=0;

bler_struct bler_data[NUM_MCS];
// needed for pdcp.c
RAN_CONTEXT_t RC;
instance_t CUuniqInstance=0;
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

extern int16_t dlsch_demod_shift;
uint16_t node_number;
static void get_options(void) {
  int CC_id=0;
  int tddflag=0;
  char *loopfile=NULL;
  int dumpframe=0;
  int timingadv=0;
  uint8_t nfapi_mode = NFAPI_MONOLITHIC;

  set_default_frame_parms(frame_parms);
  CONFIG_SETRTFLAG(CONFIG_NOEXITONHELP);
  /* unknown parameters on command line will be checked in main
     after all init have been performed                         */
  get_common_options(SOFTMODEM_4GUE_BIT );
  get_uethreads_params();
  paramdef_t cmdline_uemodeparams[] =CMDLINE_UEMODEPARAMS_DESC;
  paramdef_t cmdline_ueparams[] =CMDLINE_UEPARAMS_DESC;
  config_process_cmdline( cmdline_uemodeparams,sizeof(cmdline_uemodeparams)/sizeof(paramdef_t),NULL);
  config_process_cmdline( cmdline_ueparams,sizeof(cmdline_ueparams)/sizeof(paramdef_t),NULL);
  nfapi_setmode(nfapi_mode);


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
    frame_parms[CC_id] = (LTE_DL_FRAME_PARMS *) calloc(1, sizeof(LTE_DL_FRAME_PARMS));
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
    openair0_cfg[card].clock_source = get_softmodem_params()->clock_source;
    openair0_cfg[card].time_source = get_softmodem_params()->timing_source;
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
  }
}



/* helper function to terminate a certain ITTI task
 */
void terminate_task(task_id_t task_id, module_id_t mod_id) {
  LOG_I(ENB_APP, "sending TERMINATE_MESSAGE to task %s (%d)\n", itti_get_task_name(task_id), task_id);
  MessageDef *msg;
  msg = itti_alloc_new_message (ENB_APP, 0, TERMINATE_MESSAGE);
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

static void init_pdcp(int ue_id) {
  uint32_t pdcp_initmask = (!IS_SOFTMODEM_NOS1) ? LINK_ENB_PDCP_TO_GTPV1U_BIT : (LINK_ENB_PDCP_TO_GTPV1U_BIT | PDCP_USE_NETLINK_BIT | LINK_ENB_PDCP_TO_IP_DRIVER_BIT);

  if (IS_SOFTMODEM_BASICSIM || IS_SOFTMODEM_RFSIM || (nfapi_getmode()==NFAPI_UE_STUB_PNF)) {
    pdcp_initmask = pdcp_initmask | UE_NAS_USE_TUN_BIT;
  }

  if (IS_SOFTMODEM_NOKRNMOD)
    pdcp_initmask = pdcp_initmask | UE_NAS_USE_TUN_BIT;

  pdcp_module_init(pdcp_initmask, ue_id);
  pdcp_set_rlc_data_req_func((send_rlc_data_req_func_t) rlc_data_req);
  pdcp_set_pdcp_data_ind_func((pdcp_data_ind_func_t) pdcp_data_ind);
}

// Stupid function addition because UE itti messages queues definition is common with eNB
void *rrc_enb_process_msg(void *notUsed) {
AssertFatal(false,"");
	return NULL;
}

int main( int argc, char **argv ) {

  int CC_id;
  uint8_t  abstraction_flag=0;
  // Default value for the number of UEs. It will hold,
  // if not changed from the command line option --num-ues
  NB_UE_INST=1;
  NB_THREAD_INST=1;
  configmodule_interface_t *config_mod;
  start_background_system();
  config_mod = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY);

  if (config_mod == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }

  mode = normal_txrx;
  memset(&openair0_cfg[0],0,sizeof(openair0_config_t)*MAX_CARDS);
  logInit();
  configure_linux();
  printf("Reading in command-line options\n");

  for (int i=0; i<MAX_NUM_CCs; i++) tx_max_power[i]=23;

  get_options ();

  if (NFAPI_MODE == NFAPI_MODE_STANDALONE_PNF) {
    sf_ahead = 1;
  }
  printf("sf_ahead = %d\n", sf_ahead);

  EPC_MODE_ENABLED = !IS_SOFTMODEM_NOS1;
  printf("Running with %d UE instances\n",NB_UE_INST);

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
  itti_init(TASK_MAX, tasks_info);

  // initialize mscgen log after ITTI
  if (get_softmodem_params()->start_msc) {
    load_module_shlib("msc",NULL,0,&msc_interface);
  }

  MSC_INIT(MSC_E_UTRAN, ADDED_QUEUES_MAX+TASK_MAX);
  init_opt();
  ue_id_g = (node_number == 0) ? 0 : node_number-2; //ue_id_g = 0, 1, ...,
  if(node_number == 0)
  {
    init_pdcp(0);
  }
  else
  {
    init_pdcp(node_number-1);
  }

  //TTN for D2D
  printf ("RRC control socket\n");
  rrc_control_socket_init();
  printf ("PDCP PC5S socket\n");
  pdcp_pc5_socket_init();
  // to make a graceful exit when ctrl-c is pressed
  set_softmodem_sighandler();
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

  if(NFAPI_MODE==NFAPI_UE_STUB_PNF || NFAPI_MODE==NFAPI_MODE_STANDALONE_PNF) {
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


  cpuf=get_cpu_freq_GHz();

  if (create_tasks_ue(NB_UE_INST) < 0) {
    printf("cannot create ITTI tasks\n");
    exit(-1); // need a softer mode
  }

  if (NFAPI_MODE==NFAPI_UE_STUB_PNF || NFAPI_MODE==NFAPI_MODE_STANDALONE_PNF) { // UE-STUB-PNF
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
  } else if (NFAPI_MODE==NFAPI_MODE_STANDALONE_PNF) {
    init_queue(&dl_config_req_tx_req_queue);
    init_queue(&hi_dci0_req_queue);
    init_queue(&ul_config_req_queue);

    init_bler_table();

    config_sync_var=0;
    if (sem_init(&sfn_semaphore, 0, 0) != 0)
    {
      LOG_E(MAC, "sem_init() error\n");
      abort();
    }
    init_UE_stub_single_thread(NB_UE_INST,eMBMS_active,uecap_xer_in,emul_iface);
    init_UE_standalone_thread(ue_id_g);
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


  if(IS_SOFTMODEM_DOSCOPE)
    load_softscope("ue",NULL);

  config_check_unknown_cmdlineopt(CONFIG_CHECKALLSECTIONS);
  printf("Sending sync to all threads (%p,%p,%p)\n",&sync_var,&sync_cond,&sync_mutex);
  pthread_mutex_lock(&sync_mutex);
  sync_var=0;
  pthread_cond_broadcast(&sync_cond);
  pthread_mutex_unlock(&sync_mutex);
  printf("sync sent\n");
  /*
    printf("About to call end_configmodule() from %s() %s:%d\n", __FUNCTION__, __FILE__, __LINE__);
    We have to set properly PARAMFLAG_NOFREE
    end_configmodule();
    printf("Called end_configmodule() from %s() %s:%d\n", __FUNCTION__, __FILE__, __LINE__);
  */
  // wait for end of program
  printf("TYPE <CTRL-C> TO TERMINATE\n");
  //getchar();
  printf("Entering ITTI signals handler\n");
  itti_wait_tasks_end();
  printf("Returned from ITTI signal handler\n");
  oai_exit=1;
  printf("oai_exit=%d\n",oai_exit);

  // stop threads
  if(IS_SOFTMODEM_DOSCOPE)
    end_forms();

  printf("stopping MODEM threads\n");
  pthread_cond_destroy(&sync_cond);
  pthread_mutex_destroy(&sync_mutex);

  //  pthread_mutex_destroy(&ue_pf_po_mutex);

  // *** Handle per CC_id openair0
  if (PHY_vars_UE_g[0][0]->rfdevice.trx_end_func)
    PHY_vars_UE_g[0][0]->rfdevice.trx_end_func(&PHY_vars_UE_g[0][0]->rfdevice);

  pdcp_module_cleanup();
  terminate_opt();
  logClean();
  printf("Bye.\n");
  return 0;
}


// Read in each MCS file and build BLER-SINR-TB table
void init_bler_table(void)
{
  size_t bufSize = 1024;
  char * line = NULL;
  char * token;
  char * temp = NULL;
  const char *openair_dir = getenv("OPENAIR_DIR");
  if (!openair_dir)
  {
    LOG_E(MAC, "No $OPENAIR_DIR\n");
    abort();
  }

  // Maybe not needed... and may not work.
  memset(bler_data, 0, sizeof(bler_data));

  for (unsigned int i = 0; i < NUM_MCS; i++)
  {
    char fName[1024];
    snprintf(fName, sizeof(fName), "%s/openair1/SIMULATION/LTE_PHY/BLER_SIMULATIONS/AWGN/AWGN_results/bler_tx1_chan18_nrx1_mcs%d.csv", openair_dir, i);
    FILE *pFile = fopen(fName, "r");
    if (!pFile)
    {
      LOG_E(MAC, "Bler File ERROR! - fopen(), file: %s\n", fName);
      abort();
    }
    int nlines = 0;
    while (getline(&line, &bufSize, pFile) > 0)
    {
      if (!strncmp(line,"SNR",3))
      {
        continue;
      }

      if (nlines > NUM_SINR)
      {
        LOG_E(MAC, "BLER FILE ERROR - num lines greater than expected - file: %s\n", fName);
        abort();
      }

      token = strtok_r(line, ";", &temp);
      int ncols = 0;
      while (token != NULL)
      {
        if (ncols > NUM_BLER_COL)
        {
          LOG_E(MAC, "BLER FILE ERROR - num of cols greater than expected\n");
          abort();
        }

        bler_data[i].bler_table[nlines][ncols] = strtof(token, NULL);
        ncols++;

        token = strtok_r(NULL, ";", &temp);
      }
      nlines++;
    }
    bler_data[i].length = nlines;
    fclose(pFile);
  }
}
