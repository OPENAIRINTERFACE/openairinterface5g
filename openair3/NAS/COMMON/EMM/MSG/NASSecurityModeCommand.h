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

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "NasSecurityAlgorithms.h"
#include "NasKeySetIdentifier.h"
#include "UeSecurityCapability.h"
#include "ImeisvRequest.h"
#include "Nonce.h"

#ifndef NAS_SECURITY_MODE_COMMAND_H_
#define NAS_SECURITY_MODE_COMMAND_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define SECURITY_MODE_COMMAND_MINIMUM_LENGTH ( \
    NAS_SECURITY_ALGORITHMS_MINIMUM_LENGTH + \
    NAS_KEY_SET_IDENTIFIER_MINIMUM_LENGTH + \
    UE_SECURITY_CAPABILITY_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define SECURITY_MODE_COMMAND_MAXIMUM_LENGTH ( \
    NAS_SECURITY_ALGORITHMS_MAXIMUM_LENGTH + \
    NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH + \
    UE_SECURITY_CAPABILITY_MAXIMUM_LENGTH + \
    IMEISV_REQUEST_MAXIMUM_LENGTH + \
    NONCE_MAXIMUM_LENGTH + \
    NONCE_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define SECURITY_MODE_COMMAND_IMEISV_REQUEST_PRESENT   (1<<0)
# define SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_PRESENT (1<<1)
# define SECURITY_MODE_COMMAND_NONCEMME_PRESENT         (1<<2)

typedef enum security_mode_command_iei_tag {
  SECURITY_MODE_COMMAND_IMEISV_REQUEST_IEI    = 0xC0, /* 0xC0 = 192 */
  SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_IEI  = 0x55, /* 0x55 = 85 */
  SECURITY_MODE_COMMAND_NONCEMME_IEI          = 0x56, /* 0x56 = 86 */
} security_mode_command_iei;

/*
 * Message name: Security mode command
 * Description: This message is sent by the network to the UE to establish NAS signalling security. See table 8.2.20.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct security_mode_command_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator              protocoldiscriminator:4;
  SecurityHeaderType                 securityheadertype:4;
  MessageType                        messagetype;
  NasSecurityAlgorithms              selectednassecurityalgorithms;
  NasKeySetIdentifier                naskeysetidentifier;
  UeSecurityCapability               replayeduesecuritycapabilities;
  /* Optional fields */
  uint32_t                           presencemask;
  ImeisvRequest                      imeisvrequest;
  Nonce                              replayednonceue;
  Nonce                              noncemme;
} security_mode_command_msg;

int decode_security_mode_command(security_mode_command_msg *securitymodecommand, uint8_t *buffer, uint32_t len);

int encode_security_mode_command(security_mode_command_msg *securitymodecommand, uint8_t *buffer, uint32_t len);

#endif /* ! defined(NAS_SECURITY_MODE_COMMAND_H_) */

