/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

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

