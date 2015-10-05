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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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

