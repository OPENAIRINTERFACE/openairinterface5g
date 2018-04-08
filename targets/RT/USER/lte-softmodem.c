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


#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>


#include "T.h"

#include "rt_wrapper.h"


#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "assertions.h"
#include "msc.h"

#include "PHY/types.h"

#include "PHY/defs.h"
#include "common/ran_context.h"
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
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
#include "intertask_interface_init.h"
#include "create_tasks.h"
#endif

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

uint8_t nfapi_mode = 0; // Default to monolithic mode

pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.
int config_sync_var=-1;

uint16_t runtime_phy_rx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]
uint16_t runtime_phy_tx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]

#if defined(ENABLE_ITTI)
volatile int             start_eNB = 0;
volatile int             start_UE = 0;
#endif
volatile int             oai_exit = 0;

static clock_source_t clock_source = internal;
static int wait_for_sync = 0;

unsigned int                    mmapped_dma=0;
int                             single_thread_flag=1;

static int8_t                     threequarter_fs=0;

uint32_t                 downlink_frequency[MAX_NUM_CCs][4];
int32_t                  uplink_frequency_offset[MAX_NUM_CCs][4];



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

uint8_t dci_Format = 0;
uint8_t agregation_Level =0xFF;

uint8_t nb_antenna_tx = 1;
uint8_t nb_antenna_rx = 1;

char ref[128] = "internal";
char channels[128] = "0";

int                      rx_input_level_dBm;

#ifdef XFORMS
extern int                      otg_enabled;
static char                     do_forms=0;
#else
int                             otg_enabled;
#endif
//int                             number_of_cards =   1;


static LTE_DL_FRAME_PARMS      *frame_parms[MAX_NUM_CCs];
uint32_t target_dl_mcs = 28; //maximum allowed mcs
uint32_t target_ul_mcs = 20;
uint32_t timing_advance = 0;
uint8_t exit_missed_slots=1;
uint64_t num_missed_slots=0; // counter for the number of missed slots


extern void reset_opp_meas(void);
extern void print_opp_meas(void);

extern PHY_VARS_UE* init_ue_vars(LTE_DL_FRAME_PARMS *frame_parms,
			  uint8_t UE_id,
			  uint8_t abstraction_flag);

extern void init_eNB_afterRU(void);

int transmission_mode=1;



/* struct for ethernet specific parameters given in eNB conf file */
eth_params_t *eth_params;

openair0_config_t openair0_cfg[MAX_CARDS];

double cpuf;

extern char uecap_xer[1024];
char uecap_xer_in=0;

int oaisim_flag=0;
threads_t threads= {-1,-1,-1,-1,-1,-1,-1};

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

#if !defined(ENABLE_ITTI)
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



