/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*! \file gNB_scheduler_srs.c
 * \brief MAC procedures related to SRS
 * \date 2021
 * \version 1.0
 */

#include <softmodem-common.h>
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "common/ran_context.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/nr/nr_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "PHY/sse_intrin.h"

// #define SRS_DEBUG
const uint16_t m_SRS[64] = { 4, 8, 12, 16, 16, 20, 24, 24, 28, 32, 36, 40, 48, 48, 52, 56, 60, 64, 72, 72, 76, 80, 88,
                             96, 96, 104, 112, 120, 120, 120, 128, 128, 128, 132, 136, 144, 144, 144, 144, 152, 160,
                             160, 160, 168, 176, 184, 192, 192, 192, 192, 208, 216, 224, 240, 240, 240, 240, 256, 256,
                             256, 264, 272, 272, 272 };

#ifdef SRS_DEBUG
static void print128_number(const simde__m128i var)
{
  int32_t *var16 = (int32_t *)&var;
  for (int i = 0; i < 4; i++) {
    printf("%5d  ", var16[i]);
  }
  printf("\n");
}
#endif

int most_frequent_ri(const int *arr, int n)
{
  int maxcount = 0;
  int element_having_max_freq = -1;
  for (int i = 0; i < n; i++) {
    int count = 0;
    for (int j = 0; j < n; j++) {
      if (arr[i] == arr[j])
        count++;
    }
    if (count > maxcount) {
      maxcount = count;
      element_having_max_freq = arr[i];
    }
  }
  return element_having_max_freq;
}

static bool is_zero_c16(c16_t sample)
{
  return sample.r == 0 && sample.i == 0;
}

static bool complex_2x2_minor_nonzero(c16_t a0, c16_t a1, c16_t b0, c16_t b1)
{
  const int64_t a0b1_r = (int64_t)a0.r * b1.r - (int64_t)a0.i * b1.i;
  const int64_t a0b1_i = (int64_t)a0.r * b1.i + (int64_t)a0.i * b1.r;
  const int64_t a1b0_r = (int64_t)a1.r * b0.r - (int64_t)a1.i * b0.i;
  const int64_t a1b0_i = (int64_t)a1.r * b0.i + (int64_t)a1.i * b0.r;

  return a0b1_r != a1b0_r || a0b1_i != a1b0_i;
}

static uint8_t estimate_col2_matrix_rank_per_prg(const c16_t *ch,
                                                 uint16_t num_gnb_antenna_elements,
                                                 uint16_t num_prgs,
                                                 uint16_t pI)
{
  int first_nonzero_row = -1;

  for (int gI = 0; gI < num_gnb_antenna_elements; gI++) {
    const uint16_t base0_idx = 0 * num_gnb_antenna_elements * num_prgs + gI * num_prgs;
    const uint16_t base1_idx = 1 * num_gnb_antenna_elements * num_prgs + gI * num_prgs;
    const c16_t h0 = ch[base0_idx + pI];
    const c16_t h1 = ch[base1_idx + pI];

    if (is_zero_c16(h0) && is_zero_c16(h1))
      continue;

    if (first_nonzero_row < 0) {
      first_nonzero_row = gI;
      continue;
    }

    const uint16_t ref_base0_idx = 0 * num_gnb_antenna_elements * num_prgs + first_nonzero_row * num_prgs;
    const uint16_t ref_base1_idx = 1 * num_gnb_antenna_elements * num_prgs + first_nonzero_row * num_prgs;
    const c16_t ref0 = ch[ref_base0_idx + pI];
    const c16_t ref1 = ch[ref_base1_idx + pI];

    if (complex_2x2_minor_nonzero(ref0, ref1, h0, h1))
      return 2;
  }

  return first_nonzero_row < 0 ? 0 : 1;
}

