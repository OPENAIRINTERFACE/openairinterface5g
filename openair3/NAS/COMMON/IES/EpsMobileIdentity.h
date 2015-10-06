/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
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

