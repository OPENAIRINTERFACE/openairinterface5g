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

#include "PHY/defs_eNB.h"
#include "common/ran_context.h"
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

#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_proto.h"
#include "RRC/LTE/rrc_vars.h"
#include "PHY_INTERFACE/phy_interface_vars.h"
#include "nfapi/oai_integration/vendor_ext.h"
#ifdef SMBV
#include "PHY/TOOLS/smbv.h"
unsigned short config_frames[4] = {2,9,11,13};
#endif
#include "common/utils/LOG/log.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "enb_config.h"
//#include "PHY/TOOLS/time_meas.h"

#ifndef OPENAIR2
  #include "UTIL/OTG/otg_vars.h"
#endif


#include "create_tasks.h"


#include "PHY/INIT/phy_init.h"

#include "system.h"

#ifdef XFORMS
  #include "PHY/TOOLS/lte_phy_scope.h"
  #include "stats.h"
#endif
#include "lte-softmodem.h"
#include "NB_IoT_interface.h"
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


uint16_t sf_ahead=4;

pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.
int config_sync_var=-1;

uint16_t runtime_phy_rx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]
uint16_t runtime_phy_tx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]


volatile int             oai_exit = 0;

uint32_t                 downlink_frequency[MAX_NUM_CCs][4];
int32_t                  uplink_frequency_offset[MAX_NUM_CCs][4];

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


uint8_t dci_Format = 0;
uint8_t agregation_Level =0xFF;

uint8_t nb_antenna_tx = 1;
uint8_t nb_antenna_rx = 1;

char ref[128] = "internal";
char channels[128] = "0";

int                      rx_input_level_dBm;

#ifdef XFORMS
  extern int                      otg_enabled;
#else
  int                             otg_enabled;
#endif
//int                             number_of_cards =   1;


uint8_t exit_missed_slots=1;
uint64_t num_missed_slots=0; // counter for the number of missed slots


extern void reset_opp_meas(void);
extern void print_opp_meas(void);


extern void init_eNB_afterRU(void);

int transmission_mode=1;
int emulate_rf = 0;
int numerology = 0;

THREAD_STRUCT thread_struct;
/* struct for ethernet specific parameters given in eNB conf file */
eth_params_t *eth_params;

double cpuf;



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
    printf("Linux signal %s...\n",strsignal(sig));
    exit_function(__FILE__, __FUNCTION__, __LINE__,"softmodem starting exit procedure\n");
  }
}


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
# ifdef ENABLE_XFORMS_WRITE_STATS
  FILE *eNB_stats;
# endif
  struct sched_param sched_param;
  int UE_id, CC_id;
  int ue_cnt=0;
  sched_param.sched_priority = sched_get_priority_min(SCHED_FIFO)+1;
  sched_setscheduler(0, SCHED_FIFO,&sched_param);
  printf("Scope thread has priority %d\n",sched_param.sched_priority);
# ifdef ENABLE_XFORMS_WRITE_STATS
  eNB_stats = fopen("eNB_stats.txt", "w");
#endif

  while (!oai_exit) {
    ue_cnt=0;

    for(UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
      for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        if ((ue_cnt<scope_enb_num_ue)) {
          phy_scope_eNB(form_enb[CC_id][ue_cnt],
                        RC.eNB[0][CC_id],
                        UE_id);
          ue_cnt++;
        }
      }
    }

    sleep(1);
  }

  //  printf("%s",stats_buffer);
# ifdef ENABLE_XFORMS_WRITE_STATS

  if (eNB_stats) {
    rewind (eNB_stats);
    fwrite (stats_buffer, 1, len, eNB_stats);
    fclose (eNB_stats);
  }

# endif
  pthread_exit((void *)arg);
}
#endif


