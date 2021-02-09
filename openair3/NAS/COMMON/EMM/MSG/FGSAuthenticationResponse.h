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

/*! \file FGSAuthenticationResponse.h

\brief authentication response procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ExtendedProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "SpareHalfOctet.h"
#include "MessageType.h"
#include "AuthenticationResponseParameter.h"

#ifndef FGS_AUTHENTICATION_RESPONSE_H_
#define FGS_AUTHENTICATION_RESPONSE_H_

#define AUTHENTICATION_RESPONSE_PARAMETER_IEI     0x2d

/*
 * Message name: Identity response
 * Description: This message is sent by the UE to the AMF to provide the requested identity. See table 8.2.22.1.
 * Significance: dual
 * Direction: UE to AMF
 */

typedef struct fgs_authentication_response_msg_tag {
  /* Mandatory fields */
  ExtendedProtocolDiscriminator           protocoldiscriminator;
  SecurityHeaderType                      securityheadertype:4;
  SpareHalfOctet                          sparehalfoctet:4;
  MessageType                             messagetype;
  AuthenticationResponseParameter         authenticationresponseparameter;
} fgs_authentication_response_msg;

int encode_fgs_authentication_response(fgs_authentication_response_msg *authentication_response, uint8_t *buffer, uint32_t len);

#endif /* ! defined(FGS_AUTHENTICATION_RESPONSE_H_) */


