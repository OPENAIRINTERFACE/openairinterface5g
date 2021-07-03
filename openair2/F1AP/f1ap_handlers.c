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

/*! \file f1ap_handlers.c
 * \brief f1ap messages handlers
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"
#include "f1ap_handlers.h"
#include "f1ap_decoder.h"
#include "f1ap_cu_interface_management.h"
#include "f1ap_du_interface_management.h"
#include "f1ap_cu_rrc_message_transfer.h"
#include "f1ap_du_rrc_message_transfer.h"
#include "f1ap_cu_ue_context_management.h"
#include "f1ap_du_ue_context_management.h"

extern f1ap_setup_req_t *f1ap_du_data_from_du;

/* Handlers matrix. Only f1 related procedure present here */
f1ap_message_decoded_callback f1ap_messages_callback[][3] = {


  { 0, 0, 0 }, /* Reset */
  { CU_handle_F1_SETUP_REQUEST, DU_handle_F1_SETUP_RESPONSE, DU_handle_F1_SETUP_FAILURE }, /* F1Setup */
  { 0, 0, 0 }, /* ErrorIndication */
  { 0, 0, 0 }, /* gNBDUConfigurationUpdate */
  { DU_handle_gNB_CU_CONFIGURATION_UPDATE, CU_handle_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE, CU_handle_gNB_CU_CONFIGURATION_UPDATE_FAILURE }, /* gNBCUConfigurationUpdate */
  { DU_handle_UE_CONTEXT_SETUP_REQUEST, CU_handle_UE_CONTEXT_SETUP_RESPONSE, 0 }, /* UEContextSetup */
  { DU_handle_UE_CONTEXT_RELEASE_COMMAND, CU_handle_UE_CONTEXT_RELEASE_COMPLETE, 0 }, /* UEContextRelease */
  { 0, 0, 0 }, /* UEContextModification */
  { 0, 0, 0 }, /* UEContextModificationRequired */
  { 0, 0, 0 }, /* UEMobilityCommand */
  { CU_handle_UE_CONTEXT_RELEASE_REQUEST, 0, 0 }, /* UEContextReleaseRequest */
  { CU_handle_INITIAL_UL_RRC_MESSAGE_TRANSFER, 0, 0 }, /* InitialULRRCMessageTransfer */
  { DU_handle_DL_RRC_MESSAGE_TRANSFER, 0, 0 }, /* DLRRCMessageTransfer */
  { CU_handle_UL_RRC_MESSAGE_TRANSFER, 0, 0 }, /* ULRRCMessageTransfer */
  { 0, 0, 0 }, /* privateMessage */
  { 0, 0, 0 }, /* UEInactivityNotification */
  { 0, 0, 0 }, /* GNBDUResourceCoordination */
  { 0, 0, 0 }, /* SystemInformationDeliveryCommand */
  { 0, 0, 0 }, /* Paging */
  { 0, 0, 0 }, /* Notify */
  { 0, 0, 0 }, /* WriteReplaceWarning */
  { 0, 0, 0 }, /* PWSCancel */
  { 0, 0, 0 }, /* PWSRestartIndication */
  { 0, 0, 0 }, /* PWSFailureIndication */
};

const char *f1ap_direction2String(int f1ap_dir) {
  static const char *f1ap_direction_String[] = {
    "", /* Nothing */
    "Initiating message", /* initiating message */
    "Successfull outcome", /* successfull outcome */
    "UnSuccessfull outcome", /* successfull outcome */
  };
  return(f1ap_direction_String[f1ap_dir]);
}

int f1ap_handle_message(instance_t instance, uint32_t assoc_id, int32_t stream,
                        const uint8_t *const data, const uint32_t data_length) {
  F1AP_F1AP_PDU_t pdu= {0};
  int ret;
  DevAssert(data != NULL);

  if (f1ap_decode_pdu(&pdu, data, data_length) < 0) {
    LOG_E(F1AP, "Failed to decode PDU\n");
    return -1;
  }

  /* Checking procedure Code and direction of message */
  if (pdu.choice.initiatingMessage->procedureCode >= sizeof(f1ap_messages_callback) / (3 * sizeof(
        f1ap_message_decoded_callback))
      || (pdu.present > F1AP_F1AP_PDU_PR_unsuccessfulOutcome)) {
    LOG_E(F1AP, "[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
          assoc_id, pdu.choice.initiatingMessage->procedureCode, pdu.present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for eNB (wrong direction).
   */
  if (f1ap_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1] == NULL) {
    LOG_E(F1AP, "[SCTP %d] No handler for procedureCode %ld in %s\n",
          assoc_id, pdu.choice.initiatingMessage->procedureCode,
          f1ap_direction2String(pdu.present - 1));
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
    return -1;
  }

  /* Calling the right handler */
  LOG_I(F1AP, "Calling handler with instance %ld\n",instance);
  ret = (*f1ap_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1])
        (instance, assoc_id, stream, &pdu);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
  return ret;
}
