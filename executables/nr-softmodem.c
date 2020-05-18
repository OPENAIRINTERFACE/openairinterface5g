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

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
#include <common/utils/assertions.h>

#include "msc.h"

#include "PHY/types.h"
#include "common/ran_context.h"

#include "PHY/defs_gNB.h"
#include "PHY/defs_common.h"
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "../../ARCH/COMMON/common_lib.h"
#include "../../ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"

//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/phy_vars.h"
#include "SCHED/sched_common_vars.h"
#include "LAYER2/MAC/mac_vars.h"
#include "RRC/LTE/rrc_vars.h"
#include "PHY_INTERFACE/phy_interface_vars.h"
#include "gnb_config.h"
#include "SIMULATION/TOOLS/sim.h"

#ifdef SMBV
#include "PHY/TOOLS/smbv.h"
unsigned short config_frames[4] = {2,9,11,13};
#endif

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"

//#include "PHY/TOOLS/time_meas.h"

#ifndef OPENAIR2
  #include "UTIL/OTG/otg_vars.h"
#endif

#include "intertask_interface.h"

#include "PHY/INIT/phy_init.h"

#include "system.h"
#include <openair2/GNB_APP/gnb_app.h>

#include "PHY/TOOLS/nr_phy_scope.h"
#include "stats.h"
#include "nr-softmodem.h"
#include "executables/softmodem-common.h"
#include "executables/thread-common.h"
#include "NB_IoT_interface.h"
#include "x2ap_eNB.h"

short nr_mod_table[NR_MOD_TABLE_SIZE_SHORT] = {0,0,16384,16384,-16384,-16384,16384,16384,16384,-16384,-16384,16384,-16384,-16384,7327,7327,7327,21981,21981,7327,21981,21981,7327,-7327,7327,-21981,21981,-7327,21981,-21981,-7327,7327,-7327,21981,-21981,7327,-21981,21981,-7327,-7327,-7327,-21981,-21981,-7327,-21981,-21981,10726,10726,10726,3576,3576,10726,3576,3576,10726,17876,10726,25027,3576,17876,3576,25027,17876,10726,17876,3576,25027,10726,25027,3576,17876,17876,17876,25027,25027,17876,25027,25027,10726,-10726,10726,-3576,3576,-10726,3576,-3576,10726,-17876,10726,-25027,3576,-17876,3576,-25027,17876,-10726,17876,-3576,25027,-10726,25027,-3576,17876,-17876,17876,-25027,25027,-17876,25027,-25027,-10726,10726,-10726,3576,-3576,10726,-3576,3576,-10726,17876,-10726,25027,-3576,17876,-3576,25027,-17876,10726,-17876,3576,-25027,10726,-25027,3576,-17876,17876,-17876,25027,-25027,17876,-25027,25027,-10726,-10726,-10726,-3576,-3576,-10726,-3576,-3576,-10726,-17876,-10726,-25027,-3576,-17876,-3576,-25027,-17876,-10726,-17876,-3576,-25027,-10726,-25027,-3576,-17876,-17876,-17876,-25027,-25027,-17876,-25027,-25027,8886,8886,8886,12439,12439,8886,12439,12439,8886,5332,8886,1778,12439,5332,12439,1778,5332,8886,5332,12439,1778,8886,1778,12439,5332,5332,5332,1778,1778,5332,1778,1778,8886,19547,8886,15993,12439,19547,12439,15993,8886,23101,8886,26655,12439,23101,12439,26655,5332,19547,5332,15993,1778,19547,1778,15993,5332,23101,5332,26655,1778,23101,1778,26655,19547,8886,19547,12439,15993,8886,15993,12439,19547,5332,19547,1778,15993,5332,15993,1778,23101,8886,23101,12439,26655,8886,26655,12439,23101,5332,23101,1778,26655,5332,26655,1778,19547,19547,19547,15993,15993,19547,15993,15993,19547,23101,19547,26655,15993,23101,15993,26655,23101,19547,23101,15993,26655,19547,26655,15993,23101,23101,23101,26655,26655,23101,26655,26655,8886,-8886,8886,-12439,12439,-8886,12439,-12439,8886,-5332,8886,-1778,12439,-5332,12439,-1778,5332,-8886,5332,-12439,1778,-8886,1778,-12439,5332,-5332,5332,-1778,1778,-5332,1778,-1778,8886,-19547,8886,-15993,12439,-19547,12439,-15993,8886,-23101,8886,-26655,12439,-23101,12439,-26655,5332,-19547,5332,-15993,1778,-19547,1778,-15993,5332,-23101,5332,-26655,1778,-23101,1778,-26655,19547,-8886,19547,-12439,15993,-8886,15993,-12439,19547,-5332,19547,-1778,15993,-5332,15993,-1778,23101,-8886,23101,-12439,26655,-8886,26655,-12439,23101,-5332,23101,-1778,26655,-5332,26655,-1778,19547,-19547,19547,-15993,15993,-19547,15993,-15993,19547,-23101,19547,-26655,15993,-23101,15993,-26655,23101,-19547,23101,-15993,26655,-19547,26655,-15993,23101,-23101,23101,-26655,26655,-23101,26655,-26655,-8886,8886,-8886,12439,-12439,8886,-12439,12439,-8886,5332,-8886,1778,-12439,5332,-12439,1778,-5332,8886,-5332,12439,-1778,8886,-1778,12439,-5332,5332,-5332,1778,-1778,5332,-1778,1778,-8886,19547,-8886,15993,-12439,19547,-12439,15993,-8886,23101,-8886,26655,-12439,23101,-12439,26655,-5332,19547,-5332,15993,-1778,19547,-1778,15993,-5332,23101,-5332,26655,-1778,23101,-1778,26655,-19547,8886,-19547,12439,-15993,8886,-15993,12439,-19547,5332,-19547,1778,-15993,5332,-15993,1778,-23101,8886,-23101,12439,-26655,8886,-26655,12439,-23101,5332,-23101,1778,-26655,5332,-26655,1778,-19547,19547,-19547,15993,-15993,19547,-15993,15993,-19547,23101,-19547,26655,-15993,23101,-15993,26655,-23101,19547,-23101,15993,-26655,19547,-26655,15993,-23101,23101,-23101,26655,-26655,23101,-26655,26655,-8886,-8886,-8886,-12439,-12439,-8886,-12439,-12439,-8886,-5332,-8886,-1778,-12439,-5332,-12439,-1778,-5332,-8886,-5332,-12439,-1778,-8886,-1778,-12439,-5332,-5332,-5332,-1778,-1778,-5332,-1778,-1778,-8886,-19547,-8886,-15993,-12439,-19547,-12439,-15993,-8886,-23101,-8886,-26655,-12439,-23101,-12439,-26655,-5332,-19547,-5332,-15993,-1778,-19547,-1778,-15993,-5332,-23101,-5332,-26655,-1778,-23101,-1778,-26655,-19547,-8886,-19547,-12439,-15993,-8886,-15993,-12439,-19547,-5332,-19547,-1778,-15993,-5332,-15993,-1778,-23101,-8886,-23101,-12439,-26655,-8886,-26655,-12439,-23101,-5332,-23101,-1778,-26655,-5332,-26655,-1778,-19547,-19547,-19547,-15993,-15993,-19547,-15993,-15993,-19547,-23101,-19547,-26655,-15993,-23101,-15993,-26655,-23101,-19547,-23101,-15993,-26655,-19547,-26655,-15993,-23101,-23101,-23101,-26655,-26655,-23101,-26655,-26655};


