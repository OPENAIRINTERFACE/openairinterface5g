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

#include "SIMULATION/TOOLS/sim.h"
#include "SIMULATION/RF/rf.h"
#include "PHY/types.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/defs_gNB.h"

#include "PHY/INIT/phy_init.h"
#include "SCHED_NR/sched_nr.h"

#include "common/ran_context.h" 

PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;


double cpuf;

// dummy functions
int nfapi_mode=0;
int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req) { return(0);}
int oai_nfapi_tx_req(nfapi_tx_request_t *tx_req) { return(0); }

int oai_nfapi_dl_config_req(nfapi_dl_config_request_t *dl_config_req) { return(0); }

int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req) { return(0); }

int oai_nfapi_nr_dl_config_req(nfapi_nr_dl_config_request_t *dl_config_req) {return(0);}

uint32_t from_earfcn(int eutra_bandP,uint32_t dl_earfcn) {return(0);}
int32_t get_uldl_offset(int eutra_bandP) {return(0);}

NR_IF_Module_t *NR_IF_Module_init(int Mod_id){return(NULL);}

void exit_fun(const char *s) { exit(-1); }

int main(int argc, char **argv)
{

  char c;

  int i,l,aa;
  double sigma2, sigma2_dB=0,SNR,snr0=-2.0,snr1;
  uint8_t snr1set=0;
  int **txdata,**txdata1,**txdata2;
  double **s_re,**s_im,**s_re1,**s_im1,**s_re2,**s_im2,**r_re,**r_im,**r_re1,**r_im1,**r_re2,**r_im2;
  double iqim = 0.0;
  unsigned char pbch_pdu[6];
  //  int sync_pos, sync_pos_slot;
  //  FILE *rx_frame_file;
  FILE *output_fd = NULL;
  uint8_t write_output_file=0;
  //int result;
  int freq_offset;
  //  int subframe_offset;
  //  char fname[40], vname[40];
  int trial, n_trials, ntrials=1, n_errors,n_errors2,n_alamouti;
  uint8_t transmission_mode = 1,n_tx=1,n_rx=1;
  uint16_t Nid_cell=0;

  int n_frames=1;
  channel_desc_t *gNB2UE;
  uint32_t nsymb,tx_lev,tx_lev1 = 0,tx_lev2 = 0;
  uint8_t extended_prefix_flag=0;
  int8_t interf1=-21,interf2=-21;

  FILE *input_fd=NULL,*pbch_file_fd=NULL;
  char input_val_str[50],input_val_str2[50];
  //  double input_val1,input_val2;
  //  uint16_t amask=0;
  uint8_t frame_mod4,num_pdcch_symbols = 0;
  uint16_t NB_RB=25;

  SCM_t channel_model=AWGN;//Rayleigh1_anticorr;

  //DCI_ALLOC_t dci_alloc[8];
  uint8_t abstraction_flag=0;//,calibration_flag=0;
  double pbch_sinr;
  int pbch_tx_ant;
  uint8_t N_RB_DL=106,mu=1;

  unsigned char frame_type = 0;
  unsigned char pbch_phase = 0;

  int frame=0,subframe=0;
  int frame_length_complex_samples;
  int frame_length_complex_samples_no_prefix;
  NR_DL_FRAME_PARMS *frame_parms;

  cpuf = get_cpu_freq_GHz();

  if ( load_configmodule(argc,argv) == 0) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }

  logInit();

  while ((c = getopt (argc, argv, "f:hA:pf:g:i:j:n:s:S:t:x:y:z:N:F:GR:dP:")) != -1) {
    switch (c) {
    case 'f':
      write_output_file=1;
      output_fd = fopen(optarg,"w");

      if (output_fd==NULL) {
        printf("Error opening %s\n",optarg);
        exit(-1);
      }

      break;

    case 'd':
      frame_type = 1;
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

      default:
        msg("Unsupported channel model!\n");
        exit(-1);
      }

      break;

    case 'i':
      interf1=atoi(optarg);
      break;

    case 'j':
      interf2=atoi(optarg);
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

      /*
      case 't':
      Td= atof(optarg);
      break;
      */
    case 'p':
      extended_prefix_flag=1;
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

    case 'A':
      abstraction_flag=1;
      ntrials=10000;
      msg("Running Abstraction test\n");
      pbch_file_fd=fopen(optarg,"r");

      if (pbch_file_fd==NULL) {
        printf("Problem with filename %s\n",optarg);
        exit(-1);
      }

      break;

      //  case 'C':
      //    calibration_flag=1;
      //    msg("Running Abstraction calibration for Bias removal\n");
      //    break;
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

    case 'P':
      pbch_phase = atoi(optarg);

      if (pbch_phase>3)
        printf("Illegal PBCH phase (0-3) got %d\n",pbch_phase);

      break;

    default:
    case 'h':
      printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n",
             argv[0]);
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
      printf("-N Nid_cell\n");
      printf("-R N_RB_DL\n");
      printf("-O oversampling factor (1,2,4,8,16)\n");
      printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
      //    printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
      printf("-f Output filename (.txt format) for Pe/SNR results\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      exit (-1);
      break;
    }
  }

  s_re = malloc(2*sizeof(double*));
  s_im = malloc(2*sizeof(double*));
  s_re1 = malloc(2*sizeof(double*));
  s_im1 = malloc(2*sizeof(double*));
  s_re2 = malloc(2*sizeof(double*));
  s_im2 = malloc(2*sizeof(double*));
  r_re = malloc(2*sizeof(double*));
  r_im = malloc(2*sizeof(double*));
  r_re1 = malloc(2*sizeof(double*));
  r_im1 = malloc(2*sizeof(double*));
  r_re2 = malloc(2*sizeof(double*));
  r_im2 = malloc(2*sizeof(double*));

  gNB2UE = new_channel_desc_scm(n_tx,
                                n_rx,
                                channel_model,
 				61.44e6, //N_RB2sampling_rate(N_RB_DL),
				40e6, //N_RB2channel_bandwidth(N_RB_DL),
                                0,
                                0,
                                0);

  if (gNB2UE==NULL) {
    msg("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  RC.gNB = (PHY_VARS_gNB***) malloc(sizeof(PHY_VARS_gNB **));
  RC.gNB[0] = (PHY_VARS_gNB**) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0][0] = malloc(sizeof(PHY_VARS_gNB));
  gNB = RC.gNB[0][0];
  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)

  //NR_PHY_Config_t *phy_config = malloc(sizeof(NR_PHY_Config_t));
  nr_phy_config_request_sim(gNB);
  phy_init_nr_gNB(gNB,0,0);

  frame_length_complex_samples = frame_parms->samples_per_subframe;
  frame_length_complex_samples_no_prefix = frame_parms->samples_per_subframe_wCP;

  for (i=0; i<2; i++) {

    s_re[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(s_re[i],frame_length_complex_samples*sizeof(double));
    s_im[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(s_im[i],frame_length_complex_samples*sizeof(double));

    r_re[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(r_re[i],frame_length_complex_samples*sizeof(double));
    r_im[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(r_im[i],frame_length_complex_samples*sizeof(double));
  }


  if (pbch_file_fd!=NULL) {
    load_pbch_desc(pbch_file_fd);
  }

  if (input_fd==NULL) {
    nr_common_signal_procedures (gNB,frame,subframe);
  }

  LOG_M("txsigF0.m","txsF0", gNB->common_vars.txdataF[0],frame_length_complex_samples_no_prefix,1,1);

  if (gNB->frame_parms.nb_antennas_tx>1)
    LOG_M("txsigF1.m","txsF1", gNB->common_vars.txdataF[1],frame_length_complex_samples_no_prefix,1,1);




  // multipath channel
  /*
  for (i=0; i<frame_length_complex_samples; i++) {
    for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
      s_re[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)]);
      s_im[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)+1]);
      }
    }
  */
  
  for (SNR=snr0; SNR<snr1; SNR+=.2) {

    n_errors = 0;
    n_errors2 = 0;
    n_alamouti = 0;

    for (trial=0; trial<n_trials; trial++) {

      multipath_channel(gNB2UE,s_re,s_im,r_re,r_im,frame_length_complex_samples,0);
      
      sigma2_dB = 10*log10((double)tx_lev) +10*log10((double)gNB->frame_parms.ofdm_symbol_size/(double)(12*NB_RB)) - SNR;
      
      if (n_frames==1)
	printf("sigma2_dB %f (SNR %f dB) tx_lev_dB %f,%f,%f\n",sigma2_dB,SNR,
	       10*log10((double)tx_lev),
	       10*log10((double)tx_lev1),
	       10*log10((double)tx_lev2));
      
      //AWGN
      sigma2 = pow(10,sigma2_dB/10);
      
      
      //printf("n_trial %d\n",n_trials);
      for (i=0; i<frame_length_complex_samples; i++) {
	for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
	  
	  ((short*) UE->common_vars.rxdata[aa])[2*i] = (short) ((r_re[aa][i] +sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
	  ((short*) UE->common_vars.rxdata[aa])[2*i+1] = (short) ((r_im[aa][i] + (iqim*r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
	}
      }
      
    } //noise trials

  } // NSR

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


  if (write_output_file)
    fclose(output_fd);

  return(n_errors);

}



