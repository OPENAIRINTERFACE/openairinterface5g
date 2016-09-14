#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef NAS_KEY_SET_IDENTIFIER_H_
#define NAS_KEY_SET_IDENTIFIER_H_

#define NAS_KEY_SET_IDENTIFIER_MINIMUM_LENGTH 1
#define NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH 1

typedef struct NasKeySetIdentifier_tag {
#define NAS_KEY_SET_IDENTIFIER_NATIVE 0
#define NAS_KEY_SET_IDENTIFIER_MAPPED 1
  uint8_t  tsc:1;
#define NAS_KEY_SET_IDENTIFIER_NOT_AVAILABLE 0b111
  uint8_t  naskeysetidentifier:3;
} NasKeySetIdentifier;

int encode_nas_key_set_identifier(NasKeySetIdentifier *naskeysetidentifier, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_nas_key_set_identifier_xml(NasKeySetIdentifier *naskeysetidentifier, uint8_t iei);

uint8_t encode_u8_nas_key_set_identifier(NasKeySetIdentifier *naskeysetidentifier);

int decode_nas_key_set_identifier(NasKeySetIdentifier *naskeysetidentifier, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_nas_key_set_identifier(NasKeySetIdentifier *naskeysetidentifier, uint8_t iei, uint8_t value, uint32_t len);

#endif /* NAS KEY SET IDENTIFIER_H_ */

