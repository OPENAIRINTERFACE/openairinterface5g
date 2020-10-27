/*! \file FGSRegistrationType.c

\brief 5GS Registration Type for registration request procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "FGSRegistrationType.h"

int decode_5gs_registration_type(FGSRegistrationType *fgsregistrationtype, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  *fgsregistrationtype = *buffer & 0x7;
  decoded++;

  return decoded;
}

int encode_5gs_registration_type(FGSRegistrationType *fgsregistrationtype)
{
  uint8_t bufferReturn;
  uint8_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  uint8_t iei = 0;
  *(buffer + encoded) = 0x00 | (iei & 0xf0) | 0x8|
                        (*fgsregistrationtype & 0x7);
  encoded++;

  return bufferReturn;
}

