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


void nr_fill_cce_list(nr_cce_t cce_list[MAX_DCI_CORESET][NR_MAX_PDCCH_AGG_LEVEL], nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15) {

  nr_cce_t* cce;
  nr_reg_t* reg;

  int bsize = pdcch_pdu_rel15->RegBundleSize;
  int R = pdcch_pdu_rel15->InterleaverSize;
  int n_shift = pdcch_pdu_rel15->ShiftIndex;

  //Max number of candidates per aggregation level -- SIB1 configured search space only

  int n_rb,rb_offset;

  get_coreset_rballoc(pdcch_pdu_rel15->FreqDomainResource,&n_rb,&rb_offset);

  for (int d=0;d<pdcch_pdu_rel15->numDlDci;d++) {

    int  L = pdcch_pdu_rel15->dci_pdu[d].AggregationLevel;
    int dur = pdcch_pdu_rel15->DurationSymbols;
    int N_regs = n_rb*dur; // nb of REGs per coreset
    AssertFatal(N_regs > 0,"N_reg cannot be 0\n");

    if (pdcch_pdu_rel15->CoreSetType == NFAPI_NR_CSET_CONFIG_MIB_SIB1)
      AssertFatal(L>=4, "Invalid aggregation level for SIB1 configured PDCCH %d\n", L);

    int C = 0;

    if (pdcch_pdu_rel15->CceRegMappingType == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
      uint16_t assertFatalCond = (N_regs%(bsize*R));
      AssertFatal(assertFatalCond == 0,"CCE to REG interleaving: Invalid configuration leading to non integer C (N_reg %us, bsize %d R %d)\n",N_regs, bsize, R);
      C = N_regs/(bsize*R);
    }

    if (pdcch_pdu_rel15->dci_pdu[d].RNTI != 0xFFFF)
      LOG_D(PHY, "CCE list generation for candidate %d: bundle size %d ilv size %d CceIndex %d\n", d, bsize, R, pdcch_pdu_rel15->dci_pdu[d].CceIndex);

    for (uint8_t cce_idx=0; cce_idx<L; cce_idx++) {
      cce = &cce_list[d][cce_idx];
      cce->cce_idx = pdcch_pdu_rel15->dci_pdu[d].CceIndex + cce_idx;
      LOG_D(PHY, "cce_idx %d\n", cce->cce_idx);

      uint8_t j = cce->cce_idx;
      for (int k=6*j/bsize; k<(6*j/bsize+6/bsize); k++) { // loop over REG bundles

        int f = cce_to_reg_interleaving(R, k, n_shift, C, bsize, N_regs);

	for (uint8_t reg_idx=0; reg_idx<bsize; reg_idx++) {
	  reg = &cce->reg_list[reg_idx];
	  reg->reg_idx = bsize*f + reg_idx;
	  reg->start_sc_idx = (reg->reg_idx/dur) * NR_NB_SC_PER_RB;
	  reg->symb_idx = reg_idx%dur;
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
