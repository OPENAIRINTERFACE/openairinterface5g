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

/*! \file RegistrationAccept.c

\brief 5GS registration accept procedures
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
#include "RegistrationAccept.h"
#include "assertions.h"

int decode_registration_accept(registration_accept_msg *registration_accept, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  /* Decoding mandatory fields */
  if ((decoded_result = decode_fgs_registration_result(&registration_accept->fgsregistrationresult, 0, *(buffer + decoded), len - decoded)) < 0)
    return decoded_result;

  decoded += decoded_result;

  // todo ,Decoding optional fields
  return decoded;
}

int encode_registration_accept(registration_accept_msg *registration_accept, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;

  LOG_FUNC_IN;


  *(buffer + encoded) = encode_fgs_registration_result(&registration_accept->fgsregistrationresult);
  encoded = encoded + 2;

  // todo ,Encoding optional fields
  LOG_FUNC_RETURN(encoded);
}

