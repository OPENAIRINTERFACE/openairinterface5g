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


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "EpsMobileIdentity.h"

static int decode_guti_eps_mobile_identity(GutiEpsMobileIdentity_t *guti, uint8_t *buffer);
static int decode_imsi_eps_mobile_identity(ImsiEpsMobileIdentity_t *imsi, uint8_t *buffer);
static int decode_imei_eps_mobile_identity(ImeiEpsMobileIdentity_t *imei, uint8_t *buffer);

static int encode_guti_eps_mobile_identity(GutiEpsMobileIdentity_t *guti, uint8_t *buffer);
static int encode_imsi_eps_mobile_identity(ImsiEpsMobileIdentity_t *imsi, uint8_t *buffer);
static int encode_imei_eps_mobile_identity(ImeiEpsMobileIdentity_t *imei, uint8_t *buffer);

int decode_eps_mobile_identity(EpsMobileIdentity *epsmobileidentity, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  int decoded_rc = TLV_DECODE_VALUE_DOESNT_MATCH;
  int decoded = 0;
  uint8_t ielen = 0;

  if (iei > 0) {
    CHECK_IEI_DECODER(iei, *buffer);
    decoded++;
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER(len - decoded, ielen);

  uint8_t typeofidentity = *(buffer + decoded) & 0x7;

  if (typeofidentity == EPS_MOBILE_IDENTITY_IMSI) {
    decoded_rc = decode_imsi_eps_mobile_identity(&epsmobileidentity->imsi,
                 buffer + decoded);
  } else if (typeofidentity == EPS_MOBILE_IDENTITY_GUTI) {
    decoded_rc = decode_guti_eps_mobile_identity(&epsmobileidentity->guti,
                 buffer + decoded);
  } else if (typeofidentity == EPS_MOBILE_IDENTITY_IMEI) {
    decoded_rc = decode_imei_eps_mobile_identity(&epsmobileidentity->imei,
                 buffer + decoded);
  }

  if (decoded_rc < 0) {
    return decoded_rc;
  }

#if defined (NAS_DEBUG)
  dump_eps_mobile_identity_xml(epsmobileidentity, iei);
#endif
  return (decoded + decoded_rc);
}

int encode_eps_mobile_identity(EpsMobileIdentity *epsmobileidentity, uint8_t iei, uint8_t *buffer, uint32_t len)
{
  uint8_t *lenPtr;
  int encoded_rc = TLV_ENCODE_VALUE_DOESNT_MATCH;
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, EPS_MOBILE_IDENTITY_MINIMUM_LENGTH, len);
#if defined (NAS_DEBUG)
  dump_eps_mobile_identity_xml(epsmobileidentity, iei);
#endif

  if (iei > 0) {
    *buffer = iei;
    encoded++;
  }

  lenPtr  = (buffer + encoded);
  encoded ++;

  if (epsmobileidentity->imsi.typeofidentity == EPS_MOBILE_IDENTITY_IMSI) {
    encoded_rc = encode_imsi_eps_mobile_identity(&epsmobileidentity->imsi,
                 buffer + encoded);
  } else if (epsmobileidentity->guti.typeofidentity == EPS_MOBILE_IDENTITY_GUTI) {
    encoded_rc = encode_guti_eps_mobile_identity(&epsmobileidentity->guti,
                 buffer + encoded);
  } else if (epsmobileidentity->imei.typeofidentity == EPS_MOBILE_IDENTITY_IMEI) {
    encoded_rc = encode_imei_eps_mobile_identity(&epsmobileidentity->imei,
                 buffer + encoded);
  }

  if (encoded_rc < 0) {
    return encoded_rc;
  }

  *lenPtr = encoded  + encoded_rc - 1 - ((iei > 0) ? 1 : 0);
  return (encoded + encoded_rc);
}