pthread_cond_t nfapi_sync_cond;
pthread_mutex_t nfapi_sync_mutex;
int nfapi_sync_var=-1; //!< protected by mutex \ref nfapi_sync_mutex

uint8_t nfapi_mode = 0; // Default to monolithic mode

pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.
int config_sync_var=-1;

volatile int             start_gNB = 0;
volatile int             oai_exit = 0;

static int wait_for_sync = 0;

unsigned int mmapped_dma=0;
int single_thread_flag=1;

int8_t threequarter_fs=0;

uint64_t downlink_frequency[MAX_NUM_CCs][4];
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];

//Temp fix for inexistent NR upper layer
unsigned char NB_gNB_INST = 1;

static char                    *itti_dump_file = NULL;

int UE_scan = 1;
int UE_scan_carrier = 0;
runmode_t mode = normal_txrx;
static double snr_dB=20;

FILE *input_fd=NULL;


#if MAX_NUM_CCs == 1
rx_gain_t rx_gain_mode[MAX_NUM_CCs][4] = {{max_gain,max_gain,max_gain,max_gain}};
double tx_gain[MAX_NUM_CCs][4] = {{20,0,0,0}};
double rx_gain[MAX_NUM_CCs][4] = {{110,0,0,0}};
#else
rx_gain_t rx_gain_mode[MAX_NUM_CCs][4] = {{max_gain,max_gain,max_gain,max_gain},{max_gain,max_gain,max_gain,max_gain}};
double tx_gain[MAX_NUM_CCs][4] = {{20,0,0,0},{20,0,0,0}};
double rx_gain[MAX_NUM_CCs][4] = {{110,0,0,0},{20,0,0,0}};
#endif

double rx_gain_off = 0.0;

double sample_rate=30.72e6;
double bw = 10.0e6;

static int tx_max_power[MAX_NUM_CCs]; /* =  {0,0}*/;


int chain_offset=0;


uint8_t dci_Format = 0;
uint8_t agregation_Level =0xFF;

uint8_t nb_antenna_tx = 1;
uint8_t nb_antenna_rx = 1;

char ref[128] = "internal";
char channels[128] = "0";

int rx_input_level_dBm;

uint32_t do_forms=0;
int otg_enabled;

//int number_of_cards = 1;