void matrix_rank_128bits(int row, int col, simde__m128i mat[4])
{
  if ((row == 2 && col == 2) || (row == 4 && col == 2)) {

    int32_t *mat_k = (int32_t *)&mat[0];
    int32_t *mat_i = (int32_t *)&mat[1];

    simde__m128 mult_128 = simde_mm_setr_ps((float)mat_i[0] / (float)mat_k[0],
                                       (float)mat_i[0] / (float)mat_k[0],
                                       (float)mat_i[2] / (float)mat_k[2],
                                       (float)mat_i[2] / (float)mat_k[2]);

    simde__m128i mat_kj_128 = simde_mm_setr_epi32(mat_k[0], mat_k[1], mat_k[2], mat_k[3]);
    simde__m128 multiplication = simde_mm_mul_ps(mult_128, simde_mm_cvtepi32_ps(mat_kj_128));
    mat[1] = simde_mm_sub_epi32(mat[1], simde_mm_cvtps_epi32(multiplication));

    mat_k = (int32_t *)&mat[2];
    mat_i = (int32_t *)&mat[3];

    mult_128 = simde_mm_setr_ps((float)mat_i[0] / (float)mat_k[0],
                                (float)mat_i[0] / (float)mat_k[0],
                                (float)mat_i[2] / (float)mat_k[2],
                                (float)mat_i[2] / (float)mat_k[2]);

    mat_kj_128 = simde_mm_setr_epi32(mat_k[0], mat_k[1], mat_k[2], mat_k[3]);
    multiplication = simde_mm_mul_ps(mult_128, simde_mm_cvtepi32_ps(mat_kj_128));
    mat[3] = simde_mm_sub_epi32(mat[3], simde_mm_cvtps_epi32(multiplication));

  } else if (row == 2 && col == 4) {

    int32_t *mat_k = (int32_t *)&mat[0];
    int32_t *mat_i = (int32_t *)&mat[1];

    simde__m128 mult_128 = simde_mm_setr_ps((float)mat_i[0] / (float)mat_k[0],
                                       (float)mat_i[0] / (float)mat_k[0],
                                       (float)mat_i[0] / (float)mat_k[0],
                                       (float)mat_i[0] / (float)mat_k[0]);

    simde__m128i mat_kj_128 = simde_mm_setr_epi32(mat_k[0], mat_k[1], mat_k[2], mat_k[3]);
    simde__m128 multiplication = simde_mm_mul_ps(mult_128, simde_mm_cvtepi32_ps(mat_kj_128));
    mat[1] = simde_mm_sub_epi32(mat[1], simde_mm_cvtps_epi32(multiplication));

    mat_k = (int32_t *)&mat[2];
    mat_i = (int32_t *)&mat[3];

    mult_128 = simde_mm_setr_ps((float)mat_i[0] / (float)mat_k[0],
                                (float)mat_i[0] / (float)mat_k[0],
                                (float)mat_i[0] / (float)mat_k[0],
                                (float)mat_i[0] / (float)mat_k[0]);

    mat_kj_128 = simde_mm_setr_epi32(mat_k[0], mat_k[1], mat_k[2], mat_k[3]);
    multiplication = simde_mm_mul_ps(mult_128, simde_mm_cvtepi32_ps(mat_kj_128));
    mat[3] = simde_mm_sub_epi32(mat[3], simde_mm_cvtps_epi32(multiplication));

  } else if (row == 4 && col == 4) {

    for (int k = 0; k < col; k++) {
      int32_t *mat_k = (int32_t *)&mat[k];

      for (int i = k + 1; i < row; i++) {
        int32_t *mat_i = (int32_t *)&mat[i];

        float mult = (float)mat_i[k] / (float)mat_k[k];
        if (isnan(mult))
          mult = 0;
        if (isinf(mult))
          mult = 0;

        simde__m128 mult_128 = simde_mm_set1_ps(mult);
        simde__m128i mat_kj_128 = simde_mm_setr_epi32(mat_k[0], mat_k[1], mat_k[2], mat_k[3]);
        simde__m128 multiplication = simde_mm_mul_ps(mult_128, simde_mm_cvtepi32_ps(mat_kj_128));
        mat[i] = simde_mm_sub_epi32(mat[i], simde_mm_cvtps_epi32(multiplication));
      }
    }
  } else {
    AssertFatal(1 == 0, "matrix_rank_128bits() function is not implemented for row = %i and col = %i\n", row, col);
  }
}