void dump_eps_mobile_identity_xml(EpsMobileIdentity *epsmobileidentity, uint8_t iei)
{
  printf("<Eps Mobile Identity>\n");

  if (iei > 0)
    /* Don't display IEI if = 0 */
    printf("    <IEI>0x%X</IEI>\n", iei);

  if (epsmobileidentity->imsi.typeofidentity == EPS_MOBILE_IDENTITY_IMSI) {
    ImsiEpsMobileIdentity_t* imsi = &epsmobileidentity->imsi;
    printf("    <odd even>%u</odd even>\n", imsi->oddeven);
    printf("    <Type of identity>IMSI</Type of identity>\n");
    printf("    <digit1>%u</digit1>\n", imsi->digit1);
    printf("    <digit2>%u</digit2>\n", imsi->digit2);
    printf("    <digit3>%u</digit3>\n", imsi->digit3);
    printf("    <digit4>%u</digit4>\n", imsi->digit4);
    printf("    <digit5>%u</digit5>\n", imsi->digit5);
    printf("    <digit6>%u</digit6>\n", imsi->digit6);
    printf("    <digit7>%u</digit7>\n", imsi->digit7);
    printf("    <digit8>%u</digit8>\n", imsi->digit8);
    printf("    <digit9>%u</digit9>\n", imsi->digit9);
    printf("    <digit10>%u</digit10>\n", imsi->digit10);
    printf("    <digit11>%u</digit11>\n", imsi->digit11);
    printf("    <digit12>%u</digit12>\n", imsi->digit12);
    printf("    <digit13>%u</digit13>\n", imsi->digit13);
    printf("    <digit14>%u</digit14>\n", imsi->digit14);
    printf("    <digit15>%u</digit15>\n", imsi->digit15);
  } else if (epsmobileidentity->guti.typeofidentity == EPS_MOBILE_IDENTITY_GUTI) {
    GutiEpsMobileIdentity_t* guti = &epsmobileidentity->guti;
    printf("    <odd even>%u</odd even>\n", guti->oddeven);
    printf("    <Type of identity>GUTI</Type of identity>\n");
    printf("    <MCC digit 1>%u</MCC digit 1>\n", guti->mccdigit1);
    printf("    <MCC digit 2>%u</MCC digit 2>\n", guti->mccdigit2);
    printf("    <MCC digit 3>%u</MCC digit 3>\n", guti->mccdigit3);
    printf("    <MNC digit 1>%u</MNC digit 1>\n", guti->mncdigit1);
    printf("    <MNC digit 2>%u</MNC digit 2>\n", guti->mncdigit2);
    printf("    <MNC digit 3>%u</MNC digit 3>\n", guti->mncdigit3);
    printf("    <MME group id>%u</MME group id>\n", guti->mmegroupid);
    printf("    <MME code>%u</MME code>\n", guti->mmecode);
    printf("    <M TMSI>%u</M TMSI>\n", guti->mtmsi);
  } else if (epsmobileidentity->imei.typeofidentity == EPS_MOBILE_IDENTITY_IMEI) {
    ImeiEpsMobileIdentity_t* imei = &epsmobileidentity->imei;
    printf("    <odd even>%u</odd even>\n", imei->oddeven);
    printf("    <Type of identity>IMEI</Type of identity>\n");
    printf("    <digit1>%u</digit1>\n", imei->digit1);
    printf("    <digit2>%u</digit2>\n", imei->digit2);
    printf("    <digit3>%u</digit3>\n", imei->digit3);
    printf("    <digit4>%u</digit4>\n", imei->digit4);
    printf("    <digit5>%u</digit5>\n", imei->digit5);
    printf("    <digit6>%u</digit6>\n", imei->digit6);
    printf("    <digit7>%u</digit7>\n", imei->digit7);
    printf("    <digit8>%u</digit8>\n", imei->digit8);
    printf("    <digit9>%u</digit9>\n", imei->digit9);
    printf("    <digit10>%u</digit10>\n", imei->digit10);
    printf("    <digit11>%u</digit11>\n", imei->digit11);
    printf("    <digit12>%u</digit12>\n", imei->digit12);
    printf("    <digit13>%u</digit13>\n", imei->digit13);
    printf("    <digit14>%u</digit14>\n", imei->digit14);
    printf("    <digit15>%u</digit15>\n", imei->digit15);
  } else {
    printf("    Wrong type of EPS mobile identity (%u)\n", epsmobileidentity->guti.typeofidentity);
  }

  printf("</Eps Mobile Identity>\n");
}

