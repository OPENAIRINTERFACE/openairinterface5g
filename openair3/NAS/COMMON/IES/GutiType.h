#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef GUTI_TYPE_H_
#define GUTI_TYPE_H_

#define GUTI_TYPE_MINIMUM_LENGTH 1
#define GUTI_TYPE_MAXIMUM_LENGTH 1

#define GUTI_NATIVE 0
#define GUTI_MAPPED 1
typedef uint8_t GutiType;

int encode_guti_type(GutiType *gutitype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_guti_type_xml(GutiType *gutitype, uint8_t iei);

uint8_t encode_u8_guti_type(GutiType *gutitype);

int decode_guti_type(GutiType *gutitype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_guti_type(GutiType *gutitype, uint8_t iei, uint8_t value, uint32_t len);

#endif /* GUTI_TYPE_H_ */

