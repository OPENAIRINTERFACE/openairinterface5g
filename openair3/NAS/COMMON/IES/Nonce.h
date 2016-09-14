#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef NONCE_H_
#define NONCE_H_

#define NONCE_MINIMUM_LENGTH 5
#define NONCE_MAXIMUM_LENGTH 5

typedef uint32_t Nonce;

int encode_nonce(Nonce *nonce, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_nonce_xml(Nonce *nonce, uint8_t iei);

int decode_nonce(Nonce *nonce, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* NONCE_H_ */

