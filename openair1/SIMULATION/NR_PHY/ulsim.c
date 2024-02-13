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
#include "common/utils/nr/nr_common.h"
#include "common/utils/var_array.h"
#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/phy_vars_nr_ue.h"
#include "PHY/types.h"
#include "PHY/INIT/nr_phy_init.h"
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
#include "openair2/RRC/NR/nr_rrc_config.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "common/utils/threadPool/thread-pool.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#define inMicroS(a) (((double)(a))/(get_cpu_freq_GHz()*1000.0))
#include "SIMULATION/LTE_PHY/common_sim.h"

#include <openair2/RRC/LTE/rrc_vars.h>

#include <executables/softmodem-common.h>
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"
#include <openair3/ocp-gtpu/gtp_itf.h>
#include "executables/nr-uesoftmodem.h"
//#define DEBUG_ULSIM

const char *__asan_default_options()
{
  /* don't do leak checking in nr_ulsim, not finished yet */
  return "detect_leaks=0";
}
PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;
char *uecap_file;
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];

uint16_t sf_ahead=4 ;
int slot_ahead=6 ;
uint16_t sl_ahead=0;
double cpuf;
//uint8_t nfapi_mode = 0;
uint64_t downlink_frequency[MAX_NUM_CCs][4];
THREAD_STRUCT thread_struct;
nfapi_ue_release_request_body_t release_rntis;

//Fixme: Uniq dirty DU instance, by global var, datamodel need better management
instance_t DUuniqInstance=0;
instance_t CUuniqInstance=0;

void nr_derive_key_ng_ran_star(uint16_t pci, uint64_t nr_arfcn_dl, const uint8_t key[32], uint8_t *key_ng_ran_star)
{
}

extern void fix_scd(NR_ServingCellConfig_t *scd);// forward declaration

void e1_bearer_context_setup(const e1ap_bearer_setup_req_t *req) { abort(); }
void e1_bearer_context_modif(const e1ap_bearer_setup_req_t *req) { abort(); }
void e1_bearer_release_cmd(const e1ap_bearer_release_cmd_t *cmd) { abort(); }

