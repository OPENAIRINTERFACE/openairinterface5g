/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
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
