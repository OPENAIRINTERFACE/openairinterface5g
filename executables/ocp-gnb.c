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

// should be in a shared lib
#include <forms.h>
#include <executables/stats.h>
#include <openair1/PHY/TOOLS/nr_phy_scope.h>

// Global vars
#include <openair2/LAYER2/MAC/mac_vars.h>
#include <openair1/PHY/phy_vars.h>
#include <openair2/RRC/LTE/rrc_vars.h>
#include <openair1/SCHED/sched_common_vars.h>
volatile int oai_exit;
int transmission_mode;
int single_thread_flag=1;
uint32_t do_forms=0;
unsigned int mmapped_dma=0;
int8_t threequarter_fs=0;

uint32_t target_dl_mcs = 28; //maximum allowed mcs
uint32_t target_ul_mcs = 20;
int chain_offset=0;
uint16_t sl_ahead=6;
uint16_t sf_ahead=6;
uint32_t timing_advance = 0;
int transmission_mode=1;
int emulate_rf = 0;
int numerology = 0;


unsigned char NB_gNB_INST = 1;
int config_sync_var=-1;
pthread_mutex_t nfapi_sync_mutex;
pthread_cond_t nfapi_sync_cond;
int nfapi_sync_var=-1;
uint8_t nfapi_mode = NFAPI_MONOLITHIC; // Default to monolithic mode
double cpuf;


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
static char *itti_dump_file = NULL;
static char *parallel_config = NULL;
static char *worker_config = NULL;
static double snr_dB=20;
static int DEFBANDS[] = {7};
static int DEFENBS[] = {0};
static int DEFBFW[] = {0x00007fff};

extern double cpuf;