static void get_options(void) {
  CONFIG_SETRTFLAG(CONFIG_NOEXITONHELP);
  get_common_options();
  CONFIG_CLEARRTFLAG(CONFIG_NOEXITONHELP);

  if ( !(CONFIG_ISFLAGSET(CONFIG_ABORT)) ) {
    memset((void *)&RC,0,sizeof(RC));
    /* Read RC configuration file */
    RCConfig();
    NB_eNB_INST = RC.nb_inst;
    printf("Configuration: nb_rrc_inst %d, nb_L1_inst %d, nb_ru %d\n",NB_eNB_INST,RC.nb_L1_inst,RC.nb_RU);

    if (!IS_SOFTMODEM_NONBIOT) {
      load_NB_IoT();
      printf("               nb_nbiot_rrc_inst %d, nb_nbiot_L1_inst %d, nb_nbiot_macrlc_inst %d\n",
             RC.nb_nb_iot_rrc_inst, RC.nb_nb_iot_L1_inst, RC.nb_nb_iot_macrlc_inst);
    } else {
      printf("All Nb-IoT instances disabled\n");
      RC.nb_nb_iot_rrc_inst=RC.nb_nb_iot_L1_inst=RC.nb_nb_iot_macrlc_inst=0;
    }
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
    //    downlink_frequency[CC_id][0] = 2680000000; // Use float to avoid issue with frequency over 2^31.
    //    downlink_frequency[CC_id][1] = downlink_frequency[CC_id][0];
    //    downlink_frequency[CC_id][2] = downlink_frequency[CC_id][0];
    //    downlink_frequency[CC_id][3] = downlink_frequency[CC_id][0];
    //printf("Downlink for CC_id %d frequency set to %u\n", CC_id, downlink_frequency[CC_id][0]);
    frame_parms[CC_id]->dl_CarrierFreq=downlink_frequency[CC_id][0];
  }
}

void wait_RUs(void) {
  /* do not modify the following LOG_UI message, which is used by CI */
  LOG_UI(ENB_APP,"Waiting for RUs to be configured ... RC.ru_mask:%02lx\n", RC.ru_mask);
  // wait for all RUs to be configured over fronthaul
  pthread_mutex_lock(&RC.ru_mutex);

  while (RC.ru_mask>0) {
    pthread_cond_wait(&RC.ru_cond,&RC.ru_mutex);
    printf("RC.ru_mask:%02lx\n", RC.ru_mask);
  }

  pthread_mutex_unlock(&RC.ru_mutex);
  LOG_I(PHY,"RUs configured\n");
}

void wait_eNBs(void) {
  int i,j;
  int waiting=1;

  while (waiting==1) {
    printf("Waiting for eNB L1 instances to all get configured ... sleeping 50ms (nb_L1_inst %d)\n",RC.nb_L1_inst);
    usleep(50*1000);
    waiting=0;

    for (i=0; i<RC.nb_L1_inst; i++) {
      printf("RC.nb_L1_CC[%d]:%d\n", i, RC.nb_L1_CC[i]);

      for (j=0; j<RC.nb_L1_CC[i]; j++) {
        if (RC.eNB[i][j]->configured==0) {
          waiting=1;
          break;
        }
      }
    }
  }

  printf("eNB L1 are configured\n");
}


/*
 * helper function to terminate a certain ITTI task
 */
void terminate_task(module_id_t mod_id, task_id_t from, task_id_t to) {
  LOG_I(ENB_APP, "sending TERMINATE_MESSAGE from task %s (%d) to task %s (%d)\n",
        itti_get_task_name(from), from, itti_get_task_name(to), to);
  MessageDef *msg;
  msg = itti_alloc_new_message (from, TERMINATE_MESSAGE);
  itti_send_msg_to_task (to, ENB_MODULE_ID_TO_INSTANCE(mod_id), msg);
}

extern void  free_transport(PHY_VARS_eNB *);
extern void  phy_free_RU(RU_t *);

