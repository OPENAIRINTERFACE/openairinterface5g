#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EPS_BEARER_IDENTITY_H_
#define EPS_BEARER_IDENTITY_H_

#define EPS_BEARER_IDENTITY_MINIMUM_LENGTH 1
#define EPS_BEARER_IDENTITY_MAXIMUM_LENGTH 1

#define EPS_BEARER_IDENTITY_UNASSIGNED  0
#define EPS_BEARER_IDENTITY_FIRST 5
#define EPS_BEARER_IDENTITY_LAST  15
typedef uint8_t EpsBearerIdentity;

int encode_eps_bearer_identity(EpsBearerIdentity *epsbeareridentity, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_bearer_identity_xml(EpsBearerIdentity *epsbeareridentity, uint8_t iei);

int decode_eps_bearer_identity(EpsBearerIdentity *epsbeareridentity, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* EPS BEARER IDENTITY_H_ */

