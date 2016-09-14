#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef IDENTITY_TYPE_2_H_
#define IDENTITY_TYPE_2_H_

#define IDENTITY_TYPE_2_MINIMUM_LENGTH 1
#define IDENTITY_TYPE_2_MAXIMUM_LENGTH 1

#define IDENTITY_TYPE_2_IMSI  0b001
#define IDENTITY_TYPE_2_IMEI  0b010
#define IDENTITY_TYPE_2_IMEISV  0b011
#define IDENTITY_TYPE_2_TMSI  0b100
typedef uint8_t IdentityType2;

int encode_identity_type_2(IdentityType2 *identitytype2, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_identity_type_2_xml(IdentityType2 *identitytype2, uint8_t iei);

uint8_t encode_u8_identity_type_2(IdentityType2 *identitytype2);

int decode_identity_type_2(IdentityType2 *identitytype2, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_identity_type_2(IdentityType2 *identitytype2, uint8_t iei, uint8_t value, uint32_t len);

#endif /* IDENTITY TYPE 2_H_ */

