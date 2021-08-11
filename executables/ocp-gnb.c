/*
 * Author: Laurent Thomas, Open Cells Project
 * all rights reserved
 */

#define _GNU_SOURCE
#include <pthread.h>
#include "assertions.h"
#include <common/utils/LOG/log.h>
#include <common/utils/system.h>

#include "PHY/types.h"

#include "PHY/INIT/phy_init.h"

#include "PHY/defs_gNB.h"
#include "SCHED/sched_eNB.h"
#include "SCHED_NR/sched_nr.h"
#include "SCHED_NR/fapi_nr_l1.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "../../ARCH/COMMON/common_lib.h"

#include "PHY/phy_extern.h"
#include "LAYER2/MAC/mac.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "LAYER2/MAC/mac_proto.h"
#include "RRC/LTE/rrc_extern.h"
#include "PHY_INTERFACE/phy_interface.h"
#include "common/utils/LOG/log_extern.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "enb_config.h"
#include "s1ap_eNB.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"
#include <executables/nr-softmodem.h>
#include <openair2/GNB_APP/gnb_config.h>
#include <executables/softmodem-common.h>
#include <openair2/GNB_APP/gnb_app.h>
#include <openair2/RRC/NR/nr_rrc_extern.h>
#include <openair2/X2AP/x2ap_eNB.h>
#include <openair1/PHY/NR_TRANSPORT/nr_transport_proto.h>
#include <nfapi/oai_integration/nfapi_pnf.h>

// should be in a shared lib
#include <forms.h>
#include <executables/stats.h>
#include <openair1/PHY/TOOLS/nr_phy_scope.h>
#include <openair1/PHY/TOOLS/phy_scope_interface.h>

// Global vars
#include <openair2/LAYER2/MAC/mac_vars.h>
#include <openair1/PHY/phy_vars.h>
#include <openair2/RRC/LTE/rrc_vars.h>
#include <openair1/SCHED/sched_common_vars.h>
volatile int oai_exit;
int single_thread_flag=1;
uint32_t do_forms=0;
unsigned int mmapped_dma=0;
int8_t threequarter_fs=0;

int chain_offset=0;
uint16_t sl_ahead=6;
uint16_t sf_ahead=6;
uint32_t timing_advance = 0;
int transmission_mode=1;
int emulate_rf = 0;
int numerology = 0;


int config_sync_var=-1;
pthread_mutex_t nfapi_sync_mutex;
pthread_cond_t nfapi_sync_cond;
int nfapi_sync_var=-1;
double cpuf;

THREAD_STRUCT thread_struct;

pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.


uint64_t downlink_frequency[MAX_NUM_CCs][4];
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];
time_stats_t softmodem_stats_mt; // main thread
time_stats_t softmodem_stats_hw; //  hw acquisition
time_stats_t softmodem_stats_rxtx_sf; // total tx time
time_stats_t nfapi_meas; // total tx time
time_stats_t softmodem_stats_rx_sf; // total rx time
// not used but needed for link
openair0_config_t openair0_cfg[MAX_CARDS];
uint16_t slot_ahead=6;
msc_interface_t msc_interface;
AGENT_RRC_xface *agent_rrc_xface[NUM_MAX_ENB];
AGENT_MAC_xface *agent_mac_xface[NUM_MAX_ENB];
int flexran_agent_start(mid_t mod_id) {
  memset (agent_rrc_xface, 0, sizeof(agent_rrc_xface));
  memset (agent_mac_xface, 0, sizeof(agent_mac_xface));
  return 0;
}
void flexran_agent_slice_update(mid_t module_idP) {
}
int proto_agent_start(mod_id_t mod_id, const cudu_params_t *p) {
  return 0;
}
void proto_agent_stop(mod_id_t mod_id) {
}
int split73=0;
void sendFs6Ul(PHY_VARS_eNB *eNB, int UE_id, int harq_pid, int segmentID, int16_t *data, int dataLen, int r_offset) {
  AssertFatal(false, "Must not be called in this context\n");
}
int stop_L1L2(module_id_t gnb_id) {
  AssertFatal(false, "Must not be called in this context\n");
}
int restart_L1L2(module_id_t gnb_id) {
  AssertFatal(false, "Must not be called in this context\n");
}

static int wait_for_sync = 0;
static double snr_dB=20;
static int DEFBANDS[] = {7};
static int DEFENBS[] = {0};
static int DEFBFW[] = {0x00007fff};

extern double cpuf;