void nr_srs_ri_computation(const nfapi_nr_srs_normalized_channel_iq_matrix_t *nr_srs_normalized_channel_iq_matrix,
                           const NR_UE_UL_BWP_t *current_BWP,
                           uint8_t *ul_ri)
{
#ifdef SRS_DEBUG
  LOG_I(NR_MAC, "num_gnb_antenna_elements = %i\n", nr_srs_normalized_channel_iq_matrix->num_gnb_antenna_elements);
  LOG_I(NR_MAC, "num_ue_srs_ports = %i\n", nr_srs_normalized_channel_iq_matrix->num_ue_srs_ports);
#endif

  if (nr_srs_normalized_channel_iq_matrix->num_gnb_antenna_elements == 1 ||
      nr_srs_normalized_channel_iq_matrix->num_ue_srs_ports == 1 ||
      current_BWP->pusch_Config == NULL ||
      (current_BWP->pusch_Config && *current_BWP->pusch_Config->maxRank == 1)) {
    *ul_ri = 0;
    return;
  }

  const c16_t *ch = (c16_t *)nr_srs_normalized_channel_iq_matrix->channel_matrix;
  const uint16_t num_gnb_antenna_elements = nr_srs_normalized_channel_iq_matrix->num_gnb_antenna_elements;
  const uint16_t num_prgs = nr_srs_normalized_channel_iq_matrix->num_prgs;

  int row = num_gnb_antenna_elements;
  int col = nr_srs_normalized_channel_iq_matrix->num_ue_srs_ports;
  simde__m128i mat_real128[4];
  simde__m128i mat_imag128[4];
  simde__m128i sum_matrix[4];

  if ((row == 2 && col == 2) || (row == 4 && col == 2)) {
    int array_lim = num_prgs >> 2;
    AssertFatal(array_lim > 0 , "Needed to avoid UB\n");
    int antenna_rank[array_lim];
    int count = 0;

    const uint16_t base00_idx = 0 * num_gnb_antenna_elements * num_prgs + 0 * num_prgs; // Rx antenna 0, Tx port 0
    const uint16_t base01_idx = 1 * num_gnb_antenna_elements * num_prgs + 0 * num_prgs; // Rx antenna 0, Tx port 1
    const uint16_t base10_idx = 0 * num_gnb_antenna_elements * num_prgs + 1 * num_prgs; // Rx antenna 1, Tx port 0
    const uint16_t base11_idx = 1 * num_gnb_antenna_elements * num_prgs + 1 * num_prgs; // Rx antenna 1, Tx port 1

    for (int pI = 0; pI < num_prgs; pI += 4) {
      uint16_t pI00 = pI + base00_idx;
      uint16_t pI10 = pI + base10_idx;
      uint16_t pI01 = pI + base01_idx;
      uint16_t pI11 = pI + base11_idx;
      mat_real128[0] = simde_mm_setr_epi32(ch[pI00].r, ch[pI10].r, ch[pI00 + 1].r, ch[pI10 + 1].r);
      mat_real128[1] = simde_mm_setr_epi32(ch[pI01].r, ch[pI11].r, ch[pI01 + 1].r, ch[pI11 + 1].r);
      mat_real128[2] = simde_mm_setr_epi32(ch[pI00 + 2].r, ch[pI10 + 2].r, ch[pI00 + 3].r, ch[pI10 + 3].r);
      mat_real128[3] = simde_mm_setr_epi32(ch[pI01 + 2].r, ch[pI11 + 2].r, ch[pI01 + 3].r, ch[pI11 + 3].r);
      mat_imag128[0] = simde_mm_setr_epi32(ch[pI00].i, ch[pI10].i, ch[pI00 + 1].i, ch[pI10 + 1].i);
      mat_imag128[1] = simde_mm_setr_epi32(ch[pI01].i, ch[pI11].i, ch[pI01 + 1].i, ch[pI11 + 1].i);
      mat_imag128[2] = simde_mm_setr_epi32(ch[pI00 + 2].i, ch[pI10 + 2].i, ch[pI00 + 3].i, ch[pI10 + 3].i);
      mat_imag128[3] = simde_mm_setr_epi32(ch[pI01 + 2].i, ch[pI11 + 2].i, ch[pI01 + 3].i, ch[pI11 + 3].i);

      matrix_rank_128bits(row, col, mat_real128);
      matrix_rank_128bits(row, col, mat_imag128);

      sum_matrix[0] = simde_mm_add_epi32(mat_real128[0], mat_imag128[0]);
      sum_matrix[1] = simde_mm_add_epi32(mat_real128[1], mat_imag128[1]);
      sum_matrix[2] = simde_mm_add_epi32(mat_real128[2], mat_imag128[2]);
      sum_matrix[3] = simde_mm_add_epi32(mat_real128[3], mat_imag128[3]);

#ifdef SRS_DEBUG
      LOG_I(NR_MAC, "\nSum matrix\n");
      print128_number(sum_matrix[0]);
      print128_number(sum_matrix[1]);
      print128_number(sum_matrix[2]);
      print128_number(sum_matrix[3]);
#endif

      int count_pivots = 0;

      int32_t *sum_matrix_i = (int32_t *)&sum_matrix[1];
      if (sum_matrix_i[1] != 0)
        count_pivots++;
      if (sum_matrix_i[3] != 0)
        count_pivots++;

      sum_matrix_i = (int32_t *)&sum_matrix[3];
      if (sum_matrix_i[1] != 0)
        count_pivots++;
      if (sum_matrix_i[3] != 0)
        count_pivots++;

      antenna_rank[count] = count_pivots / 4;
      count++;
    }
    *ul_ri = most_frequent_ri(antenna_rank, array_lim);

  } else if (row == 2 && col == 4) {

    int array_lim = num_prgs >> 1;
    int antenna_rank[array_lim];
    int count = 0;

    const uint16_t base00_idx = 0 * num_gnb_antenna_elements * num_prgs + 0 * num_prgs; // Rx antenna 0, Tx port 0
    const uint16_t base10_idx = 1 * num_gnb_antenna_elements * num_prgs + 0 * num_prgs; // Rx antenna 1, Tx port 0
    const uint16_t base20_idx = 2 * num_gnb_antenna_elements * num_prgs + 0 * num_prgs; // Rx antenna 2, Tx port 0
    const uint16_t base30_idx = 3 * num_gnb_antenna_elements * num_prgs + 0 * num_prgs; // Rx antenna 3, Tx port 0

    const uint16_t base01_idx = 0 * num_gnb_antenna_elements * num_prgs + 1 * num_prgs; // Rx antenna 0, Tx port 1
    const uint16_t base11_idx = 1 * num_gnb_antenna_elements * num_prgs + 1 * num_prgs; // Rx antenna 1, Tx port 1
    const uint16_t base21_idx = 2 * num_gnb_antenna_elements * num_prgs + 1 * num_prgs; // Rx antenna 2, Tx port 1
    const uint16_t base31_idx = 3 * num_gnb_antenna_elements * num_prgs + 1 * num_prgs; // Rx antenna 3, Tx port 1

    for (int pI = 0; pI < num_prgs; pI += 2) {

      mat_real128[0] = simde_mm_setr_epi32(ch[base00_idx + pI].r,
                                           ch[base10_idx + pI].r,
                                           ch[base20_idx + pI].r,
                                           ch[base30_idx + pI].r);
      mat_real128[1] = simde_mm_setr_epi32(ch[base01_idx + pI].r,
                                           ch[base11_idx + pI].r,
                                           ch[base21_idx + pI].r,
                                           ch[base31_idx + pI].r);
      mat_real128[2] = simde_mm_setr_epi32(ch[base00_idx + (pI + 1)].r,
                                           ch[base10_idx + (pI + 1)].r,
                                           ch[base20_idx + (pI + 1)].r,
                                           ch[base30_idx + (pI + 1)].r);
      mat_real128[3] = simde_mm_setr_epi32(ch[base01_idx + (pI + 1)].r,
                                           ch[base11_idx + (pI + 1)].r,
                                           ch[base21_idx + (pI + 1)].r,
                                           ch[base31_idx + (pI + 1)].r);

      mat_imag128[0] = simde_mm_setr_epi32(ch[base00_idx + pI].i,
                                           ch[base10_idx + pI].i,
                                           ch[base20_idx + pI].i,
                                           ch[base30_idx + pI].i);
      mat_imag128[1] = simde_mm_setr_epi32(ch[base01_idx + pI].i,
                                           ch[base11_idx + pI].i,
                                           ch[base21_idx + pI].i,
                                           ch[base31_idx + pI].i);
      mat_imag128[2] = simde_mm_setr_epi32(ch[base00_idx + (pI + 1)].i,
                                           ch[base10_idx + (pI + 1)].i,
                                           ch[base20_idx + (pI + 1)].i,
                                           ch[base30_idx + (pI + 1)].i);
      mat_imag128[3] = simde_mm_setr_epi32(ch[base01_idx + (pI + 1)].i,
                                           ch[base11_idx + (pI + 1)].i,
                                           ch[base21_idx + (pI + 1)].i,
                                           ch[base31_idx + (pI + 1)].i);

      matrix_rank_128bits(row, col, mat_real128);
      matrix_rank_128bits(row, col, mat_imag128);

      sum_matrix[0] = simde_mm_add_epi32(mat_real128[0], mat_imag128[0]);
      sum_matrix[1] = simde_mm_add_epi32(mat_real128[1], mat_imag128[1]);
      sum_matrix[2] = simde_mm_add_epi32(mat_real128[2], mat_imag128[2]);
      sum_matrix[3] = simde_mm_add_epi32(mat_real128[3], mat_imag128[3]);

#ifdef SRS_DEBUG
      LOG_I(NR_MAC, "\nSum matrix\n");
      print128_number(sum_matrix[0]);
      print128_number(sum_matrix[1]);
      print128_number(sum_matrix[2]);
      print128_number(sum_matrix[3]);
#endif

      int count_pivots = 0;

      int32_t *sum_matrix_i = (int32_t *)&sum_matrix[1];
      if (sum_matrix_i[1] != 0)
        count_pivots++;
      if (sum_matrix_i[3] != 0)
        count_pivots++;

      sum_matrix_i = (int32_t *)&sum_matrix[3];
      if (sum_matrix_i[1] != 0)
        count_pivots++;
      if (sum_matrix_i[3] != 0)
        count_pivots++;

      antenna_rank[count] = count_pivots / 2;
      count++;
    }

    int rr = most_frequent_ri(antenna_rank, array_lim);
    if (rr == 0 || rr == 1)
      *ul_ri = 0;
    if (rr > 1)
      *ul_ri = 1;

  } else if (row == 4 && col == 4) {

    int antenna_rank[num_prgs];
    int count = 0;

    const uint16_t base00_idx = 0 * num_gnb_antenna_elements * num_prgs + 0 * num_prgs; // Rx antenna 0, Tx port 0
    const uint16_t base10_idx = 1 * num_gnb_antenna_elements * num_prgs + 0 * num_prgs; // Rx antenna 1, Tx port 0
    const uint16_t base20_idx = 2 * num_gnb_antenna_elements * num_prgs + 0 * num_prgs; // Rx antenna 2, Tx port 0
    const uint16_t base30_idx = 3 * num_gnb_antenna_elements * num_prgs + 0 * num_prgs; // Rx antenna 3, Tx port 0

    const uint16_t base01_idx = 0 * num_gnb_antenna_elements * num_prgs + 1 * num_prgs; // Rx antenna 0, Tx port 1
    const uint16_t base11_idx = 1 * num_gnb_antenna_elements * num_prgs + 1 * num_prgs; // Rx antenna 1, Tx port 1
    const uint16_t base21_idx = 2 * num_gnb_antenna_elements * num_prgs + 1 * num_prgs; // Rx antenna 2, Tx port 1
    const uint16_t base31_idx = 3 * num_gnb_antenna_elements * num_prgs + 1 * num_prgs; // Rx antenna 3, Tx port 1

    const uint16_t base02_idx = 0 * num_gnb_antenna_elements * num_prgs + 2 * num_prgs; // Rx antenna 0, Tx port 2
    const uint16_t base12_idx = 1 * num_gnb_antenna_elements * num_prgs + 2 * num_prgs; // Rx antenna 1, Tx port 2
    const uint16_t base22_idx = 2 * num_gnb_antenna_elements * num_prgs + 2 * num_prgs; // Rx antenna 2, Tx port 2
    const uint16_t base32_idx = 3 * num_gnb_antenna_elements * num_prgs + 2 * num_prgs; // Rx antenna 3, Tx port 2

    const uint16_t base03_idx = 0 * num_gnb_antenna_elements * num_prgs + 3 * num_prgs; // Rx antenna 0, Tx port 3
    const uint16_t base13_idx = 1 * num_gnb_antenna_elements * num_prgs + 3 * num_prgs; // Rx antenna 1, Tx port 3
    const uint16_t base23_idx = 2 * num_gnb_antenna_elements * num_prgs + 3 * num_prgs; // Rx antenna 2, Tx port 3
    const uint16_t base33_idx = 3 * num_gnb_antenna_elements * num_prgs + 3 * num_prgs; // Rx antenna 3, Tx port 3

    for (int pI = 0; pI < num_prgs; pI++) {
      mat_real128[0] = simde_mm_setr_epi32(ch[base00_idx + pI].r, ch[base10_idx + pI].r, ch[base20_idx + pI].r, ch[base30_idx + pI].r);
      mat_real128[1] = simde_mm_setr_epi32(ch[base01_idx + pI].r, ch[base11_idx + pI].r, ch[base21_idx + pI].r, ch[base31_idx + pI].r);
      mat_real128[2] = simde_mm_setr_epi32(ch[base02_idx + pI].r, ch[base12_idx + pI].r, ch[base22_idx + pI].r, ch[base32_idx + pI].r);
      mat_real128[3] = simde_mm_setr_epi32(ch[base03_idx + pI].r, ch[base13_idx + pI].r, ch[base23_idx + pI].r, ch[base33_idx + pI].r);

      mat_imag128[0] = simde_mm_setr_epi32(ch[base00_idx + pI].i, ch[base10_idx + pI].i, ch[base20_idx + pI].i, ch[base30_idx + pI].i);
      mat_imag128[1] = simde_mm_setr_epi32(ch[base01_idx + pI].i, ch[base11_idx + pI].i, ch[base21_idx + pI].i, ch[base31_idx + pI].i);
      mat_imag128[2] = simde_mm_setr_epi32(ch[base02_idx + pI].i, ch[base12_idx + pI].i, ch[base22_idx + pI].i, ch[base32_idx + pI].i);
      mat_imag128[3] = simde_mm_setr_epi32(ch[base03_idx + pI].i, ch[base13_idx + pI].i, ch[base23_idx + pI].i, ch[base33_idx + pI].i);

      matrix_rank_128bits(row, col, mat_real128);
      matrix_rank_128bits(row, col, mat_imag128);

      sum_matrix[0] = simde_mm_add_epi32(mat_real128[0], mat_imag128[0]);
      sum_matrix[1] = simde_mm_add_epi32(mat_real128[1], mat_imag128[1]);
      sum_matrix[2] = simde_mm_add_epi32(mat_real128[2], mat_imag128[2]);
      sum_matrix[3] = simde_mm_add_epi32(mat_real128[3], mat_imag128[3]);

#ifdef SRS_DEBUG
      LOG_I(NR_MAC, "\nSum matrix\n");
      print128_number(sum_matrix[0]);
      print128_number(sum_matrix[1]);
      print128_number(sum_matrix[2]);
      print128_number(sum_matrix[3]);
#endif

      int count_pivots = 0;
      for (int i = 0; i < row; i++) {
        int32_t *sum_matrix_i = (int32_t *)&sum_matrix[i];

        int found_piv = 0;
        for (int j = 0; j < col; j++) {
          if (sum_matrix_i[j] != 0 && found_piv == 0) {
            count_pivots++;
            found_piv = 1;
          }
        }
      }
      antenna_rank[count] = count_pivots;
      count++;
    }
    *ul_ri = most_frequent_ri(antenna_rank, num_prgs) - 1;

  } else if (row == 8 && col == 2) {

    int antenna_rank[num_prgs];

    for (int pI = 0; pI < num_prgs; pI++) {
      const uint8_t rank = estimate_col2_matrix_rank_per_prg(ch, num_gnb_antenna_elements, num_prgs, pI);
      antenna_rank[pI] = rank > 1 ? 1 : 0;
    }

    *ul_ri = most_frequent_ri(antenna_rank, num_prgs);

  } else {
    AssertFatal(1 == 0, "nr_srs_ri_computation() function is not implemented for row = %i and col = %i\n", row, col);
  }
}

