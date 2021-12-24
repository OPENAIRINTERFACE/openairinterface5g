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

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "common/ran_context.h"
#include "common/config/config_userapi.h"
#include "common/utils/LOG/log.h"
#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/phy_vars_nr_ue.h"
#include "PHY/types.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_sch_dmrs.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/TOOLS/tools_defs.h"
#include "SCHED_NR/fapi_nr_l1.h"
#include "SCHED_NR/sched_nr.h"
#include "SCHED_NR_UE/defs.h"
#include "SCHED_NR_UE/fapi_nr_ue_l1.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
#include "openair1/SIMULATION/RF/rf.h"
#include "openair1/SIMULATION/NR_PHY/nr_unitary_defs.h"
#include "openair2/RRC/NR/MESSAGES/asn1_msg.h"
//#include "openair1/SIMULATION/NR_PHY/nr_dummy_functions.c"
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "common/utils/threadPool/thread-pool.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#define inMicroS(a) (((double)(a))/(get_cpu_freq_GHz()*1000.0))
#include "SIMULATION/LTE_PHY/common_sim.h"

#include <openair2/LAYER2/MAC/mac_vars.h>
#include <openair2/RRC/LTE/rrc_vars.h>

#include <executables/softmodem-common.h>
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"
//#define DEBUG_ULSIM

LCHAN_DESC DCCH_LCHAN_DESC,DTCH_DL_LCHAN_DESC,DTCH_UL_LCHAN_DESC;
rlc_info_t Rlc_info_um,Rlc_info_am_config;

PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];

uint16_t sf_ahead=4 ;
int slot_ahead=6 ;
uint16_t sl_ahead=0;
double cpuf;
//uint8_t nfapi_mode = 0;
uint64_t downlink_frequency[MAX_NUM_CCs][4];
THREAD_STRUCT thread_struct;
nfapi_ue_release_request_body_t release_rntis;
uint32_t N_RB_DL = 106;

extern void fix_scd(NR_ServingCellConfig_t *scd);// forward declaration

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

int generate_dlsch_header(unsigned char *mac_header,
                          unsigned char num_sdus,
                          unsigned short *sdu_lengths,
                          unsigned char *sdu_lcids,
                          unsigned char drx_cmd,
                          unsigned short timing_advance_cmd,
                          unsigned char *ue_cont_res_id,
                          unsigned char short_padding,
                          unsigned short post_padding){return 0;}

void
rrc_data_ind(
  const protocol_ctxt_t *const ctxt_pP,
  const rb_id_t                Srb_id,
  const sdu_size_t             sdu_sizeP,
  const uint8_t   *const       buffer_pP
)
{
}

int ocp_gtpv1u_create_s1u_tunnel(instance_t instance,
                                 const gtpv1u_enb_create_tunnel_req_t  *create_tunnel_req,
                                 gtpv1u_enb_create_tunnel_resp_t *create_tunnel_resp) {
    return 0;
}

int
gtpv1u_create_s1u_tunnel(
  const instance_t                              instanceP,
  const gtpv1u_enb_create_tunnel_req_t *const  create_tunnel_req_pP,
  gtpv1u_enb_create_tunnel_resp_t *const create_tunnel_resp_pP
) {
  return 0;
}

int ocp_gtpv1u_delete_s1u_tunnel(const instance_t instance, const gtpv1u_enb_delete_tunnel_req_t *const req_pP) {
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

int
nr_rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  const gtpv1u_gnb_create_tunnel_resp_t *const create_tunnel_resp_pP,
  uint8_t                         *inde_list
){
  return 0;
}

// Dummy function to avoid linking error at compilation of nr-ulsim
int is_x2ap_enabled(void)
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

//nFAPI P7 dummy functions

int oai_nfapi_dl_tti_req(nfapi_nr_dl_tti_request_t *dl_config_req) { return(0);  }
int oai_nfapi_tx_data_req(nfapi_nr_tx_data_request_t *tx_data_req){ return(0);  }
int oai_nfapi_ul_dci_req(nfapi_nr_ul_dci_request_t *ul_dci_req){ return(0);  }
int oai_nfapi_ul_tti_req(nfapi_nr_ul_tti_request_t *ul_tti_req){ return(0);  }
int oai_nfapi_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind) { return(0);  }
int oai_nfapi_nr_crc_indication(nfapi_nr_crc_indication_t *ind) { return(0);  }
int oai_nfapi_nr_srs_indication(nfapi_nr_srs_indication_t *ind) { return(0);  }
int oai_nfapi_nr_uci_indication(nfapi_nr_uci_indication_t *ind) { return(0);  }
int oai_nfapi_nr_rach_indication(nfapi_nr_rach_indication_t *ind) { return(0);  }

int nr_derive_key(int alg_type, uint8_t alg_id,
               const uint8_t key[32], uint8_t **out)
{
  return 0;
}

typedef struct {
  uint64_t       optmask;   //mask to store boolean config options
  uint8_t        nr_dlsch_parallel; // number of threads for dlsch decoding, 0 means no parallelization
  tpool_t        Tpool;             // thread pool 
} nrUE_params_t;

void processSlotTX(void *arg) {}

nrUE_params_t nrUE_params;

nrUE_params_t *get_nrUE_params(void) {
  return &nrUE_params;
}
// needed for some functions
uint16_t n_rnti = 0x1234;
openair0_config_t openair0_cfg[MAX_CARDS];
//const uint8_t nr_rv_round_map[4] = {0, 2, 1, 3}; 

channel_desc_t *UE2gNB[NUMBER_OF_UE_MAX][NUMBER_OF_gNB_MAX];

