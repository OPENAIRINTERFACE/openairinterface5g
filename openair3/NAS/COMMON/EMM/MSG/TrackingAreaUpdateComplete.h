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

