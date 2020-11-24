/*! \file FGSRegistrationResult.c

\brief 5GS Registration result for registration request procedures
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
#include "FGSRegistrationResult.h"


int decode_fgs_registration_result(FGSRegistrationResult *fgsregistrationresult, uint8_t iei, uint16_t value, uint32_t len)
{
  int decoded = 0;
  uint16_t *buffer = &value;
  fgsregistrationresult->registrationresult = *buffer & 0x7;
  fgsregistrationresult->smsallowed = *buffer & 0x8;
  decoded = decoded+2;
  return decoded;
}

uint16_t encode_fgs_registration_result(FGSRegistrationResult *fgsregistrationresult)
{
  uint16_t bufferReturn;
  uint16_t *buffer = &bufferReturn;
  uint8_t encoded = 0;
  *(buffer + encoded) = 0x00 | (fgsregistrationresult->smsallowed & 0x8) |
                        (fgsregistrationresult->registrationresult & 0x7);
  encoded= encoded+2;

  return bufferReturn;
}

