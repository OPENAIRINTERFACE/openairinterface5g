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

int CU_send_Paging(sctp_assoc_t assoc_id, f1ap_paging_ind_t *paging)
{
  F1AP_F1AP_PDU_t pdu = {0};

  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, msg);
  msg->procedureCode = F1AP_ProcedureCode_id_Paging;
  msg->criticality = F1AP_Criticality_reject;
  msg->value.present = F1AP_InitiatingMessage__value_PR_Paging;

  /* mandatory */
  /* UEIdentityIndexValue */
  {
    asn1cSequenceAdd(msg->value.choice.Paging.protocolIEs.list,
                     F1AP_PagingIEs_t, ie);
    ie->id = F1AP_ProtocolIE_ID_id_UEIdentityIndexValue;
    ie->criticality = F1AP_Criticality_reject;
    ie->value.present = F1AP_PagingIEs__value_PR_UEIdentityIndexValue;
    ie->value.choice.UEIdentityIndexValue.present =
        F1AP_UEIdentityIndexValue_PR_indexLength10;
    UEIDENTITYINDEX_TO_BIT_STRING(
        paging->ueidentityindexvalue,
        &ie->value.choice.UEIdentityIndexValue.choice.indexLength10);
  }

  /* mandatory */
  /* PagingIdentity */
  {
    asn1cSequenceAdd(msg->value.choice.Paging.protocolIEs.list,
                     F1AP_PagingIEs_t, ie);
    ie->id = F1AP_ProtocolIE_ID_id_PagingIdentity;
    ie->criticality = F1AP_Criticality_reject;
    ie->value.present = F1AP_PagingIEs__value_PR_PagingIdentity;
    ie->value.choice.PagingIdentity.present =
        F1AP_PagingIdentity_PR_cNUEPagingIdentity;
    asn1cCalloc(ie->value.choice.PagingIdentity.choice.cNUEPagingIdentity, id);
    id->present = F1AP_CNUEPagingIdentity_PR_fiveG_S_TMSI;
    FIVEG_S_TMSI_TO_BIT_STRING(paging->fiveg_s_tmsi, &id->choice.fiveG_S_TMSI);
  }

  /* optional */
  /* PagingDRX */
  {
    asn1cSequenceAdd(msg->value.choice.Paging.protocolIEs.list,
                     F1AP_PagingIEs_t, ie);
    ie->id = F1AP_ProtocolIE_ID_id_PagingDRX;
    ie->criticality = F1AP_Criticality_ignore;
    ie->value.present = F1AP_PagingIEs__value_PR_PagingDRX;
    ie->value.choice.PagingDRX = paging->paging_drx;
  }

  /* mandatory */
  /* PagingCell_list */
  {
    asn1cSequenceAdd(msg->value.choice.Paging.protocolIEs.list,
                     F1AP_PagingIEs_t, ie);
    ie->id = F1AP_ProtocolIE_ID_id_PagingCell_List;
    ie->criticality = F1AP_Criticality_reject;
    ie->value.present = F1AP_PagingIEs__value_PR_PagingCell_list;
    asn1cSequenceAdd(ie->value.choice.PagingCell_list,
                     F1AP_PagingCell_ItemIEs_t, itemies);
    itemies->id = F1AP_ProtocolIE_ID_id_PagingCell_Item;
    itemies->criticality = F1AP_Criticality_reject;
    itemies->value.present = F1AP_PagingCell_ItemIEs__value_PR_PagingCell_Item;
    F1AP_NRCGI_t *nRCGI = &itemies->value.choice.PagingCell_Item.nRCGI;
    MCC_MNC_TO_PLMNID(paging->plmn.mcc, paging->plmn.mnc, paging->plmn.mnc_digit_length, &nRCGI->pLMN_Identity);
    NR_CELL_ID_TO_BIT_STRING(paging->nr_cellid, &nRCGI->nRCellIdentity);
  }

  uint8_t *buffer;
  uint32_t len;
  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 Paging failure\n");
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
    return -1;
  }
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}