//static NR_DL_FRAME_PARMS      *frame_parms[MAX_NUM_CCs];
//static nfapi_nr_config_request_t *config[MAX_NUM_CCs];
uint32_t target_dl_mcs = 28; //maximum allowed mcs
uint32_t target_ul_mcs = 20;
uint32_t timing_advance = 0;
uint64_t num_missed_slots=0; // counter for the number of missed slots

int split73=0;
void sendFs6Ul(PHY_VARS_eNB *eNB, int UE_id, int harq_pid, int segmentID, int16_t *data, int dataLen, int r_offset) {
  AssertFatal(false, "Must not be called in this context\n");
}

extern void reset_opp_meas(void);
extern void print_opp_meas(void);

extern void init_eNB_afterRU(void);

int transmission_mode=1;
int emulate_rf = 0;
int numerology = 0;


static char *parallel_config = NULL;
static char *worker_config = NULL;

/* struct for ethernet specific parameters given in eNB conf file */
eth_params_t *eth_params;

openair0_config_t openair0_cfg[MAX_CARDS];

double cpuf;

extern char uecap_xer[1024];
char uecap_xer_in=0;

/* see file openair2/LAYER2/MAC/main.c for why abstraction_flag is needed
 * this is very hackish - find a proper solution
 */
uint8_t abstraction_flag=0;

/* forward declarations */
void set_default_frame_parms(nfapi_nr_config_request_scf_t *config[MAX_NUM_CCs], NR_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs]);

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


#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KBLU  "\x1B[34m"
#define RESET "\033[0m"


void exit_function(const char *file, const char *function, const int line, const char *s) {
  int ru_id;

  if (s != NULL) {
    printf("%s:%d %s() Exiting OAI softmodem: %s\n",file,line, function, s);
  }

  oai_exit = 1;

  if (RC.ru == NULL)
    exit(-1); // likely init not completed, prevent crash or hang, exit now...

  for (ru_id=0; ru_id<RC.nb_RU; ru_id++) {
    if (RC.ru[ru_id] && RC.ru[ru_id]->rfdevice.trx_end_func) {
      RC.ru[ru_id]->rfdevice.trx_end_func(&RC.ru[ru_id]->rfdevice);
      RC.ru[ru_id]->rfdevice.trx_end_func = NULL;
    }

    if (RC.ru[ru_id] && RC.ru[ru_id]->ifdevice.trx_end_func) {
      RC.ru[ru_id]->ifdevice.trx_end_func(&RC.ru[ru_id]->ifdevice);
      RC.ru[ru_id]->ifdevice.trx_end_func = NULL;
    }
  }

  sleep(1); //allow lte-softmodem threads to exit first
  exit(1);
}


void *l2l1_task(void *arg) {
  MessageDef *message_p = NULL;
  int         result;
  itti_set_task_real_time(TASK_L2L1);
  itti_mark_task_ready(TASK_L2L1);
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
        /* Start gNB thread */
        LOG_D(EMU, "L2L1 TASK received %s\n", ITTI_MSG_NAME(message_p));
        start_gNB = 1;
        break;

      case TERMINATE_MESSAGE:
        printf("received terminate message\n");
        oai_exit=1;
        start_gNB = 0;
        itti_exit_task ();
        break;

      default:
        LOG_E(EMU, "Received unexpected message %s\n", ITTI_MSG_NAME(message_p));
        break;
    }
  } while (ITTI_MSG_ID(message_p) != INITIALIZE_MESSAGE);

  result = itti_free (ITTI_MSG_ORIGIN_ID(message_p), message_p);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  /* ???? no else but seems to be UE only ???
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
  */
  return NULL;
}

int create_gNB_tasks(uint32_t gnb_nb) {
  LOG_D(GNB_APP, "%s(gnb_nb:%d)\n", __FUNCTION__, gnb_nb);
  itti_wait_ready(1);

  if (itti_create_task (TASK_L2L1, l2l1_task, NULL) < 0) {
    LOG_E(PDCP, "Create task for L2L1 failed\n");
    return -1;
  }

  if (gnb_nb > 0) {
    /* Last task to create, others task must be ready before its start */
    if (itti_create_task (TASK_GNB_APP, gNB_app_task, NULL) < 0) {
      LOG_E(GNB_APP, "Create task for gNB APP failed\n");
      return -1;
    }
    if(itti_create_task(TASK_SCTP, sctp_eNB_task, NULL) < 0){
    	LOG_E(SCTP, "Create task for SCTP failed\n");
    	return -1;
    }
    if (is_x2ap_enabled()) {
    	if(itti_create_task(TASK_X2AP, x2ap_task, NULL) < 0){
    		LOG_E(X2AP, "Create task for X2AP failed\n");
    	}
    }
    else {
    	LOG_I(X2AP, "X2AP is disabled.\n");
    }
  }

  /*
    if (EPC_MODE_ENABLED) {
        if (gnb_nb > 0) {
          if (itti_create_task (TASK_SCTP, sctp_eNB_task, NULL) < 0) {
            LOG_E(SCTP, "Create task for SCTP failed\n");
            return -1;
          }

          if (itti_create_task (TASK_S1AP, s1ap_eNB_task, NULL) < 0) {
            LOG_E(S1AP, "Create task for S1AP failed\n");
            return -1;
          }
          if(!emulate_rf){
            if (itti_create_task (TASK_UDP, udp_eNB_task, NULL) < 0) {
              LOG_E(UDP_, "Create task for UDP failed\n");
              return -1;
            }
          }

          if (itti_create_task (TASK_GTPV1_U, &gtpv1u_eNB_task, NULL) < 0) {
            LOG_E(GTPU, "Create task for GTPV1U failed\n");
            return -1;
          }
        }

    }
  */

  if (gnb_nb > 0) {
    LOG_I(NR_RRC,"Creating NR RRC gNB Task\n");

    if (itti_create_task (TASK_RRC_GNB, rrc_gnb_task, NULL) < 0) {
      LOG_E(NR_RRC, "Create task for NR RRC gNB failed\n");
      return -1;
    }
  }

  return 0;
}


