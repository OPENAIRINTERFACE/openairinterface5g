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
#include "NetworkName.h"
#include "TimeZone.h"
#include "TimeZoneAndTime.h"
#include "DaylightSavingTime.h"

#ifndef EMM_INFORMATION_H_
#define EMM_INFORMATION_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define EMM_INFORMATION_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define EMM_INFORMATION_MAXIMUM_LENGTH ( \
    NETWORK_NAME_MAXIMUM_LENGTH + \
    NETWORK_NAME_MAXIMUM_LENGTH + \
    TIME_ZONE_MAXIMUM_LENGTH + \
    TIME_ZONE_AND_TIME_MAXIMUM_LENGTH + \
    DAYLIGHT_SAVING_TIME_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define EMM_INFORMATION_FULL_NAME_FOR_NETWORK_PRESENT              (1<<0)
# define EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_PRESENT             (1<<1)
# define EMM_INFORMATION_LOCAL_TIME_ZONE_PRESENT                    (1<<2)
# define EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_PRESENT (1<<3)
# define EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_PRESENT       (1<<4)

typedef enum emm_information_iei_tag {
  EMM_INFORMATION_FULL_NAME_FOR_NETWORK_IEI               = 0x43, /* 0x43 = 67 */
  EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_IEI              = 0x45, /* 0x45 = 69 */
  EMM_INFORMATION_LOCAL_TIME_ZONE_IEI                     = 0x46, /* 0x46 = 70 */
  EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_IEI  = 0x47, /* 0x47 = 71 */
  EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_IEI        = 0x49, /* 0x49 = 73 */
} emm_information_iei;

/*
 * Message name: EMM information
 * Description: This message is sent by the network at any time during EMM context is established to send certain information to the UE. See table 8.2.13.1.
 * Significance: local
 * Direction: network to UE
 */

typedef struct emm_information_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator         protocoldiscriminator:4;
  SecurityHeaderType            securityheadertype:4;
  MessageType                   messagetype;
  /* Optional fields */
  uint32_t                      presencemask;
  NetworkName                   fullnamefornetwork;
  NetworkName                   shortnamefornetwork;
  TimeZone                      localtimezone;
  TimeZoneAndTime               universaltimeandlocaltimezone;
  DaylightSavingTime            networkdaylightsavingtime;
} emm_information_msg;

int decode_emm_information(emm_information_msg *emminformation, uint8_t *buffer, uint32_t len);

int encode_emm_information(emm_information_msg *emminformation, uint8_t *buffer, uint32_t len);

#endif /* ! defined(EMM_INFORMATION_H_) */

