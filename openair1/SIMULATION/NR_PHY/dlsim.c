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
#include "common/utils/var_array.h"
#include "common/utils/LOG/log.h"
#include "LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "LAYER2/NR_MAC_UE/mac_defs.h"
#include "LAYER2/NR_MAC_UE/mac_extern.h"
#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/phy_vars_nr_ue.h"
#include "PHY/types.h"
#include "PHY/INIT/nr_phy_init.h"
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
#include "LAYER2/NR_MAC_gNB/mac_rrc_dl_handler.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "NR_asn_constant.h"
#include "RRC/NR/nr_rrc_config.h"
#include "openair1/SIMULATION/RF/rf.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
#include "openair1/SIMULATION/NR_PHY/nr_unitary_defs.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "NR_RRCReconfiguration.h"
#define inMicroS(a) (((double)(a))/(get_cpu_freq_GHz()*1000.0))
#include "SIMULATION/LTE_PHY/common_sim.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"

#include <openair2/RRC/LTE/rrc_vars.h>

#include <executables/softmodem-common.h>
#include <openair3/ocp-gtpu/gtp_itf.h>
#include <executables/nr-uesoftmodem.h>

const char *__asan_default_options()
{
  /* don't do leak checking in nr_ulsim, not finished yet */
  return "detect_leaks=0";
}

PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];
double cpuf;
char *uecap_file;

uint16_t sl_ahead=0;
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

int dummy_nr_ue_ul_indication(nr_uplink_indication_t *ul_info) { return(0);  }

void e1_bearer_context_setup(const e1ap_bearer_setup_req_t *req) { abort(); }
void e1_bearer_context_modif(const e1ap_bearer_setup_req_t *req) { abort(); }
void e1_bearer_release_cmd(const e1ap_bearer_release_cmd_t *cmd) { abort(); }

int8_t nr_rrc_RA_succeeded(const module_id_t mod_id, const uint8_t gNB_index) {
  return 0;
}

void nr_derive_key(int alg_type, uint8_t alg_id, const uint8_t key[32], uint8_t out[16])
{
  (void)alg_type;
}

void processSlotTX(void *arg) {}

nr_bler_struct nr_bler_data[NR_NUM_MCS];

// needed for some functions
openair0_config_t openair0_cfg[MAX_CARDS];
void update_ptrs_config(NR_CellGroupConfig_t *secondaryCellGroup, uint16_t *rbSize, uint8_t *mcsIndex,int8_t *ptrs_arg);
void update_dmrs_config(NR_CellGroupConfig_t *scg, int8_t* dmrs_arg);
extern void fix_scd(NR_ServingCellConfig_t *scd);// forward declaration

/* specific dlsim DL preprocessor: uses rbStart/rbSize/mcs/nrOfLayers from command line of dlsim */
int g_mcsIndex = -1, g_mcsTableIdx = 0, g_rbStart = -1, g_rbSize = -1, g_nrOfLayers = 1, g_pmi = 0;
void nr_dlsim_preprocessor(module_id_t module_id,
                           frame_t frame,
                           sub_frame_t slot) {

  NR_UE_info_t *UE_info = RC.nrmac[module_id]->UE_info.list[0];
  AssertFatal(RC.nrmac[module_id]->UE_info.list[1]==NULL, "can have only a single UE\n");
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl;
  NR_UE_DL_BWP_t *current_BWP = &UE_info->current_DL_BWP;
  NR_ServingCellConfigCommon_t *scc = RC.nrmac[0]->common_channels[0].ServingCellConfigCommon;

  uint8_t nr_of_candidates = 0;
  if (g_mcsIndex < 4) {
    find_aggregation_candidates(&sched_ctrl->aggregation_level,
                                &nr_of_candidates,
                                sched_ctrl->search_space,8);
  }
  if (nr_of_candidates == 0) {
    find_aggregation_candidates(&sched_ctrl->aggregation_level,
                                &nr_of_candidates,
                                sched_ctrl->search_space,4);
  }
  uint32_t Y = get_Y(sched_ctrl->search_space, slot, UE_info->rnti);
  int CCEIndex = find_pdcch_candidate(RC.nrmac[module_id],
                                      /* CC_id = */ 0,
                                      sched_ctrl->aggregation_level,
                                      nr_of_candidates,
                                      &sched_ctrl->sched_pdcch,
                                      sched_ctrl->coreset,
                                      Y);
  AssertFatal(CCEIndex>=0, "%4d.%2d could not find CCE for DL DCI UE %d/RNTI %04x\n", frame, slot, 0, UE_info->rnti);
  sched_ctrl->cce_index = CCEIndex;

  NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
  sched_pdsch->rbStart = g_rbStart;
  sched_pdsch->rbSize = g_rbSize;
  sched_pdsch->mcs = g_mcsIndex;
  sched_pdsch->nrOfLayers = g_nrOfLayers;
  sched_pdsch->pm_index = g_pmi;
  /* the following might override the table that is mandated by RRC
   * configuration */
  current_BWP->mcsTableIdx = g_mcsTableIdx;
  sched_pdsch->time_domain_allocation = get_dl_tda(RC.nrmac[module_id], scc, slot);
  AssertFatal(sched_pdsch->time_domain_allocation >= 0,"Unable to find PDSCH time domain allocation in list\n");

  sched_pdsch->tda_info = get_dl_tda_info(current_BWP,
                                          sched_ctrl->search_space->searchSpaceType->present,
                                          sched_pdsch->time_domain_allocation,
                                          NR_MIB__dmrs_TypeA_Position_pos2,
                                          1,
                                          TYPE_C_RNTI_,
                                          sched_ctrl->coreset->controlResourceSetId,
                                          false);

  sched_pdsch->dmrs_parms = get_dl_dmrs_params(scc,
                                               current_BWP,
                                               &sched_pdsch->tda_info,
                                               sched_pdsch->nrOfLayers);

  sched_pdsch->Qm = nr_get_Qm_dl(sched_pdsch->mcs, current_BWP->mcsTableIdx);
  sched_pdsch->R = nr_get_code_rate_dl(sched_pdsch->mcs, current_BWP->mcsTableIdx);
  sched_pdsch->tb_size = nr_compute_tbs(sched_pdsch->Qm,
                                        sched_pdsch->R,
                                        sched_pdsch->rbSize,
                                        sched_pdsch->tda_info.nrOfSymbols,
                                        sched_pdsch->dmrs_parms.N_PRB_DMRS * sched_pdsch->dmrs_parms.N_DMRS_SLOT,
                                        0 /* N_PRB_oh, 0 for initialBWP */,
                                        0 /* tb_scaling */,
                                        sched_pdsch->nrOfLayers) >> 3;

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
  AssertFatal(current_BWP->mcsTableIdx >= 0 && current_BWP->mcsTableIdx <= 2, "invalid mcsTableIdx %d\n", current_BWP->mcsTableIdx);
}