static void get_options(void) {


  paramdef_t cmdline_params[] = CMDLINE_PARAMS_DESC_GNB ;

  config_process_cmdline( cmdline_params,sizeof(cmdline_params)/sizeof(paramdef_t),NULL);



  if ( !(CONFIG_ISFLAGSET(CONFIG_ABORT)) ) {
    memset((void *)&RC,0,sizeof(RC));
    /* Read RC configuration file */
    NRRCConfig();
    NB_gNB_INST = RC.nb_nr_inst;
    NB_RU   = RC.nb_RU;
    printf("Configuration: nb_rrc_inst %d, nb_nr_L1_inst %d, nb_ru %hhu\n",NB_gNB_INST,RC.nb_nr_L1_inst,NB_RU);
  }

  if(parallel_config != NULL) set_parallel_conf(parallel_config);

  if(worker_config != NULL) set_worker_conf(worker_config);

}


void set_default_frame_parms(nfapi_nr_config_request_scf_t *config[MAX_NUM_CCs],
		             NR_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs])
{
  for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    frame_parms[CC_id] = (NR_DL_FRAME_PARMS *) malloc(sizeof(NR_DL_FRAME_PARMS));
    config[CC_id] = (nfapi_nr_config_request_scf_t *) malloc(sizeof(nfapi_nr_config_request_scf_t));
    config[CC_id]->ssb_config.scs_common.value = 1;
    config[CC_id]->cell_config.frame_duplex_type.value = 1; //FDD
    //config[CC_id]->subframe_config.dl_cyclic_prefix_type.value = 0; //NORMAL
    config[CC_id]->carrier_config.dl_grid_size[1].value = 106;
    config[CC_id]->carrier_config.ul_grid_size[1].value = 106;
    config[CC_id]->cell_config.phy_cell_id.value = 0;
    ///dl frequency to be filled in
    /*  //Set some default values that may be overwritten while reading options
        frame_parms[CC_id]->frame_type          = FDD;
        frame_parms[CC_id]->tdd_config          = 3;
        frame_parms[CC_id]->tdd_config_S        = 0;
        frame_parms[CC_id]->N_RB_DL             = 100;
        frame_parms[CC_id]->N_RB_UL             = 100;
        frame_parms[CC_id]->Ncp                 = NORMAL;
        frame_parms[CC_id]->Ncp_UL              = NORMAL;
        frame_parms[CC_id]->Nid_cell            = 0;
        frame_parms[CC_id]->num_MBSFN_config    = 0;
        frame_parms[CC_id]->nb_antenna_ports_gNB  = 1;
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

    //    downlink_frequency[CC_id][0] = 2680000000; // Use float to avoid issue with frequency over 2^31.
    //    downlink_frequency[CC_id][1] = downlink_frequency[CC_id][0];
    //    downlink_frequency[CC_id][2] = downlink_frequency[CC_id][0];
    //    downlink_frequency[CC_id][3] = downlink_frequency[CC_id][0];
        //printf("Downlink for CC_id %d frequency set to %u\n", CC_id, downlink_frequency[CC_id][0]);
        frame_parms[CC_id]->dl_CarrierFreq=downlink_frequency[CC_id][0];
    */
  }
}