static void nr_configure_srs(gNB_MAC_INST *nrmac,
                             nr_cell_sched_t *cell,
                             nfapi_nr_srs_pdu_t *srs_pdu,
                             NR_UE_info_t *UE,
                             NR_SRS_ResourceSet_t *srs_resource_set,
                             NR_SRS_Resource_t *srs_resource,
                             int beam_idx)
{
  NR_UE_UL_BWP_t *current_BWP = &UE->current_UL_BWP;

  srs_pdu->rnti = UE->rnti;
  srs_pdu->handle = 0;
  srs_pdu->bwp_size = current_BWP->BWPSize;
  srs_pdu->bwp_start = current_BWP->BWPStart;
  srs_pdu->subcarrier_spacing = current_BWP->scs;
  srs_pdu->cyclic_prefix = 0;
  srs_pdu->num_ant_ports = srs_resource->nrofSRS_Ports;
  srs_pdu->num_symbols = srs_resource->resourceMapping.nrofSymbols;
  srs_pdu->num_repetitions = srs_resource->resourceMapping.repetitionFactor;
  srs_pdu->time_start_position =  NR_SYMBOLS_PER_SLOT - 1 - srs_resource->resourceMapping.startPosition;
  srs_pdu->config_index = srs_resource->freqHopping.c_SRS;
  srs_pdu->sequence_id = srs_resource->sequenceId;
  srs_pdu->bandwidth_index = srs_resource->freqHopping.b_SRS;
  srs_pdu->comb_size = srs_resource->transmissionComb.present - 1;

  switch(srs_resource->transmissionComb.present) {
    case NR_SRS_Resource__transmissionComb_PR_n2:
      srs_pdu->comb_offset = srs_resource->transmissionComb.choice.n2->combOffset_n2;
      srs_pdu->cyclic_shift = srs_resource->transmissionComb.choice.n2->cyclicShift_n2;
      break;
    case NR_SRS_Resource__transmissionComb_PR_n4:
      srs_pdu->comb_offset = srs_resource->transmissionComb.choice.n4->combOffset_n4;
      srs_pdu->cyclic_shift = srs_resource->transmissionComb.choice.n4->cyclicShift_n4;
      break;
    default:
      LOG_W(NR_MAC, "Invalid or not implemented comb_size!\n");
  }

  srs_pdu->frequency_position = srs_resource->freqDomainPosition;
  srs_pdu->frequency_shift = srs_resource->freqDomainShift;
  srs_pdu->frequency_hopping = srs_resource->freqHopping.b_hop;
  srs_pdu->group_or_sequence_hopping = srs_resource->groupOrSequenceHopping;
  srs_pdu->resource_type = srs_resource->resourceType.present - 1;
  if (srs_resource->resourceType.present == NR_SRS_Resource__resourceType_PR_periodic) {
    srs_pdu->t_srs = srs_period[srs_resource->resourceType.choice.periodic->periodicityAndOffset_p.present];
    srs_pdu->t_offset = get_nr_srs_offset(srs_resource->resourceType.choice.periodic->periodicityAndOffset_p);
  }

  // TODO: This should be completed
  srs_pdu->srs_parameters_v4.srs_bandwidth_size = m_SRS[srs_pdu->config_index];
  srs_pdu->srs_parameters_v4.usage = 1 << srs_resource_set->usage;
  positioning_activation_info_t *pos_ue_context = get_pos_act_ue_context(nrmac, UE->rnti);
  if (pos_ue_context != NULL) {
    srs_pdu->srs_parameters_v4.usage = 1 << NFAPI_NR_SRS_POSITIONING;
  }
  srs_pdu->srs_parameters_v4.report_type[0] = 1;
  srs_pdu->srs_parameters_v4.iq_representation = 1;
  srs_pdu->srs_parameters_v4.prg_size = 1;
  srs_pdu->srs_parameters_v4.num_total_ue_antennas = 1 << srs_pdu->num_ant_ports;
  /* For srs usage: codebook, this is a bitmask of the antenna ports that should be used for data.
  * num_ant_ports num_total_ue_antennas sampled_ue_antennas
  *             0                     1                   1
  *             1                     2                   3
  *             2                     4                  15 */
  srs_pdu->srs_parameters_v4.sampled_ue_antennas = (2 << srs_pdu->num_ant_ports) - 1;
  if (srs_resource_set->usage == NR_SRS_ResourceSet__usage_beamManagement) {
    srs_pdu->beamforming.trp_scheme = 0;
    srs_pdu->beamforming.num_prgs = m_SRS[srs_pdu->config_index];
    srs_pdu->beamforming.prg_size = srs_pdu->srs_parameters_v4.srs_bandwidth_size;
  }

  // Indexing SRS antenna ports when beamformed
  const unsigned int srs_num_rx_ant_ports = cell->radio_config.pusch_AntennaPorts;
  srs_pdu->srs_parameters_v4.num_ul_spatial_streams_ports = srs_num_rx_ant_ports;
  srs_pdu->beamforming.dig_bf_interface = srs_num_rx_ant_ports;
  const uint16_t fapi_beam = convert_to_fapi_beam(UE->UE_beam_index, cell->beam_info.beam_mode);
  for (int i = 0; i < srs_num_rx_ant_ports;i++){
    srs_pdu->beamforming.prgs_list[0].dig_bf_interface_list[i].beam_idx = fapi_beam;
    srs_pdu->srs_parameters_v4.Ul_spatial_stream_ports[i] =
        cell->radio_config.spatial_stream_index[beam_idx * srs_num_rx_ant_ports + i];
  }
}

