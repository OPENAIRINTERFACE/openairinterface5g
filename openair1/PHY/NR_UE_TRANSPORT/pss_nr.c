/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/**********************************************************************
*
* FILENAME    :  pss_nr.c
*
* MODULE      :  synchronisation signal
*
* DESCRIPTION :  generation of pss
*                3GPP TS 38.211 7.4.2.2 Primary synchronisation signal
*
************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <nr-uesoftmodem.h>

#include "PHY/defs_nr_UE.h"

#include "PHY/NR_REFSIG/ss_pbch_nr.h"

#define DEFINE_VARIABLES_PSS_NR_H
#include "PHY/NR_REFSIG/pss_nr.h"
#undef DEFINE_VARIABLES_PSS_NR_H

#include "PHY/NR_REFSIG/sss_nr.h"
#include "PHY/NR_UE_TRANSPORT/cic_filter_nr.h"

// #define DBG_PSS_NR

/*******************************************************************
*
* NAME :         generate_pss_nr
*
* PARAMETERS :   N_ID_2 : element 2 of physical layer cell identity
*                value : { 0, 1, 2}
*
* RETURN :       generate binary pss sequence (this is a m-sequence)
*
* DESCRIPTION :  3GPP TS 38.211 7.4.2.2 Primary synchronisation signal
*                Sequence generation
*
*********************************************************************/

void generate_pss_nr(const int N_ID_2, int16_t *pss)
{
  AssertFatal(N_ID_2 >= 0 && N_ID_2 < NUMBER_PSS_SEQUENCE, "Illegal N_ID_2 %d\n", N_ID_2);
  int16_t x[LENGTH_PSS_NR];
#define INITIAL_PSS_NR (7)
  const int16_t x_initial[INITIAL_PSS_NR] = {0, 1, 1, 0, 1, 1, 1};
  memcpy(x, x_initial, sizeof(x_initial));

  for (int i = 0; i < (LENGTH_PSS_NR - INITIAL_PSS_NR); i++)
    x[i + INITIAL_PSS_NR] = (x[i + 4] + x[i]) % 2;
  for (int n=0; n < LENGTH_PSS_NR; n++) {
    const int m = (n + 43 * N_ID_2) % (LENGTH_PSS_NR);
    pss[n] = 1 - 2 * x[m];
  }
}

  /* call of IDFT should be done with ordered input as below
  *
  *                n input samples
  *  <------------------------------------------------>
  *  0                                                n
  *  are written into input buffer for IFFT
  *   -------------------------------------------------
  *  |xxxxxxx                       N/2       xxxxxxxx|
  *  --------------------------------------------------
  *  ^      ^                 ^               ^          ^
  *  |      |                 |               |          |
  * n/2    end of            n=0            start of    n/2-1
  *         pss                               pss
  *
  *                   Frequencies
  *      positives                   negatives
  * 0                 (+N/2)(-N/2)
  * |-----------------------><-------------------------|
  *
  * sample 0 is for continuous frequency which is used here
  */
void generate_pss_nr_time(int ofdm_symbol_size,
                          int first_carrier_offset,
                          const int N_ID_2,
                          int ssbFirstSCS,
                          c16_t pssTime[ofdm_symbol_size])
{
  unsigned int subcarrier_start = get_softmodem_params()->sl_mode == 0 ? PSS_SSS_SUB_CARRIER_START : PSS_SSS_SUB_CARRIER_START_SL;
  c16_t synchroF_tmp[ofdm_symbol_size] __attribute__((aligned(32)));
  memset(synchroF_tmp, 0, sizeof(synchroF_tmp));
  unsigned int k = CIRCULAR_INC(first_carrier_offset, ssbFirstSCS + subcarrier_start, ofdm_symbol_size);
  int16_t pss[LENGTH_PSS_NR];
  generate_pss_nr(N_ID_2, pss);
  for (int i = 0; i < LENGTH_PSS_NR; i++) {
    synchroF_tmp[k] = (c16_t){.r = pss[i] * ((1U << SCALING_PSS_NR) - 1)};
    k = CIRCULAR_INC(k, 1, ofdm_symbol_size);
  }

  /* IFFT will give temporal signal of Pss */
  idft((int16_t)get_idft(ofdm_symbol_size),
       (int16_t *)synchroF_tmp, /* complex input but legacy type is wrong*/
       (int16_t *)pssTime, /* complex output */
       1); /* scaling factor */
}

