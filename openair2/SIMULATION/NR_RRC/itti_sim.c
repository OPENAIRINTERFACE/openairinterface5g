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

/*! \file itti_sim.c
 * \brief simulator for itti message from node to UE
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 * \date 2020
 * \version 0.1
 */


#include <sched.h>


#include "T.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
#include <common/utils/assertions.h>

#include "msc.h"

#include "common/ran_context.h"

#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/phy_vars.h"
#include "SCHED/sched_common_vars.h"
#include "LAYER2/MAC/mac_vars.h"
#include "RRC/LTE/rrc_vars.h"
#include "gnb_config.h"
#include "SIMULATION/TOOLS/sim.h"

#ifdef SMBV
#include "PHY/TOOLS/smbv.h"
unsigned short config_frames[4] = {2,9,11,13};
#endif

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"

#include "intertask_interface.h"


#include "system.h"
#include <openair2/GNB_APP/gnb_app.h>
#include "PHY/TOOLS/phy_scope_interface.h"
#include "PHY/TOOLS/nr_phy_scope.h"
#include "stats.h"
#include "nr-softmodem.h"
#include "executables/softmodem-common.h"
#include "executables/thread-common.h"
#include "NB_IoT_interface.h"
#include "x2ap_eNB.h"
#include "ngap_gNB.h"
#include "RRC/NR_UE/rrc_proto.h"
#include "RRC/NR_UE/rrc_vars.h"
#include "openair3/NAS/UE/nas_ue_task.h"
#include <executables/split_headers.h>
#include <executables/nr-uesoftmodem.h>
#if ITTI_SIM
#include "nr_nas_msg_sim.h"
#endif

pthread_cond_t nfapi_sync_cond;
pthread_mutex_t nfapi_sync_mutex;
int nfapi_sync_var=-1; //!< protected by mutex \ref nfapi_sync_mutex

uint32_t timing_advance = 0;
uint64_t num_missed_slots=0;

int split73=0;
void sendFs6Ul(PHY_VARS_eNB *eNB, int UE_id, int harq_pid, int segmentID, int16_t *data, int dataLen, int r_offset) {
  AssertFatal(false, "Must not be called in this context\n");
}
void sendFs6Ulharq(enum pckType type, int UEid, PHY_VARS_eNB *eNB, LTE_eNB_UCI *uci, int frame, int subframe, uint8_t *harq_ack, uint8_t tdd_mapping_mode, uint16_t tdd_multiplexing_mask, uint16_t rnti, int32_t stat) {
  AssertFatal(false, "Must not be called in this context\n");
}

nrUE_params_t nrUE_params;
nrUE_params_t *get_nrUE_params(void) {
  return &nrUE_params;
}

void processSlotTX(void *arg) {}

THREAD_STRUCT thread_struct;
pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.
int config_sync_var=-1;

openair0_config_t openair0_cfg[MAX_CARDS];

//volatile int             start_gNB = 0;
volatile int             oai_exit = 0;

//static int wait_for_sync = 0;

unsigned int mmapped_dma=0;
int single_thread_flag=1;

int8_t threequarter_fs=0;

uint64_t downlink_frequency[MAX_NUM_CCs][4];
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];

//Temp fix for inexistent NR upper layer
unsigned char NB_gNB_INST = 1;


int UE_scan = 1;
int UE_scan_carrier = 0;
runmode_t mode = normal_txrx;
static double snr_dB=20;

FILE *input_fd=NULL;

int chain_offset=0;


uint8_t dci_Format = 0;
uint8_t agregation_Level =0xFF;

uint8_t nb_antenna_tx = 1;
uint8_t nb_antenna_rx = 1;

char ref[128] = "internal";
char channels[128] = "0";

int otg_enabled;

extern void *udp_eNB_task(void *args_p);

int transmission_mode=1;
int emulate_rf = 0;
int numerology = 0;


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


int stop_L1L2(module_id_t gnb_id) {
  return 0;
}

