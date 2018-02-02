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

#include "PHY/defs.h"
//#define DEBUG_PHY

#if defined(__x86_64__) || defined(__i386__)
__m128i avg128F;
#elif defined(__arm__)
int32x4_t avg128F;
#endif

//compute average channel_level on each (TX,RX) antenna pair
int dl_channel_level(int16_t *dl_ch,
                     LTE_DL_FRAME_PARMS *frame_parms)
{

  int16_t rb;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch128;
#elif defined(__arm__)
  int16x4_t *dl_ch128;
#endif
  int avg;

  //clear average level
#if defined(__x86_64__) || defined(__i386__)
  avg128F = _mm_setzero_si128();
  dl_ch128=(__m128i *)dl_ch;

  for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

    avg128F = _mm_add_epi32(avg128F,_mm_madd_epi16(dl_ch128[0],dl_ch128[0]));
    avg128F = _mm_add_epi32(avg128F,_mm_madd_epi16(dl_ch128[1],dl_ch128[1]));
    avg128F = _mm_add_epi32(avg128F,_mm_madd_epi16(dl_ch128[2],dl_ch128[2]));

    dl_ch128+=3;

  }
#elif defined(__arm__)
  avg128F = vdupq_n_s32(0);
  dl_ch128=(int16x4_t *)dl_ch;

  for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

       avg128F = vqaddq_s32(avg128F,vmull_s16(dl_ch128[0],dl_ch128[0]));
       avg128F = vqaddq_s32(avg128F,vmull_s16(dl_ch128[1],dl_ch128[1]));
       avg128F = vqaddq_s32(avg128F,vmull_s16(dl_ch128[2],dl_ch128[2]));
       avg128F = vqaddq_s32(avg128F,vmull_s16(dl_ch128[3],dl_ch128[3]));
       avg128F = vqaddq_s32(avg128F,vmull_s16(dl_ch128[4],dl_ch128[4]));
       avg128F = vqaddq_s32(avg128F,vmull_s16(dl_ch128[5],dl_ch128[5]));
       dl_ch128+=6;


  }


#endif
  DevAssert( frame_parms->N_RB_DL );
  avg = (((int*)&avg128F)[0] +
         ((int*)&avg128F)[1] +
         ((int*)&avg128F)[2] +
         ((int*)&avg128F)[3])/(frame_parms->N_RB_DL*12);


#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
  return(avg);
}

int lte_est_freq_offset(int **dl_ch_estimates,
                        LTE_DL_FRAME_PARMS *frame_parms,
                        int l,
                        int* freq_offset,
			int reset)
{

  int ch_offset, omega, dl_ch_shift;
  struct complex16 omega_cpx;
  double phase_offset;
  int freq_offset_est;
  unsigned char aa;
  int16_t *dl_ch,*dl_ch_prev;
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

  //  for (aa=0;aa<frame_parms->nb_antennas_rx*frame_parms->nb_antennas_tx;aa++) {
  for (aa=0; aa<1; aa++) {

    dl_ch = (int16_t *)&dl_ch_estimates[aa][12+ch_offset];

    dl_ch_shift = 6+(log2_approx(dl_channel_level(dl_ch,frame_parms))/2);
    //    printf("dl_ch_shift: %d\n",dl_ch_shift);

    if (ch_offset == 0)
      dl_ch_prev = (int16_t *)&dl_ch_estimates[aa][12+(4-frame_parms->Ncp)*(frame_parms->ofdm_symbol_size)];
    else
      dl_ch_prev = (int16_t *)&dl_ch_estimates[aa][12+0];


    // calculate omega = angle(conj(dl_ch)*dl_ch_prev))
    //    printf("Computing freq_offset\n");
    omega = dot_product(dl_ch,dl_ch_prev,(frame_parms->N_RB_DL/2 - 1)*12,dl_ch_shift);
    //omega = dot_product(dl_ch,dl_ch_prev,frame_parms->ofdm_symbol_size,15);
    omega_cpx.r = ((struct complex16*) &omega)->r;
    omega_cpx.i = ((struct complex16*) &omega)->i;

    dl_ch = (int16_t *)&dl_ch_estimates[aa][(((frame_parms->N_RB_DL/2) + 1)*12) + ch_offset];

    if (ch_offset == 0)
      dl_ch_prev = (int16_t *)&dl_ch_estimates[aa][(((frame_parms->N_RB_DL/2) + 1)*12)+(4-frame_parms->Ncp)*(frame_parms->ofdm_symbol_size)];
    else
      dl_ch_prev = (int16_t *)&dl_ch_estimates[aa][((frame_parms->N_RB_DL/2) + 1)*12];

    // calculate omega = angle(conj(dl_ch)*dl_ch_prev))
    omega = dot_product(dl_ch,dl_ch_prev,((frame_parms->N_RB_DL/2) - 1)*12,dl_ch_shift);

    omega_cpx.r += ((struct complex16*) &omega)->r;
    omega_cpx.i += ((struct complex16*) &omega)->i;

    //    phase_offset += atan2((double)omega_cpx->i,(double)omega_cpx->r);
    phase_offset += atan2((double)omega_cpx.i,(double)omega_cpx.r);
    //    LOG_I(PHY,"omega (%d,%d) -> %f\n",omega_cpx->r,omega_cpx->i,phase_offset);
  }

  //  phase_offset /= (frame_parms->nb_antennas_rx*frame_parms->nb_antennas_tx);

  freq_offset_est = (int) (phase_offset/(2*M_PI)/(frame_parms->Ncp==NORMAL ? (285.8e-6):(2.5e-4))); //2.5e-4 is the time between pilot symbols
  //  LOG_I(PHY,"symbol %d : freq_offset_est %d\n",l,freq_offset_est);

  // update freq_offset with phase_offset using a moving average filter
  if (first_run == 1) {
    *freq_offset = freq_offset_est;
    first_run = 0;
  } else
    *freq_offset = ((freq_offset_est * coef) + (*freq_offset * ncoef)) >> 15;

  //#ifdef DEBUG_PHY
  //    msg("l=%d, phase_offset = %f (%d,%d), freq_offset_est = %d Hz, freq_offset_filt = %d \n",l,phase_offset,omega_cpx->r,omega_cpx->i,freq_offset_est,*freq_offset);
  /*
  for (i=0;i<150;i++)
    msg("i %d : %d,%d <=> %d,%d\n",i,dl_ch[2*i],dl_ch[(2*i)+1],dl_ch_prev[2*i],dl_ch_prev[(2*i)+1]);
  */
  //#endif

  return(0);
}
//******************************************************************************************************
// LTE MBSFN Frequency offset estimation

