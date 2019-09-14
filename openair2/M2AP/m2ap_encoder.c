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

/*! \file m2ap_encoder.c
 * \brief m2ap encoder procedures
 * \author Javier Morgade <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "m2ap_common.h"
#include "m2ap_encoder.h"

int m2ap_encode_pdu(M2AP_M2AP_PDU_t *pdu, uint8_t **buffer, uint32_t *len)
{
  ssize_t    encoded;

  DevAssert(pdu != NULL);
  DevAssert(buffer != NULL);
  DevAssert(len != NULL);

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_M2AP_M2AP_PDU, (void *)pdu);
  }

  encoded = aper_encode_to_new_buffer(&asn_DEF_M2AP_M2AP_PDU, 0, pdu, (void **)buffer);

  if (encoded < 0) {
    return -1;
  }

  *len = encoded;

  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M2AP_M2AP_PDU, pdu);
  return encoded;
}
