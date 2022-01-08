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

/*! \file PHY/NR_TRANSPORT/nr_dci_tools.c
 * \brief
 * \author
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email:
 * \note
 * \warning
 */

#include "nr_dci.h"
#include "common/utils/nr/nr_common.h"

//#define DEBUG_FILL_DCI

#include "nr_dlsch.h"

/*
  Original version to keep code for Y that needs to be moved to MAC

void nr_fill_cce_list(PHY_VARS_gNB *gNB, uint16_t n_shift, uint8_t m) {

  nr_cce_t* cce;
  nr_reg_t* reg;
  nfapi_nr_dl_config_pdcch_pdu_rel15_t* pdcch_pdu = gNB->pdcch_pdu.pdcch;

  uint8_t L = dci_alloc->L;
  uint8_t bsize = pdcch_params->reg_bundle_size;
  uint8_t R = pdcch_params->interleaver_size;
  uint16_t N_reg = pdcch_params->n_rb * pdcch_params->n_symb;
  uint16_t Y, N_cce, M_s_max, n_CI=0, tmp, C=0;
  uint16_t n_RNTI = (pdcch_params->search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC)? pdcch_params->rnti:0;
  uint32_t A[3]={39827,39829,39839};

  if (pdcch_params->config_type == NFAPI_NR_CSET_CONFIG_MIB_SIB1)
    AssertFatal(L>=4, "Invalid aggregation level for SIB1 configured PDCCH %d\n", L);

  N_cce = N_reg / NR_NB_REG_PER_CCE;
  //Max number of candidates per aggregation level -- SIB1 configured search space only
  M_s_max = (L==4)?4:(L==8)?2:1;

  if (pdcch_params->search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_COMMON)
    Y = 0;
  else { //NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC
    Y = (A[0]*n_RNTI)%65537; // Candidate 0, antenna port 0
  }

  if (pdcch_params->cr_mapping_type == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
	  uint16_t assertFatalCond = (N_reg%(bsize*R));
	  AssertFatal(assertFatalCond == 0,"CCE to REG interleaving: Invalid configuration leading to non integer C (N_reg %us, bsize %d R %d)\n",N_reg, bsize, R);
	  C = N_reg/(bsize*R);
  }

  tmp = L * (( Y + (m*N_cce)/(L*M_s_max) + n_CI ) % CEILIDIV(N_cce,L));

  LOG_D(PHY, "CCE list generation for candidate %d: bundle size %d ilv size %d tmp %d\n", m, bsize, R, tmp);
  for (uint8_t cce_idx=0; cce_idx<L; cce_idx++) {
    cce = &dci_alloc->cce_list[cce_idx];
    cce->cce_idx = tmp + cce_idx;
    LOG_D(PHY, "cce_idx %d\n", cce->cce_idx);

    if (pdcch_params->cr_mapping_type == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
      LOG_D(PHY, "Interleaved CCE to REG mapping\n");
      uint8_t j = cce->cce_idx, j_prime;
      uint8_t r,c,idx;

      for (uint8_t bundle_idx=0; bundle_idx<NR_NB_REG_PER_CCE/bsize; bundle_idx++) {
        j_prime = 6*j/bsize + bundle_idx;
        r = j_prime%R;
        c = (j_prime-r)/R;
        idx = (r*C + c + n_shift)%(N_reg/bsize);
        LOG_D(PHY, "bundle idx = %d \n j = %d \t j_prime = %d \t r = %d \t c = %d\n", idx, j , j_prime, r, c);

        for (uint8_t reg_idx=0; reg_idx<bsize; reg_idx++) {
          reg = &cce->reg_list[reg_idx];
          reg->reg_idx = bsize*idx + reg_idx;
          reg->start_sc_idx = (reg->reg_idx/pdcch_params->n_symb) * NR_NB_SC_PER_RB;
          reg->symb_idx = reg->reg_idx % pdcch_params->n_symb;
          LOG_D(PHY, "reg %d symbol %d start subcarrier %d\n", reg->reg_idx, reg->symb_idx, reg->start_sc_idx);
        }
      }
    }
    else { // NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED
      LOG_D(PHY, "Non interleaved CCE to REG mapping\n");
      for (uint8_t reg_idx=0; reg_idx<NR_NB_REG_PER_CCE; reg_idx++) {
        reg = &cce->reg_list[reg_idx];
        reg->reg_idx = cce->cce_idx*NR_NB_REG_PER_CCE + reg_idx;
        reg->start_sc_idx = (reg->reg_idx/pdcch_params->n_symb) * NR_NB_SC_PER_RB;
        reg->symb_idx = reg->reg_idx % pdcch_params->n_symb;
        LOG_D(PHY, "reg %d symbol %d start subcarrier %d\n", reg->reg_idx, reg->symb_idx, reg->start_sc_idx);
      }

    }

  }
}

*/

