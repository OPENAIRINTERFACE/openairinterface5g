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
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include "PHY/phy_vars.h"
#include "SCHED_NR/sched_nr.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
#include "openair1/SIMULATION/RF/rf.h"
#include "openair1/SIMULATION/NR_PHY/nr_unitary_defs.h"
#include "openair1/SIMULATION/NR_PHY/nr_dummy_functions.c"
#include "openair1/PHY/MODULATION/nr_modulation.h"
#include <executables/softmodem-common.h>
//#define DEBUG_NR_PBCHSIM

PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];

double cpuf;
//uint8_t nfapi_mode = 0;
uint16_t NB_UE_INST = 1;

// needed for some functions
openair0_config_t openair0_cfg[MAX_CARDS];

uint8_t const nr_rv_round_map[4] = {0, 2, 1, 3};
uint8_t const nr_rv_round_map_ue[4] = {0, 2, 1, 3};

uint64_t get_softmodem_optmask(void) {return 0;}
softmodem_params_t *get_softmodem_params(void) {return 0;}

void init_downlink_harq_status(NR_DL_UE_HARQ_t *dl_harq) {}

int nr_ue_pdcch_procedures(uint8_t gNB_id,
			   PHY_VARS_NR_UE *ue,
			   UE_nr_rxtx_proc_t *proc,
                           int n_ss) {
  return 0;
}

int nr_ue_pdsch_procedures(PHY_VARS_NR_UE *ue,
                           UE_nr_rxtx_proc_t *proc,
                           int eNB_id, PDSCH_t pdsch,
                           NR_UE_DLSCH_t *dlsch0, NR_UE_DLSCH_t *dlsch1) {
  return 0;
}

bool nr_ue_dlsch_procedures(PHY_VARS_NR_UE *ue,
                            UE_nr_rxtx_proc_t *proc,
                            int gNB_id,
                            PDSCH_t pdsch,
                            NR_UE_DLSCH_t *dlsch0,
                            NR_UE_DLSCH_t *dlsch1,
                            int *dlsch_errors,
                            uint8_t dlsch_parallel) {
  return false;
}

