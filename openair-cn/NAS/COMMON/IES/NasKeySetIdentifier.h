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

#ifndef NAS_KEY_SET_IDENTIFIER_H_
#define NAS_KEY_SET_IDENTIFIER_H_

#define NAS_KEY_SET_IDENTIFIER_MINIMUM_LENGTH 1
#define NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH 1

typedef struct NasKeySetIdentifier_tag {
#define NAS_KEY_SET_IDENTIFIER_NATIVE 0
#define NAS_KEY_SET_IDENTIFIER_MAPPED 1
  uint8_t  tsc:1;
#define NAS_KEY_SET_IDENTIFIER_NOT_AVAILABLE 0b111
  uint8_t  naskeysetidentifier:3;
} NasKeySetIdentifier;

int encode_nas_key_set_identifier(NasKeySetIdentifier *naskeysetidentifier, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_nas_key_set_identifier_xml(NasKeySetIdentifier *naskeysetidentifier, uint8_t iei);

uint8_t encode_u8_nas_key_set_identifier(NasKeySetIdentifier *naskeysetidentifier);

int decode_nas_key_set_identifier(NasKeySetIdentifier *naskeysetidentifier, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_nas_key_set_identifier(NasKeySetIdentifier *naskeysetidentifier, uint8_t iei, uint8_t value, uint32_t len);

#endif /* NAS KEY SET IDENTIFIER_H_ */

