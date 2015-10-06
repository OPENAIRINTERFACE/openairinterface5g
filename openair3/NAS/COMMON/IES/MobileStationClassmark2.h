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

#ifndef MOBILE_STATION_CLASSMARK_2_H_
#define MOBILE_STATION_CLASSMARK_2_H_

#define MOBILE_STATION_CLASSMARK_2_MINIMUM_LENGTH 5
#define MOBILE_STATION_CLASSMARK_2_MAXIMUM_LENGTH 5

typedef struct MobileStationClassmark2_tag {
  uint8_t  revisionlevel:2;
  uint8_t  esind:1;
  uint8_t  a51:1;
  uint8_t  rfpowercapability:3;
  uint8_t  pscapability:1;
  uint8_t  ssscreenindicator:2;
  uint8_t  smcapability:1;
  uint8_t  vbs:1;
  uint8_t  vgcs:1;
  uint8_t  fc:1;
  uint8_t  cm3:1;
  uint8_t  lcsvacap:1;
  uint8_t  ucs2:1;
  uint8_t  solsa:1;
  uint8_t  cmsp:1;
  uint8_t  a53:1;
  uint8_t  a52:1;
} MobileStationClassmark2;

int encode_mobile_station_classmark_2(MobileStationClassmark2 *mobilestationclassmark2, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_mobile_station_classmark_2(MobileStationClassmark2 *mobilestationclassmark2, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_mobile_station_classmark_2_xml(MobileStationClassmark2 *mobilestationclassmark2, uint8_t iei);

#endif /* MOBILE STATION CLASSMARK 2_H_ */

