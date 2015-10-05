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

#ifndef APN_AGGREGATE_MAXIMUM_BIT_RATE_H_
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_H_

#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MINIMUM_LENGTH 4
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_LENGTH 8

#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT  (1<<0)
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION2_PRESENT (1<<1)

typedef struct ApnAggregateMaximumBitRate_tag {
  uint8_t  apnambrfordownlink;
  uint8_t  apnambrforuplink;
  uint8_t  apnambrfordownlink_extended;
  uint8_t  apnambrforuplink_extended;
  uint8_t  apnambrfordownlink_extended2;
  uint8_t  apnambrforuplink_extended2;
  uint8_t  extensions;
} ApnAggregateMaximumBitRate;

int encode_apn_aggregate_maximum_bit_rate(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_apn_aggregate_maximum_bit_rate(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_apn_aggregate_maximum_bit_rate_xml(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei);

#endif /* APN AGGREGATE MAXIMUM BIT RATE_H_ */

