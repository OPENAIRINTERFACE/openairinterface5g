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
 * Author and copyright: Laurent Thomas, open-cells.com
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

#ifndef XNAP_COMMON_H_
#define XNAP_COMMON_H_

#include "XNAP_XnAP-PDU.h"
#include "intertask_interface.h"
#include "common/openairinterface5g_limits.h"
#include "oai_asn1.h"
#include "XNAP_ProtocolIE-Field.h"
#include "XNAP_InitiatingMessage.h"
#include "XNAP_ProtocolIE-ContainerPair.h"
#include "XNAP_ProtocolExtensionField.h"
#include "XNAP_ProtocolExtensionContainer.h"
#include "XNAP_asn_constant.h"

#ifndef XNAP_PORT
#define XNAP_PORT 38422
#endif

#define XNAP_FIND_PROTOCOLIE_BY_ID(IE_TYPE, ie, container, IE_ID, mandatory)                                                   \
  do {                                                                                                                         \
    IE_TYPE **ptr;                                                                                                             \
    ie = NULL;                                                                                                                 \
    for (ptr = container->protocolIEs.list.array; ptr < &container->protocolIEs.list.array[container->protocolIEs.list.count]; \
         ptr++) {                                                                                                              \
      if ((*ptr)->id == IE_ID) {                                                                                               \
        ie = *ptr;                                                                                                             \
        break;                                                                                                                 \
      }                                                                                                                        \
    }                                                                                                                          \
    if (mandatory)                                                                                                             \
      DevAssert(ie != NULL);                                                                                                   \
  } while (0)

ssize_t xnap_generate_initiating_message(uint8_t **buffer,
                                         uint32_t *length,
                                         XNAP_ProcedureCode_t procedureCode,
                                         XNAP_Criticality_t criticality,
                                         asn_TYPE_descriptor_t *td,
                                         void *sptr);

#endif /* XNAP_COMMON_H_ */
