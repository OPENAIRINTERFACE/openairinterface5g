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


#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>

#include "T.h"
#include "assertions.h"
#include "PHY/types.h"
#include "PHY/defs_nr_UE.h"
#include "SCHED_NR_UE/defs.h"
#include "common/ran_context.h"
#include "common/config/config_userapi.h"
//#include "common/utils/threadPool/thread-pool.h"
#include "common/utils/load_module_shlib.h"
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "../../ARCH/COMMON/common_lib.h"
#include "../../ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"

//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/phy_vars_nr_ue.h"
#include "PHY/LTE_TRANSPORT/transport_vars.h"
#include "SCHED/sched_common_vars.h"
#include "PHY/MODULATION/modulation_vars.h"
//#include "../../SIMU/USER/init_lte.h"
#include "PHY/NR_REFSIG/nr_mod_table.h"

#include "LAYER2/MAC/mac_vars.h"
#include "RRC/LTE/rrc_vars.h"
#include "PHY_INTERFACE/phy_interface_vars.h"
#include "openair1/SIMULATION/TOOLS/sim.h"

#ifdef SMBV
#include "PHY/TOOLS/smbv.h"
unsigned short config_frames[4] = {2,9,11,13};
#endif
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "UTIL/OPT/opt.h"
#include "enb_config.h"
//#include "PHY/TOOLS/time_meas.h"

#ifndef OPENAIR2
  #include "UTIL/OTG/otg_vars.h"
#endif

#include "intertask_interface.h"

#include "PHY/INIT/phy_init.h"
#include "system.h"
#include <openair2/RRC/NR_UE/rrc_proto.h>
#include <openair2/LAYER2/NR_MAC_UE/mac_defs.h>
#include <openair2/LAYER2/NR_MAC_UE/mac_proto.h>
#include <openair2/NR_UE_PHY_INTERFACE/NR_IF_Module.h>
#include <openair1/SCHED_NR_UE/fapi_nr_ue_l1.h>

#include <forms.h>


/* Callbacks, globals and object handlers */

extern void reset_stats( FL_OBJECT *, long );
//extern void initTpool(char *params, tpool_t *pool, bool performanceMeas);

/* Forms and Objects */

typedef struct {
  FL_FORM    *stats_form;
  void       *vdata;
  char       *cdata;
  long        ldata;
  FL_OBJECT *stats_text;
  FL_OBJECT *stats_button;
} FD_stats_form;

extern FD_stats_form *create_form_stats_form( void );

#include "PHY/TOOLS/nr_phy_scope.h"
//#include "stats.h"
// current status is that every UE has a DL scope for a SINGLE eNB (eNB_id=0)
// at eNB 0, an UL scope for every UE
FD_phy_scope_nrue  *form_nrue[NUMBER_OF_UE_MAX];
//FD_lte_phy_scope_enb *form_enb[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
//FD_stats_form                  *form_stats=NULL,*form_stats_l2=NULL;
char title[255];
static pthread_t forms_thread; //xforms

#include <executables/nr-uesoftmodem.h>
#include "executables/softmodem-common.h"
#include "executables/thread-common.h"

// Raphael : missing
pthread_cond_t nfapi_sync_cond;
pthread_mutex_t nfapi_sync_mutex;
int nfapi_sync_var=-1; //!< protected by mutex \ref nfapi_sync_mutex
uint16_t sf_ahead=6; //??? value ???
pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.
int config_sync_var=-1;

RAN_CONTEXT_t RC;
volatile int             start_eNB = 0;
volatile int             start_UE = 0;
volatile int             oai_exit = 0;

int                      single_thread_flag=1;
static double            snr_dB=20;

int                      threequarter_fs=0;

uint64_t                 downlink_frequency[MAX_NUM_CCs][4];
int32_t                  uplink_frequency_offset[MAX_NUM_CCs][4];
//int32_t					 uplink_counter = 0;


extern int16_t nr_dlsch_demod_shift;