static int decode_guti_eps_mobile_identity(GutiEpsMobileIdentity_t *guti, uint8_t *buffer)
{
  int decoded = 0;
  guti->spare = (*(buffer + decoded) >> 4) & 0xf;

  /*
   * For the GUTI, bits 5 to 8 of octet 3 are coded as "1111"
   */
  if (guti->spare != 0xf) {
    return (TLV_ENCODE_VALUE_DOESNT_MATCH);
  }

  guti->oddeven = (*(buffer + decoded) >> 3) & 0x1;
  guti->typeofidentity = *(buffer + decoded) & 0x7;

  if (guti->typeofidentity != EPS_MOBILE_IDENTITY_GUTI) {
    return (TLV_ENCODE_VALUE_DOESNT_MATCH);
  }

  decoded++;
  guti->mccdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  guti->mccdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  guti->mncdigit3 = (*(buffer + decoded) >> 4) & 0xf;
  guti->mccdigit3 = *(buffer + decoded) & 0xf;
  decoded++;
  guti->mncdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  guti->mncdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  //IES_DECODE_U16(guti->mmegroupid, *(buffer + decoded));
  IES_DECODE_U16(buffer, decoded, guti->mmegroupid);
  guti->mmecode = *(buffer + decoded);
  decoded++;
  //IES_DECODE_U32(guti->mtmsi, *(buffer + decoded));
  IES_DECODE_U32(buffer, decoded, guti->mtmsi);
  return decoded;
}

static int decode_imsi_eps_mobile_identity(ImsiEpsMobileIdentity_t *imsi, uint8_t *buffer)
{
  int decoded = 0;
  imsi->typeofidentity = *(buffer + decoded) & 0x7;

  if (imsi->typeofidentity != EPS_MOBILE_IDENTITY_IMSI) {
    return (TLV_ENCODE_VALUE_DOESNT_MATCH);
  }

  imsi->oddeven = (*(buffer + decoded) >> 3) & 0x1;
  imsi->digit1 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imsi->digit2 = *(buffer + decoded) & 0xf;
  imsi->digit3 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imsi->digit4 = *(buffer + decoded) & 0xf;
  imsi->digit5 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imsi->digit6 = *(buffer + decoded) & 0xf;
  imsi->digit7 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imsi->digit8 = *(buffer + decoded) & 0xf;
  imsi->digit9 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imsi->digit10 = *(buffer + decoded) & 0xf;
  imsi->digit11 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imsi->digit12 = *(buffer + decoded) & 0xf;
  imsi->digit13 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imsi->digit14 = *(buffer + decoded) & 0xf;
  imsi->digit15 = (*(buffer + decoded) >> 4) & 0xf;

  /*
   * IMSI is coded using BCD coding. If the number of identity digits is
   * even then bits 5 to 8 of the last octet shall be filled with an end
   * mark coded as "1111".
   */
  if ((imsi->oddeven == EPS_MOBILE_IDENTITY_EVEN) && (imsi->digit15 != 0x0f)) {
    return (TLV_ENCODE_VALUE_DOESNT_MATCH);
  }

  decoded++;
  return decoded;
}

static int decode_imei_eps_mobile_identity(ImeiEpsMobileIdentity_t *imei, uint8_t *buffer)
{
  int decoded = 0;
  imei->typeofidentity = *(buffer + decoded) & 0x7;

  if (imei->typeofidentity != EPS_MOBILE_IDENTITY_IMEI) {
    return (TLV_ENCODE_VALUE_DOESNT_MATCH);
  }

  imei->oddeven = (*(buffer + decoded) >> 3) & 0x1;
  imei->digit1 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imei->digit2 = *(buffer + decoded) & 0xf;
  imei->digit3 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imei->digit4 = *(buffer + decoded) & 0xf;
  imei->digit5 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imei->digit6 = *(buffer + decoded) & 0xf;
  imei->digit7 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imei->digit8 = *(buffer + decoded) & 0xf;
  imei->digit9 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imei->digit10 = *(buffer + decoded) & 0xf;
  imei->digit11 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imei->digit12 = *(buffer + decoded) & 0xf;
  imei->digit13 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  imei->digit14 = *(buffer + decoded) & 0xf;
  imei->digit15 = (*(buffer + decoded) >> 4) & 0xf;
  decoded++;
  return decoded;
}

