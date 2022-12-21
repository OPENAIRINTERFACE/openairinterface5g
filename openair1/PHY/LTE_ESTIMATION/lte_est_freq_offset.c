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

/*
   file: lte_est_freq_offset.c
   author (c): florian.kaltenberger@eurecom.fr
   date: 19.11.2009
*/

#include "PHY/defs_eNB.h"
//#define DEBUG_PHY

//compute average channel_level on each (TX,RX) antenna pair
int dl_channel_level(c16_t *dl_ch, LTE_DL_FRAME_PARMS *frame_parms)
{
  int16_t rb;
  simde__m128i *dl_ch128;
  int avg;

  //clear average level
  simde__m128i avg128F = simde_mm_setzero_si128();
  dl_ch128 = (simde__m128i *)dl_ch;

  for (rb=0; rb<frame_parms->N_RB_DL; rb++) {
    avg128F = simde_mm_add_epi32(avg128F, simde_mm_madd_epi16(dl_ch128[0], dl_ch128[0]));
    avg128F = simde_mm_add_epi32(avg128F, simde_mm_madd_epi16(dl_ch128[1], dl_ch128[1]));
    avg128F = simde_mm_add_epi32(avg128F, simde_mm_madd_epi16(dl_ch128[2], dl_ch128[2]));

    dl_ch128+=3;
  }
  DevAssert( frame_parms->N_RB_DL );
  avg = (((int*)&avg128F)[0] +
         ((int*)&avg128F)[1] +
         ((int*)&avg128F)[2] +
         ((int*)&avg128F)[3])/(frame_parms->N_RB_DL*12);

  return(avg);
}

int lte_est_freq_offset(int **dl_ch_estimates,
                        LTE_DL_FRAME_PARMS *frame_parms,
                        int l,
                        int* freq_offset,
			int reset)
{
  int ch_offset;
  double phase_offset;
  int freq_offset_est;
  static int first_run = 1;
  int coef = 1<<10;
  int ncoef =  32767 - coef;

  // initialize the averaging filter to initial value
  if (reset!=0)
    first_run=1;

  ch_offset = (l*(frame_parms->ofdm_symbol_size));

  if ((l!=0) && (l!=(4-frame_parms->Ncp))) {
    LOG_D(PHY,"lte_est_freq_offset: l (%d) must be 0 or %d\n",l,4-frame_parms->Ncp);
    return(-1);
  }

  phase_offset = 0.0;

  // Warning: only one antenna used
  for (int aa = 0; aa < 1; aa++) {
    c16_t *dl_ch = (c16_t *)&dl_ch_estimates[aa][12 + ch_offset];

    int dl_ch_shift = 6 + (log2_approx(dl_channel_level(dl_ch, frame_parms)) / 2);

    c16_t *dl_ch_prev;
    if (ch_offset == 0)
      dl_ch_prev = (c16_t *)&dl_ch_estimates[aa][12 + (4 - frame_parms->Ncp) * (frame_parms->ofdm_symbol_size)];
    else
      dl_ch_prev = (c16_t *)&dl_ch_estimates[aa][12 + 0];

    // calculate omega = angle(conj(dl_ch)*dl_ch_prev))
    //    printf("Computing freq_offset\n");
    c32_t omega_tmp = dot_product(dl_ch, dl_ch_prev, (frame_parms->N_RB_DL / 2 - 1) * 12, dl_ch_shift);
    cd_t omega_cpx = {omega_tmp.r, omega_tmp.i};
    // omega = dot_product(dl_ch,dl_ch_prev,frame_parms->ofdm_symbol_size,15);

    dl_ch = (c16_t *)&dl_ch_estimates[aa][(((frame_parms->N_RB_DL / 2) + 1) * 12) + ch_offset];

    if (ch_offset == 0)
      dl_ch_prev = (c16_t *)&dl_ch_estimates[aa][(((frame_parms->N_RB_DL / 2) + 1) * 12) + (4 - frame_parms->Ncp) * (frame_parms->ofdm_symbol_size)];
    else
      dl_ch_prev = (c16_t *)&dl_ch_estimates[aa][((frame_parms->N_RB_DL / 2) + 1) * 12];

    // calculate omega = angle(conj(dl_ch)*dl_ch_prev))
    c32_t omega = dot_product(dl_ch, dl_ch_prev, ((frame_parms->N_RB_DL / 2) - 1) * 12, dl_ch_shift);

    omega_cpx.r += omega.r;
    omega_cpx.i += omega.i;
    phase_offset += atan2(omega_cpx.i, omega_cpx.r);
  }

  freq_offset_est = (int) (phase_offset/(2*M_PI)/(frame_parms->Ncp==NORMAL ? (285.8e-6):(2.5e-4))); //2.5e-4 is the time between pilot symbols

  // update freq_offset with phase_offset using a moving average filter
  if (first_run == 1) {
    *freq_offset = freq_offset_est;
    first_run = 0;
  } else
    *freq_offset = ((freq_offset_est * coef) + (*freq_offset * ncoef)) >> 15;

  return(0);
}
//******************************************************************************************************
// LTE MBSFN Frequency offset estimation