int UE_scan = 0;
int UE_scan_carrier = 0;
int UE_fo_compensation = 0;
int UE_no_timing_correction = 0;
runmode_t mode = normal_txrx;
openair0_config_t openair0_cfg[MAX_CARDS];

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

static int  tx_max_power[MAX_NUM_CCs] = {0};


int chain_offset=0;


uint8_t dci_Format = 0;
uint8_t agregation_Level =0xFF;

uint8_t nb_antenna_tx = 1;
uint8_t nb_antenna_rx = 1;

char ref[128] = "internal";
char channels[128] = "0";


static char *parallel_config = NULL;
static char *worker_config = NULL;

int rx_input_level_dBm;

//static int online_log_messages=0;

uint32_t do_forms=0;
int otg_enabled;
//int number_of_cards = 1;

static NR_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs];
int16_t node_synch_ref[MAX_NUM_CCs];

uint32_t target_dl_mcs = 28; //maximum allowed mcs
uint32_t target_ul_mcs = 20;
uint32_t timing_advance = 0;
uint64_t num_missed_slots=0; // counter for the number of missed slots


int transmission_mode=1;
int numerology = 0;

/* flag set by eNB conf file to specify if the radio head is local or remote (default option is local) */
//uint8_t local_remote_radio = BBU_LOCAL_RADIO_HEAD;
/* struct for ethernet specific parameters given in eNB conf file */
//eth_params_t *eth_params;

double cpuf;

char uecap_xer[1024],uecap_xer_in=0;

int oaisim_flag=0;
int emulate_rf = 0;

tpool_t *Tpool;
#ifdef UE_DLSCH_PARALLELISATION
  tpool_t *Tpool_dl;
#endif


char *usrp_args=NULL;

char *rrc_config_path=NULL;

/* forward declarations */
void set_default_frame_parms(NR_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs]);


/* see file openair2/LAYER2/MAC/main.c for why abstraction_flag is needed
 * this is very hackish - find a proper solution
 */
uint8_t abstraction_flag=0;

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
  LOG_I(HW,"difftimes min = %lu ns ; max = %lu ns\n", min_diff_time.tv_nsec, max_diff_time.tv_nsec);
}

void exit_function(const char *file, const char *function, const int line, const char *s) {
  int CC_id;

  if (s != NULL) {
    printf("%s:%d %s() Exiting OAI softmodem: %s\n",file,line, function, s);
  }

  oai_exit = 1;

  if (PHY_vars_UE_g && PHY_vars_UE_g[0]) {
    for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      if (PHY_vars_UE_g[0][CC_id] && PHY_vars_UE_g[0][CC_id]->rfdevice.trx_end_func)
        PHY_vars_UE_g[0][CC_id]->rfdevice.trx_end_func(&PHY_vars_UE_g[0][CC_id]->rfdevice);
    }
  }

  sleep(1); //allow lte-softmodem threads to exit first
  exit(1);
}


void reset_stats(FL_OBJECT *button, long arg) {
  //int i,j,k;
  /*PHY_VARS_eNB *phy_vars_eNB = PHY_vars_eNB_g[0][0];

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
  }*/
}

static void *scope_thread(void *arg) {
  sleep(5);

  while (!oai_exit) {
    phy_scope_nrUE(form_nrue[0],
                   PHY_vars_UE_g[0][0],
                   0,0,1);
    usleep(100*1000);
  }

  pthread_exit((void *)arg);
}