short nr_mod_table[NR_MOD_TABLE_SIZE_SHORT] = {0,0,16384,16384,-16384,-16384,16384,16384,16384,-16384,-16384,16384,-16384,-16384,7327,7327,7327,21981,21981,7327,21981,21981,7327,-7327,7327,-21981,21981,-7327,21981,-21981,-7327,7327,-7327,21981,-21981,7327,-21981,21981,-7327,-7327,-7327,-21981,-21981,-7327,-21981,-21981,10726,10726,10726,3576,3576,10726,3576,3576,10726,17876,10726,25027,3576,17876,3576,25027,17876,10726,17876,3576,25027,10726,25027,3576,17876,17876,17876,25027,25027,17876,25027,25027,10726,-10726,10726,-3576,3576,-10726,3576,-3576,10726,-17876,10726,-25027,3576,-17876,3576,-25027,17876,-10726,17876,-3576,25027,-10726,25027,-3576,17876,-17876,17876,-25027,25027,-17876,25027,-25027,-10726,10726,-10726,3576,-3576,10726,-3576,3576,-10726,17876,-10726,25027,-3576,17876,-3576,25027,-17876,10726,-17876,3576,-25027,10726,-25027,3576,-17876,17876,-17876,25027,-25027,17876,-25027,25027,-10726,-10726,-10726,-3576,-3576,-10726,-3576,-3576,-10726,-17876,-10726,-25027,-3576,-17876,-3576,-25027,-17876,-10726,-17876,-3576,-25027,-10726,-25027,-3576,-17876,-17876,-17876,-25027,-25027,-17876,-25027,-25027,8886,8886,8886,12439,12439,8886,12439,12439,8886,5332,8886,1778,12439,5332,12439,1778,5332,8886,5332,12439,1778,8886,1778,12439,5332,5332,5332,1778,1778,5332,1778,1778,8886,19547,8886,15993,12439,19547,12439,15993,8886,23101,8886,26655,12439,23101,12439,26655,5332,19547,5332,15993,1778,19547,1778,15993,5332,23101,5332,26655,1778,23101,1778,26655,19547,8886,19547,12439,15993,8886,15993,12439,19547,5332,19547,1778,15993,5332,15993,1778,23101,8886,23101,12439,26655,8886,26655,12439,23101,5332,23101,1778,26655,5332,26655,1778,19547,19547,19547,15993,15993,19547,15993,15993,19547,23101,19547,26655,15993,23101,15993,26655,23101,19547,23101,15993,26655,19547,26655,15993,23101,23101,23101,26655,26655,23101,26655,26655,8886,-8886,8886,-12439,12439,-8886,12439,-12439,8886,-5332,8886,-1778,12439,-5332,12439,-1778,5332,-8886,5332,-12439,1778,-8886,1778,-12439,5332,-5332,5332,-1778,1778,-5332,1778,-1778,8886,-19547,8886,-15993,12439,-19547,12439,-15993,8886,-23101,8886,-26655,12439,-23101,12439,-26655,5332,-19547,5332,-15993,1778,-19547,1778,-15993,5332,-23101,5332,-26655,1778,-23101,1778,-26655,19547,-8886,19547,-12439,15993,-8886,15993,-12439,19547,-5332,19547,-1778,15993,-5332,15993,-1778,23101,-8886,23101,-12439,26655,-8886,26655,-12439,23101,-5332,23101,-1778,26655,-5332,26655,-1778,19547,-19547,19547,-15993,15993,-19547,15993,-15993,19547,-23101,19547,-26655,15993,-23101,15993,-26655,23101,-19547,23101,-15993,26655,-19547,26655,-15993,23101,-23101,23101,-26655,26655,-23101,26655,-26655,-8886,8886,-8886,12439,-12439,8886,-12439,12439,-8886,5332,-8886,1778,-12439,5332,-12439,1778,-5332,8886,-5332,12439,-1778,8886,-1778,12439,-5332,5332,-5332,1778,-1778,5332,-1778,1778,-8886,19547,-8886,15993,-12439,19547,-12439,15993,-8886,23101,-8886,26655,-12439,23101,-12439,26655,-5332,19547,-5332,15993,-1778,19547,-1778,15993,-5332,23101,-5332,26655,-1778,23101,-1778,26655,-19547,8886,-19547,12439,-15993,8886,-15993,12439,-19547,5332,-19547,1778,-15993,5332,-15993,1778,-23101,8886,-23101,12439,-26655,8886,-26655,12439,-23101,5332,-23101,1778,-26655,5332,-26655,1778,-19547,19547,-19547,15993,-15993,19547,-15993,15993,-19547,23101,-19547,26655,-15993,23101,-15993,26655,-23101,19547,-23101,15993,-26655,19547,-26655,15993,-23101,23101,-23101,26655,-26655,23101,-26655,26655,-8886,-8886,-8886,-12439,-12439,-8886,-12439,-12439,-8886,-5332,-8886,-1778,-12439,-5332,-12439,-1778,-5332,-8886,-5332,-12439,-1778,-8886,-1778,-12439,-5332,-5332,-5332,-1778,-1778,-5332,-1778,-1778,-8886,-19547,-8886,-15993,-12439,-19547,-12439,-15993,-8886,-23101,-8886,-26655,-12439,-23101,-12439,-26655,-5332,-19547,-5332,-15993,-1778,-19547,-1778,-15993,-5332,-23101,-5332,-26655,-1778,-23101,-1778,-26655,-19547,-8886,-19547,-12439,-15993,-8886,-15993,-12439,-19547,-5332,-19547,-1778,-15993,-5332,-15993,-1778,-23101,-8886,-23101,-12439,-26655,-8886,-26655,-12439,-23101,-5332,-23101,-1778,-26655,-5332,-26655,-1778,-19547,-19547,-19547,-15993,-15993,-19547,-15993,-15993,-19547,-23101,-19547,-26655,-15993,-23101,-15993,-26655,-23101,-19547,-23101,-15993,-26655,-19547,-26655,-15993,-23101,-23101,-23101,-26655,-26655,-23101,-26655,-26655};

static inline int ocp_rxtx(PHY_VARS_gNB *gNB, gNB_L1_rxtx_proc_t *proc) {
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  start_meas(&softmodem_stats_rxtx_sf);

  // *******************************************************************

  if (nfapi_mode == NFAPI_MODE_PNF) {
    // I am a PNF and I need to let nFAPI know that we have a (sub)frame tick
    //add_subframe(&frame, &subframe, 4);
    //oai_subframe_ind(proc->frame_tx, proc->subframe_tx);
    //LOG_D(PHY, "oai_subframe_ind(frame:%u, subframe:%d) - NOT CALLED ********\n", frame, subframe);
    start_meas(&nfapi_meas);
    oai_subframe_ind(proc->frame_rx, proc->slot_rx);
    stop_meas(&nfapi_meas);
    /*if (gNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus||
      gNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs ||
      gNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs ||
      gNB->UL_INFO.rach_ind.rach_indication_body.number_of_preambles ||
      gNB->UL_INFO.cqi_ind.number_of_cqis
      ) {
      LOG_D(PHY, "UL_info[rx_ind:%05d:%d harqs:%05d:%d crcs:%05d:%d preambles:%05d:%d cqis:%d] RX:%04d%d TX:%04d%d \n",
      NFAPI_SFNSF2DEC(gNB->UL_INFO.rx_ind.sfn_sf),   gNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus,
      NFAPI_SFNSF2DEC(gNB->UL_INFO.harq_ind.sfn_sf), gNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs,
      NFAPI_SFNSF2DEC(gNB->UL_INFO.crc_ind.sfn_sf),  gNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs,
      NFAPI_SFNSF2DEC(gNB->UL_INFO.rach_ind.sfn_sf), gNB->UL_INFO.rach_ind.rach_indication_body.number_of_preambles,
      gNB->UL_INFO.cqi_ind.number_of_cqis,
      frame_rx, slot_rx,
      frame_tx, slot_tx);
      }*/
  }

  /// NR disabling
  // ****************************************
  // Common RX procedures subframe n
  pthread_mutex_lock(&gNB->UL_INFO_mutex);
  gNB->UL_INFO.frame     = proc->frame_rx;
  gNB->UL_INFO.slot      = proc->slot_rx;
  gNB->UL_INFO.module_id = gNB->Mod_id;
  gNB->UL_INFO.CC_id     = gNB->CC_id;
  gNB->if_inst->NR_UL_indication(&gNB->UL_INFO);
  pthread_mutex_unlock(&gNB->UL_INFO_mutex);
  // RX processing
  int tx_slot_type         = nr_slot_select(cfg,proc->frame_tx,proc->slot_tx);
  int rx_slot_type         = nr_slot_select(cfg,proc->frame_rx,proc->slot_rx);

  if (rx_slot_type == NR_UPLINK_SLOT || rx_slot_type == NR_MIXED_SLOT) {
    // Do PRACH RU processing
    L1_nr_prach_procedures(gNB,proc->frame_rx,proc->slot_rx);
    phy_procedures_gNB_uespec_RX(gNB, proc->frame_rx, proc->slot_rx);
  }

  if (oai_exit) return(-1);

  // *****************************************
  // TX processing for subframe n+sf_ahead
  // run PHY TX procedures the one after the other for all CCs to avoid race conditions
  // (may be relaxed in the future for performance reasons)
  // *****************************************

  if (tx_slot_type == NR_DOWNLINK_SLOT || tx_slot_type == NR_MIXED_SLOT) {
    phy_procedures_gNB_TX(gNB, proc->frame_tx,proc->slot_tx, 1);
  }

  stop_meas( &softmodem_stats_rxtx_sf );
  LOG_D(PHY,"%s() Exit proc[rx:%d%d tx:%d%d]\n", __FUNCTION__, proc->frame_rx, proc->slot_rx, proc->frame_tx, proc->slot_tx);
  return(0);
}


