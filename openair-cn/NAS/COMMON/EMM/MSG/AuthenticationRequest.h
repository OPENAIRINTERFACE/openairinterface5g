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
#include "NasKeySetIdentifier.h"
#include "AuthenticationParameterRand.h"
#include "AuthenticationParameterAutn.h"

#ifndef AUTHENTICATION_REQUEST_H_
#define AUTHENTICATION_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define AUTHENTICATION_REQUEST_MINIMUM_LENGTH ( \
    NAS_KEY_SET_IDENTIFIER_MINIMUM_LENGTH + \
    AUTHENTICATION_PARAMETER_RAND_MINIMUM_LENGTH + \
    AUTHENTICATION_PARAMETER_AUTN_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define AUTHENTICATION_REQUEST_MAXIMUM_LENGTH ( \
    NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH + \
    AUTHENTICATION_PARAMETER_RAND_MAXIMUM_LENGTH + \
    AUTHENTICATION_PARAMETER_AUTN_MAXIMUM_LENGTH )


/*
 * Message name: Authentication request
 * Description: This message is sent by the network to the UE to initiate authentication of the UE identity. See table 8.2.7.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct authentication_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator            protocoldiscriminator:4;
  SecurityHeaderType               securityheadertype:4;
  MessageType                      messagetype;
  NasKeySetIdentifier              naskeysetidentifierasme;
  AuthenticationParameterRand      authenticationparameterrand;
  AuthenticationParameterAutn      authenticationparameterautn;
} authentication_request_msg;

int decode_authentication_request(authentication_request_msg *authenticationrequest, uint8_t *buffer, uint32_t len);

int encode_authentication_request(authentication_request_msg *authenticationrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(AUTHENTICATION_REQUEST_H_) */

