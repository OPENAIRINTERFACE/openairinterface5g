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

/*! \file m3ap_common.c
 * \brief m3ap procedures for both eNB and MCE
 * \author Javier Morgade  <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#include <stdint.h>

#include "m3ap_common.h"
#include "M3AP_M3AP-PDU.h"

int asn_debug = 0;
int asn1_xer_print = 0;

#if defined(EMIT_ASN_DEBUG_EXTERN)
inline void ASN_DEBUG(const char *fmt, ...)
{
  if (asn_debug) {
    int adi = asn_debug_indent;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[ASN1]");

    while(adi--) fprintf(stderr, " ");

    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
  }
}
#endif

ssize_t m3ap_generate_initiating_message(
  uint8_t               **buffer,
  uint32_t               *length,
  M3AP_ProcedureCode_t    procedureCode,
  M3AP_Criticality_t      criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  M3AP_M3AP_PDU_t pdu;
  ssize_t    encoded;
  memset(&pdu, 0, sizeof(M3AP_M3AP_PDU_t));
  pdu.present = M3AP_M3AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = procedureCode;
  pdu.choice.initiatingMessage.criticality   = criticality;
  ANY_fromType_aper((ANY_t *)&pdu.choice.initiatingMessage.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_M3AP_M3AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_M3AP_M3AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;
  return encoded;
}


ssize_t m3ap_generate_unsuccessfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  M3AP_ProcedureCode_t         procedureCode,
  M3AP_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  M3AP_M3AP_PDU_t pdu;
  ssize_t    encoded;
  memset(&pdu, 0, sizeof(M3AP_M3AP_PDU_t));
  pdu.present = M3AP_M3AP_PDU_PR_unsuccessfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = procedureCode;
  pdu.choice.successfulOutcome.criticality   = criticality;
  ANY_fromType_aper((ANY_t *)&pdu.choice.successfulOutcome.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_M3AP_M3AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_M3AP_M3AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;
  return encoded;
}

void m3ap_handle_criticality(M3AP_Criticality_t criticality)
{

}

