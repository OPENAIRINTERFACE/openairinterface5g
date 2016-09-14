#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef AUTHENTICATION_PARAMETER_AUTN_H_
#define AUTHENTICATION_PARAMETER_AUTN_H_

#define AUTHENTICATION_PARAMETER_AUTN_MINIMUM_LENGTH 17
#define AUTHENTICATION_PARAMETER_AUTN_MAXIMUM_LENGTH 17

typedef struct AuthenticationParameterAutn_tag {
  OctetString autn;
} AuthenticationParameterAutn;

int encode_authentication_parameter_autn(AuthenticationParameterAutn *authenticationparameterautn, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_authentication_parameter_autn(AuthenticationParameterAutn *authenticationparameterautn, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_authentication_parameter_autn_xml(AuthenticationParameterAutn *authenticationparameterautn, uint8_t iei);

#endif /* AUTHENTICATION PARAMETER AUTN_H_ */

