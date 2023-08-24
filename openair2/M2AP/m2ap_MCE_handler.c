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

/*! \file m2ap_MCE_handler.c
 * \brief m2ap handler procedures for MCE
 * \author Javier Morgade <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#include <stdint.h>

#include "intertask_interface.h"

#include "oai_asn1.h"

#include "m2ap_common.h"
#include "m2ap_MCE_defs.h"
#include "m2ap_MCE_handler.h"
#include "m2ap_decoder.h"
#include "m2ap_ids.h"

//#include "m2ap_MCE_management_procedures.h"
#include "m2ap_MCE_generate_messages.h"

#include "m2ap_MCE_interface_management.h"
//#include "m2ap_eNB_interface_management.h"

#include "assertions.h"
#include "conversions.h"

/* Handlers matrix. Only eNB related procedure present here */
static const m2ap_message_decoded_callback m2ap_MCE_messages_callback[][3] = {
    {0, MCE_handle_MBMS_SESSION_START_RESPONSE, 0}, /* MBMSSessionStart  */
    {0, MCE_handle_MBMS_SESSION_STOP_RESPONSE, 0}, /* MBMSSessionStop */
    {0, MCE_handle_MBMS_SCHEDULING_INFORMATION_RESPONSE, 0}, /* MBMSSchedulingInformation */
    {0, 0, 0}, /* Error Indication */
    {0, 0, 0}, /* Reset */
    {MCE_handle_M2_SETUP_REQUEST, 0, 0}, /* M2 Setup */
    {0, 0, 0}, /* eNBConfigurationUpdate */
    {0, 0, 0}, /* MCEConfigurationUpdate */
    {0, 0, 0}, /* privateMessage */
    {0, 0, 0}, /* MBMSSessionUpdate */
    {0, 0, 0}, /* MBMSServiceCounting */
    {0, 0, 0}, /* MBMSServiceCountingResultReport */
    {0, 0, 0} /* MBMSOverloadNotification */
};

static char *m2ap_direction2String(int m2ap_dir) {
static char *m2ap_direction_String[] = {
  "", /* Nothing */
  "Originating message", /* originating message */
  "Successfull outcome", /* successfull outcome */
  "UnSuccessfull outcome", /* successfull outcome */
};
return(m2ap_direction_String[m2ap_dir]);
}

int m2ap_MCE_handle_message(instance_t instance,
                            sctp_assoc_t assoc_id,
                            int32_t stream,
                            const uint8_t *const data,
                            const uint32_t data_length)
{
  M2AP_M2AP_PDU_t pdu;
  int ret;

  DevAssert(data != NULL);

  memset(&pdu, 0, sizeof(pdu));

  if (m2ap_decode_pdu(&pdu, data, data_length) < 0) {
    LOG_E(M2AP, "Failed to decode PDU\n");
    return -1;
  }

  /* Checking procedure Code and direction of message */
  if (pdu.choice.initiatingMessage.procedureCode > sizeof(m2ap_MCE_messages_callback) / (3 * sizeof(
        m2ap_MCE_message_decoded_callback))
      || (pdu.present > M2AP_M2AP_PDU_PR_unsuccessfulOutcome)) {
    LOG_E(M2AP, "[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, pdu.choice.initiatingMessage.procedureCode, pdu.present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M2AP_M2AP_PDU, &pdu);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for eNB (wrong direction).
   */
  if (m2ap_MCE_messages_callback[pdu.choice.initiatingMessage.procedureCode][pdu.present - 1] == NULL) {
    LOG_E(M2AP, "[SCTP %d] No handler for procedureCode %ld in %s\n",
                assoc_id, pdu.choice.initiatingMessage.procedureCode,
               m2ap_direction2String(pdu.present - 1));
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M2AP_M2AP_PDU, &pdu);
    return -1;
  }
  
  /* Calling the right handler */
  LOG_I(M2AP, "Calling handler with instance %ld\n",instance);
  ret = (*m2ap_MCE_messages_callback[pdu.choice.initiatingMessage.procedureCode][pdu.present - 1])
        (instance, assoc_id, stream, &pdu);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M2AP_M2AP_PDU, &pdu);
  return ret;
}