static bool nr_fill_nfapi_srs(gNB_MAC_INST *nrmac,
                              nr_cell_sched_t *cell,
                              NR_UE_info_t *UE,
                              int frame,
                              int slot,
                              NR_SRS_ResourceSet_t *srs_resource_set,
                              NR_SRS_Resource_t *srs_resource)
{
  int slots_frame = cell->frame_structure.numb_slots_frame;
  int index = ul_buffer_index(frame, slot, slots_frame, cell->UL_tti_req_ahead_size);
  NR_beam_alloc_t beam = beam_allocation_procedure(&cell->beam_info, frame, slot, UE->UE_beam_index, slots_frame);
  if (beam.idx < 0) {
    LOG_W(NR_MAC, "Cannot allocate aperiodic SRS in any available beam\n");
    return false;
  }

  uint16_t *vrb_map_UL = &cell->common_channels.vrb_map_UL[beam.idx][index * MAX_BWP_SIZE];
  uint16_t num = 1 << srs_resource->resourceMapping.nrofSymbols;
  const uint8_t l0 = NR_SYMBOLS_PER_SLOT - 1 - srs_resource->resourceMapping.startPosition;
  uint16_t mask = SL_to_bitmap(l0, num);
  DevAssert(mask != 0);
  for (int i = 0; i < UE->current_UL_BWP.BWPSize; ++i) {
    int rb = i + UE->current_UL_BWP.BWPStart;
    uint16_t alloc = vrb_map_UL[rb] & mask;
    // we allocate SRS regardless of prohibited UL PRBs (already present in VRB map)
    if (alloc != 0 && cell->ulprbbl[rb] == 0) {
      LOG_W(NR_MAC, "RB %d not free for SRS: alloc 0x%02x for mask 0x%02x\n", rb, alloc, mask);
      // resetting the resources allocated for SRS
      reset_beam_status(&cell->beam_info, frame, slot, UE->UE_beam_index, slots_frame, beam.new_beam);
      for (int j = UE->current_UL_BWP.BWPStart; j < rb; ++j)
        vrb_map_UL[j] &= ~mask;
      return false;
    }
    vrb_map_UL[rb] |= mask;
  }

  nfapi_nr_ul_tti_request_t *future_ul_tti_req = &cell->UL_tti_req_ahead[index];
  AssertFatal(future_ul_tti_req->n_pdus <
              sizeof(future_ul_tti_req->pdus_list) / sizeof(future_ul_tti_req->pdus_list[0]),
              "Invalid future_ul_tti_req->n_pdus %d\n", future_ul_tti_req->n_pdus);
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE;
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_srs_pdu_t);
  nfapi_nr_srs_pdu_t *srs_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].srs_pdu;
  memset(srs_pdu, 0, sizeof(nfapi_nr_srs_pdu_t));
  future_ul_tti_req->n_pdus += 1;
  index = ul_buffer_index(frame, slot, slots_frame, cell->vrb_map_UL_size);
  nr_configure_srs(nrmac, cell, srs_pdu, UE, srs_resource_set, srs_resource, beam.idx);
  return true;
}

