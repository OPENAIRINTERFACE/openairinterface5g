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

