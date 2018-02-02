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
#include "EmmCause.h"
#include "AuthenticationFailureParameter.h"

#ifndef AUTHENTICATION_FAILURE_H_
#define AUTHENTICATION_FAILURE_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define AUTHENTICATION_FAILURE_MINIMUM_LENGTH ( \
    EMM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define AUTHENTICATION_FAILURE_MAXIMUM_LENGTH ( \
    EMM_CAUSE_MAXIMUM_LENGTH + \
    AUTHENTICATION_FAILURE_PARAMETER_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_PRESENT (1<<0)

typedef enum authentication_failure_iei_tag {
  AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_IEI  = 0x30, /* 0x30 = 48 */
} authentication_failure_iei;

/*
 * Message name: Authentication failure
 * Description: This message is sent by the UE to the network to indicate that authentication of the network has failed. See table 8.2.5.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct authentication_failure_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator            protocoldiscriminator:4;
  SecurityHeaderType               securityheadertype:4;
  MessageType                      messagetype;
  EmmCause                         emmcause;
  /* Optional fields */
  uint32_t                         presencemask;
  AuthenticationFailureParameter   authenticationfailureparameter;
} authentication_failure_msg;

int decode_authentication_failure(authentication_failure_msg *authenticationfailure, uint8_t *buffer, uint32_t len);

int encode_authentication_failure(authentication_failure_msg *authenticationfailure, uint8_t *buffer, uint32_t len);

#endif /* ! defined(AUTHENTICATION_FAILURE_H_) */

