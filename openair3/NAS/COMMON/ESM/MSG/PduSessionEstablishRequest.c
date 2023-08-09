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

/*! \file PduSessionEstablishRequest.c

\brief pdu session establishment request procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "PduSessionEstablishRequest.h"


int encode_pdu_session_establishment_request(pdu_session_establishment_request_msg *pdusessionestablishrequest, uint8_t *buffer)
{
  int encoded = 0;


  *(buffer + encoded) = pdusessionestablishrequest->protocoldiscriminator;
  encoded++;
  *(buffer + encoded) = pdusessionestablishrequest->pdusessionid;
  encoded++;
  *(buffer + encoded) = pdusessionestablishrequest->pti;
  encoded++;
  *(buffer + encoded) = pdusessionestablishrequest->pdusessionestblishmsgtype;
  encoded++;

  IES_ENCODE_U16(buffer, encoded, pdusessionestablishrequest->maxdatarate);
  *(buffer + encoded) = pdusessionestablishrequest->pdusessiontype;
  encoded++;

  return encoded;
}