nrUE_params_t nrUE_params;

nrUE_params_t *get_nrUE_params(void) {
  return &nrUE_params;
}


void validate_input_pmi(nfapi_nr_config_request_scf_t *gNB_config,
                        nr_pdsch_AntennaPorts_t pdsch_AntennaPorts,
                        int nrOfLayers,
                        int pmi)
{
  if (pmi == 0)
    return;

  nfapi_nr_pm_pdu_t *pmi_pdu = &gNB_config->pmi_list.pmi_pdu[pmi - 1]; // pmi 0 is identity matrix
  AssertFatal(pmi == pmi_pdu->pm_idx, "PMI %d doesn't match to the one in precoding matrix %d\n", pmi, pmi_pdu->pm_idx);
  AssertFatal(nrOfLayers == pmi_pdu->numLayers, "Number of layers %d doesn't match to the one in precoding matrix %d for PMI %d\n",
              nrOfLayers, pmi_pdu->numLayers, pmi);
  int num_antenna_ports = pdsch_AntennaPorts.N1 * pdsch_AntennaPorts.N2 * pdsch_AntennaPorts.XP;
  AssertFatal(num_antenna_ports == pmi_pdu->num_ant_ports, "Configured antenna ports %d does not match precoding matrix AP size %d for PMI %d\n",
              num_antenna_ports, pmi_pdu->num_ant_ports, pmi);
}