static void *process_stats_thread(void *param) {
  PHY_VARS_gNB *gNB  = (PHY_VARS_gNB *)param;
  reset_meas(&gNB->dlsch_encoding_stats);
  reset_meas(&gNB->dlsch_scrambling_stats);
  reset_meas(&gNB->dlsch_modulation_stats);

  while(!oai_exit) {
    sleep(1);
    print_meas(&gNB->dlsch_encoding_stats, "pdsch_encoding", NULL, NULL);
    print_meas(&gNB->dlsch_scrambling_stats, "pdsch_scrambling", NULL, NULL);
    print_meas(&gNB->dlsch_modulation_stats, "pdsch_modulation", NULL, NULL);
  }

  return(NULL);
}

void init_gNB_proc(int inst) {
  PHY_VARS_gNB *gNB = RC.gNB[inst];
  gNB_L1_proc_t *proc = &gNB->proc;
  gNB_L1_rxtx_proc_t *L1_proc    = &proc->L1_proc;
  gNB_L1_rxtx_proc_t *L1_proc_tx = &proc->L1_proc_tx;
  L1_proc->instance_cnt          = -1;
  L1_proc_tx->instance_cnt       = -1;
  L1_proc->instance_cnt_RUs      = 0;
  L1_proc_tx->instance_cnt_RUs   = 0;
  proc->instance_cnt_prach       = -1;
  proc->instance_cnt_asynch_rxtx = -1;
  proc->CC_id                    = 0;
  proc->first_rx                 =1;
  proc->first_tx                 =1;
  proc->RU_mask                  =0;
  proc->RU_mask_tx               = (1<<gNB->num_RU)-1;
  proc->RU_mask_prach            =0;
  pthread_mutex_init( &gNB->UL_INFO_mutex, NULL);
  gNB->threadPool = (tpool_t *)malloc(sizeof(tpool_t));
  gNB->respDecode = (notifiedFIFO_t *) malloc(sizeof(notifiedFIFO_t));
  char ul_pool[] = "-1,-1";
  initTpool(ul_pool, gNB->threadPool, false);
  initNotifiedFIFO(gNB->respDecode);
}

/// eNB kept in function name for nffapi calls, TO FIX
void init_gNB_phase2(RU_t *ru) {
  int inst;
  LOG_I(PHY,"%s() RC.nb_nr_inst:%d\n", __FUNCTION__, RC.nb_nr_inst);

  for (inst=0; inst<RC.nb_nr_inst; inst++) {
    LOG_I(PHY,"RC.nb_nr_CC[inst:%d]:%p\n", inst, RC.gNB[inst]);
    PHY_VARS_gNB *gNB =  RC.gNB[inst];
    LOG_E(PHY,"hard coded gNB->num_RU:%d\n", gNB->num_RU);
    phy_init_nr_gNB(gNB,0,0);
    RC.gNB[inst]->num_RU=1;
    RC.gNB[inst]->RU_list[0]=ru;
    //init_precoding_weights(RC.gNB[inst][CC_id]);
  }
}

void init_gNB(int single_thread_flag,int wait_for_sync) {
  for (int inst=0; inst<RC.nb_nr_L1_inst; inst++) {
    AssertFatal( RC.gNB[inst] != NULL, "Must be allocated in init_main_gNB->RCconfig_NR_L1\n");
    PHY_VARS_gNB *gNB= RC.gNB[inst];
    gNB->abstraction_flag   = false;
    gNB->single_thread_flag = true;
    /*nr_polar_init(&gNB->nrPolar_params,
      NR_POLAR_PBCH_MESSAGE_TYPE,
      NR_POLAR_PBCH_PAYLOAD_BITS,
      NR_POLAR_PBCH_AGGREGATION_LEVEL);*/
    LOG_I(PHY,"Registering with MAC interface module\n");
    AssertFatal((gNB->if_inst = NR_IF_Module_init(inst))!=NULL,"Cannot register interface");
    gNB->if_inst->NR_Schedule_response   = nr_schedule_response;
    gNB->if_inst->NR_PHY_config_req      = nr_phy_config_request;
    memset((void *)&gNB->UL_INFO,0,sizeof(gNB->UL_INFO));
    memset((void *)&gNB->UL_tti_req,0,sizeof(nfapi_nr_ul_tti_request_t));
    //memset((void *)&gNB->Sched_INFO,0,sizeof(gNB->Sched_INFO));
    LOG_I(PHY,"Setting indication lists\n");
    gNB->UL_INFO.rx_ind.pdu_list = gNB->rx_pdu_list;
    gNB->UL_INFO.crc_ind.crc_list = gNB->crc_pdu_list;
    /*gNB->UL_INFO.sr_ind.sr_indication_body.sr_pdu_list = gNB->sr_pdu_list;
      gNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list = gNB->harq_pdu_list;
      gNB->UL_INFO.cqi_ind.cqi_pdu_list = gNB->cqi_pdu_list;
      gNB->UL_INFO.cqi_ind.cqi_raw_pdu_list = gNB->cqi_raw_pdu_list;*/
    gNB->prach_energy_counter = 0;
  }

  LOG_I(PHY,"[nr-softmodem.c] gNB structure allocated\n");
}


void stop_gNB(int nb_inst) {
}

static void get_options(void) {
  paramdef_t cmdline_params[] = CMDLINE_PARAMS_DESC_GNB ;
  config_process_cmdline( cmdline_params,sizeof(cmdline_params)/sizeof(paramdef_t),NULL);

  if ( !(CONFIG_ISFLAGSET(CONFIG_ABORT)) ) {
    memset((void *)&RC,0,sizeof(RC));
    /* Read RC configuration file */
    NRRCConfig();
    printf("Configuration: nb_rrc_inst %d, nb_nr_L1_inst %d, nb_ru %hhd\n", RC.nb_nr_inst,RC.nb_nr_L1_inst,RC.nb_RU);
  }

  AssertFatal(RC.nb_nr_L1_inst == 1 && RC.nb_RU == 1, "Only one gNB, one RU and one carrier is supported\n");
}

void set_default_frame_parms(nfapi_nr_config_request_t *config[MAX_NUM_CCs],
                             NR_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs]) {
  for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    frame_parms[CC_id] = (NR_DL_FRAME_PARMS *) malloc(sizeof(NR_DL_FRAME_PARMS));
    config[CC_id] = (nfapi_nr_config_request_t *) malloc(sizeof(nfapi_nr_config_request_t));
    config[CC_id]->subframe_config.numerology_index_mu.value =1;
    config[CC_id]->subframe_config.duplex_mode.value = 1; //FDD
    config[CC_id]->subframe_config.dl_cyclic_prefix_type.value = 0; //NORMAL
    config[CC_id]->rf_config.dl_carrier_bandwidth.value = 106;
    config[CC_id]->rf_config.ul_carrier_bandwidth.value = 106;
    config[CC_id]->sch_config.physical_cell_id.value = 0;
  }
}

