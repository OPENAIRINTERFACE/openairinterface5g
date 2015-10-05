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