void init_scope(void) {
  int fl_argc=1;

  if (do_forms==1) {
    char *name="5G-UE-scope";
    fl_initialize (&fl_argc, &name, NULL, 0, 0);
    int UE_id = 0;
    form_nrue[UE_id] = create_phy_scope_nrue();
    sprintf (title, "NR DL SCOPE UE");
    fl_show_form (form_nrue[UE_id]->phy_scope_nrue, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
    threadCreate(&forms_thread, scope_thread, NULL, "scope", -1, OAI_PRIORITY_RT_LOW);
  }

}

void *l2l1_task(void *arg) {
  MessageDef *message_p = NULL;
  int         result;
  itti_set_task_real_time(TASK_L2L1);
  itti_mark_task_ready(TASK_L2L1);

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


int16_t dlsch_demod_shift;

static void get_options(void) {
  int CC_id;
  int tddflag=0, nonbiotflag, vcdflag=0;
  char *loopfile=NULL;
  int dumpframe=0;
  //uint32_t noS1;
  //uint32_t nokrnmod;
  //uint32_t nokrnmod;
  paramdef_t cmdline_params[] =CMDLINE_PARAMS_DESC_UE ;
  config_process_cmdline( cmdline_params,sizeof(cmdline_params)/sizeof(paramdef_t),NULL);


  paramdef_t cmdline_uemodeparams[] = CMDLINE_UEMODEPARAMS_DESC;
  paramdef_t cmdline_ueparams[] = CMDLINE_NRUEPARAMS_DESC;
  config_process_cmdline( cmdline_uemodeparams,sizeof(cmdline_uemodeparams)/sizeof(paramdef_t),NULL);
  config_process_cmdline( cmdline_ueparams,sizeof(cmdline_ueparams)/sizeof(paramdef_t),NULL);

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
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++)
      frame_parms[CC_id]->frame_type = TDD;
  }

  if (vcdflag > 0)
    ouput_vcd = 1;

  /*if (frame_parms[0]->N_RB_DL !=0) {
      if ( frame_parms[0]->N_RB_DL < 6 ) {
       frame_parms[0]->N_RB_DL = 6;
       printf ( "%i: Invalid number of resource blocks, adjusted to 6\n",frame_parms[0]->N_RB_DL);
      }
      if ( frame_parms[0]->N_RB_DL > 100 ) {
       frame_parms[0]->N_RB_DL = 100;
       printf ( "%i: Invalid number of resource blocks, adjusted to 100\n",frame_parms[0]->N_RB_DL);
      }
      if ( frame_parms[0]->N_RB_DL > 50 && frame_parms[0]->N_RB_DL < 100 ) {
       frame_parms[0]->N_RB_DL = 50;
       printf ( "%i: Invalid number of resource blocks, adjusted to 50\n",frame_parms[0]->N_RB_DL);
      }
      if ( frame_parms[0]->N_RB_DL > 25 && frame_parms[0]->N_RB_DL < 50 ) {
       frame_parms[0]->N_RB_DL = 25;
       printf ( "%i: Invalid number of resource blocks, adjusted to 25\n",frame_parms[0]->N_RB_DL);
      }
      UE_scan = 0;
      frame_parms[0]->N_RB_UL=frame_parms[0]->N_RB_DL;
      for (CC_id=1; CC_id<MAX_NUM_CCs; CC_id++) {
        frame_parms[CC_id]->N_RB_DL=frame_parms[0]->N_RB_DL;
        frame_parms[CC_id]->N_RB_UL=frame_parms[0]->N_RB_UL;
      }
  }*/

  for (CC_id=1; CC_id<MAX_NUM_CCs; CC_id++) {
    tx_max_power[CC_id]=tx_max_power[0];
    rx_gain[0][CC_id] = rx_gain[0][0];
    tx_gain[0][CC_id] = tx_gain[0][0];
  }


  if ( !(CONFIG_ISFLAGSET(CONFIG_ABORT))  && (!(CONFIG_ISFLAGSET(CONFIG_NOOOPT))) ) {
    // Here the configuration file is the XER encoded UE capabilities
    // Read it in and store in asn1c data structures
    sprintf(uecap_xer,"%stargets/PROJECTS/GENERIC-LTE-EPC/CONF/UE_config.xml",getenv("OPENAIR_HOME"));
    printf("%s\n",uecap_xer);
    uecap_xer_in=1;
  } /* UE with config file  */
}


