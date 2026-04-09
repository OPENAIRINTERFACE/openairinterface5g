/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
*/

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <bits/getopt_core.h>
#include "common/utils/nr/nr_common.h"
#define inMicroS(a) (((double)(a))/(get_cpu_freq_GHz()*1000.0))
#include "SIMULATION/LTE_PHY/common_sim.h"
#include "common/utils/assertions.h"
#include "executables/softmodem-common.h"
#include "NR_BCCH-BCH-Message.h"
#include "NR_IF_Module.h"
#include "NR_MAC_COMMON/nr_mac.h"
#include "NR_MAC_COMMON/nr_mac_common.h"
#include "NR_MAC_UE/mac_defs.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_PHY_INTERFACE/NR_IF_Module.h"
#include "NR_ReconfigurationWithSync.h"
#include "NR_ServingCellConfig.h"
#include "NR_UE-NR-Capability.h"
#include "PHY/CODING/nrLDPC_coding/nrLDPC_coding_interface.h"
#include "PHY/INIT/nr_phy_init.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/TOOLS/tools_defs.h"
#include "PHY/defs_RU.h"
#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/defs_nr_common.h"
#include "PHY/impl_defs_nr.h"
#include "PHY/phy_vars_nr_ue.h"
#include "SCHED_NR/sched_nr.h"
#include "SCHED_NR_UE/defs.h"
#include "SCHED_NR_UE/fapi_nr_ue_l1.h"
#include "asn_internal.h"
#include "assertions.h"
#include "common/config/config_load_configmodule.h"
#include "common/ngran_types.h"
#include "common/openairinterface5g_limits.h"
#include "common/ran_context.h"
#include "common/utils/LOG/log.h"
#include "common/utils/bits.h"
#include "common/utils/T/T.h"
#include "common/utils/nr/nr_common.h"
#include "common/utils/threadPool/thread-pool.h"
#include "e1ap_messages_types.h"
#include "executables/nr-uesoftmodem.h"
#include "fapi_nr_ue_constants.h"
#include "fapi_nr_ue_interface.h"
#include "nfapi_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "nr_ue_phy_meas.h"
#include "openair1/SIMULATION/NR_PHY/nr_unitary_defs.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_radio_config.h"
#include "time_meas.h"
#include "utils.h"

#ifdef CHANNEL_SIM_CUDA
#include <cuda.h>
#include <cuda_runtime.h>
#include "SIMULATION/TOOLS/oai_cuda.h"
#endif

//#define DEBUG_ULSIM

const char *__asan_default_options()
{
  /* don't do leak checking in nr_ulsim, not finished yet */
  return "detect_leaks=0";
}
PHY_VARS_gNB *gNB;
RAN_CONTEXT_t RC;
char *uecap_file;
int64_t uplink_frequency_offset[MAX_NUM_CCs][4];

double cpuf;
//uint8_t nfapi_mode = 0;
uint64_t downlink_frequency[MAX_NUM_CCs][4];
THREAD_STRUCT thread_struct;
nfapi_ue_release_request_body_t release_rntis;

// NTN cellSpecificKoffset-r17, but in slots for DL SCS
unsigned int NTN_UE_Koffset = 0;

void nr_derive_key_ng_ran_star(uint16_t pci, uint64_t nr_arfcn_dl, const uint8_t key[32], uint8_t *key_ng_ran_star)
{
}

/* this is a hack, but necessary for E2 agent. We compile in all of RRC
 * (because of CMakeLists.txt), but we don't need it (only nr_radio_config.c).
 * however, if E2 agent is defined, the following functions are used in
 * rrc_gNB.c, but defined in RAN functions. In order to avoid pulling this in
 * here as well, only provide a prototype (and abort if they are ever called). */
void signal_rrc_msg(void /*const nr_rrc_class_e nr_channel, const uint32_t rrc_msg_id, const byte_array_t rrc_ba*/ ) { abort(); }
void signal_rrc_state_changed_to(void /* const gNB_RRC_UE_t *rrc_ue_context, const rrc_state_e2sm_rc_e rrc_state */) { abort(); }
void signal_ue_id(void /* const gNB_RRC_UE_t *rrc_ue_context, const uint16_t class, const uint32_t msg_id */) { abort(); }

void e1_bearer_context_setup(const e1ap_bearer_setup_req_t *req) { abort(); }
void e1_bearer_context_modif(const e1ap_bearer_mod_req_t *req) { abort(); }
void e1_bearer_release_cmd(const e1ap_bearer_release_cmd_t *cmd) { abort(); }

int8_t nr_rrc_RA_succeeded(const module_id_t mod_id, const uint8_t gNB_index) {
  return 0;
}

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
openair0_config_t openair0_cfg_g[MAX_CARDS] = {};

channel_desc_t *UE2gNB[MAX_MOBILES_PER_GNB][NUMBER_OF_gNB_MAX];

static void copy_bytes_to_packed_bits(const uint8_t *in, const uint32_t num_bits, const bool is_ulsch, uint8_t *out)
{
  if (is_ulsch) { // MATLAB computes CRC for input of MSB first
    for (uint_fast32_t b = 0; b < num_bits; b++) {
      out[b / 8] |= ((in[b] & 1) << (7 - (b % 8)));
    }
  } else {
    for (uint_fast32_t b = 0; b < num_bits; b++) {
      out[b / 8] |= (in[b] << (b % 8));
    }
  }
}

static void prepare_ue_pusch_pdu_from_matlab_vector(const bool uci_on_pusch,
                                                    FILE *vect_file,
                                                    nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu,
                                                    uint8_t *cw_buf)
{
  if (!uci_on_pusch)
    return;

  if (vect_file == NULL)
    return;

  struct vect_vars {
    uint32_t A;
    uint32_t oack;
    uint32_t ocsi1;
    uint32_t ocsi2;
    uint32_t cwlen;
    uint32_t cwlen_scr;
  } __attribute__((packed));

  struct vect_vars var = {0};
  if (1 != fread(&var, sizeof(var), 1, vect_file)) {
    printf("Error reading from matlab vector file\n");
    exit(-1);
  }

  const uint16_t buff_len = var.A + var.oack + var.ocsi1 + var.ocsi2;
  uint8_t vec_bits[buff_len];
  memset(vec_bits, 0, sizeof(vect_file));

  uint8_t *p_vec_bits = vec_bits;
  if (var.A != fread(p_vec_bits, sizeof(uint8_t), var.A, vect_file)) {
    printf("Error reading ULSCH bits from file\n");
    exit(-1);
  }
  p_vec_bits += var.A;
  if (var.oack != fread(p_vec_bits, sizeof(uint8_t), var.oack, vect_file)) {
    printf("Error reading ACK bits from file\n");
    exit(-1);
  }
  p_vec_bits += var.oack;
  if (var.ocsi1 != fread(p_vec_bits, sizeof(uint8_t), var.ocsi1, vect_file)) {
    printf("Error reading CSI1 bits from file\n");
    exit(-1);
  }
  p_vec_bits += var.ocsi1;
  if (var.ocsi2 != fread(p_vec_bits, sizeof(uint8_t), var.ocsi2, vect_file)) {
    printf("Error reading CSI2 bits from file\n");
    exit(-1);
  }

  if (var.cwlen != fread(cw_buf, sizeof(uint8_t), var.cwlen, vect_file)) {
    printf("Error reading cw bits from file\n");
    exit(-1);
  }

  memset(cw_buf, 0, var.cwlen_scr);
  if (var.cwlen_scr != fread(cw_buf, sizeof(uint8_t), var.cwlen_scr, vect_file)) {
    printf("Error reading cw bits from file\n");
    exit(-1);
  }

  uint16_t tb_buf_size = (var.A + 7) / 8;
  pusch_config_pdu->pusch_data.tb_size = tb_buf_size;
  pusch_config_pdu->tx_request_body.pdu_length = tb_buf_size;
  uint8_t *pb = pusch_config_pdu->tx_request_body.fapiTxPdu;
  memset(pb, 0, tb_buf_size);
  p_vec_bits = vec_bits;
  copy_bytes_to_packed_bits(p_vec_bits, var.A, true, pb);

  pusch_config_pdu->pusch_uci.harq_ack_bit_length = var.oack;
  pb = (uint8_t *)&pusch_config_pdu->pusch_uci.harq_payload;
  memset(pb, 0, sizeof(pusch_config_pdu->pusch_uci.harq_payload));
  p_vec_bits += var.A;
  copy_bytes_to_packed_bits(p_vec_bits, var.oack, false, pb);

  pusch_config_pdu->pusch_uci.csi_payload.p1_bits = var.ocsi1;
  pb = (uint8_t *)&pusch_config_pdu->pusch_uci.csi_payload.part1_payload;
  memset(pb, 0, sizeof(pusch_config_pdu->pusch_uci.csi_payload.part1_payload));
  p_vec_bits += var.oack;
  copy_bytes_to_packed_bits(p_vec_bits, var.ocsi1, false, pb);

  pusch_config_pdu->pusch_uci.csi_payload.p2_bits = var.ocsi2;
  pb = (uint8_t *)&pusch_config_pdu->pusch_uci.csi_payload.part2_payload;
  memset(pb, 0, sizeof(pusch_config_pdu->pusch_uci.csi_payload.part2_payload));
  p_vec_bits += var.ocsi1;
  copy_bytes_to_packed_bits(p_vec_bits, var.ocsi2, false, pb);
}

