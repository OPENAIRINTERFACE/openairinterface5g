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
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "NasMessageContainer.h"

#ifndef DOWNLINK_NAS_TRANSPORT_H_
#define DOWNLINK_NAS_TRANSPORT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define DOWNLINK_NAS_TRANSPORT_MINIMUM_LENGTH ( \
    NAS_MESSAGE_CONTAINER_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define DOWNLINK_NAS_TRANSPORT_MAXIMUM_LENGTH ( \
    NAS_MESSAGE_CONTAINER_MAXIMUM_LENGTH )


/*
 * Message name: Downlink NAS Transport
 * Description: This message is sent by the network to the UE in order to carry an SMS message in encapsulated format. See table 8.2.12.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct downlink_nas_transport_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator               protocoldiscriminator:4;
  SecurityHeaderType                  securityheadertype:4;
  MessageType                         messagetype;
  NasMessageContainer                 nasmessagecontainer;
} downlink_nas_transport_msg;

int decode_downlink_nas_transport(downlink_nas_transport_msg *downlinknastransport, uint8_t *buffer, uint32_t len);

int encode_downlink_nas_transport(downlink_nas_transport_msg *downlinknastransport, uint8_t *buffer, uint32_t len);

#endif /* ! defined(DOWNLINK_NAS_TRANSPORT_H_) */

