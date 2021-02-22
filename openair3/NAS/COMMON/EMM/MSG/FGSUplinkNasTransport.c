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

/*! \file FGSUplinkNasTransport.c

\brief uplink nas transport procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "FGSUplinkNasTransport.h"
#include "TLVEncoder.h"

int encode_fgs_payload_container(FGSPayloadContainer *paycontainer, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  int encode_result;

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  encoded += 2;

  if ((encode_result = encode_octet_string(&paycontainer->payloadcontainercontents, buffer + encoded, len - encoded)) < 0) {
    return encode_result;
  } else {
    encoded += encode_result;
  }
  if(iei > 0){
    *(uint16_t*) (buffer+1) = htons(encoded - 3);
  } else {
    *(uint16_t*) (buffer) = htons(encoded - 2);
  }

  return encoded;
}

int encode_nssai(OctetString *nssai, uint8_t iei, uint8_t *buffer)
{
  uint32_t encoded = 0;
  int encode_result;

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = nssai->length;
  encoded++;


  if ((encode_result = encode_octet_string(nssai, buffer + encoded, nssai->length)) < 0) {
    return encode_result;
  } else {
    encoded += encode_result;
  }

  return encoded;
}

int encode_dnn(OctetString *dnn, uint8_t iei, uint8_t *buffer)
{
  uint32_t encoded = 0;
  int encode_result;

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = dnn->length;
  encoded++;

  if ((encode_result = encode_octet_string(dnn, buffer + encoded, dnn->length)) < 0) {
    return encode_result;
  } else {
    encoded += encode_result;
  }

  return encoded;
}

int encode_fgs_uplink_nas_transport(fgs_uplink_nas_transport_msg *fgs_up_nas_transport, uint8_t *buffer, uint32_t len)
{
    int encoded = 0;
    int encode_result = 0;

    *(buffer + encoded) = (fgs_up_nas_transport->payloadcontainertype.iei << 4) | (fgs_up_nas_transport->payloadcontainertype.type &0xf);
    encoded++;

    if ((encode_result = encode_fgs_payload_container(&fgs_up_nas_transport->fgspayloadcontainer,
                                                       0, buffer +encoded, len - encoded)) < 0) {
      return encode_result;
    } else {
      encoded += encode_result;
    }

    *(buffer + encoded) = 0x12;
    encoded++;

    IES_ENCODE_U8(buffer, encoded, fgs_up_nas_transport->pdusessionid);

    // set request type
    *(buffer + encoded) = (0x8<<4)|(fgs_up_nas_transport->requesttype &0x7);
    encoded++;

    if ((encode_result = encode_nssai(&fgs_up_nas_transport->snssai, 0x22, buffer +encoded)) < 0) {
      return encode_result;
    } else {
      encoded += encode_result;
    }

    if ((encode_result = encode_dnn(&fgs_up_nas_transport->dnn, 0x25, buffer +encoded)) < 0) {
      return encode_result;
    } else {
      encoded += encode_result;
    }

    return encoded;
}


