#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef LCS_CLIENT_IDENTITY_H_
#define LCS_CLIENT_IDENTITY_H_

#define LCS_CLIENT_IDENTITY_MINIMUM_LENGTH 3
#define LCS_CLIENT_IDENTITY_MAXIMUM_LENGTH 257

typedef struct LcsClientIdentity_tag {
  OctetString lcsclientidentityvalue;
} LcsClientIdentity;

int encode_lcs_client_identity(LcsClientIdentity *lcsclientidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_lcs_client_identity(LcsClientIdentity *lcsclientidentity, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_lcs_client_identity_xml(LcsClientIdentity *lcsclientidentity, uint8_t iei);

#endif /* LCS CLIENT IDENTITY_H_ */

