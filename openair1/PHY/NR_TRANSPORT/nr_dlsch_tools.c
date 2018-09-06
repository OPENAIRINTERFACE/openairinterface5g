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

/*! \file PHY/LTE_TRANSPORT/dlsch_decoding.c
* \brief Top-level routines for decoding  Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/

#include "nr_dlsch.h"

uint8_t nr_pdsch_default_time_alloc_A_S_nCP[23] = {2,3,2,3,2,3,2,3,2,3,9,10,4,6,5,5,9,12,1,1,2,4,8};
uint8_t nr_pdsch_default_time_alloc_A_L_nCP[23] = {12,11,10,9,9,8,7,6,5,4,4,4,4,4,7,2,2,2,13,6,4,7,4};
uint8_t nr_pdsch_default_time_alloc_A_S_eCP[23] = {2,3,2,3,2,3,2,3,2,3,6,8,4,6,5,5,9,10,1,1,2,4,8};
uint8_t nr_pdsch_default_time_alloc_A_L_eCP[23] = {6,5,10,9,9,8,7,6,5,4,4,2,4,4,6,2,2,2,11,6,4,6,4};
uint8_t nr_pdsch_default_time_alloc_B_S[16] = {2,4,6,8,10,2,4,2,4,6,8,10,2,2,3,2};
uint8_t nr_pdsch_default_time_alloc_B_L[16] = {2,2,2,2,2,2,2,4,4,4,4,4,7,12,11,4};
uint8_t nr_pdsch_default_time_alloc_C_S[15] = {2,4,6,8,10,2,4,6,8,10,2,2,3,0,2};
uint8_t nr_pdsch_default_time_alloc_C_L[15] = {2,2,2,2,2,4,4,4,4,4,7,12,11,6,6};

void nr_get_time_domain_allocation_type(nfapi_nr_config_request_t config,
                                        NR_gNB_DCI_ALLOC_t dci_alloc,
                                        NR_gNB_DLSCH_t* dlsch) {

  nfapi_nr_ssb_and_cset_mux_pattern_type_e mux_pattern = dci_alloc.pdcch_params.mux_pattern;

  switch(dci_alloc.pdcch_params.rnti_type) {

    case NFAPI_NR_RNTI_SI:
      AssertFatal(dci_alloc.pdcch_params.common_search_space_type == NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_0,
      "Invalid common search space type %d for SI RNTI, expected %d\n",
      dci_alloc.pdcch_params.common_search_space_type, NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_0);

      if (mux_pattern == NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1)
        AssertFatal(config.subframe_config.dl_cyclic_prefix_type.value == NFAPI_CP_NORMAL,
        "Invalid configuration CP extended for SI RNTI type 0 search space\n");

      dlsch->time_alloc_type = (mux_pattern == NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1)?NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A :
                               (mux_pattern == NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2)?NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_B :
                               NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_C;
      break;

    case NFAPI_NR_RNTI_RA:
    case NFAPI_NR_RNTI_TC:
      /*AssertFatal(dci_alloc.pdcch_params.common_search_space_type == NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_1,
      "Invalid common search space type %d for RNTI %d, expected %d\n",dci_alloc.pdcch_params.common_search_space_type,
      NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_1, dci_alloc.rnti_type);*/
      dlsch->time_alloc_type = (dlsch->time_alloc_list_flag) ? NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_ALLOC_LIST : NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A;
      break;

    case NFAPI_NR_RNTI_P:
      break;

    case NFAPI_NR_RNTI_C:
    case NFAPI_NR_RNTI_CS:
      if (dci_alloc.pdcch_params.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_COMMON)
        dlsch->time_alloc_type = (dlsch->time_alloc_list_flag)? NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_ALLOC_LIST : NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A;
      else
        dlsch->time_alloc_type = NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_ALLOC_LIST;

      break;

  }
}

static inline uint16_t get_SLIV(uint8_t S, uint8_t L) {
  return ( (uint16_t)(((L-1)<=7)? (14*(L-1)+S) : (14*(15-L)+(13-S))) );
}

static inline uint8_t get_K0(uint8_t row_idx, uint8_t time_alloc_type) {
  return ( (time_alloc_type == NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A)||
  (time_alloc_type == NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_C)? 0 :
  ((row_idx==6)||(row_idx==7)||(row_idx==15))? 1 : 0);
}

