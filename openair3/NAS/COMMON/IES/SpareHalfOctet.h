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


/*! \file SpareHalfOctet.h

\brief registration request procedures for gNB
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef SPARE_HALF_OCTET_H_
#define SPARE_HALF_OCTET_H_

#define SPARE_HALF_OCTET_MINIMUM_LENGTH 1
#define SPARE_HALF_OCTET_MAXIMUM_LENGTH 1

typedef uint8_t SpareHalfOctet;

int encode_spare_half_octet(SpareHalfOctet *sparehalfoctet, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_spare_half_octet(SpareHalfOctet *sparehalfoctet, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* SPARE HALF OCTET_H_ */

