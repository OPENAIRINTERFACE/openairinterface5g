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
#include "EpsQualityOfService.h"
#include "AccessPointName.h"
#include "PdnAddress.h"
#include "TransactionIdentifier.h"
#include "QualityOfService.h"
#include "LlcServiceAccessPointIdentifier.h"
#include "RadioPriority.h"
#include "PacketFlowIdentifier.h"
#include "ApnAggregateMaximumBitRate.h"
#include "EsmCause.h"
#include "ProtocolConfigurationOptions.h"

#ifndef ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_H_
#define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_MINIMUM_LENGTH ( \
    EPS_QUALITY_OF_SERVICE_MINIMUM_LENGTH + \
    ACCESS_POINT_NAME_MINIMUM_LENGTH + \
    PDN_ADDRESS_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_MAXIMUM_LENGTH ( \
    EPS_QUALITY_OF_SERVICE_MAXIMUM_LENGTH + \
    ACCESS_POINT_NAME_MAXIMUM_LENGTH + \
    PDN_ADDRESS_MAXIMUM_LENGTH + \
    TRANSACTION_IDENTIFIER_MAXIMUM_LENGTH + \
    QUALITY_OF_SERVICE_MAXIMUM_LENGTH + \
    LLC_SERVICE_ACCESS_POINT_IDENTIFIER_MAXIMUM_LENGTH + \
    RADIO_PRIORITY_MAXIMUM_LENGTH + \
    PACKET_FLOW_IDENTIFIER_MAXIMUM_LENGTH + \
    APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_LENGTH + \
    ESM_CAUSE_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT         (1<<0)
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT                 (1<<1)
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT            (1<<2)
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT                 (1<<3)
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT         (1<<4)
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT                        (1<<5)
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT                      (1<<6)
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<7)

typedef enum activate_default_eps_bearer_context_request_iei_tag {
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_IEI          = 0x5D, /* 0x5D = 93 */
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_IEI                  = 0x30, /* 0x30 = 48 */
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_IEI             = 0x32, /* 0x32 = 50 */
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_IEI                  = 0x80, /* 0x80 = 128 */
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_IEI          = 0x34, /* 0x34 = 52 */
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_IEI                         = 0x5E, /* 0x5E = 94 */
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_IEI                       = 0x58, /* 0x58 = 88 */
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} activate_default_eps_bearer_context_request_iei;

/*
 * Message name: Activate default EPS bearer context request
 * Description: This message is sent by the network to the UE to request activation of a default EPS bearer context. See table 8.3.6.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct activate_default_eps_bearer_context_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                                 protocoldiscriminator:4;
  EpsBearerIdentity                                     epsbeareridentity:4;
  ProcedureTransactionIdentity                          proceduretransactionidentity;
  MessageType                                           messagetype;
  EpsQualityOfService                                   epsqos;
  AccessPointName                                       accesspointname;
  PdnAddress                                            pdnaddress;
  /* Optional fields */
  uint32_t                                              presencemask;
  TransactionIdentifier                                 transactionidentifier;
  QualityOfService                                      negotiatedqos;
  LlcServiceAccessPointIdentifier                       negotiatedllcsapi;
  RadioPriority                                         radiopriority;
  PacketFlowIdentifier                                  packetflowidentifier;
  ApnAggregateMaximumBitRate                            apnambr;
  EsmCause                                              esmcause;
  ProtocolConfigurationOptions                          protocolconfigurationoptions;
} activate_default_eps_bearer_context_request_msg;

int decode_activate_default_eps_bearer_context_request(activate_default_eps_bearer_context_request_msg *activatedefaultepsbearercontextrequest, uint8_t *buffer, uint32_t len);

int encode_activate_default_eps_bearer_context_request(activate_default_eps_bearer_context_request_msg *activatedefaultepsbearercontextrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_H_) */

