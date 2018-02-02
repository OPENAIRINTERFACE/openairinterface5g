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

#ifndef EPS_MOBILE_IDENTITY_H_
#define EPS_MOBILE_IDENTITY_H_

#define EPS_MOBILE_IDENTITY_MINIMUM_LENGTH 3
#define EPS_MOBILE_IDENTITY_MAXIMUM_LENGTH 13

typedef struct {
  uint8_t  spare:4;
#define EPS_MOBILE_IDENTITY_EVEN  0
#define EPS_MOBILE_IDENTITY_ODD   1
  uint8_t  oddeven:1;
  uint8_t  typeofidentity:3;
  uint8_t  mccdigit2:4;
  uint8_t  mccdigit1:4;
  uint8_t  mncdigit3:4;
  uint8_t  mccdigit3:4;
  uint8_t  mncdigit2:4;
  uint8_t  mncdigit1:4;
  uint16_t mmegroupid;
  uint8_t  mmecode;
  uint32_t mtmsi;
} GutiEpsMobileIdentity_t;

typedef struct {
  uint8_t  digit1:4;
  uint8_t  oddeven:1;
  uint8_t  typeofidentity:3;
  uint8_t  digit2:4;
  uint8_t  digit3:4;
  uint8_t  digit4:4;
  uint8_t  digit5:4;
  uint8_t  digit6:4;
  uint8_t  digit7:4;
  uint8_t  digit8:4;
  uint8_t  digit9:4;
  uint8_t  digit10:4;
  uint8_t  digit11:4;
  uint8_t  digit12:4;
  uint8_t  digit13:4;
  uint8_t  digit14:4;
  uint8_t  digit15:4;
} ImsiEpsMobileIdentity_t;

typedef ImsiEpsMobileIdentity_t ImeiEpsMobileIdentity_t;

typedef union EpsMobileIdentity_tag {
#define EPS_MOBILE_IDENTITY_IMSI  0b001
#define EPS_MOBILE_IDENTITY_GUTI  0b110
#define EPS_MOBILE_IDENTITY_IMEI  0b011
  ImsiEpsMobileIdentity_t imsi;
  GutiEpsMobileIdentity_t guti;
  ImeiEpsMobileIdentity_t imei;
} EpsMobileIdentity;

int encode_eps_mobile_identity(EpsMobileIdentity *epsmobileidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_eps_mobile_identity(EpsMobileIdentity *epsmobileidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_mobile_identity_xml(EpsMobileIdentity *epsmobileidentity, uint8_t iei);

#endif /* EPS MOBILE IDENTITY_H_ */

