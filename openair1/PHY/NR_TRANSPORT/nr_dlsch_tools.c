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

/*! \file PHY/NR_TRANSPORT/nr_dlsch_tools.c
* \brief Top-level routines for decoding  DLSCH transport channel from 38-214, V15.2.0 2018-06
* \author Guy De Souza
* \date 2018
* \version 0.1
* \company Eurecom
* \email: desouza@eurecom.fr
* \note

* \warning
*/

#include "nr_dlsch.h"

extern void set_taus_seed(unsigned int seed_type);

uint8_t nr_pdsch_default_time_alloc_A_S_nCP[23] = {2,3,2,3,2,3,2,3,2,3,9,10,4,6,5,5,9,12,1,1,2,4,8};
uint8_t nr_pdsch_default_time_alloc_A_L_nCP[23] = {12,11,10,9,9,8,7,6,5,4,4,4,4,4,7,2,2,2,13,6,4,7,4};
uint8_t nr_pdsch_default_time_alloc_A_S_eCP[23] = {2,3,2,3,2,3,2,3,2,3,6,8,4,6,5,5,9,10,1,1,2,4,8};
uint8_t nr_pdsch_default_time_alloc_A_L_eCP[23] = {6,5,10,9,9,8,7,6,5,4,4,2,4,4,6,2,2,2,11,6,4,6,4};
uint8_t nr_pdsch_default_time_alloc_B_S[16] = {2,4,6,8,10,2,4,2,4,6,8,10,2,2,3,2};
uint8_t nr_pdsch_default_time_alloc_B_L[16] = {2,2,2,2,2,2,2,4,4,4,4,4,7,12,11,4};
uint8_t nr_pdsch_default_time_alloc_C_S[15] = {2,4,6,8,10,2,4,6,8,10,2,2,3,0,2};
uint8_t nr_pdsch_default_time_alloc_C_L[15] = {2,2,2,2,2,4,4,4,4,4,7,12,11,6,6};


  /// Time domain allocation routines

