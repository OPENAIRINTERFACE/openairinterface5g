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

/*! \file FGSRegistrationResult.h

\brief 5GS Registration result for registration request procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"



#ifndef FGS_REGISTRATION_RESULT_H_
#define FGS_REGISTRATION_RESULT_H_
#define FGS_REGISTRATION_RESULT_3GPP                      0b001
#define FGS_REGISTRATION_RESULT_NON_3GPP                  0b010
#define FGS_REGISTRATION_RESULT_3GPP_AND_NON_3GPP         0b011

typedef struct {
  uint8_t  iei;
  uint8_t  resultlength;
  uint8_t  spare:4;
  uint8_t  smsallowed:1;
  uint8_t  registrationresult:3;
} FGSRegistrationResult;


uint16_t encode_fgs_registration_result(FGSRegistrationResult *fgsregistrationresult);

int decode_fgs_registration_result(FGSRegistrationResult *fgsregistrationresult, uint8_t iei, uint16_t value, uint32_t len);

#endif /* FGS REGISTRATION RESULT_H_*/