static int encode_guti_eps_mobile_identity(GutiEpsMobileIdentity_t *guti, uint8_t *buffer)
{
  uint32_t encoded = 0;
  *(buffer + encoded) = 0xf0 | ((guti->oddeven & 0x1) << 3) |
                        (guti->typeofidentity & 0x7);
  encoded++;
  *(buffer + encoded) = 0x00 | ((guti->mccdigit2 & 0xf) << 4) |
                        (guti->mccdigit1 & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((guti->mncdigit3 & 0xf) << 4) |
                        (guti->mccdigit3 & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((guti->mncdigit2 & 0xf) << 4) |
                        (guti->mncdigit1 & 0xf);
  encoded++;
  IES_ENCODE_U16(buffer, encoded, guti->mmegroupid);
  *(buffer + encoded) = guti->mmecode;
  encoded++;
  IES_ENCODE_U32(buffer, encoded, guti->mtmsi);
  return encoded;
}

static int encode_imsi_eps_mobile_identity(ImsiEpsMobileIdentity_t *imsi, uint8_t *buffer)
{
  uint32_t encoded = 0;
  *(buffer + encoded) = 0x00 | (imsi->digit1 << 4) | (imsi->oddeven << 3) |
                        (imsi->typeofidentity);
  encoded++;
  *(buffer + encoded) = 0x00 | (imsi->digit3 << 4) | imsi->digit2;
  encoded++;
  *(buffer + encoded) = 0x00 | (imsi->digit5 << 4) | imsi->digit4;
  encoded++;
  *(buffer + encoded) = 0x00 | (imsi->digit7 << 4) | imsi->digit6;
  encoded++;
  *(buffer + encoded) = 0x00 | (imsi->digit9 << 4) | imsi->digit8;
  encoded++;
  *(buffer + encoded) = 0x00 | (imsi->digit11 << 4) | imsi->digit10;
  encoded++;
  *(buffer + encoded) = 0x00 | (imsi->digit13 << 4) | imsi->digit12;
  encoded++;

  if (imsi->oddeven != EPS_MOBILE_IDENTITY_EVEN) {
    *(buffer + encoded) = 0x00 | (imsi->digit15 << 4) | imsi->digit14;
  } else {
    *(buffer + encoded) = 0xf0 | imsi->digit14;
  }

  encoded++;
  return encoded;
}

static int encode_imei_eps_mobile_identity(ImeiEpsMobileIdentity_t *imei, uint8_t *buffer)
{
  uint32_t encoded = 0;
  *(buffer + encoded) = 0x00 | (imei->digit1 << 4) | (imei->oddeven << 3) |
                        (imei->typeofidentity);
  encoded++;
  *(buffer + encoded) = 0x00 | (imei->digit3 << 4) | imei->digit2;
  encoded++;
  *(buffer + encoded) = 0x00 | (imei->digit5 << 4) | imei->digit4;
  encoded++;
  *(buffer + encoded) = 0x00 | (imei->digit7 << 4) | imei->digit6;
  encoded++;
  *(buffer + encoded) = 0x00 | (imei->digit9 << 4) | imei->digit8;
  encoded++;
  *(buffer + encoded) = 0x00 | (imei->digit11 << 4) | imei->digit10;
  encoded++;
  *(buffer + encoded) = 0x00 | (imei->digit13 << 4) | imei->digit12;
  encoded++;
  *(buffer + encoded) = 0x00 | (imei->digit15 << 4) | imei->digit14;
  encoded++;
  return encoded;
}

