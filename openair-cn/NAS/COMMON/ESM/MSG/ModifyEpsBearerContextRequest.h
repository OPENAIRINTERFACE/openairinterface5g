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
#include "EpsQualityOfService.h"
#include "TrafficFlowTemplate.h"
#include "QualityOfService.h"
#include "LlcServiceAccessPointIdentifier.h"
#include "RadioPriority.h"
#include "PacketFlowIdentifier.h"
#include "ApnAggregateMaximumBitRate.h"
#include "ProtocolConfigurationOptions.h"

#ifndef MODIFY_EPS_BEARER_CONTEXT_REQUEST_H_
#define MODIFY_EPS_BEARER_CONTEXT_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define MODIFY_EPS_BEARER_CONTEXT_REQUEST_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define MODIFY_EPS_BEARER_CONTEXT_REQUEST_MAXIMUM_LENGTH ( \
    EPS_QUALITY_OF_SERVICE_MAXIMUM_LENGTH + \
    TRAFFIC_FLOW_TEMPLATE_MAXIMUM_LENGTH + \
    QUALITY_OF_SERVICE_MAXIMUM_LENGTH + \
    LLC_SERVICE_ACCESS_POINT_IDENTIFIER_MAXIMUM_LENGTH + \
    RADIO_PRIORITY_MAXIMUM_LENGTH + \
    PACKET_FLOW_IDENTIFIER_MAXIMUM_LENGTH + \
    APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_EPS_QOS_PRESENT                    (1<<0)
# define MODIFY_EPS_BEARER_CONTEXT_REQUEST_TFT_PRESENT                            (1<<1)
# define MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_QOS_PRESENT                        (1<<2)
# define MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT            (1<<3)
# define MODIFY_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT                 (1<<4)
# define MODIFY_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT         (1<<5)
# define MODIFY_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT                        (1<<6)
# define MODIFY_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<7)

typedef enum modify_eps_bearer_context_request_iei_tag {
  MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_EPS_QOS_IEI                     = 0x5B, /* 0x5B = 91 */
  MODIFY_EPS_BEARER_CONTEXT_REQUEST_TFT_IEI                             = 0x36, /* 0x36 = 54 */
  MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_QOS_IEI                         = 0x30, /* 0x30 = 48 */
  MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_IEI             = 0x32, /* 0x32 = 50 */
  MODIFY_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_IEI                  = 0x80, /* 0x80 = 128 */
  MODIFY_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_IEI          = 0x34, /* 0x34 = 52 */
  MODIFY_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_IEI                         = 0x5E, /* 0x5E = 94 */
  MODIFY_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} modify_eps_bearer_context_request_iei;

/*
 * Message name: Modify EPS bearer context request
 * Description: This message is sent by the network to the UE to request modification of an active EPS bearer context. See table 8.3.18.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct modify_eps_bearer_context_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                        protocoldiscriminator:4;
  EpsBearerIdentity                            epsbeareridentity:4;
  ProcedureTransactionIdentity                 proceduretransactionidentity;
  MessageType                                  messagetype;
  /* Optional fields */
  uint32_t                                     presencemask;
  EpsQualityOfService                          newepsqos;
  TrafficFlowTemplate                          tft;
  QualityOfService                             newqos;
  LlcServiceAccessPointIdentifier              negotiatedllcsapi;
  RadioPriority                                radiopriority;
  PacketFlowIdentifier                         packetflowidentifier;
  ApnAggregateMaximumBitRate                   apnambr;
  ProtocolConfigurationOptions                 protocolconfigurationoptions;
} modify_eps_bearer_context_request_msg;

int decode_modify_eps_bearer_context_request(modify_eps_bearer_context_request_msg *modifyepsbearercontextrequest, uint8_t *buffer, uint32_t len);

int encode_modify_eps_bearer_context_request(modify_eps_bearer_context_request_msg *modifyepsbearercontextrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(MODIFY_EPS_BEARER_CONTEXT_REQUEST_H_) */