int lte_mbsfn_est_freq_offset(int **dl_ch_estimates,
                              LTE_DL_FRAME_PARMS *frame_parms,
                              int l,
                              int* freq_offset)
{

  int ch_offset, omega, dl_ch_shift;
  struct complex16 *omega_cpx;
  double phase_offset;
  int freq_offset_est;
  unsigned char aa;
  int16_t *dl_ch,*dl_ch_prev;
  static int first_run = 1;
  int coef = 1<<10;
  int ncoef =  32767 - coef;


  ch_offset = (l*(frame_parms->ofdm_symbol_size));

  if ((l!=2) && (l!=6) && (l!=10)) {
    LOG_D(PHY,"lte_est_freq_offset: l (%d) must be 2 or 6 or 10", l);
    return(-1);
  }

  phase_offset = 0.0;

  //  for (aa=0;aa<frame_parms->nb_antennas_rx*frame_parms->nb_antennas_tx;aa++) {
  for (aa=0; aa<1; aa++) {

    dl_ch = (int16_t *)&dl_ch_estimates[aa][12+ch_offset];

    dl_ch_shift = 4+(log2_approx(dl_channel_level(dl_ch,frame_parms))/2);
    //    printf("dl_ch_shift: %d\n",dl_ch_shift);

    if (ch_offset == 0)
      dl_ch_prev = (int16_t *)&dl_ch_estimates[aa][12+(10*(frame_parms->ofdm_symbol_size))];
    else
      dl_ch_prev = (int16_t *)&dl_ch_estimates[aa][12+6];

    //else if
    // dl_ch_prev = (int16_t *)&dl_ch_estimates[aa][12+0];

    // calculate omega = angle(conj(dl_ch)*dl_ch_prev))
    //    printf("Computing freq_offset\n");
    omega = dot_product(dl_ch,dl_ch_prev,(frame_parms->N_RB_DL/2 - 1)*12,dl_ch_shift);
    //omega = dot_product(dl_ch,dl_ch_prev,frame_parms->ofdm_symbol_size,15);
    omega_cpx = (struct complex16*) &omega;

    //    printf("omega (%d,%d)\n",omega_cpx->r,omega_cpx->i);

    dl_ch = (int16_t *)&dl_ch_estimates[aa][(((frame_parms->N_RB_DL/2) + 1)*12) + ch_offset];

    if (ch_offset == 0)
      dl_ch_prev = (int16_t *)&dl_ch_estimates[aa][(((frame_parms->N_RB_DL/2) + 1)*12)+10*(frame_parms->ofdm_symbol_size)];
    else
      dl_ch_prev = (int16_t *)&dl_ch_estimates[aa][((frame_parms->N_RB_DL/2) + 1)*12+6];

    // calculate omega = angle(conj(dl_ch)*dl_ch_prev))
    omega = dot_product(dl_ch,dl_ch_prev,((frame_parms->N_RB_DL/2) - 1)*12,dl_ch_shift);
    omega_cpx->r += ((struct complex16*) &omega)->r;
    omega_cpx->i += ((struct complex16*) &omega)->i;
    //    phase_offset += atan2((double)omega_cpx->i,(double)omega_cpx->r);
    phase_offset += atan2((double)omega_cpx->i,(double)omega_cpx->r);
    //    printf("omega (%d,%d) -> %f\n",omega_cpx->r,omega_cpx->i,phase_offset);
  }

  //  phase_offset /= (frame_parms->nb_antennas_rx*frame_parms->nb_antennas_tx);

  freq_offset_est = (int) (phase_offset/(2*M_PI)/2.5e-4); //2.5e-4 is the time between pilot symbols
  //  printf("symbol %d : freq_offset_est %d\n",l,freq_offset_est);

  // update freq_offset with phase_offset using a moving average filter
  if (first_run == 1) {
    *freq_offset = freq_offset_est;
    first_run = 0;
  } else
    *freq_offset = ((freq_offset_est * coef) + (*freq_offset * ncoef)) >> 15;

  //#ifdef DEBUG_PHY
  //    msg("l=%d, phase_offset = %f (%d,%d), freq_offset_est = %d Hz, freq_offset_filt = %d \n",l,phase_offset,omega_cpx->r,omega_cpx->i,freq_offset_est,*freq_offset);
  /*
  for (i=0;i<150;i++)
    msg("i %d : %d,%d <=> %d,%d\n",i,dl_ch[2*i],dl_ch[(2*i)+1],dl_ch_prev[2*i],dl_ch_prev[(2*i)+1]);
  */
  //#endif

  return(0);
}
