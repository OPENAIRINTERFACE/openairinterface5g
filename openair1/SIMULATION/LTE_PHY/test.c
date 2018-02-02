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
#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"

//#define DEBUG_PHY
#define RF
#define IFFT_FPGA

#define BW 7.68

#define N_TRIALS 1

#define FRAME_LENGTH_COMPLEX_SAMPLES (lte_frame_parms->samples_per_tti>>1)
#define FRAME_LENGTH_BYTES (FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(int))

int main(int argc, char **argv)
{

  int i,l,aa,sector;
  double sigma2, sigma2_dB=0;
  mod_sym_t **txdataF;
#ifdef IFFT_FPGA
  int **txdataF2;
#endif
  int **txdata,**rxdata;
  double **s_re,**s_im,**r_re,**r_im;
  double amps[8] = {0.3868472 , 0.3094778 , 0.1547389 , 0.0773694 , 0.0386847 , 0.0193424 , 0.0096712 , 0.0038685};
  double aoa=.03,ricean_factor=1,Td=1.0;
  int channel_length;
  int amp;

  unsigned char pbch_pdu[6];
  int sync_pos, sync_pos_slot;
  FILE *rx_frame_file;
  int result;
  int freq_offset;
  int subframe_offset;
  char fname[40], vname[40];
  int trial, n_errors=0;
  unsigned int nb_rb = 25;
  unsigned int first_rb = 0;
  unsigned int eNb_id = 0;
  unsigned int slot_offset = 0;
  unsigned int sample_offset = 0;
  unsigned int channel_offset = 0;
  int n_frames;

  int slot=0,last_slot=0,next_slot=0;

  double nf[2] = {3.0,3.0}; //currently unused
  double ip =0.0;
  double N0W, path_loss, path_loss_dB, tx_pwr, rx_pwr;
  double rx_gain;
  int rx_pwr2, target_rx_pwr_dB;

  struct complex **ch;
  unsigned char first_call = 1;

  LTE_DL_FRAME_PARMS frame_parms;
  LTE_DL_FRAME_PARMS *lte_frame_parms = &frame_parms;

  if (argc==2)
    amp = atoi(argv[1]);
  else
    amp = 1024;

  // we normalize the tx power to 0dBm, assuming the amplitude of the signal is 1024
  // the SNR is this given by the difference of the path loss and the thermal noise (~-105dBm)
  // the rx_gain is adjusted automatically to achieve the target_rx_pwr_dB

  path_loss_dB = -90;
  path_loss    = pow(10,path_loss_dB/10);
  target_rx_pwr_dB = 60;

  lte_frame_parms->N_RB_DL            = 25;
  lte_frame_parms->N_RB_UL            = 25;
  lte_frame_parms->Ng_times6          = 1;
  lte_frame_parms->Ncp                = 1;
  lte_frame_parms->Nid_cell           = 0;
  lte_frame_parms->nushift            = 0;
  lte_frame_parms->nb_antennas_tx     = 2;
  lte_frame_parms->nb_antennas_rx     = 2;
  lte_frame_parms->first_dlsch_symbol = 4;
  lte_frame_parms->num_dlsch_symbols  = 6;
  lte_frame_parms->Csrs = 2;
  lte_frame_parms->Bsrs = 0;
  lte_frame_parms->kTC = 0;
  lte_frame_parms->n_RRC = 0;
  lte_frame_parms->mode1_flag = 1;
  lte_frame_parms->ofdm_symbol_size = 512;
  lte_frame_parms->log2_symbol_size = 9;
  lte_frame_parms->samples_per_tti = 7680;
  lte_frame_parms->first_carrier_offset = 362;
  lte_frame_parms->nb_prefix_samples>>=2;

#ifdef IFFT_FPGA
  txdata    = (int **)malloc16(2*sizeof(int*));
  txdata[0] = (int *)malloc16(FRAME_LENGTH_BYTES);
  txdata[1] = (int *)malloc16(FRAME_LENGTH_BYTES);

  bzero(txdata[0],FRAME_LENGTH_BYTES);
  bzero(txdata[1],FRAME_LENGTH_BYTES);

  rxdata    = (int **)malloc16(2*sizeof(int*));
  rxdata[0] = (int *)malloc16(2*FRAME_LENGTH_BYTES);
  rxdata[1] = (int *)malloc16(2*FRAME_LENGTH_BYTES);

  bzero(rxdata[0],2*FRAME_LENGTH_BYTES);
  bzero(rxdata[1],2*FRAME_LENGTH_BYTES);

  txdataF2    = (int **)malloc16(2*sizeof(int*));
  txdataF2[0] = (int *)malloc16(FRAME_LENGTH_BYTES);
  txdataF2[1] = (int *)malloc16(FRAME_LENGTH_BYTES);

  bzero(txdataF2[0],FRAME_LENGTH_BYTES);
  bzero(txdataF2[1],FRAME_LENGTH_BYTES);
#endif

  s_re = malloc(2*sizeof(double*));
  s_im = malloc(2*sizeof(double*));
  r_re = malloc(2*sizeof(double*));
  r_im = malloc(2*sizeof(double*));

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

  for (i=0; i<2; i++) {
    for (l=0; l<FRAME_LENGTH_COMPLEX_SAMPLES; l++) {
      ((short*) txdata[i])[2*l]   = amp * cos(M_PI/2*l);
      ((short*) txdata[i])[2*l+1] = amp * sin(M_PI/2*l);
    }
  }

  tx_pwr = signal_energy(txdata[0],lte_frame_parms->samples_per_tti>>1);
  printf("tx_pwr (DAC in) %d dB for slot %d (subframe %d)\n",dB_fixed(tx_pwr),next_slot,next_slot>>1);



  channel_length = (int) 11+2*BW*Td;

  ch = (struct complex**) malloc(4 * sizeof(struct complex*));

  for (i = 0; i<4; i++)
    ch[i] = (struct complex*) malloc(channel_length * sizeof(struct complex));

  randominit(0);
  set_taus_seed(0);

#ifdef RF
  tx_pwr = dac_fixed_gain(s_re,
                          s_im,
                          txdata,
                          lte_frame_parms->nb_antennas_tx,
                          lte_frame_parms->samples_per_tti>>1,
                          14,
                          18); //this should give 0dBm output level for input with amplitude 1024

  printf("tx_pwr (DAC out) %f dB for slot %d (subframe %d)\n",10*log10(tx_pwr),next_slot,next_slot>>1);
#else

  for (i=0; i<(lte_frame_parms->samples_per_tti>>1); i++) {
    for (aa=0; aa<lte_frame_parms->nb_antennas_tx; aa++) {
      s_re[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)]);
      s_im[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)+1]);
    }
  }