/*******************************************************************
*
* NAME :         nr_schedule_periodic_srs
*
* PARAMETERS :   module id
*                current frame number
*                current slot number
*
* DESCRIPTION :  It schedules SRS in a future slot and calls function to prepare FAPI PDU for L1
*
*********************************************************************/
void nr_schedule_periodic_srs(gNB_MAC_INST *nrmac, nr_cell_sched_t *cell, frame_t frame, int slot)
{

  NR_UEs_t *UE_info = &nrmac->UE_info;

  UE_iterator(UE_info->connected_ue_list, UE) {
    if (UE->pcell != cell)
      continue;
    NR_UE_UL_BWP_t *current_BWP = &UE->current_UL_BWP;

    if (!nr_mac_ue_is_active(UE) && !get_softmodem_params()->phy_test) {
      continue;
    }

    NR_SRS_Config_t *srs_config = current_BWP->srs_Config;
    if (!srs_config)
      continue;

    for(int rs = 0; rs < srs_config->srs_ResourceSetToAddModList->list.count; rs++) {

      // Find periodic resource set
      NR_SRS_ResourceSet_t *srs_resource_set = srs_config->srs_ResourceSetToAddModList->list.array[rs];
      if (srs_resource_set->resourceType.present != NR_SRS_ResourceSet__resourceType_PR_periodic) {
        continue;
      }

      // Find the corresponding srs resource
      NR_SRS_Resource_t *srs_resource = NULL;
      for (int r1 = 0; r1 < srs_resource_set->srs_ResourceIdList->list.count; r1++) {
        for (int r2 = 0; r2 < srs_config->srs_ResourceToAddModList->list.count; r2++) {
          if ((*srs_resource_set->srs_ResourceIdList->list.array[r1] ==
               srs_config->srs_ResourceToAddModList->list.array[r2]->srs_ResourceId) &&
              (srs_config->srs_ResourceToAddModList->list.array[r2]->resourceType.present ==
               NR_SRS_Resource__resourceType_PR_periodic)) {
            srs_resource = srs_config->srs_ResourceToAddModList->list.array[r2];
            break;
          }
        }
      }

      if (srs_resource == NULL) {
        continue;
      }

      // we are sheduling SRS max_k2 slot in advance for the presence of SRS to be taken into account when scheduling PUSCH
      const int n_slots_frame = cell->frame_structure.numb_slots_frame;
      const int n_ahead = n_slots_frame - 1 + get_NTN_Koffset(cell->common_channels.ServingCellConfigCommon);
      const int sched_slot = (slot + n_ahead) % n_slots_frame;
      const int sched_frame = (frame + (slot + n_ahead) / n_slots_frame) % MAX_FRAME_NUMBER;

      const uint16_t period = srs_period[srs_resource->resourceType.choice.periodic->periodicityAndOffset_p.present];
      const uint16_t offset = get_nr_srs_offset(srs_resource->resourceType.choice.periodic->periodicityAndOffset_p);

      // Check if UE will transmit the SRS in this frame
      if ((sched_frame * n_slots_frame + sched_slot - offset) % period != 0)
        continue;
      bool ret = nr_fill_nfapi_srs(nrmac, cell, UE, sched_frame, sched_slot, srs_resource_set, srs_resource);
      AssertFatal(ret, "Cannot allocate periodic SRS\n");
      LOG_D(NR_MAC," %d.%d Scheduling SRS reception for %d.%d\n", frame, slot, sched_frame, sched_slot);
    }
  }
}

