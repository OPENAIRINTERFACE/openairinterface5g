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

/*! \file FGCNasMessageContainer.h

\brief security mode complete procedures for gNB
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef FGC_NAS_MESSAGE_CONTAINER_H_
#define FGC_NAS_MESSAGE_CONTAINER_H_

#define FGC_NAS_MESSAGE_CONTAINER_MINIMUM_LENGTH 4
#define FGC_NAS_MESSAGE_CONTAINER_MAXIMUM_LENGTH 65535

typedef struct FGCNasMessageContainer_tag {
  OctetString nasmessagecontainercontents;
} FGCNasMessageContainer;

int encode_fgc_nas_message_container(FGCNasMessageContainer *nasmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_fgc_nas_message_container(FGCNasMessageContainer *nasmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_fgc_nas_message_container_xml(FGCNasMessageContainer *nasmessagecontainer, uint8_t iei);

#endif /* FGC NAS MESSAGE CONTAINER_H_ */

