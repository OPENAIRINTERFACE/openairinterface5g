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

/**********************************************************************
*
* FILENAME    :  sss_nr.c
*
* MODULE      :  Functions for secundary synchronisation signal
*
* DESCRIPTION :  generation of sss
*                3GPP TS 38.211 7.4.2.3 Secondary synchronisation signal
*
************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "PHY/defs_nr_UE.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "executables/softmodem-common.h"
#include "PHY/NR_REFSIG/ss_pbch_nr.h"

#define DEFINE_VARIABLES_SSS_NR_H
#include "PHY/NR_REFSIG/sss_nr.h"
#undef DEFINE_VARIABLES_SSS_NR_H

/*******************************************************************
*
* NAME :         init_context_sss_nr
*
* PARAMETERS :   N_ID_2 : element 2 of physical layer cell identity
*                value : { 0, 1, 2 }
*
* RETURN :       generate binary sss sequence (this is a m-sequence)
*                d_sss is a third dimension array depending on
*                Cell identity elements:
*                - N_ID_1 : value from 0 to 335
*                - N_ID_2 : value from 0 to 2
*
* DESCRIPTION :  3GPP TS 38.211 7.4.2.3 Secundary synchronisation signal
*                Sequence generation
*
*********************************************************************/

#define INITIAL_SSS_NR    (7)

static const int16_t phase_re_nr[PHASE_HYPOTHESIS_NUMBER]
    // -pi/3 ---- pi/3
    = {16384, 20173, 23571, 26509, 28932, 30791, 32051, 32687, 32687, 32051, 30791, 28932, 26509, 23571, 20173, 16384};

static const int16_t phase_im_nr[PHASE_HYPOTHESIS_NUMBER] // -pi/3 ---- pi/3
    = {-28377, -25821, -22762, -19260, -15383, -11207, -6813, -2286, 2286, 6813, 11207, 15383, 19260, 22762, 25821, 28377};

static int16_t d_sss[N_ID_2_NUMBER][N_ID_1_NUMBER][LENGTH_SSS_NR];

void init_context_sss_nr(int amp)
{
  int16_t x0[LENGTH_SSS_NR];
  int16_t x1[LENGTH_SSS_NR];
  int16_t dss_current;
  int m0, m1;
  int nid_2_num = get_softmodem_params()->sl_mode == 0 ? N_ID_2_NUMBER : N_ID_2_NUMBER_SL;

  const int x0_initial[INITIAL_SSS_NR] = { 1, 0, 0, 0, 0, 0, 0 };
  const int x1_initial[INITIAL_SSS_NR] = { 1, 0, 0, 0, 0, 0, 0 };
  for (int i = 0; i < INITIAL_SSS_NR; i++) {
    x0[i] = x0_initial[i];
    x1[i] = x1_initial[i];
  }
  for (int i = 0; i < (LENGTH_SSS_NR - INITIAL_SSS_NR); i++) {
    x0[i + 7] = (x0[i + 4] + x0[i]) % (2);
    x1[i + 7] = (x1[i + 1] + x1[i]) % (2);
  }

  for (int N_ID_2 = 0; N_ID_2 < nid_2_num; N_ID_2++) {
    for (int N_ID_1 = 0; N_ID_1 < N_ID_1_NUMBER; N_ID_1++) {
      m0 = 15 * (N_ID_1 / 112) + (5 * N_ID_2);
      m1 = N_ID_1 % 112;
      for (int n = 0; n < LENGTH_SSS_NR; n++) {
        dss_current = (1 - 2 * x0 [(n + m0) % (LENGTH_SSS_NR)]) * (1 - 2 * x1[(n + m1) % (LENGTH_SSS_NR)]);
      /* Modulation of SSS is a BPSK TS 36.211 chapter 5.1.2 BPSK */
#if 1
        d_sss[N_ID_2][N_ID_1][n]   = dss_current;// * amp;
	(void) amp;
#else
        (void) amp;
        d_sss[N_ID_2][N_ID_1][n]   = (dss_current * SHRT_MAX)>>SCALING_PSS_NR;
#endif
      }
    }
  }

#if 0
  for (int i = 0; i < LENGTH_SSS_NR; i++) {
    printf("sss ref[%i] : %d %d \n", i, d_sss[0][0][i], d_sss[0][0][i]);
  }
#endif
}