static int cmp_pss_peak(const void *a, const void *b)
{
  const pss_detection_result_t *pa = (const pss_detection_result_t *)a;
  const pss_detection_result_t *pb = (const pss_detection_result_t *)b;
  if (pa->peak < pb->peak)
    return 1;
  if (pa->peak > pb->peak)
    return -1;
  return 0;
}

/*******************************************************************
*
* NAME :         pss_search_time_nr
*
* PARAMETERS :   received buffer in time domain
*                frame parameters
*
* RETURN :       position of detected pss
*
* DESCRIPTION :  Synchronisation on pss sequence is based on a time domain correlation between received samples and pss sequence
*                A maximum likelihood detector finds the timing offset (position) that corresponds to the maximum correlation
*                Length of received buffer should be a minimum of 2 frames (see TS 38.213 4.1 Cell search)
*                Search pss in the received buffer is done each 4 samples which ensures a memory alignment to 128 bits (32 bits x 4).
*                This is required by SIMD (single instruction Multiple Data) Extensions of Intel processors
*                Correlation computation is based on a a dot product which is realized thank to SIMS extensions
*
*                                    (x frames)
*     <--------------------------------------------------------------------------->
*
*
*     -----------------------------------------------------------------------------
*     |                      Received UE data buffer                              |
*     ----------------------------------------------------------------------------
*                -------------
*     <--------->|    pss    |
*      position  -------------
*                ^
*                |
*            peak position
*            given by maximum of correlation result
*            position matches beginning of first ofdm symbol of pss sequence
*
*     Remark: memory position should be aligned on a multiple of 4 due to I & Q samples of int16
*             An OFDM symbol is composed of x number of received samples depending of Rf front end sample rate.
*
*     I & Q storage in memory
*
*             First samples       Second  samples
*     ------------------------- -------------------------  ...
*     |     I1     |     Q1    |     I2     |     Q2    |
*     ---------------------------------------------------  ...
*     ^    16  bits   16 bits  ^
*     |                        |
*     ---------------------------------------------------  ...
*     |         sample 1       |    sample   2          |
*    ----------------------------------------------------  ...
*     ^
*
*********************************************************************/

