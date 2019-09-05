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
#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "LAYER2/MAC/vars.h"

#include "OCG_vars.h"
#include "UTIL/LOG/log_extern.h"

#include "unitary_defs.h"

int current_dlsch_cqi; //FIXME!

PHY_VARS_eNB *eNB;
PHY_VARS_UE *UE;

#define DLSCH_RB_ALLOC 0x1fbf // igore DC component,RB13

double cpuf;

int main(int argc, char **argv) {
  char c;
  int i,l,aa;
  double sigma2, sigma2_dB=0,SNR,snr0=-2.0,snr1=0.0;
  uint8_t snr1set=0;
  //mod_sym_t **txdataF;
  int **txdata;
  double s_re0[30720],s_re1[30720],s_im0[30720],s_im1[30720],r_re0[30720],r_im0[30720],r_re1[30720],r_im1[30720];
  double *s_re[2]= {s_re0,s_re1};
  double *s_im[2]= {s_im0,s_im1};
  double *r_re[2]= {r_re0,r_re1};
  double *r_im[2]= {r_im0,r_im1};
  double ricean_factor=0.0000005,iqim=0.0;
  int trial, n_trials, ntrials=1, n_errors;
  uint8_t transmission_mode = 1,n_tx=1,n_rx=1;
  unsigned char eNB_id = 0;
  uint16_t Nid_cell=0;
  int n_frames=1;
  channel_desc_t *UE2eNB;
  uint32_t nsymb,tx_lev;
  uint8_t extended_prefix_flag=0;
  LTE_DL_FRAME_PARMS *frame_parms;
  SCM_t channel_model=Rayleigh1_corr;
  //  double pucch_sinr;
  uint8_t osf=1,N_RB_DL=25;
  uint32_t pucch_tx=0,pucch1_missed=0,pucch1_false=0,pucch3_false=0,sig;
  PUCCH_FMT_t pucch_format = pucch_format1;
  PUCCH_CONFIG_DEDICATED pucch_config_dedicated;
  uint8_t subframe=0;
  uint8_t pucch_payload,pucch_payload_rx;
  uint8_t pucch3_payload_size=7;
  uint8_t pucch3_payload[21],pucch3_payload_rx[21];
  double tx_gain=1.0;
  int32_t stat;
  double stat_no_sig,stat_sig;
  uint8_t N0=40;
  uint8_t pucch1_thres=10;
  uint16_t n1_pucch = 0;
  uint16_t n2_pucch = 0;
  uint16_t n3_pucch = 20;
  uint16_t n_rnti=0x1234;
  number_of_cards = 1;
  cpuf = get_cpu_freq_GHz();

  while ((c = getopt (argc, argv, "har:pf:g:n:s:S:x:y:z:N:F:T:R:")) != -1) {
    switch (c) {
      case 'a':
        printf("Running AWGN simulation\n");
        channel_model = AWGN;
        ntrials=1;
        break;

      case 'f':
        if (atoi(optarg)==0)
          pucch_format = pucch_format1;
        else if (atoi(optarg)==1)
          pucch_format = pucch_format1a;
        else if (atoi(optarg)==2)
          pucch_format = pucch_format1b;
        else if (atoi(optarg)==6) // 3,4,5 is reserved for format2,2a,2b
          pucch_format = pucch_format3;
        else {
          printf("Unsupported pucch_format %d\n",atoi(optarg));
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

          default:
            printf("Unsupported channel model!\n");
            exit(-1);
        }

        break;

      case 'n':
        n_frames = atoi(optarg);
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

      case 'p':
        extended_prefix_flag=1;
        break;

      case 'r':
        ricean_factor = pow(10,-.1*atof(optarg));

        if (ricean_factor>1) {
          printf("Ricean factor must be between 0 and 1\n");
          exit(-1);
        }

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

        if ((n_rx==0) || (n_rx>2)) {
          printf("Unsupported number of rx antennas %d\n",n_rx);
          exit(-1);
        }

        break;

      case 'N':
        N0 = atoi(optarg);
        break;

      case 'T':
        pucch1_thres = atoi(optarg);
        break;

      case 'R':
        N_RB_DL = atoi(optarg);
        break;

      case 'O':
        osf = atoi(optarg);
        break;

      case 'F':
        break;

      default:
      case 'h':
        printf("%s -h(elp) -a(wgn on) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -r Ricean_FactordB -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -N CellId\n",
               argv[0]);
        printf("-h This message\n");
        printf("-a Use AWGN channel and not multipath\n");
        printf("-p Use extended prefix mode\n");
        printf("-n Number of frames to simulate\n");
        printf("-r Ricean factor (dB, 0 means Rayleigh, 100 is almost AWGN\n");
        printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
        printf("-S Ending SNR, runs from SNR0 to SNR1\n");
        printf("-t Delay spread for multipath channel\n");
        printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
        printf("-x Transmission mode (1,2,6 for the moment)\n");
        printf("-y Number of TX antennas used in eNB\n");
        printf("-z Number of RX antennas used in UE\n");
        printf("-i Relative strength of first intefering eNB (in dB) - cell_id mod 3 = 1\n");
        printf("-j Relative strength of second intefering eNB (in dB) - cell_id mod 3 = 2\n");
        printf("-N Noise variance in dB\n");
        printf("-R N_RB_DL\n");
        printf("-O oversampling factor (1,2,4,8,16)\n");
        printf("-f PUCCH format (0=1,1=1a,2=1b,6=3), formats 2/2a/2b not supported\n");
        printf("-F Input filename (.txt format) for RX conformance testing\n");
        exit (-1);
        break;
    }
  }

  logInit();
  //itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info, messages_definition_xml, NULL);
  g_log->log_component[PHY].level = LOG_DEBUG;
  g_log->log_component[PHY].flag = LOG_HIGH;

  if (transmission_mode==2)
    n_tx=2;

  lte_param_init(n_tx,
                 n_tx,
                 n_rx,
                 transmission_mode,
                 extended_prefix_flag,
                 FDD,
                 Nid_cell,
                 3,
                 N_RB_DL,
                 0,
                 osf,
                 0);

  if (snr1set==0) {
    if (n_frames==1)
      snr1 = snr0+.1;
    else
      snr1 = snr0+5.0;
  }

  printf("SNR0 %f, SNR1 %f\n",snr0,snr1);
  frame_parms = &eNB->frame_parms;
  txdata = eNB->common_vars.txdata[eNB_id];
  nsymb = (frame_parms->Ncp == 0) ? 14 : 12;
  printf("FFT Size %d, Extended Prefix %d, Samples per subframe %d, Symbols per subframe %d\n",NUMBER_OF_OFDM_CARRIERS,
         frame_parms->Ncp,frame_parms->samples_per_tti,nsymb);
  printf("[SIM] Using SCM/101\n");
  UE2eNB = new_channel_desc_scm(eNB->frame_parms.nb_antennas_tx,
                                UE->frame_parms.nb_antennas_rx,
                                channel_model,
                                N_RB2sampling_rate(eNB->frame_parms.N_RB_UL),
                                N_RB2channel_bandwidth(eNB->frame_parms.N_RB_UL),
                                0.0,
                                0,
                                0);

  if (UE2eNB==NULL) {
    printf("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  init_ncs_cell(&eNB->frame_parms,eNB->ncs_cell);
  init_ncs_cell(&UE->frame_parms,UE->ncs_cell);
  init_ul_hopping(&eNB->frame_parms);
  init_ul_hopping(&UE->frame_parms);
  eNB->frame_parms.pucch_config_common.deltaPUCCH_Shift = 2;
  eNB->frame_parms.pucch_config_common.nRB_CQI          = 4;
  eNB->frame_parms.pucch_config_common.nCS_AN           = 6;
  UE->frame_parms.pucch_config_common.deltaPUCCH_Shift = 2;
  UE->frame_parms.pucch_config_common.nRB_CQI          = 4;
  UE->frame_parms.pucch_config_common.nCS_AN           = 6;

  if( (pucch_format == pucch_format1) || (pucch_format == pucch_format1a) || (pucch_format == pucch_format1b) ) {
    pucch_payload = 0;
    generate_pucch1x(UE->common_vars.txdataF,
                     frame_parms,
                     UE->ncs_cell,
                     pucch_format,
                     &pucch_config_dedicated,
                     n1_pucch,
                     0, //shortened_format,
                     &pucch_payload,
                     AMP, //amp,
                     subframe); //subframe
  } else if( pucch_format == pucch_format3) {
    for(i=0; i<pucch3_payload_size; i++)
      pucch3_payload[i]=(uint8_t)(taus()&0x1);

    generate_pucch3x(UE->common_vars.txdataF,
                     frame_parms,
                     UE->ncs_cell,
                     pucch_format,
                     &pucch_config_dedicated,
                     n3_pucch,
                     0, //shortened_format,
                     pucch3_payload,
                     AMP, //amp,
                     subframe, //subframe
                     n_rnti);  //rnti
  }

  LOG_M("txsigF0.m","txsF0", &UE->common_vars.txdataF[0][2*subframe*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX],OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX*nsymb,1,1);
  tx_lev = 0;

  for (aa=0; aa<eNB->frame_parms.nb_antennas_tx; aa++) {
    if (frame_parms->Ncp == 1)
      PHY_ofdm_mod(&UE->common_vars.txdataF[aa][2*subframe*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX],        // input,
                   &txdata[aa][eNB->frame_parms.samples_per_tti*subframe],         // output
                   frame_parms->ofdm_symbol_size,
                   nsymb,                 // number of symbols
                   frame_parms->nb_prefix_samples,               // number of prefix samples
                   CYCLIC_PREFIX);
    else {
      normal_prefix_mod(&UE->common_vars.txdataF[eNB_id][subframe*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX],
                        &txdata[aa][eNB->frame_parms.samples_per_tti*subframe],
                        nsymb,
                        frame_parms);
      //apply_7_5_kHz(UE,UE->common_vars.txdata[aa],subframe<<1);
      //apply_7_5_kHz(UE,UE->common_vars.txdata[aa],1+(subframe<<1));
      apply_7_5_kHz(UE,&txdata[aa][eNB->frame_parms.samples_per_tti*subframe],0);
      apply_7_5_kHz(UE,&txdata[aa][eNB->frame_parms.samples_per_tti*subframe],1);
    }

    tx_lev += signal_energy(&txdata[aa][subframe*eNB->frame_parms.samples_per_tti],
                            OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
  }

  LOG_M("txsig0.m","txs0", txdata[0], FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
  //LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

  // multipath channel

  for (i=0; i<2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; i++) {
    for (aa=0; aa<eNB->frame_parms.nb_antennas_tx; aa++) {
      s_re[aa][i] = ((double)(((short *)&txdata[aa][subframe*frame_parms->samples_per_tti]))[(i<<1)]);
      s_im[aa][i] = ((double)(((short *)&txdata[aa][subframe*frame_parms->samples_per_tti]))[(i<<1)+1]);
    }
  }

  for (SNR=snr0; SNR<snr1; SNR+=.2) {
    printf("n_frames %d SNR %f\n",n_frames,SNR);
    n_errors = 0;
    pucch_tx = 0;
    pucch1_missed=0;
    pucch1_false=0;
    pucch3_false=0;
    stat_no_sig = 0;
    stat_sig = 0;

    for (trial=0; trial<n_frames; trial++) {
      multipath_channel(UE2eNB,s_re,s_im,r_re,r_im,
                        eNB->frame_parms.samples_per_tti,0);
      sigma2_dB = N0;//10*log10((double)tx_lev) - SNR;
      tx_gain = sqrt(pow(10.0,.1*(N0+SNR))/(double)tx_lev);

      if (n_frames==1)
        printf("sigma2_dB %f (SNR %f dB) tx_lev_dB %f,tx_gain %f (%f dB)\n",sigma2_dB,SNR,10*log10((double)tx_lev),tx_gain,20*log10(tx_gain));

      //AWGN
      sigma2 = pow(10,sigma2_dB/10);
      //  printf("Sigma2 %f (sigma2_dB %f)\n",sigma2,sigma2_dB);

      if (n_frames==1) {
        printf("rx_level data symbol %f, tx_lev %f\n",
               10*log10(signal_energy_fp(r_re,r_im,1,OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,0)),
               10*log10(tx_lev));
      }

      if (pucch_format != pucch_format1) {
        pucch_tx++;
        sig=1;
      } else {
        if (trial<(n_frames>>1)) {
          //printf("no sig =>");
          sig= 0;
        } else {
          sig=1;
          //printf("sig =>");
          pucch_tx++;
        }
      }

      //      sig = 1;
      for (n_trials=0; n_trials<ntrials; n_trials++) {
        //printf("n_trial %d\n",n_trials);
        // fill measurement symbol (19) with noise
        for (i=0; i<OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; i++) {
          for (aa=0; aa<eNB->frame_parms.nb_antennas_rx; aa++) {
            ((short *) &eNB->common_vars.rxdata[0][aa][(frame_parms->samples_per_tti<<1) -frame_parms->ofdm_symbol_size])[2*i] = (short) ((sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
            ((short *) &eNB->common_vars.rxdata[0][aa][(frame_parms->samples_per_tti<<1) -frame_parms->ofdm_symbol_size])[2*i+1] = (short) ((sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
          }
        }

        for (i=0; i<2*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; i++) {
          for (aa=0; aa<eNB->frame_parms.nb_antennas_rx; aa++) {
            if (n_trials==0) {
              //    r_re[aa][i] += (pow(10.0,.05*interf1)*r_re1[aa][i] + pow(10.0,.05*interf2)*r_re2[aa][i]);
              //    r_im[aa][i] += (pow(10.0,.05*interf1)*r_im1[aa][i] + pow(10.0,.05*interf2)*r_im2[aa][i]);
            }

            if (sig==1) {
              ((short *) &eNB->common_vars.rxdata[0][aa][subframe*frame_parms->samples_per_tti])[2*i] = (short) (((tx_gain*r_re[aa][i]) +sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
              ((short *) &eNB->common_vars.rxdata[0][aa][subframe*frame_parms->samples_per_tti])[2*i+1] = (short) (((tx_gain*r_im[aa][i]) + (iqim*r_re[aa][i]*tx_gain) + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
            } else {
              ((short *) &eNB->common_vars.rxdata[0][aa][subframe*frame_parms->samples_per_tti])[2*i] = (short) ((sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
              ((short *) &eNB->common_vars.rxdata[0][aa][subframe*frame_parms->samples_per_tti])[2*i+1] = (short) ((sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
            }
          }
        }

        remove_7_5_kHz(eNB,subframe<<1);
        remove_7_5_kHz(eNB,1+(subframe<<1));

        for (l=0; l<eNB->frame_parms.symbols_per_tti/2; l++) {
          slot_fep_ul(&eNB->frame_parms,
                      &eNB->common_vars,
                      l,
                      subframe*2,// slot
                      0,
                      0
                     );
          slot_fep_ul(&eNB->frame_parms,
                      &eNB->common_vars,
                      l,
                      1+(subframe*2),//slot
                      0,
                      0
                     );
        }

        //      if (sig == 1)
        //    printf("*");
        lte_eNB_I0_measurements(eNB,
                                subframe,
                                0,
                                1);
        eNB->measurements[0].n0_power_tot_dB = N0;//(int8_t)(sigma2_dB-10*log10(eNB->frame_parms.ofdm_symbol_size/(12*NB_RB)));
        stat = rx_pucch(eNB,
                        pucch_format,
                        0,
                        n1_pucch,
                        n2_pucch,
                        0, //shortened_format,
                        (pucch_format==pucch_format3) ? pucch3_payload_rx : &pucch_payload_rx, //payload,
                        0 /* frame not defined, let's pass 0 */,
                        subframe,
                        pucch1_thres);

        if (trial < (n_frames>>1)) {
          stat_no_sig += (2*(double)stat/n_frames);
          //    printf("stat (no_sig) %f\n",stat_no_sig);
        } else {
          stat_sig += (2*(double)stat/n_frames);
          //    printf("stat (sig) %f\n",stat_sig);
        }

        if (pucch_format==pucch_format1) {
          pucch1_missed = ((pucch_payload_rx == 0) && (sig==1)) ? (pucch1_missed+1) : pucch1_missed;
          pucch1_false  = ((pucch_payload_rx == 1) && (sig==0)) ? (pucch1_false+1) : pucch1_false;
          /*
          if ((pucch_payload_rx == 0) && (sig==1)) {
          printf("EXIT\n");
          exit(-1);
          }*/
        } else if( (pucch_format==pucch_format1a) || (pucch_format==pucch_format1b) ) {
          pucch1_false = (pucch_payload_rx != pucch_payload) ? (pucch1_false+1) : pucch1_false;
        } else if (pucch_format==pucch_format3) {
          for(i=0; i<pucch3_payload_size; i++) {
            if(pucch3_payload[i]!=pucch3_payload_rx[i]) {
              pucch3_false = (pucch3_false+1);
              break;
            }
          }
        }

        //      printf("sig %d\n",sig);
      } // NSR
    }

    if (pucch_format==pucch_format1)
      printf("pucch_trials %d : pucch1_false %d,pucch1_missed %d, N0 %d dB, stat_no_sig %f dB, stat_sig %f dB\n",pucch_tx,pucch1_false,pucch1_missed,eNB->measurements[0].n0_power_tot_dB,
             10*log10(stat_no_sig),10*log10(stat_sig));
    else if (pucch_format==pucch_format1a)
      printf("pucch_trials %d : pucch1a_errors %d\n",pucch_tx,pucch1_false);
    else if (pucch_format==pucch_format1b)
      printf("pucch_trials %d : pucch1b_errors %d\n",pucch_tx,pucch1_false);
    else if (pucch_format==pucch_format3)
      printf("pucch_trials %d : pucch3_errors %d\n",pucch_tx,pucch3_false);
  }

  if (n_frames==1) {
    //LOG_M("txsig0.m","txs0", &txdata[0][subframe*frame_parms->samples_per_tti],frame_parms->samples_per_tti,1,1);
    LOG_M("txsig0pucch.m", "txs0", &txdata[0][0], FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
    LOG_M("rxsig0.m","rxs0", &eNB->common_vars.rxdata[0][0][subframe*frame_parms->samples_per_tti],frame_parms->samples_per_tti,1,1);
    LOG_M("rxsigF0.m","rxsF0", &eNB->common_vars.rxdataF[0][0][0],512*nsymb*2,2,1);
  }

  lte_sync_time_free();
  return(n_errors);
}



/*
  for (i=1;i<4;i++)
    memcpy((void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[i*12*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX*2],
     (void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[0],
     12*OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX*2);
*/