void exit_fun(const char* s)
{

  int ru_id;

  if (s != NULL) {
    printf("%s %s() Exiting OAI softmodem: %s\n",__FILE__, __FUNCTION__, s);
  }

  oai_exit = 1;


    if (RC.ru == NULL)
        exit(-1); // likely init not completed, prevent crash or hang, exit now...
    for (ru_id=0; ru_id<RC.nb_RU;ru_id++) {
      if (RC.ru[ru_id] && RC.ru[ru_id]->rfdevice.trx_end_func)
	RC.ru[ru_id]->rfdevice.trx_end_func(&RC.ru[ru_id]->rfdevice);
      if (RC.ru[ru_id] && RC.ru[ru_id]->ifdevice.trx_end_func)
	RC.ru[ru_id]->ifdevice.trx_end_func(&RC.ru[ru_id]->ifdevice);  
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

  pthread_exit((void*)arg);
}
#endif




#if defined(ENABLE_ITTI)
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
	/* Start eNB thread */
	LOG_D(EMU, "L2L1 TASK received %s\n", ITTI_MSG_NAME(message_p));
	start_eNB = 1;
	break;

      case TERMINATE_MESSAGE:
	printf("received terminate message\n");
	oai_exit=1;
        start_eNB = 0;
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
#endif


static void get_options(void) {
 
  int tddflag, nonbiotflag;
 
  
  uint32_t online_log_messages;
  uint32_t glog_level, glog_verbosity;
  uint32_t start_telnetsrv;

  paramdef_t cmdline_params[] =CMDLINE_PARAMS_DESC ;
  paramdef_t cmdline_logparams[] =CMDLINE_LOGPARAMS_DESC ;

  config_process_cmdline( cmdline_params,sizeof(cmdline_params)/sizeof(paramdef_t),NULL); 

  if (strlen(in_path) > 0) {
      opt_type = OPT_PCAP;
      opt_enabled=1;
      printf("Enabling OPT for PCAP  with the following file %s \n",in_path);
  }
  if (strlen(in_ip) > 0) {
      opt_enabled=1;
      opt_type = OPT_WIRESHARK;
      printf("Enabling OPT for wireshark for local interface");
  }

  config_process_cmdline( cmdline_logparams,sizeof(cmdline_logparams)/sizeof(paramdef_t),NULL);
  if(config_isparamset(cmdline_logparams,CMDLINE_ONLINELOG_IDX)) {
      set_glog_onlinelog(online_log_messages);
  }
  if(config_isparamset(cmdline_logparams,CMDLINE_GLOGLEVEL_IDX)) {
      set_glog(glog_level, -1);
  }
  if(config_isparamset(cmdline_logparams,CMDLINE_GLOGVERBO_IDX)) {
      set_glog(-1, glog_verbosity);
  }
  if (start_telnetsrv) {
     load_module_shlib("telnetsrv",NULL,0);
  }

#if T_TRACER
  paramdef_t cmdline_ttraceparams[] =CMDLINE_TTRACEPARAMS_DESC ;
  config_process_cmdline( cmdline_ttraceparams,sizeof(cmdline_ttraceparams)/sizeof(paramdef_t),NULL);   
#endif

  if ( !(CONFIG_ISFLAGSET(CONFIG_ABORT)) ) {
      memset((void*)&RC,0,sizeof(RC));
      /* Read RC configuration file */
      RCConfig();
      NB_eNB_INST = RC.nb_inst;
      NB_RU	  = RC.nb_RU;
      printf("Configuration: nb_rrc_inst %d, nb_L1_inst %d, nb_ru %d\n",NB_eNB_INST,RC.nb_L1_inst,NB_RU);
      if (nonbiotflag <= 0) {
         load_NB_IoT();
         printf("               nb_nbiot_rrc_inst %d, nb_nbiot_L1_inst %d, nb_nbiot_macrlc_inst %d\n",
                RC.nb_nb_iot_rrc_inst, RC.nb_nb_iot_L1_inst, RC.nb_nb_iot_macrlc_inst);
      } else {
         printf("All Nb-IoT instances disabled\n");
         RC.nb_nb_iot_rrc_inst=RC.nb_nb_iot_L1_inst=RC.nb_nb_iot_macrlc_inst=0;
      }
   }
}


#if T_TRACER
int T_nowait = 0;     /* by default we wait for the tracer */
int T_port = 2021;    /* default port to listen to to wait for the tracer */
int T_dont_fork = 0;  /* default is to fork, see 'T_init' to understand */
#endif



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


void init_openair0(void) {

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
      } else {
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

    printf("HW: Configuring card %d, nb_antennas_tx/rx %d/%d\n",card,
	   RC.eNB[0][0]->frame_parms.nb_antennas_tx ,
	   RC.eNB[0][0]->frame_parms.nb_antennas_rx );
    openair0_cfg[card].Mod_id = 0;

    openair0_cfg[card].num_rb_dl=frame_parms[0]->N_RB_DL;

    openair0_cfg[card].clock_source = clock_source;


    openair0_cfg[card].tx_num_channels=min(2,RC.eNB[0][0]->frame_parms.nb_antennas_tx );
    openair0_cfg[card].rx_num_channels=min(2,RC.eNB[0][0]->frame_parms.nb_antennas_rx );

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
      openair0_cfg[card].rx_gain[i] = RC.eNB[0][0]->rx_total_gain_dB;


      openair0_cfg[card].configFilename = rf_config_file;
      printf("Card %d, channel %d, Setting tx_gain %f, rx_gain %f, tx_freq %f, rx_freq %f\n",
	     card,i, openair0_cfg[card].tx_gain[i],
	     openair0_cfg[card].rx_gain[i],
	     openair0_cfg[card].tx_freq[i],
	     openair0_cfg[card].rx_freq[i]);
    }
  } /* for loop on cards */
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