/*
void init_openair0(void) {

  int card;
  int i;

  for (card=0; card<MAX_CARDS; card++) {

    openair0_cfg[card].mmapped_dma=mmapped_dma;
    openair0_cfg[card].configFilename = NULL;

    if(config[0]->rf_config.dl_carrier_bandwidth.value == 100) {
      if (frame_parms[0]->threequarter_fs) {
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
    } else if(config[0]->rf_config.dl_carrier_bandwidth.value == 50) {
      openair0_cfg[card].sample_rate=15.36e6;
      openair0_cfg[card].samples_per_frame = 153600;
      openair0_cfg[card].tx_bw = 5e6;
      openair0_cfg[card].rx_bw = 5e6;
    } else if (config[0]->rf_config.dl_carrier_bandwidth.value == 25) {
      openair0_cfg[card].sample_rate=7.68e6;
      openair0_cfg[card].samples_per_frame = 76800;
      openair0_cfg[card].tx_bw = 2.5e6;
      openair0_cfg[card].rx_bw = 2.5e6;
    } else if (config[0]->rf_config.dl_carrier_bandwidth.value == 6) {
      openair0_cfg[card].sample_rate=1.92e6;
      openair0_cfg[card].samples_per_frame = 19200;
      openair0_cfg[card].tx_bw = 1.5e6;
      openair0_cfg[card].rx_bw = 1.5e6;
    }


    if (config[0]->subframe_config.duplex_mode.value==TDD)
      openair0_cfg[card].duplex_mode = duplex_mode_TDD;
    else //FDD
      openair0_cfg[card].duplex_mode = duplex_mode_FDD;

    printf("HW: Configuring card %d, nb_antennas_tx/rx %d/%d\n",card,
     RC.gNB[0]->gNB_config.rf_config.tx_antenna_ports.value,
     RC.gNB[0]->gNB_config.rf_config.tx_antenna_ports.value );
    openair0_cfg[card].Mod_id = 0;

    openair0_cfg[card].num_rb_dl=config[0]->rf_config.dl_carrier_bandwidth.value;

    openair0_cfg[card].clock_source = clock_source;


    openair0_cfg[card].tx_num_channels=min(2,RC.gNB[0]->gNB_config.rf_config.tx_antenna_ports.value );
    openair0_cfg[card].rx_num_channels=min(2,RC.gNB[0]->gNB_config.rf_config.tx_antenna_ports.value );

    for (i=0; i<4; i++) {

      if (i<openair0_cfg[card].tx_num_channels)
  openair0_cfg[card].tx_freq[i] = downlink_frequency[0][i] ;
      else
  openair0_cfg[card].tx_freq[i]=0.0;

      if (i<openair0_cfg[card].rx_num_channels)
  openair0_cfg[card].rx_freq[i] =downlink_frequency[0][i] + uplink_frequency_offset[0][i] ;
      else
  openair0_cfg[card].rx_freq[i]=0.0;

      openair0_cfg[card].autocal[i] = 1;
      openair0_cfg[card].tx_gain[i] = tx_gain[0][i];
      openair0_cfg[card].rx_gain[i] = RC.gNB[0]->rx_total_gain_dB;


      openair0_cfg[card].configFilename = get_softmodem_params()->rf_config_file;
      printf("Card %d, channel %d, Setting tx_gain %f, rx_gain %f, tx_freq %f, rx_freq %f\n",
       card,i, openair0_cfg[card].tx_gain[i],
       openair0_cfg[card].rx_gain[i],
       openair0_cfg[card].tx_freq[i],
       openair0_cfg[card].rx_freq[i]);
    }
  } // for loop on cards
}
*/

void wait_RUs(void) {
  LOG_I(PHY,"Waiting for RUs to be configured ... RC.ru_mask:%02lx\n", RC.ru_mask);
  // wait for all RUs to be configured over fronthaul
  pthread_mutex_lock(&RC.ru_mutex);

  while (RC.ru_mask>0) {
    pthread_cond_wait(&RC.ru_cond,&RC.ru_mutex);
    printf("RC.ru_mask:%02lx\n", RC.ru_mask);
  }

  pthread_mutex_unlock(&RC.ru_mutex);
  LOG_I(PHY,"RUs configured\n");
}

void wait_gNBs(void) {
  int i;
  int waiting=1;

  while (waiting==1) {
    printf("Waiting for gNB L1 instances to all get configured ... sleeping 50ms (nb_nr_sL1_inst %d)\n",RC.nb_nr_L1_inst);
    usleep(50*1000);
    waiting=0;

    for (i=0; i<RC.nb_nr_L1_inst; i++) {

      if (RC.gNB[i]->configured==0) {
	waiting=1;
	break;
      }
    }
  }

  printf("gNB L1 are configured\n");
}

/*
 * helper function to terminate a certain ITTI task
 */
void terminate_task(task_id_t task_id, module_id_t mod_id) {
  LOG_I(GNB_APP, "sending TERMINATE_MESSAGE to task %s (%d)\n", itti_get_task_name(task_id), task_id);
  MessageDef *msg;
  msg = itti_alloc_new_message (ENB_APP, TERMINATE_MESSAGE);
  itti_send_msg_to_task (task_id, ENB_MODULE_ID_TO_INSTANCE(mod_id), msg);
}

//extern void  free_transport(PHY_VARS_gNB *);
extern void  nr_phy_free_RU(RU_t *);

