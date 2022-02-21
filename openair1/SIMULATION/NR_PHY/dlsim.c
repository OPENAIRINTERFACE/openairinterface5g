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

#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "common/ran_context.h"
#include "common/config/config_userapi.h"
#include "common/utils/nr/nr_common.h"
#include "common/utils/LOG/log.h"
#include "LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "LAYER2/NR_MAC_UE/mac_defs.h"
#include "LAYER2/NR_MAC_UE/mac_extern.h"
#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/phy_vars_nr_ue.h"
#include "PHY/types.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "SCHED_NR/fapi_nr_l1.h"
#include "SCHED_NR/sched_nr.h"
#include "SCHED_NR_UE/defs.h"
#include "SCHED_NR_UE/fapi_nr_ue_l1.h"
#include "NR_PHY_INTERFACE/NR_IF_Module.h"
#include "NR_UE_PHY_INTERFACE/NR_IF_Module.h"

#include "LAYER2/NR_MAC_UE/mac_proto.h"
//#include "LAYER2/NR_MAC_gNB/mac_proto.h"
//#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "NR_asn_constant.h"
#include "RRC/NR/MESSAGES/asn1_msg.h"
#include "openair1/SIMULATION/RF/rf.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
#include "openair1/SIMULATION/NR_PHY/nr_unitary_defs.h"
//#include "openair1/SIMULATION/NR_PHY/nr_dummy_functions.c"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "NR_RRCReconfiguration.h"
#define inMicroS(a) (((double)(a))/(get_cpu_freq_GHz()*1000.0))
#include "SIMULATION/LTE_PHY/common_sim.h"

#include <openair2/LAYER2/MAC/mac_vars.h>
#include <openair2/RRC/LTE/rrc_vars.h>

#include <executables/softmodem-common.h>
#include <openair3/ocp-gtpu/gtp_itf.h>

LCHAN_DESC DCCH_LCHAN_DESC,DTCH_DL_LCHAN_DESC,DTCH_UL_LCHAN_DESC;
rlc_info_t Rlc_info_um,Rlc_info_am_config;

PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];

double cpuf;
char *uecap_file;

uint16_t sf_ahead=4 ;
uint16_t sl_ahead=0;
//uint8_t nfapi_mode = 0;
uint64_t downlink_frequency[MAX_NUM_CCs][4];
THREAD_STRUCT thread_struct;
nfapi_ue_release_request_body_t release_rntis;
uint32_t N_RB_DL = 106;
//Fixme: Uniq dirty DU instance, by global var, datamodel need better management
instance_t DUuniqInstance=0;
instance_t CUuniqInstance=0;
teid_t newGtpuCreateTunnel(instance_t instance, rnti_t rnti, int incoming_bearer_id, int outgoing_bearer_id, teid_t outgoing_teid,
                           transport_layer_addr_t remoteAddr, int port, gtpCallback callBack) {
return 0;
}

// dummy functions
int dummy_nr_ue_ul_indication(nr_uplink_indication_t *ul_info)              { return(0);  }

int8_t nr_mac_rrc_data_ind_ue(const module_id_t module_id,
                              const int CC_id,
                              const uint8_t gNB_index,
                              const frame_t frame,
                              const sub_frame_t sub_frame,
                              const rnti_t rnti,
                              const channel_t channel,
                              const uint8_t* pduP,
                              const sdu_size_t pdu_len)
{
  return 0;
}

void nr_rrc_ue_generate_RRCSetupRequest(module_id_t module_id, const uint8_t gNB_index)
{
  return;
}

int8_t nr_mac_rrc_data_req_ue(const module_id_t Mod_idP,
                              const int         CC_id,
                              const uint8_t     gNB_id,
                              const frame_t     frameP,
                              const rb_id_t     Srb_id,
                              uint8_t           *buffer_pP)
{
  return 0;
}

void
rrc_data_ind(
  const protocol_ctxt_t *const ctxt_pP,
  const rb_id_t                Srb_id,
  const sdu_size_t             sdu_sizeP,
  const uint8_t   *const       buffer_pP
)
{
}

int
gtpv1u_create_s1u_tunnel(
  const instance_t                              instanceP,
  const gtpv1u_enb_create_tunnel_req_t *const  create_tunnel_req_pP,
  gtpv1u_enb_create_tunnel_resp_t *const create_tunnel_resp_pP
) {
  return 0;
}

int
rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  const gtpv1u_enb_create_tunnel_resp_t *const create_tunnel_resp_pP,
  uint8_t                         *inde_list
) {
  return 0;
}

int
gtpv1u_create_ngu_tunnel(
  const instance_t instanceP,
  const gtpv1u_gnb_create_tunnel_req_t *  const create_tunnel_req_pP,
        gtpv1u_gnb_create_tunnel_resp_t * const create_tunnel_resp_pP){
  return 0;
}

int
gtpv1u_update_ngu_tunnel(
  const instance_t                              instanceP,
  const gtpv1u_gnb_create_tunnel_req_t *const  create_tunnel_req_pP,
  const rnti_t                                  prior_rnti
){
  return 0;
}

int ocp_gtpv1u_delete_s1u_tunnel(const instance_t instance, const gtpv1u_enb_delete_tunnel_req_t *const req_pP) {
  return 0;
}

int
nr_rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  const gtpv1u_gnb_create_tunnel_resp_t *const create_tunnel_resp_pP,
  uint8_t                         *inde_list
){
  return 0;
}

int nr_derive_key(int alg_type, uint8_t alg_id,
               const uint8_t key[32], uint8_t **out)
{
  return 0;
}

void config_common(int Mod_idP,
                   int ssb_SubcarrierOffset,
                   int pdsch_AntennaPorts,
                   int pusch_AntennaPorts,
		   NR_ServingCellConfigCommon_t *scc
		   );

int generate_dlsch_header(unsigned char *mac_header,
                          unsigned char num_sdus,
                          unsigned short *sdu_lengths,
                          unsigned char *sdu_lcids,
                          unsigned char drx_cmd,
                          unsigned short timing_advance_cmd,
                          unsigned char *ue_cont_res_id,
                          unsigned char short_padding,
                          unsigned short post_padding){return 0;}

// Dummy function to avoid linking error at compilation of nr-dlsim
int is_x2ap_enabled(void)
{
  return 0;
}

int DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(module_id_t     module_idP,
                                            int             CC_idP,
                                            int             UE_id,
                                            rnti_t          rntiP,
                                            const uint8_t   *sduP,
                                            sdu_size_t      sdu_lenP,
                                            const uint8_t   *sdu2P,
                                            sdu_size_t      sdu2_lenP) {
  return 0;
}

void processSlotTX(void *arg) {}

//nFAPI P7 dummy functions to avoid linking errors 

int oai_nfapi_dl_tti_req(nfapi_nr_dl_tti_request_t *dl_config_req) { return(0);  }
int oai_nfapi_tx_data_req(nfapi_nr_tx_data_request_t *tx_data_req){ return(0);  }
int oai_nfapi_ul_dci_req(nfapi_nr_ul_dci_request_t *ul_dci_req){ return(0);  }
int oai_nfapi_ul_tti_req(nfapi_nr_ul_tti_request_t *ul_tti_req){ return(0);  }
int oai_nfapi_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind) { return(0);  }
int oai_nfapi_nr_crc_indication(nfapi_nr_crc_indication_t *ind) { return(0);  }
int oai_nfapi_nr_srs_indication(nfapi_nr_srs_indication_t *ind) { return(0);  }
int oai_nfapi_nr_uci_indication(nfapi_nr_uci_indication_t *ind) { return(0);  }
int oai_nfapi_nr_rach_indication(nfapi_nr_rach_indication_t *ind) { return(0);  }

// needed for some functions
openair0_config_t openair0_cfg[MAX_CARDS];
void update_ptrs_config(NR_CellGroupConfig_t *secondaryCellGroup, uint16_t *rbSize, uint8_t *mcsIndex,int8_t *ptrs_arg);
void update_dmrs_config(NR_CellGroupConfig_t *scg, int8_t* dmrs_arg);
extern void fix_scd(NR_ServingCellConfig_t *scd);// forward declaration

