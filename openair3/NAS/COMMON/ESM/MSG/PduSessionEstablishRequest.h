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

/*! \file PduSessionEstablishRequest.h

\brief pdu session establishment request procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ExtendedProtocolDiscriminator.h"
#include "MessageType.h"

#ifndef PDU_SESSION_ESTABLISHMENT_REQUEST_H_
#define PDU_SESSION_ESTABLISHMENT_REQUEST_H_


/*
 * Message name: pdu session establishment request
 * Description: The PDU SESSION ESTABLISHMENT REQUEST message is sent by the UE to the SMF to initiate establishment of a PDU session. See table 8.3.1.1.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct pdu_session_establishment_request_msg_tag {
    /* Mandatory fields */
    ExtendedProtocolDiscriminator           protocoldiscriminator;
    uint8_t                                 pdusessionid;
    uint8_t                                 pti;
    MessageType                             pdusessionestblishmsgtype;
    uint16_t                                maxdatarate;
    uint8_t                                 pdusessiontype;
    /* Optional fields */
} pdu_session_establishment_request_msg;


int encode_pdu_session_establishment_request(pdu_session_establishment_request_msg *pdusessionestablishrequest, uint8_t *buffer);

#endif /* ! defined(PDU_SESSION_ESTABLISHMENT_REQUEST_H_) */