/*
void nr_get_time_domain_allocation_type(nfapi_nr_config_request_t config,
                                        nfapi_nr_dl_config_dci_dl_pdu dci_pdu,
                                        nfapi_nr_dl_config_dlsch_pdu *dlsch_pdu) {

  nfapi_nr_dl_config_pdcch_parameters_rel15_t params_rel15 = dci_pdu.pdcch_params_rel15;
  uint8_t *alloc_type = &dlsch_pdu->dlsch_pdu_rel15.time_allocation_type;
  uint8_t mux_pattern = params_rel15.mux_pattern;
  uint8_t alloc_list_flag = dlsch_pdu->dlsch_pdu_rel15.time_alloc_list_flag;

  switch(params_rel15.rnti_type) {

    case NFAPI_NR_RNTI_SI:
      AssertFatal(params_rel15.common_search_space_type == NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_0,
      "Invalid common search space type %d for SI RNTI, expected %d\n",
      params_rel15.common_search_space_type, NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_0);

      if (mux_pattern == NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1)
        AssertFatal(config.subframe_config.dl_cyclic_prefix_type.value == NFAPI_CP_NORMAL,
        "Invalid configuration CP extended for SI RNTI type 0 search space\n");

      *alloc_type = (mux_pattern == NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1)?NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A :
                               (mux_pattern == NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2)?NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_B :
                               NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_C;
      break;

    case NFAPI_NR_RNTI_RA:
    case NFAPI_NR_RNTI_TC:
      //AssertFatal(dci_alloc.pdcch_params.common_search_space_type == NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_1,
      //"Invalid common search space type %d for RNTI %d, expected %d\n",dci_alloc.pdcch_params.common_search_space_type,
      //NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_1, dci_alloc.rnti_type);
      *alloc_type = (alloc_list_flag) ? NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_ALLOC_LIST : NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A;
      break;

    case NFAPI_NR_RNTI_P:
      break;

    case NFAPI_NR_RNTI_C:
    case NFAPI_NR_RNTI_CS:
      if (params_rel15.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_COMMON)
        *alloc_type = (alloc_list_flag)? NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_ALLOC_LIST : NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A;
      else
        *alloc_type = NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_ALLOC_LIST;

      break;

  }
}


uint16_t get_SLIV(uint8_t S, uint8_t L) {
  return ( (uint16_t)(((L-1)<=7)? (14*(L-1)+S) : (14*(15-L)+(13-S))) );
}

static inline uint8_t get_K0(uint8_t row_idx, uint8_t time_alloc_type) {
  return ( (time_alloc_type == NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A)||
  (time_alloc_type == NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_C)? 0 :
  ((row_idx==6)||(row_idx==7)||(row_idx==15))? 1 : 0);
}

// ideally combine the calculation of L in the same function once the right struct is defined
uint8_t nr_get_S(uint8_t row_idx, uint8_t CP, uint8_t time_alloc_type, uint8_t dmrs_TypeA_Position) {

  uint8_t idx;
  //uint8_t S;

  switch(time_alloc_type) {
    case NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A:
      idx = (row_idx>7)? (row_idx+6) : (((row_idx-1)<<1)-1+((dmrs_TypeA_Position==2)?0:1));
      return ((CP==NFAPI_CP_NORMAL)?nr_pdsch_default_time_alloc_A_S_nCP[idx] : nr_pdsch_default_time_alloc_A_S_eCP[idx]);
      break;

    case NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_B:
      idx = (row_idx<14)? (row_idx-1) : (row_idx == 14)? row_idx-1+((dmrs_TypeA_Position==2)?0:1) : 15;
      return (nr_pdsch_default_time_alloc_B_S[idx]);
      break;

    case NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_C:
      AssertFatal((row_idx!=6)&&(row_idx!=7)&&(row_idx<17), "Invalid row index %d in %s %s\n", row_idx, __FUNCTION__, __FILE__);
      idx = (row_idx<6)? (row_idx-1) : (row_idx<14)? (row_idx-3) : (row_idx == 14)? row_idx-3+((dmrs_TypeA_Position==2)?0:1) : (row_idx-2);
      break;

  default:
  AssertFatal(0, "Invalid Time domain allocation type %d in %s %s\n", time_alloc_type, __FUNCTION__, __FILE__);
  }
  return 0; // temp warning fix
}


void nr_check_time_alloc(uint8_t S, uint8_t L,nfapi_nr_dl_config_dlsch_pdu_rel15_t *rel15,nfapi_nr_config_request_t *cfg) {

  switch (cfg->subframe_config.dl_cyclic_prefix_type.value) {
    case NFAPI_CP_NORMAL:
      if (rel15->mapping_type == NFAPI_NR_PDSCH_MAPPING_TYPE_A) {
        AssertFatal(S<4, "Invalid value of S(%d) for mapping type A and normal CP\n", S);

        if (S==3)
          AssertFatal(rel15->dmrs_TypeA_Position == 3, "Invalid S %d for dmrs_TypeA_Position %d\n",
          S, rel15->dmrs_TypeA_Position);

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
      if (rel15->mapping_type == NFAPI_NR_PDSCH_MAPPING_TYPE_A) {
        AssertFatal(S<4, "Invalid value of S(%d) for mapping type A and extended CP\n", S);

        if (S==3)
          AssertFatal(rel15->dmrs_TypeA_Position == 3, "Invalid S %d for dmrs_TypeA_Position %d\n",
          S, rel15->dmrs_TypeA_Position);

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


  /// Frequency domain allocation routines

    // DL alloc type 0
static inline uint8_t get_RBG_size_P(uint16_t n_RB, uint8_t RBG_config) {
  if (RBG_config == NFAPI_NR_PDSCH_RBG_CONFIG_TYPE1)
    return ((n_RB<37)?2:(n_RB<73)?4:(n_RB<145)?8:16);
  else if (RBG_config == NFAPI_NR_PDSCH_RBG_CONFIG_TYPE2)
    return ((n_RB<37)?4:(n_RB<73)?8:(n_RB<145)?16:16);
  else
    AssertFatal(0, "Invalid RBG config type (%d)\n", RBG_config);
}

void nr_get_rbg_parms(NR_BWP_PARMS* bwp, uint8_t config_type) {
  nr_rbg_parms_t* rbg_parms = &bwp->rbg_parms;

  rbg_parms->P = get_RBG_size_P(bwp->N_RB, config_type);
  rbg_parms->start_size = rbg_parms->P - bwp->location%rbg_parms->P;
  rbg_parms->end_size = ((bwp->location + bwp->N_RB)%rbg_parms->P)? ((bwp->location + bwp->N_RB)%rbg_parms->P) : rbg_parms->P;
  rbg_parms->N_RBG = (uint8_t)ceil( (bwp->N_RB + (bwp->location%rbg_parms->P))/(float)rbg_parms->P);
  LOG_I(PHY, "RBG parameters for BWP %d location %d N_RB %d:\n", bwp->bwp_id, bwp->location, bwp->N_RB);
  LOG_I(PHY, "P %d\t start size %d\t endsize %d\t N_RBG %d\n", rbg_parms->P, rbg_parms->start_size, rbg_parms->end_size, rbg_parms->N_RBG);
}

void nr_get_rbg_list(uint32_t bitmap, uint8_t n_rbg, uint8_t* rbg_list) {
  uint8_t idx=0;
  for (int i=0; i<n_rbg; i++)
    if ((bitmap>>(n_rbg-i-1))&1)
      rbg_list[idx++]=i;
}

    // DL alloc type 1

uint16_t get_RIV(uint16_t rb_start, uint16_t L, uint16_t N_RB) {
  if ((L-1)<=(N_RB>>1))
    return (N_RB*(L-1)+rb_start);
  else
    return (N_RB*(N_RB-L+1) + (N_RB-1-rb_start));
}

  /// PRB bundling routines

// Precoding granularity
static inline uint8_t nr_get_P_prime(uint8_t rnti_type, uint8_t dci_format, uint8_t prb_bundling_type) {
  if (dci_format == NFAPI_NR_DL_DCI_FORMAT_1_0)
    return (NFAPI_NR_PRG_GRANULARITY_2);
  else // NFAPI_NR_DL_DCI_FORMAT_1_1
    return ((prb_bundling_type)?0:2);// incomplete for 1_1
}

void nr_get_PRG_parms(NR_BWP_PARMS* bwp, NR_gNB_DCI_ALLOC_t dci_alloc, uint8_t prb_bundling_type) {
  nr_prg_parms_t* prg_parms = &bwp->prg_parms;

  prg_parms->P_prime = nr_get_P_prime(dci_alloc.pdcch_params.rnti_type, dci_alloc.pdcch_params.dci_format, prb_bundling_type);
  prg_parms->start_size = prg_parms->P_prime - bwp->location%prg_parms->P_prime;
  prg_parms->end_size = (bwp->location + bwp->N_RB)%prg_parms->P_prime;
  if (!prg_parms->end_size)
    prg_parms->end_size = prg_parms->P_prime;
  prg_parms->N_PRG = ceil((float)bwp->N_RB/prg_parms->P_prime);
  LOG_I(PHY, "PRG parameters for BWP %d location %d N_RB %d:\n", bwp->bwp_id, bwp->location, bwp->N_RB);
  LOG_I(PHY, "P_prime %d\t start size %d\t endsize %d\t N_PRG %d\n", prg_parms->P_prime, prg_parms->start_size, prg_parms->end_size, prg_parms->N_PRG);
}
*/

  /// Payload emulation
void nr_emulate_dlsch_payload(uint8_t* pdu, uint16_t size) {
  set_taus_seed(0);
  for (int i=0; i<size; i++)
    *(pdu+i) = (uint8_t)rand();
}

void nr_fill_dlsch(processingData_L1tx_t *msgTx,
                   nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
                   uint8_t *sdu) {

  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *rel15 = &pdsch_pdu->pdsch_pdu_rel15;
 
  AssertFatal(msgTx->num_pdsch_slot < NUMBER_OF_NR_DLSCH_MAX,
              "Number of PDSCH PDUs %d exceeded the limit\n",msgTx->num_pdsch_slot);
  NR_gNB_DLSCH_t  *dlsch = msgTx->dlsch[msgTx->num_pdsch_slot][0];
  NR_DL_gNB_HARQ_t *harq  = &dlsch->harq_process;
  /// DLSCH struct
  memcpy((void*)&harq->pdsch_pdu, (void*)pdsch_pdu, sizeof(nfapi_nr_dl_tti_pdsch_pdu));
  msgTx->num_pdsch_slot++;
  AssertFatal(sdu!=NULL,"sdu is null\n");
  harq->pdu = sdu;
}

