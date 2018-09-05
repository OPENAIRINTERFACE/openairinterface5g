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

#include <stdint.h>

#include "intertask_interface.h"

#include "asn1_conversions.h"

#include "f1ap_common.h"
// #include "f1ap_eNB.h"
#include "f1ap_handlers.h"
#include "f1ap_decoder.h"

#include "f1ap_default_values.h"

#include "assertions.h"
#include "conversions.h"
#include "msc.h"

static
int f1ap_handle_f1_setup_request(uint32_t               assoc_id,
                                 uint32_t               stream,
                                 F1AP_F1AP_PDU_t        *pdu);


/* Handlers matrix. Only f1 related procedure present here */
f1ap_message_decoded_callback messages_callback[][3] = {
  

  { 0, 0, 0 }, /* Reset */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* F1Setup */
  { 0, 0, 0 }, /* ErrorIndication */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* gNBDUConfigurationUpdate */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* gNBCUConfigurationUpdate */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* UEContextSetup */
  { 0, 0, 0 }, /* UEContextRelease */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* UEContextModification */
  { 0, 0, 0 }, /* UEContextModificationRequired */
  { 0, 0, 0 }, /* UEMobilityCommand */
  { 0, 0, 0 }, /* UEContextReleaseRequest */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* InitialULRRCMessageTransfer */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* DLRRCMessageTransfer */
  { f1ap_handle_f1_setup_request, 0, 0 }, /* ULRRCMessageTransfer */
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

int f1ap_handle_message(uint32_t assoc_id, int32_t stream,
                            const uint8_t * const data, const uint32_t data_length)
{
  F1AP_F1AP_PDU_t pdu;
  int ret;

  DevAssert(data != NULL);

  memset(&pdu, 0, sizeof(pdu));

  if (f1ap_decode_pdu(&pdu, data, data_length) < 0) {
    //F1AP_ERROR("Failed to decode PDU\n");
    printf("Failed to decode PDU\n");
    return -1;
  }

  /* Checking procedure Code and direction of message */
  if (pdu.choice.initiatingMessage->procedureCode > sizeof(messages_callback) / (3 * sizeof(
        f1ap_message_decoded_callback))
      || (pdu.present > F1AP_F1AP_PDU_PR_unsuccessfulOutcome)) {
    //F1AP_ERROR("[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
    //           assoc_id, pdu.choice.initiatingMessage->procedureCode, pdu.present);
    printf("[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, pdu.choice.initiatingMessage->procedureCode, pdu.present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for eNB (wrong direction).
   */
  if (messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1] == NULL) {
    // F1AP_ERROR("[SCTP %d] No handler for procedureCode %ld in %s\n",
    //            assoc_id, pdu.choice.initiatingMessage->procedureCode,
    //            f1ap_direction2String(pdu.present - 1));
    printf("[SCTP %d] No handler for procedureCode %ld in %s\n",
                assoc_id, pdu.choice.initiatingMessage->procedureCode,
               f1ap_direction2String(pdu.present - 1));
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
    return -1;
  }

  /* Calling the right handler */
  ret = (*messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1])
        (assoc_id, stream, &pdu);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_F1AP_F1AP_PDU, &pdu);
  return ret;
}

static
int f1ap_handle_f1_setup_request(uint32_t               assoc_id,
                                 uint32_t               stream,
                                 F1AP_F1AP_PDU_t       *pdu)
{
   printf("OOOOOOOOOOOOOOOOOOOOOOOOOOO\n");

   return 0;
}
