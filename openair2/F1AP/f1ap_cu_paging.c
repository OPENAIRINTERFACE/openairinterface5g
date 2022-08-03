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

/*! \file f1ap_cu_paging.c
 * \brief f1ap interface paging for CU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"
#include "f1ap_encoder.h"
#include "f1ap_itti_messaging.h"
#include "f1ap_cu_paging.h"

extern f1ap_setup_req_t *f1ap_du_data_from_du;

int CU_send_Paging(instance_t instance, f1ap_paging_ind_t *paging) {
  F1AP_F1AP_PDU_t                 pdu;
  F1AP_Paging_t    *out;
  F1AP_PagingIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_Paging;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_Paging;
  out = &pdu.choice.initiatingMessage->value.choice.Paging;

  /* mandatory */
  /* UEIdentityIndexValue */
  ie = (F1AP_PagingIEs_t *)calloc(1, sizeof(F1AP_PagingIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_UEIdentityIndexValue;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_PagingIEs__value_PR_UEIdentityIndexValue;
  ie->value.choice.UEIdentityIndexValue.present = F1AP_UEIdentityIndexValue_PR_indexLength10;
  UEIDENTITYINDEX_TO_BIT_STRING(paging->ueidentityindexvalue, &ie->value.choice.UEIdentityIndexValue.choice.indexLength10);
  //LOG_D(F1AP, "indexLength10 %d \n", BIT_STRING_to_uint32(&ie->value.choice.UEIdentityIndexValue.choice.indexLength10));
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* PagingIdentity */
  ie = (F1AP_PagingIEs_t *)calloc(1, sizeof(F1AP_PagingIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_PagingIdentity;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_PagingIEs__value_PR_PagingIdentity;
  ie->value.choice.PagingIdentity.present = F1AP_PagingIdentity_PR_cNUEPagingIdentity;
  ie->value.choice.PagingIdentity.choice.cNUEPagingIdentity = (F1AP_CNUEPagingIdentity_t*)calloc(1, sizeof(F1AP_CNUEPagingIdentity_t));
  ie->value.choice.PagingIdentity.choice.cNUEPagingIdentity->present = F1AP_CNUEPagingIdentity_PR_fiveG_S_TMSI;
  FIVEG_S_TMSI_TO_BIT_STRING(paging->fiveg_s_tmsi,
                             &ie->value.choice.PagingIdentity.choice.cNUEPagingIdentity->choice.fiveG_S_TMSI);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* PagingDRX */
  ie = (F1AP_PagingIEs_t *)calloc(1, sizeof(F1AP_PagingIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_PagingDRX;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_PagingIEs__value_PR_PagingDRX;
  ie->value.choice.PagingDRX = paging->paging_drx;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* PagingCell_list */
  ie = (F1AP_PagingIEs_t *)calloc(1, sizeof(F1AP_PagingIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_PagingCell_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_PagingIEs__value_PR_PagingCell_list;
  F1AP_PagingCell_ItemIEs_t *itemies;
  F1AP_PagingCell_Item_t item;
  memset((void *)&item, 0, sizeof(F1AP_PagingCell_Item_t));
  itemies = (F1AP_PagingCell_ItemIEs_t *)calloc(1, sizeof(F1AP_PagingCell_ItemIEs_t));
  itemies->id = F1AP_ProtocolIE_ID_id_PagingCell_Item;
  itemies->criticality = F1AP_Criticality_reject;
  itemies->value.present = F1AP_PagingCell_ItemIEs__value_PR_PagingCell_Item;
  F1AP_NRCGI_t nRCGI;
  MCC_MNC_TO_PLMNID(paging->mcc,
                    paging->mnc,
                    paging->mnc_digit_length,
                    &nRCGI.pLMN_Identity);
  NR_CELL_ID_TO_BIT_STRING(paging->nr_cellid, &nRCGI.nRCellIdentity);
  item.nRCGI = nRCGI;
  itemies->value.choice.PagingCell_Item = item;
  ASN_SEQUENCE_ADD(&ie->value.choice.PagingCell_list, itemies);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 Paging failure\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(true, instance, buffer, len, 0);
  return 0;
}