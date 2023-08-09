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

#include <stdint.h>
#include "xnap_common.h"
#include "XNAP_XnAP-PDU.h"

ssize_t XNAP_generate_initiating_message(uint8_t **buffer,
                                         uint32_t *length,
                                         XNAP_ProcedureCode_t procedureCode,
                                         XNAP_Criticality_t criticality,
                                         asn_TYPE_descriptor_t *td,
                                         void *sptr)
{
  XNAP_XnAP_PDU_t pdu;
  ssize_t encoded;
  memset(&pdu, 0, sizeof(XNAP_XnAP_PDU_t));
  pdu.present = XNAP_XnAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage->procedureCode = procedureCode;
  pdu.choice.initiatingMessage->criticality = criticality;
  ANY_fromType_aper((ANY_t *)&pdu.choice.initiatingMessage->value, td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_XNAP_XnAP_PDU, 0, &pdu, (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;
  return encoded;
}

