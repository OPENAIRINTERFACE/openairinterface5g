#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef IMEISV_REQUEST_H_
#define IMEISV_REQUEST_H_

#define IMEISV_REQUEST_MINIMUM_LENGTH 1
#define IMEISV_REQUEST_MAXIMUM_LENGTH 1

typedef uint8_t ImeisvRequest;

int encode_imeisv_request(ImeisvRequest *imeisvrequest, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_imeisv_request_xml(ImeisvRequest *imeisvrequest, uint8_t iei);

uint8_t encode_u8_imeisv_request(ImeisvRequest *imeisvrequest);

int decode_imeisv_request(ImeisvRequest *imeisvrequest, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_imeisv_request(ImeisvRequest *imeisvrequest, uint8_t iei, uint8_t value, uint32_t len);

#endif /* IMEISV REQUEST_H_ */