static void init_pdcp(void) {
  //if (!NODE_IS_DU(RC.rrc[0]->node_type)) {
  pdcp_layer_init();
  uint32_t pdcp_initmask = (IS_SOFTMODEM_NOS1) ?
                           (PDCP_USE_NETLINK_BIT | LINK_ENB_PDCP_TO_IP_DRIVER_BIT) : LINK_ENB_PDCP_TO_GTPV1U_BIT;

  if (IS_SOFTMODEM_NOS1) {
    printf("IS_SOFTMODEM_NOS1 option enabled \n");
    pdcp_initmask = pdcp_initmask | ENB_NAS_USE_TUN_BIT | SOFTMODEM_NOKRNMOD_BIT  ;
  }

  nr_pdcp_module_init(pdcp_initmask, 0);
  pdcp_set_rlc_data_req_func((send_rlc_data_req_func_t) rlc_data_req);
  pdcp_set_pdcp_data_ind_func((pdcp_data_ind_func_t) pdcp_data_ind);
}

void init_main_gNB(void) {
  RCconfig_NR_L1();
  RCconfig_nr_macrlc();

  if (RC.nb_nr_L1_inst>0)
    AssertFatal(l1_north_init_gNB()==0,"could not initialize L1 north interface\n");

  LOG_I(GNB_APP,"Allocating gNB_RRC_INST for %d instances\n",RC.nb_nr_inst);
  RC.nrrrc = (gNB_RRC_INST **)calloc(RC.nb_nr_inst*sizeof(gNB_RRC_INST *),1);
  LOG_I(PHY, "%s() RC.nb_nr_inst:%d RC.nrrrc:%p\n", __FUNCTION__, RC.nb_nr_inst, RC.nrrrc);
  int gnb_id=0; // only 1 gnb per process, index 0 for now
  RC.nrrrc[gnb_id] = (gNB_RRC_INST *)calloc(sizeof(gNB_RRC_INST),1);
  MessageDef *msg_p = itti_alloc_new_message (TASK_GNB_APP, 0, NRRRC_CONFIGURATION_REQ);
  RCconfig_NRRRC(msg_p,gnb_id, RC.nrrrc[gnb_id]);
  openair_rrc_gNB_configuration(GNB_INSTANCE_TO_MODULE_ID(ITTI_MSG_DESTINATION_INSTANCE(msg_p)), &NRRRC_CONFIGURATION_REQ(msg_p));
  //AssertFatal(itti_create_task(TASK_GNB_APP, gNB_app_task, NULL) >= 0, "");
  AssertFatal(itti_create_task(TASK_RRC_GNB, rrc_gnb_task, NULL) >= 0, "");
  //AssertFatal(itti_create_task(TASK_X2AP, x2ap_task, NULL) >= 0, "");
}

static  void wait_nfapi_init(char *thread_name) {
  printf( "waiting for NFAPI PNF connection and population of global structure (%s)\n",thread_name);
  pthread_mutex_lock( &nfapi_sync_mutex );

  while (nfapi_sync_var<0)
    pthread_cond_wait( &nfapi_sync_cond, &nfapi_sync_mutex );

  pthread_mutex_unlock(&nfapi_sync_mutex);
  printf( "NFAPI: got sync (%s)\n", thread_name);
}

void exit_function(const char *file, const char *function, const int line, const char *s) {
  if (s != NULL) {
    printf("%s:%d %s() Exiting OAI softmodem: %s\n",file,line, function, s);
  }

  oai_exit = 1;
  sleep(1); //allow lte-softmodem threads to exit first
  exit(1);
}

void stop_RU(int nb_ru) {
  return;
}

void OCPconfig_RU(RU_t *ru) {
  int i = 0, j = 0; // Ru and gNB cardinality
  paramdef_t RUParams[] = RUPARAMS_DESC;
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};
  config_getlist( &RUParamList, RUParams, sizeof(RUParams)/sizeof(paramdef_t), NULL);
  AssertFatal( RUParamList.numelt == 1 && RC.nb_nr_L1_inst ==1,""  );
  ru->idx=0;
  ru->nr_frame_parms                      = (NR_DL_FRAME_PARMS *)calloc(sizeof(NR_DL_FRAME_PARMS),1);
  ru->frame_parms                         = (LTE_DL_FRAME_PARMS *)calloc(sizeof(LTE_DL_FRAME_PARMS),1);
  ru->if_timing                           = synch_to_ext_device;
  ru->num_gNB                           = RUParamList.paramarray[j][RU_ENB_LIST_IDX].numelt;
  ru->gNB_list[i] = &RC.gNB[RUParamList.paramarray[j][RU_ENB_LIST_IDX].iptr[i]][0];

  if (config_isparamset(RUParamList.paramarray[j], RU_SDR_ADDRS)) {
    ru->openair0_cfg.sdr_addrs = strdup(*(RUParamList.paramarray[j][RU_SDR_ADDRS].strptr));
  }

  if (config_isparamset(RUParamList.paramarray[j], RU_SDR_CLK_SRC)) {
    if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "internal") == 0) {
      ru->openair0_cfg.clock_source = internal;
      LOG_D(PHY, "RU clock source set as internal\n");
    } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "external") == 0) {
      ru->openair0_cfg.clock_source = external;
      LOG_D(PHY, "RU clock source set as external\n");
    } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "gpsdo") == 0) {
      ru->openair0_cfg.clock_source = gpsdo;
      LOG_D(PHY, "RU clock source set as gpsdo\n");
    } else {
      LOG_E(PHY, "Erroneous RU clock source in the provided configuration file: '%s'\n", *(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr));
    }
  } else {
    ru->openair0_cfg.clock_source = unset;
  }

  if (strcmp(*(RUParamList.paramarray[j][RU_LOCAL_RF_IDX].strptr), "yes") == 0) {
    if ( !(config_isparamset(RUParamList.paramarray[j],RU_LOCAL_IF_NAME_IDX)) ) {
      ru->if_south                        = REMOTE_IF5; //TBD: max value to avoid to call "ru" functions
      ru->function                        = gNodeB_3GPP;
      printf("Setting function for RU %d to gNodeB_3GPP\n",j);
    } else {
    }

    ru->max_pdschReferenceSignalPower     = *(RUParamList.paramarray[j][RU_MAX_RS_EPRE_IDX].uptr);;
    ru->max_rxgain                        = *(RUParamList.paramarray[j][RU_MAX_RXGAIN_IDX].uptr);
    ru->num_bands                         = RUParamList.paramarray[j][RU_BAND_LIST_IDX].numelt;

    for (i=0; i<ru->num_bands; i++) ru->band[i] = RUParamList.paramarray[j][RU_BAND_LIST_IDX].iptr[i];
  } //strcmp(local_rf, "yes") == 0
  else {
  }

  ru->nb_tx                             = *(RUParamList.paramarray[j][RU_NB_TX_IDX].uptr);
  ru->nb_rx                             = *(RUParamList.paramarray[j][RU_NB_RX_IDX].uptr);
  ru->att_tx                            = *(RUParamList.paramarray[j][RU_ATT_TX_IDX].uptr);
  ru->att_rx                            = *(RUParamList.paramarray[j][RU_ATT_RX_IDX].uptr);

  if (config_isparamset(RUParamList.paramarray[j], RU_BF_WEIGHTS_LIST_IDX)) {
    ru->nb_bfw = RUParamList.paramarray[j][RU_BF_WEIGHTS_LIST_IDX].numelt;

    for (i=0; i<ru->num_gNB; i++)  {
      ru->bw_list[i] = (int32_t *)malloc16_clear((ru->nb_bfw)*sizeof(int32_t));

      for (int b=0; b<ru->nb_bfw; b++) ru->bw_list[i][b] = RUParamList.paramarray[j][RU_BF_WEIGHTS_LIST_IDX].iptr[b];
    }
  }

  return;
}