/*******************************************************************
*
* NAME :         insert_sss_nr
*
* PARAMETERS :   pointer to input buffer for which sss in inserted
*                amp amplitude applied to input
*                frame parameters for cell identity
*
* RETURN :       none
*
* DESCRIPTION :  Allow to generate a reference sss sequence according to cell identity
*
*********************************************************************/

//#define DEBUG_SSS_NR
//#define DEBUG_PLOT_SSS
void insert_sss_nr(int16_t *sss_time,
                   NR_DL_FRAME_PARMS *frame_parms)
{
  const unsigned int ofdm_symbol_size = frame_parms->ofdm_symbol_size;
  int Nid2 = GET_NID2(frame_parms->Nid_cell);
  int Nid1 = GET_NID1(frame_parms->Nid_cell);

  /* call of IDFT should be done with ordered input as below
    *
    *                n input samples
    *  <------------------------------------------------>
    *  0                                                n
    *  are written into input buffer for IFFT in this order
    *   -------------------------------------------------
    *  |xxxxxxx                       N/2       xxxxxxxx|
    *  --------------------------------------------------
    *  ^      ^                 ^               ^          ^
    *  |      |                 |               |          |
    * n/2    end of            n=0            start of    n/2-1
    *         sss                               sss
    *
    *                   Frequencies
    *      positives                   negatives
    * 0                 (+N/2)(-N/2)
    * |-----------------------><-------------------------|
    *
    * sample 0 is for continuous frequency which is not used here
    */

  unsigned int k = ofdm_symbol_size - ((LENGTH_SSS_NR/2)+1);
  c16_t synchroF_tmp[2048] __attribute__((aligned(32)));
  c16_t synchro_tmp[2048] __attribute__((aligned(32)));
  bzero(synchroF_tmp, sizeof(synchroF_tmp));

  /* SSS is directly mapped to subcarrier */
  for (int i=0; i<LENGTH_SSS_NR; i++) {
    synchroF_tmp[k % ofdm_symbol_size].r = d_sss[Nid2][Nid1][i];
    k++;
  }

  /* get sss in the frequency domain by applying an inverse FFT */
  idft(IDFT_2048,
       (int16_t *)synchroF_tmp, /* complex input */
       (int16_t *)synchro_tmp, /* complex output */
       1); /* scaling factor */

  /* then get final sss in time */
  memcpy(sss_time, synchro_tmp, ofdm_symbol_size * sizeof(c16_t));
}

/*******************************************************************
*
* NAME :         pss_ch_est
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :  pss channel estimation
*
*********************************************************************/

static int pss_ch_est_nr(PHY_VARS_NR_UE *ue,
                         c16_t pss_ext[NB_ANTENNAS_RX][LENGTH_PSS_NR],
                         c16_t sss_ext[NB_ANTENNAS_RX][LENGTH_SSS_NR])
{
  int16_t *pss = get_primary_synchro_nr2(ue->common_vars.nid2);

  for (int aarx = 0; aarx < ue->frame_parms.nb_antennas_rx; aarx++) {
    c16_t *sss_ext2 = sss_ext[aarx];
    c16_t *pss_ext2 = pss_ext[aarx];
    for (int i = 0; i < LENGTH_PSS_NR; i++) {
      // This is H*(PSS) = R* \cdot PSS
      const int tmp_re = pss_ext2[i].r * pss[i];
      const int tmp_im = -pss_ext2[i].i * pss[i];
      
      const int32_t amp = tmp_re * tmp_re + tmp_im * tmp_im;
      const int shift = log2_approx(amp) / 2;
      // This is R(SSS) \cdot H*(PSS)
      const c16_t tmp = {(tmp_re * sss_ext2[i].r - tmp_im * sss_ext2[i].i) >> shift,
                         (tmp_re * sss_ext2[i].i + tmp_im * sss_ext2[i].r) >> shift};
      // printf("SSSi(%d,%d) : (%d,%d)\n",aarx,i,sss_ext2[i<<1],sss_ext2[1+(i<<1)]);
      // printf("SSSo(%d,%d) : (%d,%d)\n",aarx,i,tmp_re2,tmp_im2);
      // MRC on RX antennas
      if (aarx == 0) {
        sss_ext2[i].r = tmp.r;
        sss_ext2[i].i = tmp.i;
      } else {
        sss_ext2[i].r += tmp.r;
        sss_ext2[i].i += tmp.i;
      }
    }
  }
#if 0
  LOG_M("pssrx.m","pssrx",pss,LENGTH_PSS_NR,1,1);
  LOG_M("pss_ext.m","pssext",pss_ext2,LENGTH_PSS_NR,1,1);
  LOG_M("psschest.m","pssch",chest,LENGTH_PSS_NR,1,1);
#endif
#if 0

  for (int i = 0; i < LENGTH_PSS_NR; i++) {
    printf(" sss ext 2 [%d]  %d  %d at address %p\n", i, sss_ext2[2*i], sss_ext2[2*i+1]);
    printf(" sss ref   [%d]  %d  %d at address %p\n", i, d_sss[0][0][i], d_sss[0][0][i]);
    printf(" sss ext 3 [%d]  %d  %d at address %p\n", i, sss_ext3[2*i], sss_ext3[2*i+1]);
  }

#endif

  // sss_ext now contains the compensated SSS
  return(0);
}