void set_default_frame_parms(NR_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs]) {
  int CC_id;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    /* Set some default values that may be overwritten while reading options */
    frame_parms[CC_id] = (NR_DL_FRAME_PARMS *) calloc(sizeof(NR_DL_FRAME_PARMS),1);
    frame_parms[CC_id]->nr_band          = 78;
    frame_parms[CC_id]->frame_type          = FDD;
    frame_parms[CC_id]->tdd_config          = 3;
    //frame_parms[CC_id]->tdd_config_S        = 0;
    frame_parms[CC_id]->N_RB_DL             = 106;
    frame_parms[CC_id]->N_RB_UL             = 106;
    frame_parms[CC_id]->Ncp                 = NORMAL;
    //frame_parms[CC_id]->Ncp_UL              = NORMAL;
    frame_parms[CC_id]->Nid_cell            = 0;
    //frame_parms[CC_id]->num_MBSFN_config    = 0;
    frame_parms[CC_id]->nb_antenna_ports_gNB  = 1;
    frame_parms[CC_id]->nb_antennas_tx      = 1;
    frame_parms[CC_id]->nb_antennas_rx      = 1;
    //frame_parms[CC_id]->nushift             = 0;
    // NR: Init to legacy LTE 20Mhz params
    frame_parms[CC_id]->numerology_index  = 0;
    frame_parms[CC_id]->ttis_per_subframe = 1;
    frame_parms[CC_id]->slots_per_tti   = 2;
  }
}

