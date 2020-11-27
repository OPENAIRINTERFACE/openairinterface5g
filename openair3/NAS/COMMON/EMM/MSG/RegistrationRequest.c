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

/*! \file RegistrationRequest.c
 * \brief registration request procedures for gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 * \date 2020
 * \version 0.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "nas_log.h"

#include "RegistrationRequest.h"

int decode_registration_request(registration_request_msg *registration_request, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  LOG_FUNC_IN;
  LOG_TRACE(INFO, "EMM  - registration_request len = %d",
            len);

  /* Decoding mandatory fields */
  if ((decoded_result = decode_5gs_registration_type(&registration_request->fgsregistrationtype, 0, *(buffer + decoded)  & 0x0f, len - decoded)) < 0) {
    //         return decoded_result;
    LOG_FUNC_RETURN(decoded_result);
  }

  if ((decoded_result = decode_u8_nas_key_set_identifier(&registration_request->naskeysetidentifier, 0, *(buffer + decoded) >> 4, len - decoded)) < 0) {
    //         return decoded_result;
    LOG_FUNC_RETURN(decoded_result);
  }

  decoded++;

  if ((decoded_result = decode_5gs_mobile_identity(&registration_request->fgsmobileidentity, 0, buffer + decoded, len - decoded)) < 0) {
    //         return decoded_result;
    LOG_FUNC_RETURN(decoded_result);
  } else
    decoded += decoded_result;


  // TODO, Decoding optional fields

  return decoded;
}

int encode_registration_request(registration_request_msg *registration_request, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  *(buffer + encoded) = ((encode_u8_nas_key_set_identifier(&registration_request->naskeysetidentifier) & 0x0f) << 4) | (encode_5gs_registration_type(&registration_request->fgsregistrationtype) & 0x0f);
  encoded++;

  if ((encode_result =
         encode_5gs_mobile_identity(&registration_request->fgsmobileidentity, 0, buffer +
                                    encoded, len - encoded)) < 0)        //Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  if ((registration_request->presencemask & REGISTRATION_REQUEST_5GMM_CAPABILITY_PRESENT)
      == REGISTRATION_REQUEST_5GMM_CAPABILITY_PRESENT) {
    if ((encode_result = encode_5gmm_capability(&registration_request->fgmmcapability,
                         REGISTRATION_REQUEST_5GMM_CAPABILITY_IEI, buffer + encoded, len -
                         encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((registration_request->presencemask & REGISTRATION_REQUEST_UE_SECURITY_CAPABILITY_PRESENT)
      == REGISTRATION_REQUEST_UE_SECURITY_CAPABILITY_PRESENT) {
    if ((encode_result = encode_nrue_security_capability(&registration_request->nruesecuritycapability,
                         REGISTRATION_REQUEST_UE_SECURITY_CAPABILITY_IEI, buffer + encoded, len -
                         encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }


  // TODO, Encoding optional fields
  return encoded;
}

