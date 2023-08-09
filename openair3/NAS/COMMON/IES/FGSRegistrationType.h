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

/*! \file FGSRegistrationType.h
 * \brief 5GS Registration Type for registration request procedures
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 * \date 2020
 * \version 0.1
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef FGS_REGISTRATION_TYPE_H_
#define FGS_REGISTRATION_TYPE_H_

#define FGS_REGISTRATION_TYPE_MINIMUM_LENGTH 1
#define FGS_REGISTRATION_TYPE_MAXIMUM_LENGTH 1

typedef uint8_t FGSRegistrationType;


#define INITIAL_REGISTRATION               0b001
#define MOBILITY_REGISTRATION_UPDATING     0b010
#define PERIODIC_REGISTRATION_UPDATING     0b011
#define EMERGENCY_REGISTRATION             0b100

int encode_5gs_registration_type(FGSRegistrationType *fgsregistrationtype);

int decode_5gs_registration_type(FGSRegistrationType *fgsregistrationtype, uint8_t iei, uint8_t value, uint32_t len);


#endif /* FGS_REGISTRATION_TYPE_H_*/