int stop_L1L2(module_id_t gnb_id) {
  LOG_W(GNB_APP, "stopping nr-softmodem\n");
  oai_exit = 1;

  if (!RC.ru) {
    LOG_F(GNB_APP, "no RU configured\n");
    return -1;
  }

  /* stop trx devices, multiple carrier currently not supported by RU */
  if (RC.ru[gnb_id]) {
    if (RC.ru[gnb_id]->rfdevice.trx_stop_func) {
      RC.ru[gnb_id]->rfdevice.trx_stop_func(&RC.ru[gnb_id]->rfdevice);
      LOG_I(GNB_APP, "turned off RU rfdevice\n");
    } else {
      LOG_W(GNB_APP, "can not turn off rfdevice due to missing trx_stop_func callback, proceeding anyway!\n");
    }

    if (RC.ru[gnb_id]->ifdevice.trx_stop_func) {
      RC.ru[gnb_id]->ifdevice.trx_stop_func(&RC.ru[gnb_id]->ifdevice);
      LOG_I(GNB_APP, "turned off RU ifdevice\n");
    } else {
      LOG_W(GNB_APP, "can not turn off ifdevice due to missing trx_stop_func callback, proceeding anyway!\n");
    }
  } else {
    LOG_W(GNB_APP, "no RU found for index %d\n", gnb_id);
    return -1;
  }

  /* these tasks need to pick up new configuration */
  terminate_task(TASK_RRC_ENB, gnb_id);
  terminate_task(TASK_L2L1, gnb_id);
  LOG_I(GNB_APP, "calling kill_gNB_proc() for instance %d\n", gnb_id);
  kill_gNB_proc(gnb_id);
  LOG_I(GNB_APP, "calling kill_NR_RU_proc() for instance %d\n", gnb_id);
  kill_NR_RU_proc(gnb_id);
  oai_exit = 0;

    //free_transport(RC.gNB[gnb_id]);
  phy_free_nr_gNB(RC.gNB[gnb_id]);


  nr_phy_free_RU(RC.ru[gnb_id]);
  free_lte_top();
  return 0;
}

/*
 * Restart the nr-softmodem after it has been soft-stopped with stop_L1L2()
 */
int restart_L1L2(module_id_t gnb_id) {
  RU_t *ru = RC.ru[gnb_id];
  MessageDef *msg_p = NULL;
  LOG_W(GNB_APP, "restarting nr-softmodem\n");
  /* block threads */
  sync_var = -1;

  RC.gNB[gnb_id]->configured = 0;
  

  RC.ru_mask |= (1 << ru->idx);
  /* copy the changed frame parameters to the RU */
  /* TODO this should be done for all RUs associated to this gNB */
  memcpy(&ru->nr_frame_parms, &RC.gNB[gnb_id]->frame_parms, sizeof(NR_DL_FRAME_PARMS));
  set_function_spec_param(RC.ru[gnb_id]);
  LOG_I(GNB_APP, "attempting to create ITTI tasks\n");
  // No more rrc thread, as many race conditions are hidden behind
  rrc_enb_init();
  itti_mark_task_ready(TASK_RRC_ENB);

  if (itti_create_task (TASK_L2L1, l2l1_task, NULL) < 0) {
    LOG_E(PDCP, "Create task for L2L1 failed\n");
    return -1;
  } else {
    LOG_I(PDCP, "Re-created task for L2L1 successfully\n");
  }

  /* pass a reconfiguration request which will configure everything down to
   * RC.eNB[i][j]->frame_parms, too */
  msg_p = itti_alloc_new_message(TASK_ENB_APP, RRC_CONFIGURATION_REQ);
  RRC_CONFIGURATION_REQ(msg_p) = RC.rrc[gnb_id]->configuration;
  itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(gnb_id), msg_p);
  /* TODO XForms might need to be restarted, but it is currently (09/02/18)
   * broken, so we cannot test it */
  wait_gNBs();
  init_RU_proc(ru);
  ru->rf_map.card = 0;
  ru->rf_map.chain = 0; /* CC_id + chain_offset;*/
  wait_RUs();
  init_eNB_afterRU();
  printf("Sending sync to all threads\n");
  pthread_mutex_lock(&sync_mutex);
  sync_var=0;
  pthread_cond_broadcast(&sync_cond);
  pthread_mutex_unlock(&sync_mutex);
  return 0;
}

static  void wait_nfapi_init(char *thread_name) {
  printf( "waiting for NFAPI PNF connection and population of global structure (%s)\n",thread_name);
  pthread_mutex_lock( &nfapi_sync_mutex );

  while (nfapi_sync_var<0)
    pthread_cond_wait( &nfapi_sync_cond, &nfapi_sync_mutex );

  pthread_mutex_unlock(&nfapi_sync_mutex);
  printf( "NFAPI: got sync (%s)\n", thread_name);
}

