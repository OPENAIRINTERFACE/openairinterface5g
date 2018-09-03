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

void nr_get_time_domain_allocation_type(nfapi_nr_config_request_t config,
                                        NR_gNB_DCI_ALLOC_t dci_alloc,
                                        NR_gNB_DLSCH_t* dlsch) {

  nfapi_nr_ssb_and_cset_mux_pattern_type_e mux_pattern = dci_alloc.mux_pattern;

  switch(dci_alloc.rnti_type) {    

    case NFAPI_NR_RNTI_SI:
      AssertFatal(dci_alloc.pdcch_params.common_search_space_type == NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_0,
      "Invalid common search space type %d for SI RNTI, expected %d\n",
      dci_alloc.pdcch_params.common_search_space_type, NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_0);
      dlsch->time_alloc_type = (mux_pattern == NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1)?NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A :
                               (mux_pattern == NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2)?NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_B :
                               (mux_pattern == NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE3)?NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_C;
      break;

    case NFAPI_NR_RNTI_RA:
    case NFAPI_NR_RNTI_TC:
      /*AssertFatal(dci_alloc.pdcch_params.common_search_space_type == NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_1,
      "Invalid common search space type %d for RNTI %d, expected %d\n",dci_alloc.pdcch_params.common_search_space_type,
      NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_1, dci_alloc.rnti_type);*/
      if (!dlsch->time_alloc_list_flag)
        dlsch->time_alloc_type = NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A;
      break;

    case NFAPI_NR_RNTI_P:
      break;

    case NFAPI_NR_RNTI_C:
    case NFAPI_NR_RNTI_CS:
      if ()
      break;

    case


  }
}


