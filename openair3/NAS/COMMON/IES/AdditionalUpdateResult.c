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
#include "AdditionalUpdateResult.h"

int decode_additional_update_result(AdditionalUpdateResult *additionalupdateresult, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, ADDITIONAL_UPDATE_RESULT_MINIMUM_LENGTH, len);

  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  *additionalupdateresult = *buffer & 0x3;
  decoded++;
#if defined (NAS_DEBUG)
  dump_additional_update_result_xml(additionalupdateresult, iei);
#endif
  return decoded;
}

int decode_u8_additional_update_result(AdditionalUpdateResult *additionalupdateresult, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  *additionalupdateresult = *buffer & 0x3;
  decoded++;
#if defined (NAS_DEBUG)
  dump_additional_update_result_xml(additionalupdateresult, iei);
#endif
  return decoded;
}

int encode_additional_update_result(AdditionalUpdateResult *additionalupdateresult, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t encoded = 0;
  /* Checking length and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, ADDITIONAL_UPDATE_RESULT_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_additional_update_result_xml(additionalupdateresult, iei);
#endif
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*additionalupdateresult & 0x3);
  encoded++;
  return encoded;
}

uint8_t encode_u8_additional_update_result(AdditionalUpdateResult *additionalupdateresult)
{
  uint8_t bufferReturn;
  uint8_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  uint8_t iei = 0;
  dump_additional_update_result_xml(additionalupdateresult, 0);
  *(buffer + encoded) = 0x00 | (iei & 0xf0) |
                        (*additionalupdateresult & 0x3);
  encoded++;

  return bufferReturn;
}

void dump_additional_update_result_xml(AdditionalUpdateResult *additionalupdateresult, uint8_t iei)
{
  printf("<Additional Update Result>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Additional update result value>%u</Additional update result value>\n", *additionalupdateresult);
  printf("</Additional Update Result>\n");
}