int lte_mbsfn_est_freq_offset(int **dl_ch_estimates,
                              LTE_DL_FRAME_PARMS *frame_parms,
                              int l,
                              int* freq_offset)
{
  int ch_offset;
  double phase_offset;
  int freq_offset_est;
  static int first_run = 1;
  int coef = 1<<10;
  int ncoef =  32767 - coef;


  ch_offset = (l*(frame_parms->ofdm_symbol_size));

  if ((l!=2) && (l!=6) && (l!=10)) {
    LOG_D(PHY,"lte_est_freq_offset: l (%d) must be 2 or 6 or 10", l);
    return(-1);
  }

  phase_offset = 0.0;
  
  // Warning: only one antenna used
  for (int aa = 0; aa < 1; aa++) {
    c16_t *dl_ch_prev;
    c16_t *dl_ch = (c16_t *)&dl_ch_estimates[aa][12 + ch_offset];

    int dl_ch_shift = 4 + (log2_approx(dl_channel_level(dl_ch, frame_parms)) / 2);
    //    printf("dl_ch_shift: %d\n",dl_ch_shift);

    if (ch_offset == 0)
      dl_ch_prev = (c16_t *)&dl_ch_estimates[aa][12 + (10 * (frame_parms->ofdm_symbol_size))];
    else
      dl_ch_prev = (c16_t *)&dl_ch_estimates[aa][12 + 6];

    const c32_t omega_tmp = dot_product(dl_ch, dl_ch_prev, (frame_parms->N_RB_DL / 2 - 1) * 12, dl_ch_shift);
    cd_t omega_cpx = {omega_tmp.r, omega_tmp.i};

    dl_ch = (c16_t *)&dl_ch_estimates[aa][(((frame_parms->N_RB_DL / 2) + 1) * 12) + ch_offset];

    if (ch_offset == 0)
      dl_ch_prev = (c16_t *)&dl_ch_estimates[aa][(((frame_parms->N_RB_DL / 2) + 1) * 12) + 10 * (frame_parms->ofdm_symbol_size)];
    else
      dl_ch_prev = (c16_t *)&dl_ch_estimates[aa][((frame_parms->N_RB_DL / 2) + 1) * 12 + 6];

    // calculate omega = angle(conj(dl_ch)*dl_ch_prev))
    c32_t omega2 = dot_product(dl_ch, dl_ch_prev, ((frame_parms->N_RB_DL / 2) - 1) * 12, dl_ch_shift);
    omega_cpx.r += omega2.r;
    omega_cpx.i += omega2.i;
    phase_offset += atan2(omega_cpx.i, omega_cpx.r);
  }

  freq_offset_est = (int) (phase_offset/(2*M_PI)/2.5e-4); //2.5e-4 is the time between pilot symbols
  // update freq_offset with phase_offset using a moving average filter
  if (first_run == 1) {
    *freq_offset = freq_offset_est;
    first_run = 0;
  } else
    *freq_offset = ((freq_offset_est * coef) + (*freq_offset * ncoef)) >> 15;

  return(0);
}