// this is for RU with local RF unit
void fill_rf_config(RU_t *ru, char *rf_config_file) {
  int i;
  NR_DL_FRAME_PARMS *fp   = ru->nr_frame_parms;
  nfapi_nr_config_request_scf_t *gNB_config = &ru->gNB_list[0]->gNB_config; //tmp index
  openair0_config_t *cfg   = &ru->openair0_cfg;
  int mu = gNB_config->ssb_config.scs_common.value;
  int N_RB = gNB_config->carrier_config.dl_grid_size[gNB_config->ssb_config.scs_common.value].value;
  fp->threequarter_fs=threequarter_fs;

  if (mu == NR_MU_0) { //or if LTE
    if(N_RB == 100) {
      if (fp->threequarter_fs) {
        cfg->sample_rate=23.04e6;
        cfg->samples_per_frame = 230400;
        cfg->tx_bw = 10e6;
        cfg->rx_bw = 10e6;
      } else {
        cfg->sample_rate=30.72e6;
        cfg->samples_per_frame = 307200;
        cfg->tx_bw = 10e6;
        cfg->rx_bw = 10e6;
      }
    } else if(N_RB == 50) {
      cfg->sample_rate=15.36e6;
      cfg->samples_per_frame = 153600;
      cfg->tx_bw = 5e6;
      cfg->rx_bw = 5e6;
    } else if (N_RB == 25) {
      cfg->sample_rate=7.68e6;
      cfg->samples_per_frame = 76800;
      cfg->tx_bw = 2.5e6;
      cfg->rx_bw = 2.5e6;
    } else if (N_RB == 6) {
      cfg->sample_rate=1.92e6;
      cfg->samples_per_frame = 19200;
      cfg->tx_bw = 1.5e6;
      cfg->rx_bw = 1.5e6;
    } else AssertFatal(1==0,"Unknown N_RB %d\n",N_RB);
  } else if (mu == NR_MU_1) {
    if(N_RB == 273) {
      if (fp->threequarter_fs) {
        AssertFatal(0 == 1,"three quarter sampling not supported for N_RB 273\n");
      } else {
        cfg->sample_rate=122.88e6;
        cfg->samples_per_frame = 1228800;
        cfg->tx_bw = 100e6;
        cfg->rx_bw = 100e6;
      }
    } else if(N_RB == 217) {
      if (fp->threequarter_fs) {
        cfg->sample_rate=92.16e6;
        cfg->samples_per_frame = 921600;
        cfg->tx_bw = 80e6;
        cfg->rx_bw = 80e6;
      } else {
        cfg->sample_rate=122.88e6;
        cfg->samples_per_frame = 1228800;
        cfg->tx_bw = 80e6;
        cfg->rx_bw = 80e6;
      }
    } else if(N_RB == 106) {
      if (fp->threequarter_fs) {
        cfg->sample_rate=46.08e6;
        cfg->samples_per_frame = 460800;
        cfg->tx_bw = 40e6;
        cfg->rx_bw = 40e6;
      } else {
        cfg->sample_rate=61.44e6;
        cfg->samples_per_frame = 614400;
        cfg->tx_bw = 40e6;
        cfg->rx_bw = 40e6;
      }
    } else {
      AssertFatal(0==1,"N_RB %d not yet supported for numerology %d\n",N_RB,mu);
    }
  } else if (mu == NR_MU_3) {
    if (N_RB == 66) {
      cfg->sample_rate = 122.88e6;
      cfg->samples_per_frame = 1228800;
      cfg->tx_bw = 100e6;
      cfg->rx_bw = 100e6;
    } else if(N_RB == 32) {
      cfg->sample_rate=61.44e6;
      cfg->samples_per_frame = 614400;
      cfg->tx_bw = 50e6;
      cfg->rx_bw = 50e6;
    }
  } else {
    AssertFatal(0 == 1,"Numerology %d not supported for the moment\n",mu);
  }

  if (gNB_config->cell_config.frame_duplex_type.value==TDD)
    cfg->duplex_mode = duplex_mode_TDD;
  else //FDD
    cfg->duplex_mode = duplex_mode_FDD;

  cfg->Mod_id = 0;
  cfg->num_rb_dl=N_RB;
  cfg->tx_num_channels=ru->nb_tx;
  cfg->rx_num_channels=ru->nb_rx;

  for (i=0; i<ru->nb_tx; i++) {
    if (ru->if_frequency == 0) {
      cfg->tx_freq[i] = (double)fp->dl_CarrierFreq;
      cfg->rx_freq[i] = (double)fp->ul_CarrierFreq;
    } else {
      cfg->tx_freq[i] = (double)ru->if_frequency;
      cfg->rx_freq[i] = (double)(ru->if_frequency+fp->ul_CarrierFreq-fp->dl_CarrierFreq);
    }

    cfg->tx_gain[i] = ru->att_tx;
    cfg->rx_gain[i] = ru->max_rxgain-ru->att_rx;
    cfg->configFilename = rf_config_file;
    printf("channel %d, Setting tx_gain offset %f, rx_gain offset %f, tx_freq %f, rx_freq %f\n",
           i, cfg->tx_gain[i],
           cfg->rx_gain[i],
           cfg->tx_freq[i],
           cfg->rx_freq[i]);
  }
}

/* this function maps the RU tx and rx buffers to the available rf chains.
   Each rf chain is is addressed by the card number and the chain on the card. The
   rf_map specifies for each antenna port, on which rf chain the mapping should start. Multiple
   antennas are mapped to successive RF chains on the same card. */
