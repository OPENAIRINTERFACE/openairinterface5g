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

#ifndef TIME_ZONE_AND_TIME_H_
#define TIME_ZONE_AND_TIME_H_

#define TIME_ZONE_AND_TIME_MINIMUM_LENGTH 8
#define TIME_ZONE_AND_TIME_MAXIMUM_LENGTH 8

typedef struct TimeZoneAndTime_tag {
  uint8_t  year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
  uint8_t  timezone;
} TimeZoneAndTime;

int encode_time_zone_and_time(TimeZoneAndTime *timezoneandtime, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_time_zone_and_time_xml(TimeZoneAndTime *timezoneandtime, uint8_t iei);

int decode_time_zone_and_time(TimeZoneAndTime *timezoneandtime, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* TIME ZONE AND TIME_H_ */