/*******************************************************************
*
* NAME :         do_pss_sss_extract
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION : it allows extracting sss from samples buffer
*
*********************************************************************/

static int do_pss_sss_extract_nr(
    PHY_VARS_NR_UE *ue,
    const UE_nr_rxtx_proc_t *proc,
    c16_t pss_ext[NB_ANTENNAS_RX][LENGTH_PSS_NR],
    c16_t sss_ext[NB_ANTENNAS_RX][LENGTH_SSS_NR],
    uint8_t doPss,
    uint8_t doSss,
    uint8_t subframe,
    c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]) // add flag to indicate extracting only PSS, only SSS, or both
{
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;

  for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    int pss_symbol = 0;
    int sss_symbol = get_softmodem_params()->sl_mode == 0 ?
                     (SSS_SYMBOL_NB - PSS_SYMBOL_NB) :
                     (SSS0_SL_SYMBOL_NB - PSS0_SL_SYMBOL_NB) ;
    unsigned int ofdm_symbol_size = frame_parms->ofdm_symbol_size;

    c16_t *pss_rxF = rxdataF[aarx] + pss_symbol * ofdm_symbol_size;
    c16_t *sss_rxF = rxdataF[aarx] + sss_symbol * ofdm_symbol_size;

    c16_t *pss_rxF_ext = pss_ext[aarx];
    c16_t *sss_rxF_ext = sss_ext[aarx];

    unsigned int k = frame_parms->first_carrier_offset +
                     frame_parms->ssb_start_subcarrier +
                     ((get_softmodem_params()->sl_mode == 0) ?
                     PSS_SSS_SUB_CARRIER_START :
                     PSS_SSS_SUB_CARRIER_START_SL);

    if (k>= frame_parms->ofdm_symbol_size) k-=frame_parms->ofdm_symbol_size;

    for (int i=0; i < LENGTH_PSS_NR; i++) {
      if (doPss) {
        pss_rxF_ext[i] = pss_rxF[k];
      }

      if (doSss) {
        sss_rxF_ext[i] = sss_rxF[k];
      }

      k++;
      if (k == frame_parms->ofdm_symbol_size) k = 0;
    }
  }

#if 0

  int16_t *p = (int16_t *)sss_rxF_ext;

  for (int i = 0; i < LENGTH_PSS_NR; i++) {
    printf(" sss ext 2 [%d]  %d  %d at address %p\n", i, p[2*i], p[2*i+1], &p[2*i]);
    printf(" sss ref   [%d]  %d  %d at address %p\n", i, d_sss[0][0][i], d_sss[0][0][i], &d_sss[0][0][i]);
  }

#endif

#if 0

  int16_t *p2 = (int16_t *)pss_rxF_ext;

  for (int i = 0; i < LENGTH_PSS_NR; i++) {
    printf(" pss_rxF_ext [%d]  %d  %d at address %p\n", i, p2[2*i], p2[2*i+1], &p2[2*i]);
  }

#endif

  return(0);
}

/*******************************************************************
*
* NAME :         pss_sss_extract_nr
*
* PARAMETERS :   none
*
* RETURN :       none
*
* DESCRIPTION :
*
*********************************************************************/

