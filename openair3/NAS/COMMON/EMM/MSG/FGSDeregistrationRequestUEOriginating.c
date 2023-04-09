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

#include "FGSDeregistrationRequestUEOriginating.h"

int encode_fgs_deregistration_request_ue_originating(fgs_deregistration_request_ue_originating_msg *drr,
                                                     uint8_t *buffer,
                                                     uint32_t len)
{
  int encoded = 0;
  FGSDeregistrationType *dt = &drr->deregistrationtype;
  *(buffer + encoded) = ((dt->switchoff & 0x1) << 7)
                      | ((dt->reregistration_required & 0x1) << 6)
                      | ((dt->access_type & 0x3) << 4);

  int encode_result;
  if ((encode_result = encode_nas_key_set_identifier(&drr->naskeysetidentifier, 0, buffer + encoded, len - encoded)) < 0)
    return encode_result;

  encoded++;

  if ((encode_result = encode_5gs_mobile_identity(&drr->fgsmobileidentity, 0, buffer + encoded, len - encoded)) < 0)
    return encode_result;
  else
    encoded += encode_result;

  return encoded;
}
