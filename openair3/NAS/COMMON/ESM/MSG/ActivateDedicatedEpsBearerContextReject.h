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

#ifndef ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT_H_
#define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT_MINIMUM_LENGTH ( \
    ESM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT_MAXIMUM_LENGTH ( \
    ESM_CAUSE_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<0)

typedef enum activate_dedicated_eps_bearer_context_reject_iei_tag {
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} activate_dedicated_eps_bearer_context_reject_iei;

/*
 * Message name: Activate dedicated EPS bearer context reject
 * Description: This message is sent by UE to the network to reject activation of a dedicated EPS bearer context. See table 8.3.2.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct activate_dedicated_eps_bearer_context_reject_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                                  protocoldiscriminator:4;
  EpsBearerIdentity                                      epsbeareridentity:4;
  ProcedureTransactionIdentity                           proceduretransactionidentity;
  MessageType                                            messagetype;
  EsmCause                                               esmcause;
  /* Optional fields */
  uint32_t                                               presencemask;
  ProtocolConfigurationOptions                           protocolconfigurationoptions;
} activate_dedicated_eps_bearer_context_reject_msg;

int decode_activate_dedicated_eps_bearer_context_reject(activate_dedicated_eps_bearer_context_reject_msg *activatededicatedepsbearercontextreject, uint8_t *buffer, uint32_t len);

int encode_activate_dedicated_eps_bearer_context_reject(activate_dedicated_eps_bearer_context_reject_msg *activatededicatedepsbearercontextreject, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT_H_) */

