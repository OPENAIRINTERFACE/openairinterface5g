#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef PDN_ADDRESS_H_
#define PDN_ADDRESS_H_

#define PDN_ADDRESS_MINIMUM_LENGTH 7
#define PDN_ADDRESS_MAXIMUM_LENGTH 15

typedef struct PdnAddress_tag {
#define PDN_VALUE_TYPE_IPV4 0b001
#define PDN_VALUE_TYPE_IPV6 0b010
#define PDN_VALUE_TYPE_IPV4V6 0b011
  uint8_t  pdntypevalue:3;
  OctetString pdnaddressinformation;
} PdnAddress;

int encode_pdn_address(PdnAddress *pdnaddress, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_pdn_address(PdnAddress *pdnaddress, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_pdn_address_xml(PdnAddress *pdnaddress, uint8_t iei);

#endif /* PDN ADDRESS_H_ */

