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

