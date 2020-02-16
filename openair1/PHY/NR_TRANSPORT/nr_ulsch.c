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

/*! \file PHY/NR_TRANSPORT/nr_ulsch.c
* \brief Top-level routines for the reception of the PUSCH TS 38.211 v 15.4.0
* \author Ahmed Hussein
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: ahmed.hussein@iis.fraunhofer.de
* \note
* \warning
*/

#include <stdint.h>
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"

int16_t find_nr_ulsch(uint16_t rnti, PHY_VARS_gNB *gNB,find_type_t type) {

  uint16_t i;
  int16_t first_free_index=-1;

  AssertFatal(gNB!=NULL,"gNB is null\n");
  for (i=0; i<NUMBER_OF_NR_ULSCH_MAX; i++) {
    AssertFatal(gNB->ulsch[i]!=NULL,"gNB->ulsch[%d] is null\n",i);
    AssertFatal(gNB->ulsch[i][0]!=NULL,"gNB->ulsch[%d][0] is null\n",i);
    LOG_D(PHY,"searching for rnti %x : ulsch_index %d=> harq_mask %x, rnti %x, first_free_index %d\n", rnti,i,gNB->ulsch[i][0]->harq_mask,gNB->ulsch[i][0]->rnti,first_free_index);
    if ((gNB->ulsch[i][0]->harq_mask >0) &&
        (gNB->ulsch[i][0]->rnti==rnti))       return i;
    else if ((gNB->ulsch[i][0]->harq_mask == 0) && (first_free_index==-1)) first_free_index=i;
  }
  if (type == SEARCH_EXIST) return -1;
  if (first_free_index != -1)
    gNB->ulsch[first_free_index][0]->rnti = 0;
  return first_free_index;
}

void nr_fill_ulsch(PHY_VARS_gNB *gNB,
                   int frame,
                   int slot,
                   nfapi_nr_pusch_pdu_t *ulsch_pdu) {

 
  int ulsch_id = find_nr_ulsch(ulsch_pdu->rnti,gNB,SEARCH_EXIST_OR_FREE);
  AssertFatal( (ulsch_id>=0) && (ulsch_id<NUMBER_OF_NR_ULSCH_MAX),
              "illegal or no ulsch_id found!!! rnti %04x ulsch_id %d\n",ulsch_pdu->rnti,ulsch_id);

  NR_gNB_ULSCH_t  *ulsch = gNB->ulsch[ulsch_id][0];
  int harq_pid = ulsch_pdu->pusch_data.harq_process_id;
  ulsch->rnti = ulsch_pdu->rnti;
  //ulsch->rnti_type;
  ulsch->harq_mask |= 1<<harq_pid;
  ulsch->harq_process_id[slot] = harq_pid;

  ulsch->harq_processes[harq_pid]->frame=frame;
  ulsch->harq_processes[harq_pid]->slot=slot;
  ulsch->harq_processes[harq_pid]->handled= 0;
  ulsch->harq_processes[harq_pid]->status= NR_ACTIVE;
  nfapi_nr_ul_config_ulsch_pdu *rel15_ul = &ulsch->harq_processes[harq_pid]->ulsch_pdu;

  LOG_D(PHY,"Initializing nFAPI for ULSCH, UE %d, harq_pid %d\n",ulsch_id,harq_pid);
 
  
  //FK this is still a bad hack. We need to replace the L1 FAPI structures with the new scf ones as well.
  rel15_ul->rnti                           = ulsch_pdu->rnti;
  rel15_ul->ulsch_pdu_rel15.start_rb       = ulsch_pdu->rb_start;
  rel15_ul->ulsch_pdu_rel15.number_rbs     = ulsch_pdu->rb_size;
  rel15_ul->ulsch_pdu_rel15.start_symbol   = ulsch_pdu->start_symbol_index;
  rel15_ul->ulsch_pdu_rel15.number_symbols = ulsch_pdu->nr_of_symbols;
  rel15_ul->ulsch_pdu_rel15.length_dmrs    = gNB->dmrs_UplinkConfig.pusch_maxLength;
  rel15_ul->ulsch_pdu_rel15.Qm             = ulsch_pdu->qam_mod_order;
  rel15_ul->ulsch_pdu_rel15.mcs            = ulsch_pdu->mcs_index;
  rel15_ul->ulsch_pdu_rel15.rv             = ulsch_pdu->pusch_data.rv_index;
  rel15_ul->ulsch_pdu_rel15.n_layers       = ulsch_pdu->nrOfLayers;
  rel15_ul->ulsch_pdu_rel15.R              = ulsch_pdu->target_code_rate;

}

void nr_ulsch_unscrambling(int16_t* llr,
                         uint32_t size,
                         uint8_t q,
                         uint32_t Nid,
                         uint32_t n_RNTI) 
{
  uint8_t reset;
  uint32_t x1, x2, s=0;

  reset = 1;
  x2 = (n_RNTI<<15) + Nid;

  for (uint32_t i=0; i<size; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
    }
    if (((s>>(i&0x1f))&1)==1)
      llr[i] = -llr[i];
  }
}
