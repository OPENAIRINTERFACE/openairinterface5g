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

#include "common/config/config_userapi.h"
#include "common/utils/LOG/log.h"
#include "common/ran_context.h" 

#include "SIMULATION/TOOLS/sim.h"
#include "SIMULATION/RF/rf.h"
#include "PHY/types.h"
#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_UE.h"
#include "SCHED_NR/sched_nr.h"
#include "SCHED_NR_UE/phy_frame_config_nr.h"
#include "PHY/phy_vars_nr_ue.h"

#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_REFSIG/nr_mod_table.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_TRANSPORT/nr_transport.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "OCG_vars.h"

#include <pthread.h>

PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;
RU_t *ru;

double cpuf;

extern uint16_t prach_root_sequence_map0_3[838];

void dump_nr_prach_config(NR_DL_FRAME_PARMS *frame_parms,uint8_t subframe);

uint16_t NB_UE_INST=1;
volatile int oai_exit=0;

void exit_function(const char* file, const char* function, const int line,const char *s) { 
   const char * msg= s==NULL ? "no comment": s;
   printf("Exiting at: %s:%d %s(), %s\n", file, line, function, msg); 
   exit(-1); 
}


int8_t nr_ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP, uint8_t eNB_id, uint16_t rnti, sub_frame_t subframe) {
  AssertFatal(1==0,"Shouldn't be here ...\n");
  return 0;
}

uint8_t nr_ue_get_sdu(module_id_t module_idP, int CC_id, frame_t frameP,
           sub_frame_t subframe, uint8_t eNB_index,
           uint8_t *ulsch_buffer, uint16_t buflen, uint8_t *access_mode) {return(0);}

int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind) {return(0);}

openair0_config_t openair0_cfg[MAX_CARDS];
uint8_t nfapi_mode=0;
NR_IF_Module_t *NR_IF_Module_init(int Mod_id){return(NULL);}
int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req) { return(0); }

uint64_t get_softmodem_optmask(void) {
  return 0;
}

