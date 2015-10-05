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
#include "NasMessageContainer.h"

#ifndef UPLINK_NAS_TRANSPORT_H_
#define UPLINK_NAS_TRANSPORT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define UPLINK_NAS_TRANSPORT_MINIMUM_LENGTH ( \
    NAS_MESSAGE_CONTAINER_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define UPLINK_NAS_TRANSPORT_MAXIMUM_LENGTH ( \
    NAS_MESSAGE_CONTAINER_MAXIMUM_LENGTH )


/*
 * Message name: Uplink NAS Transport
 * Description: This message is sent by the UE to the network in order to carry an SMS message in encapsulated format. See table 8.2.30.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct uplink_nas_transport_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator             protocoldiscriminator:4;
  SecurityHeaderType                securityheadertype:4;
  MessageType                       messagetype;
  NasMessageContainer               nasmessagecontainer;
} uplink_nas_transport_msg;

int decode_uplink_nas_transport(uplink_nas_transport_msg *uplinknastransport, uint8_t *buffer, uint32_t len);

int encode_uplink_nas_transport(uplink_nas_transport_msg *uplinknastransport, uint8_t *buffer, uint32_t len);

#endif /* ! defined(UPLINK_NAS_TRANSPORT_H_) */

