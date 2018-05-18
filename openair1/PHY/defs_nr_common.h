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

/*! \file PHY/defs_nr_common.h
 \brief Top-level defines and structure definitions
 \author Guy De Souza
 \date 2018
 \version 0.1
 \company Eurecom
 \email: desouza@eurecom.fr
 \note
 \warning
*/

#ifndef __PHY_DEFS_NR_COMMON__H__
#define __PHY_DEFS_NR_COMMON__H__

#include "defs_common.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

#define nr_subframe_t lte_subframe_t

#define MAX_NUM_SUBCARRIER_SPACING 5

#define NR_SYMBOLS_PER_SLOT 14

#define ONE_OVER_SQRT2_Q15 23170
#define ONE_OVER_TWO_Q15 16384

#define NR_MOD_TABLE_SIZE_SHORT 14
#define NR_MOD_TABLE_BPSK_OFFSET 1
#define NR_MOD_TABLE_QPSK_OFFSET 3

#define NR_PSS_LENGTH 127
#define NR_SSS_LENGTH 127

#define NR_PBCH_DMRS_LENGTH 144
#define NR_PBCH_DMRS_LENGTH_DWORD 5 // roundup(NR_PBCH_DMRS_LENGTH/32)

typedef enum {
  NR_MU_0=0,
  NR_MU_1,
  NR_MU_2,
  NR_MU_3,
  NR_MU_4,
} nr_numerology_index_e;


typedef enum{
  nr_ssb_type_A = 0,
  nr_ssb_type_B,
  nr_ssb_type_C,
  nr_ssb_type_D,
  nr_ssb_type_E
} nr_ssb_type_e;

typedef struct NR_DL_FRAME_PARMS {
  /// EUTRA Band
  uint8_t eutra_band;
  /// DL carrier frequency
  uint32_t dl_CarrierFreq;
  /// UL carrier frequency
  uint32_t ul_CarrierFreq;
  /// TX attenuation
  uint32_t att_tx;
  /// RX attenuation
  uint32_t att_rx;
  ///  total Number of Resource Block Groups: this is ceil(N_PRB/P)
  uint32_t subcarrier_spacing;
  /// 3/4 sampling
  uint8_t threequarter_fs;
  /// Size of FFT
  uint16_t ofdm_symbol_size;
  /// Number of prefix samples in all but first symbol of slot
  uint16_t nb_prefix_samples;
  /// Number of prefix samples in first symbol of slot
  uint16_t nb_prefix_samples0;
  /// Carrier offset in FFT buffer for first RE in PRB0
  uint16_t first_carrier_offset;
  /// Number of OFDM/SC-FDMA symbols in one slot
  uint16_t symbols_per_slot;
  /// Number of slots per subframe
  uint16_t slots_per_subframe;
  /// Number of samples in a subframe
  uint32_t samples_per_subframe;
  /// Number of samples in a radio frame
  uint32_t samples_per_frame;
  /// Number of samples in a subframe without CP
  uint32_t samples_per_subframe_wCP;
  /// Number of samples in a radio frame without CP
  uint32_t samples_per_frame_wCP;

    //SSB related params
  /// Start in Subcarrier index of the SSB block
  uint16_t ssb_start_subcarrier;
  /// SSB type
  nr_ssb_type_e ssb_type;
  /// PBCH polar encoder params
  t_nrPolar_params pbch_polar_params;

} NR_DL_FRAME_PARMS;

#endif