short nr_mod_table[NR_MOD_TABLE_SIZE_SHORT] = {0,0,16384,16384,-16384,-16384,16384,16384,16384,-16384,-16384,16384,-16384,-16384,7327,7327,7327,21981,21981,7327,21981,21981,7327,-7327,7327,-21981,21981,-7327,21981,-21981,-7327,7327,-7327,21981,-21981,7327,-21981,21981,-7327,-7327,-7327,-21981,-21981,-7327,-21981,-21981,10726,10726,10726,3576,3576,10726,3576,3576,10726,17876,10726,25027,3576,17876,3576,25027,17876,10726,17876,3576,25027,10726,25027,3576,17876,17876,17876,25027,25027,17876,25027,25027,10726,-10726,10726,-3576,3576,-10726,3576,-3576,10726,-17876,10726,-25027,3576,-17876,3576,-25027,17876,-10726,17876,-3576,25027,-10726,25027,-3576,17876,-17876,17876,-25027,25027,-17876,25027,-25027,-10726,10726,-10726,3576,-3576,10726,-3576,3576,-10726,17876,-10726,25027,-3576,17876,-3576,25027,-17876,10726,-17876,3576,-25027,10726,-25027,3576,-17876,17876,-17876,25027,-25027,17876,-25027,25027,-10726,-10726,-10726,-3576,-3576,-10726,-3576,-3576,-10726,-17876,-10726,-25027,-3576,-17876,-3576,-25027,-17876,-10726,-17876,-3576,-25027,-10726,-25027,-3576,-17876,-17876,-17876,-25027,-25027,-17876,-25027,-25027,8886,8886,8886,12439,12439,8886,12439,12439,8886,5332,8886,1778,12439,5332,12439,1778,5332,8886,5332,12439,1778,8886,1778,12439,5332,5332,5332,1778,1778,5332,1778,1778,8886,19547,8886,15993,12439,19547,12439,15993,8886,23101,8886,26655,12439,23101,12439,26655,5332,19547,5332,15993,1778,19547,1778,15993,5332,23101,5332,26655,1778,23101,1778,26655,19547,8886,19547,12439,15993,8886,15993,12439,19547,5332,19547,1778,15993,5332,15993,1778,23101,8886,23101,12439,26655,8886,26655,12439,23101,5332,23101,1778,26655,5332,26655,1778,19547,19547,19547,15993,15993,19547,15993,15993,19547,23101,19547,26655,15993,23101,15993,26655,23101,19547,23101,15993,26655,19547,26655,15993,23101,23101,23101,26655,26655,23101,26655,26655,8886,-8886,8886,-12439,12439,-8886,12439,-12439,8886,-5332,8886,-1778,12439,-5332,12439,-1778,5332,-8886,5332,-12439,1778,-8886,1778,-12439,5332,-5332,5332,-1778,1778,-5332,1778,-1778,8886,-19547,8886,-15993,12439,-19547,12439,-15993,8886,-23101,8886,-26655,12439,-23101,12439,-26655,5332,-19547,5332,-15993,1778,-19547,1778,-15993,5332,-23101,5332,-26655,1778,-23101,1778,-26655,19547,-8886,19547,-12439,15993,-8886,15993,-12439,19547,-5332,19547,-1778,15993,-5332,15993,-1778,23101,-8886,23101,-12439,26655,-8886,26655,-12439,23101,-5332,23101,-1778,26655,-5332,26655,-1778,19547,-19547,19547,-15993,15993,-19547,15993,-15993,19547,-23101,19547,-26655,15993,-23101,15993,-26655,23101,-19547,23101,-15993,26655,-19547,26655,-15993,23101,-23101,23101,-26655,26655,-23101,26655,-26655,-8886,8886,-8886,12439,-12439,8886,-12439,12439,-8886,5332,-8886,1778,-12439,5332,-12439,1778,-5332,8886,-5332,12439,-1778,8886,-1778,12439,-5332,5332,-5332,1778,-1778,5332,-1778,1778,-8886,19547,-8886,15993,-12439,19547,-12439,15993,-8886,23101,-8886,26655,-12439,23101,-12439,26655,-5332,19547,-5332,15993,-1778,19547,-1778,15993,-5332,23101,-5332,26655,-1778,23101,-1778,26655,-19547,8886,-19547,12439,-15993,8886,-15993,12439,-19547,5332,-19547,1778,-15993,5332,-15993,1778,-23101,8886,-23101,12439,-26655,8886,-26655,12439,-23101,5332,-23101,1778,-26655,5332,-26655,1778,-19547,19547,-19547,15993,-15993,19547,-15993,15993,-19547,23101,-19547,26655,-15993,23101,-15993,26655,-23101,19547,-23101,15993,-26655,19547,-26655,15993,-23101,23101,-23101,26655,-26655,23101,-26655,26655,-8886,-8886,-8886,-12439,-12439,-8886,-12439,-12439,-8886,-5332,-8886,-1778,-12439,-5332,-12439,-1778,-5332,-8886,-5332,-12439,-1778,-8886,-1778,-12439,-5332,-5332,-5332,-1778,-1778,-5332,-1778,-1778,-8886,-19547,-8886,-15993,-12439,-19547,-12439,-15993,-8886,-23101,-8886,-26655,-12439,-23101,-12439,-26655,-5332,-19547,-5332,-15993,-1778,-19547,-1778,-15993,-5332,-23101,-5332,-26655,-1778,-23101,-1778,-26655,-19547,-8886,-19547,-12439,-15993,-8886,-15993,-12439,-19547,-5332,-19547,-1778,-15993,-5332,-15993,-1778,-23101,-8886,-23101,-12439,-26655,-8886,-26655,-12439,-23101,-5332,-23101,-1778,-26655,-5332,-26655,-1778,-19547,-19547,-19547,-15993,-15993,-19547,-15993,-15993,-19547,-23101,-19547,-26655,-15993,-23101,-15993,-26655,-23101,-19547,-23101,-15993,-26655,-19547,-26655,-15993,-23101,-23101,-23101,-26655,-26655,-23101,-26655,-26655};

