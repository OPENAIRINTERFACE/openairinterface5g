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
#include "DrxParameter.h"

int decode_drx_parameter(DrxParameter *drxparameter, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  drxparameter->splitpgcyclecode = *(buffer + decoded);
  decoded++;
  drxparameter->cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode = (*(buffer + decoded) >> 4) & 0xf;
  drxparameter->splitonccch = (*(buffer + decoded) >> 3) & 0x1;
  drxparameter->nondrxtimer = *(buffer + decoded) & 0x7;
  decoded++;
#if defined (NAS_DEBUG)
  dump_drx_parameter_xml(drxparameter, iei);
#endif
  return decoded;
}

int encode_drx_parameter(DrxParameter *drxparameter, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, DRX_PARAMETER_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_drx_parameter_xml(drxparameter, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  *(buffer + encoded) = drxparameter->splitpgcyclecode;
  encoded++;
  *(buffer + encoded) = 0x00 | ((drxparameter->cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode & 0xf) << 4) |
                        ((drxparameter->splitonccch & 0x1) << 3) |
                        (drxparameter->nondrxtimer & 0x7);
  encoded++;
  return encoded;
}

void dump_drx_parameter_xml(DrxParameter *drxparameter, uint8_t iei)
{
  printf("<Drx Parameter>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <SPLIT PG CYCLE CODE>%u</SPLIT PG CYCLE CODE>\n", drxparameter->splitpgcyclecode);
  printf("    <CN specific DRX cycle length coefficient and DRX value for S1 mode>%u</CN specific DRX cycle length coefficient and DRX value for S1 mode>\n",
         drxparameter->cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode);
  printf("    <SPLIT on CCCH>%u</SPLIT on CCCH>\n", drxparameter->splitonccch);
  printf("    <non DRX timer>%u</non DRX timer>\n", drxparameter->nondrxtimer);
  printf("</Drx Parameter>\n");
}

