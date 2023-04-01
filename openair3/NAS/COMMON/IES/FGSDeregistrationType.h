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

#ifndef FGS_DEREGISTRATION_TYPE_H_
#define FGS_DEREGISTRATION_TYPE_H_

#include <stdint.h>
#include "OctetString.h"

#define FGS_DEREGISTRATION_TYPE_MINIMUM_LENGTH 1
#define FGS_DEREGISTRATION_TYPE_MAXIMUM_LENGTH 1

typedef struct FGSDeregistrationType_tag {
#define NORMAL_DEREGISTRATION 0
#define SWITCH_OFF 1
  uint8_t switchoff: 1;
#define REREGISTRATION_NOT_REQUIRED 0
#define REREGISTRATION_REQUIRED 1
  uint8_t reregistration_required: 1;
#define TGPP_ACCESS 1
#define NON_TGPP_ACCESS 2
#define TGPP_AND_NON_TGPP_ACCESS 3
  uint8_t access_type: 2;
} FGSDeregistrationType;

int encode_fgs_deregistration_type(FGSDeregistrationType *dt, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* FGS_DEREGISTRATION_TYPE_H_ */
