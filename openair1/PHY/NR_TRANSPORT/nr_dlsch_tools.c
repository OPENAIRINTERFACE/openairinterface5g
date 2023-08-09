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
#include "../../../nfapi/oai_integration/vendor_ext.h"

extern void set_taus_seed(unsigned int seed_type);


/// Payload emulation
void nr_emulate_dlsch_payload(uint8_t* pdu, uint16_t size) {
  set_taus_seed(0);
  for (int i=0; i<size; i++)
    *(pdu+i) = (uint8_t)rand();
}

void nr_fill_dlsch(processingData_L1tx_t *msgTx, nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu, uint8_t *sdu)
{
  NR_gNB_DLSCH_t *dlsch = &msgTx->dlsch[msgTx->num_pdsch_slot][0];
  NR_DL_gNB_HARQ_t *harq = &dlsch->harq_process;
  /// DLSCH struct
  memcpy((void*)&harq->pdsch_pdu, (void*)pdsch_pdu, sizeof(nfapi_nr_dl_tti_pdsch_pdu));
  msgTx->num_pdsch_slot++;
  AssertFatal(sdu != NULL, "sdu is null\n");
  harq->pdu = sdu;
}
