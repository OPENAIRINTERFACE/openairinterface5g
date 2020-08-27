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

#define inMicroS(a) (((double)(a))/(cpu_freq_GHz*1000.0))
#include "SIMULATION/LTE_PHY/common_sim.h"

//#define DEBUG_ULSIM

PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];

int sf_ahead=4 ;
int sl_ahead=0;
double cpuf;
uint8_t nfapi_mode = 0;
uint16_t NB_UE_INST = 1;
uint64_t downlink_frequency[MAX_NUM_CCs][4];


int8_t nr_mac_rrc_data_ind_ue(const module_id_t module_id, const int CC_id, const uint8_t gNB_index,
                              const int8_t channel, const uint8_t* pduP, const sdu_size_t pdu_len) { return 0; }
int generate_dlsch_header(unsigned char *mac_header,
                          unsigned char num_sdus,
                          unsigned short *sdu_lengths,
                          unsigned char *sdu_lcids,
                          unsigned char drx_cmd,
                          unsigned short timing_advance_cmd,
                          unsigned char *ue_cont_res_id,
                          unsigned char short_padding,
                          unsigned short post_padding){return 0;}
void pdcp_layer_init (void) {}
boolean_t pdcp_data_ind(
  const protocol_ctxt_t *const ctxt_pP,
  const srb_flag_t   srb_flagP,
  const MBMS_flag_t  MBMS_flagP,
  const rb_id_t      rb_idP,
  const sdu_size_t   sdu_buffer_sizeP,
  mem_block_t *const sdu_buffer_pP
) { return(false);}

void pdcp_run (const protocol_ctxt_t *const  ctxt_pP) { return;}
void nr_ip_over_LTE_DRB_preconfiguration(void){}
int rrc_init_nr_global_param(void){return(0);}


// needed for some functions
uint16_t n_rnti = 0x1234;
openair0_config_t openair0_cfg[MAX_CARDS];

