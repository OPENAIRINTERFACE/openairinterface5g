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

#ifndef NAS_SECURITY_ALGORITHMS_H_
#define NAS_SECURITY_ALGORITHMS_H_

#define NAS_SECURITY_ALGORITHMS_MINIMUM_LENGTH 1
#define NAS_SECURITY_ALGORITHMS_MAXIMUM_LENGTH 2

typedef struct NasSecurityAlgorithms_tag {
#define NAS_SECURITY_ALGORITHMS_EEA0  0b000
#define NAS_SECURITY_ALGORITHMS_EEA1  0b001
#define NAS_SECURITY_ALGORITHMS_EEA2  0b010
#define NAS_SECURITY_ALGORITHMS_EEA3  0b011
#define NAS_SECURITY_ALGORITHMS_EEA4  0b100
#define NAS_SECURITY_ALGORITHMS_EEA5  0b101
#define NAS_SECURITY_ALGORITHMS_EEA6  0b110
#define NAS_SECURITY_ALGORITHMS_EEA7  0b111
  uint8_t  typeofcipheringalgorithm:3;
#define NAS_SECURITY_ALGORITHMS_EIA0  0b000
#define NAS_SECURITY_ALGORITHMS_EIA1  0b001
#define NAS_SECURITY_ALGORITHMS_EIA2  0b010
#define NAS_SECURITY_ALGORITHMS_EIA3  0b011
#define NAS_SECURITY_ALGORITHMS_EIA4  0b100
#define NAS_SECURITY_ALGORITHMS_EIA5  0b101
#define NAS_SECURITY_ALGORITHMS_EIA6  0b110
#define NAS_SECURITY_ALGORITHMS_EIA7  0b111
  uint8_t  typeofintegrityalgorithm:3;
} NasSecurityAlgorithms;

int encode_nas_security_algorithms(NasSecurityAlgorithms *nassecurityalgorithms, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_nas_security_algorithms_xml(NasSecurityAlgorithms *nassecurityalgorithms, uint8_t iei);

int decode_nas_security_algorithms(NasSecurityAlgorithms *nassecurityalgorithms, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* NAS SECURITY ALGORITHMS_H_ */

