/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file x2ap_common.c
 * \brief x2ap procedures for both eNB and MME
 * \author Sebastien ROUX <sebastien.roux@eurecom.fr>, Lionel GAUTHIER <Lionel.GAUTHIER@eurecom.fr>
 * \date 2014
 * \version 0.1
 */

#include <stdint.h>

#include "x2ap_common.h"
#include "X2AP-PDU.h"

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

ssize_t x2ap_generate_initiating_message(
  uint8_t               **buffer,
  uint32_t               *length,
  X2ap_ProcedureCode_t    procedureCode,
  X2ap_Criticality_t      criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  X2AP_PDU_t pdu;
  ssize_t    encoded;

  memset(&pdu, 0, sizeof(X2AP_PDU_t));

  pdu.present = X2AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = procedureCode;
  pdu.choice.initiatingMessage.criticality   = criticality;
  ANY_fromType_aper(&pdu.choice.initiatingMessage.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_X2AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_X2AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;
  return encoded;
}

ssize_t x2ap_generate_successfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  X2ap_ProcedureCode_t         procedureCode,
  X2ap_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  X2AP_PDU_t pdu;
  ssize_t    encoded;

  memset(&pdu, 0, sizeof(X2AP_PDU_t));

  pdu.present = X2AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = procedureCode;
  pdu.choice.successfulOutcome.criticality   = criticality;
  ANY_fromType_aper(&pdu.choice.successfulOutcome.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_X2AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_X2AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;

  return encoded;
}

ssize_t x2ap_generate_unsuccessfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  X2ap_ProcedureCode_t         procedureCode,
  X2ap_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr)
{
  X2AP_PDU_t pdu;
  ssize_t    encoded;

  memset(&pdu, 0, sizeof(X2AP_PDU_t));

  pdu.present = X2AP_PDU_PR_unsuccessfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = procedureCode;
  pdu.choice.successfulOutcome.criticality   = criticality;
  ANY_fromType_aper(&pdu.choice.successfulOutcome.value, td, sptr);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_X2AP_PDU, (void *)&pdu);
  }

  /* We can safely free list of IE from sptr */
  ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);

  if ((encoded = aper_encode_to_new_buffer(&asn_DEF_X2AP_PDU, 0, &pdu,
                 (void **)buffer)) < 0) {
    return -1;
  }

  *length = encoded;

  return encoded;
}

X2ap_IE_t *x2ap_new_ie(
  X2ap_ProtocolIE_ID_t   id,
  X2ap_Criticality_t     criticality,
  asn_TYPE_descriptor_t *type,
  void                  *sptr)
{
  X2ap_IE_t *buff;

  if ((buff = malloc(sizeof(X2ap_IE_t))) == NULL) {
    // Possible error on malloc
    return NULL;
  }

  memset((void *)buff, 0, sizeof(X2ap_IE_t));

  buff->id = id;
  buff->criticality = criticality;

  if (ANY_fromType_aper(&buff->value, type, sptr) < 0) {
    fprintf(stderr, "Encoding of %s failed\n", type->name);
    free(buff);
    return NULL;
  }

  if (asn1_xer_print)
    if (xer_fprint(stdout, &asn_DEF_X2ap_IE, buff) < 0) {
      free(buff);
      return NULL;
    }

  return buff;
}

void x2ap_handle_criticality(X2ap_Criticality_t criticality)
{

}
