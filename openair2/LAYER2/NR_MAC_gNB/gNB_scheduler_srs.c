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

/*! \file gNB_scheduler_srs.c
 * \brief MAC procedures related to SRS
 * \date 2021
 * \version 1.0
 */

#include <softmodem-common.h>
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "common/ran_context.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/nr/nr_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "PHY/sse_intrin.h"

//#define SRS_DEBUG

extern RAN_CONTEXT_t RC;

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
  /* already mutex protected: held in handle_nr_srs_measurements() */
  NR_SCHED_ENSURE_LOCKED(&RC.nrmac[0]->sched_lock);

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

  } else {
    AssertFatal(1 == 0, "nr_srs_ri_computation() function is not implemented for row = %i and col = %i\n", row, col);
  }
}

static void nr_configure_srs(nfapi_nr_srs_pdu_t *srs_pdu,
                             int slot,
                             int module_id,
                             int CC_id,
                             NR_UE_info_t *UE,
                             NR_SRS_ResourceSet_t *srs_resource_set,
                             NR_SRS_Resource_t *srs_resource,
                             int buffer_index)
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
  srs_pdu->time_start_position = srs_resource->resourceMapping.startPosition;
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
  srs_pdu->t_srs = srs_period[srs_resource->resourceType.choice.periodic->periodicityAndOffset_p.present];
  srs_pdu->t_offset = get_nr_srs_offset(srs_resource->resourceType.choice.periodic->periodicityAndOffset_p);

  // TODO: This should be completed
  srs_pdu->srs_parameters_v4.srs_bandwidth_size = m_SRS[srs_pdu->config_index];
  srs_pdu->srs_parameters_v4.usage = 1<<srs_resource_set->usage;
  srs_pdu->srs_parameters_v4.report_type[0] = 1;
  srs_pdu->srs_parameters_v4.iq_representation = 1;
  srs_pdu->srs_parameters_v4.prg_size = 1;
  srs_pdu->srs_parameters_v4.num_total_ue_antennas = 1<<srs_pdu->num_ant_ports;
  if (srs_resource_set->usage == NR_SRS_ResourceSet__usage_beamManagement) {
    srs_pdu->beamforming.trp_scheme = 0;
    srs_pdu->beamforming.num_prgs = m_SRS[srs_pdu->config_index];
    srs_pdu->beamforming.prg_size = 1;
  }

  uint16_t *vrb_map_UL = &RC.nrmac[module_id]->common_channels[CC_id].vrb_map_UL[buffer_index * MAX_BWP_SIZE];
  uint64_t mask = SL_to_bitmap(13 - srs_pdu->time_start_position, srs_pdu->num_symbols);
  for (int i = 0; i < srs_pdu->bwp_size; ++i)
    vrb_map_UL[i + srs_pdu->bwp_start] |= mask;
}

static void nr_fill_nfapi_srs(int module_id,
                              int CC_id,
                              NR_UE_info_t *UE,
                              int frame,
                              int slot,
                              NR_SRS_ResourceSet_t *srs_resource_set,
                              NR_SRS_Resource_t *srs_resource)
{

  int index = ul_buffer_index(frame, slot, UE->current_UL_BWP.scs, RC.nrmac[module_id]->UL_tti_req_ahead_size);
  nfapi_nr_ul_tti_request_t *future_ul_tti_req = &RC.nrmac[module_id]->UL_tti_req_ahead[0][index];
  AssertFatal(future_ul_tti_req->n_pdus <
              sizeof(future_ul_tti_req->pdus_list) / sizeof(future_ul_tti_req->pdus_list[0]),
              "Invalid future_ul_tti_req->n_pdus %d\n", future_ul_tti_req->n_pdus);
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE;
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_srs_pdu_t);
  nfapi_nr_srs_pdu_t *srs_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].srs_pdu;
  memset(srs_pdu, 0, sizeof(nfapi_nr_srs_pdu_t));
  future_ul_tti_req->n_pdus += 1;
  index = ul_buffer_index(frame, slot, UE->current_UL_BWP.scs, RC.nrmac[module_id]->vrb_map_UL_size);
  nr_configure_srs(srs_pdu, slot, module_id, CC_id, UE, srs_resource_set, srs_resource, index);
}

/*******************************************************************
*
* NAME :         nr_schedule_srs
*
* PARAMETERS :   module id
*                frame number for possible SRS reception
*
* DESCRIPTION :  It informs the PHY layer that has an SRS to receive.
*                Only for periodic scheduling yet.
*
*********************************************************************/
void nr_schedule_srs(int module_id, frame_t frame, int slot)
 {
  /* already mutex protected: held in gNB_dlsch_ulsch_scheduler() */
  gNB_MAC_INST *nrmac = RC.nrmac[module_id];
  NR_SCHED_ENSURE_LOCKED(&nrmac->sched_lock);

  NR_UEs_t *UE_info = &nrmac->UE_info;

  UE_iterator(UE_info->list, UE) {
    const int CC_id = 0;
    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
    NR_UE_UL_BWP_t *current_BWP = &UE->current_UL_BWP;

    if(sched_ctrl->sched_srs.srs_scheduled && sched_ctrl->sched_srs.frame == frame && sched_ctrl->sched_srs.slot == slot) {
      sched_ctrl->sched_srs.frame = -1;
      sched_ctrl->sched_srs.slot = -1;
      sched_ctrl->sched_srs.srs_scheduled = false;
    }

    if ((sched_ctrl->ul_failure && !get_softmodem_params()->phy_test) || sched_ctrl->rrc_processing_timer > 0) {
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

      NR_PUSCH_TimeDomainResourceAllocationList_t *tdaList = get_ul_tdalist(current_BWP,
                                                                            sched_ctrl->coreset->controlResourceSetId,
                                                                            sched_ctrl->search_space->searchSpaceType->present,
                                                                            TYPE_C_RNTI_);
      const int num_tda = tdaList->list.count;
      int max_k2 = 0;
      // avoid last one in the list (for msg3)
      for (int i = 0; i < num_tda - 1; i++) {
        int k2 = get_K2(tdaList, i, current_BWP->scs);
        max_k2 = k2 > max_k2 ? k2 : max_k2;
      }

      // we are sheduling SRS max_k2 slot in advance for the presence of SRS to be taken into account when scheduling PUSCH
      const int n_slots_frame = nr_slots_per_frame[current_BWP->scs];
      const int sched_slot = (slot + max_k2) % n_slots_frame;
      const int sched_frame = (frame + ((slot + max_k2) / n_slots_frame)) % 1024;

      const uint16_t period = srs_period[srs_resource->resourceType.choice.periodic->periodicityAndOffset_p.present];
      const uint16_t offset = get_nr_srs_offset(srs_resource->resourceType.choice.periodic->periodicityAndOffset_p);

      // Check if UE will transmit the SRS in this frame
      if ((sched_frame * n_slots_frame + sched_slot - offset) % period == 0) {
        LOG_D(NR_MAC," %d.%d Scheduling SRS reception for %d.%d\n", frame, slot, sched_frame, sched_slot);
        nr_fill_nfapi_srs(module_id, CC_id, UE, sched_frame, sched_slot, srs_resource_set, srs_resource);
        sched_ctrl->sched_srs.frame = sched_frame;
        sched_ctrl->sched_srs.slot = sched_slot;
        sched_ctrl->sched_srs.srs_scheduled = true;
      }
    }
  }
}
