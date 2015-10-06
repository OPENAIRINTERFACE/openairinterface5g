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

#ifndef UE_SECURITY_CAPABILITY_H_
#define UE_SECURITY_CAPABILITY_H_

#define UE_SECURITY_CAPABILITY_MINIMUM_LENGTH 2
#define UE_SECURITY_CAPABILITY_MAXIMUM_LENGTH 7

typedef struct UeSecurityCapability_tag {
  /* EPS encryption algorithms supported (octet 3) */
#define UE_SECURITY_CAPABILITY_EEA0 0b10000000
#define UE_SECURITY_CAPABILITY_EEA1 0b01000000
#define UE_SECURITY_CAPABILITY_EEA2 0b00100000
#define UE_SECURITY_CAPABILITY_EEA3 0b00010000
#define UE_SECURITY_CAPABILITY_EEA4 0b00001000
#define UE_SECURITY_CAPABILITY_EEA5 0b00000100
#define UE_SECURITY_CAPABILITY_EEA6 0b00000010
#define UE_SECURITY_CAPABILITY_EEA7 0b00000001
  uint8_t  eea;
  /* EPS integrity algorithms supported (octet 4) */
#define UE_SECURITY_CAPABILITY_EIA0 0b10000000
#define UE_SECURITY_CAPABILITY_EIA1 0b01000000
#define UE_SECURITY_CAPABILITY_EIA2 0b00100000
#define UE_SECURITY_CAPABILITY_EIA3 0b00010000
#define UE_SECURITY_CAPABILITY_EIA4 0b00001000
#define UE_SECURITY_CAPABILITY_EIA5 0b00000100
#define UE_SECURITY_CAPABILITY_EIA6 0b00000010
#define UE_SECURITY_CAPABILITY_EIA7 0b00000001
  uint8_t  eia;
  uint8_t  umts_present;
  uint8_t  gprs_present;
  /* UMTS encryption algorithms supported (octet 5) */
#define UE_SECURITY_CAPABILITY_UEA0 0b10000000
#define UE_SECURITY_CAPABILITY_UEA1 0b01000000
#define UE_SECURITY_CAPABILITY_UEA2 0b00100000
#define UE_SECURITY_CAPABILITY_UEA3 0b00010000
#define UE_SECURITY_CAPABILITY_UEA4 0b00001000
#define UE_SECURITY_CAPABILITY_UEA5 0b00000100
#define UE_SECURITY_CAPABILITY_UEA6 0b00000010
#define UE_SECURITY_CAPABILITY_UEA7 0b00000001
  uint8_t  uea;
  /* UMTS integrity algorithms supported (octet 6) */
#define UE_SECURITY_CAPABILITY_UIA1 0b01000000
#define UE_SECURITY_CAPABILITY_UIA2 0b00100000
#define UE_SECURITY_CAPABILITY_UIA3 0b00010000
#define UE_SECURITY_CAPABILITY_UIA4 0b00001000
#define UE_SECURITY_CAPABILITY_UIA5 0b00000100
#define UE_SECURITY_CAPABILITY_UIA6 0b00000010
#define UE_SECURITY_CAPABILITY_UIA7 0b00000001
  uint8_t  uia:7;
  /* GPRS encryption algorithms supported (octet 7) */
#define UE_SECURITY_CAPABILITY_GEA1 0b01000000
#define UE_SECURITY_CAPABILITY_GEA2 0b00100000
#define UE_SECURITY_CAPABILITY_GEA3 0b00010000
#define UE_SECURITY_CAPABILITY_GEA4 0b00001000
#define UE_SECURITY_CAPABILITY_GEA5 0b00000100
#define UE_SECURITY_CAPABILITY_GEA6 0b00000010
#define UE_SECURITY_CAPABILITY_GEA7 0b00000001
  uint8_t  gea:7;
} UeSecurityCapability;

int encode_ue_security_capability(UeSecurityCapability *uesecuritycapability, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_ue_security_capability(UeSecurityCapability *uesecuritycapability, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_ue_security_capability_xml(UeSecurityCapability *uesecuritycapability, uint8_t iei);

#endif /* UE SECURITY CAPABILITY_H_ */