void init_pdcp(void) {
  //if (!NODE_IS_DU(RC.rrc[0]->node_type)) {
  pdcp_layer_init();
  uint32_t pdcp_initmask = (IS_SOFTMODEM_NOS1) ?
                           (PDCP_USE_NETLINK_BIT | LINK_ENB_PDCP_TO_IP_DRIVER_BIT) : LINK_ENB_PDCP_TO_GTPV1U_BIT;

  if (IS_SOFTMODEM_NOS1) {
    printf("IS_SOFTMODEM_NOS1 option enabled \n");
    pdcp_initmask = pdcp_initmask | ENB_NAS_USE_TUN_BIT | SOFTMODEM_NOKRNMOD_BIT  ;
  }

  pdcp_module_init(pdcp_initmask);

  /*if (NODE_IS_CU(RC.rrc[0]->node_type)) {
    pdcp_set_rlc_data_req_func((send_rlc_data_req_func_t)proto_agent_send_rlc_data_req);
  } else {*/
  pdcp_set_rlc_data_req_func((send_rlc_data_req_func_t) rlc_data_req);
  pdcp_set_pdcp_data_ind_func((pdcp_data_ind_func_t) pdcp_data_ind);
  //}
  /*} else {
    pdcp_set_pdcp_data_ind_func((pdcp_data_ind_func_t) proto_agent_send_pdcp_data_ind);
  }*/
}


int main( int argc, char **argv )
{
  int ru_id, CC_id = 0;
  start_background_system();

  ///static configuration for NR at the moment
  if ( load_configmodule(argc,argv,CONFIG_ENABLECMDLINEONLY) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }
  set_softmodem_sighandler();
#ifdef DEBUG_CONSOLE
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
#endif
  mode = normal_txrx;
  memset(&openair0_cfg[0],0,sizeof(openair0_config_t)*MAX_CARDS);
  memset(tx_max_power,0,sizeof(int)*MAX_NUM_CCs);
  logInit();
  configure_linux();
  printf("Reading in command-line options\n");
  get_options ();
  get_common_options(SOFTMODEM_GNB_BIT );

  if (CONFIG_ISFLAGSET(CONFIG_ABORT) ) {
    fprintf(stderr,"Getting configuration failed\n");
    exit(-1);
  }

  openair0_cfg[0].threequarter_fs = threequarter_fs;

#if T_TRACER
  T_Config_Init();
#endif
  //randominit (0);
  set_taus_seed (0);
  printf("configuring for RAU/RRU\n");

  if (opp_enabled ==1) {
    reset_opp_meas();
  }

  cpuf=get_cpu_freq_GHz();
  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info);
  // initialize mscgen log after ITTI
  MSC_INIT(MSC_E_UTRAN, THREAD_MAX+TASK_MAX);


  init_opt();


#ifdef PDCP_USE_NETLINK
if(!IS_SOFTMODEM_NOS1)
  netlink_init();
#if defined(PDCP_USE_NETLINK_QUEUES)
  pdcp_netlink_init();
#endif
#endif

