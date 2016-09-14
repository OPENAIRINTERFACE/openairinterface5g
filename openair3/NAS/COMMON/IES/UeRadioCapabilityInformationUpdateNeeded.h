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