void nr_phy_config_request_sim_pbchsim(PHY_VARS_gNB *gNB,
                               int N_RB_DL,
                               int N_RB_UL,
                               int mu,
                               int Nid_cell,
                               uint64_t position_in_burst)
{
  NR_DL_FRAME_PARMS *fp                                   = &gNB->frame_parms;
  nfapi_nr_config_request_scf_t *gNB_config               = &gNB->gNB_config;
  //overwrite for new NR parameters

  uint64_t rev_burst=0;
  for (int i=0; i<64; i++)
    rev_burst |= (((position_in_burst>>(63-i))&0x01)<<i);

  gNB_config->cell_config.phy_cell_id.value             = Nid_cell;
  gNB_config->ssb_config.scs_common.value               = mu;
  gNB_config->ssb_table.ssb_subcarrier_offset.value     = 0;
  gNB_config->ssb_table.ssb_offset_point_a.value        = (N_RB_DL-20)>>1;
  gNB_config->ssb_table.ssb_mask_list[1].ssb_mask.value = (rev_burst)&(0xFFFFFFFF);
  gNB_config->ssb_table.ssb_mask_list[0].ssb_mask.value = (rev_burst>>32)&(0xFFFFFFFF);
  gNB_config->cell_config.frame_duplex_type.value       = TDD;
  gNB_config->ssb_table.ssb_period.value		= 1; //10ms
  gNB_config->carrier_config.dl_grid_size[mu].value     = N_RB_DL;
  gNB_config->carrier_config.ul_grid_size[mu].value     = N_RB_UL;
  gNB_config->carrier_config.num_tx_ant.value           = fp->nb_antennas_tx;
  gNB_config->carrier_config.num_rx_ant.value           = fp->nb_antennas_rx;

  gNB_config->tdd_table.tdd_period.value = 0;
  //gNB_config->subframe_config.dl_cyclic_prefix_type.value = (fp->Ncp == NORMAL) ? NFAPI_CP_NORMAL : NFAPI_CP_EXTENDED;

  gNB->mac_enabled   = 1;
  fp->dl_CarrierFreq = 3600000000;//from_nrarfcn(gNB_config->nfapi_config.rf_bands.rf_band[0],gNB_config->nfapi_config.nrarfcn.value);
  fp->ul_CarrierFreq = 3600000000;//fp->dl_CarrierFreq - (get_uldl_offset(gNB_config->nfapi_config.rf_bands.rf_band[0])*100000);
  if (mu>2) fp->nr_band = 257;
  else fp->nr_band = 78;
  fp->threequarter_fs= 0;

  gNB_config->carrier_config.dl_bandwidth.value = config_bandwidth(mu, N_RB_DL, fp->nr_band);

  fp->ofdm_offset_divisor = UINT_MAX;
  nr_init_frame_parms(gNB_config, fp);
  init_timeshift_rotation(fp);

  init_symbol_rotation(fp);

  gNB->configured    = 1;
  LOG_I(PHY,"gNB configured\n");
}
int main(int argc, char **argv)
{
  char c;
  int i,aa,start_symbol;
  double sigma2, sigma2_dB=10,SNR,snr0=-2.0,snr1=2.0;
  double cfo=0;
  uint8_t snr1set=0;
  int **txdata;
  double **s_re,**s_im,**r_re,**r_im;
  //double iqim = 0.0;
  double ip =0.0;
  //unsigned char pbch_pdu[6];
  //  int sync_pos, sync_pos_slot;
  //  FILE *rx_frame_file;
  FILE *output_fd = NULL;
  //uint8_t write_output_file=0;
  //int result;
  //int freq_offset;
  //  int subframe_offset;
  //  char fname[40], vname[40];
  int trial,n_trials=1,n_errors=0,n_errors_payload=0;
  uint8_t transmission_mode = 1,n_tx=1,n_rx=1;
  uint16_t Nid_cell=0;
  uint64_t SSB_positions=0x01;

  channel_desc_t *gNB2UE;

  //uint8_t extended_prefix_flag=0;
  //int8_t interf1=-21,interf2=-21;

  FILE *input_fd=NULL,*pbch_file_fd=NULL;

  //uint32_t nsymb,tx_lev,tx_lev1 = 0,tx_lev2 = 0;
  //char input_val_str[50],input_val_str2[50];
  //uint8_t frame_mod4,num_pdcch_symbols = 0;
  //double pbch_sinr;
  //int pbch_tx_ant;

  SCM_t channel_model=AWGN;//Rayleigh1_anticorr;


  int N_RB_DL=273,mu=1;

  //unsigned char frame_type = 0;
  unsigned char pbch_phase = 0;

  int frame=0;
  int frame_length_complex_samples;
  __attribute__((unused))
  int frame_length_complex_samples_no_prefix;
  NR_DL_FRAME_PARMS *frame_parms;

  int ret, payload_ret=0;
  int run_initial_sync=0;

  int loglvl=OAILOG_WARNING;

  float target_error_rate = 0.01;

  cpuf = get_cpu_freq_GHz();

  if ( load_configmodule(argc,argv,CONFIG_ENABLECMDLINEONLY) == 0) {
    exit_fun("[NR_PBCHSIM] Error, configuration module init failed\n");
  }

  randominit(0);

  while ((c = getopt (argc, argv, "f:hA:pf:g:i:j:n:o:s:S:t:x:y:z:M:N:F:GR:dP:IL:m:")) != -1) {
    switch (c) {
    /*case 'f':
      write_output_file=1;
      output_fd = fopen(optarg,"w");

      if (output_fd==NULL) {
        printf("Error opening %s\n",optarg);
        exit(-1);
      }

      break;*/

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

      default:
        printf("Unsupported channel model! Exiting.\n");
        exit(-1);
      }

      break;

    /*case 'i':
      interf1=atoi(optarg);
      break;

    case 'j':
      interf2=atoi(optarg);
      break;*/

    case 'n':
      n_trials = atoi(optarg);
      break;

    case 'o':
      cfo = atof(optarg);
#ifdef DEBUG_NR_PBCHSIM
      printf("Setting CFO to %f Hz\n",cfo);
#endif
      break;

    case 's':
      snr0 = atof(optarg);
#ifdef DEBUG_NR_PBCHSIM
      printf("Setting SNR0 to %f\n",snr0);
#endif
      break;

    case 'S':
      snr1 = atof(optarg);
      snr1set=1;
#ifdef DEBUG_NR_PBCHSIM
      printf("Setting SNR1 to %f\n",snr1);
#endif
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
      transmission_mode=atoi(optarg);

      if ((transmission_mode!=1) && (transmission_mode!=2) && (transmission_mode!=6)) {
        printf("Unsupported transmission mode %d. Exiting.\n",transmission_mode);
        exit(-1);
      }

      break;

    case 'y':
      n_tx=atoi(optarg);

      if ((n_tx==0) || (n_tx>2)) {
    	printf("Unsupported number of TX antennas %d. Exiting.\n", n_tx);
        exit(-1);
      }

      break;

    case 'z':
      n_rx=atoi(optarg);

      if ((n_rx==0) || (n_rx>2)) {
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
      N_RB_DL = atoi(optarg);
      break;

    case 'F':
      input_fd = fopen(optarg,"r");

      if (input_fd==NULL) {
        printf("Problem with filename %s. Exiting.\n", optarg);
        exit(-1);
      }

      break;

    case 'P':
      pbch_phase = atoi(optarg);

      if (pbch_phase>3)
        printf("Illegal PBCH phase (0-3) got %d\n",pbch_phase);

      break;
      
    case 'I':
      run_initial_sync=1;
      target_error_rate=0.1;
      break;

    case 'L':
      loglvl = atoi(optarg);
      break;

    case 'm':
      mu = atoi(optarg);
      break;

    default:
    case 'h':
      printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n",
             argv[0]);
      printf("-h This message\n");
      //printf("-p Use extended prefix mode\n");
      //printf("-d Use TDD\n");
      printf("-n Number of frames to simulate\n");
      printf("-m Numerology index\n");
      printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
      printf("-S Ending SNR, runs from SNR0 to SNR1\n");
      printf("-t Delay spread for multipath channel\n");
      printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
      printf("-x Transmission mode (1,2,6 for the moment)\n");
      printf("-y Number of TX antennas used in eNB\n");
      printf("-z Number of RX antennas used in UE\n");
      //printf("-i Relative strength of first intefering eNB (in dB) - cell_id mod 3 = 1\n");
      //printf("-j Relative strength of second intefering eNB (in dB) - cell_id mod 3 = 2\n");
      printf("-o Carrier frequency offset in Hz\n");
      printf("-M Multiple SSB positions in burst\n");
      printf("-N Nid_cell\n");
      printf("-R N_RB_DL\n");
      printf("-O oversampling factor (1,2,4,8,16)\n");
      printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
      //printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
      //printf("-f Output filename (.txt format) for Pe/SNR results\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      exit (-1);
      break;
    }
  }

  logInit();
  set_glog(loglvl);
  T_stdout = 1;

  if (snr1set==0)
    snr1 = snr0+10;

  printf("Initializing gNodeB for mu %d, N_RB_DL %d\n",mu,N_RB_DL);

  RC.gNB = (PHY_VARS_gNB**) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0] = malloc(sizeof(PHY_VARS_gNB));
  gNB = RC.gNB[0];
  gNB->ofdm_offset_divisor = UINT_MAX;
  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
  frame_parms->nb_antennas_tx = n_tx;
  frame_parms->nb_antennas_rx = n_rx;
  frame_parms->N_RB_DL = N_RB_DL;
  frame_parms->Nid_cell = Nid_cell;
  frame_parms->nushift = Nid_cell%4;
  frame_parms->ssb_type = nr_ssb_type_C;

  nr_phy_config_request_sim_pbchsim(gNB,N_RB_DL,N_RB_DL,mu,Nid_cell,SSB_positions);
  phy_init_nr_gNB(gNB,0,1);
  nr_set_ssb_first_subcarrier(&gNB->gNB_config,frame_parms);

  uint8_t n_hf = 0;
  int cyclic_prefix_type = NFAPI_CP_NORMAL;

  double fs=0, eps;
  double scs = 30000;
  double bw = 100e6;
  
  switch (mu) {
    case 1:
      scs = 30000;
      frame_parms->Lmax = 8;
      if (N_RB_DL == 217) {
        fs = 122.88e6;
        bw = 80e6;
      }
      else if (N_RB_DL == 245) {
        fs = 122.88e6;
        bw = 90e6;
      }
      else if (N_RB_DL == 273) {
        fs = 122.88e6;
        bw = 100e6;
      }
      else if (N_RB_DL == 106) {
        fs = 61.44e6;
        bw = 40e6;
      }
      else AssertFatal(1==0,"Unsupported numerology for mu %d, N_RB %d\n",mu, N_RB_DL);
      break;
    case 3:
      frame_parms->Lmax = 64;
      scs = 120000;
      if (N_RB_DL == 66) {
        fs = 122.88e6;
        bw = 100e6;
      }
      else AssertFatal(1==0,"Unsupported numerology for mu %d, N_RB %d\n",mu, N_RB_DL);
      break;
  }

  // cfo with respect to sub-carrier spacing
  eps = cfo/scs;

  // computation of integer and fractional FO to compare with estimation results
  int IFO;
  if(eps!=0.0){
	printf("Introducing a CFO of %lf relative to SCS of %d kHz\n",eps,(int)(scs/1000));
	if (eps>0)	
  	  IFO=(int)(eps+0.5);
	else
	  IFO=(int)(eps-0.5);
	printf("FFO = %lf; IFO = %d\n",eps-IFO,IFO);
  }

  gNB2UE = new_channel_desc_scm(n_tx,
                                n_rx,
                                channel_model,
 				fs, 
				bw, 
				300e-9,
                                0,
                                0,
                                0, 0);

  if (gNB2UE==NULL) {
	printf("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  frame_length_complex_samples = frame_parms->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
  frame_length_complex_samples_no_prefix = frame_parms->samples_per_subframe_wCP;

  s_re = malloc(2*sizeof(double*));
  s_im = malloc(2*sizeof(double*));
  r_re = malloc(2*sizeof(double*));
  r_im = malloc(2*sizeof(double*));
  txdata = malloc(2*sizeof(int*));

  for (i=0; i<2; i++) {

    s_re[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(s_re[i],frame_length_complex_samples*sizeof(double));
    s_im[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(s_im[i],frame_length_complex_samples*sizeof(double));

    r_re[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(r_re[i],frame_length_complex_samples*sizeof(double));
    r_im[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(r_im[i],frame_length_complex_samples*sizeof(double));

    printf("Allocating %d samples for txdata\n",frame_length_complex_samples);
    txdata[i] = malloc(frame_length_complex_samples*sizeof(int));
    bzero(r_re[i],frame_length_complex_samples*sizeof(int));
  }

  if (pbch_file_fd!=NULL) {
    load_pbch_desc(pbch_file_fd);
  }


  //configure UE
  UE = malloc(sizeof(PHY_VARS_NR_UE));
  memcpy(&UE->frame_parms,frame_parms,sizeof(NR_DL_FRAME_PARMS));
  //phy_init_nr_top(UE); //called from init_nr_ue_signal
  if (run_initial_sync==1)  UE->is_synchronized = 0;
  else                      UE->is_synchronized = 1;
                      
  UE->perfect_ce = 0;

  if(eps!=0.0)
	UE->UE_fo_compensation = 1; // if a frequency offset is set then perform fo estimation and compensation

  if (init_nr_ue_signal(UE, 1, 0) != 0) {
    printf("Error at UE NR initialisation\n");
    exit(-1);
  }

  nr_gold_pbch(UE);

  // generate signal
  if (input_fd==NULL) {

    for (i=0; i<frame_parms->Lmax; i++) {
      if((SSB_positions >> i) & 0x01) {

        gNB->ssb[i].ssb_pdu.ssb_pdu_rel15.bchPayload = 0x55dd33;
        gNB->ssb[i].ssb_pdu.ssb_pdu_rel15.SsbBlockIndex = i;

        start_symbol = nr_get_ssb_start_symbol(frame_parms,i);
        int slot = start_symbol/14;

        for (aa=0; aa<gNB->frame_parms.nb_antennas_tx; aa++)
          memset(gNB->common_vars.txdataF[aa],0,frame_parms->samples_per_slot_wCP*sizeof(int32_t));

        nr_common_signal_procedures (gNB,frame,slot,gNB->ssb[i].ssb_pdu);

        for (aa=0; aa<gNB->frame_parms.nb_antennas_tx; aa++) {
          if (cyclic_prefix_type == 1) {
            PHY_ofdm_mod(gNB->common_vars.txdataF[aa],
            &txdata[aa][frame_parms->get_samples_slot_timestamp(slot,frame_parms,0)],
            frame_parms->ofdm_symbol_size,
            12,
            frame_parms->nb_prefix_samples,
            CYCLIC_PREFIX);
          } else {
            /*nr_normal_prefix_mod(gNB->common_vars.txdataF[aa],
              &txdata[aa][frame_parms->get_samples_slot_timestamp(slot,frame_parms,0)],
              14,
              frame_parms);*/
            PHY_ofdm_mod(gNB->common_vars.txdataF[aa],
                         (int*)&txdata[aa][frame_parms->get_samples_slot_timestamp(slot,frame_parms,0)],
                         frame_parms->ofdm_symbol_size,
                         1,
                         frame_parms->nb_prefix_samples0,
                         CYCLIC_PREFIX);

            apply_nr_rotation(frame_parms,
                              (int16_t*)&txdata[aa][frame_parms->get_samples_slot_timestamp(slot,frame_parms,0)],
                              slot,
                              0,
                              1,
                              frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples0);

            PHY_ofdm_mod(&gNB->common_vars.txdataF[aa][frame_parms->ofdm_symbol_size],
                         (int*)&txdata[aa][frame_parms->get_samples_slot_timestamp(slot,frame_parms,0)+frame_parms->nb_prefix_samples0+frame_parms->ofdm_symbol_size],
                         frame_parms->ofdm_symbol_size,
                         13,
                         frame_parms->nb_prefix_samples,
                         CYCLIC_PREFIX);

            apply_nr_rotation(frame_parms,
                              (int16_t*)&txdata[aa][frame_parms->get_samples_slot_timestamp(slot,frame_parms,0)+frame_parms->nb_prefix_samples0+frame_parms->ofdm_symbol_size],
                              slot,
                              1,
                              13,
                              frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples);
          }
        }
      }
    }
    LOG_M("txsigF0.m","txsF0", gNB->common_vars.txdataF[0],frame_length_complex_samples_no_prefix,1,1);
    if (gNB->frame_parms.nb_antennas_tx>1)
      LOG_M("txsigF1.m","txsF1", gNB->common_vars.txdataF[1],frame_length_complex_samples_no_prefix,1,1);

  } else {
    printf("Reading %d samples from file to antenna buffer %d\n",frame_length_complex_samples,0);
    UE->UE_fo_compensation = 1; // perform fo compensation when samples from file are used
    if (fread(txdata[0],
        sizeof(int32_t),
        frame_length_complex_samples,
        input_fd) != frame_length_complex_samples) {
      printf("error reading from file\n");
      //exit(-1);
    }
  }

  LOG_M("txsig0.m","txs0", txdata[0],frame_length_complex_samples,1,1);
  if (gNB->frame_parms.nb_antennas_tx>1)
    LOG_M("txsig1.m","txs1", txdata[1],frame_length_complex_samples,1,1);

  if (output_fd) 
    fwrite(txdata[0],sizeof(int32_t),frame_length_complex_samples,output_fd);

  /*int txlev = signal_energy(&txdata[0][5*frame_parms->ofdm_symbol_size + 4*frame_parms->nb_prefix_samples + frame_parms->nb_prefix_samples0],
		  	  	  	  	    frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples);
  printf("txlev %d (%f)\n",txlev,10*log10(txlev));*/


  for (i=0; i<frame_length_complex_samples; i++) {
    for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
      r_re[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)]);
      r_im[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)+1]);
    }
  }
  
  for (SNR=snr0; SNR<snr1; SNR+=.2) {

    n_errors = 0;
    n_errors_payload = 0;

    for (trial=0; trial<n_trials; trial++) {
      // multipath channel
      //multipath_channel(gNB2UE,s_re,s_im,r_re,r_im,frame_length_complex_samples,0);
      
      //AWGN
      sigma2_dB = 20*log10((double)AMP/4)-SNR;
      sigma2 = pow(10,sigma2_dB/10);
      //printf("sigma2 %f (%f dB), tx_lev %f (%f dB)\n",sigma2,sigma2_dB,txlev,10*log10((double)txlev));

      if(eps!=0.0)
        rf_rx(r_re,  // real part of txdata
           r_im,  // imag part of txdata
           NULL,  // interference real part
           NULL, // interference imag part
           0,  // interference power
           frame_parms->nb_antennas_rx,  // number of rx antennas
           frame_length_complex_samples,  // number of samples in frame
           1.0e9/fs,   //sampling time (ns)
           cfo,	// frequency offset in Hz
           0.0, // drift (not implemented)
           0.0, // noise figure (not implemented)
           0.0, // rx gain in dB ?
           200, // 3rd order non-linearity in dB ?
           &ip, // initial phase
           30.0e3,  // phase noise cutoff in kHz
           -500.0, // phase noise amplitude in dBc
           0.0,  // IQ imbalance (dB),
	   0.0); // IQ phase imbalance (rad)

   
      for (i=0; i<frame_length_complex_samples; i++) {
	for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
	  
	  ((short*) UE->common_vars.rxdata[aa])[2*i]   = (short) ((r_re[aa][i] + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
	  ((short*) UE->common_vars.rxdata[aa])[2*i+1] = (short) ((r_im[aa][i] + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
	}
      }

      if (n_trials==1) {
	LOG_M("rxsig0.m","rxs0", UE->common_vars.rxdata[0],frame_parms->samples_per_frame,1,1);
	if (gNB->frame_parms.nb_antennas_tx>1)
	  LOG_M("rxsig1.m","rxs1", UE->common_vars.rxdata[1],frame_parms->samples_per_frame,1,1);
      }
      if (UE->is_synchronized == 0) {
	UE_nr_rxtx_proc_t proc={0};
	ret = nr_initial_sync(&proc, UE, 1, 0, 0);
	printf("nr_initial_sync1 returns %d\n",ret);
	if (ret<0) n_errors++;
      }
      else {
	UE_nr_rxtx_proc_t proc={0};
	UE->rx_offset=0;
	uint8_t ssb_index = 0;
        while (!((SSB_positions >> ssb_index) & 0x01)) ssb_index++;  // to select the first transmitted ssb
	UE->symbol_offset = nr_get_ssb_start_symbol(frame_parms,ssb_index);

        int ssb_slot = (UE->symbol_offset/14)+(n_hf*(frame_parms->slots_per_frame>>1));
	for (int i=UE->symbol_offset+1; i<UE->symbol_offset+4; i++) {
          nr_slot_fep(UE,
                      &proc,
                      i%frame_parms->symbols_per_slot,
                      ssb_slot);

          nr_pbch_channel_estimation(UE,&proc,0,ssb_slot,i%frame_parms->symbols_per_slot,i-(UE->symbol_offset+1),ssb_index%8,n_hf);

        }

        ret = nr_rx_pbch(UE,
                         &proc,
                         UE->pbch_vars[0],
                         frame_parms,
                         0,
                         ssb_index%8,
                         SISO);

	if (ret==0) {
	  //UE->rx_ind.rx_indication_body->mib_pdu.ssb_index;  //not yet detected automatically
	  //UE->rx_ind.rx_indication_body->mib_pdu.ssb_length; //Lmax, not yet detected automatically
	  uint8_t gNB_xtra_byte=0;
	  for (int i=0; i<8; i++)
	    gNB_xtra_byte |= ((gNB->pbch.pbch_a>>(31-i))&1)<<(7-i);
 
	  payload_ret = (UE->pbch_vars[0]->xtra_byte == gNB_xtra_byte);
	  for (i=0;i<3;i++){
	    payload_ret += (UE->pbch_vars[0]->decoded_output[i] == ((gNB->ssb[ssb_index].ssb_pdu.ssb_pdu_rel15.bchPayload>>(8*i)) & 0xff));
	  } 
	  //printf("xtra byte gNB: 0x%02x UE: 0x%02x\n",gNB_xtra_byte, UE->pbch_vars[0]->xtra_byte);
	  //printf("ret %d\n", payload_ret);
	  if (payload_ret!=4) 
	    n_errors_payload++;
	}

	if (ret!=0) n_errors++;
      }
    } //noise trials
    printf("SNR %f: trials %d, n_errors_crc = %d, n_errors_payload %d\n", SNR,n_trials,n_errors,n_errors_payload);

    if (((float)n_errors/(float)n_trials <= target_error_rate) && (n_errors_payload==0)) {
      printf("PBCH test OK\n");
      break;
    }
      
    if (n_trials==1)
      break;

  } // NSR

  for (i=0; i<2; i++) {
    free(s_re[i]);
    free(s_im[i]);
    free(r_re[i]);
    free(r_im[i]);
    free(txdata[i]);
  }

  free(s_re);
  free(s_im);
  free(r_re);
  free(r_im);
  free(txdata);

  if (output_fd)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  return(n_errors);

}
