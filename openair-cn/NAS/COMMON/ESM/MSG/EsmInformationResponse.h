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
#include "AccessPointName.h"
#include "ProtocolConfigurationOptions.h"

#ifndef ESM_INFORMATION_RESPONSE_H_
#define ESM_INFORMATION_RESPONSE_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ESM_INFORMATION_RESPONSE_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define ESM_INFORMATION_RESPONSE_MAXIMUM_LENGTH ( \
    ACCESS_POINT_NAME_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define ESM_INFORMATION_RESPONSE_ACCESS_POINT_NAME_PRESENT              (1<<0)
# define ESM_INFORMATION_RESPONSE_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<1)

typedef enum esm_information_response_iei_tag {
  ESM_INFORMATION_RESPONSE_ACCESS_POINT_NAME_IEI               = 0x28, /* 0x28 = 40 */
  ESM_INFORMATION_RESPONSE_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} esm_information_response_iei;

/*
 * Message name: ESM information response
 * Description: This message is sent by the UE to the network in response to an ESM INFORMATION REQUEST message and provides the requested ESM information. See table 8.3.14.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct esm_information_response_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                 protocoldiscriminator:4;
  EpsBearerIdentity                     epsbeareridentity:4;
  ProcedureTransactionIdentity          proceduretransactionidentity;
  MessageType                           messagetype;
  /* Optional fields */
  uint32_t                              presencemask;
  AccessPointName                       accesspointname;
  ProtocolConfigurationOptions          protocolconfigurationoptions;
} esm_information_response_msg;

int decode_esm_information_response(esm_information_response_msg *esminformationresponse, uint8_t *buffer, uint32_t len);

int encode_esm_information_response(esm_information_response_msg *esminformationresponse, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ESM_INFORMATION_RESPONSE_H_) */

