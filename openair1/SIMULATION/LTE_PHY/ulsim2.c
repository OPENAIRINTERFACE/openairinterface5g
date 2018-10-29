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
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "MAC_INTERFACE/vars.h"
#ifdef IFFT_FPGA
#include "PHY/LTE_REFSIG/mod_table.h"
#endif

#define BW 10.0
#define Td 1.0
#define N_TRIALS 1

int main(int argc, char **argv)
{

  int i,l,aa,sector;
  double sigma2, sigma2_dB=0;
  //mod_sym_t **txdataF;
#ifdef IFFT_FPGA
  int **txdataF2;
#endif
  int **txdata;
  double **s_re,**s_im,**r_re,**r_im;
  double amps[8] = {0.3868472 , 0.3094778 , 0.1547389 , 0.0773694 , 0.0386847 , 0.0193424 , 0.0096712 , 0.0038685};
  double aoa=.03,ricean_factor=1; //0.0000005;
  int channel_length;
  struct complex **ch;
  unsigned char pbch_pdu[6];
  int sync_pos, sync_pos_slot;
  FILE *rx_frame_file;
  int result;
  int freq_offset;
  int subframe_offset;
  char fname[40], vname[40];
  int trial, n_errors;
  unsigned int nb_rb = 25;
  unsigned int first_rb = 0;
  unsigned int eNb_id = 0;

  double nf[2] = {3.0,3.0}; //currently unused
  double ip =0.0;
  double N0W, path_loss, path_loss_dB;

  if (argc>1)
    sigma2_dB = atoi(argv[1]);


  channel_length = (int) 11+2*BW*Td;

  PHY_vars = malloc(sizeof(PHY_VARS));
  PHY_config = malloc(sizeof(PHY_CONFIG));
  mac_xface = malloc(sizeof(MAC_xface));

  lte_frame_parms = &(PHY_config->lte_frame_parms);
  lte_ue_common_vars = &(PHY_vars->lte_ue_common_vars);
  lte_ue_dlsch_vars = &(PHY_vars->lte_ue_dlsch_vars[0]);
  lte_ue_pbch_vars = &(PHY_vars->lte_ue_pbch_vars[0]);
  lte_ue_pdcch_vars = &(PHY_vars->lte_ue_pdcch_vars[0]);
  lte_eNB_common_vars = &(PHY_vars->lte_eNB_common_vars);
  lte_eNB_ulsch_vars = &(PHY_vars->lte_eNB_ulsch_vars[0]);

  lte_frame_parms->N_RB_DL            = 25;
  lte_frame_parms->N_RB_UL            = 25;
  lte_frame_parms->Ncp                = 1;
  lte_frame_parms->Nid_cell           = 0;
  lte_frame_parms->nushift            = 1;
  lte_frame_parms->nb_antennas_tx     = 2;
  lte_frame_parms->nb_antennas_rx     = 2;
  lte_frame_parms->first_dlsch_symbol = 1;
  lte_frame_parms->Csrs = 2;
  lte_frame_parms->Bsrs = 0;
  lte_frame_parms->kTC = 0;
  lte_frame_parms->n_RRC = 0;

  init_frame_parms(lte_frame_parms);

  copy_lte_parms_to_phy_framing(lte_frame_parms, &(PHY_config->PHY_framing));

  phy_init_top(NB_ANTENNAS_TX);

  lte_frame_parms->twiddle_fft      = twiddle_fft;
  lte_frame_parms->twiddle_ifft     = twiddle_ifft;
  lte_frame_parms->rev              = rev;

  /*
    rxdataF    = (int **)malloc16(2*sizeof(int*));
    rxdataF[0] = (int *)malloc16(FRAME_LENGTH_BYTES);
    rxdataF[1] = (int *)malloc16(FRAME_LENGTH_BYTES);

    rxdata    = (int **)malloc16(2*sizeof(int*));
    rxdata[0] = (int *)malloc16(FRAME_LENGTH_BYTES);
    rxdata[1] = (int *)malloc16(FRAME_LENGTH_BYTES);
  */

  lte_gold(lte_frame_parms);

  generate_ul_ref_sigs();
  generate_ul_ref_sigs_rx();

  phy_init_lte_ue(lte_frame_parms,lte_ue_common_vars,lte_ue_dlsch_vars,lte_ue_pbch_vars,lte_ue_pdcch_vars);

  /*
  txdataF    = (mod_sym_t **)malloc16(2*sizeof(mod_sym_t*));
  txdataF[0] = (mod_sym_t *)malloc16(FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(mod_sym_t));
  txdataF[1] = (mod_sym_t *)malloc16(FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(mod_sym_t));

  bzero(txdataF[0],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(mod_sym_t));
  bzero(txdataF[1],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(mod_sym_t));
  */

  phy_init_lte_eNB(lte_frame_parms,lte_eNB_common_vars,lte_eNB_ulsch_vars);

#ifdef IFFT_FPGA
  txdata    = (int **)malloc16(2*sizeof(int*));
  txdata[0] = (int *)malloc16(FRAME_LENGTH_BYTES);
  txdata[1] = (int *)malloc16(FRAME_LENGTH_BYTES);

  bzero(txdata[0],FRAME_LENGTH_BYTES);
  bzero(txdata[1],FRAME_LENGTH_BYTES);

  txdataF2    = (int **)malloc16(2*sizeof(int*));
  txdataF2[0] = (int *)malloc16(FRAME_LENGTH_BYTES_NO_PREFIX);
  txdataF2[1] = (int *)malloc16(FRAME_LENGTH_BYTES_NO_PREFIX);

  bzero(txdataF2[0],FRAME_LENGTH_BYTES_NO_PREFIX);
  bzero(txdataF2[1],FRAME_LENGTH_BYTES_NO_PREFIX);
#else
  txdata = lte_eNB_common_vars->txdata;
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

  ch = (struct complex**) malloc(4 * sizeof(struct complex*));

  for (i = 0; i<4; i++)
    ch[i] = (struct complex*) malloc(channel_length * sizeof(struct complex));

  generate_srs_tx(lte_frame_parms,lte_ue_common_vars->txdataF[0],AMP,0);
  generate_drs_puch(lte_frame_parms,lte_ue_common_vars->txdataF[0],AMP,0,first_rb,nb_rb);

#ifdef IFFT_FPGA
  LOG_M("txsigF0.m","txsF0", lte_ue_common_vars->txdataF[0],300*120,1,4);
  //LOG_M("txsigF1.m","txsF1", lte_ue_common_vars->txdataF[1],300*120,1,4);

  // do talbe lookup and write results to txdataF2
  for (aa=0; aa<lte_frame_parms->nb_antennas_tx; aa++) {
    l = 0;

    for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX; i++)
      if ((i%512>=1) && (i%512<=150))
        txdataF2[aa][i] = ((int*)mod_table)[lte_ue_common_vars->txdataF[aa][l++]];
      else if (i%512>=362)
        txdataF2[aa][i] = ((int*)mod_table)[lte_ue_common_vars->txdataF[aa][l++]];
      else
        txdataF2[aa][i] = 0;

    printf("l=%d\n",l);
  }

  LOG_M("txsigF20.m","txsF20", txdataF2[0],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);
  //LOG_M("txsigF21.m","txsF21", txdataF2[1],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

  for (aa=0; aa<lte_frame_parms->nb_antennas_tx; aa++)
    PHY_ofdm_mod(txdataF2[aa],        // input
                 txdata[aa],         // output
                 lte_frame_parms->log2_symbol_size,                // log2_fft_size
                 12*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME,                 // number of symbols
                 lte_frame_parms->nb_prefix_samples,               // number of prefix samples
                 lte_frame_parms->twiddle_ifft,  // IFFT twiddle factors
                 lte_frame_parms->rev,           // bit-reversal permutation
                 CYCLIC_PREFIX);