int setup_RU_buffers(RU_t *ru) {
  int i,j;
  int card,ant;
  //uint16_t N_TA_offset = 0;
  NR_DL_FRAME_PARMS *frame_parms;
  nfapi_nr_config_request_scf_t *config = &ru->config;

  if (ru) {
    frame_parms = ru->nr_frame_parms;
    printf("setup_RU_buffers: frame_parms = %p\n",frame_parms);
  } else {
    printf("ru pointer is NULL\n");
    return(-1);
  }

  int mu = config->ssb_config.scs_common.value;
  int N_RB = config->carrier_config.dl_grid_size[config->ssb_config.scs_common.value].value;

  if (config->cell_config.frame_duplex_type.value == TDD) {
    int N_TA_offset =  config->carrier_config.uplink_frequency.value < 6000000 ? 400 : 431; // reference samples  for 25600Tc @ 30.72 Ms/s for FR1, same @ 61.44 Ms/s for FR2
    double factor=1;

    switch (mu) {
      case 0: //15 kHz scs
        AssertFatal(N_TA_offset == 400,"scs_common 15kHz only for FR1\n");

        if (N_RB <= 25) factor = .25;      // 7.68 Ms/s
        else if (N_RB <=50) factor = .5;   // 15.36 Ms/s
        else if (N_RB <=75) factor = 1.0;  // 30.72 Ms/s
        else if (N_RB <=100) factor = 1.0; // 30.72 Ms/s
        else AssertFatal(1==0,"Too many PRBS for mu=0\n");

        break;

      case 1: //30 kHz sc
        AssertFatal(N_TA_offset == 400,"scs_common 30kHz only for FR1\n");

        if (N_RB <= 106) factor = 2.0; // 61.44 Ms/s
        else if (N_RB <= 275) factor = 4.0; // 122.88 Ms/s

        break;

      case 2: //60 kHz scs
        AssertFatal(1==0,"scs_common should not be 60 kHz\n");
        break;

      case 3: //120 kHz scs
        AssertFatal(N_TA_offset == 431,"scs_common 120kHz only for FR2\n");
        break;

      case 4: //240 kHz scs
        AssertFatal(1==0,"scs_common should not be 60 kHz\n");

        if (N_RB <= 32) factor = 1.0; // 61.44 Ms/s
        else if (N_RB <= 66) factor = 2.0; // 122.88 Ms/s
        else AssertFatal(1==0,"N_RB %d is too big for curretn FR2 implementation\n",N_RB);

        break;

        if      (N_RB == 100) ru->N_TA_offset = 624;
        else if (N_RB == 50)  ru->N_TA_offset = 624/2;
        else if (N_RB == 25)  ru->N_TA_offset = 624/4;
    }

    if (frame_parms->threequarter_fs == 1) factor = factor*.75;

    ru->N_TA_offset = (int)(N_TA_offset * factor);
    LOG_I(PHY,"RU %d Setting N_TA_offset to %d samples (factor %f, UL Freq %d, N_RB %d)\n",ru->idx,ru->N_TA_offset,factor,
          config->carrier_config.uplink_frequency.value, N_RB);
  } else ru->N_TA_offset = 0;

  if (ru->openair0_cfg.mmapped_dma == 1) {
    // replace RX signal buffers with mmaped HW versions
    for (i=0; i<ru->nb_rx; i++) {
      card = i/4;
      ant = i%4;
      printf("Mapping RU id %u, rx_ant %d, on card %d, chain %d\n",ru->idx,i,ru->rf_map.card+card, ru->rf_map.chain+ant);
      free(ru->common.rxdata[i]);
      ru->common.rxdata[i] = ru->openair0_cfg.rxbase[ru->rf_map.chain+ant];
      printf("rxdata[%d] @ %p\n",i,ru->common.rxdata[i]);

      for (j=0; j<16; j++) {
        printf("rxbuffer %d: %x\n",j,ru->common.rxdata[i][j]);
        ru->common.rxdata[i][j] = 16-j;
      }
    }

    for (i=0; i<ru->nb_tx; i++) {
      card = i/4;
      ant = i%4;
      printf("Mapping RU id %u, tx_ant %d, on card %d, chain %d\n",ru->idx,i,ru->rf_map.card+card, ru->rf_map.chain+ant);
      free(ru->common.txdata[i]);
      ru->common.txdata[i] = ru->openair0_cfg.txbase[ru->rf_map.chain+ant];
      printf("txdata[%d] @ %p\n",i,ru->common.txdata[i]);

      for (j=0; j<16; j++) {
        printf("txbuffer %d: %x\n",j,ru->common.txdata[i][j]);
        ru->common.txdata[i][j] = 16-j;
      }
    }
  } else { // not memory-mapped DMA
    //nothing to do, everything already allocated in lte_init
  }

  return(0);
}
int rx_rf(int rxBufOffet, int nbSamples, int nb_rx, int32_t **rxdata, openair0_device *rfdevice,  openair0_timestamp *HWtimeStamp) {
  void *rxp[nb_rx];

  for (int i=0; i<nb_rx; i++)
    rxp[i] = (void *)&rxdata[i][rxBufOffet];

  unsigned int rxs = rfdevice->trx_read_func(rfdevice,
                     HWtimeStamp,
                     rxp,
                     nbSamples,
                     nb_rx);

  if (rxs != nbSamples )
    LOG_E(PHY, "rx_rf: Asked for %d samples, got %d from USRP\n",nbSamples,rxs);

  return rxs;
}

void tx_rf(RU_t *ru,int frame,int slot, uint64_t timestamp) {
  RU_proc_t *proc = &ru->proc;
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &ru->gNB_list[0]->gNB_config;
  int i,txsymb;
  int slot_type         = nr_slot_select(cfg,frame,slot%fp->slots_per_frame);
  int prevslot_type     = nr_slot_select(cfg,frame,(slot+(fp->slots_per_frame-1))%fp->slots_per_frame);
  int nextslot_type     = nr_slot_select(cfg,frame,(slot+1)%fp->slots_per_frame);
  int sf_extension  = 0;                 //sf_extension = ru->sf_extension;
  int siglen=fp->get_samples_per_slot(slot,fp);
  int flags=1;

  //nr_subframe_t SF_type     = nr_slot_select(cfg,slot%fp->slots_per_frame);
  if (slot_type == NR_DOWNLINK_SLOT || slot_type == NR_MIXED_SLOT || IS_SOFTMODEM_RFSIM) {
    if(slot_type == NR_MIXED_SLOT) {
      txsymb = 0;

      for(int symbol_count =0; symbol_count<NR_NUMBER_OF_SYMBOLS_PER_SLOT; symbol_count++) {
        if (cfg->tdd_table.max_tdd_periodicity_list[slot].max_num_of_symbol_per_slot_list[symbol_count].slot_config.value==0)
          txsymb++;
      }

      AssertFatal(txsymb>0,"illegal txsymb %d\n",txsymb);

      if(slot%(fp->slots_per_subframe/2))
        siglen = txsymb * (fp->ofdm_symbol_size + fp->nb_prefix_samples);
      else
        siglen = (fp->ofdm_symbol_size + fp->nb_prefix_samples0) + (txsymb - 1) * (fp->ofdm_symbol_size + fp->nb_prefix_samples);

      //+ ru->end_of_burst_delay;
      flags=3; // end of burst
    }

    if (cfg->cell_config.frame_duplex_type.value == TDD &&
        slot_type == NR_DOWNLINK_SLOT &&
        prevslot_type == NR_UPLINK_SLOT) {
      flags = 2; // start of burst
    }

    if (cfg->cell_config.frame_duplex_type.value == TDD &&
        slot_type == NR_DOWNLINK_SLOT &&
        nextslot_type == NR_UPLINK_SLOT) {
      flags = 3; // end of burst
    }

    if (fp->freq_range==nr_FR2) {
      // the beam index is written in bits 8-10 of the flags
      // bit 11 enables the gpio programming
      int beam=0;

      if (slot==0) beam = 11; //3 for boresight & 8 to enable

      /*
        if (slot==0 || slot==40) beam=0&8;
        if (slot==10 || slot==50) beam=1&8;
        if (slot==20 || slot==60) beam=2&8;
        if (slot==30 || slot==70) beam=3&8;
      */
      flags |= beam<<8;
    }

    void *txp[ru->nb_tx];

    for (i=0; i<ru->nb_tx; i++)
      txp[i] = (void *)&ru->common.txdata[i][fp->get_samples_slot_timestamp(slot,fp,0)-sf_extension];

    // prepare tx buffer pointers
    unsigned int txs = ru->rfdevice.trx_write_func(&ru->rfdevice,
                       timestamp+ru->ts_offset-ru->openair0_cfg.tx_sample_advance-sf_extension,
                       txp,
                       siglen+sf_extension,
                       ru->nb_tx,
                       flags);
    LOG_D(PHY,"[TXPATH] RU %d tx_rf, writing to TS %llu, frame %d, unwrapped_frame %d, slot %d\n",ru->idx,
          (long long unsigned int)timestamp,frame,proc->frame_tx_unwrap,slot);
    AssertFatal(txs ==  siglen+sf_extension,"TX : Timeout (sent %u/%d)\n", txs, siglen);
  }
}

