/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef MOBILE_IDENTITY_H_
#define MOBILE_IDENTITY_H_

#define MOBILE_IDENTITY_MINIMUM_LENGTH 3
#define MOBILE_IDENTITY_MAXIMUM_LENGTH 11

#define MOBILE_IDENTITY_NOT_AVAILABLE_GSM_LENGTH  1
#define MOBILE_IDENTITY_NOT_AVAILABLE_GPRS_LENGTH 3
#define MOBILE_IDENTITY_NOT_AVAILABLE_LTE_LENGTH  3

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
} ImsiMobileIdentity_t;

typedef struct {
  uint8_t  spare:2;
  uint8_t  mbmssessionidindication:1;
  uint8_t  mccmncindication:1;
#define MOBILE_IDENTITY_EVEN    0
#define MOBILE_IDENTITY_ODD   1
  uint8_t  oddeven:1;
  uint8_t  typeofidentity:3;
  uint32_t mbmsserviceid;
  uint8_t  mccdigit2:4;
  uint8_t  mccdigit1:4;
  uint8_t  mncdigit3:4;
  uint8_t  mccdigit3:4;
  uint8_t  mncdigit2:4;
  uint8_t  mncdigit1:4;
  uint8_t  mbmssessionid;
} TmgiMobileIdentity_t;

typedef ImsiMobileIdentity_t ImeiMobileIdentity_t;
typedef ImsiMobileIdentity_t ImeisvMobileIdentity_t;
typedef ImsiMobileIdentity_t TmsiMobileIdentity_t;
typedef ImsiMobileIdentity_t NoMobileIdentity_t;

typedef union MobileIdentity_tag {
#define MOBILE_IDENTITY_IMSI    0b001
#define MOBILE_IDENTITY_IMEI    0b010
#define MOBILE_IDENTITY_IMEISV    0b011
#define MOBILE_IDENTITY_TMSI    0b100
#define MOBILE_IDENTITY_TMGI    0b101
#define MOBILE_IDENTITY_NOT_AVAILABLE 0b000
  ImsiMobileIdentity_t imsi;
  ImeiMobileIdentity_t imei;
  ImeisvMobileIdentity_t imeisv;
  TmsiMobileIdentity_t tmsi;
  TmgiMobileIdentity_t tmgi;
  NoMobileIdentity_t no_id;
} MobileIdentity;

int encode_mobile_identity(MobileIdentity *mobileidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_mobile_identity(MobileIdentity *mobileidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_mobile_identity_xml(MobileIdentity *mobileidentity, uint8_t iei);

#endif /* MOBILE IDENTITY_H_ */