void nr_fill_cce_list(nr_cce_t cce_list[MAX_DCI_CORESET][NR_MAX_PDCCH_AGG_LEVEL], uint8_t m,  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15) {

  nr_cce_t* cce;
  nr_reg_t* reg;

  int bsize = pdcch_pdu_rel15->RegBundleSize;
  int R = pdcch_pdu_rel15->InterleaverSize;
  int n_shift = pdcch_pdu_rel15->ShiftIndex;


  //Max number of candidates per aggregation level -- SIB1 configured search space only


  int n_rb,rb_offset;

  get_coreset_rballoc(pdcch_pdu_rel15->FreqDomainResource,&n_rb,&rb_offset);


  int N_reg = n_rb;
  int C=-1;

  AssertFatal(N_reg > 0,"N_reg cannot be 0\n");

  for (int d=0;d<pdcch_pdu_rel15->numDlDci;d++) {
    int  L = pdcch_pdu_rel15->dci_pdu[d].AggregationLevel;

    if (pdcch_pdu_rel15->CoreSetType == NFAPI_NR_CSET_CONFIG_MIB_SIB1)
      AssertFatal(L>=4, "Invalid aggregation level for SIB1 configured PDCCH %d\n", L);
    
    if (pdcch_pdu_rel15->CceRegMappingType == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
      uint16_t assertFatalCond = (N_reg%(bsize*R));
      AssertFatal(assertFatalCond == 0,"CCE to REG interleaving: Invalid configuration leading to non integer C (N_reg %us, bsize %d R %d)\n",N_reg, bsize, R);
      C = N_reg/(bsize*R);
    }
    
    if (pdcch_pdu_rel15->dci_pdu[d].RNTI != 0xFFFF) LOG_D(PHY, "CCE list generation for candidate %d: bundle size %d ilv size %d CceIndex %d\n", m, bsize, R, pdcch_pdu_rel15->dci_pdu[d].CceIndex);
    for (uint8_t cce_idx=0; cce_idx<L; cce_idx++) {
      cce = &cce_list[d][cce_idx];
      cce->cce_idx = pdcch_pdu_rel15->dci_pdu[d].CceIndex + cce_idx;
      LOG_D(PHY, "cce_idx %d\n", cce->cce_idx);
      
      if (pdcch_pdu_rel15->CceRegMappingType == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
	LOG_D(PHY, "Interleaved CCE to REG mapping\n");
	uint8_t j = cce->cce_idx, j_prime;
	uint8_t r,c,idx;
	
	for (uint8_t bundle_idx=0; bundle_idx<NR_NB_REG_PER_CCE/bsize; bundle_idx++) {
	  j_prime = 6*j/bsize + bundle_idx;
	  r = j_prime%R;
	  c = (j_prime-r)/R;
	  idx = (r*C + c + n_shift)%(N_reg/bsize);
	  LOG_D(PHY, "bundle idx = %d \n j = %d \t j_prime = %d \t r = %d \t c = %d\n", idx, j , j_prime, r, c);
	  
	  for (uint8_t reg_idx=0; reg_idx<bsize; reg_idx++) {
	    reg = &cce->reg_list[reg_idx];
	    reg->reg_idx = bsize*idx + reg_idx;
	    reg->start_sc_idx = reg->reg_idx * NR_NB_SC_PER_RB;
	    reg->symb_idx = 0;
	    LOG_D(PHY, "reg %d symbol %d start subcarrier %d\n", reg->reg_idx, reg->symb_idx, reg->start_sc_idx);
	  }
	}
      }
      else { // NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED
	LOG_D(PHY, "Non interleaved CCE to REG mapping\n");
	for (uint8_t reg_idx=0; reg_idx<NR_NB_REG_PER_CCE; reg_idx++) {
	  reg = &cce->reg_list[reg_idx];
	  reg->reg_idx = cce->cce_idx*NR_NB_REG_PER_CCE + reg_idx;
	  reg->start_sc_idx = reg->reg_idx * NR_NB_SC_PER_RB;
	  reg->symb_idx = 0;
	  LOG_D(PHY, "reg %d symbol %d start subcarrier %d\n", reg->reg_idx, reg->symb_idx, reg->start_sc_idx);
	}
	
      }
      
    }
  }
}

/*static inline uint64_t dci_field(uint64_t field, uint8_t size) {
  uint64_t ret = 0;
  for (int i=0; i<size; i++)
    ret |= ((field>>i)&1)<<(size-i-1);
  return ret;
}*/