int main(int argc, char **argv)
{

  char c;

  int i,aa,aarx;
  double sigma2, sigma2_dB=0,SNR,snr0=-2.0,snr1=0.0,ue_speed0=0.0,ue_speed1=0.0;
  uint8_t snr1set=0;
  uint8_t ue_speed1set=0;
  int **txdata;
  double **s_re,**s_im,**r_re,**r_im;
  double iqim=0.0;
  int trial; //, ntrials=1;
  uint8_t transmission_mode = 1,n_tx=1,n_rx=1;
  uint16_t Nid_cell=0;

  uint8_t awgn_flag=0;
  uint8_t hs_flag=0;
  int n_frames=1;
  channel_desc_t *UE2gNB;
  uint32_t tx_lev=0; //,tx_lev_dB;
  //  int8_t interf1=-19,interf2=-19;
  NR_DL_FRAME_PARMS *frame_parms;

  SCM_t channel_model=Rayleigh1;

  //  uint8_t abstraction_flag=0,calibration_flag=0;
  //  double prach_sinr;
  int N_RB_UL=273;
  uint32_t prach_errors=0;
  uint8_t subframe=9;
  uint16_t preamble_energy_list[64],preamble_tx=50,preamble_delay_list[64];
  PRACH_RESOURCES_t prach_resources;
  //uint8_t prach_fmt;
  //int N_ZC;
  int delay = 0;
  double delay_avg=0;
  double ue_speed = 0;
  int NCS_config = 13,rootSequenceIndex=0;
  int threequarter_fs = 0;
  int mu=1;
  uint64_t SSB_positions=0x01;

  int loglvl=OAILOG_INFO;

  cpuf = get_cpu_freq_GHz();


  if ( load_configmodule(argc,argv,CONFIG_ENABLECMDLINEONLY) == 0) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }

  randominit(0);


  while ((c = getopt (argc, argv, "hHaA:Cr:p:g:n:s:S:t:x:y:v:V:z:N:F:d:Z:L:R:E")) != -1) {
    switch (c) {
    case 'a':
      printf("Running AWGN simulation\n");
      awgn_flag = 1;
      /* ntrials not used later, no need to set */
      //ntrials=1;
      break;

    case 'd':
      delay = atoi(optarg);
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
        channel_model=Rayleigh8;
        break;

      case 'I':
        channel_model=Rayleigh1;
        break;

      case 'J':
        channel_model=Rayleigh1_corr;
        break;

      case 'K':
        channel_model=Rayleigh1_anticorr;
        break;

      case 'L':
        channel_model=Rice8;
        break;

      case 'M':
        channel_model=Rice1;
        break;

      case 'N':
        channel_model=Rayleigh1_800;
        break;

      default:
        msg("Unsupported channel model!\n");
        exit(-1);
      }

      break;

    case 'E':
      threequarter_fs=1;
      break;

    case 'n':
      n_frames = atoi(optarg);
      break;

    case 's':
      snr0 = atof(optarg);
      msg("Setting SNR0 to %f\n",snr0);
      break;

    case 'S':
      snr1 = atof(optarg);
      snr1set=1;
      msg("Setting SNR1 to %f\n",snr1);
      break;

    case 'p':
      preamble_tx=atoi(optarg);
      break;

    case 'v':
      ue_speed0 = atoi(optarg);
      break;

    case 'V':
      ue_speed1 = atoi(optarg);
      ue_speed1set = 1;
      break;

    case 'Z':
      NCS_config = atoi(optarg);

      if ((NCS_config > 15) || (NCS_config < 0))
        printf("Illegal NCS_config %d, (should be 0-15)\n",NCS_config);

      break;

    case 'H':
      printf("High-Speed Flag enabled\n");
      hs_flag = 1;
      break;

    case 'L':
      rootSequenceIndex = atoi(optarg);

      if ((rootSequenceIndex < 0) || (rootSequenceIndex > 837))
        printf("Illegal rootSequenceNumber %d, (should be 0-837)\n",rootSequenceIndex);

      break;

    case 'x':
      transmission_mode=atoi(optarg);

      if ((transmission_mode!=1) &&
          (transmission_mode!=2) &&
          (transmission_mode!=6)) {
        msg("Unsupported transmission mode %d\n",transmission_mode);
        exit(-1);
      }

      break;

    case 'y':
      n_tx=atoi(optarg);

      if ((n_tx==0) || (n_tx>2)) {
        msg("Unsupported number of tx antennas %d\n",n_tx);
        exit(-1);
      }

      break;

    case 'z':
      n_rx=atoi(optarg);

      if ((n_rx==0) || (n_rx>2)) {
        msg("Unsupported number of rx antennas %d\n",n_rx);
        exit(-1);
      }

      break;

    case 'N':
      Nid_cell = atoi(optarg);
      break;

    case 'R':
      N_RB_UL = atoi(optarg);
      break;

    case 'F':
      break;

    default:
    case 'h':
      printf("%s -h(elp) -a(wgn on) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n",
             argv[0]);
      printf("-h This message\n");
      printf("-a Use AWGN channel and not multipath\n");
      printf("-n Number of frames to simulate\n");
      printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
      printf("-S Ending SNR, runs from SNR0 to SNR1\n");
      printf("-g [A,B,C,D,E,F,G,I,N] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) or Rayleigh1 (I) or Rayleigh1_800 (N) models (ignores delay spread and Ricean factor)\n");
      printf("-z Number of RX antennas used in gNB\n");
      printf("-N Nid_cell\n");
      printf("-O oversampling factor (1,2,4,8,16)\n");
      //    printf("-f PRACH format (0=1,1=2,2=3,3=4)\n");
      printf("-d Channel delay \n");
      printf("-v Starting UE velocity in km/h, runs from 'v' to 'v+50km/h'. If n_frames is 1 just 'v' is simulated \n");
      printf("-V Ending UE velocity in km/h, runs from 'v' to 'V'");
      printf("-L rootSequenceIndex (0-837)\n");
      printf("-Z NCS_config (ZeroCorrelationZone) (0-15)\n");
      printf("-H Run with High-Speed Flag enabled \n");
      printf("-R Number of PRB (6,15,25,50,75,100)\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      exit (-1);
      break;
    }
  }

  logInit();

  set_glog(loglvl);
  T_stdout = 1;

  SET_LOG_DEBUG(PRACH); 

  if (snr1set==0) {
    if (n_frames==1)
      snr1 = snr0+.1;
    else
      snr1 = snr0+5.0;
  }

  RC.gNB = (PHY_VARS_gNB**) malloc(2*sizeof(PHY_VARS_gNB *));
  RC.gNB[0] = malloc(sizeof(PHY_VARS_gNB));
  memset(RC.gNB[0],0,sizeof(PHY_VARS_gNB));

  RC.ru = (RU_t**) malloc(2*sizeof(RU_t *));
  RC.ru[0] = (RU_t*) malloc(sizeof(RU_t ));
  memset(RC.ru[0],0,sizeof(RU_t));
  RC.nb_RU = 1;

  gNB = RC.gNB[0];
  ru = RC.ru[0];


  if (ue_speed1set==0) {
    if (n_frames==1)
      ue_speed1 = ue_speed0+10;
    else
      ue_speed1 = ue_speed0+50;
  }

  printf("SNR0 %f, SNR1 %f\n",snr0,snr1);

  frame_parms = &gNB->frame_parms;



  s_re = malloc(2*sizeof(double*));
  s_im = malloc(2*sizeof(double*));
  r_re = malloc(2*sizeof(double*));
  r_im = malloc(2*sizeof(double*));

  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
  frame_parms->nb_antennas_tx = n_tx;
  frame_parms->nb_antennas_rx = n_rx;
  frame_parms->N_RB_DL = N_RB_UL;
  frame_parms->N_RB_UL = N_RB_UL;
  frame_parms->threequarter_fs = threequarter_fs;


  nr_phy_config_request_sim(gNB,N_RB_UL,N_RB_UL,mu,Nid_cell,SSB_positions);

  frame_parms->frame_type = TDD;
  frame_parms->freq_range = nr_FR1;

  //nsymb = (frame_parms->Ncp == 0) ? 14 : 12;

  printf("FFT Size %d, Extended Prefix %d, Samples per subframe %d,Frame type %s, Frequency Range %s\n",NUMBER_OF_OFDM_CARRIERS,
         frame_parms->Ncp,frame_parms->samples_per_subframe,frame_parms->frame_type == FDD ? "FDD" : "TDD", frame_parms->freq_range == nr_FR1 ? "FR1" : "FR2");

  ru->nr_frame_parms=frame_parms;
  ru->if_south = LOCAL_RF;
  ru->nb_tx = n_tx;
  ru->nb_rx = n_rx;

  RC.nb_nr_L1_inst=1;
  phy_init_nr_gNB(gNB,0,0);
  nr_phy_init_RU(ru);
  set_tdd_config_nr(&gNB->gNB_config, 5000,
		    7, 6,
		    2, 4);

    //configure UE
  UE = malloc(sizeof(PHY_VARS_NR_UE));
  memset((void*)UE,0,sizeof(PHY_VARS_NR_UE));
  PHY_vars_UE_g = malloc(2*sizeof(PHY_VARS_NR_UE**));
  PHY_vars_UE_g[0] = malloc(2*sizeof(PHY_VARS_NR_UE*));
  PHY_vars_UE_g[0][0] = UE;
  memcpy(&UE->frame_parms,frame_parms,sizeof(NR_DL_FRAME_PARMS));
  if (init_nr_ue_signal(UE, 1, 0) != 0)
  {
    printf("Error at UE NR initialisation\n");
    exit(-1);
  }

  txdata = UE->common_vars.txdata;
  printf("txdata %p\n",&txdata[0][subframe*frame_parms->samples_per_subframe]);

  double fs,bw;


  bw = N_RB_UL*(180e3)*(1<<gNB->frame_parms.numerology_index);
  AssertFatal(bw<=122.88e6,"Illegal channel bandwidth %f (mu %d,N_RB_UL %d)\n",bw, gNB->frame_parms.numerology_index,N_RB_UL);
  if (bw <= 30.72e6)       fs = 30.72e6;
  else if (bw <= 61.44e6)  fs = 61.44e6;
  else if (bw <= 122.88e6) fs = 122.88e6;
  LOG_I(PHY,"Running with bandwidth %f Hz, fs %f samp/s, FRAME_LENGTH_COMPLEX_SAMPLES %d\n",bw,fs,FRAME_LENGTH_COMPLEX_SAMPLES);

  
  UE2gNB = new_channel_desc_scm(UE->frame_parms.nb_antennas_tx,
                                gNB->frame_parms.nb_antennas_rx,
                                channel_model,
				fs,
				bw,
                                0.0,
                                delay,
                                0);

  if (UE2gNB==NULL) {
    printf("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  for (i=0; i<2; i++) {

    s_re[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(s_re[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    s_im[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(s_im[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));

    r_re[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(r_re[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    r_im[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(r_im[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
  }

  UE->frame_parms.prach_config_common.rootSequenceIndex=rootSequenceIndex;
  UE->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex=98;
  UE->frame_parms.prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig=NCS_config;
  UE->frame_parms.prach_config_common.prach_ConfigInfo.highSpeedFlag=hs_flag;
  UE->frame_parms.prach_config_common.prach_ConfigInfo.restrictedSetConfig=0;
  UE->frame_parms.prach_config_common.prach_ConfigInfo.msg1_frequencystart=0;


  gNB->frame_parms.prach_config_common.rootSequenceIndex=rootSequenceIndex;
  gNB->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex=98;
  gNB->frame_parms.prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig=NCS_config;
  gNB->frame_parms.prach_config_common.prach_ConfigInfo.highSpeedFlag=hs_flag;
  gNB->frame_parms.prach_config_common.prach_ConfigInfo.restrictedSetConfig=0;
  gNB->frame_parms.prach_config_common.prach_ConfigInfo.msg1_frequencystart=0;

  gNB->proc.slot_rx    = subframe<<1;

  gNB->common_vars.rxdata = ru->common.rxdata;


  compute_nr_prach_seq(gNB->frame_parms.prach_config_common.rootSequenceIndex,
		       gNB->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
		       gNB->frame_parms.prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
		       gNB->frame_parms.prach_config_common.prach_ConfigInfo.highSpeedFlag,
		       gNB->frame_parms.frame_type,
		       gNB->frame_parms.freq_range,
		       gNB->X_u);

  compute_nr_prach_seq(UE->frame_parms.prach_config_common.rootSequenceIndex,
		       UE->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
		       UE->frame_parms.prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
		       UE->frame_parms.prach_config_common.prach_ConfigInfo.highSpeedFlag,
		       UE->frame_parms.frame_type,
		       UE->frame_parms.freq_range,
		       UE->X_u);



  UE->prach_vars[0]->amp = AMP;

  UE->prach_resources[0] = &prach_resources;

  if (preamble_tx == 99)
    preamble_tx = (uint16_t)(taus()&0x3f);

  if (n_frames == 1)
    printf("raPreamble %d\n",preamble_tx);

  UE->prach_resources[0]->ra_PreambleIndex = preamble_tx;
  UE->prach_resources[0]->ra_TDD_map_index = 0;

  /*tx_lev = generate_nr_prach(UE,
			     0, //gNB_id,
			     subframe,
			     0); //Nf */ //commented for testing purpose

  UE_nr_rxtx_proc_t proc={0};
  nr_ue_prach_procedures(UE,&proc,0,0,0);


  /* tx_lev_dB not used later, no need to set */
  //tx_lev_dB = (unsigned int) dB_fixed(tx_lev);

  LOG_M("txsig0.m","txs0", &txdata[0][subframe*frame_parms->samples_per_subframe],frame_parms->samples_per_subframe,1,1);
  //LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

  // multipath channel
  dump_nr_prach_config(&gNB->frame_parms,subframe);

  for (i=0; i<2*frame_parms->samples_per_subframe; i++) {
    for (aa=0; aa<1; aa++) {
      if (awgn_flag == 0) {
        s_re[aa][i] = ((double)(((short *)&txdata[aa][subframe*frame_parms->samples_per_subframe]))[(i<<1)]);
        s_im[aa][i] = ((double)(((short *)&txdata[aa][subframe*frame_parms->samples_per_subframe]))[(i<<1)+1]);
      } else {
        for (aarx=0; aarx<gNB->frame_parms.nb_antennas_rx; aarx++) {
          if (aa==0) {
            r_re[aarx][i] = ((double)(((short *)&txdata[aa][subframe*frame_parms->samples_per_subframe]))[(i<<1)]);
            r_im[aarx][i] = ((double)(((short *)&txdata[aa][subframe*frame_parms->samples_per_subframe]))[(i<<1)+1]);
          } else {
            r_re[aarx][i] += ((double)(((short *)&txdata[aa][subframe*frame_parms->samples_per_subframe]))[(i<<1)]);
            r_im[aarx][i] += ((double)(((short *)&txdata[aa][subframe*frame_parms->samples_per_subframe]))[(i<<1)+1]);
          }
        }
      }
    }
  }



  for (SNR=snr0; SNR<snr1; SNR+=.2) {
    for (ue_speed=ue_speed0; ue_speed<ue_speed1; ue_speed+=10) {
      delay_avg = 0.0;
      // max Doppler shift
      UE2gNB->max_Doppler = 1.9076e9*(ue_speed/3.6)/3e8;
      printf("n_frames %d SNR %f\n",n_frames,SNR);
      prach_errors=0;

      for (trial=0; trial<n_frames; trial++) {

        sigma2_dB = 10*log10((double)tx_lev) - SNR;

        if (n_frames==1)
          printf("sigma2_dB %f (SNR %f dB) tx_lev_dB %f\n",sigma2_dB,SNR,10*log10((double)tx_lev));

        //AWGN
        sigma2 = pow(10,sigma2_dB/10);
        //  printf("Sigma2 %f (sigma2_dB %f)\n",sigma2,sigma2_dB);


        if (awgn_flag == 0) {
          multipath_tv_channel(UE2gNB,s_re,s_im,r_re,r_im,
                               2*frame_parms->samples_per_subframe,0);
        }

        if (n_frames==1) {
          printf("rx_level data symbol %f, tx_lev %f\n",
                 10*log10(signal_energy_fp(r_re,r_im,1,OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0)),
                 10*log10(tx_lev));
        }

        for (i=0; i<frame_parms->samples_per_subframe; i++) {
          for (aa=0; aa<gNB->frame_parms.nb_antennas_rx; aa++) {

            ((short*) &gNB->common_vars.rxdata[aa][subframe*(frame_parms->samples_per_subframe)])[2*i] = (short) (.167*(r_re[aa][i] +sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
            ((short*) &gNB->common_vars.rxdata[aa][subframe*(frame_parms->samples_per_subframe)])[2*i+1] = (short) (.167*(r_im[aa][i] + (iqim*r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
          }
        }
	uint16_t preamble_rx;
        rx_nr_prach_ru(ru,
		       0,
		       subframe);
	gNB->prach_vars.rxsigF = ru->prach_rxsigF;

        rx_nr_prach(gNB,
		    0,
		    subframe,
		    &preamble_rx,
		    preamble_energy_list,
		    preamble_delay_list);
	
        if (preamble_rx!=preamble_tx)
          prach_errors++;
        else {
          delay_avg += (double)preamble_delay_list[preamble_tx];
        }

        if (n_frames==1) {
	  printf("preamble %d (tx %d) : energy %d, delay %d\n",preamble_rx,preamble_tx,preamble_energy_list[0],preamble_delay_list[0]);
	  
          
          LOG_M("prach0.m","prach0", &txdata[0][subframe*frame_parms->samples_per_subframe],frame_parms->samples_per_subframe,1,1);
          LOG_M("prachF0.m","prachF0", &gNB->prach_vars.prachF[0],24576,1,1);
          LOG_M("rxsig0.m","rxs0",
                       &gNB->common_vars.rxdata[0][subframe*frame_parms->samples_per_subframe],
                       frame_parms->samples_per_subframe,1,1);
          LOG_M("rxsigF0.m","rxsF0", gNB->prach_vars.rxsigF[0],839*4,1,1);
          LOG_M("prach_preamble.m","prachp",&gNB->X_u[0],839,1,1);
        }
      }

      printf("SNR %f dB, UE Speed %f km/h: errors %u/%d (delay %f)\n",SNR,ue_speed,prach_errors,n_frames,delay_avg/(double)(n_frames-prach_errors));
      //printf("(%f,%f)\n",ue_speed,(double)prach_errors/(double)n_frames);
    } // UE Speed loop

    //printf("SNR %f dB, UE Speed %f km/h: errors %d/%d (delay %f)\n",SNR,ue_speed,prach_errors,n_frames,delay_avg/(double)(n_frames-prach_errors));
    //  printf("(%f,%f)\n",SNR,(double)prach_errors/(double)n_frames);
  } //SNR loop


  for (i=0; i<2; i++) {
    free(s_re[i]);
    free(s_im[i]);
    free(r_re[i]);
    free(r_im[i]);
  }

  free(s_re);
  free(s_im);
  free(r_re);
  free(r_im);


  return(0);

}



/*
  for (i=1;i<4;i++)
    memcpy((void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[i*12*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX*2],
     (void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[0],
     12*OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX*2);
*/

