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

/*! \file f1ap_encoder.c
 * \brief f1ap pdu encode procedures
 */

#include "f1ap_common.h"
#include "f1ap_encoder.h"

int asn1_encoder_xer_print = 0;

int f1ap_encode_pdu(F1AP_F1AP_PDU_t *pdu, uint8_t **buffer, uint32_t *length) {
  ssize_t    encoded;
  DevAssert(pdu != NULL);
  DevAssert(buffer != NULL);
  DevAssert(length != NULL);

  if (asn1_encoder_xer_print) {
    LOG_E(F1AP, "----------------- ASN1 ENCODER PRINT START ----------------- \n");
    xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, pdu);
    LOG_E(F1AP, "----------------- ASN1 ENCODER PRINT END----------------- \n");
  }

  char errbuf[128]; /* Buffer for error message */
  size_t errlen = sizeof(errbuf); /* Size of the buffer */
  int ret = asn_check_constraints(&asn_DEF_F1AP_F1AP_PDU, pdu, errbuf, &errlen);

  /* assert(errlen < sizeof(errbuf)); // Guaranteed: you may rely on that */
  if(ret) {
    fprintf(stderr, "Constraint validation failed: %s\n", errbuf);
  }

  encoded = aper_encode_to_new_buffer(&asn_DEF_F1AP_F1AP_PDU, 0, pdu, (void **)buffer);

  if (encoded < 0) {
    LOG_E(F1AP, "Failed to encode F1AP message\n");
    return -1;
  }

  *length = encoded;
  return encoded;
}
