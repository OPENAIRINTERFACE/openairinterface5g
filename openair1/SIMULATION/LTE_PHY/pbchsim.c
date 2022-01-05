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

#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "LAYER2/MAC/vars.h"

#ifdef XFORMS
  #include "PHY/TOOLS/lte_phy_scope.h"
#endif

#include "OCG_vars.h"

#include "unitary_defs.h"

PHY_VARS_eNB *eNB,*eNB1,*eNB2;
PHY_VARS_UE *UE;

double cpuf;

#define DLSCH_RB_ALLOC 0x1fbf // igore DC component,RB13

int32_t *dummybuf[4];
int32_t dummy0[2048*14];
int32_t dummy1[2048*14];
int32_t dummy2[2048*14];
int32_t dummy3[2048*14];


int main(int argc, char **argv) {
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
  channel_desc_t *eNB2UE,*eNB2UE1 = NULL,*eNB2UE2 = NULL;
  uint32_t nsymb,tx_lev,tx_lev1 = 0,tx_lev2 = 0;
  uint8_t extended_prefix_flag=0;
  int8_t interf1=-21,interf2=-21;
  LTE_DL_FRAME_PARMS *frame_parms;
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
  uint8_t N_RB_DL=25,osf=1;
  unsigned char frame_type = 0;
  unsigned char pbch_phase = 0;
#ifdef XFORMS
  FD_lte_phy_scope_ue *form_ue;
  char title[255];
#endif
  cpuf = get_cpu_freq_GHz();
  logInit();
  number_of_cards = 1;

  while ((c = getopt (argc, argv, "f:hA:pf:g:i:j:n:s:S:t:x:y:z:N:F:GR:O:dP:")) != -1) {
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

      case 'O':
        osf = atoi(optarg);
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

  if (transmission_mode>=2)
    n_tx=2;

  lte_param_init(n_tx, n_tx,n_rx,transmission_mode,extended_prefix_flag,frame_type,Nid_cell,3,N_RB_DL,0,osf,0);
  eNB1 = malloc(sizeof(PHY_VARS_eNB));
  eNB2 = malloc(sizeof(PHY_VARS_eNB));
  UE->measurements.n_adj_cells=2;
  UE->measurements.adj_cell_id[0] = Nid_cell+1;
  UE->measurements.adj_cell_id[1] = Nid_cell+2;

  for (i=0; i<3; i++)
    lte_gold(&eNB->frame_parms,UE->lte_gold_table[i],Nid_cell+i);

  memcpy((void *)&eNB1->frame_parms,(void *)&eNB->frame_parms,sizeof(LTE_DL_FRAME_PARMS));
  eNB1->frame_parms.Nid_cell=Nid_cell+1;
  eNB1->frame_parms.nushift=(Nid_cell+1)%6;
  eNB1->Mod_id=1;
  memcpy((void *)&eNB2->frame_parms,(void *)&eNB->frame_parms,sizeof(LTE_DL_FRAME_PARMS));
  eNB2->frame_parms.Nid_cell=Nid_cell+2;
  eNB2->frame_parms.nushift=(Nid_cell+2)%6;
  eNB2->Mod_id=2;
  phy_init_lte_eNB(eNB1,0,0);
  phy_init_lte_eNB(eNB2,0,0);
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
  frame_parms = &eNB->frame_parms;
  txdata = eNB->common_vars.txdata[0];
  txdata1 = eNB1->common_vars.txdata[0];
  txdata2 = eNB2->common_vars.txdata[0];
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
  printf("FFT Size %d, Extended Prefix %d, Samples per subframe %d, Symbols per subframe %d, interf (%d,%d)\n",NUMBER_OF_OFDM_CARRIERS,
         frame_parms->Ncp,frame_parms->samples_per_tti,nsymb,interf1,interf2);
  printf("eNB1->common_vars.txdataF[0][0] = %p\n",
         eNB1->common_vars.txdataF[0][0]);
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
  eNB2UE = new_channel_desc_scm(eNB->frame_parms.nb_antennas_tx,
                                UE->frame_parms.nb_antennas_rx,
                                channel_model,
                                N_RB2sampling_rate(eNB->frame_parms.N_RB_DL),
                                N_RB2channel_bandwidth(eNB->frame_parms.N_RB_DL),
                                0,
                                0,
                                0, 0);

  if (interf1>-20)
    eNB2UE1 = new_channel_desc_scm(eNB->frame_parms.nb_antennas_tx,
                                   UE->frame_parms.nb_antennas_rx,
                                   channel_model,
                                   N_RB2sampling_rate(eNB->frame_parms.N_RB_DL),
                                   N_RB2channel_bandwidth(eNB->frame_parms.N_RB_DL),
                                   0,
                                   4,
                                   0, 0);

  if (interf2>-20)
    eNB2UE2 = new_channel_desc_scm(eNB->frame_parms.nb_antennas_tx,
                                   UE->frame_parms.nb_antennas_rx,
                                   channel_model,
                                   N_RB2sampling_rate(eNB->frame_parms.N_RB_DL),
                                   N_RB2channel_bandwidth(eNB->frame_parms.N_RB_DL),
                                   0,
                                   8,
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

  /*
  switch (frame_parms->nb_antennas_tx) {
  case 1:
    amask = 0x0000;
    break;
  case 2:
    amask = 0xffff;
    break;
  case 4:
    amask = 0x5555;
  }
  */
  if (pbch_file_fd!=NULL) {
    load_pbch_desc(pbch_file_fd);
  }

  if (input_fd==NULL) {
    //    for (i=0;i<6;i++)
    //      pbch_pdu[i] = i;
    pbch_pdu[0]=100;
    pbch_pdu[1]=1;
    pbch_pdu[2]=0;

    if (eNB->frame_parms.frame_type == 0) {
      generate_pss(eNB->common_vars.txdataF[0],
                   AMP,
                   &eNB->frame_parms,
                   (eNB->frame_parms.Ncp==0) ? 6 : 5,
                   0);
      generate_sss(eNB->common_vars.txdataF[0],
                   AMP,
                   &eNB->frame_parms,
                   (eNB->frame_parms.Ncp==0) ? 5 : 4,
                   0);
      generate_pss(eNB->common_vars.txdataF[0],
                   AMP,
                   &eNB->frame_parms,
                   (eNB->frame_parms.Ncp==0) ? 6 : 5,
                   10);
      generate_sss(eNB->common_vars.txdataF[0],
                   AMP,
                   &eNB->frame_parms,
                   (eNB->frame_parms.Ncp==0) ? 5 : 4,
                   10);
    } else {
      generate_sss(eNB->common_vars.txdataF[0],
                   AMP,
                   &eNB->frame_parms,
                   (eNB->frame_parms.Ncp==0) ? 6 : 5,
                   1);
      generate_pss(eNB->common_vars.txdataF[0],
                   AMP,
                   &eNB->frame_parms,
                   2,
                   2);
      generate_sss(eNB->common_vars.txdataF[0],
                   AMP,
                   &eNB->frame_parms,
                   (eNB->frame_parms.Ncp==0) ? 6 : 5,
                   11);
      generate_pss(eNB->common_vars.txdataF[0],
                   AMP,
                   &eNB->frame_parms,
                   2,
                   12);
    }

    /*
    memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu2,sizeof(DCI2_5MHz_2A_M10PRB_TDD_t));
    dci_alloc[0].dci_length = sizeof_DCI2_5MHz_2A_M10PRB_TDD_t;
    dci_alloc[0].L          = 2;
    dci_alloc[0].rnti       = 0x1234;
    dci_alloc[0].nCCE       = 0;
    memcpy(&dci_alloc[1].dci_pdu[1],&DLSCH_alloc_pdu2,sizeof(DCI2_5MHz_2A_M10PRB_TDD_t));
    dci_alloc[1].dci_length = sizeof_DCI2_5MHz_2A_M10PRB_TDD_t;
    dci_alloc[1].L          = 3;
    dci_alloc[1].rnti       = 0x1234;
    */
    printf("Generating PBCH for mode1_flag = %d\n", eNB->frame_parms.mode1_flag);
    generate_pilots(eNB,
                    eNB->common_vars.txdataF[0],
                    AMP,
                    LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);

    /*
    num_pdcch_symbols = generate_dci_top(1,
           dci_alloc,
           0,
           1024,
           &eNB->frame_parms,
           eNB->common_vars.txdataF[0],
           0);
    */

    if (num_pdcch_symbols<3) {
      printf("Less than 3 pdcch symbols\n");
      //  exit(-1);
    }

    if (pbch_phase>0) {
      dummybuf[0] = dummy0;
      dummybuf[1] = dummy1;
      dummybuf[2] = dummy2;
      dummybuf[3] = dummy3;
      generate_pbch(&eNB->pbch,
                    (int32_t **)dummybuf,
                    AMP,
                    &eNB->frame_parms,
                    pbch_pdu,
                    0);
    }

    generate_pbch(&eNB->pbch,
                  eNB->common_vars.txdataF[0],
                  AMP,
                  &eNB->frame_parms,
                  pbch_pdu,
                  pbch_phase);
    /*
    generate_pbch(&eNB->pbch,
      eNB->common_vars.txdataF[0],
      AMP,
      &eNB->frame_parms,
      pbch_pdu,
      1);
    */

    if (interf1>-20) {
      /*
      generate_pss(eNB1->common_vars.txdataF[0],
       AMP,
       &eNB1->frame_parms,
       (eNB1->frame_parms.Ncp==0)?6:5,
       0);
      */
      generate_pilots(eNB1,
                      eNB1->common_vars.txdataF[0],
                      AMP,
                      LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);
      generate_pbch(&eNB1->pbch,
                    eNB1->common_vars.txdataF[0],
                    AMP,
                    &eNB1->frame_parms,
                    pbch_pdu,
                    0);
    }

    if (interf2>-20) {
      /*
      generate_pss(eNB2->common_vars.txdataF[0],
       AMP,
       &eNB2->frame_parms,
       (eNB2->frame_parms.Ncp==0)?6:5,
       0);

      */
      generate_pilots(eNB2,
                      eNB2->common_vars.txdataF[0],
                      AMP,
                      LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);
      generate_pbch(&eNB2->pbch,
                    eNB2->common_vars.txdataF[0],
                    AMP,
                    &eNB2->frame_parms,
                    pbch_pdu,
                    0);
    }

    //  LOG_M("pilotsF.m","rsF",txdataF[0],frame_parms->ofdm_symbol_size,1,1);
    LOG_M("txsigF0.m","txsF0", eNB->common_vars.txdataF[0][0],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

    if (eNB->frame_parms.nb_antennas_tx>1)
      LOG_M("txsigF1.m","txsF1", eNB->common_vars.txdataF[0][1],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

    tx_lev = 0;
    tx_lev1 = 0;
    tx_lev2 = 0;

    for (aa=0; aa<eNB->frame_parms.nb_antennas_tx; aa++) {
      if (frame_parms->Ncp == 1)
        PHY_ofdm_mod(eNB->common_vars.txdataF[0][aa],        // input,
                     txdata[aa],         // output
                     frame_parms->ofdm_symbol_size,
                     LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,                 // number of symbols
                     frame_parms->nb_prefix_samples,               // number of prefix samples
                     CYCLIC_PREFIX);
      else {
        normal_prefix_mod(eNB->common_vars.txdataF[0][aa],
                          txdata[aa],
                          LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,
                          frame_parms);
      }

      tx_lev += signal_energy(&txdata[aa][frame_parms->samples_per_tti/2],
                              OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
    }

    if (interf1>-20) {
      for (aa=0; aa<eNB1->frame_parms.nb_antennas_tx; aa++) {
        if (frame_parms->Ncp == 1)
          PHY_ofdm_mod(eNB1->common_vars.txdataF[0][aa],        // input,
                       eNB1->common_vars.txdata[0][aa],         // output
                       frame_parms->ofdm_symbol_size,
                       2*nsymb,                 // number of symbols
                       frame_parms->nb_prefix_samples,               // number of prefix samples
                       CYCLIC_PREFIX);
        else {
          normal_prefix_mod(eNB1->common_vars.txdataF[0][aa],
                            eNB1->common_vars.txdata[0][aa],
                            2*nsymb,
                            frame_parms);
        }

        tx_lev1 += signal_energy(&txdata1[aa][frame_parms->samples_per_tti/2],         // output
                                 OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
      }

      LOG_M("txsigF0_1.m","txsF0_1", eNB1->common_vars.txdataF[0][0],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

      if (eNB1->frame_parms.nb_antennas_tx>1)
        LOG_M("txsigF1_1.m","txsF1_1", eNB1->common_vars.txdataF[0][1],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);
    }

    if (interf2>-20) {
      for (aa=0; aa<eNB2->frame_parms.nb_antennas_tx; aa++) {
        if (frame_parms->Ncp == 1)
          PHY_ofdm_mod(eNB2->common_vars.txdataF[0][aa],        // input,
                       eNB2->common_vars.txdata[0][aa],         // output
                       frame_parms->ofdm_symbol_size,
                       2*nsymb,                 // number of symbols
                       frame_parms->nb_prefix_samples,               // number of prefix samples
                       CYCLIC_PREFIX);
        else {
          normal_prefix_mod(eNB2->common_vars.txdataF[0][aa],
                            eNB2->common_vars.txdata[0][aa],
                            2*nsymb,
                            frame_parms);
        }

        tx_lev2 += signal_energy(&txdata2[aa][frame_parms->samples_per_tti/2],         // output
                                 OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
      }

      LOG_M("txsigF0_2.m","txsF0_2", eNB2->common_vars.txdataF[0][0],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

      if (eNB2->frame_parms.nb_antennas_tx>1)
        LOG_M("txsigF1_2.m","txsF1_2", eNB2->common_vars.txdataF[0][1],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);
    }

    //    tx_lev_dB = (unsigned int) dB_fixed(tx_lev);
    LOG_M("txsig0.m","txs0", txdata[0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

    if (frame_parms->nb_antennas_tx>1)
      LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

    LOG_M("txsig0_1.m","txs0_1", eNB1->common_vars.txdata[0][0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

    if (frame_parms->nb_antennas_tx>1)
      LOG_M("txsig1_1.m","txs1_1", eNB1->common_vars.txdata[0][1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

    LOG_M("txsig0_2.m","txs0_2", eNB2->common_vars.txdata[0][0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

    if (frame_parms->nb_antennas_tx>1)
      LOG_M("txsig1_2.m","txs1_2", eNB2->common_vars.txdata[0][1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
  } else { //read in from file
    i=0;

    while (!feof(input_fd)) {
      if (fscanf(input_fd,"%49s %49s",input_val_str,input_val_str2) != 2) { //&input_val1,&input_val2);
        printf("%s:%d:%s: error with fscanf, exiting\n", __FILE__, __LINE__, __FUNCTION__);
        exit(1);
      }

      if ((i%4)==0) {
        ((short *)txdata[0])[i/2] = (short)((1<<15)*strtod(input_val_str,NULL));
        ((short *)txdata[0])[(i/2)+1] = (short)((1<<15)*strtod(input_val_str2,NULL));

        if ((i/4)<100)
          printf("sample %d => %e + j%e (%d +j%d)\n",i/4,strtod(input_val_str,NULL),strtod(input_val_str2,NULL),((short *)txdata[0])[i/4],((short *)txdata[0])[(i/4)+1]); //1,input_val2,);
      }

      i++;

      if (i>(FRAME_LENGTH_SAMPLES))
        break;
    }

    printf("Read in %d samples\n",i/4);
    LOG_M("txsig0.m","txs0", txdata[0],2*frame_parms->samples_per_tti,1,1);
    //    LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
    tx_lev = signal_energy(&txdata[0][0],
                           OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
    //    tx_lev_dB = (unsigned int) dB_fixed(tx_lev);
  }

  // multipath channel

  for (i=0; i<2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; i++) {
    for (aa=0; aa<eNB->frame_parms.nb_antennas_tx; aa++) {
      s_re[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)]);
      s_im[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)+1]);

      if (interf1>-20) {
        s_re1[aa][i] = ((double)(((short *)txdata1[aa]))[(i<<1)]);
        s_im1[aa][i] = ((double)(((short *)txdata1[aa]))[(i<<1)+1]);
      }

      if (interf2>-20) {
        s_re2[aa][i] = ((double)(((short *)txdata2[aa]))[(i<<1)]);
        s_im2[aa][i] = ((double)(((short *)txdata2[aa]))[(i<<1)+1]);
      }
    }
  }

  for (SNR=snr0; SNR<snr1; SNR+=.2) {
    n_errors = 0;
    n_errors2 = 0;
    n_alamouti = 0;

    for (trial=0; trial<n_frames; trial++) {
      pbch_sinr=0.0;

      if (abstraction_flag==1)
        printf("*********************** trial %d ***************************\n",trial);

      while (pbch_sinr>-2.0) {
        multipath_channel(eNB2UE,s_re,s_im,r_re,r_im,
                          2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0);

        if (interf1>-20)
          multipath_channel(eNB2UE1,s_re1,s_im1,r_re1,r_im1,
                            2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0);

        if (interf2>-20)
          multipath_channel(eNB2UE2,s_re2,s_im2,r_re2,r_im2,
                            2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0);

        if (abstraction_flag == 1) {
          freq_channel(eNB2UE,25,51);

          if (interf1>-20)
            freq_channel(eNB2UE1,25,51);

          if (interf2>-20)
            freq_channel(eNB2UE2,25,51);

          pbch_sinr = compute_pbch_sinr(eNB2UE,eNB2UE1,eNB2UE2,SNR,SNR+interf1,SNR+interf2,25);
          printf("total_sinr %f\n",compute_sinr(eNB2UE,eNB2UE1,eNB2UE2,SNR,SNR+interf1,SNR+interf2,25));
          printf("pbch_sinr %f => BLER %f\n",pbch_sinr,pbch_bler(pbch_sinr));
        } else {
          pbch_sinr = -3.0;
        }
      }

      sigma2_dB = 10*log10((double)tx_lev) +10*log10((double)eNB->frame_parms.ofdm_symbol_size/(double)(12*NB_RB)) - SNR;

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
          for (aa=0; aa<eNB->frame_parms.nb_antennas_rx; aa++) {
            if (n_trials==0) {
              r_re[aa][i] += (pow(10.0,.05*interf1)*r_re1[aa][i] + pow(10.0,.05*interf2)*r_re2[aa][i]);
              r_im[aa][i] += (pow(10.0,.05*interf1)*r_im1[aa][i] + pow(10.0,.05*interf2)*r_im2[aa][i]);
            }

            ((short *) UE->common_vars.rxdata[aa])[2*i] = (short) (.167*(r_re[aa][i] +sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
            ((short *) UE->common_vars.rxdata[aa])[2*i+1] = (short) (.167*(r_im[aa][i] + (iqim*r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
          }
        }

        if (n_frames==1) {
          printf("rx_level data symbol %f\n",
                 10*log10(signal_energy(&UE->common_vars.rxdata[0][frame_parms->samples_per_tti/2],4*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES)));
        }

        /*
        // optional: read rx_frame from file
        if ((rx_frame_file = fopen("rx_frame.dat","r")) == NULL)
        {
          printf("[openair][CHBCH_TEST][INFO] Cannot open rx_frame.m data file\n");
          exit(0);
        }

        result = fread((void *)UE->common_vars.rxdata[0],4,FRAME_LENGTH_COMPLEX_SAMPLES,rx_frame_file);
        printf("Read %d bytes\n",result);
        if (UE->frame_parms.nb_antennas_rx>1) {
          result = fread((void *)UE->common_vars.rxdata[1],4,FRAME_LENGTH_COMPLEX_SAMPLES,rx_frame_file);
          printf("Read %d bytes\n",result);
        }

        fclose(rx_frame_file);
        */

        for (l=0; l<eNB->frame_parms.symbols_per_tti; l++) {
          //    subframe_offset = (l/eNB->frame_parms.symbols_per_tti)*eNB->frame_parms.samples_per_tti;
          //      printf("subframe_offset = %d\n",subframe_offset);
          slot_fep(UE,
                   l%(eNB->frame_parms.symbols_per_tti/2),
                   l/(eNB->frame_parms.symbols_per_tti/2),
                   0,
                   0,
                   0);

          if (l==0) {
            lte_ue_measurements(UE,
                                0,
                                1,
                                0,
                                0,
                                0);
            /*
             if (trial%100 == 0) {
               msg("[PHY_PROCEDURES_LTE] frame %d, RX RSSI %d dBm, digital (%d, %d) dB, linear (%d, %d), RX gain %d dB\n",
             trial,
             UE->measurements.rx_rssi_dBm[0],
             UE->measurements.rx_power_dB[0][0],
             UE->measurements.rx_power_dB[0][1],
             UE->measurements.rx_power[0][0],
             UE->measurements.rx_power[0][1],
             UE->rx_total_gain_dB);

               msg("[PHY_PROCEDURES_LTE] frame %d, N0 digital (%d, %d) dB, linear (%d, %d)\n",
             trial,
             UE->measurements.n0_power_dB[0],
             UE->measurements.n0_power_dB[1],
             UE->measurements.n0_power[0],
             UE->measurements.n0_power[1]);

               msg("[PHY_PROCEDURES_LTE] frame %d, freq_offset_filt = %d\n",
             trial, freq_offset);

             }
            */
          }

          if (l==((eNB->frame_parms.Ncp==0)?4:3)) {
            //sprintf(fname,"dl_ch00_%d.m",l);
            //sprintf(vname,"dl_ch00_%d",l);
            //LOG_M(fname,vname,&(common_vars->dl_ch_estimates[0][frame_parms->ofdm_symbol_size*(l%6)]),frame_parms->ofdm_symbol_size,1,1);
            lte_est_freq_offset(UE->common_vars.common_vars_rx_data_per_thread[/*subframe*/0&0x1].dl_ch_estimates[0],
                                &UE->frame_parms,
                                l,
                                &freq_offset,
                                1);
          }

          if (l==((eNB->frame_parms.Ncp==0)?10:9)) {
            for (frame_mod4=0; frame_mod4<4; frame_mod4++) {
              pbch_tx_ant = rx_pbch(&UE->common_vars,
                                    UE->pbch_vars[0],
                                    &UE->frame_parms,
                                    0,
                                    SISO,
                                    UE->high_speed_flag,
                                    frame_mod4);

              if ((pbch_tx_ant>0) && (pbch_tx_ant<4)) {
                UE->frame_parms.mode1_flag = 1;
                break;

                if (pbch_phase != frame_mod4)
                  printf("pbch_phase different!!!\n");
              }

              pbch_tx_ant = rx_pbch(&UE->common_vars,
                                    UE->pbch_vars[0],
                                    &eNB->frame_parms,
                                    0,
                                    ALAMOUTI,
                                    UE->high_speed_flag,
                                    frame_mod4);

              if ((pbch_tx_ant>0) && (pbch_tx_ant<4)) {
                UE->frame_parms.mode1_flag = 0;
                n_alamouti++;
                break;
              }
            }

            if ((pbch_tx_ant>0) && (pbch_tx_ant<4)) {
              if (n_frames==1)
                msg("pbch decoded sucessfully mode1_flag %d, frame_mod4 %d, tx_ant %d!\n",
                    UE->frame_parms.mode1_flag,frame_mod4,pbch_tx_ant);
            } else {
              n_errors++;
              n_errors2++;

              if (n_frames==1)
                msg("pbch error (%d)\n",pbch_tx_ant);
            }
          }
        }

#ifdef XFORMS
        phy_scope_UE(form_ue,
                     UE,
                     0,0,1);
#endif
      } //noise trials

      if (abstraction_flag==1) {
        printf("SNR %f : n_errors = %d/%d, n_alamouti %d\n", SNR,n_errors,ntrials,n_alamouti);

        if (write_output_file==1)
          fprintf(output_fd,"%f %f %e %e\n",SNR,pbch_sinr,(double)n_errors/ntrials,pbch_bler(pbch_sinr));
      }

      n_errors=0;

      if ((abstraction_flag==0) && (n_errors2>1000) && (trial>5000))
        break;
    } // trials

    if (abstraction_flag==0) {
      printf("SNR %f : n_errors2 = %d/%d (BLER %e,40ms BLER %e,%d,%d), n_alamouti %d\n", SNR,n_errors2,ntrials*(1+trial),(double)n_errors2/(ntrials*(1+trial)),pow((double)n_errors2/(ntrials*(1+trial)),4),
             ntrials,trial,n_alamouti);

      if (write_output_file==1)
        fprintf(output_fd,"%f %e\n",SNR,(double)n_errors2/(ntrials*(1+trial)));
    }
  } // NSR

  if (n_frames==1) {
    LOG_M("H00.m","h00",&(UE->common_vars.common_vars_rx_data_per_thread[/*subframe*/0&0x1].dl_ch_estimates[0][0][0]),((frame_parms->Ncp==0)?7:6)*(eNB->frame_parms.ofdm_symbol_size),1,1);

    if (n_tx==2)
      LOG_M("H10.m","h10",&(UE->common_vars.common_vars_rx_data_per_thread[/*subframe*/0&0x1].dl_ch_estimates[0][2][0]),((frame_parms->Ncp==0)?7:6)*(eNB->frame_parms.ofdm_symbol_size),1,1);

    LOG_M("rxsig0.m","rxs0", UE->common_vars.rxdata[0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
    LOG_M("rxsigF0.m","rxsF0", UE->common_vars.common_vars_rx_data_per_thread[/*subframe*/0&0x1].rxdataF[0],NUMBER_OF_OFDM_CARRIERS*2*((frame_parms->Ncp==0)?14:12),2,1);
    LOG_M("PBCH_rxF0_ext.m","pbch0_ext",UE->pbch_vars[0]->rxdataF_ext[0],12*4*6,1,1);
    LOG_M("PBCH_rxF0_comp.m","pbch0_comp",UE->pbch_vars[0]->rxdataF_comp[0],12*4*6,1,1);
    LOG_M("PBCH_rxF_llr.m","pbch_llr",UE->pbch_vars[0]->llr,(frame_parms->Ncp==0) ? 1920 : 1728,1,4);
  }

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
  lte_sync_time_free();

  if (write_output_file)
    fclose(output_fd);

  if (input_fd != NULL)
    fclose(input_fd);

  return(n_errors);
}



/*
  for (i=1;i<4;i++)
    memcpy((void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[i*12*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX*2],
     (void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[0],
     12*OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX*2);
*/