configmodule_interface_t *uniqCfg = NULL;
int main(int argc, char *argv[])
{
  stop = false;
  __attribute__((unused)) struct sigaction oldaction;
  sigaction(SIGINT, &sigint_action, &oldaction);

  FILE *csv_file = NULL;
  char *filename_csv = NULL;
  int i;
  double SNR, snr0 = -2.0, snr1 = 2.0;
  double snr_step = .2;
  uint8_t snr1set = 0;
  int slot = 8, frame = 1;
  int do_SRS = 0;
  FILE *output_fd = NULL;
  float **s_interleaved, **r_re, **r_im;
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
  uint8_t length_dmrs = pusch_len1;

  // unsigned char frame_type = 0;
  int loglvl = OAILOG_WARNING;
  uint16_t nb_symb_sch = 12;
  int start_symbol = 0;
  uint16_t nb_rb = 50;
  int Imcs = 9;
  uint8_t precod_nbr_layers = 1;
  int tx_offset;
  double txlev_sum = 0;
  int start_rb = 0;
  int UE_id = 0;
  int print_perf = 0;
  cpuf = get_cpu_freq_GHz();
  int msg3_flag = 0;
  bool uci_on_pusch = false;
  bool no_phase_pre_comp = false;
  int rv_index = 0;
  float roundStats;
  double effRate;
  double effTP;
  float eff_tp_check = 100;
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
  FILE *uci_ulsch_matlab_vec = NULL;
  int file_offset = 0;

  double DS_TDL = .03;
  int ibwps=24;
  int ibwp_rboffset=41;
  int params_from_file = 0;
  int threadCnt=0;
  int max_ldpc_iterations = 5;
  int num_antennas_per_thread = 1;
  uint32_t log_format = 0;
  int threequarter_fs = 0;

  if ((uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY)) == 0) {
    exit_fun("[NR_ULSIM] Error, configuration module init failed\n");
  }
  int ul_proc_error = 0; // uplink processing checking status flag
  //logInit();
  randominit();

  /* initialize the sin-cos table */
  InitSinLUT();

  int c;
  bool setAffinity=false;
  char gNBthreads[128]="n";
  int use_cuda = 0;

  void *h_tx_sig_pinned = NULL;

#ifdef CHANNEL_SIM_CUDA
  void *d_tx_sig = NULL, *d_intermediate_sig = NULL, *d_final_output = NULL;
  void *d_curand_states = NULL;
  void *h_final_output_pinned = NULL;
  float *h_channel_coeffs = NULL;
  void *d_channel_coeffs_gpu = NULL;