static int pss_sss_extract_nr(PHY_VARS_NR_UE *phy_vars_ue,
                              const UE_nr_rxtx_proc_t *proc,
                              c16_t pss_ext[NB_ANTENNAS_RX][LENGTH_PSS_NR],
                              c16_t sss_ext[NB_ANTENNAS_RX][LENGTH_SSS_NR],
                              uint8_t subframe,
                              c16_t rxdataF[][phy_vars_ue->frame_parms.samples_per_slot_wCP])
{
  return do_pss_sss_extract_nr(phy_vars_ue, proc, pss_ext, sss_ext, 1 /* doPss */, 1 /* doSss */, subframe, rxdataF);
}

/*******************************************************************
 *
 * NAME :         rx_sss_nr
 *
 * PARAMETERS :   none
 *
 * RETURN :       Set Nid_cell in ue context, return true if cell detected
 *
 * DESCRIPTION :  Determine element Nid1 of cell identity
 *                so Nid_cell in ue context is set according to Nid1 & Nid2
 *
 *********************************************************************/
bool rx_sss_nr(PHY_VARS_NR_UE *ue,
               const UE_nr_rxtx_proc_t *proc,
               int32_t *tot_metric,
               uint8_t *phase_max,
               int *freq_offset_sss,
               c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP])
{
  uint8_t i;
  c16_t pss_ext[NB_ANTENNAS_RX][LENGTH_PSS_NR];
  c16_t sss_ext[NB_ANTENNAS_RX][LENGTH_SSS_NR];
  uint8_t Nid2 = GET_NID2(ue->common_vars.nid2);
  uint16_t Nid1;
  uint8_t phase;
  NR_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  int32_t metric, metric_re;
  int16_t *d;


  // pss sss extraction
  pss_sss_extract_nr(ue,
                     proc,
                     pss_ext,
                     sss_ext,
                     0,
                     rxdataF);          /* subframe */

#ifdef DEBUG_PLOT_SSS

  write_output("rxsig0.m","rxs0",&ue->common_vars.rxdata[0][0],ue->frame_parms.samples_per_subframe,1,1);
  write_output("rxdataF0_pss.m","rxF0_pss",&ue->common_vars.rxdataF[0][0],frame_parms->ofdm_symbol_size,1,1);
  write_output("rxdataF0_sss.m","rxF0_sss",&ue->common_vars.rxdataF[0][(SSS_SYMBOL_NB-PSS_SYMBOL_NB)*frame_parms->ofdm_symbol_size],frame_parms->ofdm_symbol_size,1,1);
  write_output("pss_ext.m","pss_ext",pss_ext,LENGTH_PSS_NR,1,1);

#endif

#if 0
  int16_t *p = (int16_t *)sss_ext[0];
  int16_t *p2 = (int16_t *)pss_ext[0];

  for (int i = 0; i < LENGTH_SSS_NR; i++) {
    printf("sss ref  [%i] : %d %d \n", i, d_sss[0][0][i], d_sss[0][0][i]);
    printf("sss ext  [%i] : %d %d \n", i, p[2*i], p[2*i+1]);

    printf("pss ref [%i] : %d %d \n", i, primary_synchro_nr2[0][2*i], primary_synchro_nr2[0][2*i+1]);
    printf("pss ext [%i] : %d %d \n", i, p2[2*i], p2[2*i+1]);
  }
#endif

  // get conjugated channel estimate from PSS, H* = R* \cdot PSS
  // and do channel estimation and compensation based on PSS

  pss_ch_est_nr(ue,
                pss_ext,
                sss_ext);

  // now do the SSS detection based on the precomputed sequences in PHY/LTE_TRANSPORT/sss.h
  *tot_metric = INT_MIN;

  c16_t *sss = sss_ext[0];

#ifdef DEBUG_PLOT_SSS

  write_output("sss_ext.m","sss_ext",sss_ext[0],LENGTH_SSS_NR,1,1);
  write_output("sss_ref.m","sss_ref", d_sss,LENGTH_SSS_NR,1,1);

#endif


#if 0

  /* simulate of a phase shift on the signal */

  int phase_shift_index = 0;

  phase_shift_samples(sss, LENGTH_SSS_NR, phase_re_nr[phase_shift_index], phase_im_nr[phase_shift_index]);

#endif

#if 0
        int16_t *ps = (int16_t *)pss_ext;

        for (int i = 0; i < LENGTH_SSS_NR; i++) {
          printf("sss ref  [%i] : %d \n", i, d_sss[0][0][i]);
          printf("sss ext  [%i] : %d %d \n", i, sss[i].r, sss[i].i);

          printf("pss ref [%i] : %d %d \n", i, primary_synchro_nr2[0][2*i], primary_synchro_nr2[0][2*i+1]);
          printf("pss ext [%i] : %d %d \n", i, ps[2*i], ps[2*i+1]);
        }
#endif

  /* for phase evaluation, one uses an array of possible phase shifts */
  /* then a correlation is done between received signal with a shift pĥase and the reference signal */
  /* Computation of signal with shift phase is based on below formula */
  /* cosinus cos(x + y) = cos(x)cos(y) - sin(x)sin(y) */
  /* sinus   sin(x + y) = sin(x)cos(y) + cos(x)sin(y) */

  uint16_t Nid1_start = 0;
  uint16_t Nid1_end = N_ID_1_NUMBER;
  if (ue->target_Nid_cell != -1) {
    Nid1_start = GET_NID1(ue->target_Nid_cell);
    Nid1_end = Nid1_start + 1;
  }

  for (Nid1 = Nid1_start; Nid1 < Nid1_end; Nid1++) { // all possible Nid1 values
    for (phase = 0; phase < PHASE_HYPOTHESIS_NUMBER; phase++) { // phase offset between PSS and SSS

      metric = 0;
      metric_re = 0;

      d = (int16_t *)&d_sss[Nid2][Nid1];

      // This is the inner product using one particular value of each unknown parameter
      for (i=0; i < LENGTH_SSS_NR; i++) {
        metric_re += d[i] * ((phase_re_nr[phase] * sss[i].r - phase_im_nr[phase] * sss[i].i) >> SCALING_METRIC_SSS_NR);
      }

      metric = metric_re;

      // if the current metric is better than the last save it
      if (metric > *tot_metric) {
        *tot_metric = metric;
        ue->frame_parms.Nid_cell = Nid2+(3*Nid1);
        *phase_max = phase;

#ifdef DEBUG_SSS_NR

	LOG_D(PHY,"(phase,Nid1) (%d,%d), metric_phase %d tot_metric %d, phase_max %d \n",phase, Nid1, metric, *tot_metric, *phase_max);

#endif
      }
    }
  }

//#ifdef DEBUG_SSS_NR
  
#define SSS_METRIC_FLOOR_NR   (30000)
  if (*tot_metric > SSS_METRIC_FLOOR_NR) {	
    Nid2 = GET_NID2(frame_parms->Nid_cell);
    Nid1 = GET_NID1(frame_parms->Nid_cell);
    LOG_D(PHY,"Nid2 %d Nid1 %d tot_metric %d, phase_max %d \n", Nid2, Nid1, *tot_metric, *phase_max);
  }
  //#endif

  if (Nid1==N_ID_1_NUMBER)
    return false;

  int re = 0;
  int im = 0;
  if (Nid1 == N_ID_1_NUMBER) {
    LOG_I(PHY,"Failed to detect SSS after PSS\n");
    return false;
  }
  d = (int16_t *)&d_sss[Nid2][Nid1];
  for(i = 0; i<LENGTH_SSS_NR; i++) {
    re += d[i] * sss[i].r;
    im += d[i] * sss[i].i;
  }
  double ffo_sss = atan2(im,re)/M_PI/4.3;
  *freq_offset_sss = (int)(ffo_sss*frame_parms->subcarrier_spacing);

  double ffo_pss = ((double)ue->common_vars.freq_offset)/frame_parms->subcarrier_spacing;
  LOG_W(NR_PHY,
        "ffo_pss %f (%i Hz), ffo_sss %f (%i Hz),  ffo_pss+ffo_sss %f (%i Hz), nid1: %d, nid2: %d\n",
        ffo_pss,
        (int)(ffo_pss * frame_parms->subcarrier_spacing),
        ffo_sss,
        *freq_offset_sss,
        ffo_pss + ffo_sss,
        (int)((ffo_pss + ffo_sss) * frame_parms->subcarrier_spacing),
        Nid1,
        Nid2);

  return true;
}