#ifndef PACKAGE_VERSION
#  define PACKAGE_VERSION "UNKNOWN-EXPERIMENTAL"
#endif
  LOG_I(HW, "Version: %s\n", PACKAGE_VERSION);

  if(IS_SOFTMODEM_NOS1)
    init_pdcp();

  if (RC.nb_nr_inst > 0)  {
    // don't create if node doesn't connect to RRC/S1/GTP
    AssertFatal(create_gNB_tasks(1) == 0,"cannot create ITTI tasks\n");
  } else {
    printf("No ITTI, Initializing L1\n");
    RCconfig_L1();
  }

  /* Start the agent. If it is turned off in the configuration, it won't start */
  /*
  RCconfig_nr_flexran();

  for (i = 0; i < RC.nb_nr_L1_inst; i++) {
    flexran_agent_start(i);
  }
*/

  // init UE_PF_PO and mutex lock
  pthread_mutex_init(&ue_pf_po_mutex, NULL);
  memset (&UE_PF_PO[0][0], 0, sizeof(UE_PF_PO_t)*NUMBER_OF_UE_MAX*MAX_NUM_CCs);
  mlockall(MCL_CURRENT | MCL_FUTURE);
  pthread_cond_init(&sync_cond,NULL);
  pthread_mutex_init(&sync_mutex, NULL);

  usleep(1000);

  if (nfapi_mode) {
    printf("NFAPI*** - mutex and cond created - will block shortly for completion of PNF connection\n");
    pthread_cond_init(&sync_cond,NULL);
    pthread_mutex_init(&sync_mutex, NULL);
  }

  const char *nfapi_mode_str = "<UNKNOWN>";

  switch(nfapi_mode) {
    case 0:
      nfapi_mode_str = "MONOLITHIC";
      break;

    case 1:
      nfapi_mode_str = "PNF";
      break;

    case 2:
      nfapi_mode_str = "VNF";
      break;

    default:
      nfapi_mode_str = "<UNKNOWN NFAPI MODE>";
      break;
  }

  printf("NFAPI MODE:%s\n", nfapi_mode_str);

  if (nfapi_mode==2) // VNF
    wait_nfapi_init("main?");

  printf("START MAIN THREADS\n");
  // start the main threads
  number_of_cards = 1;
  printf("RC.nb_nr_L1_inst:%d\n", RC.nb_nr_L1_inst);

  if (RC.nb_nr_L1_inst > 0) {
    printf("Initializing gNB threads single_thread_flag:%d wait_for_sync:%d\n", single_thread_flag,wait_for_sync);
    init_gNB(single_thread_flag,wait_for_sync);
  }

  printf("wait_gNBs()\n");
  wait_gNBs();
  printf("About to Init RU threads RC.nb_RU:%d\n", RC.nb_RU);

  if (RC.nb_RU >0) {
    printf("Initializing RU threads\n");
    init_NR_RU(get_softmodem_params()->rf_config_file);

    for (ru_id=0; ru_id<RC.nb_RU; ru_id++) {
      RC.ru[ru_id]->rf_map.card=0;
      RC.ru[ru_id]->rf_map.chain=CC_id+chain_offset;
    }
  }

  config_sync_var=0;

  if (nfapi_mode==1) { // PNF
    wait_nfapi_init("main?");
  }

  printf("wait RUs\n");
  wait_RUs();
  printf("ALL RUs READY!\n");
  printf("RC.nb_RU:%d\n", RC.nb_RU);
  // once all RUs are ready initialize the rest of the gNBs ((dependence on final RU parameters after configuration)
  printf("ALL RUs ready - init gNBs\n");

  if (do_forms==1) {
    scopeParms_t p;
    p.argc=&argc;
    p.argv=argv;
    startScope(&p);
  }

  if (nfapi_mode != 1 && nfapi_mode != 2) {
    printf("Not NFAPI mode - call init_eNB_afterRU()\n");
    init_eNB_afterRU();
  } else {
    printf("NFAPI mode - DO NOT call init_gNB_afterRU()\n");
  }

  printf("ALL RUs ready - ALL gNBs ready\n");
  // connect the TX/RX buffers
  printf("Sending sync to all threads\n");
  pthread_mutex_lock(&sync_mutex);
  sync_var=0;
  pthread_cond_broadcast(&sync_cond);
  pthread_mutex_unlock(&sync_mutex);
  printf("About to call end_configmodule() from %s() %s:%d\n", __FUNCTION__, __FILE__, __LINE__);
  end_configmodule();
  printf("Called end_configmodule() from %s() %s:%d\n", __FUNCTION__, __FILE__, __LINE__);
  // wait for end of program
  printf("TYPE <CTRL-C> TO TERMINATE\n");
  //getchar();
  printf("Entering ITTI signals handler\n");
  itti_wait_tasks_end();
  printf("Returned from ITTI signal handler\n");
  oai_exit=1;
  printf("oai_exit=%d\n",oai_exit);

  // stop threads
  /*#ifdef XFORMS

      printf("waiting for XFORMS thread\n");

      if (do_forms==1) {
        pthread_join(forms_thread,&status);
        fl_hide_form(form_stats->stats_form);
        fl_free_form(form_stats->stats_form);

          fl_hide_form(form_stats_l2->stats_form);
          fl_free_form(form_stats_l2->stats_form);

          for(UE_id=0; UE_id<scope_enb_num_ue; UE_id++) {
      for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        fl_hide_form(form_enb[CC_id][UE_id]->phy_scope_gNB);
        fl_free_form(form_enb[CC_id][UE_id]->phy_scope_gNB);
      }
          }
      }

  #endif*/
  printf("stopping MODEM threads\n");
  // cleanup
  stop_gNB(NB_gNB_INST);
  stop_RU(NB_RU);

  /* release memory used by the RU/gNB threads (incomplete), after all
   * threads have been stopped (they partially use the same memory) */
  for (int inst = 0; inst < NB_gNB_INST; inst++) {
      //free_transport(RC.gNB[inst]);
      phy_free_nr_gNB(RC.gNB[inst]);
  }

  for (int inst = 0; inst < NB_RU; inst++) {
    nr_phy_free_RU(RC.ru[inst]);
  }

  free_lte_top();
  pthread_cond_destroy(&sync_cond);
  pthread_mutex_destroy(&sync_mutex);
  pthread_cond_destroy(&nfapi_sync_cond);
  pthread_mutex_destroy(&nfapi_sync_mutex);
  pthread_mutex_destroy(&ue_pf_po_mutex);

  // *** Handle per CC_id openair0

  for(ru_id=0; ru_id<NB_RU; ru_id++) {
    if (RC.ru[ru_id]->rfdevice.trx_end_func)
      RC.ru[ru_id]->rfdevice.trx_end_func(&RC.ru[ru_id]->rfdevice);

    if (RC.ru[ru_id]->ifdevice.trx_end_func)
      RC.ru[ru_id]->ifdevice.trx_end_func(&RC.ru[ru_id]->ifdevice);
  }


  logClean();
  printf("Bye.\n");
  return 0;
}