void wait_eNBs(void) {

  int i,j;
  int waiting=1;


  while (waiting==1) {
    printf("Waiting for eNB L1 instances to all get configured ... sleeping 50ms (nb_L1_inst %d)\n",RC.nb_L1_inst);
    usleep(50*1000);
    waiting=0;
    for (i=0;i<RC.nb_L1_inst;i++) {

      printf("RC.nb_L1_CC[%d]:%d\n", i, RC.nb_L1_CC[i]);

      for (j=0;j<RC.nb_L1_CC[i];j++) {
	if (RC.eNB[i][j]->configured==0) {
	  waiting=1;
	  break;
        } 
      }
    }
  }
  printf("eNB L1 are configured\n");
}

#if defined(ENABLE_ITTI)
/*
 * helper function to terminate a certain ITTI task
 */
void terminate_task(task_id_t task_id, module_id_t mod_id)
{
  LOG_I(ENB_APP, "sending TERMINATE_MESSAGE to task %s (%d)\n", itti_get_task_name(task_id), task_id);
  MessageDef *msg;
  msg = itti_alloc_new_message (ENB_APP, TERMINATE_MESSAGE);
  itti_send_msg_to_task (task_id, ENB_MODULE_ID_TO_INSTANCE(mod_id), msg);
}

extern void  free_transport(PHY_VARS_eNB *);
extern void  phy_free_RU(RU_t*);

