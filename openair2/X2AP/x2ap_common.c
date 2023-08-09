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

/*! \file x2ap_common.c
 * \brief x2ap procedures for both eNB and MME
 * \author Sebastien ROUX <sebastien.roux@eurecom.fr>, Lionel GAUTHIER <Lionel.GAUTHIER@eurecom.fr>
 * \date 2014
 * \version 0.1
 */

#include <stdint.h>

#include "x2ap_common.h"
#include "X2AP_X2AP-PDU.h"

ssize_t x2ap_generate_initiating_message(
  uint8_t               **buffer,
  uint32_t               *length,
  X2AP_ProcedureCode_t    procedureCode,
  X2AP_Criticality_t      criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  X2AP_X2AP_PDU_t pdu;
  ssize_t    encoded;
  memset(&pdu, 0, sizeof(X2AP_X2AP_PDU_t));
  pdu.present = X2AP_X2AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = procedureCode;
  pdu.choice.initiatingMessage.criticality   = criticality;
  ANY_fromType_aper((ANY_t *)&pdu.choice.initiatingMessage.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_X2AP_X2AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_X2AP_X2AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;
  return encoded;
}

ssize_t x2ap_generate_successfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  X2AP_ProcedureCode_t         procedureCode,
  X2AP_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  X2AP_X2AP_PDU_t pdu;
  ssize_t    encoded;
  memset(&pdu, 0, sizeof(X2AP_X2AP_PDU_t));
  pdu.present = X2AP_X2AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = procedureCode;
  pdu.choice.successfulOutcome.criticality   = criticality;
  ANY_fromType_aper((ANY_t *)&pdu.choice.successfulOutcome.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_X2AP_X2AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_X2AP_X2AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;
  return encoded;
}

ssize_t x2ap_generate_unsuccessfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  X2AP_ProcedureCode_t         procedureCode,
  X2AP_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  X2AP_X2AP_PDU_t pdu;
  ssize_t    encoded;
  memset(&pdu, 0, sizeof(X2AP_X2AP_PDU_t));
  pdu.present = X2AP_X2AP_PDU_PR_unsuccessfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = procedureCode;
  pdu.choice.successfulOutcome.criticality   = criticality;
  ANY_fromType_aper((ANY_t *)&pdu.choice.successfulOutcome.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_X2AP_X2AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_X2AP_X2AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;
  return encoded;
}

void x2ap_handle_criticality(X2AP_Criticality_t criticality)
{
}