#endif

  while ((c = getopt(argc, argv, "--:O:a:b:c:d:ef:g:h:i:jk:l:m:n:o::p:q:r:s:t:u:v:w:y:z:A:C:F:G:H:I:M:N:PR:S:T:U:L:ZW:E:X:Y:"))
         != -1) {
    /* ignore long options starting with '--', option '-O' and their arguments that are handled by configmodule */
    /* with this opstring getopt returns 1 for non-option arguments, refer to 'man 3 getopt' */
    if (c == 1 || c == '-' || c == 'O')
      continue;

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
#ifdef CHANNEL_SIM_CUDA
      if (strcmp(optarg, "cuda") == 0) {
        use_cuda = 1;
      } else
#endif
      {
        printf("Unsupported flag '%s' for -f. Run '-h' to see the list of available options.\n", optarg);
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

    case 'j':
      log_format |= MATLAB_RAW;
      break;

    case 'k':
      printf("Setting threequarter_fs_flag\n");
      threequarter_fs = 1;
      break;

    case 'l':
      length_dmrs = atoi(optarg);
      AssertFatal(length_dmrs == 1 || length_dmrs == 2, "Illegal PUSCH DMRS length %d\n", length_dmrs);
      printf("PUSCH DMRS length %d\n", length_dmrs);
      break;

    case 'm':
      Imcs = atoi(optarg);
      break;

    case 'o':
      uci_on_pusch = true;
      // UCI on PUSCH is not implemented in OAI gNB yet.
      // So this flag is needed to verify in MATLAB
      no_phase_pre_comp = true;
      if (optarg) { // -o with file input: use matlab vector
        uci_ulsch_matlab_vec = fopen(optarg, "rb");
        if (uci_ulsch_matlab_vec == NULL) {
          printf("Error opening %s\n", optarg);
          exit(-1);
        }
      }
      break;

    case 'W':
      precod_nbr_layers = atoi(optarg);
      AssertFatal(precod_nbr_layers > 0 && precod_nbr_layers <= 4,
                  "Number of layers per UE %d should be less than or equal to 4\n",
                  precod_nbr_layers);
      break;

    case 'n':
      n_trials = atoi(optarg);
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

    case 'A':
      num_antennas_per_thread = atoi(optarg);
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
      cpu_meas_enabled = 1;
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

    case 'Y':
      threadCnt = sizeof(gNBthreads)-1;
      strncpy(gNBthreads, optarg, threadCnt);
      gNBthreads[threadCnt]=0;
      setAffinity=true;
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
      printf("-f <flag> Enable optional feature flag. Available flags:\n");
#ifdef CHANNEL_SIM_CUDA
      printf("          cuda    Enable CUDA channel simulation\n");
#else
      printf("          (none)  No optional features were compiled into this executable\n");
#endif
      printf("-g Channel model configuration. Arguments list: Number of arguments = 3, {Channel model: [A] TDLA30, [B] TDLB100, [C] TDLC300}, {Correlation: [l] Low, [m] Medium, [h] High}, {Maximum Doppler shift} e.g. -g A,l,10\n");
      printf("-h This message\n");
      printf("-i Change channel estimation technique. Arguments list: Number of arguments=2, Frequency domain {0:Linear interpolation, 1:PRB based averaging}, Time domain {0:Estimates of last DMRS symbol, 1:Average of DMRS symbols}. e.g. -i 1,0\n");
      printf("-j Save signal buffers in binary format.");
      printf("-k 3/4 sampling\n");
      printf("-l PUSCH DMRS length: 1 or 2\n");
      printf("-m MCS value\n");
      printf("-n Number of trials to simulate\n");
      printf("-o Enable UCI on PUSCH. Optionally accepts input file (without space). This feature is not yet available in gNB so only used to verify with MATLAB generated vector\n");
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
      printf("-A Number of antennas per thread for PUSCH channel estimation\n");
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

  AssertFatal(precod_nbr_layers <= min(n_tx, n_rx),
              "Number of layers %d cannot be more than min(n_tx %d, n_rx %d)\n",
              precod_nbr_layers,
              n_tx,
              n_rx);

  logInit();
  set_glog(loglvl);

  get_softmodem_params()->phy_test = 1;
  get_softmodem_params()->do_ra = 0;

  if (snr1set == 0)
    snr1 = snr0 + 10;

  double sampling_frequency, tx_bandwidth, rx_bandwidth;
  get_samplerate_and_bw(mu,
                        N_RB_DL,
                        threequarter_fs,
                        &sampling_frequency,
                        &tx_bandwidth,
                        &rx_bandwidth);

  RC.gNB = (PHY_VARS_gNB **)malloc_or_fail(sizeof(PHY_VARS_gNB *));
  RC.gNB[0] = calloc_or_fail(1, sizeof(PHY_VARS_gNB));
  gNB = RC.gNB[0];
  gNB->ofdm_offset_divisor = UINT_MAX;
  gNB->num_pusch_symbols_per_thread = 1;
  gNB->dmrs_num_antennas_per_thread = num_antennas_per_thread;
  gNB->RU_list[0] = calloc_or_fail(1, sizeof(**gNB->RU_list));
  gNB->RU_list[0]->rfdevice.openair0_cfg = openair0_cfg_g;

  if (setAffinity == false)
    initFloatingCoresTpool(threadCnt, &gNB->threadPool, false, "gNB-tpool");
  else
    initNamedTpool(gNBthreads, &gNB->threadPool, true, "gNB-tpool");

  NR_UL_IND_t UL_INFO = {0};
  UL_INFO.crc_ind.crc_list = UL_INFO.crc_pdu_list;
  UL_INFO.rx_ind.pdu_list = UL_INFO.rx_pdu_list;
  UL_INFO.rx_ind.number_of_pdus = 0;
  UL_INFO.crc_ind.number_crcs = 0;
  gNB->max_ldpc_iterations = max_ldpc_iterations;
  gNB->pusch_thres = -20;
  gNB->frame_parms.N_RB_DL = N_RB_DL;
  gNB->frame_parms.N_RB_UL = N_RB_UL;
  gNB->frame_parms.Ncp = extended_prefix_flag ? NR_EXTENDED : NR_NORMAL;

  AssertFatal((gNB->if_inst = NR_IF_Module_init(0)) != NULL, "Cannot register interface");
  gNB->if_inst->NR_PHY_config_req = nr_phy_config_request;

  s_interleaved = malloc_or_fail(n_tx * sizeof(float *));
  r_re = malloc_or_fail(n_rx * sizeof(float *));
  r_im = malloc_or_fail(n_rx * sizeof(float *));

  NR_ServingCellConfigCommon_t *scc = calloc_or_fail(1, sizeof(*scc));
  prepare_scc(scc);
  uint64_t ssb_bitmap;
  fill_scc_sim(scc, &ssb_bitmap, N_RB_DL, N_RB_DL, mu, mu);
  fix_scc(scc,ssb_bitmap);

  frame_structure_t frame_structure = {0};
  frame_type_t frame_type = TDD;
  config_frame_structure(mu,
                         scc->tdd_UL_DL_ConfigurationCommon,
                         get_tdd_period_idx(scc->tdd_UL_DL_ConfigurationCommon),
                         frame_type,
                         &frame_structure);
  AssertFatal(is_ul_slot(slot, &frame_structure), "The slot selected is not UL. Can't run ULSIM\n");

  // TODO do a UECAP for phy-sim
  const nr_mac_config_t conf = {.pdsch_AntennaPorts = {.N1 = 1, .N2 = 1, .XP = 1},
                                .pusch_AntennaPorts = n_rx,
                                .minRXTXTIME = 0,
                                .do_CSIRS = 0,
                                .do_SRS = 0,
                                .num_dlharq = 16,
                                .num_ulharq = 16,
                                .force_256qam_off = false,
                                .timer_config.sr_ProhibitTimer = 0,
                                .timer_config.sr_TransMax = 64,
                                .timer_config.sr_ProhibitTimer_v1700 = 0,
                                .timer_config.t300 = 400,
                                .timer_config.t301 = 400,
                                .timer_config.t310 = 2000,
                                .timer_config.n310 = 10,
                                .timer_config.t311 = 3000,
                                .timer_config.n311 = 1,
                                .timer_config.t319 = 400,
                                .spatial_stream_index = {0, 1, 2, 3, 4, 5, 6, 7}};
  const nr_rlc_configuration_t rlc_config = {
    .srb = {
      .t_poll_retransmit = 45,
      .t_reassembly = 35,
      .t_status_prohibit = 0,
      .poll_pdu = -1,
      .poll_byte = -1,
      .max_retx_threshold = 8,
      .sn_field_length = 12,
    },
    .drb_am = {
      .t_poll_retransmit = 45,
      .t_reassembly = 15,
      .t_status_prohibit = 15,
      .poll_pdu = 64,
      .poll_byte = 1024 * 500,
      .max_retx_threshold = 32,
      .sn_field_length = 18,
    },
    .drb_um = {
      .t_reassembly = 15,
      .sn_field_length = 12,
    }
  };

  RC.nb_nr_macrlc_inst = 1;
  nr_cell_sched_t *cell;
  mac_top_init_gNB(ngran_gNB, scc, &conf, &rlc_config, &cell);
  gNB_MAC_INST *nrmac = RC.nrmac[0];
  cell->beam_info = (NR_beam_info_t){.beams_per_period = 1};
  nr_mac_config_scc(nrmac, cell, scc, &conf);

  NR_UE_NR_Capability_t* UE_Capability_nr = CALLOC(1,sizeof(NR_UE_NR_Capability_t));
  prepare_sim_uecap(UE_Capability_nr, scc, mu, N_RB_UL, 0, mcs_table);
  rnti_t rnti = 0x1234;
  int uid = 0;
  int ssb_index = 0;
  NR_CellGroupConfig_t *secondaryCellGroup = get_default_secondaryCellGroup(scc, UE_Capability_nr, 0, 1, &conf, cell, uid, ssb_index);
  secondaryCellGroup->spCellConfig->reconfigurationWithSync = get_reconfiguration_with_sync(rnti, uid, scc, frame);

  NR_BCCH_BCH_Message_t *mib = get_new_MIB_NR(scc);

  // UE dedicated configuration
  nr_mac_add_test_ue(nrmac, cell, rnti, secondaryCellGroup);
  gNB->frame_parms.nb_antennas_tx = 1;
  gNB->frame_parms.nb_antennas_rx = n_rx;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  cfg->carrier_config.num_tx_ant.value = 1;
  cfg->carrier_config.num_rx_ant.value = n_rx;

//  nr_phy_config_request_sim(gNB,N_RB_DL,N_RB_DL,mu,0,0x01);
  gNB->chest_freq = chest_type[0];
  gNB->chest_time = chest_type[1];

  phy_init_nr_gNB(gNB);
  /* RU handles rxdataF, and gNB just has a pointer. Here, we don't have an RU,
   * so we need to allocate that memory as well. */
  for (i = 0; i < n_rx; i++)
    gNB->common_vars.rxdataF[i] = malloc16_clear(gNB->frame_parms.samples_per_frame_wCP * sizeof(int32_t));
  N_RB_DL = gNB->frame_parms.N_RB_DL;

  /* no RU: need to have rxdata */
  c16_t **rxdata;
  rxdata = malloc_or_fail(n_rx * sizeof(*rxdata));
  for (int i = 0; i < n_rx; ++i)
    rxdata[i] = calloc_or_fail(gNB->frame_parms.samples_per_frame, sizeof(**rxdata));

  NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[0];

  // Configure channel model
  UE2gNB = new_channel_desc_scm(n_tx,
                                n_rx,
                                channel_model,
                                sampling_frequency / 1e6,
                                gNB->frame_parms.ul_CarrierFreq,
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

  const int num_samples_alloc = 153600;
#ifdef CHANNEL_SIM_CUDA
  init_cuda_chsim_buffers(use_cuda,
                          n_tx,
                          n_rx,
                          &d_tx_sig,
                          &d_intermediate_sig,
                          &d_final_output,
                          &d_curand_states,
                          &h_tx_sig_pinned,
                          &h_final_output_pinned,
                          &d_channel_coeffs_gpu);
  if (use_cuda) {
    int num_links = n_tx * n_rx;
    h_channel_coeffs = (float *)malloc_or_fail(num_links * UE2gNB->channel_length * sizeof(float2));
  }
#endif

#if !defined(CHANNEL_SIM_CUDA) || !use_cuda
  printf("Pre-allocating padded host memory for the CPU channel pipeline...\n");
  const int max_padding_alloc = 256 - 1;
  size_t padded_tx_alloc_bytes = n_tx * (num_samples_alloc + max_padding_alloc) * 2 * sizeof(float);
  h_tx_sig_pinned = malloc_or_fail(padded_tx_alloc_bytes);
  if (h_tx_sig_pinned == NULL) {
    printf("Error: Failed to allocate host buffer for CPU path\n");
    exit(-1);
  }
#endif

  // Configure UE
  nrPHY_vars_UE_g = malloc_or_fail(sizeof(PHY_VARS_NR_UE **));
  nrPHY_vars_UE_g[0] = malloc_or_fail(sizeof(PHY_VARS_NR_UE *));
  PHY_VARS_NR_UE *UE = calloc_or_fail(1, sizeof(PHY_VARS_NR_UE));
  nrPHY_vars_UE_g[0][0] = UE;
  UE->frame_parms = gNB->frame_parms;
  UE->frame_parms.nb_antennas_tx = n_tx;
  UE->frame_parms.nb_antennas_rx = 0;
  UE->nrLDPC_coding_interface = gNB->nrLDPC_coding_interface;

  if (init_nr_ue_signal(UE, 1) != 0) {
    printf("Error at UE NR initialisation\n");
    exit(-1);
  }

  init_nr_ue_transport(UE);

  //Configure UE
  NR_UE_MAC_INST_t* UE_mac = nr_l2_init_ue(0, mu);

  ue_init_config_request(UE_mac, get_slots_per_frame_from_scs(mu));
  
  UE->if_inst = nr_ue_if_module_init(0);
  UE->if_inst->scheduled_response = nr_ue_scheduled_response;
  UE->if_inst->phy_config_request = nr_ue_phy_config_request;
  UE->if_inst->dl_indication = nr_ue_dl_indication;
  UE->if_inst->ul_indication = nr_ue_ul_indication;
  UE->no_phase_pre_comp = no_phase_pre_comp;

  UE_mac->if_module = nr_ue_if_module_init(0);

  initFloatingCoresTpool(threadCnt, &nrUE_params.Tpool, false, "UE-tpool");

  nr_ue_phy_config_request(&UE_mac->phy_config);

  unsigned char harq_pid = 0;

  NR_gNB_ULSCH_t *ulsch_gNB = &gNB->ulsch[UE_id];

  NR_Sched_Rsp_t *Sched_INFO = malloc16_clear(sizeof(*Sched_INFO));
  memset((void*)Sched_INFO,0,sizeof(*Sched_INFO));
  nfapi_nr_ul_tti_request_t *UL_tti_req = &Sched_INFO->UL_tti_req;

  time_stats_t channel_stats = {0};
  time_stats_t noise_stats = {0};
  time_stats_t pipeline_stats = {0};
  init_sorted_list_meas(&gNB->phy_proc_rx, max_rounds * n_trials);

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

  uint16_t l_prime_mask = get_l_prime(nb_symb_sch, mapping_type, add_pos, length_dmrs, start_symbol, NR_MIB__dmrs_TypeA_Position_pos2);
  int number_dmrs_symbols = count_bits64_with_mask(l_prime_mask, start_symbol, nb_symb_sch);
  uint8_t  nb_re_dmrs = (dmrs_config_type == pusch_dmrs_type1) ? 6 : 4;

  uint32_t tbslbrm = 0;
  if (ilbrm)
    tbslbrm = nr_compute_tbslbrm(mcs_table, N_RB_UL, precod_nbr_layers);

  if ((UE->frame_parms.nb_antennas_tx==4)&&(precod_nbr_layers==4))
    num_dmrs_cdm_grps_no_data = 2;

  if (transform_precoding == transformPrecoder_enabled) {

    AssertFatal(enable_ptrs == 0, "PTRS NOT SUPPORTED IF TRANSFORM PRECODING IS ENABLED\n");

    int index = get_index_for_dmrs_lowpapr_seq((NR_NB_SC_PER_RB / 2) * nb_rb);
    AssertFatal(index >= 0, "Num RBs not configured according to 3GPP 38.211 section 6.3.1.4. For PUSCH with transform precoding, num RBs cannot be multiple of any other primenumber other than 2,3,5\n");

    dmrs_config_type = pusch_dmrs_type1;
    nb_re_dmrs = 6;

    printf("[ULSIM]: TRANSFORM PRECODING ENABLED. Num RBs: %d, index for DMRS_SEQ: %d\n", nb_rb, index);
  }

  nb_re_dmrs = nb_re_dmrs * num_dmrs_cdm_grps_no_data;
  unsigned int TBS = nr_compute_tbs(mod_order, code_rate, nb_rb, nb_symb_sch, nb_re_dmrs * number_dmrs_symbols, 0, 0, precod_nbr_layers);
  
  printf("[ULSIM]: length_dmrs: %u, l_prime_mask: %u	number_dmrs_symbols: %u, mapping_type: %u add_pos: %d \n",
         length_dmrs,
         l_prime_mask,
         number_dmrs_symbols,
         mapping_type,
         add_pos);
  printf("[ULSIM]: CDM groups: %u, dmrs_config_type: %d, num_rbs: %u, nb_symb_sch: %u, start_symbol %u\n",
         num_dmrs_cdm_grps_no_data,
         dmrs_config_type,
         nb_rb,
         nb_symb_sch,
         start_symbol);
  printf("[ULSIM]: MCS: %d, mod order: %u, code_rate: %u\n", Imcs, mod_order, code_rate);

  uint8_t ulsch_input_buffer[TBS/8];

  ulsch_input_buffer[0] = 0x31;
  for (i = 1; i < TBS/8; i++) {
    ulsch_input_buffer[i] = (uint8_t)rand();
  }

  uint8_t ptrs_time_density = get_L_ptrs(ptrs_mcs1, ptrs_mcs2, ptrs_mcs3, Imcs, mcs_table);
  uint8_t ptrs_freq_density = get_K_ptrs(n_rb0, n_rb1, nb_rb);

  double ts = 1.0 / (gNB->frame_parms.subcarrier_spacing * gNB->frame_parms.ofdm_symbol_size);

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
    ptrsSymPos = get_ptrs_symb_idx(nb_symb_sch, start_symbol, 1 << ptrs_time_density, l_prime_mask);
    ptrsSymbPerSlot = get_ptrs_symbols_in_slot(ptrsSymPos, start_symbol, nb_symb_sch);
    ptrsRePerSymb = ((nb_rb + ptrs_freq_density - 1) / ptrs_freq_density);
    unav_res = ptrsSymbPerSlot * ptrsRePerSymb;
    LOG_D(PHY, "[ULSIM] PTRS Symbols in a slot: %2u, RE per Symbol: %3u, RE in a slot %4d\n", ptrsSymbPerSlot, ptrsRePerSymb, unav_res);
  }

  unsigned int available_bits = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, number_dmrs_symbols, unav_res, mod_order, precod_nbr_layers);
  uint8_t cw_buf[available_bits];
  memset(cw_buf, 0, available_bits);
  UE->phy_sim_test_buf = calloc_or_fail(1, (available_bits + 7) / 8);
  printf("[ULSIM]: VALUE OF G: %u, TBS: %u\n", available_bits, TBS);

  int frame_length_complex_samples = gNB->frame_parms.samples_per_subframe * NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
  for (int aatx = 0; aatx < n_tx; aatx++) {
    s_interleaved[aatx] = calloc_or_fail(1, frame_length_complex_samples * 2 * sizeof(float));
  }

  for (int aarx = 0; aarx < n_rx; aarx++) {
    r_re[aarx] = calloc_or_fail(1, frame_length_complex_samples * sizeof(float));
    r_im[aarx] = calloc_or_fail(1, frame_length_complex_samples * sizeof(float));
  }

  //for (int i=0;i<16;i++) printf("%f\n",gaussdouble(0.0,1.0));
  int read_errors=0;

  int slot_offset = get_samples_slot_timestamp(&gNB->frame_parms, slot);
  int slot_length = slot_offset - get_samples_slot_timestamp(&gNB->frame_parms, slot - 1);

  if (input_fd != NULL)	{
    // 800 samples is N_TA_OFFSET for FR1 @ 30.72 Ms/s,
    AssertFatal(gNB->frame_parms.subcarrier_spacing == 30000,
                "only 30 kHz for file input for now (%d)\n",
                gNB->frame_parms.subcarrier_spacing);

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
    for (int irx = 0; irx < gNB->frame_parms.nb_antennas_rx; irx++) {
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
      free(s_interleaved);
      free(r_re);
      free(r_im);
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
  int srs_ret = do_SRS;
  init_sorted_list_meas(&gNB->phy_proc_tx, 4 * n_trials);
  for (SNR = snr0; SNR <= snr1 && !stop; SNR += snr_step) {

    int error_flag = 0;
    n_false_positive = 0;
    effRate = 0;
    effTP = 0;
    roundStats = 0;
    reset_meas(&gNB->phy_proc_rx);
    reset_meas(&gNB->rx_pusch_stats);
    reset_meas(&gNB->rx_pusch_init_stats);
    reset_meas(&gNB->rx_pusch_symbol_processing_stats);
    reset_meas(&gNB->pusch_extraction_stats);
    reset_meas(&gNB->pusch_channel_compensation_stats);
    reset_meas(&gNB->ulsch_llr_stats);
    reset_meas(&gNB->ulsch_layer_demapping_stats);
    reset_meas(&gNB->ulsch_unscrambling_stats);
    reset_meas(&gNB->ulsch_decoding_stats);
    reset_meas(&gNB->ts_ldpc_decode);
    reset_meas(&gNB->ulsch_channel_estimation_stats);
    reset_meas(&gNB->pusch_channel_estimation_antenna_processing_stats);
    reset_meas(&gNB->rx_srs_stats);
    reset_meas(&gNB->generate_srs_stats);
    reset_meas(&gNB->get_srs_signal_stats);
    reset_meas(&gNB->srs_channel_estimation_stats);
    reset_meas(&gNB->srs_timing_advance_stats);
    reset_meas(&gNB->srs_report_tlv_stats);
    reset_meas(&gNB->srs_beam_report_stats);
    reset_meas(&gNB->srs_iq_matrix_stats);
    init_nr_ue_phy_cpu_stats(&UE->phy_cpu_stats);

    uint32_t errors_scrambling[16] = {0};
    int n_errors[16] = {0};
    int round_trials[16] = {0};
    double blerStats[16] = {0};
    double berStats[16] = {0};

    uint64_t sum_pusch_delay = 0;
    int min_pusch_delay = INT_MAX;
    int max_pusch_delay = INT_MIN;
    int delay_pusch_est_count = 0;

    int64_t sum_srs_snr = 0;
    int srs_snr_count = 0;

    for (trial = 0; trial < n_trials && !stop; trial++) {

      uint8_t round = 0;
      crc_status = 1;
      errors_decoding = 0;
      NR_gNB_PUSCH *pusch_vars = &gNB->pusch_vars[UE_id];

      while (round < max_rounds && crc_status && !stop) {

        round_trials[round]++;
        rv_index = nr_get_rv(round % 4);

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
        pusch_pdu->pusch_ptrs.ptrs_ports_list = (nfapi_nr_ptrs_ports_t *)malloc_or_fail(2 * sizeof(nfapi_nr_ptrs_ports_t));
        pusch_pdu->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset = 0;
        pusch_pdu->pusch_ptrs.num_ptrs_ports = 1;
        pusch_pdu->maintenance_parms_v3.ldpcBaseGraph = get_BG(TBS, code_rate);
        pusch_pdu->param_v4.numSpatialStreamIndices = conf.pusch_AntennaPorts;
        memcpy(pusch_pdu->param_v4.spatialStreamIndices, conf.spatial_stream_index, sizeof(conf.spatial_stream_index));

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
          srs_pdu->subcarrier_spacing = gNB->frame_parms.subcarrier_spacing;
          srs_pdu->num_ant_ports = n_tx == 4 ? 2 : n_tx == 2 ? 1 : 0;
          srs_pdu->sequence_id = 40;
          srs_pdu->time_start_position = gNB->frame_parms.symbols_per_slot - 1;
          srs_pdu->config_index = rrc_get_max_nr_csrs(srs_pdu->bwp_size, srs_pdu->bandwidth_index);
          srs_pdu->resource_type = NR_SRS_Resource__resourceType_PR_periodic;
          srs_pdu->t_srs = 1;
          srs_pdu->srs_parameters_v4.srs_bandwidth_size = m_SRS[srs_pdu->config_index];
          srs_pdu->srs_parameters_v4.usage = 1 << NFAPI_NR_SRS_BEAMMANAGEMENT; // to get SNR
          srs_pdu->srs_parameters_v4.report_type[0] = 1;
          srs_pdu->srs_parameters_v4.iq_representation = 1;
          srs_pdu->srs_parameters_v4.prg_size = 1;
          srs_pdu->srs_parameters_v4.num_total_ue_antennas = 1 << srs_pdu->num_ant_ports;
          srs_pdu->srs_parameters_v4.num_ul_spatial_streams_ports = n_rx;
          srs_pdu->beamforming.num_prgs = m_SRS[srs_pdu->config_index];
          srs_pdu->beamforming.prg_size = 1;
        }

        // Fill FAPI PUSCH groups for 1 UE
        UL_tti_req->n_group = 1;
        nfapi_nr_ul_tti_request_number_of_groups_t *group = &UL_tti_req->groups_list[0];
        group->n_ue = 1;
        group->ue_list[0].pdu_idx = 0;

        /* load FAPI into RX of L1 */
        nr_save_ul_tti_req(gNB, &Sched_INFO->UL_tti_req);

        /// UE UL PDUs

        UE->ul_harq_processes[harq_pid].round = round;
        UE_proc.nr_slot_tx = slot;
        UE_proc.frame_tx = frame;
        UE_proc.gNB_id = 0;

        // --------- setting parameters for UE --------
        nr_scheduled_response_t scheduled_response = {.ul_config = &ul_config, .phy_data = (void *)&phy_data};

        ul_config.slot = slot;
        ul_config.number_pdus = do_SRS == 1 ? 2 : 1;

        fapi_nr_ul_config_request_pdu_t *ul_config0 = &ul_config.ul_config_list[0];
        ul_config0->pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
        nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu = &ul_config0->pusch_config_pdu;
        // Config UL TX PDU
        pusch_config_pdu->tx_request_body.fapiTxPdu = ulsch_input_buffer;
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
        pusch_config_pdu->target_code_rate = code_rate;
        pusch_config_pdu->tbslbrm = tbslbrm;
        pusch_config_pdu->ldpcBaseGraph = get_BG(TBS, code_rate);
        pusch_config_pdu->pusch_data.tb_size = TBS / 8;
        pusch_config_pdu->pusch_data.new_data_indicator = round == 0 ? true : false;
        pusch_config_pdu->pusch_data.rv_index = rv_index;
        pusch_config_pdu->pusch_data.harq_process_id = harq_pid;
        pusch_config_pdu->pusch_ptrs.ptrs_time_density = ptrs_time_density;
        pusch_config_pdu->pusch_ptrs.ptrs_freq_density = ptrs_freq_density;
        pusch_config_pdu->pusch_ptrs.ptrs_ports_list =
            (nfapi_nr_ue_ptrs_ports_t *)malloc_or_fail(2 * sizeof(nfapi_nr_ue_ptrs_ports_t));
        pusch_config_pdu->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset = 0;
        pusch_config_pdu->pusch_ptrs.num_ptrs_ports = 1;
        pusch_config_pdu->transform_precoding = transform_precoding;
        // if transform precoding is enabled
        if (transform_precoding == transformPrecoder_enabled) {
          pusch_config_pdu->dfts_ofdm.low_papr_group_number = *scc->physCellId % 30; // U as defined in 38.211 section 6.4.1.1.1.2
          pusch_config_pdu->dfts_ofdm.low_papr_sequence_number = 0; // V as defined in 38.211 section 6.4.1.1.1.2
          // pusch_config_pdu->pdu_bit_map |= PUSCH_PDU_BITMAP_DFTS_OFDM;
          pusch_config_pdu->num_dmrs_cdm_grps_no_data = num_dmrs_cdm_grps_no_data;
        }
        if (uci_on_pusch) {
          const nfapi_nr_ue_pusch_uci_t pusch_uci = {
              .alpha_scaling = 3,
              .beta_offset_csi1 = 13,
              .beta_offset_csi2 = 13,
              .beta_offset_harq_ack = 11,
              .harq_ack_bit_length = 3,
              .harq_payload = 3,
              .csi_payload = {.p1_bits = 4, .part1_payload = 15, .p2_bits = 4, .part2_payload = 15}};
          pusch_config_pdu->pusch_uci = pusch_uci;
          prepare_ue_pusch_pdu_from_matlab_vector(uci_on_pusch, uci_ulsch_matlab_vec, pusch_config_pdu, cw_buf);
        }

        if (do_SRS == 1) {
          fapi_nr_ul_config_request_pdu_t *ul_config1 = &ul_config.ul_config_list[1];
          ul_config1->pdu_type = FAPI_NR_UL_CONFIG_TYPE_SRS;
          fapi_nr_ul_config_srs_pdu *srs_config_pdu = &ul_config1->srs_config_pdu;
          memset(srs_config_pdu, 0, sizeof(fapi_nr_ul_config_srs_pdu));
          srs_config_pdu->rnti = n_rnti;
          srs_config_pdu->bwp_size = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
          srs_config_pdu->bwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
          srs_config_pdu->subcarrier_spacing = gNB->frame_parms.subcarrier_spacing;
          srs_config_pdu->num_ant_ports = n_tx == 4 ? 2 : n_tx == 2 ? 1 : 0;
          srs_config_pdu->config_index = rrc_get_max_nr_csrs(srs_config_pdu->bwp_size, srs_config_pdu->bandwidth_index);
          srs_config_pdu->sequence_id = 40;
          srs_config_pdu->resource_type = NR_SRS_Resource__resourceType_PR_periodic;
          srs_config_pdu->t_srs = 1;
          srs_config_pdu->time_start_position = gNB->frame_parms.symbols_per_slot - 1;
        }

        for (int i = 0; i < (TBS / 8); i++)
          UE->ul_harq_processes[harq_pid].payload_AB[i] = i & 0xff;

        if (input_fd == NULL) {
          // set FAPI parameters for UE, put them in the scheduled response and call
          nr_ue_scheduled_response(&scheduled_response);

          /////////////////////////phy_procedures_nr_ue_TX///////////////////////
          ///////////
          int slot_start = get_samples_slot_timestamp(&UE->frame_parms, slot);
          c16_t *tx[UE->frame_parms.nb_antennas_tx];
          for (int i = 0; i < UE->frame_parms.nb_antennas_tx; i++)
            tx[i] = UE->common_vars.txData[i] + slot_start;
          phy_procedures_nrUE_TX(UE, &UE_proc, &phy_data, tx);

          if (n_trials == 1) {
            LOG_M("txsig0.m", "txs0", &UE->common_vars.txData[0][slot_offset], slot_length, 1, 1 | log_format);
            if (precod_nbr_layers > 1) {
              LOG_M("txsig1.m", "txs1", &UE->common_vars.txData[1][slot_offset], slot_length, 1, 1 | log_format);
              if (precod_nbr_layers == 4) {
                LOG_M("txsig2.m", "txs2", &UE->common_vars.txData[2][slot_offset], slot_length, 1, 1 | log_format);
                LOG_M("txsig3.m", "txs3", &UE->common_vars.txData[3][slot_offset], slot_length, 1, 1 | log_format);
              }
            }
          }
          ///////////
          ////////////////////////////////////////////////////
          // Compute transmitter energy level
          tx_offset = get_samples_slot_timestamp(&gNB->frame_parms, slot);
          int symbol_offset = tx_offset + 5 * gNB->frame_parms.ofdm_symbol_size + 4 * gNB->frame_parms.nb_prefix_samples
                              + gNB->frame_parms.nb_prefix_samples0;
          int symbol_length = gNB->frame_parms.ofdm_symbol_size + gNB->frame_parms.nb_prefix_samples;
          txlev_sum = compute_tx_energy_level(UE->common_vars.txData,
                                              UE->frame_parms.nb_antennas_tx,
                                              symbol_offset,
                                              symbol_length,
                                              n_trials);
        } else
          n_trials = 1;

        if (input_fd == NULL) {
          double sigma =
              compute_noise_variance(txlev_sum, gNB->frame_parms.ofdm_symbol_size, nb_rb, precod_nbr_layers, SNR, n_trials);

          for (int aa = 0; aa < UE->frame_parms.nb_antennas_tx; aa++) {
            for (i = 0; i < slot_length; i++) {
              s_interleaved[aa][2 * i] = (float)UE->common_vars.txData[aa][slot_offset + i].r;
              s_interleaved[aa][2 * i + 1] = (float)UE->common_vars.txData[aa][slot_offset + i].i;
            }
          }

          const int padding_len = UE2gNB->channel_length - 1;
          const int padded_slot_length = slot_length + padding_len;
          float *h_tx_ptr = (float *)h_tx_sig_pinned;
          size_t total_padded_bytes_for_slot = n_tx * padded_slot_length * 2 * sizeof(float);
          memset(h_tx_ptr, 0, total_padded_bytes_for_slot);

          for (int j = 0; j < n_tx; j++) {
            float *data_start_ptr = h_tx_ptr + (j * padded_slot_length + padding_len) * 2;
            memcpy(data_start_ptr, s_interleaved[j], slot_length * 2 * sizeof(float));
          }

#ifdef CHANNEL_SIM_CUDA
          if (use_cuda) {
#if defined(USE_UNIFIED_MEMORY)
#if defined(CUDA_VERSION) && CUDA_VERSION >= 13000
            struct cudaMemLocation deviceId;
            deviceId.type = cudaMemLocationTypeDevice;
            cudaGetDevice(&deviceId.id);
            const int padding_len = UE2gNB->channel_length - 1;
            const int padded_slot_length = slot_length + padding_len;
            cudaMemPrefetchAsync(d_tx_sig, n_tx * padded_slot_length * 2 * sizeof(float), deviceId, 0, 0);
#else		  
            int deviceId;
            cudaGetDevice(&deviceId);
            const int padding_len = UE2gNB->channel_length - 1;
            const int padded_slot_length = slot_length + padding_len;
            cudaMemPrefetchAsync(d_tx_sig, n_tx * padded_slot_length * 2 * sizeof(float), deviceId, 0);
#endif
#endif
            start_meas(&pipeline_stats);
            random_channel(UE2gNB, 0);
            int num_links = UE2gNB->nb_tx * UE2gNB->nb_rx;
            if (h_channel_coeffs == NULL) {
              h_channel_coeffs = (float *)malloc_or_fail(num_links * 256 * sizeof(float2));
            }

            for (int link = 0; link < num_links; link++) {
              for (int l = 0; l < UE2gNB->channel_length; l++) {
                int idx = link * UE2gNB->channel_length + l;
                ((float2 *)h_channel_coeffs)[idx].x = (float)UE2gNB->ch[link][l].r;
                ((float2 *)h_channel_coeffs)[idx].y = (float)UE2gNB->ch[link][l].i;
              }
            }

            run_channel_pipeline_cuda(rxdata,
                                      n_tx,
                                      n_rx,
                                      UE2gNB->channel_length,
                                      slot_length,
                                      h_channel_coeffs,
                                      (float)sigma,
                                      ts,
                                      pdu_bit_map,
                                      PUSCH_PDU_BITMAP_PUSCH_PTRS,
                                      slot_offset,
                                      delay,
                                      d_tx_sig,
                                      d_intermediate_sig,
                                      d_final_output,
                                      d_curand_states,
                                      h_tx_sig_pinned,
                                      h_final_output_pinned,
                                      d_channel_coeffs_gpu);
            cudaDeviceSynchronize();
            stop_meas(&pipeline_stats);

          } else
#endif
          {
            float **tx_sig_for_cpu = malloc_or_fail(n_tx * sizeof(float *));
            float *h_tx_ptr = (float *)h_tx_sig_pinned;
            const int padding_len = UE2gNB->channel_length - 1;
            const int padded_slot_length = slot_length + padding_len;

            for (int j = 0; j < n_tx; j++) {
              tx_sig_for_cpu[j] = h_tx_ptr + (j * padded_slot_length + padding_len) * 2;
            }

            start_meas(&channel_stats);
            multipath_channel_float(UE2gNB, tx_sig_for_cpu, r_re, r_im, slot_length, 0, (n_trials == 1) ? 1 : 0);
            stop_meas(&channel_stats);

            free(tx_sig_for_cpu);

            bool apply_phase_noise = (pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS);
            start_meas(&noise_stats);
            add_noise_float(rxdata,
                            (const float **)r_re,
                            (const float **)r_im,
                            (float)sigma,
                            slot_length,
                            slot_offset,
                            ts,
                            delay,
                            apply_phase_noise,
                            gNB->frame_parms.nb_antennas_rx);
            stop_meas(&noise_stats);
          }
        }
        /*End input_fd */

        //----------------------------------------------------------
        //------------------- gNB phy procedures -------------------
        //----------------------------------------------------------
        UL_INFO.rx_ind.number_of_pdus = 0;
        UL_INFO.crc_ind.number_crcs = 0;
        UL_INFO.srs_ind.number_of_pdus = 0;

        //----------- OFDM Demodulation and RX rotation--------------------------
        bool was_symbol_used[NR_SYMBOLS_PER_SLOT] = {0};
        for (int i = 0; i < NR_SYMBOLS_PER_SLOT; i++) {
          was_symbol_used[i] = true;
        }
        nr_ofdm_demod_and_rx_rotation(rxdata,
                                      gNB->common_vars.rxdataF,
                                      &gNB->frame_parms,
                                      gNB->frame_parms.nb_antennas_rx,
                                      slot,
                                      0,
                                      link_type_ul,
                                      was_symbol_used);

        ul_proc_error = phy_procedures_gNB_uespec_RX(gNB, frame, slot, &UL_INFO);

        if (n_trials == 1 && round == 0) {
          LOG_M("rxsig0.m", "rx0", &rxdata[0][slot_offset], slot_length, 1, 1 | log_format);
          LOG_M("rxsigF0.m", "rxsF0", gNB->common_vars.rxdataF[0], 14 * gNB->frame_parms.ofdm_symbol_size, 1, 1 | log_format);
          if (precod_nbr_layers > 1) {
            LOG_M("rxsig1.m", "rx1", &rxdata[1][slot_offset], slot_length, 1, 1);
            LOG_M("rxsigF1.m", "rxsF1", gNB->common_vars.rxdataF[1], 14 * gNB->frame_parms.ofdm_symbol_size, 1, 1 | log_format);
            if (precod_nbr_layers == 4) {
              LOG_M("rxsig2.m", "rx2", &rxdata[2][slot_offset], slot_length, 1, 1);
              LOG_M("rxsig3.m", "rx3", &rxdata[3][slot_offset], slot_length, 1, 1);
              LOG_M("rxsigF2.m", "rxsF2", gNB->common_vars.rxdataF[2], 14 * gNB->frame_parms.ofdm_symbol_size, 1, 1 | log_format);
              LOG_M("rxsigF3.m", "rxsF3", gNB->common_vars.rxdataF[3], 14 * gNB->frame_parms.ofdm_symbol_size, 1, 1 | log_format);
            }
          }
        }

        if (n_trials == 1 && round == 0) {
          __attribute__((unused)) int off = ((nb_rb & 1) == 1) ? 4 : 0;

          LOG_M("chestF0.m",
                "chF0",
                &pusch_vars->ul_ch_estimates[0][start_symbol * gNB->frame_parms.ofdm_symbol_size],
                gNB->frame_parms.ofdm_symbol_size,
                1,
                1 | log_format);

          LOG_M("rxsigF0_comp.m",
                "rxsF0_comp",
                &pusch_vars->rxdataF_comp[0][start_symbol * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                1,
                1 | log_format);

          if (precod_nbr_layers == 2) {
            LOG_M("chestF3.m",
                  "chF3",
                  &pusch_vars->ul_ch_estimates[3][start_symbol * gNB->frame_parms.ofdm_symbol_size],
                  gNB->frame_parms.ofdm_symbol_size,
                  1,
                  1 | log_format);

            LOG_M("rxsigF1_comp.m",
                  "rxsF1_comp",
                  &pusch_vars->rxdataF_comp[1][start_symbol * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1 | log_format);
          }

          if (precod_nbr_layers == 4) {
            LOG_M("chestF5.m",
                  "chF5",
                  &pusch_vars->ul_ch_estimates[5][start_symbol * gNB->frame_parms.ofdm_symbol_size],
                  gNB->frame_parms.ofdm_symbol_size,
                  1,
                  1 | log_format);
            LOG_M("chestF10.m",
                  "chF10",
                  &pusch_vars->ul_ch_estimates[10][start_symbol * gNB->frame_parms.ofdm_symbol_size],
                  gNB->frame_parms.ofdm_symbol_size,
                  1,
                  1 | log_format);
            LOG_M("chestF15.m",
                  "chF15",
                  &pusch_vars->ul_ch_estimates[15][start_symbol * gNB->frame_parms.ofdm_symbol_size],
                  gNB->frame_parms.ofdm_symbol_size,
                  1,
                  1 | log_format);

            LOG_M("rxsigF2_comp.m",
                  "rxsF2_comp",
                  &pusch_vars->rxdataF_comp[2][start_symbol * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1 | log_format);
            LOG_M("rxsigF3_comp.m",
                  "rxsF3_comp",
                  &pusch_vars->rxdataF_comp[3][start_symbol * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size))],
                  nb_symb_sch * (off + (NR_NB_SC_PER_RB * pusch_pdu->rb_size)),
                  1,
                  1 | log_format);
          }

          LOG_M("rxsigF0_llr.m",
                "rxsF0_llr",
                &pusch_vars->llr[0],
                precod_nbr_layers * (nb_symb_sch - 1) * NR_NB_SC_PER_RB * pusch_pdu->rb_size * mod_order,
                1,
                0 | log_format);
        }

        if ((ulsch_gNB->last_iteration_cnt >= ulsch_gNB->max_ldpc_iterations) || ul_proc_error == 1) {
          error_flag = uci_on_pusch ? 0 : 1;
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

        if (uci_on_pusch) {
          for (i = 0; i < available_bits; i++) {
            const uint8_t current_bit = (UE->phy_sim_test_buf[i / 8] >> (i & 7)) & 1;
            const uint8_t test_vector_bit = cw_buf[i] & 1;
            if (current_bit != test_vector_bit)
              errors_scrambling[round]++;
          }
        } else {
          for (i = 0; i < available_bits; i++) {
            const uint8_t current_bit = (UE->ul_harq_processes[harq_pid].f[i / 8] >> (i & 7)) & 1;
            if (((current_bit == 0) && (pusch_vars->llr[i] <= 0)) || ((current_bit == 1) && (pusch_vars->llr[i] >= 0))) {
              errors_scrambling[round]++;
            }
          }
        }
        if (uci_on_pusch && uci_ulsch_matlab_vec && (errors_scrambling[round] == 0)) {
          ret = 0;
          printf("*************\n");
          printf("UCI on PUSCH test OK against MATLAB generated codeword\n");
          printf("*************\n");
          break;
        }
        round++;
      } // round

      if (n_trials == 1 && errors_scrambling[0] > 0) {
        printf("\x1B[31m""[frame %d][trial %d]\tnumber of errors in unscrambling = %u\n" "\x1B[0m", frame, trial, errors_scrambling[0]);
      }

      for (i = 0; i < TBS; i++) {
        uint8_t estimated_output_bit = (ulsch_gNB->harq_process->b[i / 8] & (1 << (i & 7))) >> (i & 7);
        uint8_t test_input_bit = (UE->ul_harq_processes[harq_pid].payload_AB[i / 8] & (1 << (i & 7))) >> (i & 7);

        if (estimated_output_bit != test_input_bit) {
          /*if(errors_decoding == 0)
              printf("\x1B[34m""[frame %d][trial %d]\t1st bit in error in decoding     = %d\n" "\x1B[0m", frame, trial, i);*/
          errors_decoding++;
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

      sum_pusch_delay += pusch_vars->delay.est_delay;
      min_pusch_delay = min(pusch_vars->delay.est_delay, min_pusch_delay);
      max_pusch_delay = max(pusch_vars->delay.est_delay, max_pusch_delay);
      delay_pusch_est_count++;

      if (do_SRS == 1) {
        DevAssert(UL_INFO.srs_ind.number_of_pdus == 1); // there must be SRS indication
        const nfapi_nr_srs_indication_pdu_t *srs_ind = &UL_INFO.srs_ind.pdu_list[0];
        DevAssert(srs_ind->srs_usage == NFAPI_NR_SRS_BEAMMANAGEMENT);
        nfapi_nr_srs_beamforming_report_t bf_rep;
        unpack_nr_srs_beamforming_report((void *)srs_ind->report_tlv.value, srs_ind->report_tlv.length, &bf_rep, sizeof(bf_rep));
        DevAssert(bf_rep.wide_band_snr != 0xff);
        int8_t snr = (bf_rep.wide_band_snr >> 1) - 64;
        sum_srs_snr += snr;
        srs_snr_count++;
      }
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

    double av_delay = (double)sum_pusch_delay / (2 * delay_pusch_est_count);
    printf("DMRS-PUSCH delay estimation: min %i, max %i, average %2.1f\n", min_pusch_delay >> 1, max_pusch_delay >> 1, av_delay);

    if (do_SRS == 1) {
      float srs_snr_av = (float)sum_srs_snr / srs_snr_count;
      srs_ret = srs_snr_av >= 0.7 * SNR || srs_snr_av > 30 ? 0 : 1;
      printf("SNR based on SRS: %2.1f dB\n", srs_snr_av);
    }

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
      printf("UE TX\n");
      for (int i = PHY_PROC_TX; i <= OFDM_MOD_STATS; i++) {
        printStatIndent(&UE->phy_cpu_stats.cpu_time_stats[i], UE->phy_cpu_stats.cpu_time_stats[i].meas_name);
      }

      printf("\ngNB RX\n");
      printDistribution(&gNB->phy_proc_rx, "Total PHY proc rx");
      printStatIndent(&gNB->rx_pusch_stats, "RX PUSCH time");
      printStatIndent2(&gNB->ulsch_channel_estimation_stats, "ULSCH channel estimation time");
      printStatIndent3(&gNB->pusch_channel_estimation_antenna_processing_stats, "Antenna Processing time");
      printStatIndent2(&gNB->rx_pusch_init_stats, "RX PUSCH Initialization time");
      printStatIndent2(&gNB->rx_pusch_symbol_processing_stats, "RX PUSCH Symbol Processing time");
      gNB->pusch_extraction_stats.trials = gNB->rx_pusch_symbol_processing_stats.trials;
      printStatIndent3(&gNB->pusch_extraction_stats, "RX PUSCH extraction");
      gNB->pusch_channel_compensation_stats.trials = gNB->rx_pusch_symbol_processing_stats.trials;
      printStatIndent3(&gNB->pusch_channel_compensation_stats, "RX PUSCH channel compensation");
      gNB->ulsch_llr_stats.trials = gNB->rx_pusch_symbol_processing_stats.trials;
      printStatIndent3(&gNB->ulsch_llr_stats, "RX PUSCH LLR");
      gNB->ulsch_layer_demapping_stats.trials = gNB->rx_pusch_symbol_processing_stats.trials;
      printStatIndent3(&gNB->ulsch_layer_demapping_stats, "RX PUSCH layer demapping");
      gNB->ulsch_unscrambling_stats.trials = gNB->rx_pusch_symbol_processing_stats.trials;
      printStatIndent3(&gNB->ulsch_unscrambling_stats, "RX PUSCH unscrambling");
      printStatIndent(&gNB->ulsch_decoding_stats,"ULSCH total decoding time");
      printStatIndent2(&gNB->ts_ldpc_decode, "ULSCH segments decoding time");
      printStatIndent(&gNB->rx_srs_stats,"RX SRS time");
      printStatIndent2(&gNB->generate_srs_stats,"Generate SRS sequence time");
      printStatIndent2(&gNB->get_srs_signal_stats,"Get SRS signal time");
      printStatIndent2(&gNB->srs_channel_estimation_stats,"SRS channel estimation time");
      printStatIndent2(&gNB->srs_timing_advance_stats,"SRS timing advance estimation time");
      printStatIndent2(&gNB->srs_report_tlv_stats,"SRS report TLV build time");
      printStatIndent3(&gNB->srs_beam_report_stats,"SRS beam report build time");
      printStatIndent3(&gNB->srs_iq_matrix_stats,"SRS IQ matrix build time");

      if (use_cuda) {
        printStatIndent(&pipeline_stats, "GPU Channel Pipeline");
      } else {
        printStatIndent(&channel_stats, "Multipath Channel (CPU)");
        printStatIndent(&noise_stats, "Add Noise (CPU)");
      }
      printf("\n");
    }

    if(n_trials==1)
      break;

    if (srs_ret == 0 && (float)effTP >= eff_tp_check) {
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

  free_sorted_list_meas(&gNB->phy_proc_rx);
  free_MIB_NR(mib);

  free_nrLDPC_coding_interface(&gNB->nrLDPC_coding_interface);

  if (output_fd)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  // closing csv file
  if (filename_csv != NULL) { // means we are asked to print stats to CSV
    fclose(csv_file);
    free(filename_csv);
  }

  if (uci_ulsch_matlab_vec)
    fclose(uci_ulsch_matlab_vec);

  free_and_zero(UE->phy_sim_test_buf);

  free(nrPHY_vars_UE_g[0][0]);
  free(nrPHY_vars_UE_g[0]);
  free(nrPHY_vars_UE_g);

#ifdef CHANNEL_SIM_CUDA
  free_cuda_chsim_buffers(use_cuda,
                          &d_tx_sig,
                          &d_intermediate_sig,
                          &d_final_output,
                          &d_curand_states,
                          &h_tx_sig_pinned,
                          &h_final_output_pinned,
                          &h_channel_coeffs,
                          &d_channel_coeffs_gpu);
#endif

  return ret;
}