#else
  LOG_M("txsigF0.m","txsF0", lte_ue_common_vars->txdataF[0],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);
  //LOG_M("txsigF1.m","txsF1", lte_ue_common_vars->txdataF[1],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);

  for (aa=0; aa<lte_frame_parms->nb_antennas_tx; aa++) {
    PHY_ofdm_mod(lte_ue_common_vars->txdataF[aa],        // input
                 txdata[aa],         // output
                 lte_frame_parms->log2_symbol_size,                // log2_fft_size
                 12*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME,                 // number of symbols
                 lte_frame_parms->nb_prefix_samples,               // number of prefix samples
                 lte_frame_parms->twiddle_ifft,  // IFFT twiddle factors
                 lte_frame_parms->rev,           // bit-reversal permutation
                 CYCLIC_PREFIX);
  }

#endif


  LOG_M("txsig0.m","txs0", txdata[0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
  //LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

  // multipath channel
  randominit(0);

  for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES; i++) {
    for (aa=0; aa<lte_frame_parms->nb_antennas_tx; aa++) {
      s_re[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)]);
      s_im[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)+1]);
    }
  }

  n_errors = 0;

  for (trial=0; trial<N_TRIALS; trial++) {

    multipath_channel(ch,s_re,s_im,r_re,r_im,
                      amps,Td,BW,ricean_factor,aoa,
                      lte_frame_parms->nb_antennas_tx,
                      lte_frame_parms->nb_antennas_rx,
                      FRAME_LENGTH_COMPLEX_SAMPLES,
                      channel_length,
                      0);

    //LOG_M("channel0.m","chan0",ch[0],channel_length,1,8);

    // scale by path_loss = NOW - P_noise
    //sigma2       = pow(10,sigma2_dB/10);
    //N0W          = -95.87;
    //path_loss_dB = N0W - sigma2;
    //path_loss    = pow(10,path_loss_dB/10);
    path_loss_dB = 0;
    path_loss = 1;

    for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES; i++) {
      for (aa=0; aa<lte_frame_parms->nb_antennas_rx; aa++) {
        r_re[aa][i]=r_re[aa][i]*sqrt(path_loss);
        r_im[aa][i]=r_im[aa][i]*sqrt(path_loss);
      }
    }

    /*
    // RF model
    rf_rx(r_re,
    r_im,
    NULL,
    NULL,
    0,
    lte_frame_parms->nb_antennas_rx,
    FRAME_LENGTH_COMPLEX_SAMPLES,
    1.0/7.68e6 * 1e9,      // sampling time (ns)
    500,            // freq offset (Hz) (-20kHz..20kHz)
    0.0,            // drift (Hz) NOT YET IMPLEMENTED
    nf,             // noise_figure NOT YET IMPLEMENTED
    -path_loss_dB,            // rx_gain (dB)
    200,            // IP3_dBm (dBm)
    &ip,            // initial phase
    30.0e3,         // pn_cutoff (kHz)
    -500.0,          // pn_amp (dBc) default: 50
    0.0,           // IQ imbalance (dB),
    0.0);           // IQ phase imbalance (rad)

    */

    //AWGN
    sigma2 = pow(10,sigma2_dB/10);

    //printf("sigma2 = %g\n",sigma2);
    for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES; i++) {
      for (aa=0; aa<lte_frame_parms->nb_antennas_rx; aa++) {
        ((short*) lte_eNB_common_vars->rxdata[eNb_id][aa])[2*i] = (short) ((r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0));
        ((short*) lte_eNB_common_vars->rxdata[eNb_id][aa])[2*i+1] = (short) ((r_im[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0));
      }
    }

    LOG_M("rxsig0.m","rxs0", lte_eNB_common_vars->rxdata[eNb_id][0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
    LOG_M("rxsig1.m","rxs1", lte_eNB_common_vars->rxdata[eNb_id][1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);

    /*
    // optional: read rx_frame from file
    if ((rx_frame_file = fopen("rx_frame.dat","r")) == NULL)
      {
        printf("[openair][CHBCH_TEST][INFO] Cannot open rx_frame.m data file\n");
        exit(0);
      }

    result = fread((void *)PHY_vars->rx_vars[0].RX_DMA_BUFFER,4,FRAME_LENGTH_COMPLEX_SAMPLES,rx_frame_file);
    printf("Read %d bytes\n",result);
    result = fread((void *)PHY_vars->rx_vars[1].RX_DMA_BUFFER,4,FRAME_LENGTH_COMPLEX_SAMPLES,rx_frame_file);
    printf("Read %d bytes\n",result);

    fclose(rx_frame_file);
    */

    for (l=0; l<lte_frame_parms->symbols_per_tti; l++) {

      slot_fep_ul(lte_frame_parms,
                  lte_eNB_common_vars,
                  l%(lte_frame_parms->symbols_per_tti/2),
                  l/(lte_frame_parms->symbols_per_tti/2),
                  eNb_id,
                  0);
      /*
      ulsch_extract_rbs_single(lte_eNB_common_vars->rxdataF,
             lte_eNB_common_vars->rxdataF_ext,
             first_rb,
             nb_rb,
             l%(lte_frame_parms->symbols_per_tti/2),
             l/(lte_frame_parms->symbols_per_tti/2),
             lte_frame_parms);

      lte_ul_channel_estimation(lte_eNB_common_vars->drs_ch_estimates[eNb_id],
        lte_eNB_common_vars->rxdataF_ext,
        lte_frame_parms,
        l%(lte_frame_parms->symbols_per_tti/2),
        l/(lte_frame_parms->symbols_per_tti/2),
        nb_rb);
      */
    }

  }

  LOG_M("rxsigF0.m","rxsF0", lte_eNB_common_vars->rxdataF[0][0],512*12*2,2,1);
  LOG_M("rxsigF1.m","rxsF1", lte_eNB_common_vars->rxdataF[0][1],512*12*2,2,1);
  LOG_M("srs_seq.m","srs",lte_eNB_common_vars->srs,2*lte_frame_parms->ofdm_symbol_size,2,1);
  LOG_M("srs_est0.m","srsest0",lte_eNB_common_vars->srs_ch_estimates[0][0],512,1,1);
  LOG_M("srs_est1.m","srsest1",lte_eNB_common_vars->srs_ch_estimates[0][1],512,1,1);
  LOG_M("rxsigF0_ext.m","rxsF0_ext", lte_eNB_ulsch_vars[0]->rxdataF_ext[0][0],300*12*2,2,1);
  LOG_M("rxsigF1_ext.m","rxsF1_ext", lte_eNB_ulsch_vars[0]->rxdataF_ext[0][1],300*12*2,2,1);
  LOG_M("drs_est0.m","drsest0",lte_eNB_ulsch_vars[0]->drs_ch_estimates[0][0],300*12,1,1);
  LOG_M("drs_est1.m","drsest1",lte_eNB_ulsch_vars[0]->drs_ch_estimates[0][1],300*12,1,1);


#ifdef IFFT_FPGA
  free(txdataF2[0]);
  free(txdataF2[1]);
  free(txdataF2);
  free(txdata[0]);
  free(txdata[1]);
  free(txdata);
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

  lte_sync_time_free();

  return(n_errors);

}



/*
  for (i=1;i<4;i++)
    memcpy((void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[i*12*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX*2],
     (void *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[0],
     12*OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX*2);
*/

