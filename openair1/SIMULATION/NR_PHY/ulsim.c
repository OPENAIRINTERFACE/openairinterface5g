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

#include "common/config/config_userapi.h"
#include "common/utils/LOG/log.h"
#include "common/ran_context.h"

#include "SIMULATION/TOOLS/sim.h"
#include "SIMULATION/RF/rf.h"

#include "PHY/types.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/defs_gNB.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_REFSIG/nr_mod_table.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/NR_TRANSPORT/nr_transport.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "SCHED_NR/sched_nr.h"
//#include "PHY/MODULATION/modulation_common.h"
//#include "common/config/config_load_configmodule.h"
//#include "UTIL/LISTS/list.h"
//#include "common/ran_context.h"

//#define DEBUG_ULSCHSIM
PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;

double cpuf;

// dummy functions
int nfapi_mode = 0;
int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req) {
  return (0);
}
int oai_nfapi_tx_req(nfapi_tx_request_t *tx_req) {
  return (0);
}

int oai_nfapi_dl_config_req(nfapi_dl_config_request_t *dl_config_req) {
  return (0);
}

int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req) {
  return (0);
}

int oai_nfapi_nr_dl_config_req(nfapi_nr_dl_config_request_t *dl_config_req) {
  return (0);
}

uint32_t from_nrarfcn(int nr_bandP, uint32_t dl_earfcn) {
  return (0);
}
int32_t get_uldl_offset(int eutra_bandP) {
  return (0);
}

NR_IF_Module_t *
NR_IF_Module_init(int Mod_id) {
  return (NULL);
}

void exit_function(const char *file, const char *function, const int line, const char *s) {
  const char *msg = s == NULL ? "no comment" : s;
  printf("Exiting at: %s:%d %s(), %s\n", file, line, function, msg);
  exit(-1);
}

// needed for some functions
PHY_VARS_NR_UE *PHY_vars_UE_g[1][1] = { { NULL } };
uint16_t n_rnti = 0x1234;
openair0_config_t openair0_cfg[MAX_CARDS];

char quantize(double D, double x, unsigned char B) {
  double qxd;
  short maxlev;
  qxd = floor(x / D);
  maxlev = 1 << (B - 1); //(char)(pow(2,B-1));

  //printf("x=%f,qxd=%f,maxlev=%d\n",x,qxd, maxlev);

  if (qxd <= -maxlev)
    qxd = -maxlev;
  else if (qxd >= maxlev)
    qxd = maxlev - 1;

  return ((char) qxd);
}

