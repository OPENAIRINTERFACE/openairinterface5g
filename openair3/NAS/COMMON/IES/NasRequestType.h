#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef NAS_REQUEST_TYPE_H_
#define NAS_REQUEST_TYPE_H_

#define REQUEST_TYPE_MINIMUM_LENGTH 1
#define REQUEST_TYPE_MAXIMUM_LENGTH 1

#define REQUEST_TYPE_INITIAL_REQUEST  0b001
#define REQUEST_TYPE_HANDOVER   0b010
#define REQUEST_TYPE_UNUSED   0b011
#define REQUEST_TYPE_EMERGENCY    0b100
typedef uint8_t RequestType;

int encode_request_type(RequestType *requesttype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_request_type_xml(RequestType *requesttype, uint8_t iei);

uint8_t encode_u8_request_type(RequestType *requesttype);

int decode_request_type(RequestType *requesttype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_request_type(RequestType *requesttype, uint8_t iei, uint8_t value, uint32_t len);

#endif /* NAS_REQUEST_TYPE_H_ */