static void *ru_thread( void *param ) {
  RU_t               *ru      = (RU_t *)param;
  NR_DL_FRAME_PARMS  *fp      = ru->nr_frame_parms;
  LOG_I(PHY,"Starting RU %d (%s,%s),\n",ru->idx,NB_functions[ru->function],NB_timing[ru->if_timing]);
  nr_dump_frame_parms(fp);
  AssertFatal(openair0_device_load(&ru->rfdevice,&ru->openair0_cfg)==0,"Cannot connect to local radio\n");
  AssertFatal(ru->rfdevice.trx_start_func(&ru->rfdevice) == 0,"Could not start the RF device\n");
  int64_t slot=-1;
  int64_t nextHWTSshouldBe=0, nextRxTSlogical=0;
  // weird globals, used in NR_IF_Module.c
  sf_ahead = (uint16_t) ceil((float)6/(0x01<<fp->numerology_index));
  sl_ahead = sf_ahead*fp->slots_per_subframe;

  // This is a forever while loop, it loops over subframes which are scheduled by incoming samples from HW devices

  while (!oai_exit) {
    int nextSlot=(slot+1)%fp->slots_per_frame;
    uint32_t samples_per_slot = fp->get_samples_per_slot(nextSlot,fp);
    int rxBuffOffset=fp->get_samples_slot_timestamp(nextSlot,fp,0);
    AssertFatal(rxBuffOffset + samples_per_slot <= fp->samples_per_frame, "Will read outside allocated buffer\n");
    int samples=fp->get_samples_per_slot(nextSlot,fp);
    openair0_timestamp HWtimeStamp=0; //for multi RU
    int rxs=rx_rf(rxBuffOffset,
                  samples,
                  ru->nb_rx,
                  ru->common.rxdata,
                  &ru->rfdevice,
                  &HWtimeStamp);
    LOG_D(PHY,"Reading %d samples for slot %d\n",samples_per_slot,nextSlot);

    if ( HWtimeStamp !=  nextHWTSshouldBe)
      LOG_E(HW,"reading a stream must be continuous, %ld, %ld\n", HWtimeStamp, nextHWTSshouldBe);

    nextHWTSshouldBe=HWtimeStamp+rxs;
    ru->proc.timestamp_rx=nextRxTSlogical;
    nextRxTSlogical+=samples_per_slot;
    int64_t HW_to_logical_RxTSoffset=(int64_t)HWtimeStamp-(int64_t)ru->proc.timestamp_rx;
    ru->proc.frame_rx    = (ru->proc.timestamp_rx / (fp->samples_per_subframe*10))&1023;
    uint32_t idx_sf = ru->proc.timestamp_rx / fp->samples_per_subframe;
    float offsetInSubframe=ru->proc.timestamp_rx % fp->samples_per_subframe;
    ru->proc.tti_rx = (idx_sf * fp->slots_per_subframe +
                       lroundf(offsetInSubframe / fp->samples_per_slot0))%
                      fp->slots_per_frame;
    LOG_D(PHY,"RU %d/%d TS %llu (off %d), frame %d, slot %d.%d / %d\n",
          ru->idx, 0,
          (unsigned long long int)ru->proc.timestamp_rx,
          (int)ru->ts_offset,ru->proc.frame_rx,ru->proc.tti_rx,ru->proc.tti_tx,fp->slots_per_frame);
    int slot_type = nr_slot_select(&ru->gNB_list[0]->gNB_config,ru->proc.frame_rx,ru->proc.tti_rx);

    if (slot_type == NR_UPLINK_SLOT || slot_type == NR_MIXED_SLOT) {
      nr_fep_full(ru,ru->proc.tti_rx);

      for (int aa=0; aa<ru->nb_rx; aa++)
        memcpy((void *)RC.gNB[0]->common_vars.rxdataF[aa],
               (void *)ru->common.rxdataF[aa], fp->symbols_per_slot*fp->ofdm_symbol_size*sizeof(int32_t));

      LOG_D(PHY, "rxdataF energy: %d\n", signal_energy(ru->common.rxdataF[0], fp->symbols_per_slot*fp->ofdm_symbol_size));
    }

    gNB_L1_proc_t *gNBproc=&RC.gNB[0][0].proc;
    gNB_L1_rxtx_proc_t *L1_proc = &gNBproc->L1_proc;
    NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
    gNBproc->timestamp_rx  = ru->proc.timestamp_rx;
    L1_proc->frame_rx = gNBproc->frame_rx = gNBproc->frame_prach = ru->proc.frame_rx =
        (ru->proc.timestamp_rx / (fp->samples_per_subframe*10))&1023;
    L1_proc->slot_rx = gNBproc->slot_rx = gNBproc->slot_prach =
                                            ru->proc.tti_rx; // computed before in caller function
    L1_proc->timestamp_tx = gNBproc->timestamp_tx = ru->proc.timestamp_tx =
                              ru->proc.timestamp_rx +
                              sf_ahead*fp->samples_per_subframe;
    L1_proc->frame_tx  = gNBproc->frame_tx = ru->proc.frame_tx =
                           (gNBproc->timestamp_tx / (fp->samples_per_subframe*10))&1023;
    L1_proc->slot_tx   =  ru->proc.tti_tx =
                            (L1_proc->slot_rx + sl_ahead)%fp->slots_per_frame;

    if (ocp_rxtx(&RC.gNB[0][0],L1_proc) < 0)
      LOG_E(PHY,"gNB %d CC_id %d failed during execution\n",RC.gNB[0][0].Mod_id,RC.gNB[0][0].CC_id);

    // do TX front-end processing if needed (precoding and/or IDFTs)
    //ru->feptx_prec(ru,proc->frame_tx,proc->tti_tx);
    nr_feptx_prec(ru,ru->proc.frame_tx,ru->proc.tti_tx);
    // do OFDM with/without TX front-end processing  if needed
    //ru->feptx_ofdm
    nfapi_nr_config_request_scf_t *cfg = &ru->gNB_list[0]->gNB_config;

    if (nr_slot_select(cfg,ru->proc.frame_tx, ru->proc.tti_tx ) != NR_UPLINK_SLOT) {
      int aa=0; // antenna 0 hardcoded
      NR_DL_FRAME_PARMS *fp=ru->nr_frame_parms;
      nr_feptx0(ru,ru->proc.tti_tx,0,fp->symbols_per_slot,aa);
      int *txdata = &ru->common.txdata[aa][fp->get_samples_slot_timestamp(ru->proc.tti_tx,fp,0)];
      int slot_sizeF = (fp->ofdm_symbol_size)*
                       ((NFAPI_CP_NORMAL == 1) ? 12 : 14);
      LOG_D(PHY,"feptx_ofdm (TXPATH): frame %d, slot %d: txp (time %ld) %d dB, txp (freq) %d dB\n",
            ru->proc.frame_tx,ru->proc.tti_tx,ru->proc.timestamp_tx,
            dB_fixed(signal_energy((int32_t *)txdata,fp->get_samples_per_slot(ru->proc.tti_tx,fp))),
            dB_fixed(signal_energy_nodc(ru->common.txdataF_BF[aa],2*slot_sizeF)));
    }

    // do outgoing fronthaul (south) if needed
    tx_rf(ru,ru->proc.frame_tx,ru->proc.tti_tx,ru->proc.timestamp_tx+HW_to_logical_RxTSoffset);
    slot++;
  }

  printf( "Exiting ru_thread \n");
  ru->rfdevice.trx_end_func(&ru->rfdevice);
  static int ru_thread_status = 0;
  return &ru_thread_status;
}

