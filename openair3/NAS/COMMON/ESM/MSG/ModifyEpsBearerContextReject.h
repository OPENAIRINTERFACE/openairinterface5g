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
#include "EsmCause.h"
#include "ProtocolConfigurationOptions.h"

#ifndef MODIFY_EPS_BEARER_CONTEXT_REJECT_H_
#define MODIFY_EPS_BEARER_CONTEXT_REJECT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define MODIFY_EPS_BEARER_CONTEXT_REJECT_MINIMUM_LENGTH ( \
    ESM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define MODIFY_EPS_BEARER_CONTEXT_REJECT_MAXIMUM_LENGTH ( \
    ESM_CAUSE_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define MODIFY_EPS_BEARER_CONTEXT_REJECT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<0)

typedef enum modify_eps_bearer_context_reject_iei_tag {
  MODIFY_EPS_BEARER_CONTEXT_REJECT_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} modify_eps_bearer_context_reject_iei;

/*
 * Message name: Modify EPS bearer context reject
 * Description: This message is sent by the UE or the network to reject a modification of an active EPS bearer context. See table 8.3.17.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct modify_eps_bearer_context_reject_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                       protocoldiscriminator:4;
  EpsBearerIdentity                           epsbeareridentity:4;
  ProcedureTransactionIdentity                proceduretransactionidentity;
  MessageType                                 messagetype;
  EsmCause                                    esmcause;
  /* Optional fields */
  uint32_t                                    presencemask;
  ProtocolConfigurationOptions                protocolconfigurationoptions;
} modify_eps_bearer_context_reject_msg;

int decode_modify_eps_bearer_context_reject(modify_eps_bearer_context_reject_msg *modifyepsbearercontextreject, uint8_t *buffer, uint32_t len);

int encode_modify_eps_bearer_context_reject(modify_eps_bearer_context_reject_msg *modifyepsbearercontextreject, uint8_t *buffer, uint32_t len);

#endif /* ! defined(MODIFY_EPS_BEARER_CONTEXT_REJECT_H_) */

