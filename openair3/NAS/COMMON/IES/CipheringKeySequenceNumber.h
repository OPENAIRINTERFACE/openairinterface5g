#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef CIPHERING_KEY_SEQUENCE_NUMBER_H_
#define CIPHERING_KEY_SEQUENCE_NUMBER_H_

#define CIPHERING_KEY_SEQUENCE_NUMBER_MINIMUM_LENGTH 1
#define CIPHERING_KEY_SEQUENCE_NUMBER_MAXIMUM_LENGTH 1

typedef uint8_t CipheringKeySequenceNumber;

int encode_ciphering_key_sequence_number(CipheringKeySequenceNumber *cipheringkeysequencenumber, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_ciphering_key_sequence_number_xml(CipheringKeySequenceNumber *cipheringkeysequencenumber, uint8_t iei);

uint8_t encode_u8_ciphering_key_sequence_number(CipheringKeySequenceNumber *cipheringkeysequencenumber);

int decode_ciphering_key_sequence_number(CipheringKeySequenceNumber *cipheringkeysequencenumber, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_ciphering_key_sequence_number(CipheringKeySequenceNumber *cipheringkeysequencenumber, uint8_t iei, uint8_t value, uint32_t len);

#endif /* CIPHERING KEY SEQUENCE NUMBER_H_ */

