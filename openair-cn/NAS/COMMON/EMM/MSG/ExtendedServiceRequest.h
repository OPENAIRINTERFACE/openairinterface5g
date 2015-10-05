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
#include "ServiceType.h"
#include "NasKeySetIdentifier.h"
#include "MobileIdentity.h"
#include "CsfbResponse.h"

#ifndef EXTENDED_SERVICE_REQUEST_H_
#define EXTENDED_SERVICE_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define EXTENDED_SERVICE_REQUEST_MINIMUM_LENGTH ( \
    SERVICE_TYPE_MINIMUM_LENGTH + \
    NAS_KEY_SET_IDENTIFIER_MINIMUM_LENGTH + \
    MOBILE_IDENTITY_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define EXTENDED_SERVICE_REQUEST_MAXIMUM_LENGTH ( \
    SERVICE_TYPE_MAXIMUM_LENGTH + \
    NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH + \
    MOBILE_IDENTITY_MAXIMUM_LENGTH + \
    CSFB_RESPONSE_MAXIMUM_LENGTH )


/*
 * Message name: Extended service request
 * Description: This message is sent by the UE to the network to initiate a CS fallback call or respond to a mobile terminated CS fallback request from the network. See table 8.2.15.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct extended_service_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                 protocoldiscriminator:4;
  SecurityHeaderType                    securityheadertype:4;
  MessageType                           messagetype;
  ServiceType                           servicetype;
  NasKeySetIdentifier                   naskeysetidentifier;
  MobileIdentity                        mtmsi;
  /* Optional fields */
  uint32_t                              presencemask;
  CsfbResponse                          csfbresponse;
} extended_service_request_msg;

int decode_extended_service_request(extended_service_request_msg *extendedservicerequest, uint8_t *buffer, uint32_t len);

int encode_extended_service_request(extended_service_request_msg *extendedservicerequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(EXTENDED_SERVICE_REQUEST_H_) */

