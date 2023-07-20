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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "MAC_INTERFACE/vars.h"
#ifdef IFFT_FPGA
  #include "PHY/LTE_REFSIG/mod_table.h"
#endif
#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "radio/COMMON/defs.h"
#include "LAYER2/MAC/vars.h"

#ifdef XFORMS
  #include <forms.h>
  #include "../../USERSPACE_TOOLS/SCOPE/lte_scope.h"
#endif //XFORMS


#include "openair_hw.h"

#define BW 5.0


PHY_VARS_eNB *PHY_vars_eNB, *PHY_vars_eNB1, *PHY_vars_eNB2;
PHY_VARS_UE *PHY_vars_UE[2];

#define DLSCH_RB_ALLOC 0x1fff // igore DC component,RB13

#ifdef XFORMS
void do_forms2(FD_lte_scope *form,
               LTE_DL_FRAME_PARMS *frame_parms,
               int pdcch_symbols,
               int16_t **channel,
               int16_t **channel_f,
               int16_t **rx_sig,
               int16_t **rx_sig_f,
               int16_t *pdcch_comp,
               int16_t *dlsch_comp,
               int16_t *dlsch_comp_i,
               int16_t *dlsch_llr,
               int16_t *pbch_comp,
               int8_t *pbch_llr,
               int coded_bits_per_codeword) {
  int i,j,k,s;
  float Re,Im;
  float mag_sig[NB_ANTENNAS_RX*4*NUMBER_OF_OFDM_CARRIERS*NUMBER_OF_OFDM_SYMBOLS_PER_SLOT],
        sig_time[NB_ANTENNAS_RX*4*NUMBER_OF_OFDM_CARRIERS*NUMBER_OF_OFDM_SYMBOLS_PER_SLOT],
        sig2[FRAME_LENGTH_COMPLEX_SAMPLES],
        time2[FRAME_LENGTH_COMPLEX_SAMPLES],
        I[25*12*11*4], Q[25*12*11*4],
        *llr,*llr_time;
  int ind;
  float avg, cum_avg;
  extern int *sync_corr_ue0;
  //  uint16_t nsymb = (frame_parms->Ncp == 0) ? 14 : 12;
  llr = malloc(max(coded_bits_per_codeword,1920)*sizeof(float));
  llr_time = malloc(max(coded_bits_per_codeword,1920)*sizeof(float));

  // Channel frequency response
  if (channel_f[0] != NULL) {
    cum_avg = 0;
    ind = 0;

    for (j=0; j<2; j++) {
      for (i=0; i<frame_parms->nb_antennas_rx; i++) {
        for (k=0; k<(13*frame_parms->N_RB_DL); k++) {
          sig_time[ind] = (float)ind;
          Re = (float)(channel_f[(j<<1)+i][(2*k)]);
          Im = (float)(channel_f[(j<<1)+i][(2*k)+1]);
          //mag_sig[ind] = (short) rand();
          mag_sig[ind] = (short)10*log10(1.0+((double)Re*Re + (double)Im*Im));
          cum_avg += (short)sqrt((double)Re*Re + (double)Im*Im) ;
          ind++;
        }

        //      ind+=NUMBER_OF_OFDM_CARRIERS/4; // spacing for visualization
      }
    }

    avg = cum_avg/NUMBER_OF_USEFUL_CARRIERS;
    //fl_set_xyplot_ybounds(form->channel_f,30,70);
    fl_set_xyplot_data(form->channel_f,sig_time,mag_sig,ind,"","","");
  }

  // sync_corr
  for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES; i++) {
    time2[i] = (float) i;
    sig2[i] = (float) sync_corr_ue0[i];
  }

  fl_set_xyplot_data(form->channel_t_im,time2,sig2,FRAME_LENGTH_COMPLEX_SAMPLES,"","","");
  fl_set_xyplot_ybounds(form->channel_t_im,0,1e6);

  // rx sig 0
  for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES; i++)  {
    //for (i=0; i<NUMBER_OF_OFDM_CARRIERS*frame_parms->symbols_per_tti/2; i++)  {
    sig2[i] = 10*log10(1.0+(double) ((rx_sig[0][2*i])*(rx_sig[0][2*i])+(rx_sig[0][2*i+1])*(rx_sig[0][2*i+1])));
    time2[i] = (float) i;
  }

  fl_set_xyplot_ybounds(form->channel_t_re,30,60);
  //fl_set_xyplot_data(form->channel_t_re,&time2[640*12*6],&sig2[640*12*6],640*12,"","","");
  fl_set_xyplot_data(form->channel_t_re,time2,sig2,FRAME_LENGTH_COMPLEX_SAMPLES,"","","");

  /*
  // rx sig 1
  for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES; i++)  {
    //for (i=0; i<NUMBER_OF_OFDM_CARRIERS*frame_parms->symbols_per_tti/2; i++)  {
    sig2[i] = 10*log10(1.0+(double) ((rx_sig[1][2*i])*(rx_sig[1][2*i])+(rx_sig[1][2*i+1])*(rx_sig[1][2*i+1])));
    time2[i] = (float) i;
  }
  fl_set_xyplot_ybounds(form->channel_t_im,30,60);
  //fl_set_xyplot_data(form->channel_t_im,&time2[640*12*6],&sig2[640*12*6],640*12,"","","");
  fl_set_xyplot_data(form->channel_t_im,time2,sig2,FRAME_LENGTH_COMPLEX_SAMPLES,"","","");
  */

  // PBCH LLR
  if (pbch_llr!=NULL) {
    j=0;

    for(i=0; i<1920; i++) {
      llr[j] = (float) pbch_llr[i];
      llr_time[j] = (float) j;
      //if (i==63)
      //  i=127;
      //else if (i==191)
      //  i=319;
      j++;
    }

    fl_set_xyplot_data(form->decoder_input,llr_time,llr,1920,"","","");
    //fl_set_xyplot_ybounds(form->decoder_input,-100,100);
  }

  // PBCH I/Q
  if (pbch_comp!=NULL) {
    j=0;

    for(i=0; i<12*12; i++) {
      I[j] = pbch_comp[2*i];
      Q[j] = pbch_comp[2*i+1];
      j++;
      //if (i==47)
      //  i=96;
      //else if (i==191)
      //  i=239;
    }

    fl_set_xyplot_data(form->scatter_plot,I,Q,12*12,"","","");
    //fl_set_xyplot_xbounds(form->scatter_plot,-100,100);
    //fl_set_xyplot_ybounds(form->scatter_plot,-100,100);
  }

  // PDCCH I/Q
  j=0;

  for(i=0; i<12*25*1; i++) {
    I[j] = pdcch_comp[2*i];
    Q[j] = pdcch_comp[2*i+1];
    j++;
  }

  fl_set_xyplot_data(form->scatter_plot1,I,Q,12*25*1,"","","");
  //fl_set_xyplot_xbounds(form->scatter_plot,-100,100);
  //fl_set_xyplot_ybounds(form->scatter_plot,-100,100);

  // DLSCH LLR
  if (dlsch_llr != NULL) {
    for(i=0; i<coded_bits_per_codeword; i++) {
      llr[i] = (float) dlsch_llr[i];
      llr_time[i] = (float) i;
    }

    fl_set_xyplot_data(form->demod_out,llr_time,llr,coded_bits_per_codeword,"","","");
    //    fl_set_xyplot_ybounds(form->demod_out,-1000,1000);
  }

  // DLSCH I/Q
  if (dlsch_comp!=NULL) {
    j=0;

    for (s=pdcch_symbols; s<frame_parms->symbols_per_tti; s++) {
      for(i=0; i<12*25; i++) {
        I[j] = dlsch_comp[(2*25*12*s)+2*i];
        Q[j] = dlsch_comp[(2*25*12*s)+2*i+1];
        j++;
      }

      //if (s==2)
      //  s=3;
      //else if (s==5)
      //  s=6;
      //else if (s==8)
      //  s=9;
    }

    fl_set_xyplot_data(form->scatter_plot2,I,Q,j,"","","");
    //fl_set_xyplot_xbounds(form->scatter_plot,-2000,2000);
    //fl_set_xyplot_ybounds(form->scatter_plot,-2000,2000);
  }

  free(llr);
  free(llr_time);
}
#endif