int restart_L1L2(module_id_t gnb_id) {
  return 0;
}

int create_gNB_tasks(uint32_t gnb_nb) {
  LOG_D(GNB_APP, "%s(gnb_nb:%d)\n", __FUNCTION__, gnb_nb);
  itti_wait_ready(1);


  if (gnb_nb > 0) {
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

  if (AMF_MODE_ENABLED && (get_softmodem_params()->phy_test==0 && get_softmodem_params()->do_ra==0 && get_softmodem_params()->sa==0)) {
    if (gnb_nb > 0) {
      if (itti_create_task (TASK_NGAP, ngap_gNB_task, NULL) < 0) {
        LOG_E(S1AP, "Create task for NGAP failed\n");
        return -1;
      }


      if(!emulate_rf){
        if (itti_create_task (TASK_UDP, udp_eNB_task, NULL) < 0) {
          LOG_E(UDP_, "Create task for UDP failed\n");
          return -1;
        }
      }

      if (itti_create_task (TASK_GTPV1_U, &nr_gtpv1u_gNB_task, NULL) < 0) {
        LOG_E(GTPU, "Create task for GTPV1U failed\n");
        return -1;
      }
    }
  }


  if (gnb_nb > 0) {
    if (itti_create_task (TASK_GNB_APP, gNB_app_task, NULL) < 0) {
      LOG_E(GNB_APP, "Create task for gNB APP failed\n");
      return -1;
    }
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

  CONFIG_SETRTFLAG(CONFIG_NOEXITONHELP);
  get_common_options(SOFTMODEM_GNB_BIT );
  config_process_cmdline( cmdline_params,sizeof(cmdline_params)/sizeof(paramdef_t),NULL);
  CONFIG_CLEARRTFLAG(CONFIG_NOEXITONHELP);




  if ( !(CONFIG_ISFLAGSET(CONFIG_ABORT)) ) {
    memset((void *)&RC,0,sizeof(RC));
    /* Read RC configuration file */
    NRRCConfig();
    NB_gNB_INST = RC.nb_nr_inst;
    NB_RU   = RC.nb_RU;
    printf("Configuration: nb_rrc_inst %d, nb_nr_L1_inst %d, nb_ru %hhu\n",NB_gNB_INST,RC.nb_nr_L1_inst,NB_RU);
  }

}


void set_default_frame_parms(nfapi_nr_config_request_scf_t *config[MAX_NUM_CCs],
		             NR_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs])
{
  for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    frame_parms[CC_id] = (NR_DL_FRAME_PARMS *) malloc(sizeof(NR_DL_FRAME_PARMS));
    config[CC_id] = (nfapi_nr_config_request_scf_t *) malloc(sizeof(nfapi_nr_config_request_scf_t));
    config[CC_id]->ssb_config.scs_common.value = 1;
    config[CC_id]->cell_config.frame_duplex_type.value = 1; //FDD
    config[CC_id]->carrier_config.dl_grid_size[1].value = 106;
    config[CC_id]->carrier_config.ul_grid_size[1].value = 106;
    config[CC_id]->cell_config.phy_cell_id.value = 0;
  }
}

/*
 * helper function to terminate a certain ITTI task
 */
void terminate_task(task_id_t task_id, module_id_t mod_id) {
  LOG_I(GNB_APP, "sending TERMINATE_MESSAGE to task %s (%d)\n", itti_get_task_name(task_id), task_id);
  MessageDef *msg;
  msg = itti_alloc_new_message (ENB_APP, 0, TERMINATE_MESSAGE);
  itti_send_msg_to_task (task_id, ENB_MODULE_ID_TO_INSTANCE(mod_id), msg);
}

//extern void  free_transport(PHY_VARS_gNB *);
extern void  nr_phy_free_RU(RU_t *);