/* specific dlsim DL preprocessor: uses rbStart/rbSize/mcs/nrOfLayers from command line of
   dlsim, does not search for CCE/PUCCH occasion but simply sets to 0 */
int g_mcsIndex = -1, g_mcsTableIdx = 0, g_rbStart = -1, g_rbSize = -1, g_nrOfLayers = 1;
void nr_dlsim_preprocessor(module_id_t module_id,
                           frame_t frame,
                           sub_frame_t slot) {
  NR_UE_info_t *UE_info = &RC.nrmac[module_id]->UE_info;
  AssertFatal(UE_info->num_UEs == 1, "can have only a single UE\n");
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[0];
  NR_ServingCellConfigCommon_t *scc = RC.nrmac[0]->common_channels[0].ServingCellConfigCommon;

  /* manually set free CCE to 0 */
  const int target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
  sched_ctrl->search_space = get_searchspace(module_id, scc, sched_ctrl->active_bwp ? sched_ctrl->active_bwp->bwp_Dedicated : NULL, target_ss);
  uint8_t nr_of_candidates;
  find_aggregation_candidates(&sched_ctrl->aggregation_level,
                              &nr_of_candidates,
                              sched_ctrl->search_space,4);
  sched_ctrl->coreset = get_coreset(module_id, scc, sched_ctrl->active_bwp->bwp_Dedicated, sched_ctrl->search_space, target_ss);
  sched_ctrl->cce_index = 0;

  NR_pdsch_semi_static_t *ps = &sched_ctrl->pdsch_semi_static;

  ps->nrOfLayers = g_nrOfLayers;

  int dci_format = sched_ctrl->search_space && sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats ?
      NR_DL_DCI_FORMAT_1_1 : NR_DL_DCI_FORMAT_1_0;

  nr_set_pdsch_semi_static(module_id,
                           scc,
                           UE_info->CellGroup[0],
                           sched_ctrl->active_bwp,
                           NULL,
                           /* tda = */ 2,
                           dci_format,
                           ps);

  NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
  sched_pdsch->rbStart = g_rbStart;
  sched_pdsch->rbSize = g_rbSize;
  sched_pdsch->mcs = g_mcsIndex;
  /* the following might override the table that is mandated by RRC
   * configuration */
  ps->mcsTableIdx = g_mcsTableIdx;

  sched_pdsch->Qm = nr_get_Qm_dl(sched_pdsch->mcs, ps->mcsTableIdx);
  sched_pdsch->R = nr_get_code_rate_dl(sched_pdsch->mcs, ps->mcsTableIdx);
  sched_pdsch->tb_size = nr_compute_tbs(sched_pdsch->Qm,
                                        sched_pdsch->R,
                                        sched_pdsch->rbSize,
                                        ps->nrOfSymbols,
                                        ps->N_PRB_DMRS * ps->N_DMRS_SLOT,
                                        0 /* N_PRB_oh, 0 for initialBWP */,
                                        0 /* tb_scaling */,
                                        ps->nrOfLayers)
                         >> 3;

  /* the simulator assumes the HARQ PID is equal to the slot number */
  sched_pdsch->dl_harq_pid = slot;

  /* The scheduler uses lists to track whether a HARQ process is
   * free/busy/awaiting retransmission, and updates the HARQ process states.
   * However, in the simulation, we never get ack or nack for any HARQ process,
   * thus the list and HARQ states don't match what the scheduler expects.
   * Therefore, below lines just "repair" everything so that the scheduler
   * won't remark that there is no HARQ feedback */
  sched_ctrl->feedback_dl_harq.head = -1; // always overwrite feedback HARQ process
  if (sched_ctrl->harq_processes[slot].round == 0) // depending on round set in simulation ...
    add_front_nr_list(&sched_ctrl->available_dl_harq, slot); // ... make PID available
  else
    add_front_nr_list(&sched_ctrl->retrans_dl_harq, slot);   // ... make PID retransmission
  sched_ctrl->harq_processes[slot].is_waiting = false;
  AssertFatal(sched_pdsch->rbStart >= 0, "invalid rbStart %d\n", sched_pdsch->rbStart);
  AssertFatal(sched_pdsch->rbSize > 0, "invalid rbSize %d\n", sched_pdsch->rbSize);
  AssertFatal(sched_pdsch->mcs >= 0, "invalid mcs %d\n", sched_pdsch->mcs);
  AssertFatal(ps->mcsTableIdx >= 0 && ps->mcsTableIdx <= 2, "invalid mcsTableIdx %d\n", ps->mcsTableIdx);
}

typedef struct {
  uint64_t       optmask;   //mask to store boolean config options
  uint8_t        nr_dlsch_parallel; // number of threads for dlsch decoding, 0 means no parallelization
  tpool_t        Tpool;             // thread pool
} nrUE_params_t;

nrUE_params_t nrUE_params;

nrUE_params_t *get_nrUE_params(void) {
  return &nrUE_params;
}

void do_nothing(void *args) {
}

