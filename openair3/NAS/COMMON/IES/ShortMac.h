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

#ifndef SHORT_MAC_H_
#define SHORT_MAC_H_

#define SHORT_MAC_MINIMUM_LENGTH 3
#define SHORT_MAC_MAXIMUM_LENGTH 3

typedef uint16_t ShortMac;

int encode_short_mac(ShortMac *shortmac, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_short_mac_xml(ShortMac *shortmac, uint8_t iei);

int decode_short_mac(ShortMac *shortmac, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* SHORT MAC_H_ */

