/*! \file FGSRegistrationType.h

\brief 5GS Registration Type for registration request procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef FGS_REGISTRATION_TYPE_H_
#define FGS_REGISTRATION_TYPE_H_

#define FGS_REGISTRATION_TYPE_MINIMUM_LENGTH 1
#define FGS_REGISTRATION_TYPE_MAXIMUM_LENGTH 1

typedef uint8_t FGSRegistrationType;


#define INITIAL_REGISTRATION               0b001
#define MOBILITY_REGISTRATION_UPDATING     0b010
#define PERIODIC_REGISTRATION_UPDATING     0b011
#define EMERGENCY_REGISTRATION             0b100

int encode_5gs_registration_type(FGSRegistrationType *fgsregistrationtype);

int decode_5gs_registration_type(FGSRegistrationType *fgsregistrationtype, uint8_t iei, uint8_t *buffer, uint32_t len);


#endif /* FGS_REGISTRATION_TYPE_H_*/

