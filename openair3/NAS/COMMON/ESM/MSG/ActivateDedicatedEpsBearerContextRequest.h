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
#include "LinkedEpsBearerIdentity.h"
#include "EpsQualityOfService.h"
#include "TrafficFlowTemplate.h"
#include "TransactionIdentifier.h"
#include "QualityOfService.h"
#include "LlcServiceAccessPointIdentifier.h"
#include "RadioPriority.h"
#include "PacketFlowIdentifier.h"
#include "ProtocolConfigurationOptions.h"

#ifndef ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_H_
#define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_MINIMUM_LENGTH ( \
    EPS_QUALITY_OF_SERVICE_MINIMUM_LENGTH + \
    TRAFFIC_FLOW_TEMPLATE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_MAXIMUM_LENGTH ( \
    EPS_QUALITY_OF_SERVICE_MAXIMUM_LENGTH + \
    TRAFFIC_FLOW_TEMPLATE_MAXIMUM_LENGTH + \
    TRANSACTION_IDENTIFIER_MAXIMUM_LENGTH + \
    QUALITY_OF_SERVICE_MAXIMUM_LENGTH + \
    LLC_SERVICE_ACCESS_POINT_IDENTIFIER_MAXIMUM_LENGTH + \
    RADIO_PRIORITY_MAXIMUM_LENGTH + \
    PACKET_FLOW_IDENTIFIER_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH)

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT         (1<<0)
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT                 (1<<1)
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT            (1<<2)
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT                 (1<<3)
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT         (1<<4)
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<5)

typedef enum activate_dedicated_eps_bearer_context_request_iei_tag {
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_IEI          = 0x5D, /* 0x5D = 93 */
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_IEI                  = 0x30, /* 0x30 = 48 */
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_IEI             = 0x32, /* 0x32 = 50 */
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_IEI                  = 0x80, /* 0x80 = 128 */
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_IEI          = 0x34, /* 0x34 = 52 */
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} activate_dedicated_eps_bearer_context_request_iei;

/*
 * Message name: Activate dedicated EPS bearer context request
 * Description: This message is sent by the network to the UE to request activation of a dedicated EPS bearer context associated with the same PDN address(es) and APN as an already active default EPS bearer context. See table 8.3.3.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct activate_dedicated_eps_bearer_context_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                                   protocoldiscriminator:4;
  EpsBearerIdentity                                       epsbeareridentity:4;
  ProcedureTransactionIdentity                            proceduretransactionidentity;
  MessageType                                             messagetype;
  LinkedEpsBearerIdentity                                 linkedepsbeareridentity;
  EpsQualityOfService                                     epsqos;
  TrafficFlowTemplate                                     tft;
  /* Optional fields */
  uint32_t                                                presencemask;
  TransactionIdentifier                                   transactionidentifier;
  QualityOfService                                        negotiatedqos;
  LlcServiceAccessPointIdentifier                         negotiatedllcsapi;
  RadioPriority                                           radiopriority;
  PacketFlowIdentifier                                    packetflowidentifier;
  ProtocolConfigurationOptions                            protocolconfigurationoptions;
} activate_dedicated_eps_bearer_context_request_msg;

int decode_activate_dedicated_eps_bearer_context_request(activate_dedicated_eps_bearer_context_request_msg *activatededicatedepsbearercontextrequest, uint8_t *buffer, uint32_t len);

int encode_activate_dedicated_eps_bearer_context_request(activate_dedicated_eps_bearer_context_request_msg *activatededicatedepsbearercontextrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST_H_) */