/*ideally combine the calculation of L in the same function once the right struct is defined*/
uint8_t nr_get_S(uint8_t row_idx, uint8_t CP, uint8_t time_alloc_type, uint8_t dmrs_typeA_position) {

  uint8_t idx, S;

  switch(time_alloc_type) {
    case NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A:
      idx = (row_idx>7)? (row_idx+6) : (((row_idx-1)<<1)-1+((dmrs_typeA_position==2)?0:1));
      return ((CP==NFAPI_CP_NORMAL)?nr_pdsch_default_time_alloc_A_S_nCP[idx] : nr_pdsch_default_time_alloc_A_S_eCP[idx]);
      break;

    case NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_B:
      idx = (row_idx<14)? (row_idx-1) : (row_idx == 14)? row_idx-1+((dmrs_typeA_position==2)?0:1) : 15;
      return (nr_pdsch_default_time_alloc_B_S[idx]);
      break;

    case NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_C:
      AssertFatal((row_idx!=6)&&(row_idx!=7)&&(row_idx<17), "Invalid row index %d in %s %s\n", row_idx, __FUNCTION__, __FILE__);
      idx = (row_idx<6)? (row_idx-1) : (row_idx<14)? (row_idx-3) : (row_idx == 14)? row_idx-3+((dmrs_typeA_position==2)?0:1) : (row_idx-2);
      break;

  default:
  AssertFatal(0, "Invalid Time domain allocation type %d in %s %s\n", time_alloc_type, __FUNCTION__, __FILE__);
  }
}

void nr_check_time_alloc(uint8_t S, uint8_t L, nfapi_nr_config_request_t config) {

  switch (config.subframe_config.dl_cyclic_prefix_type.value) {
    case NFAPI_CP_NORMAL:
      if (config.pdsch_config.time_domain_alloc_mapping_type.value == NFAPI_NR_PDSCH_MAPPING_TYPE_A) {
        AssertFatal(S<4, "Invalid value of S(%d) for mapping type A and normal CP\n", S);

        if (S==3)
          AssertFatal(config.pdsch_config.dmrs_typeA_position.value == 3, "Invalid S %d for dmrs_typeA_position %d\n",
          S, config.pdsch_config.dmrs_typeA_position);

        AssertFatal((L>2)&&(L<15), "Invalid L %d for mapping type A and normal CP\n", L);

        AssertFatal(((S+L)>2)&&((S+L)<15), "Invalid S+L %d for mapping type A and normal CP\n", S+L);
      }
      else {
        AssertFatal(S<13, "Invalid value of S(%d) for mapping type B and normal CP\n", S);

        AssertFatal((L>1)&&(L<8), "Invalid L %d for mapping type B and normal CP\n", L);

        AssertFatal(((S+L)>1)&&((S+L)<15), "Invalid S+L %d for mapping type B and normal CP\n", S+L);
      }
      break;

    case NFAPI_CP_EXTENDED:
      if (config.pdsch_config.time_domain_alloc_mapping_type.value == NFAPI_NR_PDSCH_MAPPING_TYPE_A) {
        AssertFatal(S<4, "Invalid value of S(%d) for mapping type A and extended CP\n", S);

        if (S==3)
          AssertFatal(config.pdsch_config.dmrs_typeA_position.value == 3, "Invalid S %d for dmrs_typeA_position %d\n",
          S, config.pdsch_config.dmrs_typeA_position);

        AssertFatal((L>2)&&(L<13), "Invalid L %d for mapping type A and extended CP\n", L);

        AssertFatal(((S+L)>2)&&((S+L)<13), "Invalid S+L %d for mapping type A and extended CP\n", S+L);
      }
      else {
        AssertFatal(S<11, "Invalid value of S(%d) for mapping type B and extended CP\n", S);

        AssertFatal((L>1)&&(L<7), "Invalid L %d for mapping type B and extended CP\n", L);

        AssertFatal(((S+L)>1)&&((S+L)<13), "Invalid S+L %d for mapping type B and extended CP\n", S+L);
      }
      break;
  }
}


