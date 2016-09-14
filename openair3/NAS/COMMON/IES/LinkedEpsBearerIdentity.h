#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef LINKED_EPS_BEARER_IDENTITY_H_
#define LINKED_EPS_BEARER_IDENTITY_H_

#define LINKED_EPS_BEARER_IDENTITY_MINIMUM_LENGTH 1
#define LINKED_EPS_BEARER_IDENTITY_MAXIMUM_LENGTH 1

typedef uint8_t LinkedEpsBearerIdentity;

int encode_linked_eps_bearer_identity(LinkedEpsBearerIdentity *linkedepsbeareridentity, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_linked_eps_bearer_identity_xml(LinkedEpsBearerIdentity *linkedepsbeareridentity, uint8_t iei);

uint8_t encode_u8_linked_eps_bearer_identity(LinkedEpsBearerIdentity *linkedepsbeareridentity);

int decode_linked_eps_bearer_identity(LinkedEpsBearerIdentity *linkedepsbeareridentity, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_linked_eps_bearer_identity(LinkedEpsBearerIdentity *linkedepsbeareridentity, uint8_t iei, uint8_t value, uint32_t len);

#endif /* LINKED EPS BEARER IDENTITY_H_ */

