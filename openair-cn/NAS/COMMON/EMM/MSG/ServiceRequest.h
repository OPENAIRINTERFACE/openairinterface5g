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
#include "KsiAndSequenceNumber.h"
#include "ShortMac.h"

#ifndef SERVICE_REQUEST_H_
#define SERVICE_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define SERVICE_REQUEST_MINIMUM_LENGTH ( \
    KSI_AND_SEQUENCE_NUMBER_MINIMUM_LENGTH + \
    SHORT_MAC_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define SERVICE_REQUEST_MAXIMUM_LENGTH ( \
    KSI_AND_SEQUENCE_NUMBER_MAXIMUM_LENGTH + \
    SHORT_MAC_MAXIMUM_LENGTH )


/*
 * Message name: Service request
 * Description: This message is sent by the UE to the network to request the establishment of a NAS signalling connection and of the radio and S1 bearers. Its structure does not follow the structure of a standard layer 3 message. See table 8.2.25.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct service_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator     protocoldiscriminator:4;
  SecurityHeaderType        securityheadertype:4;
  KsiAndSequenceNumber      ksiandsequencenumber;
  ShortMac                  messageauthenticationcode;
} service_request_msg;

int decode_service_request(service_request_msg *servicerequest, uint8_t *buffer, uint32_t len);

int encode_service_request(service_request_msg *servicerequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(SERVICE_REQUEST_H_) */

