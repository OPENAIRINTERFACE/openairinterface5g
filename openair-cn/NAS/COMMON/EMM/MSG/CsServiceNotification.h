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
#include "PagingIdentity.h"
#include "Cli.h"
#include "SsCode.h"
#include "LcsIndicator.h"
#include "LcsClientIdentity.h"

#ifndef CS_SERVICE_NOTIFICATION_H_
#define CS_SERVICE_NOTIFICATION_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define CS_SERVICE_NOTIFICATION_MINIMUM_LENGTH ( \
    PAGING_IDENTITY_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define CS_SERVICE_NOTIFICATION_MAXIMUM_LENGTH ( \
    PAGING_IDENTITY_MAXIMUM_LENGTH + \
    CLI_MAXIMUM_LENGTH + \
    SS_CODE_MAXIMUM_LENGTH + \
    LCS_INDICATOR_MAXIMUM_LENGTH + \
    LCS_CLIENT_IDENTITY_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define CS_SERVICE_NOTIFICATION_CLI_PRESENT                 (1<<0)
# define CS_SERVICE_NOTIFICATION_SS_CODE_PRESENT             (1<<1)
# define CS_SERVICE_NOTIFICATION_LCS_INDICATOR_PRESENT       (1<<2)
# define CS_SERVICE_NOTIFICATION_LCS_CLIENT_IDENTITY_PRESENT (1<<3)

typedef enum cs_service_notification_iei_tag {
  CS_SERVICE_NOTIFICATION_CLI_IEI                  = 0x60, /* 0x60 = 96 */
  CS_SERVICE_NOTIFICATION_SS_CODE_IEI              = 0x61, /* 0x61 = 97 */
  CS_SERVICE_NOTIFICATION_LCS_INDICATOR_IEI        = 0x62, /* 0x62 = 98 */
  CS_SERVICE_NOTIFICATION_LCS_CLIENT_IDENTITY_IEI  = 0x63, /* 0x63 = 99 */
} cs_service_notification_iei;

/*
 * Message name: CS service notification
 * Description: This message is sent by the network when a paging request with CS call indicator was received via SGs for a UE, and a NAS signalling connection is already established for the UE. See table 8.2.9.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct cs_service_notification_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                protocoldiscriminator:4;
  SecurityHeaderType                   securityheadertype:4;
  MessageType                          messagetype;
  PagingIdentity                       pagingidentity;
  /* Optional fields */
  uint32_t                             presencemask;
  Cli                                  cli;
  SsCode                               sscode;
  LcsIndicator                         lcsindicator;
  LcsClientIdentity                    lcsclientidentity;
} cs_service_notification_msg;

int decode_cs_service_notification(cs_service_notification_msg *csservicenotification, uint8_t *buffer, uint32_t len);

int encode_cs_service_notification(cs_service_notification_msg *csservicenotification, uint8_t *buffer, uint32_t len);

#endif /* ! defined(CS_SERVICE_NOTIFICATION_H_) */