void lte_param_init(unsigned char N_tx, unsigned char N_rx,unsigned char transmission_mode,unsigned char extended_prefix_flag,uint8_t frame_type,uint16_t Nid_cell,uint8_t N_RB_DL,uint8_t osf) {
  LTE_DL_FRAME_PARMS *lte_frame_parms;
  int i;
  printf("Start lte_param_init, frame_type %d, extended_prefix %d\n",frame_type,extended_prefix_flag);
  PHY_vars_eNB = malloc(sizeof(PHY_VARS_eNB));
  PHY_vars_eNB1 = malloc(sizeof(PHY_VARS_eNB));
  PHY_vars_eNB2 = malloc(sizeof(PHY_VARS_eNB));
  PHY_vars_UE[0] = malloc(sizeof(PHY_VARS_UE));
  PHY_vars_UE[1] = malloc(sizeof(PHY_VARS_UE));
  //PHY_config = malloc(sizeof(PHY_CONFIG));
  mac_xface = malloc(sizeof(MAC_xface));
  randominit(0);
  set_taus_seed(0);
  lte_frame_parms = &(PHY_vars_eNB->lte_frame_parms);
  lte_frame_parms->N_RB_DL            = N_RB_DL;   //50 for 10MHz and 25 for 5 MHz
  lte_frame_parms->N_RB_UL            = N_RB_DL;
  lte_frame_parms->Ncp                = extended_prefix_flag;
  lte_frame_parms->Nid_cell           = Nid_cell;
  lte_frame_parms->nushift            = Nid_cell%6;
  lte_frame_parms->phich_config_common.phich_resource            = oneSixth;
  lte_frame_parms->nb_antennas_tx     = N_tx;
  lte_frame_parms->nb_antennas_rx     = N_rx;
  //  lte_frame_parms->Csrs = 2;
  //  lte_frame_parms->Bsrs = 0;
  //  lte_frame_parms->kTC = 0;
  //  lte_frame_parms->n_RRC = 0;
  lte_frame_parms->mode1_flag = (transmission_mode == 1)? 1 : 0;
  lte_frame_parms->tdd_config = 3;
  lte_frame_parms->frame_type = frame_type;
  lte_frame_parms->node_id = 2;
  init_frame_parms(lte_frame_parms,osf);
  phy_init_top(lte_frame_parms); //allocation
  lte_frame_parms->twiddle_fft      = twiddle_fft;
  lte_frame_parms->twiddle_ifft     = twiddle_ifft;
  lte_frame_parms->rev              = rev;
  phy_init_lte_top(lte_frame_parms);
  memcpy((void *)&PHY_vars_UE[0]->lte_frame_parms,(void *)&PHY_vars_eNB->lte_frame_parms,sizeof(LTE_DL_FRAME_PARMS));
  memcpy((void *)&PHY_vars_UE[1]->lte_frame_parms,(void *)&PHY_vars_eNB->lte_frame_parms,sizeof(LTE_DL_FRAME_PARMS));
  phy_init_lte_ue(PHY_vars_UE[0],1,0);

  for (i=0; i<3; i++)
    lte_gold(lte_frame_parms,PHY_vars_UE[0]->lte_gold_table[i],i);

  PHY_vars_UE[0]->Mod_id = 0;
  phy_init_lte_ue(PHY_vars_UE[1],1,0);

  for (i=0; i<3; i++)
    lte_gold(lte_frame_parms,PHY_vars_UE[1]->lte_gold_table[i],i);

  PHY_vars_UE[1]->Mod_id = 1;
  phy_init_lte_eNB(PHY_vars_eNB,0,0,0);
  memcpy((void *)&PHY_vars_eNB1->lte_frame_parms,(void *)&PHY_vars_eNB->lte_frame_parms,sizeof(LTE_DL_FRAME_PARMS));
  PHY_vars_eNB1->lte_frame_parms.nushift=1;
  PHY_vars_eNB1->lte_frame_parms.Nid_cell=2;
  memcpy((void *)&PHY_vars_eNB2->lte_frame_parms,(void *)&PHY_vars_eNB->lte_frame_parms,sizeof(LTE_DL_FRAME_PARMS));
  PHY_vars_eNB2->lte_frame_parms.nushift=2;
  PHY_vars_eNB2->lte_frame_parms.Nid_cell=3;
  phy_init_lte_eNB(PHY_vars_eNB1,0,0,0);
  phy_init_lte_eNB(PHY_vars_eNB2,0,0,0);
  phy_init_lte_top(lte_frame_parms);
  printf("Done lte_param_init\n");
}



