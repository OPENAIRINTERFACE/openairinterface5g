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
#include "LinkedEpsBearerIdentity.h"
#include "TrafficFlowAggregateDescription.h"
#include "EpsQualityOfService.h"
#include "EsmCause.h"
#include "ProtocolConfigurationOptions.h"

#ifndef BEARER_RESOURCE_MODIFICATION_REQUEST_H_
#define BEARER_RESOURCE_MODIFICATION_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define BEARER_RESOURCE_MODIFICATION_REQUEST_MINIMUM_LENGTH ( \
    TRAFFIC_FLOW_AGGREGATE_DESCRIPTION_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define BEARER_RESOURCE_MODIFICATION_REQUEST_MAXIMUM_LENGTH ( \
    TRAFFIC_FLOW_AGGREGATE_DESCRIPTION_MAXIMUM_LENGTH + \
    EPS_QUALITY_OF_SERVICE_MAXIMUM_LENGTH + \
    ESM_CAUSE_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define BEARER_RESOURCE_MODIFICATION_REQUEST_REQUIRED_TRAFFIC_FLOW_QOS_PRESENT      (1<<0)
# define BEARER_RESOURCE_MODIFICATION_REQUEST_ESM_CAUSE_PRESENT                      (1<<1)
# define BEARER_RESOURCE_MODIFICATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<2)

typedef enum bearer_resource_modification_request_iei_tag {
  BEARER_RESOURCE_MODIFICATION_REQUEST_REQUIRED_TRAFFIC_FLOW_QOS_IEI       = 0x5B, /* 0x5B = 91 */
  BEARER_RESOURCE_MODIFICATION_REQUEST_ESM_CAUSE_IEI                       = 0x58, /* 0x58 = 88 */
  BEARER_RESOURCE_MODIFICATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} bearer_resource_modification_request_iei;

/*
 * Message name: Bearer resource modification request
 * Description: This message is sent by the UE to the network to request the modification of a dedicated bearer resource. See table 8.3.10.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct bearer_resource_modification_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                            protocoldiscriminator:4;
  EpsBearerIdentity                                epsbeareridentity:4;
  ProcedureTransactionIdentity                     proceduretransactionidentity;
  MessageType                                      messagetype;
  LinkedEpsBearerIdentity                          epsbeareridentityforpacketfilter;
  TrafficFlowAggregateDescription                  trafficflowaggregate;
  /* Optional fields */
  uint32_t                                         presencemask;
  EpsQualityOfService                              requiredtrafficflowqos;
  EsmCause                                         esmcause;
  ProtocolConfigurationOptions                     protocolconfigurationoptions;
} bearer_resource_modification_request_msg;

int decode_bearer_resource_modification_request(bearer_resource_modification_request_msg *bearerresourcemodificationrequest, uint8_t *buffer, uint32_t len);

int encode_bearer_resource_modification_request(bearer_resource_modification_request_msg *bearerresourcemodificationrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(BEARER_RESOURCE_MODIFICATION_REQUEST_H_) */