#endif

  //      printf("channel for slot %d (subframe %d)\n",next_slot,next_slot>>1);
  multipath_channel(ch,s_re,s_im,r_re,r_im,
                    amps,Td,BW,ricean_factor,aoa,
                    lte_frame_parms->nb_antennas_tx,
                    lte_frame_parms->nb_antennas_rx,
                    lte_frame_parms->samples_per_tti>>1,
                    channel_length,
                    0,
                    .9,
                    (first_call == 1) ? 1 : 0);

  if (first_call == 1)
    first_call = 0;

#ifdef RF

  //path_loss_dB = 0;
  //path_loss = 1;

  for (i=0; i<(lte_frame_parms->samples_per_tti>>1); i++) {
    for (aa=0; aa<lte_frame_parms->nb_antennas_rx; aa++) {
      r_re[aa][i]=r_re[aa][i]*sqrt(path_loss);
      r_im[aa][i]=r_im[aa][i]*sqrt(path_loss);

    }
  }

  rx_pwr = signal_energy_fp(r_re,r_im,lte_frame_parms->nb_antennas_rx,lte_frame_parms->samples_per_tti>>1,0);
  printf("rx_pwr (RF in) %f dB for slot %d (subframe %d)\n",10*log10(rx_pwr),next_slot,next_slot>>1);

  rx_gain = target_rx_pwr_dB - 10*log10(rx_pwr);

  // RF model
  rf_rx(r_re,
        r_im,
        NULL,
        NULL,
        0,
        lte_frame_parms->nb_antennas_rx,
        lte_frame_parms->samples_per_tti>>1,
        1.0/7.68e6 * 1e9,  // sampling time (ns)
        0.0,               // freq offset (Hz) (-20kHz..20kHz)
        0.0,               // drift (Hz) NOT YET IMPLEMENTED
        nf,                // noise_figure NOT YET IMPLEMENTED
        rx_gain-66.227,    // rx gain (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)
        200,               // IP3_dBm (dBm)
        &ip,               // initial phase
        30.0e3,            // pn_cutoff (kHz)
        -500.0,            // pn_amp (dBc) default: 50
        0.0,               // IQ imbalance (dB),
        0.0);              // IQ phase imbalance (rad)

  rx_pwr = signal_energy_fp(r_re,r_im,lte_frame_parms->nb_antennas_rx,lte_frame_parms->samples_per_tti>>1,0);

  printf("rx_pwr (ADC in) %f dB for slot %d (subframe %d)\n",10*log10(rx_pwr),next_slot,next_slot>>1);
#endif

#ifdef RF
  adc(r_re,
      r_im,
      0,
      slot_offset,
      rxdata,
      lte_frame_parms->nb_antennas_rx,
      lte_frame_parms->samples_per_tti>>1,
      12);

  rx_pwr2 = signal_energy(rxdata[0]+slot_offset,lte_frame_parms->samples_per_tti>>1);

  printf("rx_pwr (ADC out) %f dB (%d) for slot %d (subframe %d)\n",10*log10((double)rx_pwr2),rx_pwr2,next_slot,next_slot>>1);

#else

  for (i=0; i<(lte_frame_parms->samples_per_tti>>1); i++) {
    for (aa=0; aa<lte_frame_parms->nb_antennas_rx; aa++) {
      ((short*) rxdata[aa])[2*slot_offset + (2*i)]   = (short) ((r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0));
      ((short*) rxdata[aa])[2*slot_offset + (2*i)+1] = (short) ((r_im[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0));

    }
  }

#endif

  write_output("rxsig0.m","rxs0",rxdata[0],lte_frame_parms->samples_per_tti>>1,1,1);
  write_output("rxsig1.m","rxs1",rxdata[1],lte_frame_parms->samples_per_tti>>1,1,1);


#ifdef IFFT_FPGA
  free(txdataF2[0]);
  free(txdataF2[1]);
  free(txdataF2);
  free(txdata[0]);
  free(txdata[1]);
  free(txdata);
  free(rxdata[0]);
  free(rxdata[1]);
  free(rxdata);
#endif

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