nr_pss_info_t pss_search_time_nr(const pss_search_t *p)
{
  if (p->rxdata_length == 0) {
    LOG_E(PHY, "inconsistent call to pss_search_time_nr %d\n", p->rxdata_length);
    return (nr_pss_info_t){0};
  }

  c16_t(*pssTime)[p->ofdm_symbol_size] = (c16_t(*)[p->ofdm_symbol_size])p->pssTime;
  int max_size = get_softmodem_params()->sl_mode == 0 ? NUMBER_PSS_SEQUENCE : NUMBER_PSS_SEQUENCE_SL;

  /* Search pss in the received buffer each 4 samples which ensures a memory alignment on 128 bits (32 bits x 4 ) */
  /* This is required by SIMD (single instruction Multiple Data) Extensions of Intel processors. */
  /* Correlation computation is based on a dot product which is realized thank to SIMS extensions */
  int pss_index_start;
  int pss_index_end;
  if (p->target_Nid_cell != -1) {
    pss_index_start = p->target_Nid_cell % NUMBER_PSS_SEQUENCE;
    pss_index_end = pss_index_start + 1;
  } else {
    pss_index_start = 0;
    pss_index_end = max_size;
  }

  int pss_count = 0;
  nr_pss_info_t pss_info = {0};
  for (int pss_index = pss_index_start; pss_index < pss_index_end; pss_index++) {
    int64_t peak_value = 0;
    int peak_position = 0;
    int64_t avg = 0;
    for (int n = 0; n < p->rxdata_length; n += 4) { //
      int64_t pss_corr_ue = 0;
      /* calculate dot product of primary_synchro_time_nr and rxdata[ar][n]
       * (ar=0..nb_ant_rx) and store the sum in temp[n]; */
      for (int ar = 0; ar < p->nb_antennas_rx; ar++) {
        /* perform correlation of rx data and pss sequence ie it is a dot product */
        const c32_t result = dot_product(pssTime[pss_index], &p->rxdata[ar][n], p->ofdm_symbol_size, SCALING_PSS_NR);
        const c64_t r64 = {.r = result.r, .i = result.i};
        pss_corr_ue += squaredMod(r64);
      }

      /* calculate the absolute value of sync_corr[n] */
      avg += pss_corr_ue;
      if (pss_corr_ue > peak_value) {
        peak_value = pss_corr_ue;
        peak_position = n;

#ifdef DEBUG_PSS_NR
        LOG_I(NR_PHY, "pss_index %d: n %6u peak_value %li\n", pss_index, n, pss_corr_ue);
#endif
      }
    }

    avg /= (p->rxdata_length / 4);
    bool pss_not_found = (p->target_Nid_cell == -1 && peak_value < 5 * avg) 
                         || peak_position < p->nb_prefix_samples;
    pss_info.pss_elem_info[pss_count] = (pss_detection_result_t){
        .nid2 = pss_index,
        .peak = dB_fixed64(peak_value),
        .pos = peak_position,
        .avg = dB_fixed64(avg),
        .success = !pss_not_found,
    };

    double ffo_est = 0;
    if (p->fo_flag && pss_info.pss_elem_info[pss_count].success) {
      // fractional frequency offset computation according to Cross-correlation Synchronization Algorithm Using PSS
      // Shoujun Huang, Yongtao Su, Ying He and Shan Tang, "Joint time and frequency offset estimation in LTE downlink," 7th
      // International Conference on Communications and Networking in China, 2012.

      // Computing cross-correlation at peak on half the symbol size for first half of data
      const c16_t *rx_peak = &p->rxdata[0][peak_position];
      c32_t r1 = dot_product(pssTime[pss_index],
                             rx_peak,
                             p->ofdm_symbol_size >> 1,
                             SCALING_PSS_NR);
      // Computing cross-correlation at peak on half the symbol size for data shifted by half symbol size
      // as it is real and complex it is necessary to shift by a value equal to symbol size to obtain such shift
      c32_t r2 =
          dot_product(pssTime[pss_index] + (p->ofdm_symbol_size >> 1),
                      rx_peak + (p->ofdm_symbol_size >> 1),
                      p->ofdm_symbol_size >> 1,
                      SCALING_PSS_NR);
      cd_t r1d = {r1.r, r1.i}, r2d = {r2.r, r2.i};
      // estimation of fractional frequency offset: angle[(result1)'*(result2)]/pi
      ffo_est = atan2(r1d.r * r2d.i - r2d.r * r1d.i, r1d.r * r2d.r + r1d.i * r2d.i) / M_PI;

#ifdef DBG_PSS_NR
      printf("ffo %lf\n", ffo_est);
#endif
    }

    pss_info.pss_elem_info[pss_count].freq_offset = ffo_est * p->subcarrier_spacing; // Absolute value of frequency offset
    pss_count++;
  }

  // Sort array in descending order using pss_peak as sorting criterion.
  qsort(pss_info.pss_elem_info, NUMBER_PSS_SEQUENCE, sizeof(pss_detection_result_t), cmp_pss_peak);

  pss_detection_result_t *pss_el = pss_info.pss_elem_info;
  if (pss_el->success)
    LOG_D(PHY,
          "[UE] nr_synchro_time: Sync source (nid2) = %d, Peak found at pos %d, val = %d dB power over signal avg %d dB, ffo "
          "%i\n",
          pss_el->nid2,
          pss_el->pos,
          pss_el->peak,
          pss_el->avg,
          pss_el->freq_offset);

#ifdef DBG_PSS_NR
  static int debug_cnt = 0;
  if (debug_cnt == 0) {
    LOG_M("rxdata0.m", "rxd0", p->rxdata[0], p->rxdata_length, 1, 1);
  } else {
    debug_cnt++;
  }
#endif

  return pss_info;
}

