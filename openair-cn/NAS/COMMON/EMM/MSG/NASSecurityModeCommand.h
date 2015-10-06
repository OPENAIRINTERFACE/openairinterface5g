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

