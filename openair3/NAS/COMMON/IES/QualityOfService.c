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
#include "QualityOfService.h"

int decode_quality_of_service(QualityOfService *qualityofservice, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded = 0;
  uint8_t ielen = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER(len - decoded, ielen);
  qualityofservice->delayclass = (*(buffer + decoded) >> 3) & 0x7;
  qualityofservice->reliabilityclass = *(buffer + decoded) & 0x7;
  decoded++;
  qualityofservice->peakthroughput = (*(buffer + decoded) >> 4) & 0xf;
  qualityofservice->precedenceclass = *(buffer + decoded) & 0x7;
  decoded++;
  qualityofservice->meanthroughput = *(buffer + decoded) & 0x1f;
  decoded++;
  qualityofservice->trafficclass = (*(buffer + decoded) >> 5) & 0x7;
  qualityofservice->deliveryorder = (*(buffer + decoded) >> 3) & 0x3;
  qualityofservice->deliveryoferroneoussdu = *(buffer + decoded) & 0x7;
  decoded++;
  qualityofservice->maximumsdusize = *(buffer + decoded);
  decoded++;
  qualityofservice->maximumbitrateuplink = *(buffer + decoded);
  decoded++;
  qualityofservice->maximumbitratedownlink = *(buffer + decoded);
  decoded++;
  qualityofservice->residualber = (*(buffer + decoded) >> 4) & 0xf;
  qualityofservice->sduratioerror = *(buffer + decoded) & 0xf;
  decoded++;
  qualityofservice->transferdelay = (*(buffer + decoded) >> 2) & 0x3f;
  qualityofservice->traffichandlingpriority = *(buffer + decoded) & 0x3;
  decoded++;
  qualityofservice->guaranteedbitrateuplink = *(buffer + decoded);
  decoded++;
  qualityofservice->guaranteedbitratedownlink = *(buffer + decoded);
  decoded++;
  qualityofservice->signalingindication = (*(buffer + decoded) >> 4) & 0x1;
  qualityofservice->sourcestatisticsdescriptor = *(buffer + decoded) & 0xf;
  decoded++;
#if defined (NAS_DEBUG)
  dump_quality_of_service_xml(qualityofservice, iei);
#endif
  return decoded;
}
int encode_quality_of_service(QualityOfService *qualityofservice, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, QUALITY_OF_SERVICE_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_quality_of_service_xml(qualityofservice, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;
  *(buffer + encoded) = 0x00 |
                        ((qualityofservice->delayclass & 0x7) << 3) |
                        (qualityofservice->reliabilityclass & 0x7);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->peakthroughput & 0xf) << 4) |
                        (qualityofservice->precedenceclass & 0x7);
  encoded++;
  *(buffer + encoded) = 0x00 |
                        (qualityofservice->meanthroughput & 0x1f);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->trafficclass & 0x7) << 5) |
                        ((qualityofservice->deliveryorder & 0x3) << 3) |
                        (qualityofservice->deliveryoferroneoussdu & 0x7);
  encoded++;
  *(buffer + encoded) = qualityofservice->maximumsdusize;
  encoded++;
  *(buffer + encoded) = qualityofservice->maximumbitrateuplink;
  encoded++;
  *(buffer + encoded) = qualityofservice->maximumbitratedownlink;
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->residualber & 0xf) << 4) |
                        (qualityofservice->sduratioerror & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->transferdelay & 0x3f) << 2) |
                        (qualityofservice->traffichandlingpriority & 0x3);
  encoded++;
  *(buffer + encoded) = qualityofservice->guaranteedbitrateuplink;
  encoded++;
  *(buffer + encoded) = qualityofservice->guaranteedbitratedownlink;
  encoded++;
  *(buffer + encoded) = 0x00 |
                        ((qualityofservice->signalingindication & 0x1) << 4) |
                        (qualityofservice->sourcestatisticsdescriptor & 0xf);
  encoded++;
  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

void dump_quality_of_service_xml(QualityOfService *qualityofservice, uint8_t iei)
{
  printf("<Quality Of Service>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  printf("    <Delay class>%u</Delay class>\n", qualityofservice->delayclass);
  printf("    <Reliability class>%u</Reliability class>\n", qualityofservice->reliabilityclass);
  printf("    <Peak throughput>%u</Peak throughput>\n", qualityofservice->peakthroughput);
  printf("    <Precedence class>%u</Precedence class>\n", qualityofservice->precedenceclass);
  printf("    <Mean throughput>%u</Mean throughput>\n", qualityofservice->meanthroughput);
  printf("    <Traffic class>%u</Traffic class>\n", qualityofservice->trafficclass);
  printf("    <Delivery order>%u</Delivery order>\n", qualityofservice->deliveryorder);
  printf("    <Delivery of erroneous SDU>%u</Delivery of erroneous SDU>\n", qualityofservice->deliveryoferroneoussdu);
  printf("    <Maximum SDU size>%u</Maximum SDU size>\n", qualityofservice->maximumsdusize);
  printf("    <Maximum bit rate uplink>%u</Maximum bit rate uplink>\n", qualityofservice->maximumbitrateuplink);
  printf("    <Maximum bit rate downlink>%u</Maximum bit rate downlink>\n", qualityofservice->maximumbitratedownlink);
  printf("    <Residual BER>%u</Residual BER>\n", qualityofservice->residualber);
  printf("    <SDU ratio error>%u</SDU ratio error>\n", qualityofservice->sduratioerror);
  printf("    <Transfer delay>%u</Transfer delay>\n", qualityofservice->transferdelay);
  printf("    <Traffic handling priority>%u</Traffic handling priority>\n", qualityofservice->traffichandlingpriority);
  printf("    <Guaranteed bit rate uplink>%u</Guaranteed bit rate uplink>\n", qualityofservice->guaranteedbitrateuplink);
  printf("    <Guaranteed bit rate downlink>%u</Guaranteed bit rate downlink>\n", qualityofservice->guaranteedbitratedownlink);
  printf("    <Signaling indication>%u</Signaling indication>\n", qualityofservice->signalingindication);
  printf("    <Source statistics descriptor>%u</Source statistics descriptor>\n", qualityofservice->sourcestatisticsdescriptor);
  printf("</Quality Of Service>\n");
}