statis void init_pdcp(void) {
  //if (!NODE_IS_DU(RC.rrc[0]->node_type)) {
  pdcp_layer_init();
  uint32_t pdcp_initmask = (IS_SOFTMODEM_NOS1) ?
                           (PDCP_USE_NETLINK_BIT | LINK_ENB_PDCP_TO_IP_DRIVER_BIT) : LINK_ENB_PDCP_TO_GTPV1U_BIT;

  if (IS_SOFTMODEM_NOS1) {
    printf("IS_SOFTMODEM_NOS1 option enabled \n");
    pdcp_initmask = pdcp_initmask | ENB_NAS_USE_TUN_BIT | SOFTMODEM_NOKRNMOD_BIT  ;
  }

  pdcp_module_init(pdcp_initmask, 0);

  pdcp_set_rlc_data_req_func(rlc_data_req);
  pdcp_set_pdcp_data_ind_func(pdcp_data_ind);
}


int create_tasks_nrue(uint32_t ue_nb) {
  LOG_D(ENB_APP, "%s(ue_nb:%d)\n", __FUNCTION__, ue_nb);
  itti_wait_ready(1);

  if (ue_nb > 0) {
    LOG_D(NR_RRC, "create TASK_RRC_NRUE\n");
    if (itti_create_task (TASK_RRC_NRUE, rrc_nrue_task, NULL) < 0) {
      LOG_E(NR_RRC, "Create task for RRC UE failed\n");
      return -1;
    }

    LOG_D(NR_RRC,"create TASK_NAS_NRUE\n");
    if (itti_create_task (TASK_NAS_NRUE, nas_nrue_task, NULL) < 0) {
      LOG_E(NR_RRC, "Create task for NAS UE failed\n");
      return -1;
    }
  }


  itti_wait_ready(0);
  return 0;
}


