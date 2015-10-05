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
#include "DetachType.h"
#include "NasKeySetIdentifier.h"
#include "EpsMobileIdentity.h"

#ifndef DETACH_REQUEST_H_
#define DETACH_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define DETACH_REQUEST_MINIMUM_LENGTH ( \
    DETACH_TYPE_MINIMUM_LENGTH + \
    NAS_KEY_SET_IDENTIFIER_MINIMUM_LENGTH + \
    EPS_MOBILE_IDENTITY_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define DETACH_REQUEST_MAXIMUM_LENGTH ( \
    DETACH_TYPE_MAXIMUM_LENGTH + \
    NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH + \
    EPS_MOBILE_IDENTITY_MAXIMUM_LENGTH )


/*
 * Message name: Detach request
 * Description: This message is sent by the UE to request the release of an EMM context. See table 8.2.11.1.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct detach_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator        protocoldiscriminator:4;
  SecurityHeaderType           securityheadertype:4;
  MessageType                  messagetype;
  DetachType                   detachtype;
  NasKeySetIdentifier          naskeysetidentifier;
  EpsMobileIdentity            gutiorimsi;
} detach_request_msg;

int decode_detach_request(detach_request_msg *detachrequest, uint8_t *buffer, uint32_t len);

int encode_detach_request(detach_request_msg *detachrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(DETACH_REQUEST_H_) */

