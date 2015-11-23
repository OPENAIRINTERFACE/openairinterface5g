/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
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