int stop_L1L2(module_id_t enb_id) {
  LOG_W(ENB_APP, "stopping lte-softmodem\n");

  if (!RC.ru) {
    LOG_UI(ENB_APP, "no RU configured\n");
    return -1;
  }

  /* these tasks need to pick up new configuration */
  terminate_task(enb_id, TASK_ENB_APP, TASK_RRC_ENB);
  oai_exit = 1;
  LOG_I(ENB_APP, "calling kill_RU_proc() for instance %d\n", enb_id);
  kill_RU_proc(RC.ru[enb_id]);
  LOG_I(ENB_APP, "calling kill_eNB_proc() for instance %d\n", enb_id);
  kill_eNB_proc(enb_id);
  oai_exit = 0;

  for (int cc_id = 0; cc_id < RC.nb_CC[enb_id]; cc_id++) {
    free_transport(RC.eNB[enb_id][cc_id]);
    phy_free_lte_eNB(RC.eNB[enb_id][cc_id]);
  }

  phy_free_RU(RC.ru[enb_id]);
  free_lte_top();
  return 0;
}

/*
 * Restart the lte-softmodem after it has been soft-stopped with stop_L1L2()
 */
int restart_L1L2(module_id_t enb_id) {
  RU_t *ru = RC.ru[enb_id];
  int cc_id;
  MessageDef *msg_p = NULL;
  LOG_W(ENB_APP, "restarting lte-softmodem\n");
  /* block threads */
  pthread_mutex_lock(&sync_mutex);
  sync_var = -1;
  pthread_mutex_unlock(&sync_mutex);

  for (cc_id = 0; cc_id < RC.nb_L1_CC[enb_id]; cc_id++) {
    RC.eNB[enb_id][cc_id]->configured = 0;
  }

  RC.ru_mask |= (1 << ru->idx);
  /* copy the changed frame parameters to the RU */
  /* TODO this should be done for all RUs associated to this eNB */
  memcpy(&ru->frame_parms, &RC.eNB[enb_id][0]->frame_parms, sizeof(LTE_DL_FRAME_PARMS));
  set_function_spec_param(RC.ru[enb_id]);
  /* reset the list of connected UEs in the MAC, since in this process with
   * loose all UEs (have to reconnect) */
  init_UE_list(&RC.mac[enb_id]->UE_list);
  LOG_I(ENB_APP, "attempting to create ITTI tasks\n");

  if (itti_create_task (TASK_RRC_ENB, rrc_enb_task, NULL) < 0) {
    LOG_E(RRC, "Create task for RRC eNB failed\n");
    return -1;
  } else {
    LOG_I(RRC, "Re-created task for RRC eNB successfully\n");
  }

  /* pass a reconfiguration request which will configure everything down to
   * RC.eNB[i][j]->frame_parms, too */
  msg_p = itti_alloc_new_message(TASK_ENB_APP, RRC_CONFIGURATION_REQ);
  RRC_CONFIGURATION_REQ(msg_p) = RC.rrc[enb_id]->configuration;
  itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
  /* TODO XForms might need to be restarted, but it is currently (09/02/18)
   * broken, so we cannot test it */
  wait_eNBs();
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

void init_pdcp(void) {
  if (!NODE_IS_DU(RC.rrc[0]->node_type)) {
    pdcp_layer_init();
    uint32_t pdcp_initmask = (IS_SOFTMODEM_NOS1) ?
        (PDCP_USE_NETLINK_BIT | LINK_ENB_PDCP_TO_IP_DRIVER_BIT) : LINK_ENB_PDCP_TO_GTPV1U_BIT;
    if (IS_SOFTMODEM_NOS1)
      pdcp_initmask = pdcp_initmask | ENB_NAS_USE_TUN_BIT | SOFTMODEM_NOKRNMOD_BIT  ;
    pdcp_module_init(pdcp_initmask);

    if (NODE_IS_CU(RC.rrc[0]->node_type)) {
      pdcp_set_rlc_data_req_func((send_rlc_data_req_func_t)proto_agent_send_rlc_data_req);
    } else {
      pdcp_set_rlc_data_req_func((send_rlc_data_req_func_t) rlc_data_req);
      pdcp_set_pdcp_data_ind_func((pdcp_data_ind_func_t) pdcp_data_ind);
    }
  } else {
    pdcp_set_pdcp_data_ind_func((pdcp_data_ind_func_t) proto_agent_send_pdcp_data_ind);
  }
}

static  void wait_nfapi_init(char *thread_name) {
  printf( "waiting for NFAPI PNF connection and population of global structure (%s)\n",thread_name);
  pthread_mutex_lock( &nfapi_sync_mutex );

  while (nfapi_sync_var<0)
    pthread_cond_wait( &nfapi_sync_cond, &nfapi_sync_mutex );

  pthread_mutex_unlock(&nfapi_sync_mutex);
  printf( "NFAPI: got sync (%s)\n", thread_name);
}

int main( int argc, char **argv ) {
  int i;
#if defined (XFORMS)
  void *status;
#endif
  int CC_id = 0;
  int ru_id;
#if defined (XFORMS)
  int ret;
#endif

  if ( load_configmodule(argc,argv,0) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }

  mode = normal_txrx;
  set_latency_target();
  logInit();
  printf("Reading in command-line options\n");
  get_options ();

  if (is_nos1exec(argv[0]) )
    set_softmodem_optmask(SOFTMODEM_NOS1_BIT);

  EPC_MODE_ENABLED = !IS_SOFTMODEM_NOS1;

  if (CONFIG_ISFLAGSET(CONFIG_ABORT) ) {
    fprintf(stderr,"Getting configuration failed\n");
    exit(-1);
  }

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
  printf("ITTI init, useMME: %i\n",EPC_MODE_ENABLED);
  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info);

  // initialize mscgen log after ITTI
  if (get_softmodem_params()->start_msc) {
    load_module_shlib("msc",NULL,0,&msc_interface);
  }

  MSC_INIT(MSC_E_UTRAN, THREAD_MAX+TASK_MAX);
  init_opt();
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
  printf("Runtime table\n");
  fill_modeled_runtime_table(runtime_phy_rx,runtime_phy_tx);

  /* Read configuration */
  if (RC.nb_inst > 0) {
    read_config_and_init();

    /* Start the agent. If it is turned off in the configuration, it won't start */
    RCconfig_flexran();
    for (i = 0; i < RC.nb_inst; i++) {
      flexran_agent_start(i);
    }

    /* initializes PDCP and sets correct RLC Request/PDCP Indication callbacks
     * for monolithic/F1 modes */
    init_pdcp();

    if (create_tasks(1) < 0) {
      printf("cannot create ITTI tasks\n");
      exit(-1);
    }

    for (int enb_id = 0; enb_id < RC.nb_inst; enb_id++) {
      MessageDef *msg_p = itti_alloc_new_message (TASK_ENB_APP, RRC_CONFIGURATION_REQ);
      RRC_CONFIGURATION_REQ(msg_p) = RC.rrc[enb_id]->configuration;
      itti_send_msg_to_task (TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
    }
  } else {
    printf("RC.nb_inst = 0, Initializing L1\n");
    RCconfig_L1();
  }

  if (RC.nb_inst > 0 && NODE_IS_CU(RC.rrc[0]->node_type)) {
    protocol_ctxt_t ctxt;
    ctxt.module_id = 0 ;
    ctxt.instance = 0;
    ctxt.rnti = 0;
    ctxt.enb_flag = 1;
    pdcp_run(&ctxt);
  }

  /* start threads if only L1 or not a CU */
  if (RC.nb_inst == 0 || !NODE_IS_CU(RC.rrc[0]->node_type)) {
    // init UE_PF_PO and mutex lock
    pthread_mutex_init(&ue_pf_po_mutex, NULL);
    memset (&UE_PF_PO[0][0], 0, sizeof(UE_PF_PO_t)*MAX_MOBILES_PER_ENB*MAX_NUM_CCs);
    mlockall(MCL_CURRENT | MCL_FUTURE);
    pthread_cond_init(&sync_cond,NULL);
    pthread_mutex_init(&sync_mutex, NULL);
#ifdef XFORMS
    int UE_id;
    printf("XFORMS\n");

    if (get_softmodem_params()->do_forms==1) {
      fl_initialize (&argc, argv, NULL, 0, 0);
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

      ret = pthread_create(&forms_thread, NULL, scope_thread, NULL);

      if (ret == 0)
        pthread_setname_np( forms_thread, "xforms" );

      printf("Scope thread created, ret=%d\n",ret);
    }

#endif
    rt_sleep_ns(10*100000000ULL);

    if (NFAPI_MODE!=NFAPI_MONOLITHIC) {
      LOG_I(ENB_APP,"NFAPI*** - mutex and cond created - will block shortly for completion of PNF connection\n");
      pthread_cond_init(&sync_cond,NULL);
      pthread_mutex_init(&sync_mutex, NULL);
    }

    if (NFAPI_MODE==NFAPI_MODE_VNF) {// VNF
#if defined(PRE_SCD_THREAD)
      init_ru_vnf();  // ru pointer is necessary for pre_scd.
#endif
      wait_nfapi_init("main?");
    }

    LOG_I(ENB_APP,"START MAIN THREADS\n");
    // start the main threads
    number_of_cards = 1;
    printf("RC.nb_L1_inst:%d\n", RC.nb_L1_inst);

    if (RC.nb_L1_inst > 0) {
      printf("Initializing eNB threads single_thread_flag:%d wait_for_sync:%d\n", get_softmodem_params()->single_thread_flag,get_softmodem_params()->wait_for_sync);
      init_eNB(get_softmodem_params()->single_thread_flag,get_softmodem_params()->wait_for_sync);
      //      for (inst=0;inst<RC.nb_L1_inst;inst++)
      //  for (CC_id=0;CC_id<RC.nb_L1_CC[inst];CC_id++) phy_init_lte_eNB(RC.eNB[inst][CC_id],0,0);
    }

    printf("wait_eNBs()\n");
    wait_eNBs();
    printf("About to Init RU threads RC.nb_RU:%d\n", RC.nb_RU);

    // RU thread and some L1 procedure aren't necessary in VNF or L2 FAPI simulator.
    // but RU thread deals with pre_scd and this is necessary in VNF and simulator.
    // some initialization is necessary and init_ru_vnf do this.
    if (RC.nb_RU >0 && NFAPI_MODE!=NFAPI_MODE_VNF) {
      printf("Initializing RU threads\n");
      init_RU(get_softmodem_params()->rf_config_file);

      for (ru_id=0; ru_id<RC.nb_RU; ru_id++) {
        RC.ru[ru_id]->rf_map.card=0;
        RC.ru[ru_id]->rf_map.chain=CC_id+(get_softmodem_params()->chain_offset);
      }
    }

    config_sync_var=0;

    if (NFAPI_MODE==NFAPI_MODE_PNF) { // PNF
      wait_nfapi_init("main?");
    }

    printf("wait RUs\n");
    // CI -- Flushing the std outputs for the previous marker to show on the eNB / RRU log file
    fflush(stdout);
    fflush(stderr);
    // end of CI modifications
    wait_RUs();
    LOG_I(ENB_APP,"RC.nb_RU:%d\n", RC.nb_RU);
    // once all RUs are ready intiailize the rest of the eNBs ((dependence on final RU parameters after configuration)
    printf("ALL RUs ready - init eNBs\n");

    if (NFAPI_MODE!=NFAPI_MODE_PNF && NFAPI_MODE!=NFAPI_MODE_VNF) {
      LOG_I(ENB_APP,"Not NFAPI mode - call init_eNB_afterRU()\n");
      init_eNB_afterRU();
    } else {
      LOG_I(ENB_APP,"NFAPI mode - DO NOT call init_eNB_afterRU()\n");
    }

    LOG_UI(ENB_APP,"ALL RUs ready - ALL eNBs ready\n");
    // connect the TX/RX buffers
    sleep(1); /* wait for thread activation */
    LOG_I(ENB_APP,"Sending sync to all threads\n");
    pthread_mutex_lock(&sync_mutex);
    sync_var=0;
    pthread_cond_broadcast(&sync_cond);
    pthread_mutex_unlock(&sync_mutex);
    config_check_unknown_cmdlineopt(CONFIG_CHECKALLSECTIONS);
  }

  // wait for end of program
  LOG_UI(ENB_APP,"TYPE <CTRL-C> TO TERMINATE\n");
  // CI -- Flushing the std outputs for the previous marker to show on the eNB / DU / CU log file
  fflush(stdout);
  fflush(stderr);
  // end of CI modifications
  //getchar();
  itti_wait_tasks_end();
  oai_exit=1;
  LOG_I(ENB_APP,"oai_exit=%d\n",oai_exit);
  // stop threads

  if (RC.nb_inst == 0 || !NODE_IS_CU(RC.rrc[0]->node_type)) {
    int UE_id;
#ifdef XFORMS
    printf("waiting for XFORMS thread\n");

    if (get_softmodem_params()->do_forms==1) {
      pthread_join(forms_thread,&status);
      fl_hide_form(form_stats->stats_form);
      fl_free_form(form_stats->stats_form);
      fl_hide_form(form_stats_l2->stats_form);
      fl_free_form(form_stats_l2->stats_form);

      for(UE_id=0; UE_id<scope_enb_num_ue; UE_id++) {
        for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
          fl_hide_form(form_enb[CC_id][UE_id]->lte_phy_scope_enb);
          fl_free_form(form_enb[CC_id][UE_id]->lte_phy_scope_enb);
        }
      }
    }

#endif
    LOG_I(ENB_APP,"stopping MODEM threads\n");
    stop_eNB(NB_eNB_INST);
    stop_RU(RC.nb_RU);

    /* release memory used by the RU/eNB threads (incomplete), after all
     * threads have been stopped (they partially use the same memory) */
    for (int inst = 0; inst < NB_eNB_INST; inst++) {
      for (int cc_id = 0; cc_id < RC.nb_CC[inst]; cc_id++) {
        free_transport(RC.eNB[inst][cc_id]);
        phy_free_lte_eNB(RC.eNB[inst][cc_id]);
      }
    }

    for (int inst = 0; inst < RC.nb_RU; inst++) {
      phy_free_RU(RC.ru[inst]);
    }

    free_lte_top();
    end_configmodule();
    pthread_cond_destroy(&sync_cond);
    pthread_mutex_destroy(&sync_mutex);
    pthread_cond_destroy(&nfapi_sync_cond);
    pthread_mutex_destroy(&nfapi_sync_mutex);
    pthread_mutex_destroy(&ue_pf_po_mutex);

    for(ru_id=0; ru_id<RC.nb_RU; ru_id++) {
      if (RC.ru[ru_id]->rfdevice.trx_end_func) {
        RC.ru[ru_id]->rfdevice.trx_end_func(&RC.ru[ru_id]->rfdevice);
        RC.ru[ru_id]->rfdevice.trx_end_func = NULL;
      }

      if (RC.ru[ru_id]->ifdevice.trx_end_func) {
        RC.ru[ru_id]->ifdevice.trx_end_func(&RC.ru[ru_id]->ifdevice);
        RC.ru[ru_id]->ifdevice.trx_end_func = NULL;
      }
    }
  }

  terminate_opt();
  logClean();
  printf("Bye.\n");
  return 0;
}
