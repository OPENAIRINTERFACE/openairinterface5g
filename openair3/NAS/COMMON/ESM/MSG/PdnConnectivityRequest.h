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