int stop_L1L2(module_id_t enb_id)
{
  LOG_W(ENB_APP, "stopping lte-softmodem\n");
  oai_exit = 1;

  if (!RC.ru) {
    LOG_F(ENB_APP, "no RU configured\n");
    return -1;
  }

  /* stop trx devices, multiple carrier currently not supported by RU */
  if (RC.ru[enb_id]) {
    if (RC.ru[enb_id]->rfdevice.trx_stop_func) {
      RC.ru[enb_id]->rfdevice.trx_stop_func(&RC.ru[enb_id]->rfdevice);
      LOG_I(ENB_APP, "turned off RU rfdevice\n");
    } else {
      LOG_W(ENB_APP, "can not turn off rfdevice due to missing trx_stop_func callback, proceding anyway!\n");
    }
    if (RC.ru[enb_id]->ifdevice.trx_stop_func) {
      RC.ru[enb_id]->ifdevice.trx_stop_func(&RC.ru[enb_id]->ifdevice);
      LOG_I(ENB_APP, "turned off RU ifdevice\n");
    } else {
      LOG_W(ENB_APP, "can not turn off ifdevice due to missing trx_stop_func callback, proceding anyway!\n");
    }
  } else {
    LOG_W(ENB_APP, "no RU found for index %d\n", enb_id);
    return -1;
  }

  /* these tasks need to pick up new configuration */
  terminate_task(TASK_RRC_ENB, enb_id);
  terminate_task(TASK_L2L1, enb_id);
  LOG_I(ENB_APP, "calling kill_eNB_proc() for instance %d\n", enb_id);
  kill_eNB_proc(enb_id);
  LOG_I(ENB_APP, "calling kill_RU_proc() for instance %d\n", enb_id);
  kill_RU_proc(enb_id);
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
int restart_L1L2(module_id_t enb_id)
{
  RU_t *ru = RC.ru[enb_id];
  int cc_id;
  MessageDef *msg_p = NULL;

  LOG_W(ENB_APP, "restarting lte-softmodem\n");

  /* block threads */
  sync_var = -1;

  for (cc_id = 0; cc_id < RC.nb_L1_CC[enb_id]; cc_id++) {
    RC.eNB[enb_id][cc_id]->configured = 0;
  }

  RC.ru_mask |= (1 << ru->idx);
  /* copy the changed frame parameters to the RU */
  /* TODO this should be done for all RUs associated to this eNB */
  memcpy(&ru->frame_parms, &RC.eNB[enb_id][0]->frame_parms, sizeof(LTE_DL_FRAME_PARMS));
  set_function_spec_param(RC.ru[enb_id]);

  LOG_I(ENB_APP, "attempting to create ITTI tasks\n");
  if (itti_create_task (TASK_RRC_ENB, rrc_enb_task, NULL) < 0) {
    LOG_E(RRC, "Create task for RRC eNB failed\n");
    return -1;
  } else {
    LOG_I(RRC, "Re-created task for RRC eNB successfully\n");
  }
  if (itti_create_task (TASK_L2L1, l2l1_task, NULL) < 0) {
    LOG_E(PDCP, "Create task for L2L1 failed\n");
    return -1;
  } else {
    LOG_I(PDCP, "Re-created task for L2L1 successfully\n");
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
#endif

static  void wait_nfapi_init(char *thread_name) {

  printf( "waiting for NFAPI PNF connection and population of global structure (%s)\n",thread_name);
  pthread_mutex_lock( &nfapi_sync_mutex );
  
  while (nfapi_sync_var<0)
    pthread_cond_wait( &nfapi_sync_cond, &nfapi_sync_mutex );
  
  pthread_mutex_unlock(&nfapi_sync_mutex);
  
  printf( "NFAPI: got sync (%s)\n", thread_name);
}

int main( int argc, char **argv )
{
  int i;
#if defined (XFORMS)
  void *status;
#endif

  int CC_id;
  int ru_id;
#if defined (XFORMS)
  int ret;
#endif

  if ( load_configmodule(argc,argv) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  } 
      
#ifdef DEBUG_CONSOLE
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
#endif

  mode = normal_txrx;
  memset(&openair0_cfg[0],0,sizeof(openair0_config_t)*MAX_CARDS);

  memset(tx_max_power,0,sizeof(int)*MAX_NUM_CCs);

  set_latency_target();

  logInit();

  printf("Reading in command-line options\n");

  get_options (); 
  if (CONFIG_ISFLAGSET(CONFIG_ABORT) ) {
      fprintf(stderr,"Getting configuration failed\n");
      exit(-1);
  }


#if T_TRACER
  T_init(T_port, 1-T_nowait, T_dont_fork);
#endif



  //randominit (0);
  set_taus_seed (0);

  printf("configuring for RAU/RRU\n");


  if (ouput_vcd) {
      VCD_SIGNAL_DUMPER_INIT("/tmp/openair_dump_eNB.vcd");
  }

  if (opp_enabled ==1) {
    reset_opp_meas();
  }
  cpuf=get_cpu_freq_GHz();

#if defined(ENABLE_ITTI)
  log_set_instance_type (LOG_INSTANCE_ENB);

  printf("ITTI init\n");
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
  printf("PDCP netlink\n");
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

#ifndef PACKAGE_VERSION
#  define PACKAGE_VERSION "UNKNOWN-EXPERIMENTAL"
#endif

  LOG_I(HW, "Version: %s\n", PACKAGE_VERSION);




  printf("Before CC \n");

  printf("Runtime table\n");
  fill_modeled_runtime_table(runtime_phy_rx,runtime_phy_tx);


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
  memset(cpu_affinity, 0 , sizeof(cpu_affinity));
  for (int j = 0; j < CPU_SETSIZE; j++) {
    if (CPU_ISSET(j, &cpuset)) {
      char temp[1024];
      sprintf(temp, " CPU_%d ", j);
      strcat(cpu_affinity, temp);
    }
  }
  LOG_I(HW, "CPU Affinity of main() function is... %s\n", cpu_affinity);
#endif
  

  
  
#if defined(ENABLE_ITTI)
  if (RC.nb_inst > 0)  {
    
    // don't create if node doesn't connect to RRC/S1/GTP
      if (create_tasks(1) < 0) {
        printf("cannot create ITTI tasks\n");
        exit(-1); // need a softer mode
      }
    printf("ITTI tasks created\n");
  }
  else {
    printf("No ITTI, Initializing L1\n");
    RCconfig_L1();
  }
#endif

  /* Start the agent. If it is turned off in the configuration, it won't start */
  RCconfig_flexran();
  for (i = 0; i < RC.nb_L1_inst; i++) {
    flexran_agent_start(i);
  }

  // init UE_PF_PO and mutex lock
  pthread_mutex_init(&ue_pf_po_mutex, NULL);
  memset (&UE_PF_PO[0][0], 0, sizeof(UE_PF_PO_t)*NUMBER_OF_UE_MAX*MAX_NUM_CCs);
  
  mlockall(MCL_CURRENT | MCL_FUTURE);
  
  pthread_cond_init(&sync_cond,NULL);
  pthread_mutex_init(&sync_mutex, NULL);
  
#ifdef XFORMS
  int UE_id;
  
  printf("XFORMS\n");

  if (do_forms==1) {
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
    printf("RC.nb_L1_inst:%d\n", RC.nb_L1_inst);
    if (RC.nb_L1_inst > 0) {
      printf("Initializing eNB threads single_thread_flag:%d wait_for_sync:%d\n", single_thread_flag,wait_for_sync);
      init_eNB(single_thread_flag,wait_for_sync);
      //      for (inst=0;inst<RC.nb_L1_inst;inst++)
      //	for (CC_id=0;CC_id<RC.nb_L1_CC[inst];CC_id++) phy_init_lte_eNB(RC.eNB[inst][CC_id],0,0);
    }

    printf("wait_eNBs()\n");
    wait_eNBs();

    printf("About to Init RU threads RC.nb_RU:%d\n", RC.nb_RU);
    if (RC.nb_RU >0) {
      printf("Initializing RU threads\n");
      init_RU(rf_config_file);
      for (ru_id=0;ru_id<RC.nb_RU;ru_id++) {
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
    // once all RUs are ready intiailize the rest of the eNBs ((dependence on final RU parameters after configuration)
    printf("ALL RUs ready - init eNBs\n");

    if (nfapi_mode != 1 && nfapi_mode != 2)
    {
      printf("Not NFAPI mode - call init_eNB_afterRU()\n");
      init_eNB_afterRU();
    }
    else
    {
      printf("NFAPI mode - DO NOT call init_eNB_afterRU()\n");
    }
    
    printf("ALL RUs ready - ALL eNBs ready\n");
  
  
  // connect the TX/RX buffers
 
  
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

  if (do_forms==1) {
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

  printf("stopping MODEM threads\n");

  // cleanup
    stop_eNB(NB_eNB_INST);
    stop_RU(NB_RU);
    /* release memory used by the RU/eNB threads (incomplete), after all
     * threads have been stopped (they partially use the same memory) */
    for (int inst = 0; inst < NB_eNB_INST; inst++) {
      for (int cc_id = 0; cc_id < RC.nb_CC[inst]; cc_id++) {
        free_transport(RC.eNB[inst][cc_id]);
        phy_free_lte_eNB(RC.eNB[inst][cc_id]);
      }
    }
    for (int inst = 0; inst < NB_RU; inst++) {
      phy_free_RU(RC.ru[inst]);
    }
    free_lte_top();

  printf("About to call end_configmodule() from %s() %s:%d\n", __FUNCTION__, __FILE__, __LINE__);
  end_configmodule();
  printf("Called end_configmodule() from %s() %s:%d\n", __FUNCTION__, __FILE__, __LINE__);

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
  if (ouput_vcd)
    VCD_SIGNAL_DUMPER_CLOSE();
  
  if (opt_enabled == 1)
    terminate_opt();
  
  logClean();

  printf("Bye.\n");
  
  return 0;
}