static inline int rxtx(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx, int frame_tx, int slot_tx) {
  sl_ahead = sf_ahead*gNB->frame_parms.slots_per_subframe;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  start_meas(&softmodem_stats_rxtx_sf);

  // *******************************************************************

  if (nfapi_mode == NFAPI_MODE_PNF) {
    // I am a PNF and I need to let nFAPI know that we have a (sub)frame tick
    //add_subframe(&frame, &subframe, 4);
    //oai_subframe_ind(proc->frame_tx, proc->subframe_tx);
    //LOG_D(PHY, "oai_subframe_ind(frame:%u, subframe:%d) - NOT CALLED ********\n", frame, subframe);
    start_meas(&nfapi_meas);
    oai_subframe_ind(frame_rx, slot_rx);
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
  gNB->UL_INFO.frame     = frame_rx;
  gNB->UL_INFO.slot      = slot_rx;
  gNB->UL_INFO.module_id = gNB->Mod_id;
  gNB->UL_INFO.CC_id     = gNB->CC_id;
  gNB->if_inst->NR_UL_indication(&gNB->UL_INFO);
  pthread_mutex_unlock(&gNB->UL_INFO_mutex);
  // RX processing
  int tx_slot_type         = nr_slot_select(cfg,frame_tx,slot_tx);
  int rx_slot_type         = nr_slot_select(cfg,frame_rx,slot_rx);

  if (rx_slot_type == NR_UPLINK_SLOT || rx_slot_type == NR_MIXED_SLOT) {
    // UE-specific RX processing for subframe n
    // TODO: check if this is correct for PARALLEL_RU_L1_TRX_SPLIT
    phy_procedures_gNB_uespec_RX(gNB, frame_rx, slot_rx);
  }

  if (oai_exit) return(-1);

  // *****************************************
  // TX processing for subframe n+sf_ahead
  // run PHY TX procedures the one after the other for all CCs to avoid race conditions
  // (may be relaxed in the future for performance reasons)
  // *****************************************

  if (tx_slot_type == NR_DOWNLINK_SLOT || tx_slot_type == NR_MIXED_SLOT) {
    phy_procedures_gNB_TX(gNB, frame_tx,slot_tx, 1);
  }

  stop_meas( &softmodem_stats_rxtx_sf );
  LOG_D(PHY,"%s() Exit proc[rx:%d%d tx:%d%d]\n", __FUNCTION__, frame_rx, slot_rx, frame_tx, slot_tx);
  return(0);
}

void gNB_top(gNB_L1_proc_t *proc, struct RU_t_s *ru) {
  gNB_L1_rxtx_proc_t *L1_proc = &proc->L1_proc;
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  RU_proc_t *ru_proc=&ru->proc;
  sl_ahead = sf_ahead*fp->slots_per_subframe;
  L1_proc->frame_rx = proc->frame_rx = proc->frame_prach = ru_proc->frame_rx =
                                         (proc->timestamp_rx / (fp->samples_per_subframe*10))&1023;
  L1_proc->slot_rx = proc->slot_rx = proc->slot_prach =
                                       ru_proc->tti_rx; // computed before
  L1_proc->timestamp_tx = proc->timestamp_tx = ru_proc->timestamp_tx =
                            ru_proc->timestamp_rx + sl_ahead;
  L1_proc->frame_tx  = proc->frame_rx = ru_proc->frame_rx =
                                          (proc->timestamp_tx / (fp->samples_per_subframe*10))&1023;
  L1_proc->slot_tx   =  ru_proc->tti_tx =
                          (L1_proc->slot_rx + sl_ahead)%fp->slots_per_frame;
}

static void *process_stats_thread(void *param) {
  PHY_VARS_gNB *gNB  = (PHY_VARS_gNB *)param;
  reset_meas(&gNB->dlsch_encoding_stats);
  reset_meas(&gNB->dlsch_scrambling_stats);
  reset_meas(&gNB->dlsch_modulation_stats);
  wait_sync("process_stats_thread");

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
}

/// eNB kept in function name for nffapi calls, TO FIX
void init_eNB_afterRU(void) {
  int inst,ru_id,i,aa;
  PHY_VARS_gNB *gNB;
  LOG_I(PHY,"%s() RC.nb_nr_inst:%d\n", __FUNCTION__, RC.nb_nr_inst);

  for (inst=0; inst<RC.nb_nr_inst; inst++) {
    LOG_I(PHY,"RC.nb_nr_CC[inst:%d]:%p\n", inst, RC.gNB[inst]);
    gNB                                  =  RC.gNB[inst];
    phy_init_nr_gNB(gNB,0,0);
    LOG_I(PHY,"Mapping RX ports from %d RUs to gNB %d\n",gNB->num_RU,gNB->Mod_id);
    //LOG_I(PHY,"Overwriting gNB->prach_vars.rxsigF[0]:%p\n", gNB->prach_vars.rxsigF[0]);
    //      gNB->prach_vars.rxsigF[0] = (int16_t **)malloc16(64*sizeof(int16_t *));
    LOG_I(PHY,"gNB->num_RU:%d\n", gNB->num_RU);

    for (ru_id=0,aa=0; ru_id<gNB->num_RU; ru_id++) {
      AssertFatal(gNB->RU_list[ru_id]->common.rxdataF!=NULL,
                  "RU %d : common.rxdataF is NULL\n",
                  gNB->RU_list[ru_id]->idx);
      AssertFatal(gNB->RU_list[ru_id]->prach_rxsigF!=NULL,
                  "RU %d : prach_rxsigF is NULL\n",
                  gNB->RU_list[ru_id]->idx);

      for (i=0; i<gNB->RU_list[ru_id]->nb_rx; aa++,i++) {
        LOG_I(PHY,"Attaching RU %d antenna %d to gNB antenna %d\n",gNB->RU_list[ru_id]->idx,i,aa);
        gNB->prach_vars.rxsigF[aa]    =  gNB->RU_list[ru_id]->prach_rxsigF[i];
        gNB->common_vars.rxdataF[aa]     =  gNB->RU_list[ru_id]->common.rxdataF[i];
      }
    }

    //init_precoding_weights(RC.gNB[inst][CC_id]);
  }
}

void init_gNB(int single_thread_flag,int wait_for_sync) {
  if (RC.gNB == NULL)
    RC.gNB = (PHY_VARS_gNB **) calloc(1,(1+RC.nb_nr_L1_inst)*sizeof(PHY_VARS_gNB *));

  for (int inst=0; inst<RC.nb_nr_L1_inst; inst++) {
    if (RC.gNB[inst] == NULL )
      RC.gNB[inst] = (PHY_VARS_gNB *) calloc(1,sizeof(PHY_VARS_gNB));

    PHY_VARS_gNB *gNB= RC.gNB[inst];
    gNB->abstraction_flag   = false;
    gNB->single_thread_flag = true;
    /*nr_polar_init(&gNB->nrPolar_params,
              NR_POLAR_PBCH_MESSAGE_TYPE,
        NR_POLAR_PBCH_PAYLOAD_BITS,
        NR_POLAR_PBCH_AGGREGATION_LEVEL);*/
    LOG_I(PHY,"Registering with MAC interface module\n");
    AssertFatal((gNB->if_inst         = NR_IF_Module_init(inst))!=NULL,"Cannot register interface");
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
    NB_gNB_INST = RC.nb_nr_inst;
    NB_RU   = RC.nb_RU;
    printf("Configuration: nb_rrc_inst %d, nb_nr_L1_inst %d, nb_ru %hhu\n",NB_gNB_INST,RC.nb_nr_L1_inst,NB_RU);
  }
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

void init_main_gNB(void) {
  RCconfig_NR_L1();
  RCconfig_nr_macrlc();

  if (RC.nb_nr_L1_inst>0) AssertFatal(l1_north_init_gNB()==0,"could not initialize L1 north interface\n");

  LOG_I(GNB_APP,"Allocating gNB_RRC_INST for %d instances\n",RC.nb_nr_inst);
  RC.nrrrc = (gNB_RRC_INST **)malloc(RC.nb_nr_inst*sizeof(gNB_RRC_INST *));
  LOG_I(PHY, "%s() RC.nb_nr_inst:%d RC.nrrrc:%p\n", __FUNCTION__, RC.nb_nr_inst, RC.nrrrc);
  int gnb_id=0; // only 1 gnb per process, index 0 for now
  RC.nrrrc[gnb_id] = (gNB_RRC_INST *)malloc(sizeof(gNB_RRC_INST));
  memset((void *)RC.nrrrc[gnb_id],0,sizeof(gNB_RRC_INST));
  MessageDef *msg_p = itti_alloc_new_message (TASK_GNB_APP, NRRRC_CONFIGURATION_REQ);
  RCconfig_NRRRC(msg_p,gnb_id, RC.nrrrc[gnb_id]);
  openair_rrc_gNB_configuration(GNB_INSTANCE_TO_MODULE_ID(ITTI_MSG_INSTANCE(msg_p)), &NRRRC_CONFIGURATION_REQ(msg_p));
}

static  void wait_nfapi_init(char *thread_name) {
  printf( "waiting for NFAPI PNF connection and population of global structure (%s)\n",thread_name);
  pthread_mutex_lock( &nfapi_sync_mutex );

  while (nfapi_sync_var<0)
    pthread_cond_wait( &nfapi_sync_cond, &nfapi_sync_mutex );

  pthread_mutex_unlock(&nfapi_sync_mutex);
  printf( "NFAPI: got sync (%s)\n", thread_name);
}

void wait_gNBs(void) {
  int waiting=1;

  while (waiting==1) {
    printf("Waiting for gNB L1 instances to all get configured ... sleeping 50ms (nb_nr_sL1_inst %d)\n",RC.nb_nr_L1_inst);
    usleep(50*1000);
    waiting=0;

    for (int i=0; i<RC.nb_nr_L1_inst; i++) {
      if (RC.gNB[i]->configured==0) {
        waiting=1;
        break;
      }
    }
  }

  printf("gNB L1 are configured\n");
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

void stop_RU(int nb_ru) {
}

void RCconfig_RU(void) {
  int i = 0, j = 0;
  paramdef_t RUParams[] = RUPARAMS_DESC;
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};
  config_getlist( &RUParamList, RUParams, sizeof(RUParams)/sizeof(paramdef_t), NULL);

  if ( RUParamList.numelt > 0) {
    RC.ru = (RU_t **)malloc(RC.nb_RU*sizeof(RU_t *));
    RC.ru_mask=(1<<NB_RU) - 1;
    printf("Set RU mask to %lx\n",RC.ru_mask);

    for (j = 0; j < RC.nb_RU; j++) {
      RC.ru[j]                                      = (RU_t *)malloc(sizeof(RU_t));
      memset((void *)RC.ru[j],0,sizeof(RU_t));
      RC.ru[j]->idx                                 = j;
      RC.ru[j]->nr_frame_parms                      = (NR_DL_FRAME_PARMS *)malloc(sizeof(NR_DL_FRAME_PARMS));
      RC.ru[j]->frame_parms                         = (LTE_DL_FRAME_PARMS *)malloc(sizeof(LTE_DL_FRAME_PARMS));
      printf("Creating RC.ru[%d]:%p\n", j, RC.ru[j]);
      RC.ru[j]->if_timing                           = synch_to_ext_device;

      if (RC.nb_nr_L1_inst >0)
        RC.ru[j]->num_gNB                           = RUParamList.paramarray[j][RU_ENB_LIST_IDX].numelt;
      else
        RC.ru[j]->num_gNB                           = 0;

      for (i=0; i<RC.ru[j]->num_gNB; i++) RC.ru[j]->gNB_list[i] = &RC.gNB[RUParamList.paramarray[j][RU_ENB_LIST_IDX].iptr[i]][0];

      if (config_isparamset(RUParamList.paramarray[j], RU_SDR_ADDRS)) {
        RC.ru[j]->openair0_cfg.sdr_addrs = strdup(*(RUParamList.paramarray[j][RU_SDR_ADDRS].strptr));
      }

      if (config_isparamset(RUParamList.paramarray[j], RU_SDR_CLK_SRC)) {
        if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "internal") == 0) {
          RC.ru[j]->openair0_cfg.clock_source = internal;
          LOG_D(PHY, "RU clock source set as internal\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "external") == 0) {
          RC.ru[j]->openair0_cfg.clock_source = external;
          LOG_D(PHY, "RU clock source set as external\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "gpsdo") == 0) {
          RC.ru[j]->openair0_cfg.clock_source = gpsdo;
          LOG_D(PHY, "RU clock source set as gpsdo\n");
        } else {
          LOG_E(PHY, "Erroneous RU clock source in the provided configuration file: '%s'\n", *(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr));
        }
      } else {
        RC.ru[j]->openair0_cfg.clock_source = unset;
      }

      if (strcmp(*(RUParamList.paramarray[j][RU_LOCAL_RF_IDX].strptr), "yes") == 0) {
        if ( !(config_isparamset(RUParamList.paramarray[j],RU_LOCAL_IF_NAME_IDX)) ) {
          RC.ru[j]->if_south                        = LOCAL_RF;
          RC.ru[j]->function                        = gNodeB_3GPP;
          printf("Setting function for RU %d to gNodeB_3GPP\n",j);
        } else {
        }

        RC.ru[j]->max_pdschReferenceSignalPower     = *(RUParamList.paramarray[j][RU_MAX_RS_EPRE_IDX].uptr);;
        RC.ru[j]->max_rxgain                        = *(RUParamList.paramarray[j][RU_MAX_RXGAIN_IDX].uptr);
        RC.ru[j]->num_bands                         = RUParamList.paramarray[j][RU_BAND_LIST_IDX].numelt;

        for (i=0; i<RC.ru[j]->num_bands; i++) RC.ru[j]->band[i] = RUParamList.paramarray[j][RU_BAND_LIST_IDX].iptr[i];
      } //strcmp(local_rf, "yes") == 0
      else {
      }

      RC.ru[j]->nb_tx                             = *(RUParamList.paramarray[j][RU_NB_TX_IDX].uptr);
      RC.ru[j]->nb_rx                             = *(RUParamList.paramarray[j][RU_NB_RX_IDX].uptr);
      RC.ru[j]->att_tx                            = *(RUParamList.paramarray[j][RU_ATT_TX_IDX].uptr);
      RC.ru[j]->att_rx                            = *(RUParamList.paramarray[j][RU_ATT_RX_IDX].uptr);

      if (config_isparamset(RUParamList.paramarray[j], RU_BF_WEIGHTS_LIST_IDX)) {
        RC.ru[j]->nb_bfw = RUParamList.paramarray[j][RU_BF_WEIGHTS_LIST_IDX].numelt;

        for (i=0; i<RC.ru[j]->num_gNB; i++)  {
          RC.ru[j]->bw_list[i] = (int32_t *)malloc16_clear((RC.ru[j]->nb_bfw)*sizeof(int32_t));

          for (int b=0; b<RC.ru[j]->nb_bfw; b++) RC.ru[j]->bw_list[i][b] = RUParamList.paramarray[j][RU_BF_WEIGHTS_LIST_IDX].iptr[b];
        }
      }
    }// j=0..num_rus
  } else {
    RC.nb_RU = 0;
  } // setting != NULL

  return;
}

int rx_rf(RU_t *ru, int lastReadSz) {
  RU_proc_t *proc = &ru->proc;
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  void *rxp[ru->nb_rx];
  uint32_t samples_per_slot = fp->get_samples_per_slot(proc->tti_rx,fp);

  for (int i=0; i<ru->nb_rx; i++)
    rxp[i] = (void *)&ru->common.rxdata[i][fp->get_samples_slot_timestamp(proc->tti_rx,fp,0)];

  openair0_timestamp old_ts = proc->timestamp_rx;
  LOG_D(PHY,"Reading %d samples for slot %d (%p)\n",samples_per_slot,proc->tti_rx,rxp[0]);
  unsigned int rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
                     & proc->timestamp_rx,
                     rxp,
                     samples_per_slot,
                     ru->nb_rx);

  if (rxs != samples_per_slot)
    LOG_E(PHY, "rx_rf: Asked for %d samples, got %d from USRP\n",samples_per_slot,rxs);

  // In case we need offset, between RU or any other need
  // The sync system can put a offset to use betwwen RF boards
  proc->timestamp_rx+=ru->ts_offset;

  if (lastReadSz && proc->timestamp_rx - old_ts != lastReadSz)
    LOG_D(PHY,"rx_rf: rfdevice timing drift of %"PRId64" samples (ts_off %"PRId64")\n",
          proc->timestamp_rx - old_ts - samples_per_slot,ru->ts_offset);

  return rxs;
}

void tx_rf(RU_t *ru,int frame,int slot, uint64_t timestamp) {
  RU_proc_t *proc = &ru->proc;
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &ru->gNB_list[0]->gNB_config;
  void *txp[ru->nb_tx];
  unsigned int txs;
  int i,txsymb;
  T(T_ENB_PHY_OUTPUT_SIGNAL, T_INT(0), T_INT(0), T_INT(frame), T_INT(slot),
    T_INT(0), T_BUFFER(&ru->common.txdata[0][fp->get_samples_slot_timestamp(slot,fp,0)], fp->samples_per_subframe * 4));
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

    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_WRITE_FLAGS, flags );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, frame );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_TX0_RU, slot );

    for (i=0; i<ru->nb_tx; i++)
      txp[i] = (void *)&ru->common.txdata[i][fp->get_samples_slot_timestamp(slot,fp,0)-sf_extension];

    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, (timestamp-ru->openair0_cfg.tx_sample_advance)&0xffffffff );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
    // prepare tx buffer pointers
    txs = ru->rfdevice.trx_write_func(&ru->rfdevice,
                                      timestamp+ru->ts_offset-ru->openair0_cfg.tx_sample_advance-sf_extension,
                                      txp,
                                      siglen+sf_extension,
                                      ru->nb_tx,
                                      flags);
    LOG_D(PHY,"[TXPATH] RU %d tx_rf, writing to TS %llu, frame %d, unwrapped_frame %d, slot %d\n",ru->idx,
          (long long unsigned int)timestamp,frame,proc->frame_tx_unwrap,slot);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );
    AssertFatal(txs ==  siglen+sf_extension,"TX : Timeout (sent %u/%d)\n", txs, siglen);
  }
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
static void *ru_thread( void *param ) {
  RU_t               *ru      = (RU_t *)param;
  RU_proc_t          *proc    = &ru->proc;
  NR_DL_FRAME_PARMS  *fp      = ru->nr_frame_parms;
  LOG_I(PHY,"Starting RU %d (%s,%s),\n",ru->idx,NB_functions[ru->function],NB_timing[ru->if_timing]);
  fp->threequarter_fs=threequarter_fs;
  nr_init_frame_parms(&ru->gNB_list[0]->gNB_config, fp);
  nr_dump_frame_parms(fp);
  fill_rf_config(ru,ru->rf_config_file);
  nr_phy_init_RU(ru);
  AssertFatal(openair0_device_load(&ru->rfdevice,&ru->openair0_cfg)==0,"Cannot connect to local radio\n");
  AssertFatal(ru->rfdevice.trx_start_func(&ru->rfdevice) == 0,"Could not start the RF device\n");
  // This is a forever while loop, it loops over subframes which are scheduled by incoming samples from HW devices
  int lastReadSz=0;

  while (!oai_exit) {
    // synchronization on input FH interface, acquire signals/data and block
    lastReadSz=rx_rf(ru, lastReadSz);
    // do RX front-end processing (frequency-shift, dft) if needed
    uint32_t idx_sf = proc->timestamp_rx / fp->samples_per_subframe;
    uint32_t offsetInSubframe=proc->timestamp_rx % fp->samples_per_subframe;
    proc->tti_rx = (idx_sf * fp->slots_per_subframe +
                    lroundf((float)offsetInSubframe / fp->samples_per_slot0))%(fp->slots_per_frame);

    if (proc->tti_rx == NR_UPLINK_SLOT || fp->frame_type == FDD) {
      nr_fep_full(ru,proc->tti_rx);
    }

    gNB_top(&RC.gNB[0][0].proc, ru);
    gNB_L1_rxtx_proc_t *L1_proc = &RC.gNB[0][0].proc.L1_proc;

    if (rxtx(&RC.gNB[0][0],L1_proc->frame_rx,L1_proc->slot_rx,L1_proc->frame_tx,L1_proc->slot_tx) < 0)
      LOG_E(PHY,"gNB %d CC_id %d failed during execution\n",RC.gNB[0][0].Mod_id,RC.gNB[0][0].CC_id);

    // do TX front-end processing if needed (precoding and/or IDFTs)
    //ru->feptx_prec(ru,proc->frame_tx,proc->tti_tx);
    nr_feptx_prec(ru,proc->frame_tx,proc->tti_tx);
    // do OFDM with/without TX front-end processing  if needed
    //ru->feptx_ofdm
    nfapi_nr_config_request_scf_t *cfg = &ru->gNB_list[0]->gNB_config;

    if (nr_slot_select(cfg,proc->frame_tx, proc->tti_tx ) != SF_UL) {
      int aa=0; // antenna 0 hardcoded
      NR_DL_FRAME_PARMS *fp=ru->nr_frame_parms;
      nr_feptx0(ru,proc->tti_tx,0,fp->symbols_per_slot,aa);
    }

    // do outgoing fronthaul (south) if needed
    tx_rf(ru,proc->frame_tx,proc->tti_tx,proc->timestamp_tx);
  }

  printf( "Exiting ru_thread \n");
  ru->rfdevice.trx_end_func(&ru->rfdevice);
  static int ru_thread_status = 0;
  return &ru_thread_status;
}
void launch_NR_RU(char *rf_config_file) {
  printf("configuring RU from file\n");
  RCconfig_RU();
  LOG_I(PHY,"number of L1 instances %d, number of RU %d, number of CPU cores %d\n",
        RC.nb_nr_L1_inst,RC.nb_RU,get_nprocs());
  LOG_D(PHY,"Process RUs RC.nb_RU:%d\n",RC.nb_RU);

  for (int ru_id=0; ru_id<RC.nb_RU; ru_id++) {
    LOG_D(PHY,"Process RC.ru[%d]\n",ru_id);
    RU_t *ru                 = RC.ru[ru_id];
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
    memcpy((void *)ru->nr_frame_parms,&RC.gNB[0][0].frame_parms,sizeof(NR_DL_FRAME_PARMS));

    for (int i=0; i<ru->num_gNB; i++) {
      PHY_VARS_gNB *gNB0 = ru->gNB_list[i];
      gNB0->RU_list[gNB0->num_RU++] = ru;
    }

    RU_proc_t *proc = &ru->proc;
    threadCreate( &proc->pthread_FH, ru_thread, (void *)ru, "thread_FH", -1, OAI_PRIORITY_RT_MAX );
  }
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

  if (CONFIG_ISFLAGSET(CONFIG_ABORT) ) {
    fprintf(stderr,"Getting configuration failed\n");
    exit(-1);
  }

  cpuf=get_cpu_freq_GHz();
  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info);
  set_taus_seed (0);
  init_opt();

  if(IS_SOFTMODEM_NOS1)
    init_pdcp();

  init_main_gNB();
  /* Start the agent. If it is turned off in the configuration, it won't start */
  RCconfig_nr_flexran();

  for (int i = 0; i < RC.nb_nr_L1_inst; i++) {
    flexran_agent_start(i);
  }

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

  number_of_cards = 1;
  LOG_I(PHY,"Initializing gNB threads\n");
  init_gNB(single_thread_flag,wait_for_sync);
  printf("wait_gNBs()\n");
  wait_gNBs();
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

  if (nfapi_mode != NFAPI_MODE_PNF && nfapi_mode != NFAPI_MODE_VNF) {
    printf("Not NFAPI mode - call init_eNB_afterRU()\n");
    init_eNB_afterRU();
  } else {
    printf("NFAPI mode - DO NOT call init_gNB_afterRU()\n");
  }

  init_gNB_proc(0); // only instance 0 (one gNB per process)

  if (RC.nb_RU >0) {
    printf("Initializing RU threads\n");
    launch_NR_RU(get_softmodem_params()->rf_config_file);
  }

  if (opp_enabled ==1) {
    pthread_t t;
    threadCreate(&t, process_stats_thread,
                 (void *)NULL, "time_meas", -1, OAI_PRIORITY_RT_LOW);
  }

  if (do_forms==1) {
    scopeParms_t tmp= {&argc, argv};
    startScope(&tmp);
  }

  while(!oai_exit)
    sleep(1);

  for(int ru_id=0; ru_id<NB_RU; ru_id++) {
    if (RC.ru[ru_id]->rfdevice.trx_end_func)
      RC.ru[ru_id]->rfdevice.trx_end_func(&RC.ru[ru_id]->rfdevice);

    if (RC.ru[ru_id]->ifdevice.trx_end_func)
      RC.ru[ru_id]->ifdevice.trx_end_func(&RC.ru[ru_id]->ifdevice);
  }

  logClean();
  printf("Bye.\n");
  return 0;
}
