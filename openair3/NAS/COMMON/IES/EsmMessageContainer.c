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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "EsmMessageContainer.h"
#include "nas_log.h"

//#define NAS_DEBUG 1

int decode_esm_message_container(EsmMessageContainer *esmmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  int decode_result;
  uint16_t ielen;

  LOG_FUNC_IN;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  DECODE_LENGTH_U16(buffer + decoded, ielen, decoded);

  CHECK_LENGTH_DECODER(len - decoded, ielen);


  if ((decode_result = decode_octet_string(&esmmessagecontainer->esmmessagecontainercontents, ielen, buffer + decoded, len - decoded)) < 0) {
    LOG_FUNC_RETURN(decode_result);
  } else {
    decoded += decode_result;
  }

#if defined (NAS_DEBUG)
  dump_esm_message_container_xml(esmmessagecontainer, iei);
#endif
  LOG_FUNC_RETURN(decoded);
}

int encode_esm_message_container(EsmMessageContainer *esmmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  int32_t encode_result;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, ESM_MESSAGE_CONTAINER_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_esm_message_container_xml(esmmessagecontainer, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);

  //encoded += 2;
  //if ((encode_result = encode_octet_string(&esmmessagecontainer->esmmessagecontainercontents, buffer + sizeof(uint16_t), len - sizeof(uint16_t))) < 0)
  if ((encode_result = encode_octet_string(&esmmessagecontainer->esmmessagecontainercontents, lenPtr + sizeof(uint16_t), len - sizeof(uint16_t))) < 0)
    return encode_result;
  else
    encoded += encode_result;

  ENCODE_U16(lenPtr, encode_result, encoded);
  return encoded;
}

void dump_esm_message_container_xml(EsmMessageContainer *esmmessagecontainer, uint8_t iei)
{
  printf("<Esm Message Container>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("%s", dump_octet_string_xml(&esmmessagecontainer->esmmessagecontainercontents));
  printf("</Esm Message Container>\n");
}

