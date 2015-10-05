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

#ifndef UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_H_
#define UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_H_

#define UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_MINIMUM_LENGTH 1
#define UE_RADIO_CAPABILITY_INFORMATION_UPDATE_NEEDED_MAXIMUM_LENGTH 1

typedef uint8_t UeRadioCapabilityInformationUpdateNeeded;

int encode_ue_radio_capability_information_update_needed(UeRadioCapabilityInformationUpdateNeeded *ueradiocapabilityinformationupdateneeded, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_ue_radio_capability_information_update_needed_xml(UeRadioCapabilityInformationUpdateNeeded *ueradiocapabilityinformationupdateneeded, uint8_t iei);

uint8_t encode_u8_ue_radio_capability_information_update_needed(UeRadioCapabilityInformationUpdateNeeded *ueradiocapabilityinformationupdateneeded);

int decode_ue_radio_capability_information_update_needed(UeRadioCapabilityInformationUpdateNeeded *ueradiocapabilityinformationupdateneeded, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_ue_radio_capability_information_update_needed(UeRadioCapabilityInformationUpdateNeeded *ueradiocapabilityinformationupdateneeded, uint8_t iei, uint8_t value, uint32_t len);

#endif /* UE RADIO CAPABILITY INFORMATION UPDATE NEEDED_H_ */

