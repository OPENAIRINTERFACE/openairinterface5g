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

/*! \file FGSMobileIdentity.h
 * \brief 5GS Mobile Identity for registration request procedures
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 * \date 2020
 * \version 0.1
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef FGS_MOBILE_IDENTITY_H_
#define FGS_MOBILE_IDENTITY_H_


typedef struct {
  uint8_t  spare:5;
  uint8_t  typeofidentity:3;
} NoIdentity5GSMobileIdentity_t;

typedef struct {
  uint8_t  spare1:1;
  uint8_t  supiformat:3;
  uint8_t  spare2:1;
  uint8_t  typeofidentity:3;
  uint8_t  mccdigit2:4;
  uint8_t  mccdigit1:4;
  uint8_t  mncdigit3:4;
  uint8_t  mccdigit3:4;
  uint8_t  mncdigit2:4;
  uint8_t  mncdigit1:4;
  uint8_t  routingindicatordigit2:4;
  uint8_t  routingindicatordigit1:4;
  uint8_t  routingindicatordigit4:4;
  uint8_t  routingindicatordigit3:4;
  uint8_t  spare3:1;
  uint8_t  spare4:1;
  uint8_t  spare5:1;
  uint8_t  spare6:1;
  uint8_t  protectionschemeId:4;
  uint8_t  homenetworkpki;
  char schemeoutput[32];
} Suci5GSMobileIdentity_t;

typedef struct {
  uint8_t  spare:4;
  uint8_t  oddeven:1;
  uint8_t  typeofidentity:3;
  uint8_t  mccdigit2:4;
  uint8_t  mccdigit1:4;
  uint8_t  mncdigit3:4;
  uint8_t  mccdigit3:4;
  uint8_t  mncdigit2:4;
  uint8_t  mncdigit1:4;
  uint8_t  amfregionid;
  uint16_t amfsetid:10;
  uint16_t amfpointer:6;
  uint32_t tmsi;
} Guti5GSMobileIdentity_t;


typedef struct {
  uint8_t  digit1:4;
  uint8_t  oddeven:1;
  uint8_t  typeofidentity:3;
  uint8_t  digitp1:4;
  uint8_t  digitp:4;
} Imei5GSMobileIdentity_t;

typedef struct {
  uint8_t  digittac01:4;
  uint8_t  oddeven:1;
  uint8_t  typeofidentity:3;
  uint8_t  digittac02:4;
  uint8_t  digittac03:4;
  uint8_t  digittac04:4;
  uint8_t  digittac05:4;
  uint8_t  digittac06:4;
  uint8_t  digittac07:4;
  uint8_t  digittac08:4;
  uint8_t  digit09:4;
  uint8_t  digit10:4;
  uint8_t  digit11:4;
  uint8_t  digit12:4;
  uint8_t  digit13:4;
  uint8_t  digit14:4;
  uint8_t  digitsv1:4;
  uint8_t  digitsv2:4;
  uint8_t  spare:4;
} Imeisv5GSMobileIdentity_t;


typedef struct {
  uint8_t  digit1:4;
  uint8_t  spare:1;
  uint8_t  typeofidentity:3;
  uint16_t amfsetid:10;
  uint16_t amfpointer:6;
  uint32_t tmsi;
} Stmsi5GSMobileIdentity_t;

typedef struct {
  uint8_t  spare:5;
  uint8_t  typeofidentity:3;
  uint8_t  macaddr;
} Macaddr5GSMobileIdentity_t;

typedef union FGSMobileIdentity_tag {
#define FGS_MOBILE_IDENTITY_NOIDENTITY    0b000
#define FGS_MOBILE_IDENTITY_SUCI          0b001
#define FGS_MOBILE_IDENTITY_5G_GUTI       0b010
#define FGS_MOBILE_IDENTITY_IMEI          0b011
#define FGS_MOBILE_IDENTITY_5GS_TMSI      0b100
#define FGS_MOBILE_IDENTITY_IMEISV        0b101
#define FGS_MOBILE_IDENTITY_MAC_ADDR      0b110
  NoIdentity5GSMobileIdentity_t   noidentity;
  Suci5GSMobileIdentity_t         suci;
  Guti5GSMobileIdentity_t         guti;
  Imei5GSMobileIdentity_t         imei;
  Stmsi5GSMobileIdentity_t        stmsi;
  Imeisv5GSMobileIdentity_t       imeisv;
  Macaddr5GSMobileIdentity_t      macaddress;
} FGSMobileIdentity;

int encode_5gs_mobile_identity(FGSMobileIdentity *fgsmobileidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_5gs_mobile_identity(FGSMobileIdentity *fgsmobileidentity, uint8_t iei, const uint8_t *buffer, uint32_t len);

#endif /* FGS MOBILE IDENTITY_H_ */

