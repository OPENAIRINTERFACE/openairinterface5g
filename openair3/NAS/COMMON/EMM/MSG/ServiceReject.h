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
#include "GprsTimer.h"

#ifndef SERVICE_REJECT_H_
#define SERVICE_REJECT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define SERVICE_REJECT_MINIMUM_LENGTH ( \
    EMM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define SERVICE_REJECT_MAXIMUM_LENGTH ( \
    EMM_CAUSE_MAXIMUM_LENGTH + \
    GPRS_TIMER_MAXIMUM_LENGTH )


/*
 * Message name: Service reject
 * Description: This message is sent by the network to the UE in order to reject the service request procedure. See table 8.2.24.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct service_reject_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator        protocoldiscriminator:4;
  SecurityHeaderType           securityheadertype:4;
  MessageType                  messagetype;
  EmmCause                     emmcause;
  /* Optional fields */
  uint32_t                     presencemask;
  GprsTimer                    t3442value;
} service_reject_msg;

int decode_service_reject(service_reject_msg *servicereject, uint8_t *buffer, uint32_t len);

int encode_service_reject(service_reject_msg *servicereject, uint8_t *buffer, uint32_t len);

#endif /* ! defined(SERVICE_REJECT_H_) */

