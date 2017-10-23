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

/*! \file s1ap_common.c
 * \brief s1ap procedures for both eNB and MME
 * \author Sebastien ROUX and Navid Nikaein
 * \email navid.nikaein@eurecom.fr 
 * \date 2012-2015
 * \version 0.1
 */

#include <stdint.h>

#include "s1ap_common.h"
#include "S1AP-PDU.h"

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

ssize_t s1ap_generate_initiating_message(
  uint8_t               **buffer,
  uint32_t               *length,
  e_S1ap_ProcedureCode    procedureCode,
  S1ap_Criticality_t      criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  S1AP_PDU_t pdu;
  ssize_t    encoded;

  memset(&pdu, 0, sizeof(S1AP_PDU_t));

  pdu.present = S1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = procedureCode;
  pdu.choice.initiatingMessage.criticality   = criticality;
  ANY_fromType_aper(&pdu.choice.initiatingMessage.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_S1AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_S1AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;
  return encoded;
}

ssize_t s1ap_generate_successfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  e_S1ap_ProcedureCode         procedureCode,
  S1ap_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  S1AP_PDU_t pdu;
  ssize_t    encoded;

  memset(&pdu, 0, sizeof(S1AP_PDU_t));

  pdu.present = S1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = procedureCode;
  pdu.choice.successfulOutcome.criticality   = criticality;
  ANY_fromType_aper(&pdu.choice.successfulOutcome.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_S1AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_S1AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;

  return encoded;
}

ssize_t s1ap_generate_unsuccessfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  e_S1ap_ProcedureCode         procedureCode,
  S1ap_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  S1AP_PDU_t pdu;
  ssize_t    encoded;

  memset(&pdu, 0, sizeof(S1AP_PDU_t));

  pdu.present = S1AP_PDU_PR_unsuccessfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = procedureCode;
  pdu.choice.successfulOutcome.criticality   = criticality;
  ANY_fromType_aper(&pdu.choice.successfulOutcome.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_S1AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_S1AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;

  return encoded;
}

S1ap_IE_t *s1ap_new_ie(
  S1ap_ProtocolIE_ID_t   id,
  S1ap_Criticality_t     criticality,
  asn_TYPE_descriptor_t *type,
  void                  *sptr)
{
  S1ap_IE_t *buff;

  if ((buff = malloc(sizeof(S1ap_IE_t))) == NULL) {
    // Possible error on malloc
    return NULL;
  }

  memset((void *)buff, 0, sizeof(S1ap_IE_t));

  buff->id = id;
  buff->criticality = criticality;

  if (ANY_fromType_aper(&buff->value, type, sptr) < 0) {
    fprintf(stderr, "Encoding of %s failed\n", type->name);
    free(buff);
    return NULL;
  }

  if (asn1_xer_print)
    if (xer_fprint(stdout, &asn_DEF_S1ap_IE, buff) < 0) {
      free(buff);
      return NULL;
    }

  return buff;
}

void s1ap_handle_criticality(S1ap_Criticality_t criticality)
{

}
