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
#include "PHY/types.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/defs_gNB.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "SCHED_NR/sched_nr.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
#include "openair1/SIMULATION/RF/rf.h"
#include "openair1/SIMULATION/NR_PHY/nr_unitary_defs.h"
#include "openair1/SIMULATION/NR_PHY/nr_dummy_functions.c"
#include "common/utils/threadPool/thread-pool.h"

//#define DEBUG_NR_ULSCHSIM

PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];

void init_downlink_harq_status(NR_DL_UE_HARQ_t *dl_harq) {}

uint8_t const nr_rv_round_map[4] = {0, 2, 1, 3};
uint8_t const nr_rv_round_map_ue[4] = {0, 2, 1, 3};

double cpuf;
//uint8_t nfapi_mode = 0;
uint16_t NB_UE_INST = 1;

// needed for some functions
PHY_VARS_NR_UE *PHY_vars_UE_g[1][1] = { { NULL } };
uint16_t n_rnti = 0x1234;
openair0_config_t openair0_cfg[MAX_CARDS];

int nr_postDecode_sim(PHY_VARS_gNB *gNB, notifiedFIFO_elt_t *req) {
  ldpcDecode_t *rdata = (ldpcDecode_t*) NotifiedFifoData(req);
  NR_UL_gNB_HARQ_t *ulsch_harq = rdata->ulsch_harq;
  NR_gNB_ULSCH_t *ulsch = rdata->ulsch;
  int r = rdata->segment_r;

  bool decodeSuccess = (rdata->decodeIterations <= rdata->decoderParms.numMaxIter);
  ulsch_harq->processedSegments++;
  gNB->nbDecode--;

  if (decodeSuccess) {
    memcpy(ulsch_harq->b+rdata->offset,
           ulsch_harq->c[r],
           rdata->Kr_bytes - (ulsch_harq->F>>3) -((ulsch_harq->C>1)?3:0));
  } else {
    if ( rdata->nbSegments != ulsch_harq->processedSegments ) {
      int nb=abortTpool(gNB->threadPool, req->key);
      nb+=abortNotifiedFIFO(gNB->respDecode, req->key);
      gNB->nbDecode-=nb;
      AssertFatal(ulsch_harq->processedSegments+nb == rdata->nbSegments,"processed: %d, aborted: %d, total %d\n",
      ulsch_harq->processedSegments, nb, rdata->nbSegments);
      ulsch_harq->processedSegments=rdata->nbSegments;
      return 1;
    }
  }

  // if all segments are done 
  if (rdata->nbSegments == ulsch_harq->processedSegments) {
    if (decodeSuccess) {
      return 0;
    } else {
      return 1;
      }

    }
    ulsch->last_iteration_cnt = rdata->decodeIterations;
  return 0;
}
int main(int argc, char **argv)
{
  char c;
  int i,sf;
  double SNR, snr0 = -2.0, snr1 = 2.0, SNR_lin;
  double snr_step = 0.1;
  uint8_t snr1set = 0;
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
  //unsigned char pbch_phase = 0;
  int frame = 0, subframe = 0;
  NR_DL_FRAME_PARMS *frame_parms;
  double sigma;
  unsigned char qbits = 8;
  int ret=0;
  int loglvl = OAILOG_WARNING;
  uint64_t SSB_positions=0x01;
  uint16_t nb_symb_sch = 12;
  uint16_t nb_rb = 50;
  uint8_t Imcs = 9;

  double DS_TDL = .03;

  cpuf = get_cpu_freq_GHz();

  if (load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY) == 0) {
    exit_fun("[NR_ULSCHSIM] Error, configuration module init failed\n");
  }

  //logInit();
  randominit(0);

  //while ((c = getopt(argc, argv, "df:hpg:i:j:n:l:m:r:s:S:y:z:M:N:F:R:P:")) != -1) {
  while ((c = getopt(argc, argv, "hg:n:s:S:py:z:M:N:R:F:m:l:r:")) != -1) {
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
            printf("Unsupported channel model! Exiting.\n");
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
#ifdef DEBUG_NR_ULSCHSIM
        printf("n_trials (-n) = %d\n", n_trials);
#endif
        break;

      case 's':
        snr0 = atof(optarg);
#ifdef DEBUG_NR_ULSCHSIM
        printf("Setting SNR0 to %f\n", snr0);
#endif
        break;

      case 'S':
        snr1 = atof(optarg);
        snr1set = 1;
#ifdef DEBUG_NR_ULSCHSIM
        printf("Setting SNR1 to %f\n", snr1);
#endif
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
          printf("Unsupported number of TX antennas %d. Exiting.\n", n_tx);
          exit(-1);
        }

        break;

      case 'z':
        n_rx = atoi(optarg);

        if ((n_rx == 0) || (n_rx > 2)) {
          printf("Unsupported number of RX antennas %d. Exiting.\n", n_rx);
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
        N_RB_UL = atoi(optarg);
#ifdef DEBUG_NR_ULSCHSIM
        printf("N_RB_UL (-R) = %d\n", N_RB_UL);
#endif
        break;

      case 'F':
        input_fd = fopen(optarg, "r");

        if (input_fd == NULL) {
            printf("Problem with filename %s. Exiting.\n", optarg);
            exit(-1);
        }

        break;

      /*case 'P':
        pbch_phase = atoi(optarg);
        if (pbch_phase > 3)
          printf("Illegal PBCH phase (0-3) got %d\n", pbch_phase);
        break;*/

      case 'm':
        Imcs = atoi(optarg);
#ifdef DEBUG_NR_ULSCHSIM
        printf("Imcs (-m) = %d\n", Imcs);
#endif
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
          printf("%s -h(elp) -g channel_model -n n_frames -s snr0 -S snr1 -p(extended_prefix) -y TXant -z RXant -M -N cell_id -R -F input_filename -m -l -r\n", argv[0]);
          //printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n", argv[0]);
          printf("-h This message\n");
          printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
          printf("-n Number of frames to simulate\n");
          //printf("-d Use TDD\n");
          printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
          printf("-S Ending SNR, runs from SNR0 to SNR1\n");
          printf("-p Use extended prefix mode\n");
          //printf("-t Delay spread for multipath channel\n");
          //printf("-x Transmission mode (1,2,6 for the moment)\n");
          printf("-y Number of TX antennas used in eNB\n");
          printf("-z Number of RX antennas used in UE\n");
          //printf("-i Relative strength of first intefering eNB (in dB) - cell_id mod 3 = 1\n");
          //printf("-j Relative strength of second intefering eNB (in dB) - cell_id mod 3 = 2\n");
          printf("-M Multiple SSB positions in burst\n");
          printf("-N Nid_cell\n");
          printf("-R N_RB_UL\n");
          printf("-F Input filename (.txt format) for RX conformance testing\n");
          printf("-m\n");
          printf("-l\n");
          printf("-r\n");
          //printf("-O oversampling factor (1,2,4,8,16)\n");
          //printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
          //printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
          //printf("-f Output filename (.txt format) for Pe/SNR results\n");
          exit(-1);
          break;
    }
  }

  logInit();
  set_glog(loglvl);
  T_stdout = 1;

  if (snr1set == 0)
    snr1 = snr0 + 10;

  gNB2UE = new_channel_desc_scm(n_tx,
		                n_rx,
				channel_model,
                                61.44e6, //N_RB2sampling_rate(N_RB_DL),
                                40e6, //N_RB2channel_bandwidth(N_RB_DL),
                                DS_TDL,
                                0,0,0, 0);

  if (gNB2UE == NULL) {
    printf("Problem generating channel model. Exiting.\n");
    exit(-1);
  }


  RC.gNB = (PHY_VARS_gNB **) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0] = malloc(sizeof(PHY_VARS_gNB));
  gNB = RC.gNB[0];
  //gNB_config = &gNB->gNB_config;

  gNB->threadPool = (tpool_t*)malloc(sizeof(tpool_t));
  gNB->respDecode = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  char tp_param[] = "n";
  initTpool(tp_param, gNB->threadPool, true);
  initNotifiedFIFO(gNB->respDecode);
  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
  frame_parms->nb_antennas_tx = n_tx;
  frame_parms->nb_antennas_rx = n_rx;
  frame_parms->N_RB_DL = N_RB_DL;
  frame_parms->N_RB_UL = N_RB_UL;
  frame_parms->Ncp = extended_prefix_flag ? EXTENDED : NORMAL;
  crcTableInit();

  memcpy(&gNB->frame_parms, frame_parms, sizeof(NR_DL_FRAME_PARMS));

  nr_phy_config_request_sim(gNB, N_RB_UL, N_RB_UL, mu, Nid_cell, SSB_positions);

  phy_init_nr_gNB(gNB, 0, 1); //lowmem

  //configure UE
  UE = malloc(sizeof(PHY_VARS_NR_UE));
  memcpy(&UE->frame_parms, frame_parms, sizeof(NR_DL_FRAME_PARMS));

  //phy_init_nr_top(frame_parms);
  if (init_nr_ue_signal(UE, 1, 0) != 0) {
    printf("Error at UE NR initialisation.\n");
    exit(-1);
  }

  for (sf = 0; sf < 2; sf++) {
    for (i = 0; i < 2; i++) {

        UE->ulsch[sf][0][i] = new_nr_ue_ulsch(N_RB_UL, 8, 0);

        if (!UE->ulsch[sf][0][i]) {
          printf("Can't get ue ulsch structures.\n");
          exit(-1);
        }

    }
  }

  unsigned char harq_pid = 0;
  unsigned int TBS = 8424;
  unsigned int available_bits;
  uint8_t nb_re_dmrs = 6;
  uint8_t length_dmrs = 1;
  uint8_t N_PRB_oh;
  uint16_t N_RE_prime,code_rate;
  unsigned char mod_order;
  uint8_t Nl = 1;
  uint8_t rvidx = 0;
  uint8_t UE_id = 0;

  NR_gNB_ULSCH_t *ulsch_gNB = gNB->ulsch[UE_id][0];
  NR_UL_gNB_HARQ_t *harq_process_gNB = ulsch_gNB->harq_processes[harq_pid];
  nfapi_nr_pusch_pdu_t *rel15_ul = &harq_process_gNB->ulsch_pdu;

  NR_UE_ULSCH_t *ulsch_ue = UE->ulsch[0][0][0];

  mod_order = nr_get_Qm_ul(Imcs, 0);
  code_rate = nr_get_code_rate_ul(Imcs, 0);
  available_bits = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, 1);
  TBS = nr_compute_tbs(mod_order,code_rate, nb_rb, nb_symb_sch, nb_re_dmrs*length_dmrs, 0, 0, Nl);

  printf("\nAvailable bits %u TBS %u mod_order %d\n", available_bits, TBS, mod_order);

  /////////// setting rel15_ul parameters ///////////
  rel15_ul->rb_size             = nb_rb;
  rel15_ul->nr_of_symbols       = nb_symb_sch;
  rel15_ul->qam_mod_order       = mod_order;
  rel15_ul->mcs_index           = Imcs;
  rel15_ul->pusch_data.rv_index = rvidx;
  rel15_ul->nrOfLayers          = Nl;
  rel15_ul->target_code_rate    = code_rate;
  rel15_ul->pusch_data.tb_size  = TBS/8;
  ///////////////////////////////////////////////////

  double *modulated_input = malloc16(sizeof(double) * 16 * 68 * 384); // [hna] 16 segments, 68*Zc
  short *channel_output_fixed = malloc16(sizeof(short) * 16 * 68 * 384);
  short *channel_output_uncoded = malloc16(sizeof(unsigned short) * 16 * 68 * 384);
  unsigned int errors_bit_uncoded = 0;
  unsigned char *estimated_output_bit;
  unsigned char *test_input_bit;
  unsigned int errors_bit = 0;

  test_input_bit = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
  estimated_output_bit = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);

  unsigned char *test_input;
  test_input = (unsigned char *) malloc16(sizeof(unsigned char) * TBS / 8);

  for (i = 0; i < TBS / 8; i++)
    test_input[i] = (unsigned char) rand();

  /////////////////////////[adk] preparing UL harq_process parameters/////////////////////////
  ///////////
  NR_UL_UE_HARQ_t *harq_process_ul_ue = ulsch_ue->harq_processes[harq_pid];

  N_PRB_oh   = 0; // higher layer (RRC) parameter xOverhead in PUSCH-ServingCellConfig
  N_RE_prime = NR_NB_SC_PER_RB*nb_symb_sch - nb_re_dmrs - N_PRB_oh;

  if (harq_process_ul_ue) {

    harq_process_ul_ue->pusch_pdu.rnti = n_rnti;
    harq_process_ul_ue->pusch_pdu.mcs_index = Imcs;
    harq_process_ul_ue->pusch_pdu.nrOfLayers = Nl;
    harq_process_ul_ue->pusch_pdu.rb_size = nb_rb;
    harq_process_ul_ue->pusch_pdu.nr_of_symbols = nb_symb_sch;
    harq_process_ul_ue->num_of_mod_symbols = N_RE_prime*nb_rb*nb_codewords;
    harq_process_ul_ue->pusch_pdu.pusch_data.rv_index = rvidx;
    harq_process_ul_ue->pusch_pdu.pusch_data.tb_size  = TBS/8;
    harq_process_ul_ue->a = &test_input[0];

  }
  ///////////
  ////////////////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_NR_ULSCHSIM
  for (i = 0; i < TBS / 8; i++) printf("test_input[i]=%hhu \n",test_input[i]);
