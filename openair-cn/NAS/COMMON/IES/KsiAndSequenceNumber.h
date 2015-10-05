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

#ifndef KSI_AND_SEQUENCE_NUMBER_H_
#define KSI_AND_SEQUENCE_NUMBER_H_

#define KSI_AND_SEQUENCE_NUMBER_MINIMUM_LENGTH 2
#define KSI_AND_SEQUENCE_NUMBER_MAXIMUM_LENGTH 2

typedef struct KsiAndSequenceNumber_tag {
  uint8_t  ksi:3;
  uint8_t  sequencenumber:5;
} KsiAndSequenceNumber;

int encode_ksi_and_sequence_number(KsiAndSequenceNumber *ksiandsequencenumber, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_ksi_and_sequence_number_xml(KsiAndSequenceNumber *ksiandsequencenumber, uint8_t iei);

int decode_ksi_and_sequence_number(KsiAndSequenceNumber *ksiandsequencenumber, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* KSI AND SEQUENCE NUMBER_H_ */