int main(int argc, char **argv) {

  char c;
  int i,sf;
  double SNR, SNR_lin, snr0 = -2.0, snr1 = 2.0;
  double sigma2, sigma2_dB;
  double snr_step = 0.1;
  uint8_t snr1set = 0;
  int slot = 0;
  int **txdata;
  int32_t **txdataF;
  int16_t **r_re, **r_im;
  FILE *output_fd = NULL;
  //uint8_t write_output_file = 0;
  int trial, n_trials = 1, n_errors = 0, n_false_positive = 0;
  uint8_t n_tx = 1, n_rx = 1, nb_codewords = 1;
  //uint8_t transmission_mode = 1;
  uint16_t Nid_cell = 0;
  channel_desc_t *gNB2UE;
  uint8_t extended_prefix_flag = 0;
  //int8_t interf1 = -21, interf2 = -21;
  FILE *input_fd = NULL;
  SCM_t channel_model = AWGN;  //Rayleigh1_anticorr;
  uint16_t N_RB_DL = 106, N_RB_UL = 106, mu = 1;
  //unsigned char frame_type = 0;
  int frame = 0, subframe = 0;
  int frame_length_complex_samples;
  NR_DL_FRAME_PARMS *frame_parms;
  double sigma;
  unsigned char qbits = 8;
  int ret;
  int loglvl = OAILOG_WARNING;
  float target_error_rate = 0.01;
  uint64_t SSB_positions=0x01;
  uint16_t nb_symb_sch = 12;
  int start_symbol = 14 - nb_symb_sch;
  uint16_t nb_rb = 50;
  uint8_t Imcs = 9;
  int eNB_id = 0;
  int ap;
  int tx_offset;
  int sample_offsetF;
  double txlev;

  cpuf = get_cpu_freq_GHz();


  if (load_configmodule(argc, argv) == 0) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }

  //logInit();
  randominit(0);

  while ((c = getopt(argc, argv, "df:hpg:i:j:n:l:m:r:s:S:y:z:M:N:F:R:P:")) != -1) {
    switch (c) {
      /*case 'f':
         write_output_file = 1;
         output_fd = fopen(optarg, "w");

         if (output_fd == NULL) {
             printf("Error opening %s\n", optarg);
             exit(-1);
         }

         break;*/

      /*case 'd':
        frame_type = 1;
        break;*/

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
            msg("Unsupported channel model!\n");
            exit(-1);
        }

        break;

      /*case 'i':
        interf1 = atoi(optarg);
        break;

      case 'j':
        interf2 = atoi(optarg);
        break;*/

      case 'n':
        n_trials = atoi(optarg);
        break;

      case 's':
        snr0 = atof(optarg);
        msg("Setting SNR0 to %f\n", snr0);
        break;

      case 'S':
        snr1 = atof(optarg);
        snr1set = 1;
        msg("Setting SNR1 to %f\n", snr1);
        break;

      case 'p':
        extended_prefix_flag = 1;
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

      case 'y':
        n_tx = atoi(optarg);

        if ((n_tx == 0) || (n_tx > 2)) {
          msg("Unsupported number of tx antennas %d\n", n_tx);
          exit(-1);
        }

        break;

      case 'z':
        n_rx = atoi(optarg);

        if ((n_rx == 0) || (n_rx > 2)) {
          msg("Unsupported number of rx antennas %d\n", n_rx);
          exit(-1);
        }

        break;

      case 'M':
        SSB_positions = atoi(optarg);
        break;

      case 'N':
        Nid_cell = atoi(optarg);
        break;

      case 'R':
        N_RB_DL = atoi(optarg);
        N_RB_UL = N_RB_DL;
        break;

      case 'F':
        input_fd = fopen(optarg, "r");

        if (input_fd == NULL) {
            printf("Problem with filename %s\n", optarg);
            exit(-1);
        }

        break;

      case 'm':
        Imcs = atoi(optarg);
        break;

      case 'l':
        nb_symb_sch = atoi(optarg);
        break;

      case 'r':
        nb_rb = atoi(optarg);
        break;

      /*case 'x':
        transmission_mode = atoi(optarg);
        break;*/

      default:
        case 'h':
          printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n", argv[0]);
          printf("-h This message\n");
          printf("-p Use extended prefix mode\n");
          //printf("-d Use TDD\n");
          printf("-n Number of frames to simulate\n");
          printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
          printf("-S Ending SNR, runs from SNR0 to SNR1\n");
          printf("-t Delay spread for multipath channel\n");
          printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
          //printf("-x Transmission mode (1,2,6 for the moment)\n");
          printf("-y Number of TX antennas used in eNB\n");
          printf("-z Number of RX antennas used in UE\n");
          //printf("-i Relative strength of first intefering eNB (in dB) - cell_id mod 3 = 1\n");
          //printf("-j Relative strength of second intefering eNB (in dB) - cell_id mod 3 = 2\n");
          printf("-M Multiple SSB positions in burst\n");
          printf("-N Nid_cell\n");
          printf("-R N_RB_DL\n");
          printf("-O oversampling factor (1,2,4,8,16)\n");
          printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
          //printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
          //printf("-f Output filename (.txt format) for Pe/SNR results\n");
          printf("-F Input filename (.txt format) for RX conformance testing\n");
          exit(-1);
          break;
    }
  }

  logInit();
  set_glog(loglvl);
  T_stdout = 1;

  if (snr1set == 0)
    snr1 = snr0 + 10;

  gNB2UE = new_channel_desc_scm(n_tx, n_rx, channel_model,
                                61.44e6, //N_RB2sampling_rate(N_RB_DL),
                                40e6, //N_RB2channel_bandwidth(N_RB_DL),
                                0, 0, 0);

  if (gNB2UE == NULL) {
    msg("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  RC.gNB = (PHY_VARS_gNB ** *) malloc(sizeof(PHY_VARS_gNB **));
  RC.gNB[0] = (PHY_VARS_gNB **) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0][0] = malloc(sizeof(PHY_VARS_gNB));
  gNB = RC.gNB[0][0];
  //gNB_config = &gNB->gNB_config;

  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
  frame_parms->nb_antennas_tx = n_tx;
  frame_parms->nb_antennas_rx = n_rx;
  frame_parms->N_RB_DL = N_RB_DL;
  frame_parms->N_RB_UL = N_RB_UL;
  frame_parms->Ncp = extended_prefix_flag ? EXTENDED : NORMAL;

  crcTableInit();

  nr_phy_config_request_sim(gNB, N_RB_DL, N_RB_DL, mu, Nid_cell, SSB_positions);

  phy_init_nr_gNB(gNB, 0, 0);
  //init_eNB_afterRU();

  frame_length_complex_samples = frame_parms->samples_per_subframe;
  //frame_length_complex_samples_no_prefix = frame_parms->samples_per_subframe_wCP;
  r_re   = malloc(2 * sizeof(int16_t *));
  r_im   = malloc(2 * sizeof(int16_t *));

  for (i = 0; i < 2; i++) {
    r_re[i] = malloc(frame_length_complex_samples * sizeof(int16_t));
    bzero(r_re[i], frame_length_complex_samples * sizeof(int16_t));
    r_im[i] = malloc(frame_length_complex_samples * sizeof(int16_t));
    bzero(r_im[i], frame_length_complex_samples * sizeof(int16_t));
  }

  //configure UE
  UE = malloc(sizeof(PHY_VARS_NR_UE));
  memcpy(&UE->frame_parms, frame_parms, sizeof(NR_DL_FRAME_PARMS));

  //phy_init_nr_top(frame_parms);
  if (init_nr_ue_signal(UE, 1, 0) != 0) {
    printf("Error at UE NR initialisation\n");
    exit(-1);
  }

  //nr_init_frame_parms_ue(&UE->frame_parms);
  //init_nr_ue_transport(UE, 0);
  for (sf = 0; sf < 2; sf++) {
    for (i = 0; i < 2; i++) {

        UE->ulsch[sf][0][i] = new_nr_ue_ulsch(N_RB_UL, 8, 0);

        if (!UE->ulsch[sf][0][i]) {
          printf("Can't get ue ulsch structures\n");
          exit(-1);
        }

    }
  }

  unsigned char harq_pid = 0;
  uint8_t is_crnti = 0, llr8_flag = 0;
  unsigned int TBS = 8424;
  unsigned int available_bits;
  uint8_t  nb_re_dmrs  = UE->dmrs_UplinkConfig.pusch_maxLength*(UE->dmrs_UplinkConfig.pusch_dmrs_type == pusch_dmrs_type1)?6:4;
  uint16_t length_dmrs = 1;
  uint8_t  N_PRB_oh;
  uint16_t N_RE_prime;
  unsigned char mod_order;
  uint8_t Nl    = 1;
  uint8_t rvidx = 0;
  uint8_t UE_id = 1;
  uint8_t cwd;
  uint16_t start_sc, start_rb;
  int8_t Wf[2], Wt[2], l0, l_prime[2], delta;
  uint32_t ***pusch_dmrs;


  NR_gNB_ULSCH_t *ulsch_gNB = gNB->ulsch[UE_id][0];
  nfapi_nr_ul_config_ulsch_pdu_rel15_t *rel15_ul = &ulsch_gNB->harq_processes[harq_pid]->ulsch_pdu.ulsch_pdu_rel15;

  NR_UE_PUSCH *pusch_ue = UE->pusch_vars[0][eNB_id];
  NR_UE_ULSCH_t **ulsch_ue = UE->ulsch[0][0];
  NR_UL_UE_HARQ_t *harq_process_ul_ue;

  mod_order      = nr_get_Qm(Imcs, 1);
  available_bits = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, 1);
  TBS            = nr_compute_tbs(Imcs, nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, Nl);

  printf("available bits %d TBS %d mod_order %d\n", available_bits, TBS, mod_order);

  /////////// setting rel15_ul parameters ///////////
  rel15_ul->number_rbs     = nb_rb;
  rel15_ul->number_symbols = nb_symb_sch;
  rel15_ul->Qm             = mod_order;
  rel15_ul->mcs            = Imcs;
  rel15_ul->rv             = rvidx;
  rel15_ul->n_layers       = Nl;
  ///////////////////////////////////////////////////


  double *modulated_input        = malloc16(sizeof(double) * 16 * 68 * 384); // [hna] 16 segments, 68*Zc
  short  *channel_output_fixed   = malloc16(sizeof(short) * 16 * 68 * 384);
  short  *channel_output_uncoded = malloc16(sizeof(unsigned short) * 16 * 68 * 384);

  uint32_t scrambled_output[NR_MAX_NB_CODEWORDS][NR_MAX_PDSCH_ENCODED_LENGTH>>5];
  unsigned char *estimated_output_bit;
  unsigned char *test_input_bit;
  unsigned char *test_input;
  unsigned int errors_bit_uncoded;
  unsigned int errors_bit;
  uint8_t  bit_index;
  uint32_t errors_scrambling;
  uint32_t scrambling_index;
  uint8_t symbol;
  int16_t **tx_layers;
  int32_t *mod_symbols[MAX_NUM_NR_RE];
  uint16_t n_dmrs;
  uint8_t dmrs_type;
  uint8_t mapping_type;
  int amp;


  test_input           = (unsigned char *) malloc16(sizeof(unsigned char) * TBS / 8);
  test_input_bit       = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
  estimated_output_bit = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
  

  for (i = 0; i < TBS / 8; i++)
    test_input[i] = 1;//(unsigned char) rand();

  for (cwd=0; cwd<nb_codewords; cwd++) {

    /////////////////////////[adk] preparing NR_UE_ULSCH_t parameters///////////////////////// A HOT FIX until creating nfapi_nr_ul_config_ulsch_pdu_rel15_t
    ///////////
    ulsch_ue[cwd]->nb_re_dmrs = nb_re_dmrs;
    ulsch_ue[cwd]->length_dmrs =  length_dmrs;
    ulsch_ue[cwd]->rnti = n_rnti;
    ulsch_ue[cwd]->Nsc_pusch = nb_rb*NR_NB_SC_PER_RB;
    ulsch_ue[cwd]->Nsymb_pusch = nb_symb_sch;
    ///////////
    ////////////////////////////////////////////////////////////////////////////////////////////


    /////////////////////////[adk] preparing UL harq_process parameters/////////////////////////
    ///////////
    harq_process_ul_ue = ulsch_ue[cwd]->harq_processes[harq_pid];

    N_PRB_oh   = 0; // higher layer (RRC) parameter xOverhead in PUSCH-ServingCellConfig
    N_RE_prime = NR_NB_SC_PER_RB*nb_symb_sch - nb_re_dmrs - N_PRB_oh;

    if (harq_process_ul_ue) {

      harq_process_ul_ue->mcs = Imcs;
      harq_process_ul_ue->Nl = Nl;
      harq_process_ul_ue->nb_rb = nb_rb;
      harq_process_ul_ue->number_of_symbols = nb_symb_sch;
      harq_process_ul_ue->num_of_mod_symbols = N_RE_prime*nb_rb*nb_codewords;
      harq_process_ul_ue->rvidx = rvidx;
      harq_process_ul_ue->TBS = TBS;
      harq_process_ul_ue->a = &test_input[0];

    }
    ///////////
    ////////////////////////////////////////////////////////////////////////////////////////////

    #ifdef DEBUG_ULSCHSIM
      for (i = 0; i < TBS / 8; i++) printf("test_input[i]=%d \n",test_input[i]);
    #endif


    /////////////////////////ULSCH coding/////////////////////////
    ///////////

    if (input_fd == NULL) {
      nr_ulsch_encoding(ulsch_ue[cwd], frame_parms, harq_pid);
    }

    ///////////
    ////////////////////////////////////////////////////////////////////


    /////////////////////////ULSCH scrambling/////////////////////////
    ///////////

    memset(scrambled_output[cwd], 0, ((available_bits>>5)+1)*sizeof(uint32_t));

    nr_pusch_codeword_scrambling(ulsch_ue[cwd]->g,
                                 available_bits,
                                 Nid_cell,
                                 ulsch_ue[cwd]->rnti,
                                 scrambled_output[cwd]); // assume one codeword for the moment


    /////////////
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////ULSCH modulation/////////////////////////
    ///////////

    nr_modulation(scrambled_output[cwd], // assume one codeword for the moment
                  available_bits,
                  mod_order,
                  (int16_t *)ulsch_ue[cwd]->d_mod);

    pusch_transform_precoding(ulsch_ue[cwd], frame_parms, harq_pid);

    ///////////
    ////////////////////////////////////////////////////////////////////////

    mod_symbols[cwd] = (int32_t *)malloc16((NR_MAX_PUSCH_ENCODED_LENGTH)*sizeof(int32_t*));

    memcpy(mod_symbols[cwd],ulsch_ue[cwd]->d_mod,(available_bits/mod_order)*sizeof(int32_t));

  }

  /////////////////////////DMRS Modulation/////////////////////////
  ///////////

  pusch_dmrs = UE->nr_gold_pusch_dmrs[slot];
  n_dmrs = (nb_rb*nb_re_dmrs);
  int16_t mod_dmrs[n_dmrs<<1];
  dmrs_type = UE->dmrs_UplinkConfig.pusch_dmrs_type;
  mapping_type = UE->pusch_config.pusch_TimeDomainResourceAllocation[0]->mappingType;

  l0 = get_l0_ul(mapping_type, 2);
  nr_modulation(pusch_dmrs[l0][0], n_dmrs*2, DMRS_MOD_ORDER, mod_dmrs); // currently only codeword 0 is modulated. Qm = 2 as DMRS is QPSK modulated


  ///////////
  ////////////////////////////////////////////////////////////////////////

  /////////////////////////ULSCH layer mapping/////////////////////////
  ///////////

  tx_layers = (int16_t **)pusch_ue->txdataF_layers;

  nr_layer_mapping((int16_t **)mod_symbols,
                   harq_process_ul_ue->Nl,
                   available_bits/mod_order,
                   tx_layers);


  ///////////
  ////////////////////////////////////////////////////////////////////////

  /////////////////////////ULSCH RE mapping/////////////////////////
  ///////////

  txdataF = UE->common_vars.txdataF;
  amp = AMP;

  start_rb = 0;
  start_sc = frame_parms->first_carrier_offset + start_rb*NR_NB_SC_PER_RB;

  if (start_sc >= frame_parms->ofdm_symbol_size)
    start_sc -= frame_parms->ofdm_symbol_size;

  for (ap=0; ap<harq_process_ul_ue->Nl; ap++) {

    // DMRS params for this ap
    get_Wt(Wt, ap, dmrs_type);
    get_Wf(Wf, ap, dmrs_type);
    delta = get_delta(ap, dmrs_type);
    l_prime[0] = 0; // single symbol ap 0
    uint8_t dmrs_symbol = l0+l_prime[0]; // Assuming dmrs-AdditionalPosition = 0

    uint8_t k_prime=0, l;
    uint16_t m=0, n=0, dmrs_idx=0, k=0;

    for (l=start_symbol; l<start_symbol+nb_symb_sch; l++) {

      k = start_sc;

      for (i=0; i<nb_rb*NR_NB_SC_PER_RB; i++) {

        sample_offsetF = l*frame_parms->ofdm_symbol_size + k;

        if ((l == dmrs_symbol) && (k == ((start_sc+get_dmrs_freq_idx_ul(n, k_prime, delta, dmrs_type))%(frame_parms->ofdm_symbol_size)))) {

          ((int16_t*)txdataF[ap])[(sample_offsetF)<<1] = (Wt[l_prime[0]]*Wf[k_prime]*amp*mod_dmrs[dmrs_idx<<1]) >> 15;
          ((int16_t*)txdataF[ap])[((sample_offsetF)<<1) + 1] = (Wt[l_prime[0]]*Wf[k_prime]*amp*mod_dmrs[(dmrs_idx<<1) + 1]) >> 15;

          #ifdef DEBUG_PUSCH_MAPPING
            printf("dmrs_idx %d\t l %d \t k %d \t k_prime %d \t n %d \t txdataF: %d %d\n",
            dmrs_idx, l, k, k_prime, n, ((int16_t*)txdataF[ap])[(sample_offsetF)<<1],
            ((int16_t*)txdataF[ap])[((sample_offsetF)<<1) + 1]);
          #endif

          dmrs_idx++;
          k_prime++;
          k_prime&=1;
          n+=(k_prime)?0:1;
        }

        else {

          ((int16_t*)txdataF[ap])[(sample_offsetF)<<1] = (amp * tx_layers[ap][m<<1]) >> 15;
          ((int16_t*)txdataF[ap])[((sample_offsetF)<<1) + 1] = (amp * tx_layers[ap][(m<<1) + 1]) >> 15;

          #ifdef DEBUG_PUSCH_MAPPING
            printf("m %d\t l %d \t k %d \t txdataF: %d %d\n",
            m, l, k, ((int16_t*)txdataF[ap])[(sample_offsetF)<<1],
            ((int16_t*)txdataF[ap])[((sample_offsetF)<<1) + 1]);
          #endif

          m++;
        }

        if (++k >= frame_parms->ofdm_symbol_size)
          k -= frame_parms->ofdm_symbol_size;
      }
    }
  }

  ///////////
  ////////////////////////////////////////////////////////////////////////


  /////////////////////////IFFT///////////////////////
  ///////////

  tx_offset = slot*frame_parms->samples_per_slot;
  txdata = UE->common_vars.txdata;

  for (ap=0; ap<harq_process_ul_ue->Nl; ap++) {
      if (frame_parms->Ncp == 1) { // extended cyclic prefix
  PHY_ofdm_mod(txdataF[ap],
         &txdata[ap][tx_offset],
         frame_parms->ofdm_symbol_size,
         12,
         frame_parms->nb_prefix_samples,
         CYCLIC_PREFIX);
      } else { // normal cyclic prefix
  nr_normal_prefix_mod(txdataF[ap],
           &txdata[ap][tx_offset],
           14,
           frame_parms);
      }
    }

  ///////////
  ////////////////////////////////////////////////////

  for (i=0; i<frame_length_complex_samples; i++) {
    for (ap=0; ap<frame_parms->nb_antennas_tx; ap++) {
      r_re[ap][i] = ((int16_t *)txdata[ap])[(i<<1)];
      r_im[ap][i] = ((int16_t *)txdata[ap])[(i<<1)+1];
    }
  }

  txlev = (double) signal_energy_amp_shift(&txdata[0][tx_offset + 5*frame_parms->ofdm_symbol_size + 4*frame_parms->nb_prefix_samples + frame_parms->nb_prefix_samples0],
          frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples);

  txlev = txlev/512.0; // output of signal_energy is fixed point representation


  for (SNR = snr0; SNR < snr1; SNR += snr_step) {

    n_errors = 0;
    n_false_positive = 0;

    SNR_lin = pow(10, SNR / 10.0);
    sigma   = 1.0 / sqrt(2 * SNR_lin);

    //AWGN
    sigma2_dB = 10*log10((double)txlev)-SNR;
    sigma2 = pow(10,sigma2_dB/10);

    
    for (trial = 0; trial < n_trials; trial++) {

      errors_bit_uncoded = 0;
      errors_scrambling  = 0;
      errors_bit         = 0;
      scrambling_index   = 0;

      for (i=0; i<frame_length_complex_samples; i++) {
        for (ap=0; ap<frame_parms->nb_antennas_rx; ap++) {
          ((short*) gNB->common_vars.rxdata[ap])[2*i]   = (r_re[ap][i] + (int16_t)(sqrt(sigma2/2)*gaussdouble(0.0,1.0)*512.0)); // convert to fixed point
          ((short*) gNB->common_vars.rxdata[ap])[2*i+1] = (r_im[ap][i] + (int16_t)(sqrt(sigma2/2)*gaussdouble(0.0,1.0)*512.0));
        }
      }

      for (i = 0; i < available_bits; i++) {

#ifdef DEBUG_CODER
        if ((i&0xf)==0)
          printf("\ne %d..%d:    ",i,i+15);
#endif


////////////////////////////////////////////
// Modulate bit-wise the scrambled output //
////////////////////////////////////////////

        bit_index = i & 0x1f;

        if ((bit_index == 0) && (i != 0)) {
          scrambling_index++;
        }

        if(((scrambled_output[0][scrambling_index] >> bit_index) & 1) == 0)
            modulated_input[i] = 1.0;     ///sqrt(2);  //QPSK
        else
          modulated_input[i] = -1.0;    ///sqrt(2);

////////////////////////////////////////////

#if 1
        channel_output_fixed[i] = (short) quantize(sigma / 4.0 / 4.0,
                                                   modulated_input[i] + sigma * gaussdouble(0.0, 1.0),
                                                   qbits);
#else
        channel_output_fixed[i] = (short) quantize(0.01, modulated_input[i], qbits);
#endif
        //channel_output_fixed[i] = (char)quantize8bit(sigma/4.0,(2.0*modulated_input[i]) - 1.0 + sigma*gaussdouble(0.0,1.0));
        //printf("channel_output_fixed[%d]: %d\n",i,channel_output_fixed[i]);

        //Uncoded BER
        if (channel_output_fixed[i] < 0)
            channel_output_uncoded[i] = 1;  //QPSK demod
        else
            channel_output_uncoded[i] = 0;

        if (channel_output_uncoded[i] != ((scrambled_output[0][scrambling_index] >> bit_index) & 1)) {
            errors_bit_uncoded = errors_bit_uncoded + 1;
        }
      }

      printf("errors bits uncoded = %u\n", errors_bit_uncoded);


#ifdef DEBUG_CODER
      printf("\n");
      exit(-1);
#endif
      

      //----------------------------------------------------------
      //-------------------- LLRs computation --------------------
      //----------------------------------------------------------

      int sch_sym_start   = NR_SYMBOLS_PER_SLOT-nb_symb_sch;
      uint32_t nb_re;
      uint32_t d_mod_offset = 0;
      uint32_t llr_offset   = 0;

      for(symbol = sch_sym_start; symbol < 14; symbol++) {

        if (symbol == 2)  // [hna] here it is assumed that symbol 2 carries 6 DMRS REs (dmrs-type 1)
        nb_re = nb_rb*6;
        else
        nb_re = nb_rb*12;

        nr_ulsch_compute_llr(&ulsch_ue[0]->d_mod[d_mod_offset],
                             gNB->pusch_vars[UE_id]->ul_ch_mag,
                             gNB->pusch_vars[UE_id]->ul_ch_magb,
                             &gNB->pusch_vars[UE_id]->llr[llr_offset],
                             nb_re,
                             symbol,
                             rel15_ul->Qm);

        d_mod_offset = d_mod_offset +  nb_re;                 // [hna] d_mod is incremented by  nb_re
        llr_offset   = llr_offset   + (nb_re * rel15_ul->Qm); // [hna] llr   is incremented by (nb_re*mod_order) because each RE has (mod_order) coded bit (i.e LLRs)
      }

      ////////////////////////////////////////////////////////////
      

      //----------------------------------------------------------
      //------------------- ULSCH unscrambling -------------------
      //----------------------------------------------------------

      nr_ulsch_unscrambling(gNB->pusch_vars[UE_id]->llr, available_bits, 0, Nid_cell, n_rnti);

      ////////////////////////////////////////////////////////////
      

      //----------------------------------------------------------
      //--------------------- ULSCH decoding ---------------------
      //----------------------------------------------------------

      ret = nr_ulsch_decoding(gNB, UE_id, gNB->pusch_vars[UE_id]->llr, frame_parms, frame,
                              nb_symb_sch, subframe, harq_pid, is_crnti, llr8_flag);

      if (ret > ulsch_gNB->max_ldpc_iterations)
        n_errors++;

      ////////////////////////////////////////////////////////////


      //----------------------------------------------------------
      //---------------------- count errors ----------------------
      //----------------------------------------------------------      

      for (i = 0; i < TBS; i++) {
        
        if(((ulsch_ue[0]->g[i] == 0) && (gNB->pusch_vars[UE_id]->llr[i] <= 0)) || 
           ((ulsch_ue[0]->g[i] == 1) && (gNB->pusch_vars[UE_id]->llr[i] >= 0)))
        {
          if(errors_scrambling == 0)
            printf("First bit in error = %d\n",i);
          errors_scrambling++;
        }

        estimated_output_bit[i] = (ulsch_gNB->harq_processes[harq_pid]->b[i/8] & (1 << (i & 7))) >> (i & 7);
        test_input_bit[i]       = (test_input[i / 8] & (1 << (i & 7))) >> (i & 7); // Further correct for multiple segments

        if (estimated_output_bit[i] != test_input_bit[i]) {
          errors_bit++;
        }
        
      }

      ////////////////////////////////////////////////////////////

      if (errors_scrambling > 0) {
        if (n_trials == 1)
          printf("errors_scrambling %d (trial %d)\n", errors_scrambling, trial);
      }

      if (errors_bit > 0) {
        n_false_positive++;
        if (n_trials == 1)
          printf("errors_bit %d (trial %d)\n", errors_bit, trial);
      }
    } // [hna] for (trial = 0; trial < n_trials; trial++)
    
    printf("*****************************************\n");
    printf("SNR %f, BLER %f (false positive %f)\n", SNR,
           (float) n_errors / (float) n_trials,
           (float) n_false_positive / (float) n_trials);
    printf("*****************************************\n");

    if ((float) n_errors / (float) n_trials < target_error_rate) {
      printf("PUSCH test OK\n");
      break;
    }
  } // [hna] for (SNR = snr0; SNR < snr1; SNR += snr_step)


  for (i = 0; i < 2; i++) {

    printf("----------------------\n");
    printf("freeing codeword %d\n", i);
    printf("----------------------\n");

    printf("gNB ulsch[0][%d]\n", i); // [hna] ulsch[0] is for RA

    free_gNB_ulsch(gNB->ulsch[0][i]);

    printf("gNB ulsch[%d][%d]\n",UE_id, i);

    free_gNB_ulsch(gNB->ulsch[UE_id][i]);

    for (sf = 0; sf < 2; sf++) {

      printf("UE  ulsch[%d][0][%d]\n", sf, i);

      if (UE->ulsch[sf][0][i])
        free_nr_ue_ulsch(UE->ulsch[sf][0][i]);
    }

    printf("\n");
  }

  for (i = 0; i < 2; i++) {
    free(r_re[i]);
    free(r_im[i]);
  }

  free(r_re);
  free(r_im);

  if (output_fd)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  return (n_errors);
}
