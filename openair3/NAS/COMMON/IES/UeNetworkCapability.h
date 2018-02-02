/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef UE_NETWORK_CAPABILITY_H_
#define UE_NETWORK_CAPABILITY_H_

#define UE_NETWORK_CAPABILITY_MINIMUM_LENGTH 4
#define UE_NETWORK_CAPABILITY_MAXIMUM_LENGTH 7

typedef struct UeNetworkCapability_tag {
  /* EPS encryption algorithms supported (octet 3) */
#define UE_NETWORK_CAPABILITY_EEA0  0b10000000
#define UE_NETWORK_CAPABILITY_EEA1  0b01000000
#define UE_NETWORK_CAPABILITY_EEA2  0b00100000
#define UE_NETWORK_CAPABILITY_EEA3  0b00010000
#define UE_NETWORK_CAPABILITY_EEA4  0b00001000
#define UE_NETWORK_CAPABILITY_EEA5  0b00000100
#define UE_NETWORK_CAPABILITY_EEA6  0b00000010
#define UE_NETWORK_CAPABILITY_EEA7  0b00000001
  uint8_t  eea;
  /* EPS integrity algorithms supported (octet 4) */
#define UE_NETWORK_CAPABILITY_EIA0  0b10000000
#define UE_NETWORK_CAPABILITY_EIA1  0b01000000
#define UE_NETWORK_CAPABILITY_EIA2  0b00100000
#define UE_NETWORK_CAPABILITY_EIA3  0b00010000
#define UE_NETWORK_CAPABILITY_EIA4  0b00001000
#define UE_NETWORK_CAPABILITY_EIA5  0b00000100
#define UE_NETWORK_CAPABILITY_EIA6  0b00000010
#define UE_NETWORK_CAPABILITY_EIA7  0b00000001
  uint8_t  eia;
  /* UMTS encryption algorithms supported (octet 5) */
#define UE_NETWORK_CAPABILITY_UEA0  0b10000000
#define UE_NETWORK_CAPABILITY_UEA1  0b01000000
#define UE_NETWORK_CAPABILITY_UEA2  0b00100000
#define UE_NETWORK_CAPABILITY_UEA3  0b00010000
#define UE_NETWORK_CAPABILITY_UEA4  0b00001000
#define UE_NETWORK_CAPABILITY_UEA5  0b00000100
#define UE_NETWORK_CAPABILITY_UEA6  0b00000010
#define UE_NETWORK_CAPABILITY_UEA7  0b00000001
  uint8_t  uea;
  /* UCS2 support (octet 6, bit 8) */
#define UE_NETWORK_CAPABILITY_DEFAULT_ALPHABET  0
#define UE_NETWORK_CAPABILITY_UCS2_ALPHABET 1
  uint8_t  ucs2:1;
  /* UMTS integrity algorithms supported (octet 6) */
#define UE_NETWORK_CAPABILITY_UIA1  0b01000000
#define UE_NETWORK_CAPABILITY_UIA2  0b00100000
#define UE_NETWORK_CAPABILITY_UIA3  0b00010000
#define UE_NETWORK_CAPABILITY_UIA4  0b00001000
#define UE_NETWORK_CAPABILITY_UIA5  0b00000100
#define UE_NETWORK_CAPABILITY_UIA6  0b00000010
#define UE_NETWORK_CAPABILITY_UIA7  0b00000001
  uint8_t  uia:7;
  /* Bits 8 to 6 of octet 7 are spare and shall be coded as zero */
  uint8_t  spare:3;
  /* eNodeB-based access class control for CSFB capability */
#define UE_NETWORK_CAPABILITY_CSFB  1
  uint8_t  csfb:1;
  /* LTE Positioning Protocol capability */
#define UE_NETWORK_CAPABILITY_LPP 1
  uint8_t  lpp:1;
  /* Location services notification mechanisms capability */
#define UE_NETWORK_CAPABILITY_LCS 1
  uint8_t  lcs:1;
  /* 1xSRVCC capability */
#define UE_NETWORK_CAPABILITY_SRVCC 1
  uint8_t  srvcc:1;
  /* NF notification procedure capability */
#define UE_NETWORK_CAPABILITY_NF  1
  uint8_t  nf:1;

  uint8_t  umts_present;
  uint8_t  gprs_present;
} UeNetworkCapability;

int encode_ue_network_capability(UeNetworkCapability *uenetworkcapability, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_ue_network_capability(UeNetworkCapability *uenetworkcapability, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_ue_network_capability_xml(UeNetworkCapability *uenetworkcapability, uint8_t iei);

#endif /* UE NETWORK CAPABILITY_H_ */

