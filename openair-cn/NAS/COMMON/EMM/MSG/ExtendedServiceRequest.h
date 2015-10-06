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

