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
#include "MobileIdentity.h"

#ifndef SECURITY_MODE_COMPLETE_H_
#define SECURITY_MODE_COMPLETE_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define SECURITY_MODE_COMPLETE_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define SECURITY_MODE_COMPLETE_MAXIMUM_LENGTH ( \
    MOBILE_IDENTITY_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define SECURITY_MODE_COMPLETE_IMEISV_PRESENT (1<<0)

typedef enum security_mode_complete_iei_tag {
  SECURITY_MODE_COMPLETE_IMEISV_IEI  = 0x23, /* 0x23 = 35 */
} security_mode_complete_iei;

/*
 * Message name: Security mode complete
 * Description: This message is sent by the UE to the network in response to a SECURITY MODE COMMAND message. See table 8.2.21.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct security_mode_complete_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator               protocoldiscriminator:4;
  SecurityHeaderType                  securityheadertype:4;
  MessageType                         messagetype;
  /* Optional fields */
  uint32_t                            presencemask;
  MobileIdentity                      imeisv;
} security_mode_complete_msg;

int decode_security_mode_complete(security_mode_complete_msg *securitymodecomplete, uint8_t *buffer, uint32_t len);

int encode_security_mode_complete(security_mode_complete_msg *securitymodecomplete, uint8_t *buffer, uint32_t len);

#endif /* ! defined(SECURITY_MODE_COMPLETE_H_) */

