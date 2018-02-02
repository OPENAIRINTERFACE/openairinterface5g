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
#include "EpsBearerIdentity.h"
#include "ProcedureTransactionIdentity.h"
#include "MessageType.h"
#include "EsmCause.h"

#ifndef ESM_STATUS_H_
#define ESM_STATUS_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ESM_STATUS_MINIMUM_LENGTH ( \
    ESM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define ESM_STATUS_MAXIMUM_LENGTH ( \
    ESM_CAUSE_MAXIMUM_LENGTH )


/*
 * Message name: ESM status
 * Description: This message is sent by the network or the UE to pass information on the status of the indicated EPS bearer context and report certain error conditions (e.g. as listed in clause 7). See table 8.3.15.1.
 * Significance: dual
 * Direction: both
 */

typedef struct esm_status_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator        protocoldiscriminator:4;
  EpsBearerIdentity            epsbeareridentity:4;
  ProcedureTransactionIdentity proceduretransactionidentity;
  MessageType                  messagetype;
  EsmCause                     esmcause;
} esm_status_msg;

int decode_esm_status(esm_status_msg *esmstatus, uint8_t *buffer, uint32_t len);

int encode_esm_status(esm_status_msg *esmstatus, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ESM_STATUS_H_) */

