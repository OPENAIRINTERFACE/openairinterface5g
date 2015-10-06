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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef QUALITY_OF_SERVICE_H_
#define QUALITY_OF_SERVICE_H_

#define QUALITY_OF_SERVICE_MINIMUM_LENGTH 14
#define QUALITY_OF_SERVICE_MAXIMUM_LENGTH 14

typedef struct QualityOfService_tag {
  uint8_t  delayclass:3;
  uint8_t  reliabilityclass:3;
  uint8_t  peakthroughput:4;
  uint8_t  precedenceclass:3;
  uint8_t  meanthroughput:5;
  uint8_t  trafficclass:3;
  uint8_t  deliveryorder:2;
  uint8_t  deliveryoferroneoussdu:3;
  uint8_t  maximumsdusize;
  uint8_t  maximumbitrateuplink;
  uint8_t  maximumbitratedownlink;
  uint8_t  residualber:4;
  uint8_t  sduratioerror:4;
  uint8_t  transferdelay:6;
  uint8_t  traffichandlingpriority:2;
  uint8_t  guaranteedbitrateuplink;
  uint8_t  guaranteedbitratedownlink;
  uint8_t  signalingindication:1;
  uint8_t  sourcestatisticsdescriptor:4;
} QualityOfService;

int encode_quality_of_service(QualityOfService *qualityofservice, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_quality_of_service(QualityOfService *qualityofservice, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_quality_of_service_xml(QualityOfService *qualityofservice, uint8_t iei);

#endif /* QUALITY OF SERVICE_H_ */

