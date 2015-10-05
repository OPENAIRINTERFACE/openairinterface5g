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
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "EsmMessageContainer.h"

#ifndef ATTACH_COMPLETE_H_
#define ATTACH_COMPLETE_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ATTACH_COMPLETE_MINIMUM_LENGTH ( \
    ESM_MESSAGE_CONTAINER_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define ATTACH_COMPLETE_MAXIMUM_LENGTH ( \
    ESM_MESSAGE_CONTAINER_MAXIMUM_LENGTH )


/*
 * Message name: Attach complete
 * Description: This message is sent by the UE to the network in response to an ATTACH ACCEPT message. See table 8.2.2.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct attach_complete_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator         protocoldiscriminator:4;
  SecurityHeaderType            securityheadertype:4;
  MessageType                   messagetype;
  EsmMessageContainer           esmmessagecontainer;
} attach_complete_msg;

int decode_attach_complete(attach_complete_msg *attachcomplete, uint8_t *buffer, uint32_t len);

int encode_attach_complete(attach_complete_msg *attachcomplete, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ATTACH_COMPLETE_H_) */