void launch_NR_RU(RU_t *ru, char *rf_config_file) {
  LOG_I(PHY,"number of L1 instances %d, number of RU %d, number of CPU cores %d\n",
        RC.nb_nr_L1_inst,RC.nb_RU,get_nprocs());
  LOG_D(PHY,"Process RUs RC.nb_RU:%d\n",RC.nb_RU);

  for (int ru_id=0; ru_id<RC.nb_RU; ru_id++) {
    LOG_D(PHY,"Process RC.ru[%d]\n",ru_id);
    ru->rf_config_file = rf_config_file;
    ru->idx            = ru_id;
    ru->ts_offset      = 0;
    // use gNB_list[0] as a reference for RU frame parameters
    // NOTE: multiple CC_id are not handled here yet!
    LOG_D(PHY, "%s() RC.ru[%d].num_gNB:%d ru->gNB_list[0]:%p rf_config_file:%s\n",
          __FUNCTION__, ru_id, ru->num_gNB, ru->gNB_list[0],  ru->rf_config_file);
    LOG_E(PHY,"ru->gNB_list ru->num_gNB hardcoded: one RU connected to carrier 0 of gNB 0\n");
    ru->gNB_list[0] = &RC.gNB[0][0];
    ru->num_gNB=1;
    LOG_I(PHY,"Copying frame parms from gNB in RC to ru %d and frame_parms in ru\n",ru->idx);
    RU_proc_t *proc = &ru->proc;
    threadCreate( &proc->pthread_FH, ru_thread, (void *)ru, "MainLoop", -1, OAI_PRIORITY_RT_MAX );
  }
}

void init_eNB_afterRU(void) {
  AssertFatal(false,"");
}

int main( int argc, char **argv ) {
  AssertFatal(load_configmodule(argc,argv,CONFIG_ENABLECMDLINEONLY),
              "[SOFTMODEM] Error, configuration module init failed\n");
  logInit();
#ifndef PACKAGE_VERSION
#  define PACKAGE_VERSION "UNKNOWN"
#endif
  LOG_I(HW, "Version: %s\n", PACKAGE_VERSION);
  configure_linux();
  get_options ();
  get_common_options(SOFTMODEM_GNB_BIT );
  AssertFatal(!CONFIG_ISFLAGSET(CONFIG_ABORT),"Getting configuration failed\n");
  cpuf=get_cpu_freq_GHz();
  itti_init(TASK_MAX, tasks_info);
  set_taus_seed (0);
  init_opt();
  init_pdcp();
  init_main_gNB();
  init_gNB(true, wait_for_sync);
  /* Start the agent. If it is turned off in the configuration, it won't start */
  RCconfig_nr_flexran();

  for (int i = 0; i < RC.nb_nr_L1_inst; i++) {
    flexran_agent_start(i);
  }

  /*
   *nfapi stuff very buggy in 4G, not yet implemented in 5G
   */
  // init UE_PF_PO and mutex lock (paging from S1AP)
  pthread_mutex_init(&ue_pf_po_mutex, NULL);
  printf("NFAPI*** - mutex and cond created - will block shortly for completion of PNF connection\n");
  pthread_cond_init(&nfapi_sync_cond,NULL);
  pthread_mutex_init(&nfapi_sync_mutex, NULL);
  const char *nfapi_mode_str[] = {
    "MONOLITHIC", "PNF", "VNF",
  };
  AssertFatal(nfapi_mode < 3,"");
  printf("NFAPI MODE:%s\n", nfapi_mode_str[nfapi_mode]);

  if (nfapi_mode==NFAPI_MODE_VNF) // VNF
    wait_nfapi_init("main?");

  for (int i=0; i<RC.nb_nr_L1_inst; i++)
    AssertFatal(RC.gNB[i]->configured, "Remain threads to manage\n");

  printf("About to Init RU threads RC.nb_RU:%d\n", RC.nb_RU);
  config_sync_var=0;

  if (nfapi_mode==NFAPI_MODE_PNF) { // PNF
    wait_nfapi_init("main?");
  }

  printf("wait RUs\n");
  printf("ALL RUs READY!\n");
  printf("RC.nb_RU:%d\n", RC.nb_RU);
  // once all RUs are ready initialize the rest of the gNBs ((dependence on final RU parameters after configuration)
  printf("ALL RUs ready - init gNBs\n");
  LOG_E(PHY,"configuring RU from file,  hardcoded one gNB for one RU, one carrier\n");
  RU_t ru= {0};
  OCPconfig_RU(&ru);
  ru.nr_frame_parms->threequarter_fs=threequarter_fs;
  fill_rf_config(&ru,ru.rf_config_file);
  init_gNB_phase2(&ru);
  memcpy((void *)ru.nr_frame_parms,&RC.gNB[0][0].frame_parms,sizeof(NR_DL_FRAME_PARMS));
  memcpy((void *)&ru.config,(void *)&RC.gNB[0]->gNB_config,sizeof(ru.config));
  AssertFatal(setup_RU_buffers(&ru)==0,"Inconsistent configuration");
  nr_phy_init_RU(&ru);
  init_gNB_proc(0); // only instance 0 (one gNB per process)

  if (RC.nb_RU >0) {
    printf("Initializing RU threads\n");
    launch_NR_RU(&ru, get_softmodem_params()->rf_config_file);
  }

  if (opp_enabled ==1) {
    pthread_t t;
    threadCreate(&t, process_stats_thread,
                 (void *)NULL, "time_meas", -1, OAI_PRIORITY_RT_LOW);
  }

  if(IS_SOFTMODEM_DOSCOPE) {
    scopeParms_t tmp= {&argc, argv, &ru, RC.gNB[0]};
    load_softscope("nr",&tmp);
  }

  while(!oai_exit)
    sleep(1);

  logClean();
  printf("Bye.\n");
  return 0;
}
