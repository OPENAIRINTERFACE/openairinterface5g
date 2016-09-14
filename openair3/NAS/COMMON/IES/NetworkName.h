#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef NETWORK_NAME_H_
#define NETWORK_NAME_H_

#define NETWORK_NAME_MINIMUM_LENGTH 3
#define NETWORK_NAME_MAXIMUM_LENGTH 255

typedef struct NetworkName_tag {
  uint8_t  codingscheme:3;
  uint8_t  addci:1;
  uint8_t  numberofsparebitsinlastoctet:3;
  OctetString textstring;
} NetworkName;

int encode_network_name(NetworkName *networkname, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_network_name(NetworkName *networkname, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_network_name_xml(NetworkName *networkname, uint8_t iei);

#endif /* NETWORK NAME_H_ */

