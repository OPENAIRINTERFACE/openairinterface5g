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
#include "NasRequestType.h"
#include "PdnType.h"
#include "EsmInformationTransferFlag.h"
#include "AccessPointName.h"
#include "ProtocolConfigurationOptions.h"

#ifndef PDN_CONNECTIVITY_REQUEST_H_
#define PDN_CONNECTIVITY_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define PDN_CONNECTIVITY_REQUEST_MINIMUM_LENGTH ( \
    PDN_TYPE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define PDN_CONNECTIVITY_REQUEST_MAXIMUM_LENGTH ( \
    PDN_TYPE_MAXIMUM_LENGTH + \
    ESM_INFORMATION_TRANSFER_FLAG_MAXIMUM_LENGTH + \
    ACCESS_POINT_NAME_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define PDN_CONNECTIVITY_REQUEST_ESM_INFORMATION_TRANSFER_FLAG_PRESENT  (1<<0)
# define PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT              (1<<1)
# define PDN_CONNECTIVITY_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<2)

typedef enum pdn_connectivity_request_iei_tag {
  PDN_CONNECTIVITY_REQUEST_ESM_INFORMATION_TRANSFER_FLAG_IEI   = 0xD0, /* 0xD0 = 208 */
  PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_IEI               = 0x28, /* 0x28 = 40 */
  PDN_CONNECTIVITY_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} pdn_connectivity_request_iei;

/*
 * Message name: PDN connectivity request
 * Description: This message is sent by the UE to the network to initiate establishment of a PDN connection. See table 8.3.20.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct pdn_connectivity_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                 protocoldiscriminator:4;
  EpsBearerIdentity                     epsbeareridentity:4;
  ProcedureTransactionIdentity          proceduretransactionidentity;
  MessageType                           messagetype;
  RequestType                           requesttype;
  PdnType                               pdntype;
  /* Optional fields */
  uint32_t                              presencemask;
  EsmInformationTransferFlag            esminformationtransferflag;
  AccessPointName                       accesspointname;
  ProtocolConfigurationOptions          protocolconfigurationoptions;
} pdn_connectivity_request_msg;

int decode_pdn_connectivity_request(pdn_connectivity_request_msg *pdnconnectivityrequest, uint8_t *buffer, uint32_t len);

int encode_pdn_connectivity_request(pdn_connectivity_request_msg *pdnconnectivityrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(PDN_CONNECTIVITY_REQUEST_H_) */

