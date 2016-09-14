#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef CSFB_RESPONSE_H_
#define CSFB_RESPONSE_H_

#define CSFB_RESPONSE_MINIMUM_LENGTH 1
#define CSFB_RESPONSE_MAXIMUM_LENGTH 1

typedef uint8_t CsfbResponse;

int encode_csfb_response(CsfbResponse *csfbresponse, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_csfb_response_xml(CsfbResponse *csfbresponse, uint8_t iei);

uint8_t encode_u8_csfb_response(CsfbResponse *csfbresponse);

int decode_csfb_response(CsfbResponse *csfbresponse, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_csfb_response(CsfbResponse *csfbresponse, uint8_t iei, uint8_t value, uint32_t len);

#endif /* CSFB RESPONSE_H_ */