void init_openair0(void) {
  int card;
  int i;

  for (card=0; card<MAX_CARDS; card++) {
    openair0_cfg[card].configFilename = NULL;
    openair0_cfg[card].threequarter_fs = frame_parms[0]->threequarter_fs;
    numerology = frame_parms[0]->numerology_index;

    if(frame_parms[0]->N_RB_DL == 66) {
      if (numerology==3) {
          openair0_cfg[card].sample_rate=122.88e6;
          openair0_cfg[card].samples_per_frame = 1228800;
        } else {
          LOG_E(PHY,"Unsupported numerology! FR2 supports only 120KHz SCS for now.\n");
          exit(-1);
        }
    }else if(frame_parms[0]->N_RB_DL == 32) {
      if (numerology==3) {
          openair0_cfg[card].sample_rate=61.44e6;
          openair0_cfg[card].samples_per_frame = 614400;
        } else {
          LOG_E(PHY,"Unsupported numerology! FR2 supports only 120KHz SCS for now.\n");
          exit(-1);
        }
    }else if(frame_parms[0]->N_RB_DL == 217) {
      if (numerology==1) {
        if (frame_parms[0]->threequarter_fs) {
          openair0_cfg[card].sample_rate=92.16e6;
          openair0_cfg[card].samples_per_frame = 921600;
        }
        else {
          openair0_cfg[card].sample_rate=122.88e6;
          openair0_cfg[card].samples_per_frame = 1228800;
        }
      } else {
        LOG_E(PHY,"Unsupported numerology!\n");
        exit(-1);
      }
    } else if(frame_parms[0]->N_RB_DL == 273) {
      if (numerology==1) {
        if (frame_parms[0]->threequarter_fs) {
          AssertFatal(0 == 1,"three quarter sampling not supported for N_RB 273\n");
        }
        else {
          openair0_cfg[card].sample_rate=122.88e6;
          openair0_cfg[card].samples_per_frame = 1228800;
        }
      } else {
        LOG_E(PHY,"Unsupported numerology!\n");
        exit(-1);
      }
    } else if(frame_parms[0]->N_RB_DL == 106) {
      if (numerology==0) {
        if (frame_parms[0]->threequarter_fs) {
          openair0_cfg[card].sample_rate=23.04e6;
          openair0_cfg[card].samples_per_frame = 230400;
        } else {
          openair0_cfg[card].sample_rate=30.72e6;
          openair0_cfg[card].samples_per_frame = 307200;
        }
      } else if (numerology==1) {
        if (frame_parms[0]->threequarter_fs) {
          openair0_cfg[card].sample_rate=46.08e6;
          openair0_cfg[card].samples_per_frame = 460800;
	}
	else {
          openair0_cfg[card].sample_rate=61.44e6;
          openair0_cfg[card].samples_per_frame = 614400;
        }
      } else if (numerology==2) {
        openair0_cfg[card].sample_rate=122.88e6;
        openair0_cfg[card].samples_per_frame = 1228800;
      } else {
        LOG_E(PHY,"Unsupported numerology!\n");
        exit(-1);
      }
    } else if(frame_parms[0]->N_RB_DL == 50) {
      openair0_cfg[card].sample_rate=15.36e6;
      openair0_cfg[card].samples_per_frame = 153600;
    } else if (frame_parms[0]->N_RB_DL == 25) {
      openair0_cfg[card].sample_rate=7.68e6;
      openair0_cfg[card].samples_per_frame = 76800;
    } else if (frame_parms[0]->N_RB_DL == 6) {
      openair0_cfg[card].sample_rate=1.92e6;
      openair0_cfg[card].samples_per_frame = 19200;
    }
    else {
      LOG_E(PHY,"Unknown NB_RB %d!\n",frame_parms[0]->N_RB_DL);
      exit(-1);
    }

    if (frame_parms[0]->frame_type==TDD)
      openair0_cfg[card].duplex_mode = duplex_mode_TDD;
    else //FDD
      openair0_cfg[card].duplex_mode = duplex_mode_FDD;

    printf("HW: Configuring card %d, nb_antennas_tx/rx %hhu/%hhu\n",card,
           PHY_vars_UE_g[0][0]->frame_parms.nb_antennas_tx,
           PHY_vars_UE_g[0][0]->frame_parms.nb_antennas_rx);
    openair0_cfg[card].Mod_id = 0;
    openair0_cfg[card].num_rb_dl=frame_parms[0]->N_RB_DL;
    openair0_cfg[card].clock_source = get_softmodem_params()->clock_source;
    openair0_cfg[card].time_source = get_softmodem_params()->timing_source;
    openair0_cfg[card].tx_num_channels=min(2,PHY_vars_UE_g[0][0]->frame_parms.nb_antennas_tx);
    openair0_cfg[card].rx_num_channels=min(2,PHY_vars_UE_g[0][0]->frame_parms.nb_antennas_rx);

    for (i=0; i<4; i++) {
      if (i<openair0_cfg[card].tx_num_channels)
        openair0_cfg[card].tx_freq[i] = frame_parms[0]->ul_CarrierFreq;
      else
        openair0_cfg[card].tx_freq[i]=0.0;

      if (i<openair0_cfg[card].rx_num_channels)
        openair0_cfg[card].rx_freq[i] = frame_parms[0]->dl_CarrierFreq;
      else
        openair0_cfg[card].rx_freq[i]=0.0;

      openair0_cfg[card].autocal[i] = 1;
      openair0_cfg[card].tx_gain[i] = tx_gain[0][i];
      openair0_cfg[card].rx_gain[i] = PHY_vars_UE_g[0][0]->rx_total_gain_dB - rx_gain_off;
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

void init_pdcp(void) {
  uint32_t pdcp_initmask = (!IS_SOFTMODEM_NOS1) ? LINK_ENB_PDCP_TO_GTPV1U_BIT : (LINK_ENB_PDCP_TO_GTPV1U_BIT | PDCP_USE_NETLINK_BIT | LINK_ENB_PDCP_TO_IP_DRIVER_BIT);

  /*if (IS_SOFTMODEM_BASICSIM || IS_SOFTMODEM_RFSIM || (nfapi_getmode()==NFAPI_UE_STUB_PNF)) {
    pdcp_initmask = pdcp_initmask | UE_NAS_USE_TUN_BIT;
  }*/

  if (IS_SOFTMODEM_NOKRNMOD)
    pdcp_initmask = pdcp_initmask | UE_NAS_USE_TUN_BIT;

  /*if (rlc_module_init() != 0) {
    LOG_I(RLC, "Problem at RLC initiation \n");
  }
  pdcp_layer_init();
  nr_ip_over_LTE_DRB_preconfiguration();*/
  pdcp_module_init(pdcp_initmask);
  pdcp_set_rlc_data_req_func((send_rlc_data_req_func_t) rlc_data_req);
  pdcp_set_pdcp_data_ind_func((pdcp_data_ind_func_t) pdcp_data_ind);
  LOG_I(PDCP, "Before getting out from init_pdcp() \n");
}

// Stupid function addition because UE itti messages queues definition is common with eNB
void *rrc_enb_process_msg(void *notUsed) {
  return NULL;
}


int main( int argc, char **argv ) {
  //uint8_t beta_ACK=0,beta_RI=0,beta_CQI=2;
  PHY_VARS_NR_UE *UE[MAX_NUM_CCs];
  start_background_system();

  if ( load_configmodule(argc,argv,CONFIG_ENABLECMDLINEONLY) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }
  set_softmodem_sighandler();
  CONFIG_SETRTFLAG(CONFIG_NOEXITONHELP);
  set_default_frame_parms(frame_parms);
  mode = normal_txrx;
  memset(openair0_cfg,0,sizeof(openair0_config_t)*MAX_CARDS);
  memset(tx_max_power,0,sizeof(int)*MAX_NUM_CCs);
  // initialize logging
  logInit();
  // get options and fill parameters from configuration file
  get_options (); //Command-line options, enb_properties
  get_common_options(SOFTMODEM_5GUE_BIT );
#if T_TRACER
  T_Config_Init();
#endif
  //randominit (0);
  set_taus_seed (0);
  tpool_t pool;
  Tpool = &pool;
  char params[]="-1,-1";
  initTpool(params, Tpool, false);
#ifdef UE_DLSCH_PARALLELISATION
  tpool_t pool_dl;
  Tpool_dl = &pool_dl;
  char params_dl[]="-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1";
  initTpool(params_dl, Tpool_dl, false);
#endif
  cpuf=get_cpu_freq_GHz();
  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info);

  init_opt() ;
  load_nrLDPClib();

  if (ouput_vcd) {
    vcd_signal_dumper_init("/tmp/openair_dump_nrUE.vcd");
  }

  #ifndef PACKAGE_VERSION
#  define PACKAGE_VERSION "UNKNOWN-EXPERIMENTAL"
#endif
  LOG_I(HW, "Version: %s\n", PACKAGE_VERSION);

  init_NR_UE(1,rrc_config_path);
  if(IS_SOFTMODEM_NOS1)
	  init_pdcp();
/*
#ifdef PDCP_USE_NETLINK
  netlink_init();
#if defined(PDCP_USE_NETLINK_QUEUES)
  pdcp_netlink_init();
#endif
#endif
*/

  NB_UE_INST=1;
  NB_INST=1;
  PHY_vars_UE_g = malloc(sizeof(PHY_VARS_NR_UE **));
  PHY_vars_UE_g[0] = malloc(sizeof(PHY_VARS_NR_UE *)*MAX_NUM_CCs);

  for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    printf("frame_parms %d\n",frame_parms[CC_id]->ofdm_symbol_size);
    frame_parms[CC_id]->nb_antennas_tx     = nb_antenna_tx;
    frame_parms[CC_id]->nb_antennas_rx     = nb_antenna_rx;
    frame_parms[CC_id]->nb_antenna_ports_gNB = 1; //initial value overwritten by initial sync later
    frame_parms[CC_id]->threequarter_fs = threequarter_fs;
    LOG_I(PHY,"Set nb_rx_antenna %d , nb_tx_antenna %d \n",frame_parms[CC_id]->nb_antennas_rx, frame_parms[CC_id]->nb_antennas_tx);

    PHY_vars_UE_g[0][CC_id] = (PHY_VARS_NR_UE *)malloc(sizeof(PHY_VARS_NR_UE));

    UE[CC_id] = PHY_vars_UE_g[0][CC_id];
    memset(UE[CC_id],0,sizeof(PHY_VARS_NR_UE));

    NR_UE_MAC_INST_t *mac = get_mac_inst(0);
    if(mac->if_module != NULL && mac->if_module->phy_config_request != NULL)
      mac->if_module->phy_config_request(&mac->phy_config);

    fapi_nr_config_request_t *nrUE_config = &UE[CC_id]->nrUE_config;
    nr_init_frame_parms_ue(frame_parms[CC_id],nrUE_config,NORMAL);
    
    // Overwrite DL frequency (for FR2 testing)
    if (downlink_frequency[0][0]!=0)
      frame_parms[CC_id]->dl_CarrierFreq = downlink_frequency[0][0];
   
    init_nr_ue_vars(UE[CC_id],frame_parms[CC_id],0,abstraction_flag);

    if (get_softmodem_params()->phy_test==1)
      UE[CC_id]->mac_enabled = 0;
    else
      UE[CC_id]->mac_enabled = 1;

    UE[CC_id]->mac_enabled = 1;
    UE[CC_id]->if_inst = nr_ue_if_module_init(0);
    UE[CC_id]->UE_scan = UE_scan;
    UE[CC_id]->UE_scan_carrier = UE_scan_carrier;
    UE[CC_id]->UE_fo_compensation = UE_fo_compensation;
    UE[CC_id]->mode    = mode;
    UE[CC_id]->no_timing_correction = UE_no_timing_correction;
    printf("UE[%d]->mode = %d\n",CC_id,mode);

    UE[CC_id]->rx_total_gain_dB =  (int)rx_gain[CC_id][0] + rx_gain_off;
    UE[CC_id]->tx_power_max_dBm = tx_max_power[CC_id];

    if (frame_parms[CC_id]->frame_type==FDD) {
      UE[CC_id]->N_TA_offset = 0;
    } else {
      if (frame_parms[CC_id]->N_RB_DL == 100)
        UE[CC_id]->N_TA_offset = 624;
      else if (frame_parms[CC_id]->N_RB_DL == 50)
        UE[CC_id]->N_TA_offset = 624/2;
      else if (frame_parms[CC_id]->N_RB_DL == 25)
        UE[CC_id]->N_TA_offset = 624/4;
    }
  }

  //  printf("tx_max_power = %d -> amp %d\n",tx_max_power[0],get_tx_amp(tx_max_poHwer,tx_max_power));
  init_openair0();
  // init UE_PF_PO and mutex lock
  pthread_mutex_init(&ue_pf_po_mutex, NULL);
  memset (&UE_PF_PO[0][0], 0, sizeof(UE_PF_PO_t)*NUMBER_OF_UE_MAX*MAX_NUM_CCs);
  configure_linux();
  mlockall(MCL_CURRENT | MCL_FUTURE);
  init_scope();
  number_of_cards = 1;

  for(int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    PHY_vars_UE_g[0][CC_id]->rf_map.card=0;
    PHY_vars_UE_g[0][CC_id]->rf_map.chain=CC_id+chain_offset;
#if defined(OAI_USRP) || defined(OAI_ADRV9371_ZC706)
    PHY_vars_UE_g[0][CC_id]->hw_timing_advance = timing_advance;
    PHY_vars_UE_g[0][CC_id]->timing_advance = timing_advance;
#else
    PHY_vars_UE_g[0][CC_id]->hw_timing_advance = 160;
#endif

  }

  init_NR_UE_threads(1);
  printf("UE threads created by %ld\n", gettid());
  
  // wait for end of program
  printf("TYPE <CTRL-C> TO TERMINATE\n");

  while(true)
    sleep(3600);

  if (ouput_vcd)
    vcd_signal_dumper_close();

  return 0;
}
