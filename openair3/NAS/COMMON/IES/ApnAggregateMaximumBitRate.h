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

#include "OctetString.h"

#ifndef APN_AGGREGATE_MAXIMUM_BIT_RATE_H_
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_H_

#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MINIMUM_LENGTH 4
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_LENGTH 8

#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT  (1<<0)
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION2_PRESENT (1<<1)

typedef struct ApnAggregateMaximumBitRate_tag {
  uint8_t  apnambrfordownlink;
  uint8_t  apnambrforuplink;
  uint8_t  apnambrfordownlink_extended;
  uint8_t  apnambrforuplink_extended;
  uint8_t  apnambrfordownlink_extended2;
  uint8_t  apnambrforuplink_extended2;
  uint8_t  extensions;
} ApnAggregateMaximumBitRate;

int encode_apn_aggregate_maximum_bit_rate(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_apn_aggregate_maximum_bit_rate(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_apn_aggregate_maximum_bit_rate_xml(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei);

#endif /* APN AGGREGATE MAXIMUM BIT RATE_H_ */

