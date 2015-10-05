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

#ifndef PDN_ADDRESS_H_
#define PDN_ADDRESS_H_

#define PDN_ADDRESS_MINIMUM_LENGTH 7
#define PDN_ADDRESS_MAXIMUM_LENGTH 15

typedef struct PdnAddress_tag {
#define PDN_VALUE_TYPE_IPV4 0b001
#define PDN_VALUE_TYPE_IPV6 0b010
#define PDN_VALUE_TYPE_IPV4V6 0b011
  uint8_t  pdntypevalue:3;
  OctetString pdnaddressinformation;
} PdnAddress;

int encode_pdn_address(PdnAddress *pdnaddress, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_pdn_address(PdnAddress *pdnaddress, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_pdn_address_xml(PdnAddress *pdnaddress, uint8_t iei);

#endif /* PDN ADDRESS_H_ */

