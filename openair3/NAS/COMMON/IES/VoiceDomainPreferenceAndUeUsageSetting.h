#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_H_
#define VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_H_

#define VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_MINIMUM_LENGTH 1
#define VOICE_DOMAIN_PREFERENCE_AND_UE_USAGE_SETTING_MAXIMUM_LENGTH 1

typedef struct VoiceDomainPreferenceAndUeUsageSetting_tag {
  uint8_t  spare:5;
#define UE_USAGE_SETTING_VOICE_CENTRIC 0b0
#define UE_USAGE_SETTING_DATA_CENTRIC  0b1
  uint8_t  ue_usage_setting:1;
#define VOICE_DOMAIN_PREFERENCE_CS_VOICE_ONLY                                    0b00
#define VOICE_DOMAIN_PREFERENCE_IMS_PS_VOICE_ONLY                                0b01
#define VOICE_DOMAIN_PREFERENCE_CS_VOICE_PREFERRED_IMS_PS_VOICE_AS_SECONDARY     0b10
#define VOICE_DOMAIN_PREFERENCE_IMS_PS_VOICE_PREFERRED_CS_VOICE_AS_SECONDARY     0b11
  uint8_t  voice_domain_for_eutran:2;
} VoiceDomainPreferenceAndUeUsageSetting;

int encode_voice_domain_preference_and_ue_usage_setting(VoiceDomainPreferenceAndUeUsageSetting *voicedomainpreferenceandueusagesetting, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_voice_domain_preference_and_ue_usage_setting(VoiceDomainPreferenceAndUeUsageSetting *voicedomainpreferenceandueusagesetting, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_voice_domain_preference_and_ue_usage_setting_xml(VoiceDomainPreferenceAndUeUsageSetting *voicedomainpreferenceandueusagesetting, uint8_t iei);

#endif /* VOICE DOMAIN PREFERENCE AND UE USAGE SETTING_H_ */

