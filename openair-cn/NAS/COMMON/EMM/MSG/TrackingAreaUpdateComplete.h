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

#ifndef TRACKING_AREA_UPDATE_COMPLETE_H_
#define TRACKING_AREA_UPDATE_COMPLETE_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define TRACKING_AREA_UPDATE_COMPLETE_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define TRACKING_AREA_UPDATE_COMPLETE_MAXIMUM_LENGTH (0)

/*
 * Message name: Tracking area update complete
 * Description: This message shall be sent by the UE to the network in response to a tracking area update accept message if a GUTI has been changed or a new TMSI has been assigned. See table 8.2.27.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct tracking_area_update_complete_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                     protocoldiscriminator:4;
  SecurityHeaderType                        securityheadertype:4;
  MessageType                               messagetype;
} tracking_area_update_complete_msg;

int decode_tracking_area_update_complete(tracking_area_update_complete_msg *trackingareaupdatecomplete, uint8_t *buffer, uint32_t len);

int encode_tracking_area_update_complete(tracking_area_update_complete_msg *trackingareaupdatecomplete, uint8_t *buffer, uint32_t len);

#endif /* ! defined(TRACKING_AREA_UPDATE_COMPLETE_H_) */

