/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
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