#endif

  /////////////////////////ULSCH coding/////////////////////////
  ///////////
  unsigned int G = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, Nl);

  if (input_fd == NULL) {
    nr_ulsch_encoding(UE, ulsch_ue, frame_parms, harq_pid, G);
  }
  
  printf("\n");

  ///////////
  ////////////////////////////////////////////////////////////////////

  for (SNR = snr0; SNR < snr1; SNR += snr_step) {
    n_errors = 0;
    n_false_positive = 0;

    for (trial = 0; trial < n_trials; trial++) {

      errors_bit_uncoded = 0;

      for (i = 0; i < available_bits; i++) {

#ifdef DEBUG_CODER
        if ((i&0xf)==0)
          printf("\ne %d..%d:    ",i,i+15);
#endif
        /*
            if (i<16){
               printf("ulsch_encoder output f[%d] = %d\n",i,ulsch_ue->harq_processes[0]->f[i]);
            }
        */

        if (ulsch_ue->g[i] == 0)
          modulated_input[i] = 1.0;        ///sqrt(2);  //QPSK
        else
          modulated_input[i] = -1.0;        ///sqrt(2);
  
        //if (i<16) printf("modulated_input[%d] = %d\n",i,modulated_input[i]);

#if 1
        SNR_lin = pow(10, SNR / 10.0);
        sigma = 1.0 / sqrt(2 * SNR_lin);
        channel_output_fixed[i] = (short) quantize(sigma / 4.0 / 4.0,
                                                   modulated_input[i] + sigma * gaussdouble(0.0, 1.0),
                                                   qbits);
#else
        channel_output_fixed[i] = (short) quantize(0.01, modulated_input[i], qbits);
#endif
        //printf("channel_output_fixed[%d]: %d\n",i,channel_output_fixed[i]);

        //Uncoded BER
        if (channel_output_fixed[i] < 0)
          channel_output_uncoded[i] = 1;  //QPSK demod
        else
          channel_output_uncoded[i] = 0;

        if (channel_output_uncoded[i] != ulsch_ue->harq_processes[harq_pid]->f[i])
          errors_bit_uncoded = errors_bit_uncoded + 1;
      }
/*
      printf("errors bits uncoded %u\n", errors_bit_uncoded);
      printf("\n");
*/
#ifdef DEBUG_CODER
      printf("\n");
      exit(-1);
#endif

     uint32_t G = nr_get_G(rel15_ul->rb_size,
                           rel15_ul->nr_of_symbols,
                           nb_re_dmrs,
                           1, // FIXME only single dmrs is implemented 
                           rel15_ul->qam_mod_order,
                           rel15_ul->nrOfLayers);

      nr_ulsch_decoding(gNB, UE_id, channel_output_fixed, frame_parms, rel15_ul,
                              frame, subframe, harq_pid, G);
      while (gNB->nbDecode > 0) {
        notifiedFIFO_elt_t *req=pullTpool(gNB->respDecode, gNB->threadPool);
        ret = nr_postDecode_sim(gNB, req);
        delNotifiedFIFO_elt(req);
      }

      if (ret)
        n_errors++;

      //count errors
      errors_bit = 0;

      for (i = 0; i < TBS; i++) {
        estimated_output_bit[i] = (ulsch_gNB->harq_processes[harq_pid]->b[i/8] & (1 << (i & 7))) >> (i & 7);
        test_input_bit[i] = (test_input[i / 8] & (1 << (i & 7))) >> (i & 7); // Further correct for multiple segments

        if (estimated_output_bit[i] != test_input_bit[i]) {
          errors_bit++;
        }
      }
/*
      if (errors_bit > 0) {
        n_false_positive++;
        if (n_trials == 1)
          printf("errors_bit %u (trial %d)\n", errors_bit, trial);
      }
      printf("\n");*/
    }
    
    printf("*****************************************\n");
    printf("SNR %f, BLER %f (false positive %f)\n", SNR,
           (float) n_errors / (float) n_trials,
           (float) n_false_positive / (float) n_trials);
    printf("*****************************************\n");
    printf("\n");

    if (n_errors == 0) {
      printf("PUSCH test OK\n");
      printf("\n");
      break;
    }
    printf("\n");
  }


  if (output_fd)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  return (n_errors);
}