bool nr_schedule_aperiodic_srs(gNB_MAC_INST *nrmac,nr_cell_sched_t *cell, NR_UE_info_t *UE, int sched_frame, int sched_slot, int k2, int sched_srs)
{
  NR_UE_UL_BWP_t *current_BWP = &UE->current_UL_BWP;
  NR_SRS_Config_t *srs_config = current_BWP->srs_Config;
  AssertFatal(srs_config, "Attempting to schedule aperiodic SRS without SRS configuration\n");

  for(int rs = 0; rs < srs_config->srs_ResourceSetToAddModList->list.count; rs++) {
    // Find periodic resource set
    NR_SRS_ResourceSet_t *srs_resource_set = srs_config->srs_ResourceSetToAddModList->list.array[rs];
    if (srs_resource_set->resourceType.present != NR_SRS_ResourceSet__resourceType_PR_aperiodic)
      continue;

    // We aim to schedule SRS in the same slot as PUSCH
    struct NR_SRS_ResourceSet__resourceType__aperiodic *aperiodic = srs_resource_set->resourceType.choice.aperiodic;
    if (aperiodic->aperiodicSRS_ResourceTrigger != sched_srs)
      continue;
    int offset = aperiodic->slotOffset ? *aperiodic->slotOffset : 0;
    if (offset != k2) {
      LOG_E(NR_MAC, "Aperiodic SRS offset %d for trigger state %d doesn't match with K2 %d\n", offset, sched_srs, k2);
      return false;
    }

    // Find the corresponding srs resource
    for (int r1 = 0; r1 < srs_resource_set->srs_ResourceIdList->list.count; r1++) {
      for (int r2 = 0; r2 < srs_config->srs_ResourceToAddModList->list.count; r2++) {
        if ((*srs_resource_set->srs_ResourceIdList->list.array[r1] ==
             srs_config->srs_ResourceToAddModList->list.array[r2]->srs_ResourceId) &&
            (srs_config->srs_ResourceToAddModList->list.array[r2]->resourceType.present ==
             NR_SRS_Resource__resourceType_PR_aperiodic)) {
          NR_SRS_Resource_t *srs_resource = srs_config->srs_ResourceToAddModList->list.array[r2];
          if (!nr_fill_nfapi_srs(nrmac, cell, UE, sched_frame, sched_slot, srs_resource_set, srs_resource))
            continue;
          LOG_D(NR_MAC,"Scheduling aperiodic SRS reception for %d.%d\n", sched_frame, sched_slot);
          nr_timer_start(&UE->UE_sched_ctrl.aperiodic_srs_trigger);  // restart the timer, we are scheduling aperiodic SRS
          return true;
        }
      }
    }
  }
  return false;
}
