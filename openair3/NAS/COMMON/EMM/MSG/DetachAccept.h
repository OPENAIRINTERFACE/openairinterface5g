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

#ifndef DETACH_ACCEPT_H_
#define DETACH_ACCEPT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define DETACH_ACCEPT_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define DETACH_ACCEPT_MAXIMUM_LENGTH (0)

/*
 * Message name: Detach accept
 * Description: This message is sent by the network to indicate that the detach procedure has been completed. See table 8.2.10.1.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct detach_accept_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator       protocoldiscriminator:4;
  SecurityHeaderType          securityheadertype:4;
  MessageType                 messagetype;
} detach_accept_msg;

int decode_detach_accept(detach_accept_msg *detachaccept, uint8_t *buffer, uint32_t len);

int encode_detach_accept(detach_accept_msg *detachaccept, uint8_t *buffer, uint32_t len);

#endif /* ! defined(DETACH_ACCEPT_H_) */