int main(int argc, char **argv) {
  char c;
  int i,iout,l,aa,aarx;
  double sigma2, sigma2_dB=0,SNR,snr0=10.0,snr1=11.0;
  uint8_t snr1set=0;
  //mod_sym_t **txdataF;
#ifdef IFFT_FPGA
  int **txdataF2;
#endif
  int **txdata,**txdata1 = NULL,**txdata2 = NULL;
  double **s_re,**s_im,**s_re1,**s_im1,**s_re2,**s_im2,**r_re,**r_im,**r_re1,**r_im1,**r_re2,**r_im2;
  double iqim = 0.0;
  unsigned char pbch_pdu[6];
  FILE *output_fd=NULL;
  uint8_t write_output_file=0;
  int trial, n_errors=0,n_errors2=0,n_alamouti=0;
  uint8_t transmission_mode = 1,n_tx=1,n_rx=1;
  unsigned char eNb_id = 0;
  uint16_t Nid_cell=0;
  uint8_t awgn_flag=0;
  int n_frames=1;
  channel_desc_t *eNB2UE=NULL,*eNB2UE1=NULL,*eNB2UE2=NULL;
  uint32_t nsymb,tx_lev=0;
  uint8_t extended_prefix_flag=0,frame_type=1;
  int8_t interf1=-21,interf2=-21;
  LTE_DL_FRAME_PARMS *frame_parms;
  FILE *input_fd=NULL,*pbch_file_fd=NULL;
  char input_val_str[50],input_val_str2[50];
  uint8_t num_pdcch_symbols=1;
  uint16_t NB_RB=25;
  SCM_t channel_model=Rayleigh8;
  uint8_t abstraction_flag=0;
  double pbch_sinr;
  uint8_t N_RB_DL=25,osf=1;
  int openair_fd=(int)0;
  int tcxo=74,fc=0;
  unsigned char temp[4];
  int oai_hw_input=0;
  int oai_hw_output=0;
  DCI_ALLOC_t dci_alloc[8],dci_alloc_rx[8];
  uint16_t n_rnti=1234,dci_cnt;
  unsigned int coded_bits_per_codeword;
  double tmp_re,tmp_im,foff,deltaF=0.0,cs,sn;
  uint32_t carrier_freq[4]= {1907600000,1907600000,1907600000,1907600000};
  uint32_t rf_mode[4]     = {55759,55759,55759,55759};
  uint32_t rf_local[4]    = {8254681, 8254816, 8254617, 8254617}; //eNB khalifa
  //{8255067,8254810,8257340,8257340}; // eNB PETRONAS
  uint32_t rf_vcocal[4]   = {910,910,910,910};
  uint32_t rf_rxdc[4]     = {32896,32896,32896,32896};
  uint32_t rxgain[4]= {30,30,30,30};
  uint32_t do_forms=0;
  int ret;
#ifdef XFORMS
  FD_lte_scope *form_dl=NULL;
  FD_lte_scope *form_dl1=NULL;
  char title[255];
#endif
  int UE_idx,N_carriers=1,subframe=2;
  int dual_stream_flag=0;
  unsigned char i_mod = 2;
  int rx_offset_mod;
  logInit();
  number_of_cards = 1;
  openair_daq_vars.rx_rf_mode = 1;

  /*
    rxdataF    = (int **)malloc16(2*sizeof(int*));
    rxdataF[0] = (int *)malloc16(FRAME_LENGTH_BYTES);
    rxdataF[1] = (int *)malloc16(FRAME_LENGTH_BYTES);

    rxdata    = (int **)malloc16(2*sizeof(int*));
    rxdata[0] = (int *)malloc16(FRAME_LENGTH_BYTES);
    rxdata[1] = (int *)malloc16(FRAME_LENGTH_BYTES);
  */
  while ((c = getopt (argc, argv, "aehc:f:g:i:j:n:r:s:t:x:y:z:A:F:N:O:R:S:ZYDT:C:G:dB:U:u")) != -1) {
    switch (c) {
      case 'a':
        printf("Running AWGN simulation\n");
        awgn_flag = 1;
        break;

      case 'f':
        output_fd = fopen(optarg,"w");
        write_output_file=1;
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

      case 'C':
        sscanf(optarg,"%d,%d,%d,%d",&carrier_freq[0],&carrier_freq[1],&carrier_freq[2],&carrier_freq[3]);
        printf("Configuring for frequencies %d,%d,%d,%d\n",carrier_freq[0],carrier_freq[1],carrier_freq[2],carrier_freq[3]);
        //carrier_freq = atoi(optarg);
        break;

      case 'G':
        sscanf(optarg,"%d,%d,%d,%d",&rxgain[0],&rxgain[1],&rxgain[2],&rxgain[3]);
        printf("Configuring for gains %d,%d,%d,%d\n",rxgain[0],rxgain[1],rxgain[2],rxgain[3]);
        //rxgain = atoi(optarg);
        break;

      case 'i':
        interf1=atoi(optarg);
        break;

      case 'c':
        deltaF=atof(optarg);
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
      case 'e':
        extended_prefix_flag=1;
        break;

      case 'd':
        do_forms=1;
        break;

      case 'r':
        n_rnti=atoi(optarg);
        break;

      case 'x':
        transmission_mode=atoi(optarg);

        if ((transmission_mode!=1) &&
            (transmission_mode!=2) &&
            (transmission_mode!=5) &&
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

      /*
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
      */
      case 'D':
        frame_type=0;
        msg("Running in FDD\n");
        break;

      case 'N':
        Nid_cell = atoi(optarg);
        break;

      case 'R':
        N_RB_DL = atoi(optarg);
        break;

      case 'O':
        osf = atoi(optarg);
        break;

      case 'Z':
        oai_hw_input = 1;
        break;

      case 'Y':
        oai_hw_output = 1;
        break;

      case 'F':
        input_fd = fopen(optarg,"r");

        if (input_fd==NULL) {
          printf("Problem with filename %s\n",optarg);
          exit(-1);
        }

        break;

      case 'T':
        tcxo = atoi(optarg);
        break;

      case 'B':
        N_carriers = atoi(optarg);

        if ((N_carriers!=1) && (N_carriers!=2)) {
          printf("N_carriers must be 1 or 2!\n");
          exit(-1);
        }

        break;

      case 'U':
        subframe = atoi(optarg);
        break;

      case 'u':
        dual_stream_flag=1;
        break;

      default:
      case 'h':
        printf("-h This message\n");
        printf("-a Use AWGN channel and not multipath\n");
        printf("-e Use extended prefix mode\n");
        printf("-d Display signal output on XFORMS scope\n");
        printf("-D Use FDD frame\n");
        printf("-n Number of frames to simulate\n");
        printf("-r RNTI for DCI detection in SF 0/5\n");
        printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
        printf("-S Ending SNR, runs from SNR0 to SNR1\n");
        printf("-t Delay spread for multipath channel\n");
        printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
        printf("-c Frequency offset\n");
        printf("-x Transmission mode (1,2,6 for the moment)\n");
        printf("-y Number of TX antennas used in eNB\n");
        printf("-z Number of RX antennas used in UE\n");
        printf("-i Relative strength of first intefering eNB (in dB) - cell_id mod 3 = 1\n");
        printf("-j Relative strength of second intefering eNB (in dB) - cell_id mod 3 = 2\n");
        printf("-N Nid_cell\n");
        printf("-R N_RB_DL\n");
        printf("-O oversampling factor (1,2,4,8,16)\n");
        //printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
        printf("-f Output filename (.txt format) for Pe/SNR results\n");
        printf("-F Input filename (.txt format) for RX conformance testing\n");
        printf("-Y just generate tx frame and send it to hardware\n");
        printf("-Z grab frame from hardware and do rx processing\n");
        printf("-T set TCXO parameter on CBMIMO1 hardware\n");
        printf("-C set frequency for ExpressMIMO hardware. Can take up to four parameters in format f1,f2,f3,f4\n");
        printf("-G set RX gain for ExpressMIMO hardware. Can take up to four parameters in format g1,g2,g3,g4\n");
        printf("-d enables the graphical display");
        exit (-1);
        break;
    }
  }

#ifdef XFORMS

  if (do_forms==1) {
    fl_initialize (&argc, argv, NULL, 0, 0);
    form_dl = create_form_lte_scope();
    sprintf (title, "LTE DL SCOPE UE carrier 0");
    fl_show_form (form_dl->lte_scope, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);

    if (N_carriers==2) {
      form_dl1 = create_form_lte_scope();
      sprintf (title, "LTE DL SCOPE UE carrier 1");
      fl_show_form (form_dl1->lte_scope, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
    }
  }

#endif

  if (transmission_mode==2)
    n_tx=2;

  lte_param_init(n_tx,n_rx,transmission_mode,extended_prefix_flag,
                 frame_type,Nid_cell,N_RB_DL,osf);

  if (snr1set==0) {
    if (n_frames==1)
      snr1 = snr0+.1;
    else
      snr1 = snr0+5.0;
  }

  printf("SNR0 %f, SNR1 %f\n",snr0,snr1);
  frame_parms = &PHY_vars_eNB->lte_frame_parms;
  frame_parms->dual_tx      = 0;
  frame_parms->freq_idx     = 0;

  for (i=0; i<4; i++) {
    frame_parms->carrier_freq[i] = carrier_freq[i];
    frame_parms->carrier_freqtx[i] = carrier_freq[i];
    frame_parms->rxgain[i]       = rxgain[i];
    frame_parms->rfmode[i]       = rf_mode[i];
    frame_parms->rflocal[i]      = rf_local[i];
    frame_parms->rfvcolocal[i]   = rf_vcocal[i];
    frame_parms->rxdc[i]         = rf_rxdc[i];
  }

  if (oai_hw_input == 1) {
    openair_fd=setup_oai_hw(frame_parms);
    setup_ue_buffers(PHY_vars_UE[0],frame_parms,0);

    if (N_carriers==2)
      setup_ue_buffers(PHY_vars_UE[1],frame_parms,1);
  }

  if (oai_hw_output == 1) {
    openair_fd=setup_oai_hw(frame_parms);
    setup_eNB_buffers(PHY_vars_eNB,frame_parms);
  }

  if ((oai_hw_input==1) ||
      (oai_hw_output==1)) {
    msg("setting TCXO to %d\n",tcxo);
    ioctl(openair_fd,openair_SET_TCXO_DAC,(void *)&tcxo);
  }

#ifdef IFFT_FPGA
  txdata    = (int **)malloc16(2*sizeof(int *));
  txdata[0] = (int *)malloc16(FRAME_LENGTH_BYTES);
  txdata[1] = (int *)malloc16(FRAME_LENGTH_BYTES);
  bzero(txdata[0],FRAME_LENGTH_BYTES);
  bzero(txdata[1],FRAME_LENGTH_BYTES);
  txdataF2    = (int **)malloc16(2*sizeof(int *));
  txdataF2[0] = (int *)malloc16(FRAME_LENGTH_BYTES_NO_PREFIX);
  txdataF2[1] = (int *)malloc16(FRAME_LENGTH_BYTES_NO_PREFIX);
  bzero(txdataF2[0],FRAME_LENGTH_BYTES_NO_PREFIX);
  bzero(txdataF2[1],FRAME_LENGTH_BYTES_NO_PREFIX);
#else
  txdata = PHY_vars_eNB->common_vars.txdataF[eNb_id];
  txdata1 = PHY_vars_eNB1->lte_eNB_common_vars.txdata[eNb_id];
  txdata2 = PHY_vars_eNB2->lte_eNB_common_vars.txdata[eNb_id];
#endif
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
  eNB2UE = new_channel_desc_scm(PHY_vars_eNB->lte_frame_parms.nb_antennas_tx,
                                PHY_vars_UE[0]->lte_frame_parms.nb_antennas_rx,
                                channel_model,
                                BW,
                                0,
                                0,
                                0, 0);

  if (interf1>-20)
    eNB2UE1 = new_channel_desc_scm(PHY_vars_eNB->lte_frame_parms.nb_antennas_tx,
                                   PHY_vars_UE[0]->lte_frame_parms.nb_antennas_rx,
                                   channel_model,
                                   BW,
                                   0,
                                   0,
                                   0, 0);

  if (interf2>-20)
    eNB2UE2 = new_channel_desc_scm(PHY_vars_eNB->lte_frame_parms.nb_antennas_tx,
                                   PHY_vars_UE[0]->lte_frame_parms.nb_antennas_rx,
                                   channel_model,
                                   BW,
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

  for (i=0; i<2; i++) {
    PHY_vars_UE[0]->dlsch_ue[0][i]  = new_ue_dlsch(1,8,0);

    if (!PHY_vars_UE[0]->dlsch_ue[0][i]) {
      printf("Can't get ue dlsch structures\n");
      exit(-1);
    }
  }

  for (i=0; i<2; i++) {
    PHY_vars_UE[1]->dlsch_ue[0][i]  = new_ue_dlsch(1,8,0);

    if (!PHY_vars_UE[1]->dlsch_ue[0][i]) {
      printf("Can't get ue dlsch structures\n");
      exit(-1);
    }
  }

  PHY_vars_UE[0]->transmission_mode[0] = transmission_mode;
  PHY_vars_UE[1]->transmission_mode[0] = transmission_mode;

  //  if (pbch_file_fd!=NULL) {
  //    load_pbch_desc(pbch_file_fd);
  //  }

  if ((input_fd==NULL)&&(oai_hw_input==0)) {
    //    for (i=0;i<6;i++)
    //      pbch_pdu[i] = i;
    //pbch_pdu[0]=100;
    //pbch_pdu[1]=1;
    //pbch_pdu[2]=0;
    ((uint8_t *) pbch_pdu)[0] = 0;

    switch (PHY_vars_eNB->lte_frame_parms.N_RB_DL) {
      case 6:
        ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0x1f) | (0<<5);
        break;

      case 15:
        ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0x1f) | (1<<5);
        break;

      case 25:
        ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0x1f) | (2<<5);
        break;

      case 50:
        ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0x1f) | (3<<5);
        break;

      case 100:
        ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0x1f) | (4<<5);
        break;

      default:
        ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0x1f) | (2<<5);
        break;
    }

    ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0xef) |
                                ((PHY_vars_eNB->lte_frame_parms.phich_config_common.phich_duration << 4)&0x10);

    switch (PHY_vars_eNB->lte_frame_parms.phich_config_common.phich_resource) {
      case oneSixth:
        ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0xf3) | (0<<3);
        break;

      case half:
        ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0xf3) | (1<<3);
        break;

      case one:
        ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0xf3) | (2<<3);
        break;

      case two:
        ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0xf3) | (3<<3);
        break;

      default:
        break;
    }

    ((uint8_t *) pbch_pdu)[0] = (((uint8_t *) pbch_pdu)[0]&0xfc) | ((PHY_vars_eNB->frame>>8)&0x3);
    ((uint8_t *) pbch_pdu)[1] = PHY_vars_eNB->frame&0xfc;
    ((uint8_t *) pbch_pdu)[2] = 0;

    if (PHY_vars_eNB->lte_frame_parms.frame_type == 1) {
      generate_pss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                   AMP,
                   &PHY_vars_eNB->lte_frame_parms,
                   2,
                   2);
      generate_pss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                   AMP,
                   &PHY_vars_eNB->lte_frame_parms,
                   2,
                   12);
      generate_sss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                   AMP,
                   &PHY_vars_eNB->lte_frame_parms,
                   (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
                   1);
      generate_sss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                   AMP,
                   &PHY_vars_eNB->lte_frame_parms,
                   (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
                   11);
    } else {
      generate_pss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                   AMP,
                   &PHY_vars_eNB->lte_frame_parms,
                   (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
                   0);
      generate_sss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                   AMP,
                   &PHY_vars_eNB->lte_frame_parms,
                   (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 5 : 4,
                   0);
      generate_pss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                   AMP,
                   &PHY_vars_eNB->lte_frame_parms,
                   (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
                   10);
      generate_sss(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                   AMP,
                   &PHY_vars_eNB->lte_frame_parms,
                   (PHY_vars_eNB->lte_frame_parms.Ncp==0) ? 5 : 4,
                   10);
    }

    printf("Generating PBCH for mode1_flag = %d, frame_type %d\n", PHY_vars_eNB->lte_frame_parms.mode1_flag,PHY_vars_eNB->lte_frame_parms.frame_type);
    generate_pilots(PHY_vars_eNB,
                    PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                    AMP,
                    LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);
    generate_pbch(&PHY_vars_eNB->lte_eNB_pbch,
                  PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
                  AMP,
                  &PHY_vars_eNB->lte_frame_parms,
                  pbch_pdu,
                  0);
    /*
    generate_pbch(PHY_vars_eNB->lte_eNB_common_vars.txdataF[0],
      AMP,
      &PHY_vars_eNB->lte_frame_parms,
      pbch_pdu,
      3);
    */

    if (interf1>-20) {
      generate_pss(PHY_vars_eNB1->lte_eNB_common_vars.txdataF[0],
                   AMP,
                   &PHY_vars_eNB1->lte_frame_parms,
                   (PHY_vars_eNB1->lte_frame_parms.Ncp==0)?6:5,
                   0);
      generate_pilots(PHY_vars_eNB1,
                      PHY_vars_eNB1->lte_eNB_common_vars.txdataF[0],
                      AMP,
                      LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);
      generate_pbch(&PHY_vars_eNB1->lte_eNB_pbch,
                    PHY_vars_eNB1->lte_eNB_common_vars.txdataF[0],
                    AMP,
                    &PHY_vars_eNB1->lte_frame_parms,
                    pbch_pdu,
                    0);
    }

    if (interf2>-20) {
      generate_pss(PHY_vars_eNB2->lte_eNB_common_vars.txdataF[0],
                   AMP,
                   &PHY_vars_eNB2->lte_frame_parms,
                   (PHY_vars_eNB2->lte_frame_parms.Ncp==0)?6:5,
                   0);
      generate_pilots(PHY_vars_eNB2,
                      PHY_vars_eNB2->lte_eNB_common_vars.txdataF[0],
                      AMP,
                      LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);
      generate_pbch(&PHY_vars_eNB2->lte_eNB_pbch,
                    PHY_vars_eNB2->lte_eNB_common_vars.txdataF[0],
                    AMP,
                    &PHY_vars_eNB2->lte_frame_parms,
                    pbch_pdu,
                    0);
    }

    // Generate two PDCCH

    if (frame_type == 0) {
      ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu)->rah             = 0;
      ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu)->rballoc         = DLSCH_RB_ALLOC;
      ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu)->TPC             = 0;
      ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu)->harq_pid        = 0;
      ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu)->mcs             = 0;
      ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu)->ndi             = 1;
      ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu)->rv              = 0;
      dci_alloc[0].dci_length   = sizeof_DCI1_5MHz_FDD_t;
      memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu,sizeof(DCI1_5MHz_FDD_t));
      dci_alloc[0].L            = 2;
      dci_alloc[0].rnti         = n_rnti;
      dci_alloc[0].format       = format1;
      dci_alloc[0].search_space = DCI_UE_SPACE;
    } else {
      ((DCI1E_5MHz_2A_M10PRB_TDD_t *)&DLSCH_alloc_pdu)->rah             = 0;
      ((DCI1E_5MHz_2A_M10PRB_TDD_t *)&DLSCH_alloc_pdu)->rballoc         = DLSCH_RB_ALLOC;
      ((DCI1E_5MHz_2A_M10PRB_TDD_t *)&DLSCH_alloc_pdu)->TPC             = 0;
      ((DCI1E_5MHz_2A_M10PRB_TDD_t *)&DLSCH_alloc_pdu)->dai             = 0;
      ((DCI1E_5MHz_2A_M10PRB_TDD_t *)&DLSCH_alloc_pdu)->harq_pid        = 0;
      ((DCI1E_5MHz_2A_M10PRB_TDD_t *)&DLSCH_alloc_pdu)->mcs             = 0;
      ((DCI1E_5MHz_2A_M10PRB_TDD_t *)&DLSCH_alloc_pdu)->ndi             = 1;
      ((DCI1E_5MHz_2A_M10PRB_TDD_t *)&DLSCH_alloc_pdu)->rv              = 0;
      ((DCI1E_5MHz_2A_M10PRB_TDD_t *)&DLSCH_alloc_pdu)->tpmi            = (transmission_mode>=5 ? 5 : 0); // precoding
      ((DCI1E_5MHz_2A_M10PRB_TDD_t *)&DLSCH_alloc_pdu)->dl_power_off    = (transmission_mode==5 ? 0 : 1);
      dci_alloc[0].dci_length   = sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t;
      memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu,sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));
      dci_alloc[0].L            = 2;
      dci_alloc[0].rnti         = n_rnti;
      dci_alloc[0].format       = format1E_2A_M10PRB;
      dci_alloc[0].search_space = DCI_UE_SPACE;
    }

    num_pdcch_symbols = generate_dci_top(1,
                                         dci_alloc,
                                         0,
                                         AMP,
                                         &PHY_vars_eNB->lte_frame_parms,
                                         PHY_vars_eNB->lte_eNB_common_vars.txdataF[eNb_id],
                                         subframe);
    /*
    num_pdcch_symbols = generate_dci_top(1,
           dci_alloc,
           0,
           AMP,
           &PHY_vars_eNB->lte_frame_parms,
           PHY_vars_eNB->lte_eNB_common_vars.txdataF[eNb_id],
           5);
    */
    //  LOG_M("pilotsF.m","rsF",txdataF[0],lte_frame_parms->ofdm_symbol_size,1,1);
