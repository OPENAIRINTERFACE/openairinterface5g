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

#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "MAC_INTERFACE/vars.h"

#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "LAYER2/MAC/vars.h"

#ifdef XFORMS
  #include "PHY/TOOLS/lte_phy_scope.h"
#endif

#define BW 20.0


PHY_VARS_eNB *PHY_vars_eNB,*PHY_vars_eNB1,*PHY_vars_eNB2;
PHY_VARS_UE *PHY_vars_UE;

#define DLSCH_RB_ALLOC 0x1fbf // igore DC component,RB13

mod_sym_t *dummybuf[4];
mod_sym_t dummy0[2048*14];
mod_sym_t dummy1[2048*14];
mod_sym_t dummy2[2048*14];
mod_sym_t dummy3[2048*14];


int main(int argc, char **argv) {
  char c;
  int i,l,aa;
  double sigma2, sigma2_dB=0,SNR,snr0=-2.0,snr1;
  uint8_t snr1set=0;
  //mod_sym_t **txdataF;
  int **txdata,**txdata1,**txdata2;
  double **s_re,**s_im,**s_re1,**s_im1,**s_re2,**s_im2,**r_re,**r_im,**r_re1,**r_im1,**r_re2,**r_im2;
  double iqim = 0.0;
  unsigned char pbch_pdu[6];
  //  int sync_pos, sync_pos_slot;
  //  FILE *rx_frame_file;
  FILE *output_fd;
  uint8_t write_output_file=0;
  int result;
  int freq_offset;
  //  int subframe_offset;
  //  char fname[40], vname[40];
  int trial, n_trials, ntrials=1, n_errors,n_errors2,n_alamouti;
  uint8_t transmission_mode = 1,n_tx=1,n_rx=1;
  uint16_t Nid_cell=0;
  int n_frames=1;
  channel_desc_t *eNB2UE,*eNB2UE1,*eNB2UE2;
  uint32_t nsymb,tx_lev,tx_lev1,tx_lev2;
  uint8_t extended_prefix_flag=0;
  LTE_DL_FRAME_PARMS *frame_parms;
  FILE *input_fd=NULL,*pbch_file_fd=NULL;
  char input_val_str[50],input_val_str2[50];
  //  double input_val1,input_val2;
  //  uint16_t amask=0;
  uint8_t frame_mod4,num_pdcch_symbols;
  uint16_t NB_RB=25;
  SCM_t channel_model=AWGN;//Rayleigh1_anticorr;
  DCI_ALLOC_t dci_alloc[8];
  uint8_t abstraction_flag=0;//,calibration_flag=0;
  int pbch_tx_ant;
  uint8_t N_RB_DL=100,osf=1;
  unsigned char frame_type = FDD;
  unsigned char pbch_phase = 0;
#ifdef XFORMS
  FD_lte_phy_scope_ue *form_ue;
  char title[255];
#endif
  logInit();
  number_of_cards = 1;
  openair_daq_vars.rx_rf_mode = 1;

  while ((c = getopt (argc, argv, "f:hpf:g:n:s:S:t:x:y:z:N:F:GdP:")) != -1) {
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
        frame_type = TDD;
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
        extended_prefix_flag=1;
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

      case 'N':
        Nid_cell = atoi(optarg);
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
        printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -N CellId\n",
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
        printf("-N Nid_cell\n");
        printf("-f Output filename (.txt format) for Pe/SNR results\n");
        printf("-F Input filename (.txt format) for RX conformance testing\n");
        exit (-1);
        break;
    }
  }

  if (transmission_mode>=2)
    n_tx=2;

  lte_param_init(n_tx,
                 n_rx,
                 transmission_mode,
                 extended_prefix_flag,
                 frame_type,
                 Nid_cell,
                 3,
                 N_RB_DL,
                 osf,
                 0);