int main(int argc, char **argv)
{
  char c;
  int i;
  double SNR, snr0 = -2.0, snr1 = 2.0;
  double sigma, sigma_dB;
  double snr_step = .2;
  uint8_t snr1set = 0;
  int slot = 8, frame = 1;
  FILE *output_fd = NULL;
  double **s_re,**s_im,**r_re,**r_im;
  //uint8_t write_output_file = 0;
  int trial, n_trials = 1, n_false_positive = 0, delay = 0;
  double maxDoppler = 0.0;
  uint8_t n_tx = 1, n_rx = 1;
  //uint8_t transmission_mode = 1;
  //uint16_t Nid_cell = 0;
  channel_desc_t *UE2gNB;
  uint8_t extended_prefix_flag = 0;
  //int8_t interf1 = -21, interf2 = -21;
  FILE *input_fd = NULL;
  SCM_t channel_model = AWGN;  //Rayleigh1_anticorr;
  uint16_t N_RB_DL = 106, N_RB_UL = 106, mu = 1;

  NB_UE_INST = 1;

  //unsigned char frame_type = 0;
  NR_DL_FRAME_PARMS *frame_parms;
  int loglvl = OAILOG_INFO;
  //uint64_t SSB_positions=0x01;
  uint16_t nb_symb_sch = 12;
  int start_symbol = 0;
  uint16_t nb_rb = 50;
  int Imcs = 9;
  uint8_t precod_nbr_layers = 1;
  int gNB_id = 0;
  int ap;
  int tx_offset;
  int32_t txlev=0;
  int start_rb = 0;
  int UE_id =0; // [hna] only works for UE_id = 0 because NUMBER_OF_NR_UE_MAX is set to 1 (phy_init_nr_gNB causes segmentation fault)
  float target_error_rate = 0.01;
  int print_perf = 0;
  cpuf = get_cpu_freq_GHz();
  int msg3_flag = 0;
  int rv_index = 0;
  float roundStats[100];
  double effRate[100]; 
  double effTP[100]; 
  //float eff_tp_check = 0.7;
  uint8_t snrRun;
  int prb_inter = 0;

  int enable_ptrs = 0;
  int modify_dmrs = 0;
  /* L_PTRS = ptrs_arg[0], K_PTRS = ptrs_arg[1] */
  int ptrs_arg[2] = {-1,-1};// Invalid values
  /* DMRS TYPE = dmrs_arg[0], Add Pos = dmrs_arg[1] */
  int dmrs_arg[2] = {-1,-1};// Invalid values
  uint16_t ptrsSymPos = 0;
  uint16_t ptrsSymbPerSlot = 0;
  uint16_t ptrsRePerSymb = 0;

  uint8_t transform_precoding = 1; // 0 - ENABLE, 1 - DISABLE
  uint8_t num_dmrs_cdm_grps_no_data = 1;
  uint8_t mcs_table = 0;

  UE_nr_rxtx_proc_t UE_proc;
  FILE *scg_fd=NULL;
  int file_offset = 0;

  double DS_TDL = .03;
  int ibwps=24;
  int ibwp_rboffset=41;
  int params_from_file = 0;
  int threadCnt=1;
  if ( load_configmodule(argc,argv,CONFIG_ENABLECMDLINEONLY) == 0 ) {
    exit_fun("[NR_ULSIM] Error, configuration module init failed\n");
  }
  int ul_proc_error = 0; // uplink processing checking status flag
  //logInit();
  randominit(0);

  /* initialize the sin-cos table */
   InitSinLUT();

  while ((c = getopt(argc, argv, "a:b:c:d:ef:g:h:ikl:m:n:p:r:s:t:u:w:y:z:F:G:H:M:N:PR:S:T:U:L:Z")) != -1) {
    printf("handling optarg %c\n",c);
    switch (c) {

      /*case 'd':
        frame_type = 1;
        break;*/

    case 'a':
      start_symbol = atoi(optarg);
      AssertFatal(start_symbol >= 0 && start_symbol < 13,"start_symbol %d is not in 0..12\n",start_symbol);
      break;

    case 'b':
      nb_symb_sch = atoi(optarg);
      AssertFatal(nb_symb_sch > 0 && nb_symb_sch < 15,"start_symbol %d is not in 1..14\n",nb_symb_sch);
      break;
    case 'c':
      n_rnti = atoi(optarg);
      AssertFatal(n_rnti > 0 && n_rnti<=65535,"Illegal n_rnti %x\n",n_rnti);
      break;

    case 'd':
      delay = atoi(optarg);
      break;
      
    case 'e':
      msg3_flag = 1;
      break;
      
    case 'f':
      scg_fd = fopen(optarg, "r");
      
      if (scg_fd == NULL) {
	printf("Error opening %s\n", optarg);
	exit(-1);
      }

      break;
      
    case 'g':
      switch ((char) *optarg) {
      case 'A':
	channel_model = SCM_A;
	break;
	
      case 'B':
	channel_model = SCM_B;
	break;
	
      case 'C':
	channel_model = SCM_C;
	break;
	
      case 'D':
	channel_model = SCM_D;
	break;
	
      case 'E':
	channel_model = EPA;
	break;
	
      case 'F':
	channel_model = EVA;
	break;
	
      case 'G':
	channel_model = ETU;
	break;

      case 'H':
        channel_model = TDL_C;
	DS_TDL = .030; // 30 ns
	break;
  
      case 'I':
	channel_model = TDL_C;
	DS_TDL = .3;  // 300ns
        break;
     
      case 'J':
	channel_model=TDL_D;
	DS_TDL = .03;
	break;

      default:
	printf("Unsupported channel model!\n");
	exit(-1);
      }
      
      break;
      
    case 'i':
      prb_inter=1;
      break;
	
    case 'k':
      printf("Setting threequarter_fs_flag\n");
      openair0_cfg[0].threequarter_fs= 1;
      break;

    case 'l':
      nb_symb_sch = atoi(optarg);
      break;
      
    case 'm':
      Imcs = atoi(optarg);
      break;
      
    case 'n':
      n_trials = atoi(optarg);
      break;
      
    case 'p':
      extended_prefix_flag = 1;
      break;
      
    case 'r':
      nb_rb = atoi(optarg);
      break;
      
    case 's':
      snr0 = atof(optarg);
      printf("Setting SNR0 to %f\n", snr0);
      break;
    case 't':
      threadCnt = atoi(optarg);
      break;
    case 'u':
      mu = atoi(optarg);
      break;

    case 'w':
      start_rb = atoi(optarg);
      break;

/*
    case 't':
      eff_tp_check = (float)atoi(optarg)/100;
      break;
*/
      /*
	case 'r':
	ricean_factor = pow(10,-.1*atof(optarg));
	if (ricean_factor>1) {
	printf("Ricean factor must be between 0 and 1\n");
	exit(-1);
	}
	break;
      */
      
      /*case 'x':
        transmission_mode = atoi(optarg);
        break;*/
      
    case 'y':
      n_tx = atoi(optarg);
      
      if ((n_tx == 0) || (n_tx > 2)) {
	printf("Unsupported number of tx antennas %d\n", n_tx);
	exit(-1);
      }
      
      break;
      
    case 'z':
      n_rx = atoi(optarg);
      
      if ((n_rx == 0) || (n_rx > 8)) {
	printf("Unsupported number of rx antennas %d\n", n_rx);
	exit(-1);
      }
      
      break;
      
    case 'F':
      input_fd = fopen(optarg, "r");
      
      if (input_fd == NULL) {
	printf("Problem with filename %s\n", optarg);
	exit(-1);
      }
      
      break;

    case 'G':
      file_offset = atoi(optarg);
      break;

    case 'H':
      slot = atoi(optarg);
      break;

    case 'M':
     // SSB_positions = atoi(optarg);
      break;
      
    case 'N':
     // Nid_cell = atoi(optarg);
      break;
      
    case 'R':
      N_RB_DL = atoi(optarg);
      N_RB_UL = N_RB_DL;
      break;
      
    case 'S':
      snr1 = atof(optarg);
      snr1set = 1;
      printf("Setting SNR1 to %f\n", snr1);
      break;
      
    case 'P':
      print_perf=1;
      opp_enabled=1;
      break;
      
    case 'L':
      loglvl = atoi(optarg);
      break;

   case 'T':
      enable_ptrs=1;
      for(i=0; i < atoi(optarg); i++){
        ptrs_arg[i] = atoi(argv[optind++]);
      }
      break;

    case 'U':
      modify_dmrs = 1;
      for(i=0; i < atoi(optarg); i++){
        dmrs_arg[i] = atoi(argv[optind++]);
      }
      break;

    case 'Q':
      params_from_file = 1;
      break;

    case 'Z':

      transform_precoding = 0; // enabled
      num_dmrs_cdm_grps_no_data = 2;
      mcs_table = 3;
      
      printf("NOTE: TRANSFORM PRECODING (SC-FDMA) is ENABLED in UPLINK (0 - ENABLE, 1 - DISABLE) : %d \n",  transform_precoding);

      break;

    default:
    case 'h':
      printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId -Z Enable SC-FDMA in Uplink \n", argv[0]);
      //printf("-d Use TDD\n");
      printf("-d Introduce delay in terms of number of samples\n");
      printf("-f Number of frames to simulate\n");
      printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
      printf("-h This message\n");
      printf("-i Activate PRB based averaging for channel estimation. Frequncy domain interpolation by default.\n");
      //printf("-j Relative strength of second intefering eNB (in dB) - cell_id mod 3 = 2\n");
      printf("-s Starting SNR, runs from SNR0 to SNR0 + 10 dB if ending SNR isn't given\n");
      printf("-m MCS value\n");
      printf("-n Number of trials to simulate\n");
      printf("-p Use extended prefix mode\n");
      printf("-t Delay spread for multipath channel\n");
      printf("-u Set the numerology\n");
      printf("-w Start PRB for PUSCH\n");
      //printf("-x Transmission mode (1,2,6 for the moment)\n");
      printf("-y Number of TX antennas used in eNB\n");
      printf("-z Number of RX antennas used in UE\n");
      printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
      //printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      printf("-G Offset of samples to read from file (0 default)\n");
      printf("-M Multiple SSB positions in burst\n");
      printf("-N Nid_cell\n");
      printf("-O oversampling factor (1,2,4,8,16)\n");
      printf("-R N_RB_DL\n");
      printf("-t Acceptable effective throughput (in percentage)\n");
      printf("-S Ending SNR, runs from SNR0 to SNR1\n");
      printf("-P Print ULSCH performances\n");
      printf("-T Enable PTRS, arguments list L_PTRS{0,1,2} K_PTRS{2,4}, e.g. -T 2 0 2 \n");
      printf("-U Change DMRS Config, arguments list DMRS TYPE{0=A,1=B} DMRS AddPos{0:3}, e.g. -U 2 0 2 \n");
      printf("-Q If -F used, read parameters from file\n");
      printf("-Z If -Z is used, SC-FDMA or transform precoding is enabled in Uplink \n");
      exit(-1);
      break;

    }
  }
  
  logInit();
  set_glog(loglvl);
  T_stdout = 1;

  get_softmodem_params()->phy_test = 1;
  get_softmodem_params()->do_ra = 0;
  get_softmodem_params()->usim_test = 1;


  if (snr1set == 0)
    snr1 = snr0 + 10;
  double sampling_frequency;
  double bandwidth;

  if (N_RB_UL >= 217) sampling_frequency = 122.88;
  else if (N_RB_UL >= 106) sampling_frequency = 61.44;
  else if (N_RB_UL >= 32) sampling_frequency = 32.72;
  else { printf("Need at least 106 PRBs\b"); exit(-1); }
  if (N_RB_UL == 273) bandwidth = 100;
  else if (N_RB_UL == 217) bandwidth = 80;
  else if (N_RB_UL == 106) bandwidth = 40;
  else if (N_RB_UL == 32) bandwidth = 50;
  else { printf("Add N_RB_UL %d\n",N_RB_UL); exit(-1); }
			   
  if (openair0_cfg[0].threequarter_fs == 1) sampling_frequency*=.75;

  UE2gNB = new_channel_desc_scm(n_tx, n_rx, channel_model,
                                sampling_frequency,
                                bandwidth,
				DS_TDL,
                                0, 0, 0, 0);

  if (UE2gNB == NULL) {
    printf("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  UE2gNB->max_Doppler = maxDoppler;

  RC.gNB = (PHY_VARS_gNB **) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0] = calloc(1,sizeof(PHY_VARS_gNB));
  gNB = RC.gNB[0];
  gNB->ofdm_offset_divisor = UINT_MAX;
  gNB->threadPool = (tpool_t*)malloc(sizeof(tpool_t));
  gNB->respDecode = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  char tp_param[80];
  sprintf(tp_param,"-1");
  int s_offset = 0;
  for (int icpu=1; icpu<threadCnt; icpu++) {
    sprintf(tp_param+2+s_offset,",-1");
    s_offset += 3;
  }

  initTpool(tp_param, gNB->threadPool, false);
  initNotifiedFIFO(gNB->respDecode);
  gNB->resp_L1_tx = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  initNotifiedFIFO(gNB->resp_L1_tx);
  notifiedFIFO_elt_t *msgL1Tx = newNotifiedFIFO_elt(sizeof(processingData_L1tx_t),0,gNB->resp_L1_tx,NULL);
  processingData_L1tx_t *msgDataTx = (processingData_L1tx_t *)NotifiedFifoData(msgL1Tx);
  msgDataTx->slot = -1;
  gNB->phy_proc_tx_0 = &msgDataTx->phy_proc_tx;
  pushNotifiedFIFO(gNB->resp_L1_tx,msgL1Tx); // to unblock the process in the beginning
  //gNB_config = &gNB->gNB_config;

  //memset((void *)&gNB->UL_INFO,0,sizeof(gNB->UL_INFO));
  gNB->UL_INFO.rx_ind.pdu_list = (nfapi_nr_rx_data_pdu_t *)malloc(NB_UE_INST*sizeof(nfapi_nr_rx_data_pdu_t));
  gNB->UL_INFO.crc_ind.crc_list = (nfapi_nr_crc_t *)malloc(NB_UE_INST*sizeof(nfapi_nr_crc_t));
  gNB->UL_INFO.rx_ind.number_of_pdus = 0;
  gNB->UL_INFO.crc_ind.number_crcs = 0;
  gNB->prb_interpolation = prb_inter;
  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)

  frame_parms->N_RB_DL = N_RB_DL;
  frame_parms->N_RB_UL = N_RB_UL;
  frame_parms->Ncp = extended_prefix_flag ? EXTENDED : NORMAL;

  s_re = malloc(n_tx*sizeof(double*));
  s_im = malloc(n_tx*sizeof(double*));
  r_re = malloc(n_rx*sizeof(double*));
  r_im = malloc(n_rx*sizeof(double*));

  RC.nb_nr_macrlc_inst = 1;
  RC.nb_nr_mac_CC = (int*)malloc(RC.nb_nr_macrlc_inst*sizeof(int));
  for (i = 0; i < RC.nb_nr_macrlc_inst; i++)
    RC.nb_nr_mac_CC[i] = 1;
  mac_top_init_gNB();
  //gNB_MAC_INST* gNB_mac = RC.nrmac[0];
  gNB_RRC_INST rrc;
  memset((void*)&rrc,0,sizeof(rrc));

  rrc.carrier.servingcellconfigcommon = calloc(1,sizeof(*rrc.carrier.servingcellconfigcommon));

  NR_ServingCellConfigCommon_t *scc = rrc.carrier.servingcellconfigcommon;
  NR_ServingCellConfig_t *scd = calloc(1,sizeof(NR_ServingCellConfig_t));
  NR_CellGroupConfig_t *secondaryCellGroup=calloc(1,sizeof(*secondaryCellGroup));
  prepare_scc(rrc.carrier.servingcellconfigcommon);
  uint64_t ssb_bitmap;
  fill_scc_sim(rrc.carrier.servingcellconfigcommon,&ssb_bitmap,N_RB_DL,N_RB_DL,mu,mu);

  fix_scc(scc,ssb_bitmap);

  prepare_scd(scd);

  fill_default_secondaryCellGroup(scc, scd, secondaryCellGroup, 0, 1, n_tx, 0, 0, 0, 0);

  // xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void*)secondaryCellGroup);

  /* RRC parameter validation for secondaryCellGroup */
  fix_scd(scd);

  AssertFatal((gNB->if_inst         = NR_IF_Module_init(0))!=NULL,"Cannot register interface");

  gNB->if_inst->NR_PHY_config_req      = nr_phy_config_request;
  // common configuration
  rrc_mac_config_req_gNB(0,0, n_tx, n_rx, 0, scc, &rrc.carrier.mib,0, 0, NULL);
  // UE dedicated configuration
  rrc_mac_config_req_gNB(0,0, n_tx, n_rx, 0, scc, &rrc.carrier.mib,1, secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity,secondaryCellGroup);
  frame_parms->nb_antennas_tx = n_tx;
  frame_parms->nb_antennas_rx = n_rx;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  cfg->carrier_config.num_tx_ant.value = n_tx;
  cfg->carrier_config.num_rx_ant.value = n_rx;
//  nr_phy_config_request_sim(gNB,N_RB_DL,N_RB_DL,mu,0,0x01);
  phy_init_nr_gNB(gNB,0,1);
  N_RB_DL = gNB->frame_parms.N_RB_DL;


  NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[0];


  //configure UE
  UE = malloc(sizeof(PHY_VARS_NR_UE));
  memset((void*)UE,0,sizeof(PHY_VARS_NR_UE));
  PHY_vars_UE_g = malloc(sizeof(PHY_VARS_NR_UE**));
  PHY_vars_UE_g[0] = malloc(sizeof(PHY_VARS_NR_UE*));
  PHY_vars_UE_g[0][0] = UE;
  memcpy(&UE->frame_parms, frame_parms, sizeof(NR_DL_FRAME_PARMS));

  //phy_init_nr_top(frame_parms);
  if (init_nr_ue_signal(UE, 1, 0) != 0) {
    printf("Error at UE NR initialisation\n");
    exit(-1);
  }

  //nr_init_frame_parms_ue(&UE->frame_parms);
  init_nr_ue_transport(UE, 0);

  /*
  for (int sf = 0; sf < 2; sf++) {
    for (i = 0; i < 2; i++) {

        UE->ulsch[sf][0][i] = new_nr_ue_ulsch(N_RB_UL, 8, 0);

        if (!UE->ulsch[sf][0][i]) {
          printf("Can't get ue ulsch structures\n");
          exit(-1);
        }
    }
  }
  */

  //Configure UE
  NR_UE_RRC_INST_t rrcue;
  memset(&rrcue,0,sizeof(NR_UE_RRC_INST_t));
  rrc.carrier.MIB = (uint8_t*) malloc(4);
  rrc.carrier.sizeof_MIB = do_MIB_NR(&rrc,0);
  rrcue.mib = rrc.carrier.mib.message.choice.mib;
  rrcue.scell_group_config=secondaryCellGroup;
  nr_l2_init_ue(&rrcue);

  NR_UE_MAC_INST_t* UE_mac = get_mac_inst(0);
  
  UE->if_inst = nr_ue_if_module_init(0);
  UE->if_inst->scheduled_response = nr_ue_scheduled_response;
  UE->if_inst->phy_config_request = nr_ue_phy_config_request;
  UE->if_inst->dl_indication = nr_ue_dl_indication;
  UE->if_inst->ul_indication = nr_ue_ul_indication;
  
  UE_mac->if_module = nr_ue_if_module_init(0);

//  nr_rrc_mac_config_req_ue(0,0,0,rrc.carrier.mib.message.choice.mib, NULL, NULL, secondaryCellGroup);

  nr_ue_phy_config_request(&UE_mac->phy_config);


  unsigned char harq_pid = 0;

  NR_gNB_ULSCH_t *ulsch_gNB = gNB->ulsch[UE_id][0];
  //nfapi_nr_ul_config_ulsch_pdu *rel15_ul = &ulsch_gNB->harq_processes[harq_pid]->ulsch_pdu;
  nfapi_nr_ul_tti_request_t     *UL_tti_req  = malloc(sizeof(*UL_tti_req));
  NR_Sched_Rsp_t *Sched_INFO = malloc(sizeof(*Sched_INFO));
  memset((void*)Sched_INFO,0,sizeof(*Sched_INFO));
  Sched_INFO->UL_tti_req=UL_tti_req;

  nfapi_nr_pusch_pdu_t  *pusch_pdu = &UL_tti_req->pdus_list[0].pusch_pdu;

  NR_UE_ULSCH_t **ulsch_ue = UE->ulsch[0][0];

  unsigned char *estimated_output_bit;
  unsigned char *test_input_bit;
  uint32_t errors_decoding   = 0;
  

  test_input_bit       = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
  estimated_output_bit = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);

  nr_scheduled_response_t scheduled_response;
  fapi_nr_ul_config_request_t ul_config;
  fapi_nr_tx_request_t tx_req;

  memset(&scheduled_response, 0, sizeof(scheduled_response));
  memset(&ul_config, 0, sizeof(ul_config));
  memset(&tx_req, 0, sizeof(tx_req));

  uint8_t ptrs_mcs1 = 2;
  uint8_t ptrs_mcs2 = 4;
  uint8_t ptrs_mcs3 = 10;
  uint16_t n_rb0 = 25;
  uint16_t n_rb1 = 75;
  
  uint16_t pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA; // | PUSCH_PDU_BITMAP_PUSCH_PTRS;
  uint8_t max_rounds = 4;
  uint8_t crc_status = 0;

  unsigned char mod_order = nr_get_Qm_ul(Imcs, mcs_table);
  uint16_t      code_rate = nr_get_code_rate_ul(Imcs, mcs_table);

  uint8_t mapping_type = typeB; // Default Values
  pusch_dmrs_type_t dmrs_config_type = pusch_dmrs_type1; // Default Values
  pusch_dmrs_AdditionalPosition_t add_pos = pusch_dmrs_pos0; // Default Values

  /* validate parameters othwerwise default values are used */
  /* -U flag can be used to set DMRS parameters*/
  if(modify_dmrs)
  {
    if(dmrs_arg[0] == 0)
    {
      mapping_type = typeA;
    }
    else if (dmrs_arg[0] == 1)
    {
      mapping_type = typeB;
    }
    /* Additional DMRS positions */
    if(dmrs_arg[1] >= 0 && dmrs_arg[1] <=3 )
    {
      add_pos = dmrs_arg[1];
    }
  }
  printf("NOTE: DMRS config is modified with Mapping Type %d , Additional Position %d \n", mapping_type, add_pos );

  uint8_t  length_dmrs         = pusch_len1;
  uint16_t l_prime_mask        = get_l_prime(nb_symb_sch, mapping_type, add_pos, length_dmrs, start_symbol, NR_MIB__dmrs_TypeA_Position_pos2);
  uint16_t number_dmrs_symbols = get_dmrs_symbols_in_slot(l_prime_mask, nb_symb_sch);
  printf("num dmrs sym %d\n",number_dmrs_symbols);
  uint8_t  nb_re_dmrs          = (dmrs_config_type == pusch_dmrs_type1) ? 6 : 4;

  // if transform precoding is enabled
  if (transform_precoding == 0) {

    AssertFatal(enable_ptrs == 0, "PTRS NOT SUPPORTED IF TRANSFORM PRECODING IS ENABLED\n");

    int8_t index = get_index_for_dmrs_lowpapr_seq((NR_NB_SC_PER_RB/2) * nb_rb);
	  AssertFatal(index >= 0, "Num RBs not configured according to 3GPP 38.211 section 6.3.1.4. For PUSCH with transform precoding, num RBs cannot be multiple of any other primenumber other than 2,3,5\n");
    
    dmrs_config_type = pusch_dmrs_type1;

    printf("[ULSIM]: TRANSFORM PRECODING ENABLED. Num RBs: %d, index for DMRS_SEQ: %d\n", nb_rb, index);
  }

  nb_re_dmrs   = nb_re_dmrs * num_dmrs_cdm_grps_no_data;

  unsigned int available_bits  = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, number_dmrs_symbols, mod_order, 1);
  unsigned int TBS             = nr_compute_tbs(mod_order, code_rate, nb_rb, nb_symb_sch, nb_re_dmrs * number_dmrs_symbols, 0, 0, precod_nbr_layers);

  
  printf("[ULSIM]: length_dmrs: %u, l_prime_mask: %u	number_dmrs_symbols: %u, mapping_type: %u add_pos: %d \n", length_dmrs, l_prime_mask, number_dmrs_symbols, mapping_type, add_pos);
  printf("[ULSIM]: CDM groups: %u, dmrs_config_type: %d, num_rbs: %u, nb_symb_sch: %u\n", num_dmrs_cdm_grps_no_data, dmrs_config_type, nb_rb, nb_symb_sch);
  printf("[ULSIM]: MCS: %d, mod order: %u, code_rate: %u\n", Imcs, mod_order, code_rate);
  printf("[ULSIM]: VALUE OF G: %u, TBS: %u\n", available_bits, TBS);
  

  
  uint8_t ulsch_input_buffer[TBS/8];

  ulsch_input_buffer[0] = 0x31;
  for (i = 1; i < TBS/8; i++) {
    ulsch_input_buffer[i] = (unsigned char) rand();
  }

  uint8_t ptrs_time_density = get_L_ptrs(ptrs_mcs1, ptrs_mcs2, ptrs_mcs3, Imcs, mcs_table);
  uint8_t ptrs_freq_density = get_K_ptrs(n_rb0, n_rb1, nb_rb);
  int     ptrs_symbols      = 0; // to calculate total PTRS RE's in a slot

  double ts = 1.0/(frame_parms->subcarrier_spacing * frame_parms->ofdm_symbol_size);

  /* -T option enable PTRS */
  if(enable_ptrs)
  {
    /* validate parameters othwerwise default values are used */
    if(ptrs_arg[0] == 0 || ptrs_arg[0] == 1 || ptrs_arg[0] == 2 )
    {
      ptrs_time_density = ptrs_arg[0];
    }
    if(ptrs_arg[1] == 2 || ptrs_arg[1] == 4 )
    {
      ptrs_freq_density = ptrs_arg[1];
    }
    pdu_bit_map |= PUSCH_PDU_BITMAP_PUSCH_PTRS;
    printf("NOTE: PTRS Enabled with L %d, K %d \n", ptrs_time_density, ptrs_freq_density );
  }

  if (input_fd != NULL || n_trials == 1) max_rounds=1;

  if(1<<ptrs_time_density >= nb_symb_sch)
    pdu_bit_map &= ~PUSCH_PDU_BITMAP_PUSCH_PTRS; // disable PUSCH PTRS

  printf("\n");

  int frame_length_complex_samples = frame_parms->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
  for (int aatx=0; aatx<n_tx; aatx++) {
    s_re[aatx] = calloc(1,frame_length_complex_samples*sizeof(double));
    s_im[aatx] = calloc(1,frame_length_complex_samples*sizeof(double));
  }

  for (int aarx=0; aarx<n_rx; aarx++) {
    r_re[aarx] = calloc(1,frame_length_complex_samples*sizeof(double));
    r_im[aarx] = calloc(1,frame_length_complex_samples*sizeof(double));
  }

  //for (int i=0;i<16;i++) printf("%f\n",gaussdouble(0.0,1.0));
  snrRun = 0;
  int n_errs = 0;
  int read_errors=0;

  int slot_offset = frame_parms->get_samples_slot_timestamp(slot,frame_parms,0);
  int slot_length = slot_offset - frame_parms->get_samples_slot_timestamp(slot-1,frame_parms,0);

  if (input_fd != NULL)	{
    AssertFatal(frame_parms->nb_antennas_rx == 1, "nb_ant != 1\n");
    // 800 samples is N_TA_OFFSET for FR1 @ 30.72 Ms/s,
    AssertFatal(frame_parms->subcarrier_spacing==30000,"only 30 kHz for file input for now (%d)\n",frame_parms->subcarrier_spacing);
  
    if (params_from_file) {
      fseek(input_fd,file_offset*((slot_length<<2)+4000+16),SEEK_SET);
      read_errors+=fread((void*)&n_rnti,sizeof(int16_t),1,input_fd);
      printf("rnti %x\n",n_rnti);
      read_errors+=fread((void*)&nb_rb,sizeof(int16_t),1,input_fd);
      printf("nb_rb %d\n",nb_rb);
      int16_t dummy;
      read_errors+=fread((void*)&start_rb,sizeof(int16_t),1,input_fd);
      //fread((void*)&dummy,sizeof(int16_t),1,input_fd);
      printf("rb_start %d\n",start_rb);
      read_errors+=fread((void*)&nb_symb_sch,sizeof(int16_t),1,input_fd);
      //fread((void*)&dummy,sizeof(int16_t),1,input_fd);
      printf("nb_symb_sch %d\n",nb_symb_sch);
      read_errors+=fread((void*)&start_symbol,sizeof(int16_t),1,input_fd);
      printf("start_symbol %d\n",start_symbol);
      read_errors+=fread((void*)&Imcs,sizeof(int16_t),1,input_fd);
      printf("mcs %d\n",Imcs);
      read_errors+=fread((void*)&rv_index,sizeof(int16_t),1,input_fd);
      printf("rv_index %d\n",rv_index);
      //    fread((void*)&harq_pid,sizeof(int16_t),1,input_fd);
      read_errors+=fread((void*)&dummy,sizeof(int16_t),1,input_fd);
      printf("harq_pid %d\n",harq_pid);
    }
    fseek(input_fd,file_offset*sizeof(int16_t)*2,SEEK_SET);
    read_errors+=fread((void*)&gNB->common_vars.rxdata[0][slot_offset-delay],
    sizeof(int16_t),
    slot_length<<1,
    input_fd);
    if (read_errors==0) {
      printf("error reading file\n");
      exit(1);
    }
    for (int i=0;i<16;i+=2) printf("slot_offset %d : %d,%d\n",
				   slot_offset,
				   ((int16_t*)&gNB->common_vars.rxdata[0][slot_offset])[i],
				   ((int16_t*)&gNB->common_vars.rxdata[0][slot_offset])[1+i]);

    mod_order = nr_get_Qm_ul(Imcs, mcs_table);
    code_rate = nr_get_code_rate_ul(Imcs, mcs_table);
  }
  
  uint32_t errors_scrambling[4][100];
  int n_errors[4][100];
  int round_trials[4][100];
  double blerStats[4][100];
  double berStats[4][100];
  double snrStats[100];
  memset(errors_scrambling, 0, sizeof(uint32_t)*4*100);
  memset(n_errors, 0, sizeof(int)*4*100);
  memset(round_trials, 0, sizeof(int)*4*100);
  memset(blerStats, 0, sizeof(double)*4*100);
  memset(berStats, 0, sizeof(double)*4*100);
  memset(snrStats, 0, sizeof(double)*100);
  for (SNR = snr0; SNR < snr1; SNR += snr_step) {
    varArray_t *table_rx=initVarArray(1000,sizeof(double));
    int error_flag = 0;
    n_false_positive = 0;
    effRate[snrRun] = 0;
    effTP[snrRun] = 0;
    reset_meas(&gNB->phy_proc_rx);
    reset_meas(&gNB->rx_pusch_stats);
    reset_meas(&gNB->ulsch_decoding_stats);
    reset_meas(&gNB->ulsch_deinterleaving_stats);
    reset_meas(&gNB->ulsch_rate_unmatching_stats);
    reset_meas(&gNB->ulsch_ldpc_decoding_stats);
    reset_meas(&gNB->ulsch_unscrambling_stats);
    reset_meas(&gNB->ulsch_channel_estimation_stats);
    reset_meas(&gNB->ulsch_llr_stats);
    reset_meas(&gNB->ulsch_channel_compensation_stats);
    reset_meas(&gNB->ulsch_rbs_extraction_stats);
    reset_meas(&UE->ulsch_ldpc_encoding_stats);
    reset_meas(&UE->ulsch_rate_matching_stats);
    reset_meas(&UE->ulsch_interleaving_stats);
    reset_meas(&UE->ulsch_encoding_stats);

    clear_pusch_stats(gNB);
    for (trial = 0; trial < n_trials; trial++) {
    uint8_t round = 0;

    crc_status = 1;
    errors_decoding    = 0;
    memset((void*)roundStats,0,50*sizeof(roundStats[0]));
    while (round<max_rounds && crc_status) {
      round_trials[round][snrRun]++;
      ulsch_ue[0]->harq_processes[harq_pid]->round = round;
      gNB->ulsch[0][0]->harq_processes[harq_pid]->round = round;
      rv_index = nr_rv_round_map[round];

      UE_proc.thread_id = 0;
      UE_proc.nr_slot_tx = slot;
      UE_proc.frame_tx = frame;

      UL_tti_req->SFN = frame;
      UL_tti_req->Slot = slot;
      UL_tti_req->n_pdus = 1;
      UL_tti_req->pdus_list[0].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
      UL_tti_req->pdus_list[0].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
      memset(pusch_pdu,0,sizeof(nfapi_nr_pusch_pdu_t));
      
      int abwp_size  = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
      int abwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
      int ibwp_size  = ibwps;
      int ibwp_start = ibwp_rboffset;
      if (msg3_flag == 1) {
	if ((ibwp_start < abwp_start) || (ibwp_size > abwp_size))
	  pusch_pdu->bwp_start = abwp_start;
	else
	  pusch_pdu->bwp_start = ibwp_start;
	pusch_pdu->bwp_size = ibwp_size;
	start_rb = (ibwp_start - abwp_start);
	printf("msg3: ibwp_size %d, abwp_size %d, ibwp_start %d, abwp_start %d\n",
	       ibwp_size,abwp_size,ibwp_start,abwp_start);
      }
      else {
	pusch_pdu->bwp_start = abwp_start;
	pusch_pdu->bwp_size = abwp_size;
      }

      pusch_pdu->pusch_data.tb_size = TBS/8;
      pusch_pdu->pdu_bit_map = pdu_bit_map;
      pusch_pdu->rnti = n_rnti;
      pusch_pdu->mcs_index = Imcs;
      pusch_pdu->mcs_table = mcs_table;
      pusch_pdu->target_code_rate = code_rate;
      pusch_pdu->qam_mod_order = mod_order;
      pusch_pdu->transform_precoding = transform_precoding;
      pusch_pdu->data_scrambling_id = *scc->physCellId;
      pusch_pdu->nrOfLayers = 1;
      pusch_pdu->ul_dmrs_symb_pos = l_prime_mask;
      pusch_pdu->dmrs_config_type = dmrs_config_type;
      pusch_pdu->ul_dmrs_scrambling_id =  *scc->physCellId;
      pusch_pdu->scid = 0;
      pusch_pdu->dmrs_ports = 1;
      pusch_pdu->num_dmrs_cdm_grps_no_data = num_dmrs_cdm_grps_no_data;
      pusch_pdu->resource_alloc = 1; 
      pusch_pdu->rb_start = start_rb;
      pusch_pdu->rb_size = nb_rb;
      pusch_pdu->vrb_to_prb_mapping = 0;
      pusch_pdu->frequency_hopping = 0;
      pusch_pdu->uplink_frequency_shift_7p5khz = 0;
      pusch_pdu->start_symbol_index = start_symbol;
      pusch_pdu->nr_of_symbols = nb_symb_sch;
      pusch_pdu->pusch_data.rv_index = rv_index;
      pusch_pdu->pusch_data.harq_process_id = 0;
      pusch_pdu->pusch_data.new_data_indicator = trial & 0x1;
      pusch_pdu->pusch_data.num_cb = 0;
      pusch_pdu->pusch_ptrs.ptrs_time_density = ptrs_time_density;
      pusch_pdu->pusch_ptrs.ptrs_freq_density = ptrs_freq_density;
      pusch_pdu->pusch_ptrs.ptrs_ports_list   = (nfapi_nr_ptrs_ports_t *) malloc(2*sizeof(nfapi_nr_ptrs_ports_t));
      pusch_pdu->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset = 0;

      // if transform precoding is enabled
      if (transform_precoding == 0) {

        pusch_pdu->dfts_ofdm.low_papr_group_number = *scc->physCellId % 30; // U as defined in 38.211 section 6.4.1.1.1.2 
        pusch_pdu->dfts_ofdm.low_papr_sequence_number = 0;     // V as defined in 38.211 section 6.4.1.1.1.2
        pusch_pdu->num_dmrs_cdm_grps_no_data = num_dmrs_cdm_grps_no_data;        
      }

      // prepare ULSCH/PUSCH reception
      nr_schedule_response(Sched_INFO);

      // --------- setting parameters for UE --------

      scheduled_response.module_id = 0;
      scheduled_response.CC_id = 0;
      scheduled_response.frame = frame;
      scheduled_response.slot = slot;
      scheduled_response.thread_id = UE_proc.thread_id;
      scheduled_response.dl_config = NULL;
      scheduled_response.ul_config = &ul_config;
      scheduled_response.tx_request = &tx_req;
      
      // Config UL TX PDU
      tx_req.slot = slot;
      tx_req.sfn = frame;
      // tx_req->tx_config // TbD
      tx_req.number_of_pdus = 1;
      tx_req.tx_request_body[0].pdu_length = TBS/8;
      tx_req.tx_request_body[0].pdu_index = 0;
      tx_req.tx_request_body[0].pdu = &ulsch_input_buffer[0];

      ul_config.slot = slot;
      ul_config.number_pdus = 1;
      ul_config.ul_config_list[0].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
      ul_config.ul_config_list[0].pusch_config_pdu.rnti = n_rnti;
      ul_config.ul_config_list[0].pusch_config_pdu.pdu_bit_map = pdu_bit_map;
      ul_config.ul_config_list[0].pusch_config_pdu.qam_mod_order = mod_order;
      ul_config.ul_config_list[0].pusch_config_pdu.rb_size = nb_rb;
      ul_config.ul_config_list[0].pusch_config_pdu.rb_start = start_rb;
      ul_config.ul_config_list[0].pusch_config_pdu.nr_of_symbols = nb_symb_sch;
      ul_config.ul_config_list[0].pusch_config_pdu.start_symbol_index = start_symbol;
      ul_config.ul_config_list[0].pusch_config_pdu.ul_dmrs_symb_pos = l_prime_mask;
      ul_config.ul_config_list[0].pusch_config_pdu.dmrs_config_type = dmrs_config_type;
      ul_config.ul_config_list[0].pusch_config_pdu.mcs_index = Imcs;
      ul_config.ul_config_list[0].pusch_config_pdu.mcs_table = mcs_table;
      ul_config.ul_config_list[0].pusch_config_pdu.num_dmrs_cdm_grps_no_data = num_dmrs_cdm_grps_no_data;
      ul_config.ul_config_list[0].pusch_config_pdu.nrOfLayers = precod_nbr_layers;
      ul_config.ul_config_list[0].pusch_config_pdu.absolute_delta_PUSCH = 0;

      ul_config.ul_config_list[0].pusch_config_pdu.pusch_data.tb_size = TBS/8;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_data.new_data_indicator = trial & 0x1;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_data.rv_index = rv_index;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_data.harq_process_id = harq_pid;

      ul_config.ul_config_list[0].pusch_config_pdu.pusch_ptrs.ptrs_time_density = ptrs_time_density;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_ptrs.ptrs_freq_density = ptrs_freq_density;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_ptrs.ptrs_ports_list   = (nfapi_nr_ue_ptrs_ports_t *) malloc(2*sizeof(nfapi_nr_ue_ptrs_ports_t));
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset = 0;

      ul_config.ul_config_list[0].pusch_config_pdu.transform_precoding = transform_precoding;

      // if transform precoding is enabled
      if (transform_precoding == 0) {
   
        ul_config.ul_config_list[0].pusch_config_pdu.dfts_ofdm.low_papr_group_number = *scc->physCellId % 30;// U as defined in 38.211 section 6.4.1.1.1.2 
        ul_config.ul_config_list[0].pusch_config_pdu.dfts_ofdm.low_papr_sequence_number = 0;// V as defined in 38.211 section 6.4.1.1.1.2
        //ul_config.ul_config_list[0].pusch_config_pdu.pdu_bit_map |= PUSCH_PDU_BITMAP_DFTS_OFDM; 
        ul_config.ul_config_list[0].pusch_config_pdu.num_dmrs_cdm_grps_no_data = num_dmrs_cdm_grps_no_data;

      }


      //nr_fill_ulsch(gNB,frame,slot,pusch_pdu); // Not needed as its its already filled as apart of "nr_schedule_response(Sched_INFO);"

      for (int i=0;i<(TBS/8);i++) ulsch_ue[0]->harq_processes[harq_pid]->a[i]=i&0xff;
      if (input_fd == NULL) {

        // set FAPI parameters for UE, put them in the scheduled response and call
        nr_ue_scheduled_response(&scheduled_response);


        /////////////////////////phy_procedures_nr_ue_TX///////////////////////
        ///////////

        phy_procedures_nrUE_TX(UE, &UE_proc, gNB_id);

        /* We need to call common sending function to send signal */
        LOG_D(PHY, "Sending Uplink data \n");
        nr_ue_pusch_common_procedures(UE,
                                      slot,
                                      &UE->frame_parms,1);

        if (n_trials==1) {
          LOG_M("txsig0.m","txs0", UE->common_vars.txdata[0],frame_parms->samples_per_subframe*10,1,1);
          LOG_M("txsig0F.m","txs0F", UE->common_vars.txdataF[0],frame_parms->ofdm_symbol_size*14,1,1);
        }
        ///////////
        ////////////////////////////////////////////////////
        tx_offset = frame_parms->get_samples_slot_timestamp(slot,frame_parms,0);

        txlev = signal_energy(&UE->common_vars.txdata[0][tx_offset + 5*frame_parms->ofdm_symbol_size + 4*frame_parms->nb_prefix_samples + frame_parms->nb_prefix_samples0],
                              frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples);
      }	
      else n_trials = 1;

      if (input_fd == NULL ) {

        sigma_dB = 10 * log10((double)txlev * ((double)frame_parms->ofdm_symbol_size/(12*nb_rb))) - SNR;;
        sigma    = pow(10,sigma_dB/10);


        if(n_trials==1) printf("sigma %f (%f dB), txlev %f (factor %f)\n",sigma,sigma_dB,10*log10((double)txlev),(double)(double)
                                frame_parms->ofdm_symbol_size/(12*nb_rb));

        for (i=0; i<slot_length; i++) {
          for (int aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
            s_re[aa][i] = ((double)(((short *)&UE->common_vars.txdata[aa][slot_offset]))[(i<<1)]);
            s_im[aa][i] = ((double)(((short *)&UE->common_vars.txdata[aa][slot_offset]))[(i<<1)+1]);
          }
        }


        if (UE2gNB->max_Doppler == 0) {
          multipath_channel(UE2gNB, s_re, s_im, r_re, r_im, slot_length, 0, (n_trials==1)?1:0);
        } else {
          multipath_tv_channel(UE2gNB, s_re, s_im, r_re, r_im, 2*slot_length, 0);
        }
        for (i=0; i<slot_length; i++) {
          for (ap=0; ap<frame_parms->nb_antennas_rx; ap++) {
            ((int16_t*) &gNB->common_vars.rxdata[ap][slot_offset])[(2*i)   + (delay*2)] = (int16_t)((r_re[ap][i]) + (sqrt(sigma/2)*gaussdouble(0.0,1.0))); // convert to fixed point
            ((int16_t*) &gNB->common_vars.rxdata[ap][slot_offset])[(2*i)+1 + (delay*2)] = (int16_t)((r_im[ap][i]) + (sqrt(sigma/2)*gaussdouble(0.0,1.0)));
            /* Add phase noise if enabled */
            if (pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
              phase_noise(ts, &((int16_t*)&gNB->common_vars.rxdata[ap][slot_offset])[(2*i)],
                          &((int16_t*)&gNB->common_vars.rxdata[ap][slot_offset])[(2*i)+1]);
            }
          }
        }

      } /*End input_fd */


      if(pusch_pdu->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
        set_ptrs_symb_idx(&ptrsSymPos,
                          pusch_pdu->nr_of_symbols,
                          pusch_pdu->start_symbol_index,
                          1<<ptrs_time_density,
                          pusch_pdu->ul_dmrs_symb_pos);
        ptrsSymbPerSlot = get_ptrs_symbols_in_slot(ptrsSymPos, pusch_pdu->start_symbol_index, pusch_pdu->nr_of_symbols);
        ptrsRePerSymb = ((pusch_pdu->rb_size + ptrs_freq_density - 1)/ptrs_freq_density);
        printf("[ULSIM] PTRS Symbols in a slot: %2u, RE per Symbol: %3u, RE in a slot %4d\n", ptrsSymbPerSlot,ptrsRePerSymb, ptrsSymbPerSlot*ptrsRePerSymb );
      }
	////////////////////////////////////////////////////////////
	
	//----------------------------------------------------------
	//------------------- gNB phy procedures -------------------
	//----------------------------------------------------------
	gNB->UL_INFO.rx_ind.number_of_pdus = 0;
	gNB->UL_INFO.crc_ind.number_crcs = 0;

        phy_procedures_gNB_common_RX(gNB, frame, slot);

        ul_proc_error = phy_procedures_gNB_uespec_RX(gNB, frame, slot);

	if (n_trials==1 && round==0) {
	  LOG_M("rxsig0.m","rx0",&gNB->common_vars.rxdata[0][slot_offset],slot_length,1,1);

	  LOG_M("rxsigF0.m","rxsF0",gNB->common_vars.rxdataF[0]+start_symbol*frame_parms->ofdm_symbol_size,nb_symb_sch*frame_parms->ofdm_symbol_size,1,1);

	}


	if (n_trials == 1  && round==0) {
#ifdef __AVX2__
	  int off = ((nb_rb&1) == 1)? 4:0;
#else
	  int off = 0;
#endif

	  LOG_M("rxsigF0_ext.m","rxsF0_ext",
		&gNB->pusch_vars[0]->rxdataF_ext[0][start_symbol*NR_NB_SC_PER_RB * pusch_pdu->rb_size],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
	  LOG_M("chestF0.m","chF0",
		&gNB->pusch_vars[0]->ul_ch_estimates[0][start_symbol*frame_parms->ofdm_symbol_size],frame_parms->ofdm_symbol_size,1,1);
	  LOG_M("chestT0.m","chT0",
		&gNB->pusch_vars[0]->ul_ch_estimates_time[0][0],frame_parms->ofdm_symbol_size,1,1);
	  LOG_M("chestF0_ext.m","chF0_ext",
		&gNB->pusch_vars[0]->ul_ch_estimates_ext[0][(start_symbol+1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
		(nb_symb_sch-1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
	  LOG_M("rxsigF0_comp.m","rxsF0_comp",
		&gNB->pusch_vars[0]->rxdataF_comp[0][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
    LOG_M("chmagF0.m","chmF0",
    &gNB->pusch_vars[0]->ul_ch_mag[0][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
    LOG_M("chmagbF0.m","chmbF0",
    &gNB->pusch_vars[0]->ul_ch_magb[0][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
    if (n_rx == 2) {
      LOG_MM("rxsigF0_comp.m","rxsF1_comp",
      &gNB->pusch_vars[0]->rxdataF_comp[1][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("rxsigF0_ext.m","rxsF1_ext",
      &gNB->pusch_vars[0]->rxdataF_ext[1][start_symbol*NR_NB_SC_PER_RB * pusch_pdu->rb_size],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chestF0_ext.m","chF1_ext",
      &gNB->pusch_vars[0]->ul_ch_estimates_ext[1][(start_symbol+1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
      (nb_symb_sch-1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chmagF0.m","chmF1",
      &gNB->pusch_vars[0]->ul_ch_mag[1][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chmagbF0.m","chmbF1",
      &gNB->pusch_vars[0]->ul_ch_magb[1][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
    } else if (n_rx == 4) {
      LOG_MM("rxsigF0_comp.m","rxsF1_comp",
      &gNB->pusch_vars[0]->rxdataF_comp[1][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("rxsigF0_comp.m","rxsF2_comp",
      &gNB->pusch_vars[0]->rxdataF_comp[2][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("rxsigF0_comp.m","rxsF3_comp",
      &gNB->pusch_vars[0]->rxdataF_comp[3][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("rxsigF0_ext.m","rxsF1_ext",
      &gNB->pusch_vars[0]->rxdataF_ext[1][start_symbol*NR_NB_SC_PER_RB * pusch_pdu->rb_size],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("rxsigF0_ext.m","rxsF2_ext",
      &gNB->pusch_vars[0]->rxdataF_ext[2][start_symbol*NR_NB_SC_PER_RB * pusch_pdu->rb_size],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("rxsigF0_ext.m","rxsF3_ext",
      &gNB->pusch_vars[0]->rxdataF_ext[3][start_symbol*NR_NB_SC_PER_RB * pusch_pdu->rb_size],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chestF0_ext.m","chF1_ext",
      &gNB->pusch_vars[0]->ul_ch_estimates_ext[1][(start_symbol+1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
      (nb_symb_sch-1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chestF0_ext.m","chF2_ext",
      &gNB->pusch_vars[0]->ul_ch_estimates_ext[2][(start_symbol+1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
      (nb_symb_sch-1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chestF0_ext.m","chF3_ext",
      &gNB->pusch_vars[0]->ul_ch_estimates_ext[3][(start_symbol+1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
      (nb_symb_sch-1)*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chmagF0.m","chmF1",
      &gNB->pusch_vars[0]->ul_ch_mag[1][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chmagF0.m","chmF2",
      &gNB->pusch_vars[0]->ul_ch_mag[2][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chmagF0.m","chmF3",
      &gNB->pusch_vars[0]->ul_ch_mag[3][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chmagbF0.m","chmbF1",
      &gNB->pusch_vars[0]->ul_ch_magb[1][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chmagbF0.m","chmbF2",
      &gNB->pusch_vars[0]->ul_ch_magb[2][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
      LOG_MM("chmagbF0.m","chmbF3",
      &gNB->pusch_vars[0]->ul_ch_magb[3][start_symbol*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size))],nb_symb_sch*(off+(NR_NB_SC_PER_RB * pusch_pdu->rb_size)),1,1);
    }

	  LOG_M("rxsigF0_llr.m","rxsF0_llr",
		&gNB->pusch_vars[0]->llr[0],(nb_symb_sch-1)*NR_NB_SC_PER_RB * pusch_pdu->rb_size * mod_order,1,0);
	}
        ////////////////////////////////////////////////////////////

	if ((gNB->ulsch[0][0]->last_iteration_cnt >=
	    gNB->ulsch[0][0]->max_ldpc_iterations+1) || ul_proc_error == 1) {
	  error_flag = 1; 
	  n_errors[round][snrRun]++;
	  crc_status = 1;
	} else {
	  crc_status = 0;
	}
	if(n_trials==1) printf("end of round %d rv_index %d\n",round, rv_index);

        //----------------------------------------------------------
        //----------------- count and print errors -----------------
        //----------------------------------------------------------

        if ((pusch_pdu->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) && (SNR==snr0) && (trial==0) && (round==0)) {
            ptrs_symbols = 0;
            for (int i = pusch_pdu->start_symbol_index; i < pusch_pdu->start_symbol_index + pusch_pdu->nr_of_symbols; i++){
               ptrs_symbols += ((gNB->pusch_vars[UE_id]->ptrs_symbols) >> i) & 1;
            }
            /*  2*5*(50/2), for RB = 50,K = 2 for 5 OFDM PTRS symbols */
            available_bits -= 2 * ptrs_symbols * ((nb_rb + ptrs_freq_density - 1) /ptrs_freq_density);
            printf("[ULSIM][PTRS] Available bits are: %5u, removed PTRS bits are: %5d \n",available_bits, (ptrsSymbPerSlot * ptrsRePerSymb * 2) );
        }

	for (i = 0; i < available_bits; i++) {
	  
	  if(((ulsch_ue[0]->g[i] == 0) && (gNB->pusch_vars[UE_id]->llr[i] <= 0)) ||
	     ((ulsch_ue[0]->g[i] == 1) && (gNB->pusch_vars[UE_id]->llr[i] >= 0)))
	    {
	      /*if(errors_scrambling == 0)
		printf("\x1B[34m" "[frame %d][trial %d]\t1st bit in error in unscrambling = %d\n" "\x1B[0m", frame, trial, i);*/
	      errors_scrambling[round][snrRun]++;
	    }
	}
	round++;

    } // round
    
    if (n_trials == 1 && errors_scrambling[0][snrRun] > 0) {
      printf("\x1B[31m""[frame %d][trial %d]\tnumber of errors in unscrambling = %u\n" "\x1B[0m", frame, trial, errors_scrambling[0][snrRun]);
    }
    
    for (i = 0; i < TBS; i++) {
      
      estimated_output_bit[i] = (ulsch_gNB->harq_processes[harq_pid]->b[i/8] & (1 << (i & 7))) >> (i & 7);
      test_input_bit[i]       = (ulsch_ue[0]->harq_processes[harq_pid]->b[i/8] & (1 << (i & 7))) >> (i & 7);
      
      if (estimated_output_bit[i] != test_input_bit[i]) {
	/*if(errors_decoding == 0)
	  printf("\x1B[34m""[frame %d][trial %d]\t1st bit in error in decoding     = %d\n" "\x1B[0m", frame, trial, i);*/
	errors_decoding++;
      }
    }
    if (n_trials == 1) {
      for (int r=0;r<ulsch_ue[0]->harq_processes[harq_pid]->C;r++) 
	for (int i=0;i<ulsch_ue[0]->harq_processes[harq_pid]->K>>3;i++) {
	  if ((ulsch_ue[0]->harq_processes[harq_pid]->c[r][i]^ulsch_gNB->harq_processes[harq_pid]->c[r][i]) != 0) printf("************");
	    /*printf("r %d: in[%d] %x, out[%d] %x (%x)\n",r,
	    i,ulsch_ue[0]->harq_processes[harq_pid]->c[r][i],
	    i,ulsch_gNB->harq_processes[harq_pid]->c[r][i],
	    ulsch_ue[0]->harq_processes[harq_pid]->c[r][i]^ulsch_gNB->harq_processes[harq_pid]->c[r][i]);*/
	}
    }
    if (errors_decoding > 0 && error_flag == 0) {
      n_false_positive++;
      if (n_trials==1)
	printf("\x1B[31m""[frame %d][trial %d]\tnumber of errors in decoding     = %u\n" "\x1B[0m", frame, trial, errors_decoding);
    } 
    roundStats[snrRun] += ((float)round);
    if (!crc_status) effRate[snrRun] += ((double)TBS)/(double)round;
    } // trial loop
    
    roundStats[snrRun]/=((float)n_trials);
    effRate[snrRun] /= (double)n_trials;
    
    printf("*****************************************\n");
    printf("SNR %f: n_errors (%d/%d,%d/%d,%d/%d,%d/%d) (negative CRC), false_positive %d/%d, errors_scrambling (%u/%u,%u/%u,%u/%u,%u/%u\n", SNR, n_errors[0][snrRun], round_trials[0][snrRun],n_errors[1][snrRun], round_trials[1][snrRun],n_errors[2][snrRun], round_trials[2][snrRun],n_errors[3][snrRun], round_trials[3][snrRun], n_false_positive, n_trials, errors_scrambling[0][snrRun],available_bits*n_trials,errors_scrambling[1][snrRun],available_bits*n_trials,errors_scrambling[2][snrRun],available_bits*n_trials,errors_scrambling[3][snrRun],available_bits*n_trials);
    printf("\n");
    blerStats[0][snrRun] = (double)n_errors[0][snrRun]/round_trials[0][snrRun];
    blerStats[1][snrRun] = (double)n_errors[1][snrRun]/round_trials[1][snrRun];
    blerStats[2][snrRun] = (double)n_errors[2][snrRun]/round_trials[2][snrRun];
    blerStats[3][snrRun] = (double)n_errors[3][snrRun]/round_trials[3][snrRun];

    berStats[0][snrRun] = (double)errors_scrambling[0][snrRun]/available_bits/round_trials[0][snrRun];
    berStats[1][snrRun] = (double)errors_scrambling[1][snrRun]/available_bits/round_trials[1][snrRun];
    berStats[2][snrRun] = (double)errors_scrambling[2][snrRun]/available_bits/round_trials[2][snrRun];
    berStats[3][snrRun] = (double)errors_scrambling[3][snrRun]/available_bits/round_trials[3][snrRun];
    effTP[snrRun] = effRate[snrRun]/(double)TBS*(double)100;
    printf("SNR %f: Channel BLER (%e,%e,%e,%e), Channel BER (%e,%e,%e,%e) Avg round %.2f, Eff Rate %.4f bits/slot, Eff Throughput %.2f, TBS %u bits/slot\n", 
	   SNR,
     blerStats[0][snrRun],
     blerStats[1][snrRun],
     blerStats[2][snrRun],
     blerStats[3][snrRun],
     berStats[0][snrRun],
     berStats[1][snrRun],
     berStats[2][snrRun],
     berStats[3][snrRun],
	   roundStats[snrRun],effRate[snrRun],effTP[snrRun],TBS);

    FILE *fd=fopen("nr_ulsim.log","w");
    dump_pusch_stats(fd,gNB);

    printf("*****************************************\n");
    printf("\n");
    
    if (print_perf==1) {
      printDistribution(&gNB->phy_proc_rx,table_rx,"Total PHY proc rx");
      printStatIndent(&gNB->rx_pusch_stats,"RX PUSCH time");
      printStatIndent2(&gNB->ulsch_channel_estimation_stats,"ULSCH channel estimation time");
      printStatIndent2(&gNB->ulsch_ptrs_processing_stats,"ULSCH PTRS Processing time");
      printStatIndent2(&gNB->ulsch_rbs_extraction_stats,"ULSCH rbs extraction time");
      printStatIndent2(&gNB->ulsch_channel_compensation_stats,"ULSCH channel compensation time");
      printStatIndent2(&gNB->ulsch_mrc_stats,"ULSCH mrc computation");
      printStatIndent2(&gNB->ulsch_llr_stats,"ULSCH llr computation");
      printStatIndent(&gNB->ulsch_unscrambling_stats,"ULSCH unscrambling");
      printStatIndent(&gNB->ulsch_decoding_stats,"ULSCH total decoding time");
      printStatIndent(&UE->ulsch_encoding_stats,"ULSCH total encoding time");
      printStatIndent2(&UE->ulsch_segmentation_stats,"ULSCH segmentation time");
      printStatIndent2(&UE->ulsch_ldpc_encoding_stats,"ULSCH LDPC encoder time");
      printStatIndent2(&UE->ulsch_rate_matching_stats,"ULSCH rate-matching time");
      printStatIndent2(&UE->ulsch_interleaving_stats,"ULSCH interleaving time");
      //printStatIndent2(&gNB->ulsch_deinterleaving_stats,"ULSCH deinterleaving");
      //printStatIndent2(&gNB->ulsch_rate_unmatching_stats,"ULSCH rate matching rx");
      //printStatIndent2(&gNB->ulsch_ldpc_decoding_stats,"ULSCH ldpc decoding");
      printf("\n");
    }

    if(n_trials==1)
      break;

    if ((float)n_errors[0][snrRun]/(float)n_trials <= target_error_rate) {
      printf("*************\n");
      printf("PUSCH test OK\n");
      printf("*************\n");
      break;
    }

    snrStats[snrRun] = SNR;
    snrRun++;
    n_errs = n_errors[0][snrRun];
  } // SNR loop
  printf("\n");

  printf( "Num RB:\t%d\n"
          "Num symbols:\t%d\n"
          "MCS:\t%d\n"
          "DMRS config type:\t%d\n"
          "DMRS add pos:\t%d\n"
          "PUSCH mapping type:\t%d\n"
          "DMRS length:\t%d\n"
          "DMRS CDM gr w/o data:\t%d\n",
          nb_rb,
          nb_symb_sch,
          Imcs,
          dmrs_config_type,
          add_pos,
          mapping_type,
          length_dmrs,
          num_dmrs_cdm_grps_no_data);
              
  LOG_M("ulsimStats.m","SNR",snrStats,snrRun,1,7);
  LOG_MM("ulsimStats.m","BLER_round0",blerStats[0],snrRun,1,7);
  LOG_MM("ulsimStats.m","BLER_round1",blerStats[1],snrRun,1,7);
  LOG_MM("ulsimStats.m","BLER_round2",blerStats[2],snrRun,1,7);
  LOG_MM("ulsimStats.m","BLER_round3",blerStats[3],snrRun,1,7);
  LOG_MM("ulsimStats.m","BER_round0",berStats[0],snrRun,1,7);
  LOG_MM("ulsimStats.m","BER_round1",berStats[1],snrRun,1,7);
  LOG_MM("ulsimStats.m","BER_round2",berStats[2],snrRun,1,7);
  LOG_MM("ulsimStats.m","BER_round3",berStats[3],snrRun,1,7);
  LOG_MM("ulsimStats.m","EffRate",effRate,snrRun,1,7);
  LOG_MM("ulsimStats.m","EffTP",effTP,snrRun,1,7);
  free(test_input_bit);
  free(estimated_output_bit);

  if (output_fd)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  if (scg_fd)
    fclose(scg_fd);

  return (n_errs);
}