#ifdef IFFT_FPGA
    LOG_M("txsigF0.m","txsF0", PHY_vars_eNB->lte_eNB_common_vars.txdataF[eNb_id][0],300*120,1,4);

    if (PHY_vars_eNB->lte_frame_parms.nb_antennas_tx>1)
      LOG_M("txsigF1.m","txsF1", PHY_vars_eNB->lte_eNB_common_vars.txdataF[eNb_id][1],300*120,1,4);

    for (i=0; i<10; i++)
      debug_msg("%08x\n",((unsigned int *)&PHY_vars_eNB->lte_eNB_common_vars.txdataF[0][0][1*(PHY_vars_eNB->lte_frame_parms.N_RB_DL*12)*(PHY_vars_eNB->lte_frame_parms.symbols_per_tti>>1)])[i]);

    // do talbe lookup and write results to txdataF2
    for (aa=0; aa<PHY_vars_eNB->lte_frame_parms.nb_antennas_tx; aa++) {
      l = 0;

      for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX; i++)
        if ((i%512>=1) && (i%512<=150))
          txdataF2[aa][i] = ((int *)mod_table)[PHY_vars_eNB->lte_eNB_common_vars.txdataF[eNb_id][aa][l++]];
        else if (i%512>=362)
          txdataF2[aa][i] = ((int *)mod_table)[PHY_vars_eNB->lte_eNB_common_vars.txdataF[eNb_id][aa][l++]];
        else
          txdataF2[aa][i] = 0;

      //printf("l=%d\n",l);
    }

    LOG_M("txsigF20.m","txsF20", txdataF2[0],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

    if (PHY_vars_eNB->lte_frame_parms.nb_antennas_tx>1)
      LOG_M("txsigF21.m","txsF21", txdataF2[1],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

    tx_lev=0;

    for (aa=0; aa<PHY_vars_eNB->lte_frame_parms.nb_antennas_tx; aa++) {
      if (frame_parms->Ncp == 1)
        PHY_ofdm_mod(txdataF2[aa],        // input
                     txdata[aa],         // output
                     PHY_vars_eNB->lte_frame_parms.ofdm_symbol_size,                // log2_fft_size
                     LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,                 // number of symbols
                     PHY_vars_eNB->lte_frame_parms.nb_prefix_samples,               // number of prefix samples
                     PHY_vars_eNB->lte_frame_parms.twiddle_ifft,  // IFFT twiddle factors
                     PHY_vars_eNB->lte_frame_parms.rev,           // bit-reversal permutation
                     CYCLIC_PREFIX);
      else {
        normal_prefix_mod(txdataF2[aa],txdata[aa],LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,frame_parms);
      }

      tx_lev += signal_energy(&txdata[aa][0],
                              OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
    }

#else
    LOG_M("txsigF0.m","txsF0", PHY_vars_eNB->lte_eNB_common_vars.txdataF[eNb_id][0],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

    if (PHY_vars_eNB->lte_frame_parms.nb_antennas_tx>1)
      LOG_M("txsigF1.m","txsF1", PHY_vars_eNB->lte_eNB_common_vars.txdataF[eNb_id][1],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

    tx_lev = 0;

    for (aa=0; aa<PHY_vars_eNB->lte_frame_parms.nb_antennas_tx; aa++) {
      if (frame_parms->Ncp == 1)
        PHY_ofdm_mod(PHY_vars_eNB->lte_eNB_common_vars.txdataF[eNb_id][aa],        // input,
                     txdata[aa],         // output
                     frame_parms->ofdm_symbol_size,                // log2_fft_size
                     LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,                 // number of symbols
                     frame_parms->nb_prefix_samples,               // number of prefix samples
                     frame_parms->twiddle_ifft,  // IFFT twiddle factors
                     frame_parms->rev,           // bit-reversal permutation
                     CYCLIC_PREFIX);
      else {
        normal_prefix_mod(PHY_vars_eNB->lte_eNB_common_vars.txdataF[eNb_id][aa],
                          txdata[aa],
                          LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,
                          frame_parms);
      }

      tx_lev += signal_energy(&txdata[aa][0],
                              OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
    }

    if (interf1>-20) {
      for (aa=0; aa<PHY_vars_eNB1->lte_frame_parms.nb_antennas_tx; aa++) {
        if (frame_parms->Ncp == 1)
          PHY_ofdm_mod(PHY_vars_eNB1->lte_eNB_common_vars.txdataF[eNb_id][aa],        // input,
                       PHY_vars_eNB1->lte_eNB_common_vars.txdata[eNb_id][aa],         // output
                       frame_parms->ofdm_symbol_size,                // log2_fft_size
                       LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,                 // number of symbols
                       frame_parms->nb_prefix_samples,               // number of prefix samples
                       frame_parms->twiddle_ifft,  // IFFT twiddle factors
                       frame_parms->rev,           // bit-reversal permutation
                       CYCLIC_PREFIX);
        else {
          normal_prefix_mod(PHY_vars_eNB1->lte_eNB_common_vars.txdataF[eNb_id][aa],
                            PHY_vars_eNB1->lte_eNB_common_vars.txdata[eNb_id][aa],
                            LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,
                            frame_parms);
        }
      }
    }

    if (interf2>-20) {
      for (aa=0; aa<PHY_vars_eNB2->lte_frame_parms.nb_antennas_tx; aa++) {
        if (frame_parms->Ncp == 1)
          PHY_ofdm_mod(PHY_vars_eNB2->lte_eNB_common_vars.txdataF[eNb_id][aa],        // input,
                       PHY_vars_eNB2->lte_eNB_common_vars.txdata[eNb_id][aa],         // output
                       frame_parms->ofdm_symbol_size,                // log2_fft_size
                       LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,                 // number of symbols
                       frame_parms->nb_prefix_samples,               // number of prefix samples
                       frame_parms->twiddle_ifft,  // IFFT twiddle factors
                       frame_parms->rev,           // bit-reversal permutation
                       CYCLIC_PREFIX);
        else {
          normal_prefix_mod(PHY_vars_eNB2->lte_eNB_common_vars.txdataF[eNb_id][aa],
                            PHY_vars_eNB2->lte_eNB_common_vars.txdata[eNb_id][aa],
                            LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,
                            frame_parms);
        }
      }
    }

#endif
    LOG_M("txsig0.m","txs0", txdata[0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

    if (frame_parms->nb_antennas_tx>1)
      LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
  } else if ((oai_hw_input==0)&&(oai_hw_output==0)) { //read in from file
    i=0;

    while (!feof(input_fd)) {
      ret=fscanf(input_fd,"%49s %49s",input_val_str,input_val_str2);//&input_val1,&input_val2);
      /*
      if ((i%4)==0) {
      ((short*)txdata[0])[i/2] = (short)((1<<15)*strtod(input_val_str,NULL));
      ((short*)txdata[0])[(i/2)+1] = (short)((1<<15)*strtod(input_val_str2,NULL));
      if ((i/4)<100)
      printf("sample %d => %e + j%e (%d +j%d)\n",i/4,strtod(input_val_str,NULL),strtod(input_val_str2,NULL),((short*)txdata[0])[i/4],((short*)txdata[0])[(i/4)+1]);//1,input_val2,);
      }
      */
      ((short *)txdata[0])[i<<1] = 16*(short)(strtod(input_val_str,NULL));
      ((short *)txdata[0])[(i<<1)+1] = 16*(short)(strtod(input_val_str2,NULL));
      i++;

      if (i==(FRAME_LENGTH_COMPLEX_SAMPLES))
        break;
    }

    printf("Read in %d samples (%d)\n",i,FRAME_LENGTH_COMPLEX_SAMPLES);
    LOG_M("txsig0.m","txs0", txdata[0],10*frame_parms->samples_per_tti,1,1);
    //    LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
    tx_lev = signal_energy(&txdata[0][0],
                           OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
  } else { // get from OAI HW
    // set check if we have to set up a signal generator here
    printf("Doing Acquisition from OAI HW\n");
    snr0=snr1-.1;
  }

  foff = deltaF/(PHY_vars_UE[0]->lte_frame_parms.ofdm_symbol_size*15e3);

  for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES; i++) {
    cs = cos(2*M_PI*foff*i);
    sn = sin(2*M_PI*foff*i);

    for (aa=0; aa<PHY_vars_eNB->lte_frame_parms.nb_antennas_tx; aa++) {
      tmp_re =  (double)((short *)txdata[aa])[(i<<1)]*cs - (double)((short *)txdata[aa])[1+(i<<1)]*sn;
      tmp_im =  (double)((short *)txdata[aa])[1+(i<<1)]*cs + (double)((short *)txdata[aa])[(i<<1)]*sn;

      if (awgn_flag == 0) {
        s_re[aa][i] = tmp_re;
        s_im[aa][i] = tmp_im;

        if (interf1>-20) {
          s_re1[aa][i] = ((double)(((short *)txdata1[aa]))[(i<<1)]);
          s_im1[aa][i] = ((double)(((short *)txdata1[aa]))[(i<<1)+1]);
        }

        if (interf2>-20) {
          s_re2[aa][i] = ((double)(((short *)txdata2[aa]))[(i<<1)]);
          s_im2[aa][i] = ((double)(((short *)txdata2[aa]))[(i<<1)+1]);
        }
      } else {
        for (aarx=0; aarx<PHY_vars_UE[0]->lte_frame_parms.nb_antennas_rx; aarx++) {
          if (aa==0) {
            r_re[aarx][i] = tmp_re;//((double)(((short *)txdata[aa]))[(i<<1)]);
            r_im[aarx][i] = tmp_im;//((double)(((short *)txdata[aa]))[(i<<1)+1]);
          } else {
            r_re[aarx][i] += tmp_re;//((double)(((short *)txdata[aa]))[(i<<1)]);
            r_im[aarx][i] += tmp_im;//((double)(((short *)txdata[aa]))[(i<<1)+1]);
          }

          /*
             if (interf1>=-20) {
             r_re[aarx][i]+= pow(10.0,.05*interf1)*((double)(((short *)PHY_vars_eNB1->lte_eNB_common_vars.txdata[eNb_id][aa]))[(i<<1)]);
             r_im[aarx][i]+= pow(10.0,.05*interf1)*((double)(((short *)PHY_vars_eNB1->lte_eNB_common_vars.txdata[eNb_id][aa]))[(i<<1)+1]);

             }
             if (interf2>=-20) {
             r_re[aarx][i]+=pow(10.0,.05*interf2)*((double)(((short *)PHY_vars_eNB2->lte_eNB_common_vars.txdata[eNb_id][aa]))[(i<<1)]);
             r_im[aarx][i]+=pow(10.0,.05*interf2)*((double)(((short *)PHY_vars_eNB2->lte_eNB_common_vars.txdata[eNb_id][aa]))[(i<<1)+1]);
             }
          */
        }
      }
    }
  }

  if (oai_hw_output==0) {
    for (SNR=snr0; SNR<snr1; SNR+=.2) {
      n_errors = 0;
      n_errors2 = 0;
      n_alamouti = 0;

      for (trial=0; trial<n_frames; trial++) {
        if (oai_hw_input == 0) {
          if (awgn_flag == 0) {
            multipath_channel(eNB2UE,s_re,s_im,r_re,r_im,
                              FRAME_LENGTH_COMPLEX_SAMPLES,0);//LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0);

            if (interf1>-20)
              multipath_channel(eNB2UE1,s_re1,s_im1,r_re1,r_im1,
                                LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0);

            if (interf2>-20)
              multipath_channel(eNB2UE2,s_re2,s_im2,r_re2,r_im2,
                                LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0);
          } // awgn_flag

          sigma2_dB = 10*log10((double)tx_lev) +10*log10(PHY_vars_eNB->lte_frame_parms.ofdm_symbol_size/(12*NB_RB)) - SNR;

          if (n_frames==1)
            printf("sigma2_dB %f (SNR %f dB) tx_lev_dB %f\n",sigma2_dB,SNR,10*log10((double)tx_lev));

          //AWGN
          sigma2 = pow(10,sigma2_dB/10);
          /*
              if (n_frames==1) {
              printf("rx_level data symbol %f, tx_lev %f\n",
              10*log10(signal_energy_fp(r_re,r_im,1,OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0)),
              10*log10(tx_lev));
              }
          */
          iout = 0;//taus()%(FRAME_LENGTH_COMPLEX_SAMPLES>>2);

          for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES; i++) { //nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; i++) {
            for (aa=0; aa<PHY_vars_eNB->lte_frame_parms.nb_antennas_rx; aa++) {
              r_re[aa][i] += (pow(10.0,.05*interf1)*r_re1[aa][i] + pow(10.0,.05*interf2)*r_re2[aa][i]);
              r_im[aa][i] += (pow(10.0,.05*interf1)*r_im1[aa][i] + pow(10.0,.05*interf2)*r_im2[aa][i]);
              ((short *) PHY_vars_UE[0]->lte_ue_common_vars.rxdata[aa])[2*i] = (short) (.167*(r_re[aa][i] +sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
              ((short *) PHY_vars_UE[0]->lte_ue_common_vars.rxdata[aa])[(2*i)+1] = (short) (.167*(r_im[aa][i] + (iqim*r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
            }

            iout++;

            if (iout==FRAME_LENGTH_COMPLEX_SAMPLES)
              iout=0;
          }
        } else {
          fc=0;
          ioctl(openair_fd,openair_GET_BUFFER,(void *)&fc);
          //      sleep(1);
        }

        //  printf("Calling initial_sync\n");
        PHY_vars_UE[0]->rx_offset = 0;
        PHY_vars_UE[0]->lte_frame_parms.frame_type = 1;
        PHY_vars_UE[0]->lte_frame_parms.Ncp = 0;
        PHY_vars_UE[0]->lte_frame_parms.Nid_cell = 0;

        if (initial_sync(PHY_vars_UE[0],normal_txrx)==0) {
          //if (1) {
          printf("Synchronized to %s %s prefix Cell with id %d\n",
                 (PHY_vars_UE[0]->lte_frame_parms.frame_type == 0) ? "FDD\0" : "TDD\0",
                 (PHY_vars_UE[0]->lte_frame_parms.Ncp == 0) ? "Normal\0" : "Extended\0",
                 PHY_vars_UE[0]->lte_frame_parms.Nid_cell);

          if (subframe*PHY_vars_UE[0]->lte_frame_parms.samples_per_tti+PHY_vars_UE[0]->rx_offset>
              LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*PHY_vars_UE[0]->lte_frame_parms.samples_per_tti)
            rx_offset_mod = PHY_vars_UE[0]->rx_offset - LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*PHY_vars_UE[0]->lte_frame_parms.samples_per_tti;
          else
            rx_offset_mod = PHY_vars_UE[0]->rx_offset;

          // overwrite some values until source is sure
          PHY_vars_UE[0]->lte_frame_parms.N_RB_DL=N_RB_DL;
          PHY_vars_UE[0]->lte_frame_parms.phich_config_common.phich_duration=0;
          PHY_vars_UE[0]->lte_frame_parms.phich_config_common.phich_resource = oneSixth;
          generate_pcfich_reg_mapping(&PHY_vars_UE[0]->lte_frame_parms);
          generate_phich_reg_mapping(&PHY_vars_UE[0]->lte_frame_parms);

          if (N_carriers==2) {
            PHY_vars_UE[1]->lte_frame_parms = PHY_vars_UE[0]->lte_frame_parms;

            for (i=0; i<3; i++)
              lte_gold(&PHY_vars_UE[1]->lte_frame_parms,PHY_vars_UE[1]->lte_gold_table[i],i);

            generate_pcfich_reg_mapping(&PHY_vars_UE[1]->lte_frame_parms);
            generate_phich_reg_mapping(&PHY_vars_UE[1]->lte_frame_parms);
          }

          for (UE_idx=0; UE_idx<N_carriers; UE_idx++) { // loop over 2 carriers here
            // Do DCI
            for (l=0; l<(1+((PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?4:3)); l++) {
              slot_fep(PHY_vars_UE[UE_idx],
                       l,
                       subframe<<1,
                       rx_offset_mod,
                       0);
            }

            PHY_vars_UE[UE_idx]->lte_ue_pdcch_vars[0]->crnti = n_rnti;
            PHY_vars_UE[UE_idx]->transmission_mode[0] = transmission_mode;
            PHY_vars_UE[UE_idx]->UE_mode[0] = PUSCH;
            printf("Fine Frequency offset %d\n",PHY_vars_UE[UE_idx]->lte_ue_common_vars.freq_offset);
            printf("Doing PDCCH RX : num_pdcch_symbols at TX %d\n",num_pdcch_symbols);
            rx_pdcch(&PHY_vars_UE[UE_idx]->lte_ue_common_vars,
                     PHY_vars_UE[UE_idx]->lte_ue_pdcch_vars,
                     &PHY_vars_UE[UE_idx]->lte_frame_parms,
                     frame,
                     subframe,
                     0,
                     (PHY_vars_UE[UE_idx]->lte_frame_parms.mode1_flag == 1) ? SISO : ALAMOUTI,
                     PHY_vars_UE[UE_idx]->is_secondary_ue);
            printf("Got PCFICH for %d pdcch symbols\n",PHY_vars_UE[UE_idx]->lte_ue_pdcch_vars[0]->num_pdcch_symbols);
            dci_cnt = dci_decoding_procedure(PHY_vars_UE[UE_idx],
                                             dci_alloc_rx,
                                             0,
                                             0,
                                             subframe);
            printf("Found %d DCIs\n",dci_cnt);

            if (dci_cnt>0) {
              generate_ue_dlsch_params_from_dci(subframe,
                                                (void *)&dci_alloc_rx[0].dci_pdu,
                                                PHY_vars_UE[UE_idx]->lte_ue_pdcch_vars[0]->crnti,
                                                dci_alloc_rx[0].format,
                                                PHY_vars_UE[UE_idx]->dlsch_ue[0],
                                                &PHY_vars_UE[UE_idx]->lte_frame_parms,
                                                PHY_vars_UE[UE_idx]->pdsch_config_dedicated,
                                                SI_RNTI,
                                                0,
                                                P_RNTI);
              i_mod = get_Qm(PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->harq_processes[0]->mcs);
              /*
              // overwrite some values until source is sure
              PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->nb_rb = 25;
              PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->rb_alloc[0] = 0x1ffffff;
              PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->harq_processes[0]->Ndi = 1;
              PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->harq_processes[0]->mcs = 0;
              */
              dump_dci(&PHY_vars_UE[UE_idx]->lte_frame_parms, &dci_alloc_rx[0]);

              for (l=PHY_vars_UE[UE_idx]->lte_ue_pdcch_vars[0]->num_pdcch_symbols;
                   l<(((PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?4:3));
                   l++) {
                rx_pdsch(PHY_vars_UE[UE_idx],
                         PDSCH,
                         0,
                         1,
                         frame,
                         subframe,  // subframe,
                         l,  // symbol
                         (l==PHY_vars_UE[UE_idx]->lte_ue_pdcch_vars[0]->num_pdcch_symbols)?1:0,   // first_symbol_flag
                         dual_stream_flag,  // dual stream
                         i_mod);
              }

              for (l=1+(PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?4:3 ;
                   l<((PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?7:6);
                   l++) {
                slot_fep(PHY_vars_UE[UE_idx],
                         l,
                         subframe<<1,
                         rx_offset_mod,
                         0);
              }

              slot_fep(PHY_vars_UE[UE_idx],
                       0,
                       (subframe<<1)+1,
                       rx_offset_mod,
                       0);

              for (l=(PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?4:3 ;
                   l<((PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?7:6);
                   l++) {
                rx_pdsch(PHY_vars_UE[UE_idx],
                         PDSCH,
                         0,
                         1,
                         frame,
                         subframe,  // subframe,
                         l,  // symbol
                         0,   // first_symbol_flag
                         dual_stream_flag,  // dual stream
                         i_mod);
              }

              for (l=1;
                   l<1+((PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?4:3);
                   l++) {
                slot_fep(PHY_vars_UE[UE_idx],
                         l,
                         (subframe<<1)+1,   //slot 1
                         rx_offset_mod,
                         0);
              }

              for (l=(PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?7:6 ;
                   l<((PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?11:9);
                   l++) {
                rx_pdsch(PHY_vars_UE[UE_idx],
                         PDSCH,
                         0,
                         1,
                         frame,
                         subframe,  // subframe,
                         l,  // symbol
                         0,   // first_symbol_flag
                         dual_stream_flag,  // dual stream
                         i_mod);
              }

              for (l=1+(PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?4:3 ;
                   l<((PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?7:6);
                   l++) {
                slot_fep(PHY_vars_UE[UE_idx],
                         l,
                         (subframe<<1)+1,
                         rx_offset_mod,
                         0);
              }

              slot_fep(PHY_vars_UE[UE_idx],
                       0,
                       (subframe<<1)+2,   //slot 1
                       rx_offset_mod,
                       0);

              for (l=(PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?11:9 ;
                   l<((PHY_vars_UE[UE_idx]->lte_frame_parms.Ncp==0)?14:12);
                   l++) {
                rx_pdsch(PHY_vars_UE[UE_idx],
                         PDSCH,
                         0,
                         1,
                         frame,
                         subframe,  // subframe,
                         l,  // symbol
                         0,   // first_symbol_flag
                         dual_stream_flag,  // dual stream
                         i_mod);
              }

              coded_bits_per_codeword = get_G(&PHY_vars_UE[UE_idx]->lte_frame_parms,
                                              PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->nb_rb,
                                              PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->rb_alloc,
                                              get_Qm(PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->harq_processes[PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->current_harq_pid]->mcs),
                                              PHY_vars_UE[UE_idx]->lte_ue_pdcch_vars[0]->num_pdcch_symbols,
                                              subframe);
              printf("G %u, TBS %d, pdcch_sym %d\n",
                     coded_bits_per_codeword,
                     dlsch_tbs25[get_I_TBS(PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->harq_processes[0]->mcs)][PHY_vars_UE[UE_idx]->dlsch_ue[0][0]->nb_rb-1],
                     num_pdcch_symbols);
              dlsch_unscrambling(&PHY_vars_UE[UE_idx]->lte_frame_parms,
                                 PHY_vars_UE[UE_idx]->lte_ue_pdcch_vars[0]->num_pdcch_symbols,
                                 PHY_vars_UE[UE_idx]->dlsch_ue[0][0],
                                 coded_bits_per_codeword,
                                 PHY_vars_UE[UE_idx]->lte_ue_pdsch_vars[0]->llr[0],
                                 0,
                                 subframe<<1);
              ret = dlsch_decoding(PHY_vars_UE[UE_idx],
                                   PHY_vars_UE[UE_idx]->lte_ue_pdsch_vars[0]->llr[0],
                                   &PHY_vars_UE[UE_idx]->lte_frame_parms,
                                   PHY_vars_UE[UE_idx]->dlsch_ue[0][0],
                                   frame,
                                   subframe,
                                   0,
                                   PHY_vars_UE[UE_idx]->lte_ue_pdcch_vars[0]->num_pdcch_symbols);

              if (ret <= MAX_TURBO_ITERATIONS)
                printf("DLSCH decoded correctly!!!\n");
              else
                printf("DLSCH not decoded!\n");
            }
          } //UE_idx
        } //if sync
        else {
          if (PHY_vars_UE[0]->lte_frame_parms.Nid_cell !=  Nid_cell)
            n_errors2++;
          else
            n_errors++;
        }

#ifdef XFORMS

        if (do_forms==1) {
          do_forms2(form_dl,
                    frame_parms,
                    PHY_vars_UE[0]->lte_ue_pdcch_vars[0]->num_pdcch_symbols,
                    (int16_t **)PHY_vars_UE[0]->lte_ue_common_vars.dl_ch_estimates_time,
                    (int16_t **)PHY_vars_UE[0]->lte_ue_common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[0],
                    (int16_t **)PHY_vars_UE[0]->lte_ue_common_vars.rxdata,
                    (int16_t **)PHY_vars_UE[0]->lte_ue_common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF,
                    (int16_t *)PHY_vars_UE[0]->lte_ue_pdcch_vars[0]->rxdataF_comp[0],
                    (int16_t *)PHY_vars_UE[0]->lte_ue_pdsch_vars[0]->rxdataF_comp[0],
                    (int16_t *)PHY_vars_UE[0]->lte_ue_pdsch_vars[1]->rxdataF_comp[0],
                    (int16_t *)PHY_vars_UE[0]->lte_ue_pdsch_vars[0]->llr[0],
                    (int16_t *)PHY_vars_UE[0]->lte_ue_pbch_vars[0]->rxdataF_comp[0],
                    (int8_t *)PHY_vars_UE[0]->lte_ue_pbch_vars[0]->llr,
                    coded_bits_per_codeword);

          if (N_carriers==2)
            do_forms2(form_dl1,
                      frame_parms,
                      PHY_vars_UE[1]->lte_ue_pdcch_vars[0]->num_pdcch_symbols,
                      (int16_t **)PHY_vars_UE[1]->lte_ue_common_vars.dl_ch_estimates_time,
                      (int16_t **)PHY_vars_UE[1]->lte_ue_common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[0],
                      (int16_t **)PHY_vars_UE[1]->lte_ue_common_vars.rxdata,
                      (int16_t **)PHY_vars_UE[1]->lte_ue_common_vars.common_vars_rx_data_per_thread[subframe&0x1].rxdataF,
                      (int16_t *)PHY_vars_UE[1]->lte_ue_pdcch_vars[0]->rxdataF_comp[0],
                      (int16_t *)PHY_vars_UE[1]->lte_ue_pdsch_vars[0]->rxdataF_comp[0],
                      (int16_t *)PHY_vars_UE[1]->lte_ue_pdsch_vars[3]->rxdataF_comp[0],
                      (int16_t *)PHY_vars_UE[1]->lte_ue_pdsch_vars[0]->llr[0],
                      (int16_t *)PHY_vars_UE[1]->lte_ue_pbch_vars[0]->rxdataF_comp[0],
                      (int8_t *)PHY_vars_UE[1]->lte_ue_pbch_vars[0]->llr,
                      1920);
        }

#endif
      } // trials
    } // SNR

    if (n_frames==1) {
      LOG_M("H00.m","h00",&(PHY_vars_UE[0]->lte_ue_common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[0][0][0]),
            ((frame_parms->Ncp==0)?7:6)*(PHY_vars_eNB->lte_frame_parms.ofdm_symbol_size),1,1);

      if (n_tx==2)
        LOG_M("H10.m","h10",&(PHY_vars_UE[0]->lte_ue_common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[0][2][0]),
              ((frame_parms->Ncp==0)?7:6)*(PHY_vars_eNB->lte_frame_parms.ofdm_symbol_size),1,1);

      LOG_M("rxsig0.m","rxs0", PHY_vars_UE[0]->lte_ue_common_vars.rxdata[0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
      LOG_M("rxsigF0.m","rxsF0", PHY_vars_UE[0]->lte_ue_common_vars.rxdataF[0],NUMBER_OF_OFDM_CARRIERS*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,2,1);
      LOG_M("PBCH_rxF0_ext.m","pbch0_ext",PHY_vars_UE[0]->lte_ue_pbch_vars[0]->rxdataF_ext[0],12*4*6,1,1);
      LOG_M("PBCH_rxF0_comp.m","pbch0_comp",PHY_vars_UE[0]->lte_ue_pbch_vars[0]->rxdataF_comp[0],12*4*6,1,1);
      LOG_M("PBCH_rxF_llr.m","pbch_llr",PHY_vars_UE[0]->lte_ue_pbch_vars[0]->llr,(frame_parms->Ncp==0) ? 1920 : 1728,1,4);
      LOG_M("pdcch_rxF_ext0.m","pdcch_rxF_ext0",PHY_vars_UE[0]->lte_ue_pdcch_vars[eNb_id]->rxdataF_ext[0],3*300,1,1);
      LOG_M("pdcch_rxF_comp0.m","pdcch0_rxF_comp0",PHY_vars_UE[0]->lte_ue_pdcch_vars[eNb_id]->rxdataF_comp[0],4*300,1,1);
      LOG_M("pdcch_rxF_llr.m","pdcch_llr",PHY_vars_UE[0]->lte_ue_pdcch_vars[eNb_id]->llr,2400,1,4);
      coded_bits_per_codeword = get_G(&PHY_vars_UE[0]->lte_frame_parms,
                                      PHY_vars_UE[0]->dlsch_ue[0][0]->nb_rb,
                                      PHY_vars_UE[0]->dlsch_ue[0][0]->rb_alloc,
                                      get_Qm(PHY_vars_UE[0]->dlsch_ue[0][0]->harq_processes[0]->mcs),
                                      PHY_vars_UE[0]->lte_ue_pdcch_vars[0]->num_pdcch_symbols,
                                      0);
      dump_dlsch2(PHY_vars_UE[0],0,0,&coded_bits_per_codeword);
    }
  } else {
    printf("Sending frame to OAI HW\n");
    temp[0] = 110;
    temp[1] = 110;
    temp[2] = 110;
    temp[3] = 110;
    ioctl(openair_fd,openair_SET_TX_GAIN,(void *)&temp[0]);
    ioctl(openair_fd,openair_START_TX_SIG,(void *)NULL);
  }

#ifdef XFORMS

  if (do_forms==1) {
    fl_hide_form(form_dl->lte_scope);
    fl_free_form(form_dl->lte_scope);
  }

#endif
#ifdef IFFT_FPGA
  free(txdataF2[0]);
  free(txdataF2[1]);
  free(txdataF2);
  free(txdata[0]);
  free(txdata[1]);
  free(txdata);
#endif

  for (i=0; i<2; i++) {
    printf("Freeing s_re[%d]\n",i);
    free(s_re[i]);
    printf("Freeing s_im[%d]\n",i);
    free(s_im[i]);
    printf("Freeing r_re[%d]\n",i);
    free(r_re[i]);
    printf("Freeing r_im[%d]\n",i);
    free(r_im[i]);
  }

  printf("Freeing s_re\n");
  free(s_re);
  printf("Freeing s_im\n");
  free(s_im);
  printf("Freeing r_re\n");
  free(r_re);
  printf("Freeing r_im\n");
  free(r_im);
  lte_sync_time_free();

  if (write_output_file)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  if ((oai_hw_input==1)||
      (oai_hw_output==1)) {
    close(openair_fd);
  }

  return(n_errors);
}



/*
  for (i=1;i<4;i++)
    memcpy((void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[i*12*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX*2],
     (void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[0],
     12*OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX*2);
*/

