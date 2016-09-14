#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef AUTHENTICATION_FAILURE_PARAMETER_H_
#define AUTHENTICATION_FAILURE_PARAMETER_H_

#define AUTHENTICATION_FAILURE_PARAMETER_MINIMUM_LENGTH 16
#define AUTHENTICATION_FAILURE_PARAMETER_MAXIMUM_LENGTH 16

typedef struct AuthenticationFailureParameter_tag {
  OctetString auts;
} AuthenticationFailureParameter;

int encode_authentication_failure_parameter(AuthenticationFailureParameter *authenticationfailureparameter, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_authentication_failure_parameter(AuthenticationFailureParameter *authenticationfailureparameter, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_authentication_failure_parameter_xml(AuthenticationFailureParameter *authenticationfailureparameter, uint8_t iei);

#endif /* AUTHENTICATION FAILURE PARAMETER_H_ */

