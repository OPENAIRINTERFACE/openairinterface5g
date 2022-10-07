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

int compfunc(const void *a, const void *b)
{
  return (*(int *)a - *(int *)b);
}

void nr_fill_reg_list(int reg_list[MAX_DCI_CORESET][NR_MAX_PDCCH_AGG_LEVEL * NR_NB_REG_PER_CCE], nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15)
{
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

    int list_idx = 0;
    for (uint8_t cce_idx=0; cce_idx<L; cce_idx++) {
      int cce = pdcch_pdu_rel15->dci_pdu[d].CceIndex + cce_idx;
      LOG_D(PHY, "cce_idx %d\n", cce);
      for (uint8_t bundle_idx=0; bundle_idx<NR_NB_REG_PER_CCE/bsize; bundle_idx++) {
        uint8_t k = 6 * cce / bsize + bundle_idx;
        int f = cce_to_reg_interleaving(R, k, n_shift, C, bsize, N_regs);
        LOG_D(PHY, "Bundle index %d: f(%d) = %d\n", bundle_idx, k, f);
        // reg_list contains the regs to be allocated per symbol
        // the same rbs are allocated in each symbol
        for (uint8_t reg_idx = 0; reg_idx < bsize / dur; reg_idx++) {
          reg_list[d][list_idx] = f * bsize / dur + reg_idx;
          LOG_D(PHY, "rb %d nb of symbols per rb %d start subcarrier %d\n", reg_list[d][list_idx], dur, reg_list[d][list_idx] * NR_NB_SC_PER_RB);
          list_idx++;
        }
      }
    }
    // sorting the elements of the list (smaller goes first)
    qsort(reg_list[d], L * NR_NB_REG_PER_CCE / dur, sizeof(int), compfunc);
  }
}

/*static inline uint64_t dci_field(uint64_t field, uint8_t size) {
  uint64_t ret = 0;
  for (int i=0; i<size; i++)
    ret |= ((field>>i)&1)<<(size-i-1);
  return ret;
}*/
