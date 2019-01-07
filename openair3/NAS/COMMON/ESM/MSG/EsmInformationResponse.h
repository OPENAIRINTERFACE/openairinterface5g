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

