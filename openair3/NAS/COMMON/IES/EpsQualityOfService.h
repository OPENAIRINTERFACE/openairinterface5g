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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef EPS_QUALITY_OF_SERVICE_H_
#define EPS_QUALITY_OF_SERVICE_H_

#define EPS_QUALITY_OF_SERVICE_MINIMUM_LENGTH 2
#define EPS_QUALITY_OF_SERVICE_MAXIMUM_LENGTH 10

typedef struct {
  uint8_t maxBitRateForUL;
  uint8_t maxBitRateForDL;
  uint8_t guarBitRateForUL;
  uint8_t guarBitRateForDL;
} EpsQoSBitRates;

typedef struct {
  uint8_t bitRatesPresent:1;
  uint8_t bitRatesExtPresent:1;
  uint8_t qci;
  EpsQoSBitRates bitRates;
  EpsQoSBitRates bitRatesExt;
} EpsQualityOfService;

//typedef uint8_t EpsQualityOfService;

int encode_eps_quality_of_service(EpsQualityOfService *epsqualityofservice, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_eps_quality_of_service(EpsQualityOfService *epsqualityofservice, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_quality_of_service_xml(EpsQualityOfService *epsqualityofservice, uint8_t iei);

int eps_qos_bit_rate_value(uint8_t br);
int eps_qos_bit_rate_ext_value(uint8_t br);

#endif /* EPS QUALITY OF SERVICE_H_ */

