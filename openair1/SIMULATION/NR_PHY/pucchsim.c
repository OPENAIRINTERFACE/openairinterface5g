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
#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/NR_UE_TRANSPORT/pucch_nr.h"
#include "SCHED_NR/sched_nr.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
#include "openair1/SIMULATION/RF/rf.h"
#include "openair1/SIMULATION/NR_PHY/nr_unitary_defs.h"
#include "openair1/SIMULATION/NR_PHY/nr_dummy_functions.c"

THREAD_STRUCT thread_struct;
PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;
openair0_config_t openair0_cfg[MAX_CARDS];
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];
uint64_t downlink_frequency[MAX_NUM_CCs][4];

double cpuf;
//uint8_t nfapi_mode = 0;
uint16_t NB_UE_INST = 1;
uint8_t const nr_rv_round_map[4] = {0, 2, 1, 3};
uint8_t const nr_rv_round_map_ue[4] = {0, 2, 1, 3};

// needed for some functions
PHY_VARS_NR_UE * PHY_vars_UE_g[1][1]={{NULL}};

void init_downlink_harq_status(NR_DL_UE_HARQ_t *dl_harq) {}

int main(int argc, char **argv)
{
  char c;
  int i;//,l;
  double sigma2, sigma2_dB=10,SNR,snr0=-2.0,snr1=2.0;
  double cfo=0;
  uint8_t snr1set=0;
  int **txdataF,**rxdataF;
  double **s_re,**s_im,**r_re,**r_im;
  //int sync_pos, sync_pos_slot;
  //FILE *rx_frame_file;
  FILE *output_fd = NULL;
  //uint8_t write_output_file=0;
  //int result;
  //int freq_offset;
  //int subframe_offset;
  //char fname[40], vname[40];
  int trial,n_trials=100,n_errors=0,ack_nack_errors=0,sr_errors=0;
  uint8_t transmission_mode = 1,n_tx=1,n_rx=1;
  uint16_t Nid_cell=0;
  uint64_t SSB_positions=0x01;
  channel_desc_t *UE2gNB;
  int format=0;
  //uint8_t extended_prefix_flag=0;
  FILE *input_fd=NULL;
  //uint8_t nacktoack_flag=0;
  int16_t amp=0x7FFF;
  int nr_slot_tx=0;
  int nr_frame_tx=0;
  uint64_t actual_payload=0,payload_received;
  int nr_bit=1; // maximum value possible is 2
  uint8_t m0=0;// higher layer paramater initial cyclic shift
  uint8_t nrofSymbols=1; //number of OFDM symbols can be 1-2 for format 1
  uint8_t startingSymbolIndex=0; // resource allocated see 9.2.1, 38.213 for more info.should be actually present in the resource set provided
  uint16_t startingPRB=0,startingPRB_intraSlotHopping=0; //PRB number not sure see 9.2.1, 38.213 for more info. Should be actually present in the resource set provided
  uint16_t nrofPRB=2;
  uint8_t timeDomainOCC=0;
  SCM_t channel_model=AWGN;//Rayleigh1_anticorr;

  double DS_TDL = .03;
 
  double delay_us = 0;
  int N_RB_DL=273,mu=1;
  float target_error_rate=0.001;
  int frame_length_complex_samples;
  //int frame_length_complex_samples_no_prefix;
  NR_DL_FRAME_PARMS *frame_parms;
  //unsigned char frame_type = 0;
  int loglvl=OAILOG_WARNING;
  int sr_flag = 0;
  int pucch_DTX_thres = 50;
  cpuf = get_cpu_freq_GHz();

  if ( load_configmodule(argc,argv,CONFIG_ENABLECMDLINEONLY) == 0) {
    exit_fun("[NR_PUCCHSIM] Error, configuration module init failed\n");
  }

  randominit(0);
  logInit();

  while ((c = getopt (argc, argv, "f:hA:f:g:i:I:P:B:b:t:T:m:n:r:o:s:S:x:y:z:N:F:GR:IL:q:cd:")) != -1) {
    switch (c) {
    case 'f':
      //write_output_file=1;
      output_fd = fopen(optarg,"w");

      if (output_fd==NULL) {
        printf("Error opening %s\n",optarg);
        exit(-1);
      }
      break;



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

    case 'n':
      n_trials = atoi(optarg);
      break;

    case 'o':
      cfo = atof(optarg);
      printf("Setting CFO to %f Hz\n",cfo);
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

      
    case 't':
      pucch_DTX_thres= atoi(optarg);
      break;
      /*
    case 'p':
      extended_prefix_flag=1;
      break;

    case 'd':
      frame_type = 1;
      break;

    case 'r':
      ricean_factor = pow(10,-.1*atof(optarg));
      if (ricean_factor>1) {
        printf("Ricean factor must be between 0 and 1\n");
        exit(-1);
      }
      break;
      */
    case 'd':
       delay_us=atof(optarg);
       break;
    case 'x':
      transmission_mode=atoi(optarg);

      if ((transmission_mode!=1) &&
          (transmission_mode!=2) &&
          (transmission_mode!=6)) {
        printf("Unsupported transmission mode %d\n",transmission_mode);
        exit(-1);
      }
      break;

    case 'y':
      n_tx=atoi(optarg);

      if ((n_tx==0) || (n_tx>2)) {
        printf("Unsupported number of tx antennas %d\n",n_tx);
        exit(-1);
      }
      break;

    case 'z':
      n_rx=atoi(optarg);

      if ((n_rx==0) || (n_rx>8)) {
        printf("Unsupported number of rx antennas %d\n",n_rx);
        exit(-1);
      }
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
        printf("Problem with filename %s\n",optarg);
        exit(-1);
      }
      break;

    case 'L':
      loglvl = atoi(optarg);
      break;
    case 'i':
      nrofSymbols=(uint8_t)atoi(optarg);
      break;
    case 'I':
      startingSymbolIndex=(uint8_t)atoi(optarg);
      break;
    case 'r':
      startingPRB=atoi(optarg);
      break;
    case 'q':
      nrofPRB=atoi(optarg);
      break;
    case 'P':
      format=atoi(optarg);
      break;
    case 'm':
      m0=atoi(optarg);
      break;
    case 'b':
      nr_bit=atoi(optarg);
      break;
    case 'c':
      sr_flag=1;
      break;
    case 'B':
      actual_payload=atoi(optarg);
      break;
    case 'T':
      //nacktoack_flag=(uint8_t)atoi(optarg);
      target_error_rate=0.001;
      break;
    default:
    case 'h':
      printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n", argv[0]);
      printf("-h This message\n");
      printf("-p Use extended prefix mode\n");
      printf("-d Use TDD\n");
      printf("-n Number of frames to simulate\n");
      printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
      printf("-S Ending SNR, runs from SNR0 to SNR1\n");
      printf("-t Delay spread for multipath channel\n");
      printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
      printf("-x Transmission mode (1,2,6 for the moment)\n");
      printf("-y Number of TX antennas used in eNB\n");
      printf("-z Number of RX antennas used in UE\n");
      printf("-i Relative strength of first intefering eNB (in dB) - cell_id mod 3 = 1\n");
      printf("-j Relative strength of second intefering eNB (in dB) - cell_id mod 3 = 2\n");
      printf("-o Carrier frequency offset in Hz\n");
      printf("-N Nid_cell\n");
      printf("-R N_RB_DL\n");
      printf("-O oversampling factor (1,2,4,8,16)\n");
      printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
      //printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
      printf("-f Output filename (.txt format) for Pe/SNR results\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      printf("-i Enter number of ofdm symbols for pucch\n");
      printf("-I Starting symbol index for pucch\n");
      printf("-r PUCCH starting PRB\n");
      printf("-q PUCCH number of PRB\n");
      printf("-P Enter the format of PUCCH\n");
      printf("-b number of HARQ bits (1-2)\n");
      printf("-B payload to be transmitted on PUCCH\n");
      printf("-m initial cyclic shift m0\n");
      printf("-T to check nacktoack miss for format 1");
      exit (-1);
      break;
    }
  }

  double phase = (1<<mu)*30e-3*delay_us;

  set_glog(loglvl);

  if (snr1set==0) snr1 = snr0+10;

  printf("Initializing gNodeB for mu %d, N_RB_DL %d, n_rx %d\n",mu,N_RB_DL,n_rx);

  if((format!=0) && (format!=1) && (format!=2)){
    printf("PUCCH format %d not supported\n",format);
    exit(0); 
  }

  AssertFatal(((format < 2)&&(nr_bit<3)&&(actual_payload<5)) ||
	      ((format == 2)&&(nr_bit>2)&&(nr_bit<65)),"illegal combination format %d, nr_bit %d\n",
	      format,nr_bit);
  int do_DTX=0;
  if ((format < 2) && (actual_payload == 4)) do_DTX=1;

  actual_payload &= ((1<<nr_bit)-1);

  printf("Transmitted payload is %ld, do_DTX = %d\n",actual_payload,do_DTX);

  RC.gNB = (PHY_VARS_gNB**) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0] = calloc(1,sizeof(PHY_VARS_gNB));
  gNB = RC.gNB[0];
  gNB->pucch0_thres = pucch_DTX_thres;
  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
  frame_parms->nb_antennas_tx = n_tx;
  frame_parms->nb_antennas_rx = n_rx;
  frame_parms->N_RB_DL = N_RB_DL;
  frame_parms->N_RB_UL = N_RB_DL;
  frame_parms->Nid_cell = Nid_cell;

  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  cfg->carrier_config.num_tx_ant.value = n_tx;
  cfg->carrier_config.num_rx_ant.value = n_rx;
  nr_phy_config_request_sim(gNB,N_RB_DL,N_RB_DL,mu,Nid_cell,SSB_positions);
  phy_init_nr_gNB(gNB,0,0);

  double fs,bw,scs,eps;

  if (mu == 1 && N_RB_DL == 217) { 
    fs = 122.88e6;
    bw = 80e6;
    scs = 30000;
  }					       
  else if (mu == 1 && N_RB_DL == 245) {
    fs = 122.88e6;
    bw = 90e6;
    scs = 30000;
  }
  else if (mu == 1 && N_RB_DL == 273) {
    fs = 122.88e6;
    bw = 100e6;
    scs = 30000;
  }
  else if (mu == 1 && N_RB_DL == 106) { 
    fs = 61.44e6;
    bw = 40e6;
    scs = 30000;
  }
  else AssertFatal(1==0,"Unsupported numerology for mu %d, N_RB %d\n",mu, N_RB_DL);

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

  UE2gNB = new_channel_desc_scm(n_tx, n_rx, channel_model, fs, bw, DS_TDL,0, 0, 0, 0);

  if (UE2gNB==NULL) {
    printf("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  frame_length_complex_samples = frame_parms->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
  //frame_length_complex_samples_no_prefix = frame_parms->samples_per_subframe_wCP;

  s_re = malloc(n_tx*sizeof(double*));
  s_im = malloc(n_tx*sizeof(double*));
  r_re = malloc(n_rx*sizeof(double*));
  r_im = malloc(n_rx*sizeof(double*));
  txdataF = malloc(n_tx*sizeof(int*));
  rxdataF = malloc(n_rx*sizeof(int*));
  gNB->common_vars.rxdataF=rxdataF;
  memcpy((void*)&gNB->frame_parms,(void*)frame_parms,sizeof(frame_parms));
  for (int aatx=0; aatx<n_tx; aatx++) {
    s_re[aatx] = calloc(1,frame_length_complex_samples*sizeof(double));
    s_im[aatx] = calloc(1,frame_length_complex_samples*sizeof(double));
    printf("Allocating %d samples for txdataF\n",frame_parms->symbols_per_slot*frame_parms->ofdm_symbol_size);
    txdataF[aatx] = memalign(32,14*frame_parms->ofdm_symbol_size*sizeof(int));
    bzero(txdataF[aatx],14*frame_parms->ofdm_symbol_size*sizeof(int));
  }

  for (int aarx=0; aarx<n_rx; aarx++) {
    r_re[aarx] = calloc(1,frame_length_complex_samples*sizeof(double));
    r_im[aarx] = calloc(1,frame_length_complex_samples*sizeof(double));
    printf("Allocating %d samples for rxdataF\n",frame_parms->symbols_per_slot*frame_parms->ofdm_symbol_size);
    rxdataF[aarx] = memalign(32,14*frame_parms->ofdm_symbol_size*sizeof(int));
    bzero(rxdataF[aarx],14*frame_parms->ofdm_symbol_size*sizeof(int));
  }

  uint8_t mcs=0;
  int shift = 0;
  if(format==0){
    if (sr_flag)
      shift = 1<<nr_bit;
    if (nr_bit ==0) 
      mcs=table1_mcs[0];
    else if(nr_bit==1)
      mcs=table1_mcs[actual_payload+shift];
    else if(nr_bit==2)
      mcs=table2_mcs[actual_payload+shift];
    else AssertFatal(1==0,"Either nr_bit %d or sr_flag %d must be non-zero\n", nr_bit, sr_flag);
  }
  else if (format == 2 && nr_bit > 11) gNB->uci_polarParams = nr_polar_params(2, nr_bit, nrofPRB, 1, NULL);

  startingPRB_intraSlotHopping = N_RB_DL-1;
  uint32_t hopping_id = Nid_cell;
  uint32_t dmrs_scrambling_id = 0;
  uint32_t data_scrambling_id = 0;

  //configure UE
  UE = calloc(1,sizeof(PHY_VARS_NR_UE));
  memcpy(&UE->frame_parms,frame_parms,sizeof(NR_DL_FRAME_PARMS));
  UE->frame_parms.nb_antennas_rx=1;
  UE->perfect_ce = 0;

  if(eps!=0.0)
    UE->UE_fo_compensation = 1; // if a frequency offset is set then perform fo estimation and compensation

  if (init_nr_ue_signal(UE, 1, 0) != 0)
  {
    printf("Error at UE NR initialisation\n");
    exit(-1);
  }

  fapi_nr_ul_config_pucch_pdu pucch_tx_pdu;
  if (format==0) {
    pucch_tx_pdu.format_type = 0;
    pucch_tx_pdu.nr_of_symbols = nrofSymbols;
    pucch_tx_pdu.start_symbol_index = startingSymbolIndex;
    pucch_tx_pdu.bwp_start = 0;
    pucch_tx_pdu.prb_start = startingPRB;
    pucch_tx_pdu.hopping_id = hopping_id;
    pucch_tx_pdu.group_hop_flag = 0;
    pucch_tx_pdu.sequence_hop_flag = 0;
    pucch_tx_pdu.freq_hop_flag = 0;
    pucch_tx_pdu.mcs = mcs;
    pucch_tx_pdu.initial_cyclic_shift = 0;
    pucch_tx_pdu.second_hop_prb = startingPRB_intraSlotHopping;
  }
  if (format==2) {
    pucch_tx_pdu.format_type = 2;
    pucch_tx_pdu.rnti = 0x1234;
    pucch_tx_pdu.n_bit = nr_bit;
    pucch_tx_pdu.payload = actual_payload;
    pucch_tx_pdu.nr_of_symbols = nrofSymbols;
    pucch_tx_pdu.start_symbol_index = startingSymbolIndex;
    pucch_tx_pdu.bwp_start = 0;
    pucch_tx_pdu.prb_start = startingPRB;
    pucch_tx_pdu.prb_size = nrofPRB;
    pucch_tx_pdu.hopping_id = hopping_id;
    pucch_tx_pdu.group_hop_flag = 0;
    pucch_tx_pdu.sequence_hop_flag = 0;
    pucch_tx_pdu.freq_hop_flag = 0;
    pucch_tx_pdu.dmrs_scrambling_id = dmrs_scrambling_id;
    pucch_tx_pdu.data_scrambling_id = data_scrambling_id;
    pucch_tx_pdu.second_hop_prb = startingPRB_intraSlotHopping;
  }

  pucch_GroupHopping_t PUCCH_GroupHopping = pucch_tx_pdu.group_hop_flag + (pucch_tx_pdu.sequence_hop_flag<<1);

  for(SNR=snr0;SNR<=snr1;SNR+=1){
    ack_nack_errors=0;
    sr_errors=0;
    n_errors = 0;
    for (trial=0; trial<n_trials; trial++) {
      for (int aatx=0;aatx<1;aatx++)
        bzero(txdataF[aatx],frame_parms->ofdm_symbol_size*sizeof(int));
      if(format==0 && do_DTX==0){
        nr_generate_pucch0(UE, txdataF, frame_parms, amp, nr_slot_tx, &pucch_tx_pdu);
      } else if (format == 1 && do_DTX==0){
        nr_generate_pucch1(UE, txdataF, frame_parms, amp, nr_slot_tx, &pucch_tx_pdu);
      } else if (do_DTX == 0){
        nr_generate_pucch2(UE, txdataF, frame_parms, amp, nr_slot_tx, &pucch_tx_pdu);
      }

      // SNR Computation
      // standard says: SNR = S / N, where S is the total signal energy, N is the noise energy in the transmission bandwidth (i.e. N_RB_DL resource blocks)
      // txlev = S.
      int txlev = signal_energy(&txdataF[0][startingSymbolIndex*frame_parms->ofdm_symbol_size], frame_parms->ofdm_symbol_size);

      // sigma2 is variance per dimension, so N/(N_RB*12)
      // so, sigma2 = N/(N_RB_DL*12) => (S/SNR)/(N_RB*12)
      int N_RB = (format == 0 || format == 1) ? 1 : nrofPRB;
      sigma2_dB = 10*log10(txlev*(N_RB_DL/N_RB))-SNR;
      sigma2 = pow(10.0,sigma2_dB/10.0);

      if (n_trials==1) printf("txlev %d (%f dB), offset %d, sigma2 %f ( %f dB)\n",txlev,10*log10(txlev),startingSymbolIndex*frame_parms->ofdm_symbol_size,sigma2,sigma2_dB);

      int i0;
      double txr,txi,rxr,rxi,nr,ni;
      for (int symb=0; symb<gNB->frame_parms.symbols_per_slot;symb++) {
        if (symb<startingSymbolIndex || symb >= startingSymbolIndex+nrofSymbols) {
          i0 = symb*gNB->frame_parms.ofdm_symbol_size;
          for (int re=0;re<N_RB_DL*12;re++) {
            i=i0+((gNB->frame_parms.first_carrier_offset + re)%gNB->frame_parms.ofdm_symbol_size);
            for (int aarx=0;aarx<n_rx;aarx++) {
              nr = sqrt(sigma2/2)*gaussdouble(0.0,1.0);
              ni = sqrt(sigma2/2)*gaussdouble(0.0,1.0);
              ((int16_t*)rxdataF[aarx])[i<<1] = (int16_t)(100.0*(nr)/sqrt((double)txlev));
              ((int16_t*)rxdataF[aarx])[1+(i<<1)] = (int16_t)(100.0*(ni)/sqrt((double)txlev));
            }
          }
        }
      }

      random_channel(UE2gNB,0);
      freq_channel(UE2gNB,N_RB_DL,2*N_RB_DL+1,scs/1000);
      struct complexd phasor;
      double rxr_tmp;
      for (int symb=0; symb<nrofSymbols; symb++) {
        i0 = (startingSymbolIndex + symb)*gNB->frame_parms.ofdm_symbol_size;
        for (int re=0;re<N_RB_DL*12;re++) {
          i=i0+((gNB->frame_parms.first_carrier_offset + re)%gNB->frame_parms.ofdm_symbol_size);
          phasor.r = cos(2*M_PI*phase*re);
          phasor.i = sin(2*M_PI*phase*re);
          for (int aarx=0;aarx<n_rx;aarx++) {
            txr = (double)(((int16_t *)txdataF[0])[(i<<1)]);
            txi = (double)(((int16_t *)txdataF[0])[1+(i<<1)]);
            rxr = txr*UE2gNB->chF[aarx][re].r - txi*UE2gNB->chF[aarx][re].i;
            rxi = txr*UE2gNB->chF[aarx][re].i + txi*UE2gNB->chF[aarx][re].r;
            rxr_tmp = rxr*phasor.r - rxi*phasor.i;
            rxi     = rxr*phasor.i + rxi*phasor.r;
            rxr = rxr_tmp;
            nr = sqrt(sigma2/2)*gaussdouble(0.0,1.0);
            ni = sqrt(sigma2/2)*gaussdouble(0.0,1.0);
            ((int16_t*)rxdataF[aarx])[i<<1] = (int16_t)(100.0*(rxr + nr)/sqrt((double)txlev));
            ((int16_t*)rxdataF[aarx])[1+(i<<1)]=(int16_t)(100.0*(rxi + ni)/sqrt((double)txlev));

            if (n_trials==1 && abs(txr) > 0) printf("symb %d, re %d , aarx %d : txr %f, txi %f, chr %f, chi %f, nr %f, ni %f, rxr %f, rxi %f => %d,%d\n",
                                                    symb, re, aarx, txr,txi,
                                                    UE2gNB->chF[aarx][re].r,UE2gNB->chF[aarx][re].i,
                                                    nr,ni, rxr,rxi,
                                                    ((int16_t*)rxdataF[aarx])[i<<1],((int16_t*)rxdataF[aarx])[1+(i<<1)]);
          }
        }
      }

      int rxlev=0;
      for (int aarx=0;aarx<n_rx;aarx++) rxlev += signal_energy(&rxdataF[aarx][startingSymbolIndex*frame_parms->ofdm_symbol_size],
                                                           frame_parms->ofdm_symbol_size);

      int rxlev_pucch=0;

      for (int aarx=0;aarx<n_rx;aarx++) rxlev_pucch += signal_energy(&rxdataF[aarx][startingSymbolIndex*frame_parms->ofdm_symbol_size],
                                                           12);

      // set UL mask for pucch allocation
      for (int s=0;s<frame_parms->symbols_per_slot;s++){
        if (s>=startingSymbolIndex && s<(startingSymbolIndex+nrofSymbols))
          for (int rb=0; rb<N_RB; rb++) {
            int rb2 = rb+startingPRB;
            gNB->rb_mask_ul[s][rb2>>5] |= (1<<(rb2&31));
          }
      }

      // noise measurement (all PRBs)
      gNB_I0_measurements(gNB, nr_slot_tx, 0, gNB->frame_parms.symbols_per_slot);

      if (n_trials==1) printf("noise rxlev %d (%d dB), rxlev pucch %d dB sigma2 %f dB, SNR %f, TX %f, I0 (pucch) %d, I0 (avg) %d\n",rxlev,dB_fixed(rxlev),dB_fixed(rxlev_pucch),sigma2_dB,SNR,10*log10((double)txlev*UE->frame_parms.ofdm_symbol_size/12),gNB->measurements.n0_subband_power_tot_dB[startingPRB],gNB->measurements.n0_subband_power_avg_dB);
      if(format==0){
        nfapi_nr_uci_pucch_pdu_format_0_1_t uci_pdu;
        nfapi_nr_pucch_pdu_t pucch_pdu;
        gNB->uci_stats[0].rnti          = 0x1234;
        pucch_pdu.rnti                  = 0x1234;
        pucch_pdu.subcarrier_spacing    = 1;
        pucch_pdu.group_hop_flag        = PUCCH_GroupHopping&1;
        pucch_pdu.sequence_hop_flag     = (PUCCH_GroupHopping>>1)&1;
        pucch_pdu.bit_len_harq          = nr_bit;
        pucch_pdu.bit_len_csi_part1     = 0;
        pucch_pdu.bit_len_csi_part2     = 0;
        pucch_pdu.sr_flag               = sr_flag;
        pucch_pdu.nr_of_symbols         = nrofSymbols;
        pucch_pdu.hopping_id            = hopping_id;
        pucch_pdu.initial_cyclic_shift  = 0;
        pucch_pdu.start_symbol_index    = startingSymbolIndex;
        pucch_pdu.prb_start             = startingPRB;
        pucch_pdu.bwp_start             = 0;
        pucch_pdu.bwp_size              = N_RB_DL;
        if (nrofSymbols>1) {
          pucch_pdu.freq_hop_flag       = 1;
          pucch_pdu.second_hop_prb      = N_RB_DL-1;
        }
        else pucch_pdu.freq_hop_flag = 0;

        nr_decode_pucch0(gNB, nr_frame_tx, nr_slot_tx,&uci_pdu,&pucch_pdu);
        if(sr_flag==1){
          if (uci_pdu.sr->sr_indication == 0 || uci_pdu.sr->sr_confidence_level == 1)
            sr_errors+=1;
        }
        // harq value 0 -> pass
        // confidence value 0 -> good confidence
        const int harq_value0 = uci_pdu.harq->harq_list[0].harq_value;
        const int harq_value1 = uci_pdu.harq->harq_list[1].harq_value;
        const int confidence_lvl = uci_pdu.harq->harq_confidence_level;
        if(nr_bit>0){
          if (nr_bit==1 && do_DTX == 0)
            ack_nack_errors+=(actual_payload^(!harq_value0));
          else if (do_DTX == 0)
            ack_nack_errors+=(((actual_payload&1)^(!harq_value0))+((actual_payload>>1)^(!harq_value1)));
          else if ((!confidence_lvl && !harq_value0) ||
                   (!confidence_lvl && nr_bit == 2 && !harq_value1))
            ack_nack_errors++;
          free(uci_pdu.harq->harq_list);
        }
      }
      else if (format==1) {
        nr_decode_pucch1(rxdataF,PUCCH_GroupHopping,hopping_id,
                         &(payload_received),frame_parms,amp,nr_slot_tx,
                         m0,nrofSymbols,startingSymbolIndex,startingPRB,
                         startingPRB_intraSlotHopping,timeDomainOCC,nr_bit);
        if(nr_bit==1)
          ack_nack_errors+=((actual_payload^payload_received)&1);
        else
          ack_nack_errors+=((actual_payload^payload_received)&1) + (((actual_payload^payload_received)&2)>>1);
      }
      else if (format==2) {
        nfapi_nr_uci_pucch_pdu_format_2_3_4_t uci_pdu;
        nfapi_nr_pucch_pdu_t pucch_pdu;
        pucch_pdu.rnti = 0x1234;
        pucch_pdu.subcarrier_spacing    = 1;
        pucch_pdu.group_hop_flag        = PUCCH_GroupHopping&1;
        pucch_pdu.sequence_hop_flag     = (PUCCH_GroupHopping>>1)&1;
        pucch_pdu.bit_len_csi_part1     = nr_bit;
        pucch_pdu.bit_len_harq          = 0;
        pucch_pdu.bit_len_csi_part2     = 0;
        pucch_pdu.sr_flag               = 0;
        pucch_pdu.nr_of_symbols         = nrofSymbols;
        pucch_pdu.hopping_id            = hopping_id;
        pucch_pdu.initial_cyclic_shift  = 0;
        pucch_pdu.start_symbol_index    = startingSymbolIndex;
        pucch_pdu.prb_size              = nrofPRB;
        pucch_pdu.prb_start             = startingPRB;
        pucch_pdu.dmrs_scrambling_id    = dmrs_scrambling_id;
        pucch_pdu.data_scrambling_id    = data_scrambling_id;
        if (nrofSymbols>1) {
          pucch_pdu.freq_hop_flag       = 1;
          pucch_pdu.second_hop_prb      = N_RB_DL-1;
        }
        else pucch_pdu.freq_hop_flag = 0;
        nr_decode_pucch2(gNB,nr_slot_tx,&uci_pdu,&pucch_pdu);
        int csi_part1_bytes=pucch_pdu.bit_len_csi_part1>>3;
        if ((pucch_pdu.bit_len_csi_part1&7) > 0) csi_part1_bytes++;
        for (int i=0;i<csi_part1_bytes;i++) {
          if (uci_pdu.csi_part1.csi_part1_payload[i] != ((uint8_t*)&actual_payload)[i]) {
            ack_nack_errors++;
            break;
          }
        }
        free(uci_pdu.csi_part1.csi_part1_payload);

      }
      n_errors=((actual_payload^payload_received)&1)+(((actual_payload^payload_received)&2)>>1)+(((actual_payload^payload_received)&4)>>2)+n_errors;
    }
    if (sr_flag == 1)
      printf("SR: SNR=%f, n_trials=%d, n_bit_errors=%d\n",SNR,n_trials,sr_errors);
    if(nr_bit > 0)
      printf("ACK/NACK: SNR=%f, n_trials=%d, n_bit_errors=%d\n",SNR,n_trials,ack_nack_errors);
    if((float)(ack_nack_errors+sr_errors)/(float)(n_trials)<=target_error_rate){
      printf("PUCCH test OK\n");
      break;
    }
  }

  for (int aatx=0; aatx<n_tx; aatx++) {
    free(s_re[aatx]);
    free(s_im[aatx]);
    free(txdataF[aatx]);
  }
  for (int aarx=0; aarx<n_rx; aarx++) {
    free(r_re[aarx]);
    free(r_im[aarx]);
    free(rxdataF[aarx]);
  }
  free(s_re);
  free(s_im);
  free(r_re);
  free(r_im);
  free(txdataF);
  free(rxdataF);

  if (output_fd) fclose(output_fd);
  if (input_fd)  fclose(input_fd);

  return(n_errors);
}
