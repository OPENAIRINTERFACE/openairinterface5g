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
#include "EpsBearerIdentity.h"
#include "ProcedureTransactionIdentity.h"
#include "MessageType.h"
#include "AccessPointName.h"
#include "ProtocolConfigurationOptions.h"

#ifndef ESM_INFORMATION_RESPONSE_H_
#define ESM_INFORMATION_RESPONSE_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ESM_INFORMATION_RESPONSE_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define ESM_INFORMATION_RESPONSE_MAXIMUM_LENGTH ( \
    ACCESS_POINT_NAME_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define ESM_INFORMATION_RESPONSE_ACCESS_POINT_NAME_PRESENT              (1<<0)
# define ESM_INFORMATION_RESPONSE_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<1)

typedef enum esm_information_response_iei_tag {
  ESM_INFORMATION_RESPONSE_ACCESS_POINT_NAME_IEI               = 0x28, /* 0x28 = 40 */
  ESM_INFORMATION_RESPONSE_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} esm_information_response_iei;

/*
 * Message name: ESM information response
 * Description: This message is sent by the UE to the network in response to an ESM INFORMATION REQUEST message and provides the requested ESM information. See table 8.3.14.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct esm_information_response_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                 protocoldiscriminator:4;
  EpsBearerIdentity                     epsbeareridentity:4;
  ProcedureTransactionIdentity          proceduretransactionidentity;
  MessageType                           messagetype;
  /* Optional fields */
  uint32_t                              presencemask;
  AccessPointName                       accesspointname;
  ProtocolConfigurationOptions          protocolconfigurationoptions;
} esm_information_response_msg;

int decode_esm_information_response(esm_information_response_msg *esminformationresponse, uint8_t *buffer, uint32_t len);

int encode_esm_information_response(esm_information_response_msg *esminformationresponse, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ESM_INFORMATION_RESPONSE_H_) */