int NB_UE_INST = 1;
configmodule_interface_t *uniqCfg = NULL;
int main(int argc, char **argv)
{
  FILE *csv_file = NULL;
  char *filename_csv = NULL;
  setbuf(stdout, NULL);
  int c;
  int i,aa;//,l;
  double sigma2, sigma2_dB=10, SNR, snr0=-2.0, snr1=2.0;
  uint8_t snr1set=0;
  float effRate;
  //float psnr;
  float eff_tp_check = 0.7;
  uint32_t TBS = 0;
  c16_t **txdata;
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
  int trial, n_trials = 1, n_false_positive = 0;
  //int n_errors2, n_alamouti;
  uint8_t n_tx=1,n_rx=1;
  uint8_t round;
  uint8_t num_rounds = 4;
  int ldpc_offload_flag = 0;
  char gNBthreads[128]="n";

  channel_desc_t *gNB2UE;
  //uint32_t nsymb,tx_lev,tx_lev1 = 0,tx_lev2 = 0;
  //uint8_t extended_prefix_flag=0;
  //int8_t interf1=-21,interf2=-21;

  FILE *input_fd=NULL,*pbch_file_fd=NULL;
  //char input_val_str[50],input_val_str2[50];

  //uint8_t frame_mod4,num_pdcch_symbols = 0;

  SCM_t channel_model = AWGN; // AWGN Rayleigh1 Rayleigh1_anticorr;
  double DS_TDL = .03;
  int delay = 0;

  //double pbch_sinr;
  //int pbch_tx_ant;
  int N_RB_DL=106,mu=1;

  //unsigned char frame_type = 0;

  int frame=1,slot=1;
  int frame_length_complex_samples;
  //int frame_length_complex_samples_no_prefix;
  NR_DL_FRAME_PARMS *frame_parms;
  UE_nr_rxtx_proc_t UE_proc;
  NR_Sched_Rsp_t *Sched_INFO;
  gNB_MAC_INST *gNB_mac;
  NR_UE_MAC_INST_t *UE_mac;
  int cyclic_prefix_type = NFAPI_CP_NORMAL;
  int run_initial_sync=0;
  int loglvl=OAILOG_WARNING;

  //float target_error_rate = 0.01;
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
  int chest_type[2] = {0};
  uint8_t  max_ldpc_iterations = 5;
  if ((uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY)) == 0) {
    exit_fun("[NR_DLSIM] Error, configuration module init failed\n");
  }

  randominit(0);

  int print_perf = 0;

  FILE *scg_fd=NULL;

  while ((c = getopt(argc, argv, "f:hA:p:f:g:i:n:s:S:t:v:x:y:z:o:M:N:F:GR:d:PI:L:a:b:e:m:w:T:U:q:X:Y:Z:c")) != -1) {
    switch (c) {
    case 'f':
      scg_fd = fopen(optarg,"r");

      if (scg_fd==NULL) {
        printf("Error opening %s\n",optarg);
        exit(-1);
      }
      break;

    case 'g':
      switch ((char)*optarg) {
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

      break;

    case 'i':
      for(i=0; i < atoi(optarg); i++){
        chest_type[i] = atoi(argv[optind++]);
      }
      break;

    case 'n':
      n_trials = atoi(optarg);
      break;

    case 'c':
      ldpc_offload_flag = 1;
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

    case 'x':
      g_nrOfLayers = atoi(optarg);

      if ((g_nrOfLayers == 0) || (g_nrOfLayers > 4)) {
        printf("Unsupported nr Of Layers %d\n", g_nrOfLayers);
        exit(-1);
      }
      break;

    case 'p':
     g_pmi = atoi(optarg);
     break;

    case 'v':
      num_rounds = atoi(optarg);

      AssertFatal(num_rounds > 0 && num_rounds < 16, "Unsupported number of rounds %d, should be in [1,16]\n", num_rounds);
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
      max_ldpc_iterations = atoi(optarg);
      break;

    case 'L':
      loglvl = atoi(optarg);
      break;

    case 'a':
      g_rbStart = atoi(optarg);
      break;

    case 'b':
      g_rbSize = atoi(optarg);
      break;

    case 'd':
      dlsch_threads = atoi(optarg);
      break;

    case 'e':
      g_mcsIndex = atoi(optarg);
      break;

    case 'q':
      g_mcsTableIdx = atoi(optarg);
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
      strncpy(gNBthreads, optarg, sizeof(gNBthreads)-1);
      gNBthreads[sizeof(gNBthreads)-1]=0;
      break;

    case 'Y':
      run_initial_sync=1;
      //target_error_rate=0.1;
      slot = 0;
      break;
    case 'Z' :
      filename_csv = strdup(optarg);
      AssertFatal(filename_csv != NULL, "strdup() error: errno %d\n", errno);
      break;

    case 'o':
      delay = atoi(optarg);
      break;

    default:
    case 'h':
      printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n",
             argv[0]);
      printf("-h This message\n");
      printf("-L <log level, 0(errors), 1(warning), 2(analysis), 3(info), 4(debug), 5(trace)>\n");
      //printf("-p Use extended prefix mode\n");
      //printf("-d Use TDD\n");
      printf("-n Number of frames to simulate\n");
      printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
      printf("-S Ending SNR, runs from SNR0 to SNR1\n");
      //printf("-t Delay spread for multipath channel\n");
      printf("-g Channel model: [A] TDLA30, [B] TDLB100, [C] TDLC300, e.g. -g A\n");
      printf("-o Introduce delay in terms of number of samples\n");
      printf("-y Number of TX antennas used in gNB\n");
      printf("-z Number of RX antennas used in UE\n");
      printf("-x Num of layer for PDSCH\n");
      printf("-p Precoding matrix index\n");
      printf("-i Change channel estimation technique. Arguments list: Frequency domain {0:Linear interpolation, 1:PRB based averaging}, Time domain {0:Estimates of last DMRS symbol, 1:Average of DMRS symbols}\n");
      //printf("-j Relative strength of second intefering gNB (in dB) - cell_id mod 3 = 2\n");
      printf("-R N_RB_DL\n");
      printf("-O oversampling factor (1,2,4,8,16)\n");
      printf("-c ldpc offload flag\n");
      printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
      //printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
      printf("-f raw file containing RRC configuration (generated by gNB)\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      printf("-a Start PRB for PDSCH\n");
      printf("-b Number of PRB for PDSCH\n");
      printf("-d number of dlsch threads, 0: no dlsch parallelization\n");
      printf("-e MSC index\n");
      printf("-f raw file containing RRC configuration (generated by gNB)\n");
      printf("-g Channel model: [A] TDLA30, [B] TDLB100, [C] TDLC300, e.g. -g A\n");
      printf("-h This message\n");
      printf("-i Change channel estimation technique. Arguments list: Frequency domain {0:Linear interpolation, 1:PRB based averaging}, Time domain {0:Estimates of last DMRS symbol, 1:Average of DMRS symbols}\n");
      printf("-m Numerology\n");
      printf("-n Number of frames to simulate\n");
      printf("-o Introduce delay in terms of number of samples\n");
      printf("-p Precoding matrix index\n");
      printf("-q MCS Table index\n");
      printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
      printf("-S Ending SNR, runs from SNR0 to SNR1\n");
      printf("-t Acceptable effective throughput (in percentage)\n");
      printf("-v Maximum number of rounds\n");
      printf("-w Write txdata to binary file (one frame)\n");
      printf("-x Num of layer for PDSCH\n");
      printf("-y Number of TX antennas used in gNB\n");
      printf("-z Number of RX antennas used in UE\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      printf("-I Maximum LDPC decoder iterations\n");
      printf("-L <log level, 0(errors), 1(warning), 2(analysis), 3(info), 4(debug), 5(trace)>\n");
      printf("-P Print DLSCH performances\n");
      printf("-R N_RB_DL\n");
      printf("-T Enable PTRS, arguments list L_PTRS{0,1,2} K_PTRS{2,4}, e.g. -T 2 0 2 \n");
      printf("-U Change DMRS Config, arguments list DMRS TYPE{0=A,1=B} DMRS AddPos{0:2} DMRS ConfType{1:2}, e.g. -U 3 0 2 1 \n");
      printf("-X gNB thread pool configuration, n => no threads\n");
      printf("-Y Run initial sync in UE\n");
      printf("-Z Output filename (.csv format) for stats\n");
      exit (-1);
      break;
    }
  }

  logInit();
  set_glog(loglvl);
  /* initialize the sin table */
  InitSinLUT();

  get_softmodem_params()->phy_test = 1;
  get_softmodem_params()->usim_test = 1;
  get_softmodem_params()->do_ra = 0;
  set_softmodem_optmask(SOFTMODEM_DLSIM_BIT);

  if (snr1set==0)
    snr1 = snr0+10;

  RC.gNB = (PHY_VARS_gNB**) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0] = (PHY_VARS_gNB*) malloc(sizeof(PHY_VARS_gNB ));
  memset(RC.gNB[0],0,sizeof(PHY_VARS_gNB));

  gNB = RC.gNB[0];
  gNB->ofdm_offset_divisor = UINT_MAX;
  gNB->ldpc_offload_flag = ldpc_offload_flag;
  gNB->phase_comp = true; // we need to perform phase compensation, otherwise everything will fail
  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
  frame_parms->nb_antennas_tx = n_tx;
  frame_parms->nb_antennas_rx = n_rx;
  frame_parms->N_RB_DL = N_RB_DL;
  frame_parms->N_RB_UL = N_RB_DL;

  AssertFatal((gNB->if_inst = NR_IF_Module_init(0)) != NULL, "Cannot register interface");
  gNB->if_inst->NR_PHY_config_req = nr_phy_config_request;

  NR_ServingCellConfigCommon_t *scc = calloc(1,sizeof(*scc));;
  prepare_scc(scc);
  uint64_t ssb_bitmap = 1; // Enable only first SSB with index ssb_indx=0
  fill_scc_sim(scc, &ssb_bitmap, N_RB_DL, N_RB_DL, mu, mu);
  fix_scc(scc, ssb_bitmap);

  // TODO do a UECAP for phy-sim
  nr_pdsch_AntennaPorts_t pdsch_AntennaPorts = {0};
  pdsch_AntennaPorts.N1 = n_tx > 1 ? n_tx >> 1 : 1;
  pdsch_AntennaPorts.N2 = 1;
  pdsch_AntennaPorts.XP = n_tx > 1 ? 2 : 1;
  const nr_mac_config_t conf = {.pdsch_AntennaPorts = pdsch_AntennaPorts,
                                .pusch_AntennaPorts = n_tx,
                                .minRXTXTIME = 6,
                                .do_CSIRS = 0,
                                .do_SRS = 0,
                                .force_256qam_off = false};

  RC.nb_nr_macrlc_inst = 1;
  RC.nb_nr_mac_CC = (int*)malloc(RC.nb_nr_macrlc_inst*sizeof(int));
  for (i = 0; i < RC.nb_nr_macrlc_inst; i++)
    RC.nb_nr_mac_CC[i] = 1;
  mac_top_init_gNB(ngran_gNB, scc, NULL, &conf);
  gNB_mac = RC.nrmac[0];

  gNB_mac->dl_bler.harq_round_max = num_rounds;

  /*
  // read in SCGroupConfig
  AssertFatal(scg_fd != NULL,"no reconfig.raw file\n");
  char buffer[1024];
  int msg_len=fread(buffer,1,1024,scg_fd);
  NR_RRCReconfiguration_t *NR_RRCReconfiguration = NULL;

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
  NR_CellGroupConfig_t *secondaryCellGroup = NULL;
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

  NR_ServingCellConfig_t *scd = calloc(1,sizeof(*scd));
  prepare_scd(scd);

  gNB->ap_N1 = pdsch_AntennaPorts.N1;
  gNB->ap_N2 = pdsch_AntennaPorts.N2;
  gNB->ap_XP = pdsch_AntennaPorts.XP;

  validate_input_pmi(&gNB_mac->config[0], pdsch_AntennaPorts, g_nrOfLayers, g_pmi);

  NR_UE_NR_Capability_t* UE_Capability_nr = CALLOC(1,sizeof(NR_UE_NR_Capability_t));
  prepare_sim_uecap(UE_Capability_nr,scc,mu,
                    N_RB_DL,g_mcsTableIdx,0);

  NR_CellGroupConfig_t *secondaryCellGroup = get_default_secondaryCellGroup(scc, scd, UE_Capability_nr, 0, 1, &conf, 0);

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

  // UE dedicated configuration
  nr_mac_add_test_ue(RC.nrmac[0], secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity, secondaryCellGroup);
  // reset preprocessor to the one of DLSIM after it has been set during
  // nr_mac_config_scc()
  gNB_mac->pre_processor_dl = nr_dlsim_preprocessor;
  phy_init_nr_gNB(gNB);
  N_RB_DL = gNB->frame_parms.N_RB_DL;
  NR_UE_info_t *UE_info = RC.nrmac[0]->UE_info.list[0];

  configure_UE_BWP(RC.nrmac[0], scc, &UE_info->UE_sched_ctrl, NULL, UE_info, -1, -1);

  // stub to configure frame_parms
  //  nr_phy_config_request_sim(gNB,N_RB_DL,N_RB_DL,mu,Nid_cell,SSB_positions);
  // call MAC to configure common parameters

  /* nr_mac_add_test_ue() has created one user, so set the scheduling
   * parameters from command line in global variables that will be picked up by
   * scheduling preprocessor */
  if (g_mcsIndex < 0) g_mcsIndex = 9;
  if (g_rbStart < 0) g_rbStart=0;
  if (g_rbSize < 0) g_rbSize = N_RB_DL - g_rbStart;

  double fs,txbw,rxbw;
  uint32_t samples;

  get_samplerate_and_bw(mu,
                        N_RB_DL,
                        frame_parms->threequarter_fs,
                        &fs,
                        &samples,
                        &txbw,
                        &rxbw);

  gNB2UE = new_channel_desc_scm(n_tx,
                                n_rx,
                                channel_model,
                                fs/1e6,//sampling frequency in MHz
                                0,
                                txbw,
                                DS_TDL,
                                0.0,
                                CORR_LEVEL_LOW,
                                0,
                                delay,
                                0,
                                0);

  if (gNB2UE==NULL) {
    printf("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  frame_length_complex_samples = frame_parms->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
  //frame_length_complex_samples_no_prefix = frame_parms->samples_per_subframe_wCP*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
  int slot_offset = frame_parms->get_samples_slot_timestamp(slot,frame_parms,0);
  int slot_length = slot_offset - frame_parms->get_samples_slot_timestamp(slot-1,frame_parms,0);

  s_re = malloc(n_tx*sizeof(double*));
  s_im = malloc(n_tx*sizeof(double*));
  r_re = malloc(n_rx*sizeof(double*));
  r_im = malloc(n_rx*sizeof(double*));
  txdata = malloc(n_tx*sizeof(int*));

  for (i = 0; i < n_tx; i++) {
    s_re[i] = calloc(1, slot_length * sizeof(double));
    s_im[i] = calloc(1, slot_length * sizeof(double));

    printf("Allocating %d samples for txdata\n", frame_length_complex_samples);
    txdata[i] = calloc(1, frame_length_complex_samples * sizeof(int));
  }

  for (i = 0; i < n_rx; i++) {
    r_re[i] = calloc(1, slot_length * sizeof(double));
    r_im[i] = calloc(1, slot_length * sizeof(double));
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
  UE->frame_parms.nb_antenna_ports_gNB = n_tx;
  UE->max_ldpc_iterations = max_ldpc_iterations;

  if (run_initial_sync==1)
    UE->is_synchronized = 0;
  else
    UE->is_synchronized = 1;

  if (init_nr_ue_signal(UE, 1) != 0)
  {
    printf("Error at UE NR initialisation\n");
    exit(-1);
  }

  init_nr_ue_transport(UE);

  nr_gold_pbch(UE);

  // compute the scramblingID_pdcch and the gold pdcch
  UE->scramblingID_pdcch = frame_parms->Nid_cell;
  nr_gold_pdcch(UE, frame_parms->Nid_cell);

  // compute the scrambling IDs for PDSCH DMRS
  for (int i = 0; i < 2; i++) {
    UE->scramblingID_dlsch[i] = frame_parms->Nid_cell;
    nr_gold_pdsch(UE, i, UE->scramblingID_dlsch[i]);
  }

  nr_l2_init_ue(1);
  UE_mac = get_mac_inst(0);
  ue_init_config_request(UE_mac, mu);

  UE->if_inst = nr_ue_if_module_init(0);
  UE->if_inst->scheduled_response = nr_ue_scheduled_response;
  UE->if_inst->phy_config_request = nr_ue_phy_config_request;
  UE->if_inst->dl_indication = nr_ue_dl_indication;
  UE->if_inst->ul_indication = dummy_nr_ue_ul_indication;
  UE->chest_freq = chest_type[0];
  UE->chest_time = chest_type[1];

  UE_mac->if_module = nr_ue_if_module_init(0);

  unsigned int available_bits=0;
  unsigned char *estimated_output_bit;
  unsigned char *test_input_bit;
  unsigned int errors_bit = 0;

  initFloatingCoresTpool(dlsch_threads, &nrUE_params.Tpool, false, "UE-tpool");

  test_input_bit = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
  estimated_output_bit = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
  
  // generate signal
  AssertFatal(input_fd==NULL,"Not ready for input signal file\n");

  // clone CellGroup to have a separate copy at UE
  NR_CellGroupConfig_t *UE_CellGroup = clone_CellGroupConfig(secondaryCellGroup);

  //Configure UE
  NR_BCCH_BCH_Message_t *mib = get_new_MIB_NR(scc);
  nr_rrc_mac_config_req_mib(0, 0, mib->message.choice.mib, false);
  nr_rrc_mac_config_req_cg(0, 0, UE_CellGroup);

  asn1cFreeStruc(asn_DEF_NR_CellGroupConfig, UE_CellGroup);

  UE_mac->state = UE_CONNECTED;
  UE_mac->ra.ra_state = RA_SUCCEEDED;

  nr_phy_data_t phy_data = {0};
  fapi_nr_dl_config_request_t dl_config = {.sfn = frame, .slot = slot};
  nr_scheduled_response_t scheduled_response = {.dl_config = &dl_config, .phy_data = &phy_data, .mac = UE_mac};

  nr_ue_phy_config_request(&UE_mac->phy_config);
  //NR_COMMON_channels_t *cc = RC.nrmac[0]->common_channels;
  int n_errs = 0;

  initNamedTpool(gNBthreads, &gNB->threadPool, true, "gNB-tpool");
  initNotifiedFIFO(&gNB->L1_tx_free);
  initNotifiedFIFO(&gNB->L1_tx_filled);
  initNotifiedFIFO(&gNB->L1_tx_out);
  // we create 2 threads for L1 tx processing
  notifiedFIFO_elt_t *msgL1Tx = newNotifiedFIFO_elt(sizeof(processingData_L1tx_t),0,&gNB->L1_tx_free,processSlotTX);
  processingData_L1tx_t *msgDataTx = (processingData_L1tx_t *)NotifiedFifoData(msgL1Tx);
  init_DLSCH_struct(gNB, msgDataTx);
  msgDataTx->slot = slot;
  msgDataTx->frame = frame;
  memset(msgDataTx->ssb, 0, 64*sizeof(NR_gNB_SSB_t));
  gNB->msgDataTx = msgDataTx;

  // Buffers to store internal memory of slot process
  int rx_size = (((14 * frame_parms->N_RB_DL * 12 * sizeof(int32_t)) + 15) >> 4) << 4;
  UE->phy_sim_rxdataF = calloc(sizeof(int32_t *) * frame_parms->nb_antennas_rx * g_nrOfLayers, frame_parms->samples_per_slot_wCP * sizeof(int32_t));
  UE->phy_sim_pdsch_llr = calloc(1, (8 * (3 * 8 * 8448)) * sizeof(int16_t)); // Max length
  UE->phy_sim_pdsch_rxdataF_ext = calloc(sizeof(int32_t *) * frame_parms->nb_antennas_rx * g_nrOfLayers, rx_size);
  UE->phy_sim_pdsch_rxdataF_comp = calloc(sizeof(int32_t *) * frame_parms->nb_antennas_rx * g_nrOfLayers, rx_size);
  UE->phy_sim_pdsch_dl_ch_estimates = calloc(sizeof(int32_t *) * frame_parms->nb_antennas_rx * g_nrOfLayers, rx_size);
  UE->phy_sim_pdsch_dl_ch_estimates_ext = calloc(sizeof(int32_t *) * frame_parms->nb_antennas_rx * g_nrOfLayers, rx_size);
  int a_segments = MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER*NR_MAX_NB_LAYERS;  //number of segments to be allocated
  if (g_rbSize != 273) {
    a_segments = a_segments*g_rbSize;
    a_segments = (a_segments/273)+1;
  }
  uint32_t dlsch_bytes = a_segments*1056;  // allocated bytes per segment
  UE->phy_sim_dlsch_b = calloc(1, dlsch_bytes);

  // csv file
  if (filename_csv != NULL) {
    csv_file = fopen(filename_csv, "a");
    if (csv_file == NULL) {
      printf("Can't open file \"%s\", errno %d\n", filename_csv, errno);
      return 1;
    }
    // adding name of parameters into file
    fprintf(csv_file,"SNR,false_positive,");
    for (int r = 0; r < num_rounds; r++)
      fprintf(csv_file,"n_errors_%d,errors_scrambling_%d,channel_bler_%d,channel_ber_%d,",r,r,r,r);
    fprintf(csv_file,"avg_round,eff_rate,eff_throughput,TBS\n");
  }
  //---------------
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

    uint32_t errors_scrambling[16] = {0};
    int n_errors[16] = {0};
    int round_trials[16] = {0};
    double roundStats = {0};
    double blerStats[16] = {0};
    double berStats[16] = {0};

    effRate = 0;
    //n_errors2 = 0;
    //n_alamouti = 0;
    n_false_positive = 0;
    if (n_trials== 1) num_rounds = 1;

    for (trial = 0; trial < n_trials; trial++) {

      errors_bit = 0;
      //multipath channel
      //multipath_channel(gNB2UE,s_re,s_im,r_re,r_im,frame_length_complex_samples,0);

      UE->rx_offset=0;
      UE_proc.frame_rx = frame;
      UE_proc.nr_slot_rx = slot;
      UE_proc.gNB_id = 0;

      NR_UE_DLSCH_t *dlsch0 = &phy_data.dlsch[0];

      int harq_pid = slot;
      NR_DL_UE_HARQ_t *UE_harq_process = &UE->dl_harq_processes[0][harq_pid];

      NR_gNB_DLSCH_t *gNB_dlsch = &msgDataTx->dlsch[0][0];
      nfapi_nr_dl_tti_pdsch_pdu_rel15_t *rel15 = &gNB_dlsch->harq_process.pdsch_pdu.pdsch_pdu_rel15;
      
      UE_harq_process->ack = 0;
      round = 0;
      UE_harq_process->DLround = round;
      UE_harq_process->first_rx = 1;

      Sched_INFO = malloc(sizeof(*Sched_INFO));
      if (Sched_INFO == NULL) {
        LOG_E(PHY, "out of memory\n");
        exit(1);
      }
      memset(Sched_INFO, 0, sizeof(*Sched_INFO));
      Sched_INFO->sched_response_id = -1;

      while ((round<num_rounds) && (UE_harq_process->ack==0)) {
        round_trials[round]++;

        clear_nr_nfapi_information(RC.nrmac[0], 0, frame, slot, &Sched_INFO->DL_req, &Sched_INFO->TX_req, &Sched_INFO->UL_dci_req);
        UE_info->UE_sched_ctrl.harq_processes[harq_pid].ndi = !(trial&1);
        UE_info->UE_sched_ctrl.harq_processes[harq_pid].round = round;

        // nr_schedule_ue_spec() requires the mutex to be locked
        NR_SCHED_LOCK(&gNB_mac->sched_lock);
        nr_schedule_ue_spec(0, frame, slot, &Sched_INFO->DL_req, &Sched_INFO->TX_req);
        NR_SCHED_UNLOCK(&gNB_mac->sched_lock);
        Sched_INFO->module_id = 0;
        Sched_INFO->CC_id = 0;
        Sched_INFO->frame = frame;
        Sched_INFO->slot = slot;
        Sched_INFO->UL_dci_req.numPdus = 0;
        pushNotifiedFIFO(&gNB->L1_tx_free,msgL1Tx);
        nr_schedule_response(Sched_INFO);

        /* PTRS values for DLSIM calculations   */
        nfapi_nr_dl_tti_request_body_t *dl_req = &Sched_INFO->DL_req.dl_tti_request_body;
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
          ptrsRePerSymb = ((rel15->rbSize + rel15->PTRSFreqDensity - 1) / rel15->PTRSFreqDensity);
          LOG_D(PHY,"[DLSIM] PTRS Symbols in a slot: %2u, RE per Symbol: %3u, RE in a slot %4d\n", ptrsSymbPerSlot, ptrsRePerSymb, ptrsSymbPerSlot * ptrsRePerSymb);
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
        if (n_trials == 1) printf("slot_offset %d, txdataF_offset %d \n", slot_offset, txdataF_offset);

        //TODO: loop over slots
        for (aa=0; aa<gNB->frame_parms.nb_antennas_tx; aa++) {

          if (cyclic_prefix_type == 1) {
            PHY_ofdm_mod((int *)&gNB->common_vars.txdataF[aa][txdataF_offset],
                         (int *)&txdata[aa][slot_offset],
                         frame_parms->ofdm_symbol_size,
                         12,
                         frame_parms->nb_prefix_samples,
                         CYCLIC_PREFIX);
          } else {
            nr_normal_prefix_mod(&gNB->common_vars.txdataF[aa][txdataF_offset],
                                 &txdata[aa][slot_offset],
                                 14,
                                 frame_parms,
                                 slot);
          }
        }
        if (n_trials==1) {
          char filename[100];//LOG_M
          for (aa=0;aa<n_tx;aa++) {
            sprintf(filename,"txsig%d.m", aa);//LOG_M
            LOG_M(filename,"txs", &txdata[aa][slot_offset +frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples0],6*(frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples),1,1);
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
          txlev[aa] = signal_energy((int32_t *)&txdata[aa][slot_offset +l_ofdm*frame_parms->ofdm_symbol_size + (l_ofdm-1)*frame_parms->nb_prefix_samples + frame_parms->nb_prefix_samples0],
          frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples);
          txlev_sum += txlev[aa];
          if (n_trials==1) printf("txlev[%d] = %d (%f dB) txlev_sum %d\n",aa,txlev[aa],10*log10((double)txlev[aa]),txlev_sum);
        }

        for (i = 0; i < slot_length; i++) {
          for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
            s_re[aa][i] = ((double)(((short *)&txdata[aa][slot_offset]))[(i << 1)]);
            s_im[aa][i] = ((double)(((short *)&txdata[aa][slot_offset]))[(i << 1) + 1]);
          }
        }

        double ts = 1.0/(frame_parms->subcarrier_spacing * frame_parms->ofdm_symbol_size); 
        //Compute AWGN variance
        sigma2_dB = 10 * log10((double)txlev_sum * ((double)UE->frame_parms.ofdm_symbol_size/(12*rel15->rbSize))) - SNR;
        sigma2    = pow(10, sigma2_dB/10);
        if (n_trials==1) printf("sigma2 %f (%f dB), txlev_sum %f (factor %f)\n",sigma2,sigma2_dB,10*log10((double)txlev_sum),(double)(double)UE->frame_parms.ofdm_symbol_size/(12*rel15->rbSize));

        for (aa = 0; aa < n_rx; aa++) {
          bzero(r_re[aa], slot_length * sizeof(double));
          bzero(r_im[aa], slot_length * sizeof(double));
        }

        // Apply MIMO Channel
        multipath_channel(gNB2UE, s_re, s_im, r_re, r_im, slot_length, 0, (n_trials == 1) ? 1 : 0);
        add_noise(UE->common_vars.rxdata, (const double **) r_re, (const double **) r_im, sigma2, slot_length, slot_offset, ts, delay, pdu_bit_map, 0x1, frame_parms->nb_antennas_rx);
        dl_config.sfn = frame;
        dl_config.slot = slot;
        ue_dci_configuration(UE_mac, &dl_config, frame, slot);
        nr_ue_scheduled_response(&scheduled_response);

        pbch_pdcch_processing(UE,
                              &UE_proc,
                              &phy_data);
        pdsch_processing(UE,
                         &UE_proc,
                         &phy_data);
        //----------------------------------------------------------
        //---------------------- count errors ----------------------
        //----------------------------------------------------------

        if (dlsch0->last_iteration_cnt >= dlsch0->max_ldpc_iterations+1)
          n_errors[round]++;

        int16_t *UE_llr = (int16_t*)UE->phy_sim_pdsch_llr;

        TBS                  = dlsch0->dlsch_config.TBS;//rel15->TBSize[0];
        uint16_t length_dmrs = get_num_dmrs(rel15->dlDmrsSymbPos);
        uint16_t nb_rb       = rel15->rbSize;
        uint8_t  nb_re_dmrs  = rel15->dmrsConfigType == NFAPI_NR_DMRS_TYPE1 ? 6*dlsch0->dlsch_config.n_dmrs_cdm_groups : 4*dlsch0->dlsch_config.n_dmrs_cdm_groups;
        uint8_t  mod_order   = rel15->qamModOrder[0];
        uint8_t  nb_symb_sch = rel15->NrOfSymbols;
        uint32_t unav_res = ptrsSymbPerSlot * ptrsRePerSymb;
        available_bits = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, unav_res, mod_order, rel15->nrOfLayers);
        if (pdu_bit_map & 0x1) {
          if (trial == 0 && round == 0) {
            printf("[DLSIM][PTRS] Available bits are: %5u, removed PTRS bits are: %5u \n", available_bits, (ptrsSymbPerSlot * ptrsRePerSymb * rel15->nrOfLayers * mod_order));
          }
        }

        for (i = 0; i < available_bits; i++) {
          if(((gNB_dlsch->harq_process.f[i] == 0) && (UE_llr[i] <= 0)) ||
             ((gNB_dlsch->harq_process.f[i] == 1) && (UE_llr[i] >= 0)))
          {
            if (errors_scrambling[round] == 0) {
              LOG_D(PHY,"First bit in error in unscrambling = %d\n",i);
            }
            errors_scrambling[round]++;
          }
        }

        //printf("dlsim round %d ends\n",round);
        round++;
      } // round

      for (i = 0; i < TBS; i++) {

	estimated_output_bit[i] = (UE->phy_sim_dlsch_b[i/8] & (1 << (i & 7))) >> (i & 7);
	test_input_bit[i]       = (gNB_dlsch->harq_process.b[i / 8] & (1 << (i & 7))) >> (i & 7); // Further correct for multiple segments
	
	if (estimated_output_bit[i] != test_input_bit[i]) {
	  if(errors_bit == 0)
	    LOG_D(PHY,"First bit in error in decoding = %d\n",i);
	  errors_bit++;
	}
	
      }
      ////////////////////////////////////////////////////////////

      if (errors_bit > 0) {
	n_false_positive++;
	if (n_trials == 1)
	  printf("errors_bit = %u (trial %d)\n", errors_bit, trial);
      }
      roundStats += ((float)round);
      if (UE_harq_process->ack==1) effRate += ((float)TBS)/round;
    } // noise trials

    roundStats /= ((float)n_trials);

    for (int r = 0; r < num_rounds; r++) {
      blerStats[r] = (double)n_errors[r] / round_trials[r];
      berStats[r] = (double)errors_scrambling[r] / available_bits / round_trials[r];
    }

    effRate /= n_trials;
    printf("*****************************************\n");
    printf("SNR %f: n_errors (%d/%d", SNR, n_errors[0], round_trials[0]);
    for (int r = 1; r < num_rounds; r++)
      printf(",%d/%d", n_errors[r], round_trials[r]);
    printf(") (negative CRC), false_positive %d/%d, errors_scrambling (%u/%u", n_false_positive, n_trials, errors_scrambling[0], available_bits * round_trials[0]);
    for (int r = 1; r < num_rounds; r++)
      printf(",%u/%u", errors_scrambling[r], available_bits * round_trials[r]);
    printf(")\n\n");
    dump_pdsch_stats(stdout,gNB);
    printf("SNR %f: Channel BLER (%e", SNR, blerStats[0]);
    for (int r = 1; r < num_rounds; r++)
      printf(",%e", blerStats[r]);
    printf("), Channel BER (%e", berStats[0]);
    for (int r = 1; r < num_rounds; r++)
      printf(",%e", berStats[r]);
    printf(") Avg round %.2f, Eff Rate %.4f bits/slot, Eff Throughput %.2f, TBS %u bits/slot\n", roundStats, effRate, effRate / TBS * 100, TBS);
    printf("*****************************************\n");
    printf("\n");
    // writing to csv file
    if (filename_csv != NULL) { // means we are asked to print stats to CSV
      fprintf(csv_file,"%f,%d/%d,",SNR,n_false_positive,n_trials);
      for (int r = 0; r < num_rounds; r++)
        fprintf(csv_file,"%d/%d,%u/%u,%f,%e,",n_errors[r], round_trials[r], errors_scrambling[r], available_bits * round_trials[r],blerStats[r],berStats[r]);
      fprintf(csv_file,"%.2f,%.4f,%.2f,%u\n", roundStats, effRate, effRate / TBS * 100, TBS);
    }
    if (print_perf==1) {
      printf("\ngNB TX function statistics (per %d us slot, NPRB %d, mcs %d, block %d)\n",
             1000 >> *scc->ssbSubcarrierSpacing,
             g_rbSize,
             g_mcsIndex,
             msgDataTx->dlsch[0][0].harq_process.pdsch_pdu.pdsch_pdu_rel15.TBSize[0] << 3);
      printDistribution(&gNB->phy_proc_tx,table_tx,"PHY proc tx");
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
	     UE->dlsch[0][0]->harq_processes[0]->Cminus ?
	     UE->dlsch[0][0]->harq_processes[0]->Kminus :
	     UE->dlsch[0][0]->harq_processes[0]->Kplus,
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
      LOG_M("rxF0.m","rxF0", UE->phy_sim_rxdataF, frame_parms->samples_per_slot_wCP, 1, 1);
      LOG_M("rxF_ext.m","rxFe",UE->phy_sim_pdsch_rxdataF_ext,g_rbSize*12*14,1,1);
      LOG_M("chestF0.m","chF0",UE->phy_sim_pdsch_dl_ch_estimates_ext,g_rbSize*12*14,1,1);
      write_output("rxF_comp.m","rxFc",UE->phy_sim_pdsch_rxdataF_comp,N_RB_DL*12*14,1,1);
      LOG_M("rxF_llr.m","rxFllr",UE->phy_sim_pdsch_llr,available_bits,1,0);
      break;
    }

    if (effRate > (eff_tp_check*TBS)) {
      printf("PDSCH test OK\n");
      break;
    }

    n_errs = n_errors[0];
  } // NSR

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
  free(UE->phy_sim_rxdataF);
  free(UE->phy_sim_pdsch_llr);
  free(UE->phy_sim_pdsch_rxdataF_ext);
  free(UE->phy_sim_pdsch_rxdataF_comp);
  free(UE->phy_sim_pdsch_dl_ch_estimates);
  free(UE->phy_sim_pdsch_dl_ch_estimates_ext);
  free(UE->phy_sim_dlsch_b);

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

  return n_errs;
}


void update_ptrs_config(NR_CellGroupConfig_t *secondaryCellGroup, uint16_t *rbSize, uint8_t *mcsIndex, int8_t *ptrs_arg)
{
  NR_BWP_Downlink_t *bwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[0];
  long ptrsFreqDenst[] = {25, 115};
  long ptrsTimeDenst[] = {2, 4, 10};

  long epre_Ratio = 0;
  long reOffset = 0;

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
  if (dmrs_arg[1] >= 0 && dmrs_arg[1] < 4) {
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
    switch (add_pos) {
      case 0:
        *dmrs_config->dmrs_AdditionalPosition = NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0;
        break;
      case 1:
        *dmrs_config->dmrs_AdditionalPosition = NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1;
        break;
      case 3:
        *dmrs_config->dmrs_AdditionalPosition = NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos3;
        break;
      default:
        AssertFatal(false, "DMRS additional position %d not valid\n", add_pos);
    }
  } else { // if NULL, Value pos2
    free(dmrs_config->dmrs_AdditionalPosition);
    dmrs_config->dmrs_AdditionalPosition = NULL;
  }

  for (int i=0;i<bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count;i++) {
    bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->mappingType = mapping_type;
  }

  printf("[DLSIM] DMRS Config is modified with Mapping Type %d, Additional Positions %d Config. Type %d \n", mapping_type, add_pos, dmrs_config_type);
}
