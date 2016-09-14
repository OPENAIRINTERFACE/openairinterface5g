#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef AUTHENTICATION_PARAMETER_RAND_H_
#define AUTHENTICATION_PARAMETER_RAND_H_

#define AUTHENTICATION_PARAMETER_RAND_MINIMUM_LENGTH 16
#define AUTHENTICATION_PARAMETER_RAND_MAXIMUM_LENGTH 16

typedef struct AuthenticationParameterRand_tag {
  OctetString rand;
} AuthenticationParameterRand;

int encode_authentication_parameter_rand(AuthenticationParameterRand *authenticationparameterrand, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_authentication_parameter_rand_xml(AuthenticationParameterRand *authenticationparameterrand, uint8_t iei);

int decode_authentication_parameter_rand(AuthenticationParameterRand *authenticationparameterrand, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* AUTHENTICATION PARAMETER RAND_H_ */