int main(int argc, char **argv)
{
  char c;
  int i,aa;//,l;
  double sigma2, sigma2_dB=10, SNR, snr0=-2.0, snr1=2.0;
  uint8_t snr1set=0;
  double roundStats[500] = {0};
  double blerStats[500] = {0};
  double berStats[500] = {0};
  double snrStats[500] = {0};
  float effRate;
  //float psnr;
  float eff_tp_check = 0.7;
  uint8_t snrRun;
  uint32_t TBS = 0;
  int **txdata;
  double **s_re,**s_im,**r_re,**r_im;
  //double iqim = 0.0;
  //unsigned char pbch_pdu[6];
  //  int sync_pos, sync_pos_slot;
  //  FILE *rx_frame_file;
  FILE *output_fd = NULL;
  //uint8_t write_output_file=0;
  //int result;
  //int freq_offset;
  //  int subframe_offset;
  //  char fname[40], vname[40];
  int trial, n_trials = 1, n_errors = 0, n_false_positive = 0;
  //int n_errors2, n_alamouti;
  uint8_t n_tx=1,n_rx=1;
  uint8_t round;
  uint8_t num_rounds = 4;
  char gNBthreads[128]="n";

  channel_desc_t *gNB2UE;
  //uint32_t nsymb,tx_lev,tx_lev1 = 0,tx_lev2 = 0;
  //uint8_t extended_prefix_flag=0;
  //int8_t interf1=-21,interf2=-21;

  FILE *input_fd=NULL,*pbch_file_fd=NULL;
  //char input_val_str[50],input_val_str2[50];

  //uint8_t frame_mod4,num_pdcch_symbols = 0;

  SCM_t channel_model = AWGN; // AWGN Rayleigh1 Rayleigh1_anticorr;

  NB_UE_INST = 1;
  //double pbch_sinr;
  //int pbch_tx_ant;
  int N_RB_DL=106,mu=1;

  //unsigned char frame_type = 0;

  int frame=1,slot=1;
  int frame_length_complex_samples;
  //int frame_length_complex_samples_no_prefix;
  NR_DL_FRAME_PARMS *frame_parms;
  UE_nr_rxtx_proc_t UE_proc;
  NR_Sched_Rsp_t Sched_INFO;
  gNB_MAC_INST *gNB_mac;
  NR_UE_MAC_INST_t *UE_mac;
  int cyclic_prefix_type = NFAPI_CP_NORMAL;
  int run_initial_sync=0;
  int loglvl=OAILOG_WARNING;

  //float target_error_rate = 0.01;
  int css_flag=0;

  cpuf = get_cpu_freq_GHz();
  int8_t enable_ptrs = 0;
  int8_t modify_dmrs = 0;

  int8_t dmrs_arg[3] = {-1,-1,-1};// Invalid values
  /* L_PTRS = ptrs_arg[0], K_PTRS = ptrs_arg[1] */
  int8_t ptrs_arg[2] = {-1,-1};// Invalid values

  uint16_t ptrsRePerSymb = 0;
  uint16_t pdu_bit_map = 0x0;
  uint16_t dlPtrsSymPos = 0;
  uint16_t ptrsSymbPerSlot = 0;
  uint16_t rbSize = 106;
  uint8_t  mcsIndex = 9;
  uint8_t  dlsch_threads = 0;
  int      prb_inter = 0;
  if ( load_configmodule(argc,argv,CONFIG_ENABLECMDLINEONLY) == 0) {
    exit_fun("[NR_DLSIM] Error, configuration module init failed\n");
  }

  randominit(0);

  int print_perf             = 0;

  FILE *scg_fd=NULL;
  
  while ((c = getopt (argc, argv, "f:hA:pf:g:in:s:S:t:x:y:z:M:N:F:GR:dPIL:Ea:b:d:e:m:w:T:U:qX:")) != -1) {
    switch (c) {
    case 'f':
      scg_fd = fopen(optarg,"r");

      if (scg_fd==NULL) {
        printf("Error opening %s\n",optarg);
        exit(-1);
      }
      break;

    /*case 'd':
      frame_type = 1;
      break;*/

    case 'g':
      switch((char)*optarg) {
      case 'A':
        channel_model=SCM_A;
        break;

      case 'B':
        channel_model=SCM_B;
        break;

      case 'C':
        channel_model=SCM_C;
        break;

      case 'D':
        channel_model=SCM_D;
        break;

      case 'E':
        channel_model=EPA;
        break;

      case 'F':
        channel_model=EVA;
        break;

      case 'G':
        channel_model=ETU;
        break;

      case 'R':
        channel_model=Rayleigh1;
        break;

      default:
        printf("Unsupported channel model!\n");
        exit(-1);
      }

      break;

    case 'i':
      prb_inter=1;
      break;

    case 'n':
      n_trials = atoi(optarg);
      break;

    case 's':
      snr0 = atof(optarg);
      printf("Setting SNR0 to %f\n",snr0);
      break;

    case 'S':
      snr1 = atof(optarg);
      snr1set=1;
      printf("Setting SNR1 to %f\n",snr1);
      break;

      /*
      case 't':
      Td= atof(optarg);
      break;
      */
    /*case 'p':
      extended_prefix_flag=1;
      break;*/

      /*
      case 'r':
      ricean_factor = pow(10,-.1*atof(optarg));
      if (ricean_factor>1) {
        printf("Ricean factor must be between 0 and 1\n");
        exit(-1);
      }
      break;
      */
    case 'x':
      g_nrOfLayers=atoi(optarg);

      if ((g_nrOfLayers!=1) &&
          (g_nrOfLayers!=2)) {
        printf("Unsupported nr Of Layers %d\n",g_nrOfLayers);
        exit(-1);
      }

      break;

    case 'y':
      n_tx=atoi(optarg);

      if ((n_tx==0) || (n_tx>4)) {//extend gNB to support n_tx = 4
        printf("Unsupported number of tx antennas %d\n",n_tx);
        exit(-1);
      }

      break;

    case 'z':
      n_rx=atoi(optarg);

      if ((n_rx==0) || (n_rx>4)) {//extend UE to support n_tx = 4
        printf("Unsupported number of rx antennas %d\n",n_rx);
        exit(-1);
      }

      break;

    case 'R':
      N_RB_DL = atoi(optarg);
      break;

    case 'F':
      input_fd = fopen(optarg,"r");

      if (input_fd==NULL) {
        printf("Problem with filename %s\n",optarg);
        exit(-1);
      }

      break;

    case 'P':
      print_perf=1;
      opp_enabled=1;
      break;
      
    case 'I':
      run_initial_sync=1;
      //target_error_rate=0.1;
      slot = 0;
      break;

    case 'L':
      loglvl = atoi(optarg);
      break;


    case 'E':
	css_flag=1;
	break;


    case 'a':
      g_rbStart = atoi(optarg);
      break;

    case 'b':
      g_rbSize = atoi(optarg);
      break;
    case 'D':
      dlsch_threads = atoi(optarg);
      break;    
    case 'e':
      g_mcsIndex = atoi(optarg);
      break;

    case 'q':
      g_mcsTableIdx = 1;
      get_softmodem_params()->use_256qam_table = 1;
      break;

    case 'm':
      mu = atoi(optarg);
      break;

    case 't':
      eff_tp_check = (float)atoi(optarg)/100;
      break;

    case 'w':
      output_fd = fopen("txdata.dat", "w+");
      break;

    case 'T':
      enable_ptrs=1;
      for(i=0; i < atoi(optarg); i++) {
        ptrs_arg[i] = atoi(argv[optind++]);
      }
      break;

    case 'U':
      modify_dmrs = 1;
      for(i=0; i < atoi(optarg); i++) {
        dmrs_arg[i] = atoi(argv[optind++]);
      }
      break;
      
    case 'X':
      strncpy(gNBthreads, optarg, sizeof(gNBthreads));
      gNBthreads[sizeof(gNBthreads)-1]=0;
      break;
      
    default:
    case 'h':
      printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n",
             argv[0]);
      printf("-h This message\n");
      printf("-L <log level, 0(errors), 1(warning), 2(info) 3(debug) 4 (trace)>\n");  
      //printf("-p Use extended prefix mode\n");
      //printf("-d Use TDD\n");
      printf("-n Number of frames to simulate\n");
      printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
      printf("-S Ending SNR, runs from SNR0 to SNR1\n");
      printf("-t Delay spread for multipath channel\n");
      printf("-g [A,B,C,D,E,F,G,R] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models or R for MIMO model (ignores delay spread and Ricean factor)\n");
      printf("-y Number of TX antennas used in gNB\n");
      printf("-z Number of RX antennas used in UE\n");
      printf("-i Activate PRB based averaging for channel estimation. Frequncy domain interpolation by default.\n");
      //printf("-j Relative strength of second intefering gNB (in dB) - cell_id mod 3 = 2\n");
      printf("-R N_RB_DL\n");
      printf("-O oversampling factor (1,2,4,8,16)\n");
      printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
      //printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
      printf("-f raw file containing RRC configuration (generated by gNB)\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      printf("-E used CSS scheduler\n");
      printf("-o CORESET offset\n");
      printf("-a Start PRB for PDSCH\n");
      printf("-b Number of PRB for PDSCH\n");
      printf("-c Start symbol for PDSCH (fixed for now)\n");
      printf("-j Number of symbols for PDSCH (fixed for now)\n");
      printf("-e MSC index\n");
      printf("-q Use 2nd MCS table (256 QAM table) for PDSCH\n");
      printf("-t Acceptable effective throughput (in percentage)\n");
      printf("-T Enable PTRS, arguments list L_PTRS{0,1,2} K_PTRS{2,4}, e.g. -T 2 0 2 \n");
      printf("-U Change DMRS Config, arguments list DMRS TYPE{0=A,1=B} DMRS AddPos{0:2} DMRS ConfType{1:2}, e.g. -U 3 0 2 1 \n");
      printf("-P Print DLSCH performances\n");
      printf("-w Write txdata to binary file (one frame)\n");
      printf("-d number of dlsch threads, 0: no dlsch parallelization\n");
      printf("-X gNB thread pool configuration, n => no threads");
      exit (-1);
      break;
    }
  }

  logInit();
  set_glog(loglvl);
  T_stdout = 1;
  /* initialize the sin table */
  InitSinLUT();

  get_softmodem_params()->phy_test = 1;
  get_softmodem_params()->do_ra = 0;

  if (snr1set==0)
    snr1 = snr0+10;
  init_dlsch_tpool(dlsch_threads);


  RC.gNB = (PHY_VARS_gNB**) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0] = (PHY_VARS_gNB*) malloc(sizeof(PHY_VARS_gNB ));
  memset(RC.gNB[0],0,sizeof(PHY_VARS_gNB));

  gNB = RC.gNB[0];
  gNB->ofdm_offset_divisor = UINT_MAX;
  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
  frame_parms->nb_antennas_tx = n_tx;
  frame_parms->nb_antennas_rx = n_rx;
  frame_parms->N_RB_DL = N_RB_DL;
  frame_parms->N_RB_UL = N_RB_DL;

  RC.nb_nr_macrlc_inst = 1;
  RC.nb_nr_mac_CC = (int*)malloc(RC.nb_nr_macrlc_inst*sizeof(int));
  for (i = 0; i < RC.nb_nr_macrlc_inst; i++)
    RC.nb_nr_mac_CC[i] = 1;
  mac_top_init_gNB();
  gNB_mac = RC.nrmac[0];
  gNB_RRC_INST rrc;
  memset((void*)&rrc,0,sizeof(rrc));

  /*
  // read in SCGroupConfig
  AssertFatal(scg_fd != NULL,"no reconfig.raw file\n");
  char buffer[1024];
  int msg_len=fread(buffer,1,1024,scg_fd);
  NR_RRCReconfiguration_t *NR_RRCReconfiguration;

  printf("Decoding NR_RRCReconfiguration (%d bytes)\n",msg_len);
  asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
						  &asn_DEF_NR_RRCReconfiguration,
						  (void **)&NR_RRCReconfiguration,
						  (uint8_t *)buffer,
						  msg_len); 
  
  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    AssertFatal(1==0,"NR_RRCReConfiguration decode error\n");
    // free the memory
    SEQUENCE_free( &asn_DEF_NR_RRCReconfiguration, NR_RRCReconfiguration, 1 );
    exit(-1);
  }      
  fclose(scg_fd);

  AssertFatal(NR_RRCReconfiguration->criticalExtensions.present == NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration,"wrong NR_RRCReconfiguration->criticalExstions.present type\n");

  NR_RRCReconfiguration_IEs_t *reconfig_ies = NR_RRCReconfiguration->criticalExtensions.choice.rrcReconfiguration;
  NR_CellGroupConfig_t *secondaryCellGroup;
  dec_rval = uper_decode_complete( NULL,
				   &asn_DEF_NR_CellGroupConfig,
				   (void **)&secondaryCellGroup,
				   (uint8_t *)reconfig_ies->secondaryCellGroup->buf,
				   reconfig_ies->secondaryCellGroup->size); 
  
  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    AssertFatal(1==0,"NR_CellGroupConfig decode error\n");
    // free the memory
    SEQUENCE_free( &asn_DEF_NR_CellGroupConfig, secondaryCellGroup, 1 );
    exit(-1);
  }      
  
  NR_ServingCellConfigCommon_t *scc = secondaryCellGroup->spCellConfig->reconfigurationWithSync->spCellConfigCommon;
  */


  rrc.carrier.servingcellconfigcommon = calloc(1,sizeof(*rrc.carrier.servingcellconfigcommon));

  NR_ServingCellConfigCommon_t *scc = rrc.carrier.servingcellconfigcommon;
  NR_ServingCellConfig_t *scd = calloc(1,sizeof(NR_ServingCellConfig_t));
  NR_CellGroupConfig_t *secondaryCellGroup=calloc(1,sizeof(*secondaryCellGroup));
  prepare_scc(rrc.carrier.servingcellconfigcommon);
  uint64_t ssb_bitmap = 1;
  fill_scc_sim(rrc.carrier.servingcellconfigcommon,&ssb_bitmap,N_RB_DL,N_RB_DL,mu,mu);
  ssb_bitmap = 1;// Enable only first SSB with index ssb_indx=0
  fix_scc(scc,ssb_bitmap);

  prepare_scd(scd);

  fill_default_secondaryCellGroup(scc, scd, secondaryCellGroup, 0, 1, n_tx, 6, 0, 0, 0, 0);

  /* RRC parameter validation for secondaryCellGroup */
  fix_scd(scd);
  /* -U option modify DMRS */
  if(modify_dmrs) {
    update_dmrs_config(secondaryCellGroup, dmrs_arg);
  }
  /* -T option enable PTRS */
  if(enable_ptrs) {
    update_ptrs_config(secondaryCellGroup, &rbSize, &mcsIndex, ptrs_arg);
  }


  //xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void*)secondaryCellGroup);

  AssertFatal((gNB->if_inst         = NR_IF_Module_init(0))!=NULL,"Cannot register interface");
  gNB->if_inst->NR_PHY_config_req      = nr_phy_config_request;
  // common configuration
  rrc_mac_config_req_gNB(0,0, n_tx, n_tx, 0, 6, scc, NULL, NULL, 0, 0, NULL);
  // UE dedicated configuration
  rrc_mac_config_req_gNB(0,0, n_tx, n_tx, 0, 6, scc, NULL, NULL, 1, secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity,secondaryCellGroup);
  // reset preprocessor to the one of DLSIM after it has been set during
  // rrc_mac_config_req_gNB
  gNB_mac->pre_processor_dl = nr_dlsim_preprocessor;
  phy_init_nr_gNB(gNB,0,1);
  N_RB_DL = gNB->frame_parms.N_RB_DL;
  NR_UE_info_t *UE_info = &RC.nrmac[0]->UE_info;
  UE_info->num_UEs=1;

  // stub to configure frame_parms
  //  nr_phy_config_request_sim(gNB,N_RB_DL,N_RB_DL,mu,Nid_cell,SSB_positions);
  // call MAC to configure common parameters

  /* rrc_mac_config_req_gNB() has created one user, so set the scheduling
   * parameters from command line in global variables that will be picked up by
   * scheduling preprocessor */
  if (g_mcsIndex < 0) g_mcsIndex = 9;
  if (g_rbStart < 0) g_rbStart=0;
  if (g_rbSize < 0) g_rbSize = N_RB_DL - g_rbStart;

  double fs,bw;

  if (mu == 1 && N_RB_DL == 217) { 
    fs = 122.88e6;
    bw = 80e6;
  }					       
  else if (mu == 1 && N_RB_DL == 245) {
    fs = 122.88e6;
    bw = 90e6;
  }
  else if (mu == 1 && N_RB_DL == 273) {
    fs = 122.88e6;
    bw = 100e6;
  }
  else if (mu == 1 && N_RB_DL == 106) { 
    fs = 61.44e6;
    bw = 40e6;
  }
  else if (mu == 1 && N_RB_DL == 133) { 
    fs = 61.44e6;
    bw = 50e6;
  }
  else if (mu == 1 && N_RB_DL == 162) { 
    fs = 61.44e6;
    bw = 60e6;
  }
  else if (mu == 3 && N_RB_DL == 66) {
    fs = 122.88e6;
    bw = 100e6;
  }
  else if (mu == 3 && N_RB_DL == 32) {
    fs = 61.44e6;
    bw = 50e6;
  }
  else AssertFatal(1==0,"Unsupported numerology for mu %d, N_RB %d\n",mu, N_RB_DL);

  gNB2UE = new_channel_desc_scm(n_tx,
                                n_rx,
                                channel_model,
                                fs/1e6,//sampling frequency in MHz
				bw,
				30e-9,
                                0,
                                0,
                                0, 0);

  if (gNB2UE==NULL) {
    printf("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  frame_length_complex_samples = frame_parms->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
  //frame_length_complex_samples_no_prefix = frame_parms->samples_per_subframe_wCP*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;

  s_re = malloc(n_tx*sizeof(double*));
  s_im = malloc(n_tx*sizeof(double*));
  r_re = malloc(n_rx*sizeof(double*));
  r_im = malloc(n_rx*sizeof(double*));
  txdata = malloc(n_tx*sizeof(int*));

  for (i=0; i<n_tx; i++) {
    s_re[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(s_re[i],frame_length_complex_samples*sizeof(double));
    s_im[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(s_im[i],frame_length_complex_samples*sizeof(double));

    printf("Allocating %d samples for txdata\n",frame_length_complex_samples);
    txdata[i] = malloc(frame_length_complex_samples*sizeof(int));
    bzero(txdata[i],frame_length_complex_samples*sizeof(int));
  }

  for (i=0; i<n_rx; i++) {
    r_re[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(r_re[i],frame_length_complex_samples*sizeof(double));
    r_im[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(r_im[i],frame_length_complex_samples*sizeof(double));
  }

  if (pbch_file_fd!=NULL) {
    load_pbch_desc(pbch_file_fd);
  }


  //configure UE
  UE = malloc(sizeof(PHY_VARS_NR_UE));
  memset((void*)UE,0,sizeof(PHY_VARS_NR_UE));
  PHY_vars_UE_g = malloc(sizeof(PHY_VARS_NR_UE**));
  PHY_vars_UE_g[0] = malloc(sizeof(PHY_VARS_NR_UE*));
  PHY_vars_UE_g[0][0] = UE;
  memcpy(&UE->frame_parms,frame_parms,sizeof(NR_DL_FRAME_PARMS));
  UE->frame_parms.nb_antennas_rx = n_rx;

  if (run_initial_sync==1)  UE->is_synchronized = 0;
  else                      {UE->is_synchronized = 1; UE->UE_mode[0]=PUSCH;}
                      
  UE->perfect_ce = 0;

  if (init_nr_ue_signal(UE, 1, 0) != 0)
  {
    printf("Error at UE NR initialisation\n");
    exit(-1);
  }

  init_nr_ue_transport(UE,0);

  nr_gold_pbch(UE);
  nr_gold_pdcch(UE,0);

  nr_l2_init_ue(NULL);
  UE_mac = get_mac_inst(0);

  UE->if_inst = nr_ue_if_module_init(0);
  UE->if_inst->scheduled_response = nr_ue_scheduled_response;
  UE->if_inst->phy_config_request = nr_ue_phy_config_request;
  UE->if_inst->dl_indication = nr_ue_dl_indication;
  UE->if_inst->ul_indication = dummy_nr_ue_ul_indication;
  UE->prb_interpolation = prb_inter;


  UE_mac->if_module = nr_ue_if_module_init(0);

  unsigned int available_bits=0;
  unsigned char *estimated_output_bit;
  unsigned char *test_input_bit;
  unsigned int errors_bit    = 0;
  uint32_t errors_scrambling = 0;

  initTpool("N", &(nrUE_params.Tpool), false);

  test_input_bit       = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
  estimated_output_bit = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
  
  // generate signal
  AssertFatal(input_fd==NULL,"Not ready for input signal file\n");
  gNB->pbch_configured = 1;

  //Configure UE
  rrc.carrier.MIB = (uint8_t*) malloc(4);
  rrc.carrier.sizeof_MIB = do_MIB_NR(&rrc,0);

  nr_rrc_mac_config_req_ue(0,0,0,rrc.carrier.mib.message.choice.mib, NULL, NULL, secondaryCellGroup);


  nr_dcireq_t dcireq;
  nr_scheduled_response_t scheduled_response;
  memset((void*)&dcireq,0,sizeof(dcireq));
  memset((void*)&scheduled_response,0,sizeof(scheduled_response));
  dcireq.module_id = 0;
  dcireq.gNB_index = 0;
  dcireq.cc_id     = 0;
  
  scheduled_response.dl_config = &dcireq.dl_config_req;
  scheduled_response.ul_config = &dcireq.ul_config_req;
  scheduled_response.tx_request = NULL;
  scheduled_response.module_id = 0;
  scheduled_response.CC_id     = 0;
  scheduled_response.frame = frame;
  scheduled_response.slot  = slot;
  scheduled_response.thread_id = 0;

  nr_ue_phy_config_request(&UE_mac->phy_config);
  //NR_COMMON_channels_t *cc = RC.nrmac[0]->common_channels;
  snrRun = 0;

  gNB->threadPool = (tpool_t*)malloc(sizeof(tpool_t));
  initTpool(gNBthreads, gNB->threadPool, true);
  gNB->L1_tx_free = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  gNB->L1_tx_filled = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  gNB->L1_tx_out = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  initNotifiedFIFO(gNB->L1_tx_free);
  initNotifiedFIFO(gNB->L1_tx_filled);
  initNotifiedFIFO(gNB->L1_tx_out);
  // we create 2 threads for L1 tx processing
  notifiedFIFO_elt_t *msgL1Tx = newNotifiedFIFO_elt(sizeof(processingData_L1tx_t),0,gNB->L1_tx_free,processSlotTX);
  processingData_L1tx_t *msgDataTx = (processingData_L1tx_t *)NotifiedFifoData(msgL1Tx);
  init_DLSCH_struct(gNB, msgDataTx);
  msgDataTx->slot = slot;
  msgDataTx->frame = frame;
  memset(msgDataTx->ssb, 0, 64*sizeof(NR_gNB_SSB_t));
  reset_meas(&msgDataTx->phy_proc_tx);
  gNB->phy_proc_tx[0] = &msgDataTx->phy_proc_tx;

  for (SNR = snr0; SNR < snr1; SNR += .2) {

    varArray_t *table_tx=initVarArray(1000,sizeof(double));
    reset_meas(&gNB->dlsch_scrambling_stats);
    reset_meas(&gNB->dlsch_interleaving_stats);
    reset_meas(&gNB->dlsch_rate_matching_stats);
    reset_meas(&gNB->dlsch_segmentation_stats);
    reset_meas(&gNB->dlsch_modulation_stats);
    reset_meas(&gNB->dlsch_encoding_stats);
    reset_meas(&gNB->tinput);
    reset_meas(&gNB->tprep);
    reset_meas(&gNB->tparity);
    reset_meas(&gNB->toutput);

    clear_pdsch_stats(gNB);

    n_errors = 0;
    effRate = 0;
    //n_errors2 = 0;
    //n_alamouti = 0;
    errors_scrambling=0;
    n_false_positive = 0;
    if (n_trials== 1) num_rounds = 1;

    for (trial = 0; trial < n_trials; trial++) {

      errors_bit = 0;
      //multipath channel
      //multipath_channel(gNB2UE,s_re,s_im,r_re,r_im,frame_length_complex_samples,0);

      UE->rx_offset=0;
      UE_proc.thread_id  = 0;
      UE_proc.frame_rx   = frame;
      UE_proc.nr_slot_rx = slot;
      
      dcireq.frame     = frame;
      dcireq.slot      = slot;

      NR_UE_DLSCH_t *dlsch0 = UE->dlsch[UE_proc.thread_id][0][0];

      int harq_pid = slot;
      NR_DL_UE_HARQ_t *UE_harq_process = dlsch0->harq_processes[harq_pid];

      NR_gNB_DLSCH_t *gNB_dlsch = msgDataTx->dlsch[0][0];
      nfapi_nr_dl_tti_pdsch_pdu_rel15_t *rel15 = &gNB_dlsch->harq_process.pdsch_pdu.pdsch_pdu_rel15;
      
      UE_harq_process->ack = 0;
      round = 0;
      UE_harq_process->round = round;
      UE_harq_process->first_rx = 1;
        
      while ((round<num_rounds) && (UE_harq_process->ack==0)) {

        clear_nr_nfapi_information(RC.nrmac[0], 0, frame, slot);

        UE_info->UE_sched_ctrl[0].harq_processes[harq_pid].ndi = !(trial&1);


        UE_info->UE_sched_ctrl[0].harq_processes[harq_pid].round = round;
        for (int i=0; i<MAX_NUM_CORESET; i++)
          gNB_mac->pdcch_cand[i] = 0;
      
        if (css_flag == 0) {
          nr_schedule_ue_spec(0, frame, slot);
        } else {
          nr_schedule_css_dlsch_phytest(0,frame,slot);
        }
        Sched_INFO.module_id = 0;
        Sched_INFO.CC_id     = 0;
        Sched_INFO.frame     = frame;
        Sched_INFO.slot      = slot;
        Sched_INFO.DL_req    = &gNB_mac->DL_req[0];
        Sched_INFO.UL_tti_req    = gNB_mac->UL_tti_req_ahead[slot];
        Sched_INFO.UL_dci_req  = NULL;
        Sched_INFO.TX_req    = &gNB_mac->TX_req[0];
        pushNotifiedFIFO(gNB->L1_tx_free,msgL1Tx);
        nr_schedule_response(&Sched_INFO);

        /* PTRS values for DLSIM calculations   */
        nfapi_nr_dl_tti_request_body_t *dl_req = &gNB_mac->DL_req[Sched_INFO.CC_id].dl_tti_request_body;
        nfapi_nr_dl_tti_request_pdu_t  *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[1];
        nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;
        pdu_bit_map = pdsch_pdu_rel15->pduBitmap;
        if(pdu_bit_map & 0x1) {
          set_ptrs_symb_idx(&dlPtrsSymPos,
                            pdsch_pdu_rel15->NrOfSymbols,
                            pdsch_pdu_rel15->StartSymbolIndex,
                            1<<pdsch_pdu_rel15->PTRSTimeDensity,
                            pdsch_pdu_rel15->dlDmrsSymbPos);
          ptrsSymbPerSlot = get_ptrs_symbols_in_slot(dlPtrsSymPos, pdsch_pdu_rel15->StartSymbolIndex, pdsch_pdu_rel15->NrOfSymbols);
          ptrsRePerSymb = ((rel15->rbSize + rel15->PTRSFreqDensity - 1)/rel15->PTRSFreqDensity);
          printf("[DLSIM] PTRS Symbols in a slot: %2u, RE per Symbol: %3u, RE in a slot %4d\n", ptrsSymbPerSlot,ptrsRePerSymb, ptrsSymbPerSlot*ptrsRePerSymb );
        }

        msgDataTx->ssb[0].ssb_pdu.ssb_pdu_rel15.bchPayload=0x001234;
        msgDataTx->ssb[0].ssb_pdu.ssb_pdu_rel15.SsbBlockIndex = 0;
        msgDataTx->gNB = gNB;
        if (run_initial_sync)
          nr_common_signal_procedures(gNB,frame,slot,msgDataTx->ssb[0].ssb_pdu);
        else
          phy_procedures_gNB_TX(msgDataTx,frame,slot,1);
            
        int txdataF_offset = slot * frame_parms->samples_per_slot_wCP;
        
        if (n_trials==1) {
          LOG_M("txsigF0.m","txsF0=", &gNB->common_vars.txdataF[0][txdataF_offset+2*frame_parms->ofdm_symbol_size],frame_parms->ofdm_symbol_size,1,1);
          if (gNB->frame_parms.nb_antennas_tx>1)
            LOG_M("txsigF1.m","txsF1=", &gNB->common_vars.txdataF[1][txdataF_offset+2*frame_parms->ofdm_symbol_size],frame_parms->ofdm_symbol_size,1,1);
        }
        int tx_offset = frame_parms->get_samples_slot_timestamp(slot,frame_parms,0);
        if (n_trials==1) printf("tx_offset %d, txdataF_offset %d \n", tx_offset,txdataF_offset);

        //TODO: loop over slots
        for (aa=0; aa<gNB->frame_parms.nb_antennas_tx; aa++) {
    
          if (cyclic_prefix_type == 1) {
            PHY_ofdm_mod(&gNB->common_vars.txdataF[aa][txdataF_offset],
                         &txdata[aa][tx_offset],
                         frame_parms->ofdm_symbol_size,
                         12,
                         frame_parms->nb_prefix_samples,
                         CYCLIC_PREFIX);
          } else {
            nr_normal_prefix_mod(&gNB->common_vars.txdataF[aa][txdataF_offset],
                                 &txdata[aa][tx_offset],
                                 14,
                                 frame_parms,
                                 slot);
          }
        }
       
        if (n_trials==1) {
          char filename[100];//LOG_M
          for (aa=0;aa<n_tx;aa++) {
            sprintf(filename,"txsig%d.m", aa);//LOG_M
            LOG_M(filename,"txs", &txdata[aa][tx_offset+frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples0],6*(frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples),1,1);
          }
        }
        if (output_fd) {
          printf("writing txdata to binary file\n");
          fwrite(txdata[0],sizeof(int32_t),frame_length_complex_samples,output_fd);
        }

        int txlev[n_tx];
        int txlev_sum = 0;
        int l_ofdm = 6;
        for (aa=0; aa<n_tx; aa++) {
          txlev[aa] = signal_energy(&txdata[aa][tx_offset+l_ofdm*frame_parms->ofdm_symbol_size + (l_ofdm-1)*frame_parms->nb_prefix_samples + frame_parms->nb_prefix_samples0],
          frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples);
          txlev_sum += txlev[aa];
          if (n_trials==1) printf("txlev[%d] = %d (%f dB) txlev_sum %d\n",aa,txlev[aa],10*log10((double)txlev[aa]),txlev_sum);
        }
        
        for (i=(frame_parms->get_samples_slot_timestamp(slot,frame_parms,0)); 
             i<(frame_parms->get_samples_slot_timestamp(slot+1,frame_parms,0)); 
             i++) {
    
          for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
            s_re[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)]);
            s_im[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)+1]);
          }
        }

        double ts = 1.0/(frame_parms->subcarrier_spacing * frame_parms->ofdm_symbol_size); 
        //Compute AWGN variance
        sigma2_dB = 10 * log10((double)txlev_sum * ((double)UE->frame_parms.ofdm_symbol_size/(12*rel15->rbSize))) - SNR;
        sigma2    = pow(10, sigma2_dB/10);
        if (n_trials==1) printf("sigma2 %f (%f dB), txlev_sum %f (factor %f)\n",sigma2,sigma2_dB,10*log10((double)txlev_sum),(double)(double)UE->frame_parms.ofdm_symbol_size/(12*rel15->rbSize));

        for (aa=0; aa<n_rx; aa++) {
          bzero(r_re[aa],frame_length_complex_samples*sizeof(double));
          bzero(r_im[aa],frame_length_complex_samples*sizeof(double));
        }
        
        // Apply MIMO Channel
        if (channel_model != AWGN) multipath_tv_channel(gNB2UE,
                             s_re,
                             s_im,
                             r_re,
                             r_im,
                             frame_length_complex_samples,
                             0);

        double H_awgn_mimo[4][4] ={{1.0, 0.5, 0.25, 0.125},//rx 0
                                   {0.5, 1.0, 0.5, 0.25},  //rx 1
                                   {0.25, 0.5, 1.0, 0.5},  //rx 2
                                   {0.125, 0.25, 0.5, 1.0}};//rx 3

        for (i=frame_parms->get_samples_slot_timestamp(slot,frame_parms,0); 
             i<frame_parms->get_samples_slot_timestamp(slot+1,frame_parms,0);
             i++) {

          for (int aa_rx=0; aa_rx<n_rx; aa_rx++) {

            if (channel_model == AWGN) {
              // sum up signals from different Tx antennas
              r_re[aa_rx][i] = 0;
              r_im[aa_rx][i] = 0;
             for (aa=0; aa<n_tx; aa++) {
                r_re[aa_rx][i] += s_re[aa][i]*H_awgn_mimo[aa_rx][aa];
                r_im[aa_rx][i] += s_im[aa][i]*H_awgn_mimo[aa_rx][aa];
              }
            }
            // Add Gaussian noise
            ((short*) UE->common_vars.rxdata[aa_rx])[2*i]   = (short) ((r_re[aa_rx][i] + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
            ((short*) UE->common_vars.rxdata[aa_rx])[2*i+1] = (short) ((r_im[aa_rx][i] + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
            /* Add phase noise if enabled */
            if (pdu_bit_map & 0x1) {
              phase_noise(ts, &((short*) UE->common_vars.rxdata[aa_rx])[2*i],
                          &((short*) UE->common_vars.rxdata[aa_rx])[2*i+1]);
            }
          }
        }

        nr_ue_dcireq(&dcireq); //to be replaced with function pointer later
        nr_ue_scheduled_response(&scheduled_response);
        
        phy_procedures_nrUE_RX(UE,
                               &UE_proc,
                               0,
                               dlsch_threads,
                               NULL);
        
        //printf("dlsim round %d ends\n",round);
        round++;
      } // round

      //----------------------------------------------------------
      //---------------------- count errors ----------------------
      //----------------------------------------------------------

      if (UE->dlsch[UE_proc.thread_id][0][0]->last_iteration_cnt >=
        UE->dlsch[UE_proc.thread_id][0][0]->max_ldpc_iterations+1)
        n_errors++;

      //NR_UE_PDSCH **pdsch_vars = UE->pdsch_vars[UE_proc.thread_id];
      //int16_t *UE_llr = pdsch_vars[0]->llr[0];

      TBS                  = UE_harq_process->TBS;//rel15->TBSize[0];
      uint16_t length_dmrs = get_num_dmrs(rel15->dlDmrsSymbPos);
      uint16_t nb_rb       = rel15->rbSize;
      uint8_t  nb_re_dmrs  = rel15->dmrsConfigType == NFAPI_NR_DMRS_TYPE1 ? 6 : 4;
      uint8_t  mod_order   = rel15->qamModOrder[0];
      uint8_t  nb_symb_sch = rel15->NrOfSymbols;

      available_bits = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, rel15->nrOfLayers);
      if(pdu_bit_map & 0x1) {
        available_bits-= (ptrsSymbPerSlot * ptrsRePerSymb *rel15->nrOfLayers* 2);
        printf("[DLSIM][PTRS] Available bits are: %5u, removed PTRS bits are: %5u \n",available_bits, (ptrsSymbPerSlot * ptrsRePerSymb *rel15->nrOfLayers* 2) );
      }

      /*
      for (i = 0; i < available_bits; i++) {

	if(((gNB_dlsch->harq_process.f[i] == 0) && (UE_llr[i] <= 0)) ||
	   ((gNB_dlsch->harq_process.f[i] == 1) && (UE_llr[i] >= 0)))
	  {
	    if(errors_scrambling == 0) {
	      LOG_D(PHY,"\n");
	      LOG_D(PHY,"First bit in error in unscrambling = %d\n",i);
	    }
	    errors_scrambling++;
	  }

      }
      */
      for (i = 0; i < TBS; i++) {

	estimated_output_bit[i] = (UE_harq_process->b[i/8] & (1 << (i & 7))) >> (i & 7);
	test_input_bit[i]       = (gNB_dlsch->harq_process.b[i / 8] & (1 << (i & 7))) >> (i & 7); // Further correct for multiple segments
	
	if (estimated_output_bit[i] != test_input_bit[i]) {
	  if(errors_bit == 0)
	    LOG_D(PHY,"First bit in error in decoding = %d (errors scrambling %d)\n",i,errors_scrambling);
	  errors_bit++;
	}
	
      }
      
      ////////////////////////////////////////////////////////////
      
      if (errors_scrambling > 0) {
	if (n_trials == 1)
	  printf("errors_scrambling = %u/%u (trial %d)\n", errors_scrambling, available_bits,trial);
      }
      
      if (errors_bit > 0) {
	n_false_positive++;
	if (n_trials == 1)
	  printf("errors_bit = %u (trial %d)\n", errors_bit, trial);
      }
      roundStats[snrRun]+=((float)round); 
      if (UE_harq_process->ack==1) effRate += ((float)TBS)/round;
    } // noise trials

    blerStats[snrRun] = (float) n_errors / (float) n_trials;
    roundStats[snrRun]/=((float)n_trials);
    berStats[snrRun] = (double)errors_scrambling/available_bits/n_trials;
    effRate /= n_trials;
    printf("*****************************************\n");
    printf("SNR %f, (false positive %f)\n", SNR,
           (float) n_errors / (float) n_trials);
    printf("*****************************************\n");
    printf("\n");
    dump_pdsch_stats(stdout,gNB);
    printf("SNR %f : n_errors (negative CRC) = %d/%d, Avg round %.2f, Channel BER %e, BLER %.2f, Eff Rate %.4f bits/slot, Eff Throughput %.2f, TBS %u bits/slot\n", SNR, n_errors, n_trials,roundStats[snrRun],berStats[snrRun],blerStats[snrRun],effRate,effRate/TBS*100,TBS);
    printf("\n");

    if (print_perf==1) {
      printf("\ngNB TX function statistics (per %d us slot, NPRB %d, mcs %d, block %d)\n",
	     1000>>*scc->ssbSubcarrierSpacing, g_rbSize, g_mcsIndex,
	     msgDataTx->dlsch[0][0]->harq_process.pdsch_pdu.pdsch_pdu_rel15.TBSize[0]<<3);
      printDistribution(gNB->phy_proc_tx[0],table_tx,"PHY proc tx");
      printStatIndent2(&gNB->dlsch_encoding_stats,"DLSCH encoding time");
      printStatIndent3(&gNB->dlsch_segmentation_stats,"DLSCH segmentation time");
      printStatIndent3(&gNB->tinput,"DLSCH LDPC input processing time");
      printStatIndent3(&gNB->tprep,"DLSCH LDPC input preparation time");
      printStatIndent3(&gNB->tparity,"DLSCH LDPC parity generation time");
      printStatIndent3(&gNB->toutput,"DLSCH LDPC output generation time");
      printStatIndent3(&gNB->dlsch_rate_matching_stats,"DLSCH Rate Mataching time");
      printStatIndent3(&gNB->dlsch_interleaving_stats,  "DLSCH Interleaving time");
      printStatIndent2(&gNB->dlsch_modulation_stats,"DLSCH modulation time");
      printStatIndent2(&gNB->dlsch_scrambling_stats,  "DLSCH scrambling time");
      printStatIndent2(&gNB->dlsch_layer_mapping_stats, "DLSCH Layer Mapping time");
      printStatIndent2(&gNB->dlsch_resource_mapping_stats, "DLSCH Resource Mapping time");
      printStatIndent2(&gNB->dlsch_precoding_stats,"DLSCH Layer Precoding time");


      printf("\nUE RX function statistics (per %d us slot)\n",1000>>*scc->ssbSubcarrierSpacing);
      /*
      printDistribution(&phy_proc_rx_tot, table_rx,"Total PHY proc rx");
      printStatIndent(&ue_front_end_tot,"Front end processing");
      printStatIndent(&dlsch_llr_tot,"rx_pdsch processing");
      printStatIndent2(&pdsch_procedures_tot,"pdsch processing");
      printStatIndent2(&dlsch_procedures_tot,"dlsch processing");
      printStatIndent2(&UE->crnti_procedures_stats,"C-RNTI processing");
      printStatIndent(&UE->ofdm_demod_stats,"ofdm demodulation");
      printStatIndent(&UE->dlsch_channel_estimation_stats,"DLSCH channel estimation time");
      printStatIndent(&UE->dlsch_freq_offset_estimation_stats,"DLSCH frequency offset estimation time");
      printStatIndent(&dlsch_decoding_tot, "DLSCH Decoding time ");
      printStatIndent(&UE->dlsch_unscrambling_stats,"DLSCH unscrambling time");
      printStatIndent(&UE->dlsch_rate_unmatching_stats,"DLSCH Rate Unmatching");
      printf("|__ DLSCH Turbo Decoding(%d bits), avg iterations: %.1f       %.2f us (%d cycles, %d trials)\n",
	     UE->dlsch[UE_proc.thread_id][0][0]->harq_processes[0]->Cminus ?
	     UE->dlsch[UE_proc.thread_id][0][0]->harq_processes[0]->Kminus :
	     UE->dlsch[UE_proc.thread_id][0][0]->harq_processes[0]->Kplus,
	     UE->dlsch_tc_intl1_stats.trials/(double)UE->dlsch_tc_init_stats.trials,
	     (double)UE->dlsch_turbo_decoding_stats.diff/UE->dlsch_turbo_decoding_stats.trials*timeBase,
	     (int)((double)UE->dlsch_turbo_decoding_stats.diff/UE->dlsch_turbo_decoding_stats.trials),
	     UE->dlsch_turbo_decoding_stats.trials);
      printStatIndent2(&UE->dlsch_tc_init_stats,"init");
      printStatIndent2(&UE->dlsch_tc_alpha_stats,"alpha");
      printStatIndent2(&UE->dlsch_tc_beta_stats,"beta");
      printStatIndent2(&UE->dlsch_tc_gamma_stats,"gamma");
      printStatIndent2(&UE->dlsch_tc_ext_stats,"ext");
      printStatIndent2(&UE->dlsch_tc_intl1_stats,"turbo internal interleaver");
      printStatIndent2(&UE->dlsch_tc_intl2_stats,"intl2+HardDecode+CRC");
      */
    }

    if (n_trials == 1) {
      
      LOG_M("rxsig0.m","rxs0", UE->common_vars.rxdata[0], frame_length_complex_samples, 1, 1);
      if (UE->frame_parms.nb_antennas_rx>1)
	LOG_M("rxsig1.m","rxs1", UE->common_vars.rxdata[1], frame_length_complex_samples, 1, 1);
      LOG_M("chestF0.m","chF0",&UE->pdsch_vars[0][0]->dl_ch_estimates_ext[0][0],g_rbSize*12*14,1,1);
      write_output("rxF_comp.m","rxFc",&UE->pdsch_vars[0][0]->rxdataF_comp0[0][0],N_RB_DL*12*14,1,1);
      LOG_M("rxF_llr.m","rxFllr",UE->pdsch_vars[UE_proc.thread_id][0]->llr[0],available_bits,1,0);
      LOG_M("pdcch_rxFcomp.m","pdcch_rxFcomp",&UE->pdcch_vars[0][0]->rxdataF_comp[0][0],96*12,1,1);
      LOG_M("pdcch_rxFllr.m","pdcch_rxFllr",UE->pdcch_vars[0][0]->llr,96*12,1,1);
      break;
    }

    if (effRate > (eff_tp_check*TBS)) {
      printf("PDSCH test OK\n");
      break;
    }

    snrStats[snrRun] = SNR;
    snrRun++;
  } // NSR

  LOG_M("dlsimStats.m","SNR",snrStats,snrRun,1,7);
  LOG_MM("dlsimStats.m","BLER",blerStats,snrRun,1,7);
  LOG_MM("dlsimStats.m","BER",berStats,snrRun,1,7);
  LOG_MM("dlsimStats.m","rounds",roundStats,snrRun,1,7);
  /*if (n_trials>1) {
    printf("HARQ stats:\nSNR\tRounds\n");
    psnr = snr0;
    for (uint8_t i=0; i<snrRun; i++) {
      printf("%.1f\t%.2f\n",psnr,roundStats[i]);
      psnr+=0.2;
    }
  }*/

  free_channel_desc_scm(gNB2UE);

  for (i = 0; i < n_tx; i++) {
    free(s_re[i]);
    free(s_im[i]);
    free(txdata[i]);
  }
  for (i = 0; i < n_rx; i++) {
    free(r_re[i]);
    free(r_im[i]);
  }

  free(s_re);
  free(s_im);
  free(r_re);
  free(r_im);
  free(txdata);
  free(test_input_bit);
  free(estimated_output_bit);
  
  if (output_fd)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  if (scg_fd)
    fclose(scg_fd);
  return(n_errors);
  
}


void update_ptrs_config(NR_CellGroupConfig_t *secondaryCellGroup, uint16_t *rbSize, uint8_t *mcsIndex, int8_t *ptrs_arg)
{
  NR_BWP_Downlink_t *bwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[0];
  int *ptrsFreqDenst = calloc(2, sizeof(long));
  ptrsFreqDenst[0]= 25;
  ptrsFreqDenst[1]= 115;
  int *ptrsTimeDenst = calloc(3, sizeof(long));
  ptrsTimeDenst[0]= 2;
  ptrsTimeDenst[1]= 4;
  ptrsTimeDenst[2]= 10;

  int epre_Ratio = 0;
  int reOffset = 0;

  if(ptrs_arg[0] ==0) {
    ptrsTimeDenst[2]= *mcsIndex -1;
  }
  else if(ptrs_arg[0] == 1) {
    ptrsTimeDenst[1]= *mcsIndex - 1;
    ptrsTimeDenst[2]= *mcsIndex + 1;
  }
  else if(ptrs_arg[0] ==2) {
    ptrsTimeDenst[0]= *mcsIndex - 1;
    ptrsTimeDenst[1]= *mcsIndex + 1;
  }
  else {
    printf("[DLSIM] Wrong L_PTRS value, using default values 1\n");
  }
  /* L = 4 if Imcs < MCS4 */
  if(ptrs_arg[1] ==2) {
    ptrsFreqDenst[0]= *rbSize - 1;
    ptrsFreqDenst[1]= *rbSize + 1;
  }
  else if(ptrs_arg[1] == 4) {
    ptrsFreqDenst[1]= *rbSize - 1;
  }
  else {
    printf("[DLSIM] Wrong K_PTRS value, using default values 2\n");
  }
  printf("[DLSIM] PTRS Enabled with L %d, K %d \n", 1<<ptrs_arg[0], ptrs_arg[1] );
  /* overwrite the values */
  rrc_config_dl_ptrs_params(bwp, ptrsFreqDenst, ptrsTimeDenst, &epre_Ratio, &reOffset);
}

void update_dmrs_config(NR_CellGroupConfig_t *scg, int8_t* dmrs_arg)
{
  int8_t  mapping_type = typeA;//default value
  int8_t  add_pos = pdsch_dmrs_pos0;//default value
  int8_t  dmrs_config_type = NFAPI_NR_DMRS_TYPE1;//default value

  if(dmrs_arg[0] == 0) {
    mapping_type = typeA;
  }
  else if (dmrs_arg[0] == 1) {
    mapping_type = typeB;
  } else {
    AssertFatal(1==0,"Incorrect Mappingtype, valid options 0-typeA, 1-typeB\n");
  }

  /* Additional DMRS positions 0 ,1 ,2 and 3 */
  if(dmrs_arg[1] >= 0 && dmrs_arg[1] <4 ) {
    add_pos = dmrs_arg[1];
  } else {
    AssertFatal(1==0,"Incorrect Additional Position, valid options 0-pos1, 1-pos1, 2-pos2, 3-pos3\n");
  }

  /* DMRS Conf Type 1 or 2 */
  if(dmrs_arg[2] == 1) {
    dmrs_config_type = NFAPI_NR_DMRS_TYPE1;
  } else if(dmrs_arg[2] == 2) {
    dmrs_config_type = NFAPI_NR_DMRS_TYPE2;
  }

  NR_BWP_Downlink_t *bwp = scg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[0];

  AssertFatal((bwp->bwp_Dedicated->pdsch_Config != NULL && bwp->bwp_Dedicated->pdsch_Config->choice.setup != NULL), "Base RRC reconfig structures are not allocated.\n");

  if(mapping_type == typeA) {
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA = calloc(1,sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA));
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->present= NR_SetupRelease_DMRS_DownlinkConfig_PR_setup;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup = calloc(1,sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup));
    if (dmrs_config_type == NFAPI_NR_DMRS_TYPE2)
      bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type = calloc(1,sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type));
    else
      bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type = NULL;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->maxLength=NULL;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->scramblingID0=NULL;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->scramblingID1=NULL;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS=NULL;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = NULL;
    printf("DLSIM: Allocated Mapping TypeA in RRC reconfig message\n");
  }

  if(mapping_type == typeB) {
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB = calloc(1,sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB));
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->present= NR_SetupRelease_DMRS_DownlinkConfig_PR_setup;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup = calloc(1,sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup));
    if (dmrs_config_type == NFAPI_NR_DMRS_TYPE2)
      bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->dmrs_Type = calloc(1,sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->dmrs_Type));
    else
      bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->dmrs_Type = NULL;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->maxLength=NULL;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->scramblingID0=NULL;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->scramblingID1=NULL;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->phaseTrackingRS=NULL;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->dmrs_AdditionalPosition = NULL;
    printf("DLSIM: Allocated Mapping TypeB in RRC reconfig message\n");
  }

  struct NR_SetupRelease_DMRS_DownlinkConfig	*dmrs_MappingtypeA = bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA;
  struct NR_SetupRelease_DMRS_DownlinkConfig	*dmrs_MappingtypeB = bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeB;


  NR_DMRS_DownlinkConfig_t *dmrs_config = (mapping_type == typeA) ? dmrs_MappingtypeA->choice.setup : dmrs_MappingtypeB->choice.setup;

  if (add_pos != 2) { // pos0,pos1,pos3
    if (dmrs_config->dmrs_AdditionalPosition == NULL) {
      dmrs_config->dmrs_AdditionalPosition = calloc(1,sizeof(*dmrs_MappingtypeA->choice.setup->dmrs_AdditionalPosition));
    }
    *dmrs_config->dmrs_AdditionalPosition = add_pos;
  } else { // if NULL, Value pos2
    free(dmrs_config->dmrs_AdditionalPosition);
    dmrs_config->dmrs_AdditionalPosition = NULL;
  }

  for (int i=0;i<bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count;i++) {
    bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->mappingType = mapping_type;
  }

  printf("[DLSIM] DMRS Config is modified with Mapping Type %d, Additional Positions %d Config. Type %d \n", mapping_type, add_pos, dmrs_config_type);
}
