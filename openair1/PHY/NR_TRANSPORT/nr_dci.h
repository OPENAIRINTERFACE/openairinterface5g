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

#ifndef __PHY_NR_TRANSPORT_DCI__H
#define __PHY_NR_TRANSPORT_DCI__H

#include "PHY/defs_gNB.h"
#include "PHY/NR_REFSIG/nr_refsig.h"

typedef enum {
  nr_dci_format_0_0=0,
  nr_dci_format_0_1,
  nr_dci_format_2_0,
  nr_dci_format_2_1,
  nr_dci_format_2_2,
  nr_dci_format_2_3,
  nr_dci_format_1_0,
  nr_dci_format_1_1,
} nr_dci_format_e;

typedef enum {
  nr_rnti_type_SI_RNTI=0,
  nr_rnti_type_RA_RNTI,
  nr_rnti_type_C_RNTI,
  nr_rnti_type_TC_RNTI,
  nr_rnti_type_CS_RNTI,
  nr_rnti_type_P_RNTI
} nr_rnti_type_e;

typedef struct {
  uint8_t param_O;
  uint8_t param_M;
  uint8_t nb_ss_sets_per_slot;
  uint8_t first_symbol_idx;
} nr_pdcch_ss_params_t;

typedef struct {
  uint8_t n_rb;
  uint8_t n_symb;
  uint8_t rb_offset;
  nr_cce_reg_mapping_type_e cr_mapping_type;
  nr_ssb_and_cset_mux_pattern_type_e mux_pattern;
  nr_coreset_precoder_granularity_type_e precoder_granularity;
  nr_coreset_config_type_e config_type;
} nr_pdcch_coreset_params_t;

typedef struct {
  uint8_t reg_idx;
  uint8_t start_sc_idx;
  uint8_t symb_idx;
} nr_reg_t;

typedef struct {
  uint8_t cce_idx;
  nr_reg_t reg_list[NR_NB_REG_PER_CCE];
} nr_cce_t;

typedef struct {
  uint8_t first_slot;
  uint8_t nb_slots;
  uint8_t sfn_mod2;
  uint32_t dmrs_scrambling_id;
  nr_cce_t cce_list[NR_MAX_PDCCH_AGG_LEVEL];
  nr_pdcch_ss_params_t ss_params;
  nr_pdcch_coreset_params_t coreset_params;
} nr_pdcch_vars_t;

typedef unsigned __int128 uint128_t;

uint8_t nr_get_dci_size(nr_dci_format_e format,
                        nr_rnti_type_e rnti_type,
                        NR_BWP_PARMS* bwp,
                        nfapi_nr_config_request_t* config);

uint8_t nr_generate_dci_top(NR_gNB_DCI_ALLOC_t dci_alloc,
                            uint32_t *gold_pdcch_dmrs,
                            int32_t** txdataF,
                            int16_t amp,
                            NR_DL_FRAME_PARMS frame_parms,
                            nfapi_nr_config_request_t config,
                            nr_pdcch_vars_t pdcch_vars);

void nr_pdcch_scrambling(NR_gNB_DCI_ALLOC_t dci_alloc,
                         nr_pdcch_vars_t pdcch_vars,
                         nfapi_nr_config_request_t config,
                         uint32_t* out);

#endif //__PHY_NR_TRANSPORT_DCI__H
