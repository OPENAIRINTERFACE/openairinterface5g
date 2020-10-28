/*! \file FGSRegistrationResult.h

\brief 5GS Registration result for registration request procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"



#ifndef FGS_REGISTRATION_RESULT_H_
#define FGS_REGISTRATION_RESULT_H_
#define FGS_REGISTRATION_RESULT_3GPP                      0b001
#define FGS_REGISTRATION_RESULT_NON_3GPP                  0b010
#define FGS_REGISTRATION_RESULT_3GPP_AND_NON_3GPP         0b011

typedef struct {
  uint8_t  iei;
  uint8_t  resultlength;
  uint8_t  spare:4;
  uint8_t  smsallowed:1;
  uint8_t  registrationresult:3;
} FGSRegistrationResult;


uint16_t encode_fgs_registration_result(FGSRegistrationResult *fgsregistrationresult);

int decode_fgs_registration_result(FGSRegistrationResult *fgsregistrationresult, uint8_t iei, uint16_t value, uint32_t len);

#endif /* FGS REGISTRATION RESULT_H_*/

