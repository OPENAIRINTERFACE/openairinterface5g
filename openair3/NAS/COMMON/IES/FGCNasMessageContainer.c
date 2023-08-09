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

/*! \file FGCNasMessageContainer.c

\brief security mode complete procedures for gNB
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "FGCNasMessageContainer.h"

int decode_fgc_nas_message_container(FGCNasMessageContainer *nasmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  uint8_t ielen = 0;
  int decode_result;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded += 2;
  CHECK_LENGTH_DECODER(len - decoded, ielen);

  if ((decode_result = decode_octet_string(&nasmessagecontainer->nasmessagecontainercontents, ielen, buffer + decoded, len - decoded)) < 0)
    return decode_result;
  else
    decoded += decode_result;

  return decoded;
}
int encode_fgc_nas_message_container(FGCNasMessageContainer *nasmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  int encode_result;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, FGC_NAS_MESSAGE_CONTAINER_MINIMUM_LENGTH, len);

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  encoded += 2;

  if ((encode_result = encode_octet_string(&nasmessagecontainer->nasmessagecontainercontents, buffer + encoded, len - encoded)) < 0) {
    return encode_result;
  } else {
    uint16_t tmp = htons(encoded + encode_result - 3);
    memcpy(buffer + 1, &tmp, sizeof(tmp));
    encoded += encode_result;
  }

  return encoded;
}

void dump_fgc_nas_message_container_xml(FGCNasMessageContainer *nasmessagecontainer, uint8_t iei)
{
  printf("<Nas Message Container>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("%s", dump_octet_string_xml(&nasmessagecontainer->nasmessagecontainercontents));
  printf("</Nas Message Container>\n");
}