void sl_generate_pss(SL_NR_UE_INIT_PARAMS_t *sl_init_params, uint8_t n_sl_id2, uint16_t scaling)
{
  int i = 0, m = 0;
  int16_t x[SL_NR_PSS_SEQUENCE_LENGTH];
  const int x_initial[7] = {0, 1, 1, 0, 1, 1, 1};
  int16_t *sl_pss = sl_init_params->sl_pss[n_sl_id2];
  int16_t *sl_pss_for_sync = sl_init_params->sl_pss_for_sync[n_sl_id2];

  LOG_D(PHY, "SIDELINK PSBCH INIT: PSS Generation with N_SL_id2:%d\n", n_sl_id2);

#ifdef SL_DEBUG_INIT
  printf("SIDELINK: PSS Generation with N_SL_id2:%d\n", n_sl_id2);
#endif

  /// Sequence generation
  for (i = 0; i < 7; i++)
    x[i] = x_initial[i];

  for (i = 0; i < (SL_NR_PSS_SEQUENCE_LENGTH - 7); i++) {
    x[i + 7] = (x[i + 4] + x[i]) % 2;
  }

  for (i = 0; i < SL_NR_PSS_SEQUENCE_LENGTH; i++) {
    m = (i + 22 + 43 * n_sl_id2) % SL_NR_PSS_SEQUENCE_LENGTH;
    sl_pss_for_sync[i] = (1 - 2 * x[m]);
    sl_pss[i] = sl_pss_for_sync[i] * scaling;

#ifdef SL_DEBUG_INIT_DATA
    printf("m:%d, sl_pss[%d]:%d\n", m, i, sl_pss[i]);
#endif
  }

#ifdef SL_DUMP_INIT_SAMPLES
  LOG_M("sl_pss_seq.m", "sl_pss", (void *)sl_pss, SL_NR_PSS_SEQUENCE_LENGTH, 1, 0);
#endif
}

// This cannot be done at init time as ofdm symbol size, ssb start subcarrier depends on configuration
// done at SLSS read time.
void sl_generate_pss_ifft_samples(sl_nr_ue_phy_params_t *sl_ue_params, SL_NR_UE_INIT_PARAMS_t *sl_init_params)
{
  uint8_t id2 = 0;
  int16_t *sl_pss = NULL;
  NR_DL_FRAME_PARMS *sl_fp = &sl_ue_params->sl_frame_params;
  int16_t scaling_factor = AMP;

  int32_t *pss_T = NULL;
  c16_t pss_F[sl_fp->ofdm_symbol_size]; // IQ samples in freq domain

  LOG_I(PHY, "SIDELINK INIT: Generation of PSS time domain samples. scaling_factor:%d\n", scaling_factor);

  for (id2 = 0; id2 < SL_NR_NUM_IDs_IN_PSS; id2++) {
    int k = CIRCULAR_INC(sl_fp->first_carrier_offset,
                         sl_fp->ssb_start_subcarrier + 2,
                         sl_fp->ofdm_symbol_size); // PSS in from REs 2-129
    pss_T = &sl_init_params->sl_pss_for_correlation[id2][0];
    sl_pss = sl_init_params->sl_pss[id2];

    memset(pss_T, 0, sl_fp->ofdm_symbol_size * sizeof(pss_T[0]));
    memset(pss_F, 0, sl_fp->ofdm_symbol_size * sizeof(c16_t));

    for (int i = 0; i < SL_NR_PSS_SEQUENCE_LENGTH; i++) {
      pss_F[k].r = (sl_pss[i] * scaling_factor) >> 15;
      // pss_F[2*k] = (sl_pss[i]/23170) * 4192;
      // pss_F[2*k+1] = 0;

#ifdef SL_DEBUG_INIT_DATA
      printf("id:%d, k:%d, pss_F[%d]:%d, sl_pss[%d]:%d\n", id2, k, 2 * k, pss_F[2 * k], i, sl_pss[i]);
#endif
      k = CIRCULAR_INC(k, 1, sl_fp->ofdm_symbol_size);
    }

    idft((int16_t)get_idft(sl_fp->ofdm_symbol_size),
         (int16_t *)&pss_F[0], /* complex input */
         (int16_t *)&pss_T[0], /* complex output */
         1); /* scaling factor */
  }

#ifdef SL_DUMP_PSBCH_TX_SAMPLES
  LOG_M("sl_pss_TD_id0.m", "pss_TD_0", (void *)sl_init_params->sl_pss_for_correlation[0], sl_fp->ofdm_symbol_size, 1, 1);
  LOG_M("sl_pss_TD_id1.m", "pss_TD_1", (void *)sl_init_params->sl_pss_for_correlation[1], sl_fp->ofdm_symbol_size, 1, 1);
#endif
}
