#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef PDN_TYPE_H_
#define PDN_TYPE_H_

#define PDN_TYPE_MINIMUM_LENGTH 1
#define PDN_TYPE_MAXIMUM_LENGTH 1

#define PDN_TYPE_IPV4   0b001
#define PDN_TYPE_IPV6   0b010
#define PDN_TYPE_IPV4V6   0b011
#define PDN_TYPE_UNUSED   0b100
typedef uint8_t PdnType;

int encode_pdn_type(PdnType *pdntype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_pdn_type_xml(PdnType *pdntype, uint8_t iei);

uint8_t encode_u8_pdn_type(PdnType *pdntype);

int decode_pdn_type(PdnType *pdntype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_pdn_type(PdnType *pdntype, uint8_t iei, uint8_t value, uint32_t len);

#endif /* PDN TYPE_H_ */