int8_t nr_rrc_RA_succeeded(const module_id_t mod_id, const uint8_t gNB_index) {
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

nr_bler_struct nr_bler_data[NR_NUM_MCS];

void nr_derive_key(int alg_type, uint8_t alg_id, const uint8_t key[32], uint8_t out[16])
{
  (void)alg_type;
}

void processSlotTX(void *arg) {}

nrUE_params_t nrUE_params;

nrUE_params_t *get_nrUE_params(void) {
  return &nrUE_params;
}
// needed for some functions
uint16_t n_rnti = 0x1234;
openair0_config_t openair0_cfg[MAX_CARDS];

channel_desc_t *UE2gNB[NUMBER_OF_UE_MAX][NUMBER_OF_gNB_MAX];
int NB_UE_INST = 1;

configmodule_interface_t *uniqCfg = NULL;
int main(int argc, char *argv[])
{
  FILE *csv_file = NULL;
  char *filename_csv = NULL;
  char c;
  int i;
  double SNR, snr0 = -2.0, snr1 = 2.0;
  double sigma, sigma_dB;
  double snr_step = .2;
  uint8_t snr1set = 0;
  int slot = 8, frame = 1;
  int do_SRS = 0;
  FILE *output_fd = NULL;
  double **s_re,**s_im,**r_re,**r_im;
  //uint8_t write_output_file = 0;
  int trial, n_trials = 1, n_false_positive = 0, delay = 0;
  double maxDoppler = 0.0;
  uint8_t n_tx = 1, n_rx = 1;
  channel_desc_t *UE2gNB;
  uint8_t extended_prefix_flag = 0;
  //int8_t interf1 = -21, interf2 = -21;
  FILE *input_fd = NULL;
  SCM_t channel_model = AWGN;  //Rayleigh1_anticorr;
  corr_level_t corr_level = CORR_LEVEL_LOW;
  uint16_t N_RB_DL = 106, N_RB_UL = 106, mu = 1;

  //unsigned char frame_type = 0;
  NR_DL_FRAME_PARMS *frame_parms;
  int loglvl = OAILOG_WARNING;
  uint16_t nb_symb_sch = 12;
  int start_symbol = 0;
  uint16_t nb_rb = 50;
  int Imcs = 9;
  uint8_t precod_nbr_layers = 1;
  int tx_offset;
  int32_t txlev_sum = 0, atxlev[4];
  int start_rb = 0;
  int UE_id = 0;
  int print_perf = 0;
  cpuf = get_cpu_freq_GHz();
  int msg3_flag = 0;
  int rv_index = 0;
  float roundStats;
  double effRate;
  double effTP;
  float eff_tp_check = 100;
  int ldpc_offload_flag = 0;
  uint8_t max_rounds = 4;
  int chest_type[2] = {0};
  int enable_ptrs = 0;
  int modify_dmrs = 0;
  /* L_PTRS = ptrs_arg[0], K_PTRS = ptrs_arg[1] */
  int ptrs_arg[2] = {-1,-1};// Invalid values
  int dmrs_arg[4] = {-1,-1,-1,-1};// Invalid values
  uint16_t ptrsSymPos = 0;
  uint16_t ptrsSymbPerSlot = 0;
  uint16_t ptrsRePerSymb = 0;

  uint8_t transform_precoding = transformPrecoder_disabled; // 0 - ENABLE, 1 - DISABLE
  uint8_t num_dmrs_cdm_grps_no_data = 1;
  uint8_t mcs_table = 0;
  int ilbrm = 0;

  UE_nr_rxtx_proc_t UE_proc;
  FILE *scg_fd=NULL;
  int file_offset = 0;

  double DS_TDL = .03;
  int ibwps=24;
  int ibwp_rboffset=41;
  int params_from_file = 0;
  int threadCnt=0;
  int max_ldpc_iterations = 5;
  if ((uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY)) == 0) {
    exit_fun("[NR_ULSIM] Error, configuration module init failed\n");
  }
  int ul_proc_error = 0; // uplink processing checking status flag
  //logInit();
  randominit(0);

  /* initialize the sin-cos table */
  InitSinLUT();

  while ((c = getopt(argc, argv, "a:b:c:d:ef:g:h:i:k:m:n:op:q:r:s:t:u:v:w:y:z:C:F:G:H:I:M:N:PR:S:T:U:L:ZW:E:X:")) != -1) {
    printf("handling optarg %c\n",c);
    switch (c) {

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
          channel_model = TDL_A;
          DS_TDL = 0.030; // 30 ns
          printf("Channel model: TDLA30\n");
          break;
        case 'B':
          channel_model = TDL_B;
          DS_TDL = 0.100; // 100ns
          printf("Channel model: TDLB100\n");
          break;
        case 'C':
          channel_model = TDL_C;
          DS_TDL = 0.300; // 300 ns
          printf("Channel model: TDLC300\n");
          break;
        default:
          printf("Unsupported channel model!\n");
          exit(-1);
      }

      if (optarg[1] == ',') {
        switch (optarg[2]) {
          case 'l':
            corr_level = CORR_LEVEL_LOW;
            break;
          case 'm':
            corr_level = CORR_LEVEL_MEDIUM;
            break;
          case 'h':
            corr_level = CORR_LEVEL_HIGH;
            break;
          default:
            printf("Invalid correlation level!\n");
        }
      }

      if (optarg[3] == ',') {
        maxDoppler = atoi(&optarg[4]);
        printf("Maximum Doppler Frequency: %.0f Hz\n", maxDoppler);
      }
      break;

    case 'i':
      i=0;
      do {
        chest_type[i>>1] = atoi(&optarg[i]);
        i+=2;
      } while (optarg[i-1] == ',');
      break;
	
    case 'k':
      printf("Setting threequarter_fs_flag\n");
      openair0_cfg[0].threequarter_fs= 1;
      break;

    case 'm':
      Imcs = atoi(optarg);
      break;

    case 'W':
      precod_nbr_layers = atoi(optarg);
      break;

    case 'n':
      n_trials = atoi(optarg);
      break;

    case 'o':
      ldpc_offload_flag = 1;
      break;

    case 'p':
      extended_prefix_flag = 1;
      break;

    case 'q':
      mcs_table = atoi(optarg);
      break;

    case 'r':
      nb_rb = atoi(optarg);
      break;

    case 's':
      snr0 = atof(optarg);
      printf("Setting SNR0 to %f\n", snr0);
      break;

    case 'C':
      threadCnt = atoi(optarg);
      break;

    case 'u':
      mu = atoi(optarg);
      break;

    case 'v':
      max_rounds = atoi(optarg);
      AssertFatal(max_rounds > 0 && max_rounds < 16, "Unsupported number of rounds %d, should be in [1,16]\n", max_rounds);
      break;

    case 'w':
      start_rb = atoi(optarg);
      break;

    case 't':
      eff_tp_check = atof(optarg);
      break;

    case 'y':
      n_tx = atoi(optarg);
      if ((n_tx == 0) || (n_tx > 4)) {
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

    case 'I':
      max_ldpc_iterations = atoi(optarg);
      break;

    case 'M':
      ilbrm = atoi(optarg);
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
      i=0;
      do {
        ptrs_arg[i>>1] = atoi(&optarg[i]);
        i+=2;
      } while (optarg[i-1] == ',');
      break;

    case 'U':
      modify_dmrs = 1;
      i=0;
      do {
        dmrs_arg[i>>1] = atoi(&optarg[i]);
        i+=2;
      } while (optarg[i-1] == ',');
      break;

    case 'Q':
      params_from_file = 1;
      break;

    case 'X' :
      filename_csv = strdup(optarg);
      AssertFatal(filename_csv != NULL, "strdup() error: errno %d\n", errno);
      break;

    case 'Z':
      transform_precoding = transformPrecoder_enabled;
      num_dmrs_cdm_grps_no_data = 2;
      mcs_table = 3;
      printf("NOTE: TRANSFORM PRECODING (SC-FDMA) is ENABLED in UPLINK (0 - ENABLE, 1 - DISABLE) : %d \n", transform_precoding);
      break;

    case 'E':
      do_SRS = atoi(optarg);
      if (do_SRS == 0) {
        printf("SRS disabled\n");
      } else if (do_SRS == 1) {
        printf("SRS enabled\n");
      } else {
        printf("Invalid SRS option. SRS disabled.\n");
        do_SRS = 0;
      }
      break;

    default:
    case 'h':
      printf("%s -h(elp)\n", argv[0]);
      printf("-a ULSCH starting symbol\n");
      printf("-b ULSCH number of symbols\n");
      printf("-c RNTI\n");
      printf("-d Introduce delay in terms of number of samples\n");
      printf("-e To simulate MSG3 configuration\n");
      printf("-f Input file to read from\n");// file not used in the code
      printf("-g Channel model configuration. Arguments list: Number of arguments = 3, {Channel model: [A] TDLA30, [B] TDLB100, [C] TDLC300}, {Correlation: [l] Low, [m] Medium, [h] High}, {Maximum Doppler shift} e.g. -g A,l,10\n");
      printf("-h This message\n");
      printf("-i Change channel estimation technique. Arguments list: Number of arguments=2, Frequency domain {0:Linear interpolation, 1:PRB based averaging}, Time domain {0:Estimates of last DMRS symbol, 1:Average of DMRS symbols}. e.g. -i 1,0\n");
      printf("-k 3/4 sampling\n");
      printf("-m MCS value\n");
      printf("-n Number of trials to simulate\n");
      printf("-o ldpc offload flag\n");
      printf("-p Use extended prefix mode\n");
      printf("-q MCS table\n");
      printf("-r Number of allocated resource blocks for PUSCH\n");
      printf("-s Starting SNR, runs from SNR0 to SNR0 + 10 dB if ending SNR isn't given\n");
      printf("-S Ending SNR, runs from SNR0 to SNR1\n");
      printf("-t Acceptable effective throughput (in percentage)\n");
      printf("-u Set the numerology\n");
      printf("-v Set the max rounds\n");
      printf("-w Start PRB for PUSCH\n");
      printf("-y Number of TX antennas used at UE\n");
      printf("-z Number of RX antennas used at gNB\n");
      printf("-C Specify the number of threads for the simulation\n");
      printf("-E {SRS: [0] Disabled, [1] Enabled} e.g. -E 1\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      printf("-G Offset of samples to read from file (0 default)\n");
      printf("-H Slot number\n");
      printf("-I Maximum LDPC decoder iterations\n");
      printf("-L <log level, 0(errors), 1(warning), 2(info) 3(debug) 4 (trace)>\n");
      printf("-M Use limited buffer rate-matching\n");
      printf("-P Print ULSCH performances\n");
      printf("-Q If -F used, read parameters from file\n");
      printf("-R Maximum number of available resorce blocks (N_RB_DL)\n");
      printf("-T Enable PTRS, arguments list: Number of arguments=2 L_PTRS{0,1,2} K_PTRS{2,4}, e.g. -T 0,2 \n");
      printf("-U Change DMRS Config, arguments list: Number of arguments=4, DMRS Mapping Type{0=A,1=B}, DMRS AddPos{0:3}, DMRS Config Type{1,2}, Number of CDM groups without data{1,2,3} e.g. -U 0,2,0,1 \n");
      printf("-W Num of layer for PUSCH\n");
      printf("-X Output filename (.csv format) for stats\n");
      printf("-Z If -Z is used, SC-FDMA or transform precoding is enabled in Uplink \n");
      exit(-1);
      break;

    }
  }
  
  logInit();
  set_glog(loglvl);

  get_softmodem_params()->phy_test = 1;
  get_softmodem_params()->do_ra = 0;
  get_softmodem_params()->usim_test = 1;

  if (snr1set == 0)
    snr1 = snr0 + 10;

  double sampling_frequency, tx_bandwidth, rx_bandwidth;
  uint32_t samples;
  get_samplerate_and_bw(mu,
                        N_RB_DL,
                        openair0_cfg[0].threequarter_fs,
                        &sampling_frequency,
                        &samples,
                        &tx_bandwidth,
                        &rx_bandwidth);

  RC.gNB = (PHY_VARS_gNB **) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0] = calloc(1,sizeof(PHY_VARS_gNB));
  gNB = RC.gNB[0];
  gNB->ofdm_offset_divisor = UINT_MAX;
  gNB->num_pusch_symbols_per_thread = 1;

  initFloatingCoresTpool(threadCnt, &gNB->threadPool, false, "gNB-tpool");
  initNotifiedFIFO(&gNB->respDecode);

  initNotifiedFIFO(&gNB->respPuschSymb);
  initNotifiedFIFO(&gNB->L1_tx_free);
  initNotifiedFIFO(&gNB->L1_tx_filled);
  initNotifiedFIFO(&gNB->L1_tx_out);
  notifiedFIFO_elt_t *msgL1Tx = newNotifiedFIFO_elt(sizeof(processingData_L1tx_t), 0, &gNB->L1_tx_free, NULL);
  processingData_L1tx_t *msgDataTx = (processingData_L1tx_t *)NotifiedFifoData(msgL1Tx);
  msgDataTx->slot = -1;
  gNB->msgDataTx = msgDataTx;
  //gNB_config = &gNB->gNB_config;

  //memset((void *)&gNB->UL_INFO,0,sizeof(gNB->UL_INFO));
  gNB->UL_INFO.rx_ind.pdu_list = (nfapi_nr_rx_data_pdu_t *)malloc(NB_UE_INST*sizeof(nfapi_nr_rx_data_pdu_t));
  gNB->UL_INFO.crc_ind.crc_list = (nfapi_nr_crc_t *)malloc(NB_UE_INST*sizeof(nfapi_nr_crc_t));
  gNB->UL_INFO.rx_ind.number_of_pdus = 0;
  gNB->UL_INFO.crc_ind.number_crcs = 0;
  gNB->max_ldpc_iterations = max_ldpc_iterations;
  gNB->pusch_thres = -20;
  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)

  frame_parms->N_RB_DL = N_RB_DL;
  frame_parms->N_RB_UL = N_RB_UL;
  frame_parms->Ncp = extended_prefix_flag ? EXTENDED : NORMAL;

  AssertFatal((gNB->if_inst = NR_IF_Module_init(0)) != NULL, "Cannot register interface");
  gNB->if_inst->NR_PHY_config_req = nr_phy_config_request;

  s_re = malloc(n_tx*sizeof(double*));
  s_im = malloc(n_tx*sizeof(double*));
  r_re = malloc(n_rx*sizeof(double*));
  r_im = malloc(n_rx*sizeof(double*));

  NR_ServingCellConfigCommon_t *scc = calloc(1,sizeof(*scc));;
  prepare_scc(scc);
  uint64_t ssb_bitmap;
  fill_scc_sim(scc, &ssb_bitmap, N_RB_DL, N_RB_DL, mu, mu);
  fix_scc(scc,ssb_bitmap);

  // TODO do a UECAP for phy-sim
  const nr_mac_config_t conf = {.pdsch_AntennaPorts = {.N1 = 1, .N2 = 1, .XP = 1},
                                .pusch_AntennaPorts = n_rx,
                                .minRXTXTIME = 0,
                                .do_CSIRS = 0,
                                .do_SRS = 0,
                                .force_256qam_off = false};

  RC.nb_nr_macrlc_inst = 1;
  RC.nb_nr_mac_CC = (int*)malloc(RC.nb_nr_macrlc_inst*sizeof(int));
  for (i = 0; i < RC.nb_nr_macrlc_inst; i++)
    RC.nb_nr_mac_CC[i] = 1;
  mac_top_init_gNB(ngran_gNB, scc, NULL /* scd will be updated further below */, &conf);

  NR_ServingCellConfig_t *scd = calloc(1,sizeof(NR_ServingCellConfig_t));
  prepare_scd(scd);

  NR_UE_NR_Capability_t* UE_Capability_nr = CALLOC(1,sizeof(NR_UE_NR_Capability_t));
  prepare_sim_uecap(UE_Capability_nr,scc,mu,
                    N_RB_UL,0,mcs_table);

  NR_CellGroupConfig_t *secondaryCellGroup = get_default_secondaryCellGroup(scc, scd, UE_Capability_nr, 0, 1, &conf, 0);

  /* RRC parameter validation for secondaryCellGroup */
  fix_scd(scd);

  NR_BCCH_BCH_Message_t *mib = get_new_MIB_NR(scc);

  // UE dedicated configuration
  nr_mac_add_test_ue(RC.nrmac[0], secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity, secondaryCellGroup);
  frame_parms->nb_antennas_tx = 1;
  frame_parms->nb_antennas_rx = n_rx;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  cfg->carrier_config.num_tx_ant.value = 1;
  cfg->carrier_config.num_rx_ant.value = n_rx;

