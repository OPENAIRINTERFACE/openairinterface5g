#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef SHORT_MAC_H_
#define SHORT_MAC_H_

#define SHORT_MAC_MINIMUM_LENGTH 3
#define SHORT_MAC_MAXIMUM_LENGTH 3

typedef uint16_t ShortMac;

int encode_short_mac(ShortMac *shortmac, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_short_mac_xml(ShortMac *shortmac, uint8_t iei);

int decode_short_mac(ShortMac *shortmac, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* SHORT MAC_H_ */

