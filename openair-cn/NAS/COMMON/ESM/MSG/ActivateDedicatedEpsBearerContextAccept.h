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
#include "ProtocolConfigurationOptions.h"

#ifndef ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_H_
#define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_MAXIMUM_LENGTH ( \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<0)

typedef enum activate_dedicated_eps_bearer_context_accept_iei_tag {
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} activate_dedicated_eps_bearer_context_accept_iei;

/*
 * Message name: Activate dedicated EPS bearer context accept
 * Description: This message is sent by the UE to the network to acknowledge activation of a dedicated EPS bearer context associated with the same PDN address(es) and APN as an already active EPS bearer context. See table 8.3.1.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct activate_dedicated_eps_bearer_context_accept_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                                  protocoldiscriminator:4;
  EpsBearerIdentity                                      epsbeareridentity:4;
  ProcedureTransactionIdentity                           proceduretransactionidentity;
  MessageType                                            messagetype;
  /* Optional fields */
  uint32_t                                               presencemask;
  ProtocolConfigurationOptions                           protocolconfigurationoptions;
} activate_dedicated_eps_bearer_context_accept_msg;

int decode_activate_dedicated_eps_bearer_context_accept(activate_dedicated_eps_bearer_context_accept_msg *activatededicatedepsbearercontextaccept, uint8_t *buffer, uint32_t len);

int encode_activate_dedicated_eps_bearer_context_accept(activate_dedicated_eps_bearer_context_accept_msg *activatededicatedepsbearercontextaccept, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_H_) */

