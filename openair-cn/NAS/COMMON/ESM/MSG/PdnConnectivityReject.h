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
#include "EsmCause.h"
#include "ProtocolConfigurationOptions.h"

#ifndef PDN_CONNECTIVITY_REJECT_H_
#define PDN_CONNECTIVITY_REJECT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define PDN_CONNECTIVITY_REJECT_MINIMUM_LENGTH ( \
    ESM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define PDN_CONNECTIVITY_REJECT_MAXIMUM_LENGTH ( \
    ESM_CAUSE_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define PDN_CONNECTIVITY_REJECT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<0)

typedef enum pdn_connectivity_reject_iei_tag {
  PDN_CONNECTIVITY_REJECT_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} pdn_connectivity_reject_iei;

/*
 * Message name: PDN connectivity reject
 * Description: This message is sent by the network to the UE to reject establishment of a PDN connection. See table 8.3.19.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct pdn_connectivity_reject_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                protocoldiscriminator:4;
  EpsBearerIdentity                    epsbeareridentity:4;
  ProcedureTransactionIdentity         proceduretransactionidentity;
  MessageType                          messagetype;
  EsmCause                             esmcause;
  /* Optional fields */
  uint32_t                             presencemask;
  ProtocolConfigurationOptions         protocolconfigurationoptions;
} pdn_connectivity_reject_msg;

int decode_pdn_connectivity_reject(pdn_connectivity_reject_msg *pdnconnectivityreject, uint8_t *buffer, uint32_t len);

int encode_pdn_connectivity_reject(pdn_connectivity_reject_msg *pdnconnectivityreject, uint8_t *buffer, uint32_t len);

#endif /* ! defined(PDN_CONNECTIVITY_REJECT_H_) */