void *itti_sim_ue_rrc_task( void *args_p) {
  MessageDef   *msg_p, *message_p;
  instance_t    instance;
  unsigned int  ue_mod_id;
  int           result;
  itti_mark_task_ready (TASK_RRC_UE_SIM);

  while(1) {
    // Wait for a message
    itti_receive_msg (TASK_RRC_UE_SIM, &msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);
    ue_mod_id = UE_INSTANCE_TO_MODULE_ID(instance);

    switch (ITTI_MSG_ID(msg_p)) {
      case TERMINATE_MESSAGE:
        LOG_W(NR_RRC, " *** Exiting RRC thread\n");
        itti_exit_task ();
        break;

      case MESSAGE_TEST:
        LOG_D(NR_RRC, "[UE %d] Received %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
        break;
      case GNB_RRC_BCCH_DATA_IND:
          message_p = itti_alloc_new_message (TASK_RRC_UE_SIM, 0, NR_RRC_MAC_BCCH_DATA_IND);
          memset (NR_RRC_MAC_BCCH_DATA_IND (message_p).sdu, 0, BCCH_SDU_SIZE);
          NR_RRC_MAC_BCCH_DATA_IND (message_p).sdu_size  = GNB_RRC_BCCH_DATA_IND(msg_p).size;
          memcpy (NR_RRC_MAC_BCCH_DATA_IND (message_p).sdu, GNB_RRC_BCCH_DATA_IND(msg_p).sdu, GNB_RRC_BCCH_DATA_IND(msg_p).size);
          itti_send_msg_to_task (TASK_RRC_NRUE, instance, message_p);
        break;
      case GNB_RRC_CCCH_DATA_IND:
        message_p = itti_alloc_new_message (TASK_RRC_UE_SIM, 0, NR_RRC_MAC_CCCH_DATA_IND);
        printf("receive GNB_RRC_CCCH_DATA_IND\n");
        memset (NR_RRC_MAC_CCCH_DATA_IND (message_p).sdu, 0, CCCH_SDU_SIZE);
        memcpy (NR_RRC_MAC_CCCH_DATA_IND (message_p).sdu, GNB_RRC_CCCH_DATA_IND(msg_p).sdu, GNB_RRC_CCCH_DATA_IND(msg_p).size);
        NR_RRC_MAC_CCCH_DATA_IND (message_p).sdu_size  = GNB_RRC_CCCH_DATA_IND(msg_p).size;
        itti_send_msg_to_task (TASK_RRC_NRUE, instance, message_p);
        break;
      case GNB_RRC_DCCH_DATA_IND:
        printf("receive GNB_RRC_DCCH_DATA_IND\n");
        message_p = itti_alloc_new_message (TASK_RRC_UE_SIM, 0, NR_RRC_DCCH_DATA_IND);
        NR_RRC_DCCH_DATA_IND (message_p).dcch_index = GNB_RRC_DCCH_DATA_IND(msg_p).rbid;
        NR_RRC_DCCH_DATA_IND (message_p).sdu_size   = GNB_RRC_DCCH_DATA_IND(msg_p).size;
        NR_RRC_DCCH_DATA_IND (message_p).sdu_p      = GNB_RRC_DCCH_DATA_IND(msg_p).sdu;
        itti_send_msg_to_task (TASK_RRC_NRUE, instance, message_p);
        break;
      default:
        LOG_E(NR_RRC, "[UE %d] Received unexpected message %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
        break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    msg_p = NULL;
  }
}


void *itti_sim_gnb_rrc_task( void *args_p) {
  MessageDef   *msg_p, *message_p;
  instance_t    instance;
  unsigned int  ue_mod_id;
  int           result;
  itti_mark_task_ready (TASK_RRC_GNB_SIM);

  while(1) {
    // Wait for a message
    itti_receive_msg (TASK_RRC_GNB_SIM, &msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);
    ue_mod_id = UE_INSTANCE_TO_MODULE_ID(instance);

    switch (ITTI_MSG_ID(msg_p)) {
      case TERMINATE_MESSAGE:
        LOG_W(NR_RRC, " *** Exiting RRC thread\n");
        itti_exit_task ();
        break;

      case MESSAGE_TEST:
        LOG_D(NR_RRC, "[UE %d] Received %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
        break;
      case UE_RRC_CCCH_DATA_IND:
          message_p = itti_alloc_new_message (TASK_RRC_GNB_SIM, 0, NR_RRC_MAC_CCCH_DATA_IND);
          NR_RRC_MAC_CCCH_DATA_IND (message_p).sdu_size = UE_RRC_CCCH_DATA_IND(msg_p).size;
          memset (NR_RRC_MAC_CCCH_DATA_IND (message_p).sdu, 0, CCCH_SDU_SIZE);
          memcpy (NR_RRC_MAC_CCCH_DATA_IND (message_p).sdu, UE_RRC_CCCH_DATA_IND(msg_p).sdu, UE_RRC_CCCH_DATA_IND(msg_p).size);
          itti_send_msg_to_task (TASK_RRC_GNB, instance, message_p);
           break;
      case UE_RRC_DCCH_DATA_IND:
    	    message_p = itti_alloc_new_message (TASK_RRC_GNB_SIM, 0, NR_RRC_DCCH_DATA_IND);
    	    NR_RRC_DCCH_DATA_IND (message_p).sdu_size   = UE_RRC_DCCH_DATA_IND(msg_p).size;
          NR_RRC_DCCH_DATA_IND (message_p).dcch_index = UE_RRC_DCCH_DATA_IND(msg_p).rbid;
          NR_RRC_DCCH_DATA_IND (message_p).sdu_p      = UE_RRC_DCCH_DATA_IND(msg_p).sdu;
    	    itti_send_msg_to_task (TASK_RRC_GNB, instance, message_p);
           break;

      default:
        LOG_E(NR_RRC, "[UE %d] Received unexpected message %s\n", ue_mod_id, ITTI_MSG_NAME (msg_p));
        break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    msg_p = NULL;
  }
}


int main( int argc, char **argv )
{
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

  logInit();
  //set_latency_target();
  printf("Reading in command-line options\n");
  get_options ();

  if (CONFIG_ISFLAGSET(CONFIG_ABORT) ) {
    fprintf(stderr,"Getting configuration failed\n");
    exit(-1);
  }

  AMF_MODE_ENABLED = !IS_SOFTMODEM_NOS1;
//  AMF_MODE_ENABLED = 0;
  NGAP_CONF_MODE   = !IS_SOFTMODEM_NOS1; //!get_softmodem_params()->phy_test;

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
  itti_init(TASK_MAX, tasks_info);
  // initialize mscgen log after ITTI
  MSC_INIT(MSC_E_UTRAN, ADDED_QUEUES_MAX+TASK_MAX);


  init_opt();

  LOG_I(HW, "Version: %s\n", PACKAGE_VERSION);

  if(IS_SOFTMODEM_NOS1)
    init_pdcp();

  if (RC.nb_nr_inst > 0)  {
    nr_read_config_and_init();
    // don't create if node doesn't connect to RRC/S1/GTP
    AssertFatal(create_gNB_tasks(1) == 0,"cannot create ITTI tasks\n");
    for (int gnb_id = 0; gnb_id < RC.nb_nr_inst; gnb_id++) {
      MessageDef *msg_p = itti_alloc_new_message (TASK_GNB_APP, 0, NRRRC_CONFIGURATION_REQ);
      NRRRC_CONFIGURATION_REQ(msg_p) = RC.nrrrc[gnb_id]->configuration;
      itti_send_msg_to_task (TASK_RRC_GNB, GNB_MODULE_ID_TO_INSTANCE(gnb_id), msg_p);
    }
  } else {
    printf("No ITTI, Initializing L1\n");
    return 0;
  }

  if(itti_create_task (TASK_RRC_GNB_SIM, itti_sim_gnb_rrc_task, NULL) < 0){
    printf("cannot create ITTI tasks\n");
    exit(-1); // need a softer mode
  }

  openair_rrc_top_init_ue_nr("./");

  if (create_tasks_nrue(1) < 0) {
    printf("cannot create ITTI tasks\n");
    exit(-1); // need a softer mode
  }

  if(itti_create_task (TASK_RRC_UE_SIM, itti_sim_ue_rrc_task, NULL) < 0){
    printf("cannot create ITTI tasks\n");
    exit(-1); // need a softer mode
  }

  pthread_cond_init(&sync_cond,NULL);
  pthread_mutex_init(&sync_mutex, NULL);

  printf("Sending sync to all threads\n");
  pthread_mutex_lock(&sync_mutex);
  sync_var=0;
  pthread_cond_broadcast(&sync_cond);
  pthread_mutex_unlock(&sync_mutex);
  // wait for end of program
  printf("TYPE <CTRL-C> TO TERMINATE\n");

  usleep(100000);
  protocol_ctxt_t ctxt;
  struct rrc_gNB_ue_context_s *ue_context_p = NULL;


  ue_context_p = rrc_gNB_allocate_new_UE_context(RC.nrrrc[0]);

  if(ue_context_p == NULL){
    printf("ue_context_p == NULL");
  }
  PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt,
                                0,
                                ENB_FLAG_YES,
                                0,
                                0,
                                0);
  NR_UE_rrc_inst[ctxt.module_id].Info[0].State = RRC_SI_RECEIVED;

  nr_rrc_ue_generate_RRCSetupRequest(ctxt.module_id, 0);

  printf("Entering ITTI signals handler\n");
  itti_wait_tasks_end();
  printf("Returned from ITTI signal handler\n");
  oai_exit=1;
  printf("oai_exit=%d\n",oai_exit);

  printf("stopping MODEM threads\n");


  /* release memory used by the RU/gNB threads (incomplete), after all
   * threads have been stopped (they partially use the same memory) */

  pthread_cond_destroy(&sync_cond);
  pthread_mutex_destroy(&sync_mutex);

  logClean();
  printf("Bye.\n");
  return 0;
}



