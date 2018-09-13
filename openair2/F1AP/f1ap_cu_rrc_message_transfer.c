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

/*! \file f1ap_cu_rrc_message_transfer.c
 * \brief f1ap rrc message transfer for CU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"
#include "f1ap_cu_rrc_message_transfer.h"

/*
    Initial UL RRC Message Transfer
*/

void CU_handle_UL_INITIAL_RRC_MESSAGE_TRANSFER(void) {

  printf("CU_handle_UL_INITIAL_RRC_MESSAGE_TRANSFER\n");
  // decode the F1 message
  // get the rrc message from the contauiner 
  // call func rrc_eNB_decode_ccch: <-- needs some update here

  // if size > 0 
  // CU_send_DL_RRC_MESSAGE_TRANSFER(C.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Payload, RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size)
}


/*
    DL RRC Message Transfer.
*/

//void CU_send_DL_RRC_MESSAGE_TRANSFER(F1AP_DLRRCMessageTransfer_t *DLRRCMessageTransfer) {
void CU_send_DL_RRC_MESSAGE_TRANSFER(void) {
  F1AP_F1AP_PDU_t                pdu;
  F1AP_DLRRCMessageTransfer_t    *out;
  F1AP_DLRRCMessageTransferIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_DLRRCMessageTransfer;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_DLRRCMessageTransfer;
  out = &pdu.choice.initiatingMessage->value.choice.DLRRCMessageTransfer;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_DLRRCMessageTransferIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_DLRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = 651L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c3. oldgNB_DU_UE_F1AP_ID */
  if (0) {
    ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
    ie->id                            = F1AP_ProtocolIE_ID_id_oldgNB_DU_UE_F1AP_ID;
    ie->criticality                   = F1AP_Criticality_reject;
    //ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_NOTHING;
    //ie->value.choice.            = 1;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c4. SRBID */
  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_SRBID;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_SRBID;
  ie->value.choice.SRBID            = 2L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c5. ExecuteDuplication */
  if (0) {
    ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
    ie->id                            = F1AP_ProtocolIE_ID_id_ExecuteDuplication;
    ie->criticality                   = F1AP_Criticality_ignore;
    ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_ExecuteDuplication;
    ie->value.choice.ExecuteDuplication = F1AP_ExecuteDuplication_true;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  // issue in here
  /* mandatory */
  /* c6. RRCContainer */
  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, "A", strlen("A"));
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c7. RAT_FrequencyPriorityInformation */
  if (0) {
    ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
    ie->id                            = F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation;
    ie->criticality                   = F1AP_Criticality_reject;
    ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_RAT_FrequencyPriorityInformation;

    ie->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_subscriberProfileIDforRFP;
    ie->value.choice.RAT_FrequencyPriorityInformation.choice.subscriberProfileIDforRFP = 123L;

    //ie->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_rAT_FrequencySelectionPriority;
    //ie->value.choice.RAT_FrequencyPriorityInformation.choice.rAT_FrequencySelectionPriority = 123L;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
  }

  printf("\n");

  /* decode */
  if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
    printf("Failed to decode F1 setup request\n");
  }
  //AssertFatal(1==0,"Not implemented yet\n");
}

/*
    UL RRC Message Transfer
*/

void CU_handle_UL_RRC_MESSAGE_TRANSFER(F1AP_ULRRCMessageTransfer_t *ULRRCMessageTransfer) {
  AssertFatal(1==0,"Not implemented yet\n");
}