//  nr_phy_config_request_sim(gNB,N_RB_DL,N_RB_DL,mu,0,0x01);
  gNB->ldpc_offload_flag = ldpc_offload_flag;
  gNB->chest_freq = chest_type[0];
  gNB->chest_time = chest_type[1];

  phy_init_nr_gNB(gNB);
  /* RU handles rxdataF, and gNB just has a pointer. Here, we don't have an RU,
   * so we need to allocate that memory as well. */
  for (i = 0; i < n_rx; i++)
    gNB->common_vars.rxdataF[i] = malloc16_clear(gNB->frame_parms.samples_per_frame_wCP*sizeof(int32_t));
  N_RB_DL = gNB->frame_parms.N_RB_DL;

  /* no RU: need to have rxdata */
  c16_t **rxdata;
  rxdata = malloc(n_rx * sizeof(*rxdata));
  for (int i = 0; i < n_rx; ++i)
    rxdata[i] = calloc(gNB->frame_parms.samples_per_frame, sizeof(**rxdata));

  NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[0];

  // Configure channel model
  UE2gNB = new_channel_desc_scm(n_tx,
                                n_rx,
                                channel_model,
                                sampling_frequency / 1e6,
                                frame_parms->ul_CarrierFreq,
                                tx_bandwidth,
                                DS_TDL,
                                maxDoppler,
                                corr_level,
                                0,
                                delay,
                                0,
                                0);

  if (UE2gNB == NULL) {
    printf("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  // Configure UE
  UE = calloc(1, sizeof(PHY_VARS_NR_UE));
  PHY_vars_UE_g = malloc(sizeof(PHY_VARS_NR_UE**));
  PHY_vars_UE_g[0] = malloc(sizeof(PHY_VARS_NR_UE*));
  PHY_vars_UE_g[0][0] = UE;
  memcpy(&UE->frame_parms, frame_parms, sizeof(NR_DL_FRAME_PARMS));
  UE->frame_parms.nb_antennas_tx = n_tx;
  UE->frame_parms.nb_antennas_rx = 0;

  if (init_nr_ue_signal(UE, 1) != 0) {
    printf("Error at UE NR initialisation\n");
    exit(-1);
  }

  init_nr_ue_transport(UE);

  for(int n_scid = 0; n_scid<2; n_scid++) {
    UE->scramblingID_ulsch[n_scid] = frame_parms->Nid_cell;
    nr_init_pusch_dmrs(UE, frame_parms->Nid_cell, n_scid);
  }

  //Configure UE
  nr_l2_init_ue(1);
  NR_UE_MAC_INST_t* UE_mac = get_mac_inst(0);

  ue_init_config_request(UE_mac, mu);
  
  UE->if_inst = nr_ue_if_module_init(0);
  UE->if_inst->scheduled_response = nr_ue_scheduled_response;
  UE->if_inst->phy_config_request = nr_ue_phy_config_request;
  UE->if_inst->dl_indication = nr_ue_dl_indication;
  UE->if_inst->ul_indication = nr_ue_ul_indication;
  
  UE_mac->if_module = nr_ue_if_module_init(0);

  nr_ue_phy_config_request(&UE_mac->phy_config);

  unsigned char harq_pid = 0;

  NR_gNB_ULSCH_t *ulsch_gNB = &gNB->ulsch[UE_id];

  NR_Sched_Rsp_t *Sched_INFO = malloc(sizeof(*Sched_INFO));
  memset((void*)Sched_INFO,0,sizeof(*Sched_INFO));
  nfapi_nr_ul_tti_request_t *UL_tti_req = &Sched_INFO->UL_tti_req;
  Sched_INFO->sched_response_id = -1;

  nr_phy_data_tx_t phy_data = {0};

  uint32_t errors_decoding = 0;

  fapi_nr_ul_config_request_t ul_config = {0};

  uint8_t ptrs_mcs1 = 2;
  uint8_t ptrs_mcs2 = 4;
  uint8_t ptrs_mcs3 = 10;
  uint16_t n_rb0 = 25;
  uint16_t n_rb1 = 75;
  
  uint16_t pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA; // | PUSCH_PDU_BITMAP_PUSCH_PTRS;
  uint8_t crc_status = 0;

  unsigned char mod_order = nr_get_Qm_ul(Imcs, mcs_table);
  uint16_t code_rate = nr_get_code_rate_ul(Imcs, mcs_table);

  uint8_t mapping_type = typeB; // Default Values
  pusch_dmrs_type_t dmrs_config_type = pusch_dmrs_type1; // Default Values
  pusch_dmrs_AdditionalPosition_t add_pos = pusch_dmrs_pos0; // Default Values

  /* validate parameters othwerwise default values are used */
  /* -U flag can be used to set DMRS parameters*/
  if(modify_dmrs) {
    if(dmrs_arg[0] == 0)
      mapping_type = typeA;
    else if (dmrs_arg[0] == 1)
      mapping_type = typeB;
    /* Additional DMRS positions */
    if(dmrs_arg[1] >= 0 && dmrs_arg[1] <=3 )
      add_pos = dmrs_arg[1];
    /* DMRS Conf Type 1 or 2 */
    if(dmrs_arg[2] == 1)
      dmrs_config_type = pusch_dmrs_type1;
    else if(dmrs_arg[2] == 2)
      dmrs_config_type = pusch_dmrs_type2;
    num_dmrs_cdm_grps_no_data = dmrs_arg[3];
  }

  uint8_t  length_dmrs = pusch_len1;
  uint16_t l_prime_mask = get_l_prime(nb_symb_sch, mapping_type, add_pos, length_dmrs, start_symbol, NR_MIB__dmrs_TypeA_Position_pos2);
  uint16_t number_dmrs_symbols = get_dmrs_symbols_in_slot(l_prime_mask, nb_symb_sch);
  printf("num dmrs sym %d\n",number_dmrs_symbols);
  uint8_t  nb_re_dmrs = (dmrs_config_type == pusch_dmrs_type1) ? 6 : 4;

  uint32_t tbslbrm = 0;
  if (ilbrm)
    tbslbrm = nr_compute_tbslbrm(mcs_table,
                                 N_RB_UL,
                                 precod_nbr_layers);

  if ((UE->frame_parms.nb_antennas_tx==4)&&(precod_nbr_layers==4))
    num_dmrs_cdm_grps_no_data = 2;

  if (transform_precoding == transformPrecoder_enabled) {

    AssertFatal(enable_ptrs == 0, "PTRS NOT SUPPORTED IF TRANSFORM PRECODING IS ENABLED\n");

    int8_t index = get_index_for_dmrs_lowpapr_seq((NR_NB_SC_PER_RB/2) * nb_rb);
    AssertFatal(index >= 0, "Num RBs not configured according to 3GPP 38.211 section 6.3.1.4. For PUSCH with transform precoding, num RBs cannot be multiple of any other primenumber other than 2,3,5\n");

    dmrs_config_type = pusch_dmrs_type1;
    nb_re_dmrs = 6;

    printf("[ULSIM]: TRANSFORM PRECODING ENABLED. Num RBs: %d, index for DMRS_SEQ: %d\n", nb_rb, index);
  }

  nb_re_dmrs = nb_re_dmrs * num_dmrs_cdm_grps_no_data;
  unsigned int TBS = nr_compute_tbs(mod_order, code_rate, nb_rb, nb_symb_sch, nb_re_dmrs * number_dmrs_symbols, 0, 0, precod_nbr_layers);
  
  printf("[ULSIM]: length_dmrs: %u, l_prime_mask: %u	number_dmrs_symbols: %u, mapping_type: %u add_pos: %d \n", length_dmrs, l_prime_mask, number_dmrs_symbols, mapping_type, add_pos);
  printf("[ULSIM]: CDM groups: %u, dmrs_config_type: %d, num_rbs: %u, nb_symb_sch: %u\n", num_dmrs_cdm_grps_no_data, dmrs_config_type, nb_rb, nb_symb_sch);
  printf("[ULSIM]: MCS: %d, mod order: %u, code_rate: %u\n", Imcs, mod_order, code_rate);

  uint8_t ulsch_input_buffer[TBS/8];

  ulsch_input_buffer[0] = 0x31;
  for (i = 1; i < TBS/8; i++) {
    ulsch_input_buffer[i] = (uint8_t)rand();
  }

  uint8_t ptrs_time_density = get_L_ptrs(ptrs_mcs1, ptrs_mcs2, ptrs_mcs3, Imcs, mcs_table);
  uint8_t ptrs_freq_density = get_K_ptrs(n_rb0, n_rb1, nb_rb);

  double ts = 1.0/(frame_parms->subcarrier_spacing * frame_parms->ofdm_symbol_size);

  /* -T option enable PTRS */
  if(enable_ptrs) {
    /* validate parameters othwerwise default values are used */
    if(ptrs_arg[0] == 0 || ptrs_arg[0] == 1 || ptrs_arg[0] == 2 )
      ptrs_time_density = ptrs_arg[0];
    if(ptrs_arg[1] == 2 || ptrs_arg[1] == 4 )
      ptrs_freq_density = ptrs_arg[1];
    pdu_bit_map |= PUSCH_PDU_BITMAP_PUSCH_PTRS;
    printf("NOTE: PTRS Enabled with L %d, K %d \n", ptrs_time_density, ptrs_freq_density );
  }

  if (input_fd != NULL || n_trials == 1) max_rounds=1;

  if (enable_ptrs && 1 << ptrs_time_density >= nb_symb_sch)
    pdu_bit_map &= ~PUSCH_PDU_BITMAP_PUSCH_PTRS; // disable PUSCH PTRS

  printf("\n");

  uint32_t unav_res = 0;
  if (pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
    set_ptrs_symb_idx(&ptrsSymPos, nb_symb_sch, start_symbol, 1 << ptrs_time_density, l_prime_mask);
    ptrsSymbPerSlot = get_ptrs_symbols_in_slot(ptrsSymPos, start_symbol, nb_symb_sch);
    ptrsRePerSymb = ((nb_rb + ptrs_freq_density - 1) / ptrs_freq_density);
    unav_res = ptrsSymbPerSlot * ptrsRePerSymb;
    LOG_D(PHY, "[ULSIM] PTRS Symbols in a slot: %2u, RE per Symbol: %3u, RE in a slot %4d\n", ptrsSymbPerSlot, ptrsRePerSymb, unav_res);
  }

  unsigned int available_bits = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, number_dmrs_symbols, unav_res, mod_order, precod_nbr_layers);
  printf("[ULSIM]: VALUE OF G: %u, TBS: %u\n", available_bits, TBS);

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
  int read_errors=0;

  int slot_offset = frame_parms->get_samples_slot_timestamp(slot,frame_parms,0);
  int slot_length = slot_offset - frame_parms->get_samples_slot_timestamp(slot-1,frame_parms,0);

  if (input_fd != NULL)	{
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
    for (int irx=0; irx<frame_parms->nb_antennas_rx; irx++) {
      fseek(input_fd,irx*(slot_length+15)*sizeof(int16_t)*2,SEEK_SET); // matlab adds samlples to the end to emulate channel delay
      read_errors += fread((void *)&rxdata[irx][slot_offset-delay], sizeof(int16_t), slot_length<<1, input_fd);
      if (read_errors==0) {
        printf("error reading file\n");
        exit(1);
      }
      for (int i=0;i<16;i+=2)
        printf("slot_offset %d : %d,%d\n",
               slot_offset,
               rxdata[irx][slot_offset].r,
               rxdata[irx][slot_offset].i);
    }

    mod_order = nr_get_Qm_ul(Imcs, mcs_table);
    code_rate = nr_get_code_rate_ul(Imcs, mcs_table);
  }

  // csv file
  if (filename_csv != NULL) {
    csv_file = fopen(filename_csv, "a");
    if (csv_file == NULL) {
      printf("Can't open file \"%s\", errno %d\n", filename_csv, errno);
      return 1;
    }
    // adding name of parameters into file
    fprintf(csv_file,"SNR,false_positive,");
    for (int r = 0; r < max_rounds; r++)
      fprintf(csv_file,"n_errors_%d,errors_scrambling_%d,channel_bler_%d,channel_ber_%d,",r,r,r,r);
    fprintf(csv_file,"avg_round,eff_rate,eff_throughput,TBS,DMRS-PUSCH delay estimation: (min,max,average)\n");
  }
  //---------------
  int ret = 1;
  for (SNR = snr0; SNR <= snr1; SNR += snr_step) {

    varArray_t *table_rx=initVarArray(1000,sizeof(double));
    int error_flag = 0;
    n_false_positive = 0;
    effRate = 0;
    effTP = 0;
    roundStats = 0;
    reset_meas(&gNB->phy_proc_rx);
    reset_meas(&gNB->rx_pusch_stats);
    reset_meas(&gNB->rx_pusch_init_stats);
    reset_meas(&gNB->rx_pusch_symbol_processing_stats);
    reset_meas(&gNB->ulsch_channel_estimation_stats);
    reset_meas(&UE->ulsch_ldpc_encoding_stats);
    reset_meas(&UE->ulsch_rate_matching_stats);
    reset_meas(&UE->ulsch_interleaving_stats);
    reset_meas(&UE->ulsch_encoding_stats);
    reset_meas(&gNB->rx_srs_stats);
    reset_meas(&gNB->generate_srs_stats);
    reset_meas(&gNB->get_srs_signal_stats);
    reset_meas(&gNB->srs_channel_estimation_stats);
    reset_meas(&gNB->srs_timing_advance_stats);
    reset_meas(&gNB->srs_report_tlv_stats);
    reset_meas(&gNB->srs_beam_report_stats);
    reset_meas(&gNB->srs_iq_matrix_stats);

    uint32_t errors_scrambling[16] = {0};
    int n_errors[16] = {0};
    int round_trials[16] = {0};
    double blerStats[16] = {0};
    double berStats[16] = {0};

    uint64_t sum_pusch_delay = 0;
    int min_pusch_delay = INT_MAX;
    int max_pusch_delay = INT_MIN;
    int delay_pusch_est_count = 0;

    for (trial = 0; trial < n_trials; trial++) {

      uint8_t round = 0;
      crc_status = 1;
      errors_decoding = 0;

      while (round < max_rounds && crc_status) {

        round_trials[round]++;
        rv_index = nr_rv_round_map[round % 4];

        /// gNB UL PDUs

        UL_tti_req->SFN = frame;
        UL_tti_req->Slot = slot;
        UL_tti_req->n_pdus = do_SRS == 1 ? 2 : 1;

        nfapi_nr_ul_tti_request_number_of_pdus_t *pdu_element0 = &UL_tti_req->pdus_list[0];
        pdu_element0->pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
        pdu_element0->pdu_size = sizeof(nfapi_nr_pusch_pdu_t);

        nfapi_nr_pusch_pdu_t *pusch_pdu = &pdu_element0->pusch_pdu;
        memset(pusch_pdu, 0, sizeof(nfapi_nr_pusch_pdu_t));

        int abwp_size = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
        int abwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
        int ibwp_size = ibwps;
        int ibwp_start = ibwp_rboffset;
        if (msg3_flag == 1) {
          if ((ibwp_start < abwp_start) || (ibwp_size > abwp_size))
            pusch_pdu->bwp_start = abwp_start;
          else
            pusch_pdu->bwp_start = ibwp_start;
          pusch_pdu->bwp_size = ibwp_size;
          start_rb = (ibwp_start - abwp_start);
          printf("msg3: ibwp_size %d, abwp_size %d, ibwp_start %d, abwp_start %d\n", ibwp_size, abwp_size, ibwp_start, abwp_start);
        } else {
          pusch_pdu->bwp_start = abwp_start;
          pusch_pdu->bwp_size = abwp_size;
        }

        pusch_pdu->pusch_data.tb_size = TBS >> 3;
        pusch_pdu->pdu_bit_map = pdu_bit_map;
        pusch_pdu->rnti = n_rnti;
        pusch_pdu->mcs_index = Imcs;
        pusch_pdu->mcs_table = mcs_table;
        pusch_pdu->target_code_rate = code_rate;
        pusch_pdu->qam_mod_order = mod_order;
        pusch_pdu->transform_precoding = transform_precoding;
        pusch_pdu->data_scrambling_id = *scc->physCellId;
        pusch_pdu->nrOfLayers = precod_nbr_layers;
        pusch_pdu->ul_dmrs_symb_pos = l_prime_mask;
        pusch_pdu->dmrs_config_type = dmrs_config_type;
        pusch_pdu->ul_dmrs_scrambling_id = *scc->physCellId;
        pusch_pdu->scid = 0;
        pusch_pdu->dmrs_ports = ((1 << precod_nbr_layers) - 1);
        pusch_pdu->num_dmrs_cdm_grps_no_data = num_dmrs_cdm_grps_no_data;
        pusch_pdu->resource_alloc = 1;
        pusch_pdu->rb_start = start_rb;
        pusch_pdu->rb_size = nb_rb;
        pusch_pdu->vrb_to_prb_mapping = 0;
        pusch_pdu->frequency_hopping = 0;
        pusch_pdu->uplink_frequency_shift_7p5khz = 0;
        pusch_pdu->start_symbol_index = start_symbol;
        pusch_pdu->nr_of_symbols = nb_symb_sch;
        pusch_pdu->maintenance_parms_v3.tbSizeLbrmBytes = tbslbrm;
        pusch_pdu->pusch_data.rv_index = rv_index;
        pusch_pdu->pusch_data.harq_process_id = 0;
        pusch_pdu->pusch_data.new_data_indicator = round == 0 ? true : false;
        pusch_pdu->pusch_data.num_cb = 0;
        pusch_pdu->pusch_ptrs.ptrs_time_density = ptrs_time_density;
        pusch_pdu->pusch_ptrs.ptrs_freq_density = ptrs_freq_density;
        pusch_pdu->pusch_ptrs.ptrs_ports_list = (nfapi_nr_ptrs_ports_t *)malloc(2 * sizeof(nfapi_nr_ptrs_ports_t));
        pusch_pdu->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset = 0;
        pusch_pdu->maintenance_parms_v3.ldpcBaseGraph = get_BG(TBS, code_rate);

        // if transform precoding is enabled
        if (transform_precoding == transformPrecoder_enabled) {
          pusch_pdu->dfts_ofdm.low_papr_group_number = *scc->physCellId % 30; // U as defined in 38.211 section 6.4.1.1.1.2
          pusch_pdu->dfts_ofdm.low_papr_sequence_number = 0; // V as defined in 38.211 section 6.4.1.1.1.2
          pusch_pdu->num_dmrs_cdm_grps_no_data = num_dmrs_cdm_grps_no_data;
        }

        if (do_SRS == 1) {
          const uint16_t m_SRS[64] = { 4, 8, 12, 16, 16, 20, 24, 24, 28, 32, 36, 40, 48, 48, 52, 56, 60, 64, 72, 72, 76, 80, 88,
                                      96, 96, 104, 112, 120, 120, 120, 128, 128, 128, 132, 136, 144, 144, 144, 144, 152, 160,
                                      160, 160, 168, 176, 184, 192, 192, 192, 192, 208, 216, 224, 240, 240, 240, 240, 256, 256,
                                      256, 264, 272, 272, 272 };
          nfapi_nr_ul_tti_request_number_of_pdus_t *pdu_element1 = &UL_tti_req->pdus_list[1];
          pdu_element1->pdu_type = NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE;
          pdu_element1->pdu_size = sizeof(nfapi_nr_srs_pdu_t);
          nfapi_nr_srs_pdu_t *srs_pdu = &pdu_element1->srs_pdu;
          memset(srs_pdu, 0, sizeof(nfapi_nr_srs_pdu_t));
          srs_pdu->rnti = n_rnti;
          srs_pdu->bwp_size = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
          srs_pdu->bwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
          srs_pdu->subcarrier_spacing = frame_parms->subcarrier_spacing;
          srs_pdu->num_ant_ports = n_tx == 4 ? 2 : n_tx == 2 ? 1 : 0;
          srs_pdu->sequence_id = 40;
          srs_pdu->config_index = rrc_get_max_nr_csrs(srs_pdu->bwp_size, srs_pdu->bandwidth_index);
          srs_pdu->resource_type = NR_SRS_Resource__resourceType_PR_periodic;
          srs_pdu->t_srs = 1;
          srs_pdu->srs_parameters_v4.srs_bandwidth_size = m_SRS[srs_pdu->config_index];
          srs_pdu->srs_parameters_v4.usage = 1 << NR_SRS_ResourceSet__usage_codebook;
          srs_pdu->srs_parameters_v4.report_type[0] = 1;
          srs_pdu->srs_parameters_v4.iq_representation = 1;
          srs_pdu->srs_parameters_v4.prg_size = 1;
          srs_pdu->srs_parameters_v4.num_total_ue_antennas = 1 << srs_pdu->num_ant_ports;
          srs_pdu->beamforming.num_prgs = m_SRS[srs_pdu->config_index];
          srs_pdu->beamforming.prg_size = 1;
        }

        /// UE UL PDUs

        UE->ul_harq_processes[harq_pid].round = round;
        UE_proc.nr_slot_tx = slot;
        UE_proc.frame_tx = frame;
        UE_proc.gNB_id = 0;

        // prepare ULSCH/PUSCH reception
        pushNotifiedFIFO(&gNB->L1_tx_free, msgL1Tx); // to unblock the process in the beginning
        nr_schedule_response(Sched_INFO);

        // --------- setting parameters for UE --------
        nr_scheduled_response_t scheduled_response = {.ul_config = &ul_config, .phy_data = (void *)&phy_data};

        ul_config.slot = slot;
        ul_config.number_pdus = do_SRS == 1 ? 2 : 1;

        fapi_nr_ul_config_request_pdu_t *ul_config0 = &ul_config.ul_config_list[0];
        ul_config0->pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
        nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu = &ul_config0->pusch_config_pdu;
        // Config UL TX PDU
        pusch_config_pdu->tx_request_body.pdu = ulsch_input_buffer;
        pusch_config_pdu->tx_request_body.pdu_length = TBS / 8;
        pusch_config_pdu->rnti = n_rnti;
        pusch_config_pdu->pdu_bit_map = pdu_bit_map;
        pusch_config_pdu->qam_mod_order = mod_order;
        pusch_config_pdu->rb_size = nb_rb;
        pusch_config_pdu->rb_start = start_rb;
        pusch_config_pdu->nr_of_symbols = nb_symb_sch;
        pusch_config_pdu->start_symbol_index = start_symbol;
        pusch_config_pdu->ul_dmrs_symb_pos = l_prime_mask;
        pusch_config_pdu->dmrs_config_type = dmrs_config_type;
        pusch_config_pdu->mcs_index = Imcs;
        pusch_config_pdu->mcs_table = mcs_table;
        pusch_config_pdu->num_dmrs_cdm_grps_no_data = num_dmrs_cdm_grps_no_data;
        pusch_config_pdu->nrOfLayers = precod_nbr_layers;
        pusch_config_pdu->dmrs_ports = ((1 << precod_nbr_layers) - 1);
        pusch_config_pdu->absolute_delta_PUSCH = 0;
        pusch_config_pdu->target_code_rate = code_rate;
        pusch_config_pdu->tbslbrm = tbslbrm;
        pusch_config_pdu->ldpcBaseGraph = get_BG(TBS, code_rate);
        pusch_config_pdu->pusch_data.tb_size = TBS / 8;
        pusch_config_pdu->pusch_data.new_data_indicator = round == 0 ? true : false;
        pusch_config_pdu->pusch_data.rv_index = rv_index;
        pusch_config_pdu->pusch_data.harq_process_id = harq_pid;
        pusch_config_pdu->pusch_ptrs.ptrs_time_density = ptrs_time_density;
        pusch_config_pdu->pusch_ptrs.ptrs_freq_density = ptrs_freq_density;
        pusch_config_pdu->pusch_ptrs.ptrs_ports_list = (nfapi_nr_ue_ptrs_ports_t *)malloc(2 * sizeof(nfapi_nr_ue_ptrs_ports_t));
        pusch_config_pdu->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset = 0;
        pusch_config_pdu->transform_precoding = transform_precoding;
        // if transform precoding is enabled
        if (transform_precoding == transformPrecoder_enabled) {
          pusch_config_pdu->dfts_ofdm.low_papr_group_number = *scc->physCellId % 30; // U as defined in 38.211 section 6.4.1.1.1.2
          pusch_config_pdu->dfts_ofdm.low_papr_sequence_number = 0; // V as defined in 38.211 section 6.4.1.1.1.2
          // pusch_config_pdu->pdu_bit_map |= PUSCH_PDU_BITMAP_DFTS_OFDM;
          pusch_config_pdu->num_dmrs_cdm_grps_no_data = num_dmrs_cdm_grps_no_data;
        }

        if (do_SRS == 1) {
          fapi_nr_ul_config_request_pdu_t *ul_config1 = &ul_config.ul_config_list[1];
          ul_config1->pdu_type = FAPI_NR_UL_CONFIG_TYPE_SRS;
          fapi_nr_ul_config_srs_pdu *srs_config_pdu = &ul_config1->srs_config_pdu;
          memset(srs_config_pdu, 0, sizeof(fapi_nr_ul_config_srs_pdu));
          srs_config_pdu->rnti = n_rnti;
          srs_config_pdu->bwp_size = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
          srs_config_pdu->bwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
          srs_config_pdu->subcarrier_spacing = frame_parms->subcarrier_spacing;
          srs_config_pdu->num_ant_ports = n_tx == 4 ? 2 : n_tx == 2 ? 1 : 0;
          srs_config_pdu->config_index = rrc_get_max_nr_csrs(srs_config_pdu->bwp_size, srs_config_pdu->bandwidth_index);
          srs_config_pdu->sequence_id = 40;
          srs_config_pdu->resource_type = NR_SRS_Resource__resourceType_PR_periodic;
          srs_config_pdu->t_srs = 1;
        }

        for (int i = 0; i < (TBS / 8); i++)
          UE->ul_harq_processes[harq_pid].a[i] = i & 0xff;

        if (input_fd == NULL) {
          // set FAPI parameters for UE, put them in the scheduled response and call
          nr_ue_scheduled_response(&scheduled_response);

          /////////////////////////phy_procedures_nr_ue_TX///////////////////////
          ///////////

          phy_procedures_nrUE_TX(UE, &UE_proc, &phy_data);

          if (n_trials == 1) {
            LOG_M("txsig0.m", "txs0", &UE->common_vars.txData[0][slot_offset], slot_length, 1, 1);
            if (precod_nbr_layers > 1) {
            LOG_M("txsig1.m", "txs1", &UE->common_vars.txData[1][slot_offset], slot_length, 1, 1);
            if (precod_nbr_layers == 4) {
              LOG_M("txsig2.m", "txs2", &UE->common_vars.txData[2][slot_offset], slot_length, 1, 1);
              LOG_M("txsig3.m", "txs3", &UE->common_vars.txData[3][slot_offset], slot_length, 1, 1);
              }
            }
          }
          ///////////
          ////////////////////////////////////////////////////
          tx_offset = frame_parms->get_samples_slot_timestamp(slot, frame_parms, 0);
          txlev_sum = 0;
          for (int aa = 0; aa < UE->frame_parms.nb_antennas_tx; aa++) {
            atxlev[aa] = signal_energy(
                (int32_t *)&UE->common_vars.txData[aa][tx_offset + 5 * frame_parms->ofdm_symbol_size
                                                       + 4 * frame_parms->nb_prefix_samples + frame_parms->nb_prefix_samples0],
                frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples);

            txlev_sum += atxlev[aa];

            if (n_trials == 1)
              printf("txlev[%d] = %d (%f dB) txlev_sum %d\n", aa, atxlev[aa], 10 * log10((double)atxlev[aa]), txlev_sum);
          }
        } else
          n_trials = 1;

        if (input_fd == NULL) {
          // Justification of division by precod_nbr_layers:
          // When the channel is the identity matrix, the results in terms of SNR should be almost equal for 2x2 and 4x4.
          sigma_dB = 10 * log10((double)txlev_sum / precod_nbr_layers * ((double)frame_parms->ofdm_symbol_size / (12 * nb_rb))) - SNR;
          sigma = pow(10, sigma_dB / 10);

          if (n_trials == 1)
            printf("sigma %f (%f dB), txlev_sum %f (factor %f)\n", sigma, sigma_dB, 10 * log10((double)txlev_sum), (double)(double)frame_parms->ofdm_symbol_size / (12 * nb_rb));

          for (i = 0; i < slot_length; i++) {
            for (int aa = 0; aa < UE->frame_parms.nb_antennas_tx; aa++) {
              s_re[aa][i] = (double)UE->common_vars.txData[aa][slot_offset + i].r;
              s_im[aa][i] = (double)UE->common_vars.txData[aa][slot_offset + i].i;
            }
          }

          multipath_channel(UE2gNB, s_re, s_im, r_re, r_im, slot_length, 0, (n_trials == 1) ? 1 : 0);
          add_noise(rxdata, (const double **) r_re, (const double **) r_im, sigma, slot_length, slot_offset, ts, delay, pdu_bit_map, PUSCH_PDU_BITMAP_PUSCH_PTRS, frame_parms->nb_antennas_rx);

        } /*End input_fd */

        //----------------------------------------------------------
        //------------------- gNB phy procedures -------------------
        //----------------------------------------------------------
        gNB->UL_INFO.rx_ind.number_of_pdus = 0;
        gNB->UL_INFO.crc_ind.number_crcs = 0;
        gNB->UL_INFO.srs_ind.number_of_pdus = 0;

        for(uint8_t symbol = 0; symbol < (gNB->frame_parms.Ncp == EXTENDED ? 12 : 14); symbol++) {
          for (int aa = 0; aa < gNB->frame_parms.nb_antennas_rx; aa++)
            nr_slot_fep_ul(&gNB->frame_parms,
                           (int32_t *)rxdata[aa],
                           (int32_t *)gNB->common_vars.rxdataF[aa],
                           symbol,
                           slot,
                           0);
        }
        int offset = (slot & 3) * gNB->frame_parms.symbols_per_slot * gNB->frame_parms.ofdm_symbol_size;
        for (int aa = 0; aa < gNB->frame_parms.nb_antennas_rx; aa++)  {
          apply_nr_rotation_RX(&gNB->frame_parms,
                               gNB->common_vars.rxdataF[aa],
                               gNB->frame_parms.symbol_rotation[1],
                               slot,
                               gNB->frame_parms.N_RB_UL,
                               offset,
                               0,
                               gNB->frame_parms.Ncp == EXTENDED ? 12 : 14);
        }

        ul_proc_error = phy_procedures_gNB_uespec_RX(gNB, frame, slot);

        if (n_trials == 1 && round == 0) {
          LOG_M("rxsig0.m", "rx0", &rxdata[0][slot_offset], slot_length, 1, 1);
          LOG_M("rxsigF0.m", "rxsF0", gNB->common_vars.rxdataF[0], 14 * frame_parms->ofdm_symbol_size, 1, 1);
          if (precod_nbr_layers > 1) {
            LOG_M("rxsig1.m", "rx1", &rxdata[1][slot_offset], slot_length, 1, 1);
            LOG_M("rxsigF1.m", "rxsF1", gNB->common_vars.rxdataF[1], 14 * frame_parms->ofdm_symbol_size, 1, 1);
            if (precod_nbr_layers == 4) {
              LOG_M("rxsig2.m", "rx2", &rxdata[2][slot_offset], slot_length, 1, 1);
              LOG_M("rxsig3.m", "rx3", &rxdata[3][slot_offset], slot_length, 1, 1);
              LOG_M("rxsigF2.m", "rxsF2", gNB->common_vars.rxdataF[2], 14 * frame_parms->ofdm_symbol_size, 1, 1);
              LOG_M("rxsigF3.m", "rxsF3", gNB->common_vars.rxdataF[3], 14 * frame_parms->ofdm_symbol_size, 1, 1);
            }
          }
        }

        NR_gNB_PUSCH *pusch_vars = &gNB->pusch_vars[UE_id];
        if (n_trials == 1 && round == 0) {
          __attribute__((unused)) int off = ((nb_rb & 1) == 1) ? 4 : 0;

          LOG_M("rxsigF0_ext.m",
                "rxsF0_ext",
                &pusch_vars->rxdataF_ext[0][start_symbol * NR_NB_SC_PER_RB * pusch_pdu->rb_size],
                nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                1,
                1);
          LOG_M("chestF0.m",
                "chF0",
                &pusch_vars->ul_ch_estimates[0][start_symbol * frame_parms->ofdm_symbol_size],
                frame_parms->ofdm_symbol_size,
                1,
                1);
          LOG_M("chestF0_ext.m",
                "chF0_ext",
                &pusch_vars->ul_ch_estimates_ext[0][(start_symbol + 1) * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                (nb_symb_sch - 1) * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                1,
                1);
          LOG_M("rxsigF0_comp.m",
                "rxsF0_comp",
                &pusch_vars->rxdataF_comp[0][start_symbol * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                1,
                1);
          LOG_M("rxsigF0_llrlayers0.m",
                "rxsF0_llrlayers0",
                &pusch_vars->llr_layers[0][0],
                (nb_symb_sch - 1) * NR_NB_SC_PER_RB * pusch_pdu->rb_size * mod_order,
                1,
                0);

          if (precod_nbr_layers == 2) {
            LOG_M("rxsigF1_ext.m",
                  "rxsF1_ext",
                  &pusch_vars->rxdataF_ext[1][start_symbol * NR_NB_SC_PER_RB * pusch_pdu->rb_size],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);

            LOG_M("chestF3.m",
                  "chF3",
                  &pusch_vars->ul_ch_estimates[3][start_symbol * frame_parms->ofdm_symbol_size],
                  frame_parms->ofdm_symbol_size,
                  1,
                  1);

            LOG_M("chestF3_ext.m",
                  "chF3_ext",
                  &pusch_vars->ul_ch_estimates_ext[3][(start_symbol + 1) * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  (nb_symb_sch - 1) * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);

            LOG_M("rxsigF2_comp.m",
                  "rxsF2_comp",
                  &pusch_vars->rxdataF_comp[2][start_symbol * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);

            LOG_M("rxsigF0_llrlayers1.m",
                  "rxsF0_llrlayers1",
                  &pusch_vars->llr_layers[1][0],
                  (nb_symb_sch - 1) * NR_NB_SC_PER_RB * pusch_pdu->rb_size * mod_order,
                  1,
                  0);
          }

          if (precod_nbr_layers == 4) {
            LOG_M("rxsigF1_ext.m",
                  "rxsF1_ext",
                  &pusch_vars->rxdataF_ext[1][start_symbol * NR_NB_SC_PER_RB * pusch_pdu->rb_size],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);
            LOG_M("rxsigF2_ext.m",
                  "rxsF2_ext",
                  &pusch_vars->rxdataF_ext[2][start_symbol * NR_NB_SC_PER_RB * pusch_pdu->rb_size],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);
            LOG_M("rxsigF3_ext.m",
                  "rxsF3_ext",
                  &pusch_vars->rxdataF_ext[3][start_symbol * NR_NB_SC_PER_RB * pusch_pdu->rb_size],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);

            LOG_M("chestF5.m",
                  "chF5",
                  &pusch_vars->ul_ch_estimates[5][start_symbol * frame_parms->ofdm_symbol_size],
                  frame_parms->ofdm_symbol_size,
                  1,
                  1);
            LOG_M("chestF10.m",
                  "chF10",
                  &pusch_vars->ul_ch_estimates[10][start_symbol * frame_parms->ofdm_symbol_size],
                  frame_parms->ofdm_symbol_size,
                  1,
                  1);
            LOG_M("chestF15.m",
                  "chF15",
                  &pusch_vars->ul_ch_estimates[15][start_symbol * frame_parms->ofdm_symbol_size],
                  frame_parms->ofdm_symbol_size,
                  1,
                  1);

            LOG_M("chestF5_ext.m",
                  "chF5_ext",
                  &pusch_vars->ul_ch_estimates_ext[5][(start_symbol + 1) * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  (nb_symb_sch - 1) * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);
            LOG_M("chestF10_ext.m",
                  "chF10_ext",
                  &pusch_vars->ul_ch_estimates_ext[10][(start_symbol + 1) * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  (nb_symb_sch - 1) * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);
            LOG_M("chestF15_ext.m",
                  "chF15_ext",
                  &pusch_vars->ul_ch_estimates_ext[15][(start_symbol + 1) * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  (nb_symb_sch - 1) * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);

            LOG_M("rxsigF4_comp.m",
                  "rxsF4_comp",
                  &pusch_vars->rxdataF_comp[4][start_symbol * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);
            LOG_M("rxsigF8_comp.m",
                  "rxsF8_comp",
                  &pusch_vars->rxdataF_comp[8][start_symbol * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);
            LOG_M("rxsigF12_comp.m",
                  "rxsF12_comp",
                  &pusch_vars->rxdataF_comp[12][start_symbol * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1);
            LOG_M("rxsigF0_llrlayers1.m",
                  "rxsF0_llrlayers1",
                  &pusch_vars->llr_layers[1][0],
                  (nb_symb_sch - 1) * NR_NB_SC_PER_RB * pusch_pdu->rb_size * mod_order,
                  1,
                  0);
            LOG_M("rxsigF0_llrlayers2.m",
                  "rxsF0_llrlayers2",
                  &pusch_vars->llr_layers[2][0],
                  (nb_symb_sch - 1) * NR_NB_SC_PER_RB * pusch_pdu->rb_size * mod_order,
                  1,
                  0);
            LOG_M("rxsigF0_llrlayers3.m",
                  "rxsF0_llrlayers3",
                  &pusch_vars->llr_layers[3][0],
                  (nb_symb_sch - 1) * NR_NB_SC_PER_RB * pusch_pdu->rb_size * mod_order,
                  1,
                  0);
          }

          LOG_M("rxsigF0_llr.m",
                "rxsF0_llr",
                &pusch_vars->llr[0],
                precod_nbr_layers * (nb_symb_sch - 1) * NR_NB_SC_PER_RB * pusch_pdu->rb_size * mod_order,
                1,
                0);
        }

        if ((ulsch_gNB->last_iteration_cnt >= ulsch_gNB->max_ldpc_iterations + 1) || ul_proc_error == 1) {
          error_flag = 1;
          n_errors[round]++;
          crc_status = 1;
        } else
          crc_status = 0;
        if (n_trials == 1)
          printf("end of round %d rv_index %d\n", round, rv_index);

        //----------------------------------------------------------
        //----------------- count and print errors -----------------
        //----------------------------------------------------------

        if ((pusch_pdu->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) && (SNR == snr0) && (trial == 0) && (round == 0)) {
          printf("[ULSIM][PTRS] Available bits are: %5u, removed PTRS bits are: %5d \n",
                 available_bits, (ptrsSymbPerSlot * ptrsRePerSymb * mod_order * precod_nbr_layers));
        }

        for (i = 0; i < available_bits; i++) {
          if (((UE->ul_harq_processes[harq_pid].f[i] == 0) && (pusch_vars->llr[i] <= 0))
              || ((UE->ul_harq_processes[harq_pid].f[i] == 1) && (pusch_vars->llr[i] >= 0))) {
            /*if(errors_scrambling == 0)
              printf("\x1B[34m" "[frame %d][trial %d]\t1st bit in error in unscrambling = %d\n" "\x1B[0m", frame, trial, i);*/
            errors_scrambling[round]++;
          }
        }
        round++;
      } // round

      if (n_trials == 1 && errors_scrambling[0] > 0) {
        printf("\x1B[31m""[frame %d][trial %d]\tnumber of errors in unscrambling = %u\n" "\x1B[0m", frame, trial, errors_scrambling[0]);
      }

      for (i = 0; i < TBS; i++) {
        uint8_t estimated_output_bit = (ulsch_gNB->harq_process->b[i / 8] & (1 << (i & 7))) >> (i & 7);
        uint8_t test_input_bit = (UE->ul_harq_processes[harq_pid].b[i / 8] & (1 << (i & 7))) >> (i & 7);
      
        if (estimated_output_bit != test_input_bit) {
          /*if(errors_decoding == 0)
              printf("\x1B[34m""[frame %d][trial %d]\t1st bit in error in decoding     = %d\n" "\x1B[0m", frame, trial, i);*/
          errors_decoding++;
        }
      }
      if (n_trials == 1) {
        for (int r = 0; r < UE->ul_harq_processes[harq_pid].C; r++)
          for (int i = 0; i < UE->ul_harq_processes[harq_pid].K >> 3; i++) {
            if ((UE->ul_harq_processes[harq_pid].c[r][i] ^ ulsch_gNB->harq_process->c[r][i]) != 0)
              printf("************");
            /*printf("r %d: in[%d] %x, out[%d] %x (%x)\n",r,
              i,UE->ul_harq_processes[harq_pid].c[r][i],
              i,ulsch_gNB->harq_process->c[r][i],
              UE->ul_harq_processes[harq_pid].c[r][i]^ulsch_gNB->harq_process->c[r][i]);*/
          }
      }
      if (errors_decoding > 0 && error_flag == 0) {
        n_false_positive++;
        if (n_trials==1)
	  printf("\x1B[31m""[frame %d][trial %d]\tnumber of errors in decoding     = %u\n" "\x1B[0m", frame, trial, errors_decoding);
      } 
      roundStats += ((float)round);
      if (!crc_status)
        effRate += ((double)TBS) / (double)round;

      sum_pusch_delay += ulsch_gNB->delay.est_delay;
      min_pusch_delay = min(ulsch_gNB->delay.est_delay, min_pusch_delay);
      max_pusch_delay = max(ulsch_gNB->delay.est_delay, max_pusch_delay);
      delay_pusch_est_count++;

    } // trial loop

    roundStats/=((float)n_trials);
    effRate /= (double)n_trials;
    

    // -------csv file-------

    // adding values into file
    printf("*****************************************\n");
    printf("SNR %f: n_errors (%d/%d", SNR, n_errors[0], round_trials[0]);
    for (int r = 1; r < max_rounds; r++)
      printf(",%d/%d", n_errors[r], round_trials[r]);
    printf(") (negative CRC), false_positive %d/%d, errors_scrambling (%u/%u",
           n_false_positive, n_trials, errors_scrambling[0], available_bits * round_trials[0]);
    for (int r = 1; r < max_rounds; r++)
      printf(",%u/%u", errors_scrambling[r], available_bits * round_trials[r]);
    printf(")\n");
    printf("\n");


    for (int r = 0; r < max_rounds; r++) {
      blerStats[r] = (double)n_errors[r] / round_trials[r];
      berStats[r] = (double)errors_scrambling[r] / available_bits/round_trials[r];
    }
    effTP = effRate/(double)TBS * (double)100;
    printf("SNR %f: Channel BLER (%e", SNR, blerStats[0]);
    for (int r = 1; r < max_rounds; r++)
      printf(",%e", blerStats[r]);
    printf(" Channel BER (%e", berStats[0]);
    for (int r = 1; r < max_rounds; r++)
      printf(",%e", berStats[r]);

    printf(") Avg round %.2f, Eff Rate %.4f bits/slot, Eff Throughput %.2f, TBS %u bits/slot\n", roundStats,effRate,effTP,TBS);

    printf("DMRS-PUSCH delay estimation: min %i, max %i, average %f\n",
           min_pusch_delay >> 1, max_pusch_delay >> 1, (double)sum_pusch_delay / (2 * delay_pusch_est_count));

    printf("*****************************************\n");
    printf("\n");
    // writing to csv file
    if (filename_csv) { // means we are asked to print stats to CSV
      fprintf(csv_file,"%f,%d/%d,",SNR,n_false_positive,n_trials);
      for (int r = 0; r < max_rounds; r++)
        fprintf(csv_file,"%d/%d,%u/%u,%f,%e,",n_errors[r], round_trials[r], errors_scrambling[r], available_bits * round_trials[r],blerStats[r],berStats[r]);
      fprintf(csv_file,"%.2f,%.4f,%.2f,%u,(%i,%i,%f)\n", roundStats, effRate, effTP, TBS,min_pusch_delay >> 1, max_pusch_delay >> 1, (double)sum_pusch_delay / (2 * delay_pusch_est_count));
    }
    FILE *fd=fopen("nr_ulsim.log","w");
    if (fd == NULL) {
      printf("Problem with filename %s\n", "nr_ulsim.log");
      exit(-1);
    }
    dump_pusch_stats(fd,gNB);
    fclose(fd);

    if (print_perf==1) 
    {
      printf("gNB RX\n");
      printDistribution(&gNB->phy_proc_rx,table_rx, "Total PHY proc rx");
      printStatIndent(&gNB->rx_pusch_stats, "RX PUSCH time");
      printStatIndent2(&gNB->ulsch_channel_estimation_stats, "ULSCH channel estimation time");
      printStatIndent2(&gNB->rx_pusch_init_stats, "RX PUSCH Initialization time");
      printStatIndent2(&gNB->rx_pusch_symbol_processing_stats, "RX PUSCH Symbol Processing time");

      printf("\nUE TX\n");
      printStatIndent(&UE->ulsch_encoding_stats,"ULSCH total encoding time");
      printStatIndent2(&UE->ulsch_segmentation_stats,"ULSCH segmentation time");
      printStatIndent2(&UE->ulsch_ldpc_encoding_stats,"ULSCH LDPC encoder time");
      printStatIndent2(&UE->ulsch_rate_matching_stats,"ULSCH rate-matching time");
      printStatIndent2(&UE->ulsch_interleaving_stats,"ULSCH interleaving time");
      printStatIndent(&gNB->rx_srs_stats,"RX SRS time");
      printStatIndent2(&gNB->generate_srs_stats,"Generate SRS sequence time");
      printStatIndent2(&gNB->get_srs_signal_stats,"Get SRS signal time");
      printStatIndent2(&gNB->srs_channel_estimation_stats,"SRS channel estimation time");
      printStatIndent2(&gNB->srs_timing_advance_stats,"SRS timing advance estimation time");
      printStatIndent2(&gNB->srs_report_tlv_stats,"SRS report TLV build time");
      printStatIndent3(&gNB->srs_beam_report_stats,"SRS beam report build time");
      printStatIndent3(&gNB->srs_iq_matrix_stats,"SRS IQ matrix build time");
      printf("\n");
    }

    if(n_trials==1)
      break;

    if ((float)effTP >= eff_tp_check) {
      printf("*************\n");
      printf("PUSCH test OK\n");
      printf("*************\n");
      ret = 0;
      break;
    }
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

  free_MIB_NR(mib);
  if (gNB->ldpc_offload_flag)
    free_LDPClib(&ldpc_interface_offload);

  if (output_fd)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  if (scg_fd)
    fclose(scg_fd);

  // closing csv file
  if (filename_csv != NULL) { // means we are asked to print stats to CSV
    fclose(csv_file);
    free(filename_csv);
  }

  return ret;
}