int main(int argc, char **argv)
{
  char c;
  int i;
  double SNR, snr0 = -2.0, snr1 = 2.0;
  double sigma, sigma_dB;
  double snr_step = .2;
  uint8_t snr1set = 0;
  int slot = 8, frame = 0;
  FILE *output_fd = NULL;
  //uint8_t write_output_file = 0;
  int trial, n_trials = 1, n_errors = 0, n_false_positive = 0, delay = 0;
  uint8_t n_tx = 1, n_rx = 1;
  //uint8_t transmission_mode = 1;
  //uint16_t Nid_cell = 0;
  channel_desc_t *gNB2UE;
  uint8_t extended_prefix_flag = 0;
  //int8_t interf1 = -21, interf2 = -21;
  FILE *input_fd = NULL;
  SCM_t channel_model = AWGN;  //Rayleigh1_anticorr;
  uint16_t N_RB_DL = 106, N_RB_UL = 106, mu = 1;
  //unsigned char frame_type = 0;
  NR_DL_FRAME_PARMS *frame_parms;
  int loglvl = OAILOG_WARNING;
  //uint64_t SSB_positions=0x01;
  uint16_t nb_symb_sch = 12;
  int start_symbol = 2;
  uint16_t nb_rb = 50;
  uint8_t Imcs = 9;
  uint8_t precod_nbr_layers = 1;
  int gNB_id = 0;
  int ap;
  int tx_offset;
  double txlev_float;
  int32_t txlev;
  int start_rb = 0;
  int UE_id =0; // [hna] only works for UE_id = 0 because NUMBER_OF_NR_UE_MAX is set to 1 (phy_init_nr_gNB causes segmentation fault)
  float target_error_rate = 0.01;
  int print_perf = 0;
  cpuf = get_cpu_freq_GHz();
  int msg3_flag = 0;

  UE_nr_rxtx_proc_t UE_proc;
  FILE *scg_fd=NULL;

  int ibwps=24;
  int ibwp_rboffset=41;
  if ( load_configmodule(argc,argv,CONFIG_ENABLECMDLINEONLY) == 0 ) {
    exit_fun("[NR_ULSIM] Error, configuration module init failed\n");
  }

  //logInit();
  randominit(0);

  while ((c = getopt(argc, argv, "a:b:c:d:ef:g:h:i:j:kl:m:n:p:r:s:y:z:F:M:N:PR:S:L:")) != -1) {
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
	
      default:
	printf("Unsupported channel model!\n");
	exit(-1);
      }
      
      break;
      
      /*case 'i':
        interf1 = atoi(optarg);
        break;
	
	case 'j':
        interf2 = atoi(optarg);
        break;*/

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
      
      if ((n_rx == 0) || (n_rx > 2)) {
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
      
    default:
    case 'h':
      printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n", argv[0]);
      //printf("-d Use TDD\n");
      printf("-d Introduce delay in terms of number of samples\n");
      printf("-f Number of frames to simulate\n");
      printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
      printf("-h This message\n");
      //printf("-i Relative strength of first intefering eNB (in dB) - cell_id mod 3 = 1\n");
      //printf("-j Relative strength of second intefering eNB (in dB) - cell_id mod 3 = 2\n");
      printf("-s Starting SNR, runs from SNR0 to SNR0 + 10 dB if ending SNR isn't given\n");
      printf("-m MCS value\n");
      printf("-n Number of trials to simulate\n");
      printf("-p Use extended prefix mode\n");
      printf("-t Delay spread for multipath channel\n");
      //printf("-x Transmission mode (1,2,6 for the moment)\n");
      printf("-y Number of TX antennas used in eNB\n");
      printf("-z Number of RX antennas used in UE\n");
      printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
      //printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      printf("-G raw file containing RRC configuration (generated by gNB)\n");
      printf("-M Multiple SSB positions in burst\n");
      printf("-N Nid_cell\n");
      printf("-O oversampling factor (1,2,4,8,16)\n");
      printf("-R N_RB_DL\n");
      printf("-S Ending SNR, runs from SNR0 to SNR1\n");
      printf("-P Print ULSCH performances\n");
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

  gNB2UE = new_channel_desc_scm(n_tx, n_rx, channel_model,
                                61.44e6, //N_RB2sampling_rate(N_RB_DL),
                                40e6, //N_RB2channel_bandwidth(N_RB_DL),
                                0, 0, 0);

  if (gNB2UE == NULL) {
    printf("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  RC.gNB = (PHY_VARS_gNB **) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0] = malloc(sizeof(PHY_VARS_gNB));
  gNB = RC.gNB[0];
  //gNB_config = &gNB->gNB_config;

  //memset((void *)&gNB->UL_INFO,0,sizeof(gNB->UL_INFO));
  gNB->UL_INFO.rx_ind.pdu_list = (nfapi_nr_rx_data_pdu_t *)malloc(NB_UE_INST*sizeof(nfapi_nr_rx_data_pdu_t));
  gNB->UL_INFO.crc_ind.crc_list = (nfapi_nr_crc_t *)malloc(NB_UE_INST*sizeof(nfapi_nr_crc_t));
  gNB->UL_INFO.rx_ind.number_of_pdus = 0;
  gNB->UL_INFO.crc_ind.number_crcs = 0;
  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)


  frame_parms->nb_antennas_tx = n_tx;
  frame_parms->nb_antennas_rx = n_rx;
  frame_parms->N_RB_DL = N_RB_DL;
  frame_parms->N_RB_UL = N_RB_UL;
  frame_parms->Ncp = extended_prefix_flag ? EXTENDED : NORMAL;


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
  NR_CellGroupConfig_t *secondaryCellGroup=calloc(1,sizeof(*secondaryCellGroup));
  prepare_scc(rrc.carrier.servingcellconfigcommon);
  uint64_t ssb_bitmap;
  fill_scc(rrc.carrier.servingcellconfigcommon,&ssb_bitmap,N_RB_DL,N_RB_DL,mu,mu);

  fill_default_secondaryCellGroup(scc,
				  secondaryCellGroup,
				  0,
				  1,
				  n_tx,
				  0);
  fix_scc(scc,ssb_bitmap);

  xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void*)secondaryCellGroup);

  AssertFatal((gNB->if_inst         = NR_IF_Module_init(0))!=NULL,"Cannot register interface");

  gNB->if_inst->NR_PHY_config_req      = nr_phy_config_request;
  // common configuration
  rrc_mac_config_req_gNB(0,0,1,scc,0,0,NULL);
  // UE dedicated configuration
  rrc_mac_config_req_gNB(0,0,1,NULL,1,secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity,secondaryCellGroup);
  phy_init_nr_gNB(gNB,0,0);
  N_RB_DL = gNB->frame_parms.N_RB_DL;



  NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[0];

  //crcTableInit();

  //nr_phy_config_request_sim(gNB, N_RB_DL, N_RB_UL, mu, Nid_cell, SSB_positions);


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
  
  nr_l2_init_ue(NULL);
  NR_UE_MAC_INST_t* UE_mac = get_mac_inst(0);
  
  UE->if_inst = nr_ue_if_module_init(0);
  UE->if_inst->scheduled_response = nr_ue_scheduled_response;
  UE->if_inst->phy_config_request = nr_ue_phy_config_request;
  UE->if_inst->dl_indication = nr_ue_dl_indication;
  UE->if_inst->ul_indication = nr_ue_ul_indication;
  
  UE_mac->if_module = nr_ue_if_module_init(0);

  //Configure UE
  rrc.carrier.MIB = (uint8_t*) malloc(4);
  rrc.carrier.sizeof_MIB = do_MIB_NR(&rrc,0);

  nr_rrc_mac_config_req_ue(0,0,0,rrc.carrier.mib.message.choice.mib,secondaryCellGroup);

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
  uint32_t errors_scrambling = 0;

  test_input_bit       = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
  estimated_output_bit = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);

  nr_scheduled_response_t scheduled_response;
  fapi_nr_ul_config_request_t ul_config;

  unsigned int TBS;
  uint16_t number_dmrs_symbols = 0;
  unsigned int available_bits;
  uint8_t nb_re_dmrs;
  unsigned char mod_order;
  uint16_t code_rate;
  uint8_t ptrs_mcs1 = 2;
  uint8_t ptrs_mcs2 = 4;
  uint8_t ptrs_mcs3 = 10;
  uint16_t n_rb0 = 25;
  uint16_t n_rb1 = 75;
  uint8_t mcs_table = 0;
  uint16_t pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA; // | PUSCH_PDU_BITMAP_PUSCH_PTRS;

  uint8_t length_dmrs = pusch_len1; // [hna] remove dmrs struct
  uint16_t l_prime_mask = get_l_prime(nb_symb_sch, typeB, pusch_dmrs_pos0, length_dmrs);  // [hna] remove dmrs struct
  uint8_t ptrs_time_density = get_L_ptrs(ptrs_mcs1, ptrs_mcs2, ptrs_mcs3, Imcs, mcs_table);
  uint8_t ptrs_freq_density = get_K_ptrs(n_rb0, n_rb1, nb_rb);

  if(1<<ptrs_time_density >= nb_symb_sch)
    pdu_bit_map &= ~PUSCH_PDU_BITMAP_PUSCH_PTRS; // disable PUSCH PTRS

  for (i = 0; i < nb_symb_sch; i++) {
    number_dmrs_symbols += (l_prime_mask >> i) & 0x01;
  }

  mod_order      = nr_get_Qm_ul(Imcs, 0);
  code_rate      = nr_get_code_rate_ul(Imcs, 0);

  printf("\n");

  for (int i=0;i<16;i++) printf("%f\n",gaussdouble(0.0,1.0));
  for (SNR = snr0; SNR < snr1; SNR += snr_step) {

      varArray_t *table_rx=initVarArray(1000,sizeof(double));
      reset_meas(&gNB->phy_proc_rx);
      reset_meas(&gNB->ulsch_decoding_stats);
      reset_meas(&gNB->ulsch_deinterleaving_stats);
      reset_meas(&gNB->ulsch_rate_unmatching_stats);
      reset_meas(&gNB->ulsch_ldpc_decoding_stats);
      reset_meas(&gNB->ulsch_unscrambling_stats);
      reset_meas(&gNB->ulsch_channel_estimation_stats);
      reset_meas(&gNB->ulsch_llr_stats);
      reset_meas(&gNB->ulsch_channel_compensation_stats);
      reset_meas(&gNB->ulsch_rbs_extraction_stats);

      UE_proc.nr_tti_tx = slot;
      UE_proc.frame_tx = frame;

      // --------- setting parameters for gNB --------
      /*
      rel15_ul->rnti                           = n_rnti;
      rel15_ul->ulsch_pdu_rel15.start_rb       = start_rb;
      rel15_ul->ulsch_pdu_rel15.number_rbs     = nb_rb;
      rel15_ul->ulsch_pdu_rel15.start_symbol   = start_symbol;
      rel15_ul->ulsch_pdu_rel15.number_symbols = nb_symb_sch;
      rel15_ul->ulsch_pdu_rel15.length_dmrs    = length_dmrs;
      rel15_ul->ulsch_pdu_rel15.Qm             = mod_order;
      rel15_ul->ulsch_pdu_rel15.mcs            = Imcs;
      rel15_ul->ulsch_pdu_rel15.rv             = 0;
      rel15_ul->ulsch_pdu_rel15.ndi            = 0;
      rel15_ul->ulsch_pdu_rel15.n_layers       = precod_nbr_layers;
      rel15_ul->ulsch_pdu_rel15.R              = code_rate; 
      ///////////////////////////////////////////////////
      */

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
	start_rb += (ibwp_start - abwp_start);
	printf("msg3: ibwp_size %d, abwp_size %d, ibwp_start %d, abwp_start %d\n",
	       ibwp_size,abwp_size,ibwp_start,abwp_start);
      }
      else {
	pusch_pdu->bwp_start = abwp_start;
	pusch_pdu->bwp_size = abwp_size;
      }

      pusch_pdu->pdu_bit_map = pdu_bit_map;
      pusch_pdu->rnti = n_rnti;
      pusch_pdu->mcs_index = Imcs;
      pusch_pdu->mcs_table = mcs_table;
      pusch_pdu->target_code_rate = code_rate;
      pusch_pdu->qam_mod_order = mod_order;
      pusch_pdu->transform_precoding = 1;
      pusch_pdu->data_scrambling_id = *scc->physCellId;
      pusch_pdu->nrOfLayers = 1;
      pusch_pdu->ul_dmrs_symb_pos = l_prime_mask << start_symbol;
      pusch_pdu->dmrs_config_type = 0;
      pusch_pdu->ul_dmrs_scrambling_id =  *scc->physCellId;
      pusch_pdu->scid = 0;
      pusch_pdu->dmrs_ports = 1;
      pusch_pdu->num_dmrs_cdm_grps_no_data = 1;
      pusch_pdu->resource_alloc = 1; 
      pusch_pdu->rb_start = start_rb;
      pusch_pdu->rb_size = nb_rb;
      pusch_pdu->vrb_to_prb_mapping = 0;
      pusch_pdu->frequency_hopping = 0;
      pusch_pdu->uplink_frequency_shift_7p5khz = 0;
      pusch_pdu->start_symbol_index = start_symbol;
      pusch_pdu->nr_of_symbols = nb_symb_sch;
      pusch_pdu->pusch_data.rv_index = 0;
      pusch_pdu->pusch_data.harq_process_id = 0;
      pusch_pdu->pusch_data.new_data_indicator = 0;
      pusch_pdu->pusch_data.num_cb = 0;
      pusch_pdu->pusch_ptrs.ptrs_time_density = ptrs_time_density;
      pusch_pdu->pusch_ptrs.ptrs_freq_density = ptrs_freq_density;
      pusch_pdu->pusch_ptrs.ptrs_ports_list   = (nfapi_nr_ptrs_ports_t *) malloc(2*sizeof(nfapi_nr_ptrs_ports_t));
      pusch_pdu->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset = 0;

      // prepare ULSCH/PUSCH reception
      nr_schedule_response(Sched_INFO);

      // --------- setting parameters for UE --------

      scheduled_response.module_id = 0;
      scheduled_response.CC_id = 0;
      scheduled_response.frame = frame;
      scheduled_response.slot = slot;
      scheduled_response.dl_config = NULL;
      scheduled_response.ul_config = &ul_config;
      scheduled_response.tx_request = (fapi_nr_tx_request_t *) malloc(sizeof(fapi_nr_tx_request_t));
      scheduled_response.tx_request->tx_request_body = (fapi_nr_tx_request_body_t *) malloc(sizeof(fapi_nr_tx_request_body_t));

      ul_config.slot = slot;
      ul_config.number_pdus = 1;
      ul_config.ul_config_list[0].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
      ul_config.ul_config_list[0].pusch_config_pdu.rnti = n_rnti;
      ul_config.ul_config_list[0].pusch_config_pdu.pdu_bit_map = pdu_bit_map;
      ul_config.ul_config_list[0].pusch_config_pdu.rb_size = nb_rb;
      ul_config.ul_config_list[0].pusch_config_pdu.rb_start = start_rb;
      ul_config.ul_config_list[0].pusch_config_pdu.nr_of_symbols = nb_symb_sch;
      ul_config.ul_config_list[0].pusch_config_pdu.start_symbol_index = start_symbol;
      ul_config.ul_config_list[0].pusch_config_pdu.ul_dmrs_symb_pos = l_prime_mask << start_symbol;
      ul_config.ul_config_list[0].pusch_config_pdu.dmrs_config_type = 0;
      ul_config.ul_config_list[0].pusch_config_pdu.mcs_index = Imcs;
      ul_config.ul_config_list[0].pusch_config_pdu.mcs_table = mcs_table;
      ul_config.ul_config_list[0].pusch_config_pdu.num_dmrs_cdm_grps_no_data = 1;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_data.new_data_indicator = 0;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_data.rv_index = 0;
      ul_config.ul_config_list[0].pusch_config_pdu.nrOfLayers = precod_nbr_layers;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_data.harq_process_id = harq_pid;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_ptrs.ptrs_time_density = ptrs_time_density;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_ptrs.ptrs_freq_density = ptrs_freq_density;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_ptrs.ptrs_ports_list   = (nfapi_nr_ue_ptrs_ports_t *) malloc(2*sizeof(nfapi_nr_ue_ptrs_ports_t));
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset = 0;
      //there are plenty of other parameters that we don't seem to be using for now. e.g.
      ul_config.ul_config_list[0].pusch_config_pdu.absolute_delta_PUSCH = 0;

      nb_re_dmrs = ((ul_config.ul_config_list[0].pusch_config_pdu.dmrs_config_type == pusch_dmrs_type1) ? 6 : 4);

      available_bits = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, number_dmrs_symbols, mod_order, 1);
      TBS            = nr_compute_tbs(mod_order, code_rate, nb_rb, nb_symb_sch, nb_re_dmrs * number_dmrs_symbols, 0, 0, precod_nbr_layers);
      pusch_pdu->pusch_data.tb_size = TBS>>3;
      ul_config.ul_config_list[0].pusch_config_pdu.pusch_data.tb_size = TBS;

      nr_fill_ulsch(gNB,frame,slot,pusch_pdu);
      int slot_offset = frame_parms->get_samples_slot_timestamp(slot,frame_parms,0);// - (int)(800*factor);
      int slot_length = slot_offset - frame_parms->get_samples_slot_timestamp(slot-1,frame_parms,0);

      for (int i=0;i<(TBS>>3);i++) ulsch_ue[0]->harq_processes[harq_pid]->a[i]=i&0xff;
      double scale = 1;

      if (input_fd == NULL) {

        if (SNR==snr0) { 
	  // set FAPI parameters for UE, put them in the scheduled response and call
	  nr_ue_scheduled_response(&scheduled_response);
	  
	  
	  /////////////////////////phy_procedures_nr_ue_TX///////////////////////
	  ///////////
	  
	  phy_procedures_nrUE_TX(UE, &UE_proc, gNB_id, 0);
	  
	  
	  if (n_trials==1) {
	    LOG_M("txsig0.m","txs0", UE->common_vars.txdata[0],frame_parms->samples_per_subframe*10,1,1);
	    LOG_M("txsig0F.m","txs0F", UE->common_vars.txdataF[0],frame_parms->ofdm_symbol_size*14,1,1);
	  }
	  ///////////
	  ////////////////////////////////////////////////////
	  tx_offset = frame_parms->get_samples_slot_timestamp(slot,frame_parms,0);
	  
	  txlev = signal_energy(&UE->common_vars.txdata[0][tx_offset + 5*frame_parms->ofdm_symbol_size + 4*frame_parms->nb_prefix_samples + frame_parms->nb_prefix_samples0],
				frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples);
	  
	  txlev_float = (double)txlev/scale; // output of signal_energy is fixed point representation
	  
	  
	  //AWGN
	}
      }	
      else n_trials = 1;


      sigma_dB = 10*log10(txlev_float)-SNR;
      sigma    = pow(10,sigma_dB/10);
      printf("txlev_float %f, sigma_dB %f\n",10*log10(txlev_float),sigma_dB);

      n_errors = 0;
      n_false_positive = 0;

      errors_scrambling  = 0;
      errors_decoding    = 0;

      int error_flag;
      for (trial = 0; trial < n_trials; trial++) {
	
	error_flag = 0;
	  //----------------------------------------------------------
	  //------------------------ add noise -----------------------
	  //----------------------------------------------------------
	if (input_fd == NULL ) {
	  for (i=0; i<slot_length; i++) {
	    for (ap=0; ap<frame_parms->nb_antennas_rx; ap++) {
	      ((int16_t*) &gNB->common_vars.rxdata[ap][slot_offset])[(2*i) + (delay*2)]   = (int16_t)((double)(((int16_t *)&UE->common_vars.txdata[ap][slot_offset])[(i<<1)])/sqrt(scale)   + (sqrt(sigma/2)*gaussdouble(0.0,1.0))); // convert to fixed point
	      ((int16_t*) &gNB->common_vars.rxdata[ap][slot_offset])[(2*i)+1 + (delay*2)]   = (int16_t)((double)(((int16_t *)&UE->common_vars.txdata[ap][slot_offset])[(i<<1)+1])/sqrt(scale) + (sqrt(sigma/2)*gaussdouble(0.0,1.0)));
	    }
	  }
	}
	else {
	  AssertFatal(frame_parms->nb_antennas_rx == 1, "nb_ant != 1\n");
	  // 800 samples is N_TA_OFFSET for FR1 @ 30.72 Ms/s,
	  AssertFatal(frame_parms->subcarrier_spacing==30000,"only 30 kHz for file input for now (%d)\n",frame_parms->subcarrier_spacing);
	  double factor = 1;
	  if (openair0_cfg[0].threequarter_fs== 1) factor =.75;
	  int ta_offset=1600;
	  if (N_RB_DL <217) ta_offset=800;
	  else if (N_RB_DL < 106) ta_offset = 400;

	  fread((void*)&gNB->common_vars.rxdata[0][slot_offset],
		sizeof(int16_t),
		slot_length<<1,
		input_fd);
	  for (int i=0;i<16;i+=2) printf("slot_offset %d : %d,%d\n",
					 slot_offset,
					 ((int16_t*)&gNB->common_vars.rxdata[0][slot_offset])[i],
					 ((int16_t*)&gNB->common_vars.rxdata[0][slot_offset])[1+i]);
	  fclose(input_fd);
	}
	////////////////////////////////////////////////////////////
	
	//----------------------------------------------------------
	//------------------- gNB phy procedures -------------------
	//----------------------------------------------------------
	gNB->UL_INFO.rx_ind.number_of_pdus = 0;
	gNB->UL_INFO.crc_ind.number_crcs = 0;

        start_meas(&gNB->phy_proc_rx);
        phy_procedures_gNB_common_RX(gNB, frame, slot);

	if (n_trials==1) {
	  LOG_M("rxsig0.m","rx0",&gNB->common_vars.rxdata[0][0],frame_parms->samples_per_subframe*10,1,1);

	  LOG_M("rxsigF0.m","rxsF0",gNB->common_vars.rxdataF[0]+start_symbol*frame_parms->ofdm_symbol_size,nb_symb_sch*frame_parms->ofdm_symbol_size,1,1);

	}
        phy_procedures_gNB_uespec_RX(gNB, frame, slot);
	if (n_trials == 1) { 
	  LOG_M("rxsigF0_ext.m","rxsF0_ext",
		&gNB->pusch_vars[0]->rxdataF_ext[0][(start_symbol+1)*NR_NB_SC_PER_RB * pusch_pdu->rb_size],(nb_symb_sch-1)*NR_NB_SC_PER_RB * pusch_pdu->rb_size,1,1);
	  LOG_M("chestF0.m","chF0",
		&gNB->pusch_vars[0]->ul_ch_estimates[0][start_symbol*frame_parms->ofdm_symbol_size],frame_parms->ofdm_symbol_size,1,1);
	  LOG_M("chestF0_ext.m","chF0_ext",
		&gNB->pusch_vars[0]->ul_ch_estimates_ext[0][(start_symbol+1)*NR_NB_SC_PER_RB * pusch_pdu->rb_size],(nb_symb_sch-1)*NR_NB_SC_PER_RB * pusch_pdu->rb_size,1,1);
	  LOG_M("rxsigF0_comp.m","rxsF0_comp",
		&gNB->pusch_vars[0]->rxdataF_comp[0][(start_symbol+1)*NR_NB_SC_PER_RB * pusch_pdu->rb_size],(nb_symb_sch-1)*NR_NB_SC_PER_RB * pusch_pdu->rb_size,1,1);
	}
        start_meas(&gNB->phy_proc_rx);
        ////////////////////////////////////////////////////////////

	if (gNB->ulsch[0][0]->last_iteration_cnt >= 
	    gNB->ulsch[0][0]->max_ldpc_iterations+1) {
	  error_flag = 1; 
	  n_errors++;
	}
	
        //----------------------------------------------------------
        //----------------- count and print errors -----------------
        //----------------------------------------------------------

        for (i = 0; i < available_bits; i++) {

          if(((ulsch_ue[0]->g[i] == 0) && (gNB->pusch_vars[UE_id]->llr[i] <= 0)) ||
             ((ulsch_ue[0]->g[i] == 1) && (gNB->pusch_vars[UE_id]->llr[i] >= 0)))
          {
            /*if(errors_scrambling == 0)
              printf("\x1B[34m" "[frame %d][trial %d]\t1st bit in error in unscrambling = %d\n" "\x1B[0m", frame, trial, i);*/
            errors_scrambling++;
          }
        }

        if (errors_scrambling > 0) {
	  if (n_trials==1)
	    printf("\x1B[31m""[frame %d][trial %d]\tnumber of errors in unscrambling = %u\n" "\x1B[0m", frame, trial, errors_scrambling);
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
	      printf("r %d: in[%d] %x, out[%d] %x (%x)\n",r,
		     i,ulsch_ue[0]->harq_processes[harq_pid]->c[r][i],
		     i,ulsch_gNB->harq_processes[harq_pid]->c[r][i],
		     ulsch_ue[0]->harq_processes[harq_pid]->c[r][i]^ulsch_gNB->harq_processes[harq_pid]->c[r][i]);
	    }
	}
        if (errors_decoding > 0 && error_flag == 0) {
          n_false_positive++;
	  if (n_trials==1)
	    printf("\x1B[31m""[frame %d][trial %d]\tnumber of errors in decoding     = %u\n" "\x1B[0m", frame, trial, errors_decoding);
        } 
      } // trial loop

      printf("*****************************************\n");
      printf("SNR %f: n_errors (negative CRC) = %d/%d, false_positive %d/%d, errors_scrambling %u/%u\n", SNR, n_errors, n_trials, n_false_positive, n_trials, errors_scrambling, available_bits*n_trials);
      printf("\n");
      printf("SNR %f: Channel BLER %e, Channel BER %e\n", SNR,(double)n_errors/n_trials,(double)errors_scrambling/available_bits/n_trials);
      printf("*****************************************\n");
      printf("\n");
 
      if (print_perf==1) {
        printDistribution(&gNB->phy_proc_rx,table_rx,"Total PHY proc rx");
        printStatIndent(&gNB->ulsch_channel_estimation_stats,"ULSCH channel estimation time");
        printStatIndent(&gNB->ulsch_rbs_extraction_stats,"ULSCH rbs extraction time");
        printStatIndent(&gNB->ulsch_channel_compensation_stats,"ULSCH channel compensation time");
        printStatIndent(&gNB->ulsch_llr_stats,"ULSCH llr computation");
        printStatIndent(&gNB->ulsch_unscrambling_stats,"ULSCH unscrambling");
        printStatIndent(&gNB->ulsch_decoding_stats,"ULSCH total decoding time");
        printStatIndent2(&gNB->ulsch_deinterleaving_stats,"ULSCH deinterleaving");
        printStatIndent2(&gNB->ulsch_rate_unmatching_stats,"ULSCH rate matching rx");
        printStatIndent2(&gNB->ulsch_ldpc_decoding_stats,"ULSCH ldpc decoding");
        printf("\n");
      }

      if(n_trials==1)
	break;

      if ((float)n_errors/(float)n_trials <= target_error_rate) {
	printf("*************\n");
	printf("PUSCH test OK\n");
	printf("*************\n");
	break;
      }
      
  } // SNR loop
  printf("\n");

  free(test_input_bit);
  free(estimated_output_bit);

  if (output_fd)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  if (scg_fd)
    fclose(scg_fd);

  return (n_errors);
}