#ifdef XFORMS
  fl_initialize (&argc, argv, NULL, 0, 0);
  form_ue = create_lte_phy_scope_ue();
  sprintf (title, "LTE PHY SCOPE UE");
  fl_show_form (form_ue->lte_phy_scope_ue, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
#endif

  if (snr1set==0) {
    if (n_frames==1)
      snr1 = snr0+.1;
    else
      snr1 = snr0+5.0;
  }

  printf("SNR0 %f, SNR1 %f\n",snr0,snr1);
  frame_parms = &PHY_vars_eNB->lte_frame_parms;
  txdata = PHY_vars_eNB->lte_eNB_common_vars.txdata[0];
  txdata1 = PHY_vars_eNB1->lte_eNB_common_vars.txdata[0];
  txdata2 = PHY_vars_eNB2->lte_eNB_common_vars.txdata[0];
  s_re = malloc(2*sizeof(double *));
  s_im = malloc(2*sizeof(double *));
  s_re1 = malloc(2*sizeof(double *));
  s_im1 = malloc(2*sizeof(double *));
  s_re2 = malloc(2*sizeof(double *));
  s_im2 = malloc(2*sizeof(double *));
  r_re = malloc(2*sizeof(double *));
  r_im = malloc(2*sizeof(double *));
  r_re1 = malloc(2*sizeof(double *));
  r_im1 = malloc(2*sizeof(double *));
  r_re2 = malloc(2*sizeof(double *));
  r_im2 = malloc(2*sizeof(double *));
  nsymb = (frame_parms->Ncp == 0) ? 14 : 12;
  printf("FFT Size %d, Extended Prefix %d, Samples per subframe %d, Symbols per subframe %u\n",NUMBER_OF_OFDM_CARRIERS,
         frame_parms->Ncp,frame_parms->samples_per_tti,nsymb);
  printf("PHY_vars_eNB1->lte_eNB_common_vars.txdataF[0][0] = %p\n",
         PHY_vars_eNB1->lte_eNB_common_vars.txdataF[0][0]);
  DLSCH_alloc_pdu2.rah              = 0;
  DLSCH_alloc_pdu2.rballoc          = DLSCH_RB_ALLOC;
  DLSCH_alloc_pdu2.TPC              = 0;
  DLSCH_alloc_pdu2.dai              = 0;
  DLSCH_alloc_pdu2.harq_pid         = 0;
  DLSCH_alloc_pdu2.tb_swap          = 0;
  DLSCH_alloc_pdu2.mcs1             = 0;
  DLSCH_alloc_pdu2.ndi1             = 1;
  DLSCH_alloc_pdu2.rv1              = 0;
  // Forget second codeword
  DLSCH_alloc_pdu2.tpmi             = (transmission_mode==6 ? 5 : 0) ;  // precoding
  eNB2UE = new_channel_desc_scm(PHY_vars_eNB->lte_frame_parms.nb_antennas_tx,
                                PHY_vars_UE->lte_frame_parms.nb_antennas_rx,
                                channel_model,
                                N_RB2sampling_rate(PHY_vars_eNB->lte_frame_parms.N_RB_DL),
                                N_RB2channel_bandwidth(PHY_vars_eNB->lte_frame_parms.N_RB_DL),
                                0,
                                0,
                                0, 0);

  if (eNB2UE==NULL) {
    msg("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  for (i=0; i<2; i++) {
    s_re[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(s_re[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    s_im[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(s_im[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    s_re1[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(s_re1[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    s_im1[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(s_im1[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    s_re2[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(s_re2[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    s_im2[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(s_im2[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    r_re[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(r_re[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    r_im[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(r_im[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    r_re1[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(r_re1[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    r_im1[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(r_im1[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    r_re2[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(r_re2[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    r_im2[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero(r_im2[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
  }

  pbch_pdu[0]=100;
  pbch_pdu[1]=1;
  pbch_pdu[2]=0;

  if (PHY_vars_eNB->lte_frame_parms.frame_type == FDD) {
    generate_pss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                 AMP,
                 &PHY_vars_eNB->lte_frame_parms,
                 (PHY_vars_eNB->lte_frame_parms.Ncp==NORMAL) ? 6 : 5,
                 0);
    /*
    generate_sss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                 AMP,
                 &PHY_vars_eNB->lte_frame_parms,
                 (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 5 : 4,
                 0);*/
    generate_pss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                 AMP,
                 &PHY_vars_eNB->lte_frame_parms,
                 (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
                 10);
    /*
    generate_sss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                 AMP,
                 &PHY_vars_eNB->lte_frame_parms,
                 (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 5 : 4,
                 10);
    */
  } else {
    generate_sss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                 AMP,
                 &PHY_vars_eNB->lte_frame_parms,
                 (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
                 1);
    generate_pss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                 AMP,
                 &PHY_vars_eNB->lte_frame_parms,
                 2,
                 2);
    generate_sss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                 AMP,
                 &PHY_vars_eNB->lte_frame_parms,
                 (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
                 11);
    generate_pss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                 AMP,
                 &PHY_vars_eNB->lte_frame_parms,
                 2,
                 12);
  }

  /*
  generate_pilots(PHY_vars_eNB,
      PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
      AMP,
      LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);



     num_pdcch_symbols = generate_dci_top(1,
     dci_alloc,
     0,
     1024,
     &PHY_vars_eNB->lte_frame_parms,
     PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
     0);
  */
  /*
  if (num_pdcch_symbols<3) {
    printf("Less than 3 pdcch symbols\n");
    //  exit(-1);
  }

  if (pbch_phase>0) {
    dummybuf[0] = dummy0;
    dummybuf[1] = dummy1;
    dummybuf[2] = dummy2;
    dummybuf[3] = dummy3;
    generate_pbch(&PHY_vars_eNB->lte_eNB_pbch,
      (mod_sym_t**)dummybuf,
      AMP,
      &PHY_vars_eNB->lte_frame_parms,
      pbch_pdu,
      0);
  }

  generate_pbch(&PHY_vars_eNB->lte_eNB_pbch,
    PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
    AMP,
    &PHY_vars_eNB->lte_frame_parms,
    pbch_pdu,
    pbch_phase);
  */
  LOG_M("txsigF0.m","txsF0", PHY_vars_eNB->lte_eNB_common_vars.txdataF[0][0],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

  if (PHY_vars_eNB->lte_frame_parms.nb_antennas_tx>1)
    LOG_M("txsigF1.m","txsF1", PHY_vars_eNB->lte_eNB_common_vars.txdataF[0][1],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

  tx_lev = 0;
  tx_lev1 = 0;
  tx_lev2 = 0;

  for (aa=0; aa<PHY_vars_eNB->lte_frame_parms.nb_antennas_tx; aa++) {
    if (frame_parms->Ncp == 1)
      PHY_ofdm_mod(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0][aa],        // input,
                   txdata[aa],         // output
                   frame_parms->ofdm_symbol_size,
                   LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,                 // number of symbols
                   frame_parms->nb_prefix_samples,               // number of prefix samples
                   CYCLIC_PREFIX);
    else {
      normal_prefix_mod(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0][aa],
                        txdata[aa],
                        LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,
                        frame_parms);
    }

    tx_lev += signal_energy(&txdata[aa][frame_parms->samples_per_tti/2],
                            OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
  }

  LOG_M("txsig0.m","txs0", txdata[0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

  if (frame_parms->nb_antennas_tx>1)
    LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

  // multipath channel

  for (i=0; i<2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; i++) {
    for (aa=0; aa<PHY_vars_eNB->lte_frame_parms.nb_antennas_tx; aa++) {
      s_re[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)]);
      s_im[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)+1]);
    }
  }

  for (SNR=snr0; SNR<snr1; SNR+=.2) {
    n_errors = 0;
    n_errors2 = 0;
    n_alamouti = 0;

    for (trial=0; trial<n_frames; trial++) {
      multipath_channel(eNB2UE,s_re,s_im,r_re,r_im,
                        2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0);
      sigma2_dB = 10*log10((double)tx_lev) +10*log10((double)PHY_vars_eNB->lte_frame_parms.ofdm_symbol_size/(double)(12*NB_RB)) - SNR;

      if (n_frames==1)
        printf("sigma2_dB %f (SNR %f dB) tx_lev_dB %f,%f,%f\n",sigma2_dB,SNR,
               10*log10((double)tx_lev),
               10*log10((double)tx_lev1),
               10*log10((double)tx_lev2));

      //AWGN
      sigma2 = pow(10,sigma2_dB/10);

      /*
      if (n_frames==1) {
      printf("rx_level data symbol %f, tx_lev %f\n",
      10*log10(signal_energy_fp(r_re,r_im,1,OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0)),
      10*log10(tx_lev));
      }
      */

      for (n_trials=0; n_trials<ntrials; n_trials++) {
        //printf("n_trial %d\n",n_trials);
        for (i=0; i<2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; i++) {
          for (aa=0; aa<PHY_vars_eNB->lte_frame_parms.nb_antennas_rx; aa++) {
            ((short *) PHY_vars_UE->lte_ue_common_vars.rxdata[aa])[2*i] = (short) ((r_re[aa][i] +sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
            ((short *) PHY_vars_UE->lte_ue_common_vars.rxdata[aa])[2*i+1] = (short) ((r_im[aa][i] + (iqim*r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
          }
        }

        lte_sync_timefreq(PHY_vars_UE,0,2680000000);

        if (n_frames==1) {
          printf("rx_level data symbol %f\n",
                 10*log10(signal_energy(&PHY_vars_UE->lte_ue_common_vars.rxdata[0][frame_parms->samples_per_tti/2],4*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES)));
        }
      } //noise trials
    } // trials

    if (abstraction_flag==0) {
      printf("SNR %f : n_errors2 = %d/%d (BLER %e,40ms BLER %e,%d,%d), n_alamouti %d\n", SNR,n_errors2,ntrials*(1+trial),(double)n_errors2/(ntrials*(1+trial)),pow((double)n_errors2/(ntrials*(1+trial)),4),
             ntrials,trial,n_alamouti);

      if (write_output_file==1)
        fprintf(output_fd,"%f %e\n",SNR,(double)n_errors2/(ntrials*(1+trial)));
    }
  } // NSR

  if (n_frames==1) {
  }

  for (i=0; i<2; i++) {
    free(s_re[i]);
    free(s_im[i]);

    if (s_im1)
      free(s_im1[i]);

    if (s_im2)
      free(s_im2[i]);

    free(r_re[i]);

    if (r_re1)
      free(r_re1[i]);

    if (r_re2)
      free(r_re2[i]);

    free(r_im[i]);

    if (s_re1)
      free(s_re1[i]);

    if (s_re2)
      free(s_re2[i]);

    if (r_im1)
      free(r_im1[i]);

    if (r_im2)
      free(r_im2[i]);
  }

  free(s_re);
  free(s_re1);
  free(s_re2);
  free(s_im);
  free(s_im1);
  free(s_im2);
  free(r_re);
  free(r_re1);
  free(r_re2);
  free(r_im);
  free(r_im1);
  free(r_im2);
  lte_sync_time_free();

  if (write_output_file)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  if (pbch_file_fd)
    fclose(pbch_file_fd);

  return(n_errors);
}



/*
    for (i=1;i<4;i++)
    memcpy((void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[i*12*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX*2],
    (void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[0],
    12*OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX*2);
*/

