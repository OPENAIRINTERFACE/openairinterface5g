/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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

