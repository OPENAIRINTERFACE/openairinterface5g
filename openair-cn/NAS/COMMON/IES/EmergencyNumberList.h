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

#ifndef EMERGENCY_NUMBER_LIST_H_
#define EMERGENCY_NUMBER_LIST_H_

#define EMERGENCY_NUMBER_LIST_MINIMUM_LENGTH 5
#define EMERGENCY_NUMBER_LIST_MAXIMUM_LENGTH 50

typedef struct EmergencyNumberList_tag {
  uint8_t  lengthofemergency;
  uint8_t  emergencyservicecategoryvalue:5;
} EmergencyNumberList;

int encode_emergency_number_list(EmergencyNumberList *emergencynumberlist, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_emergency_number_list(EmergencyNumberList *emergencynumberlist, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_emergency_number_list_xml(EmergencyNumberList *emergencynumberlist, uint8_t iei);

#endif /* EMERGENCY NUMBER LIST_H_ */

