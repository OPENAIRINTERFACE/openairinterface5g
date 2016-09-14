#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef SERVICE_TYPE_H_
#define SERVICE_TYPE_H_

#define SERVICE_TYPE_MINIMUM_LENGTH 1
#define SERVICE_TYPE_MAXIMUM_LENGTH 1

typedef uint8_t ServiceType;

int encode_service_type(ServiceType *servicetype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_service_type_xml(ServiceType *servicetype, uint8_t iei);

uint8_t encode_u8_service_type(ServiceType *servicetype);

int decode_service_type(ServiceType *servicetype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_service_type(ServiceType *servicetype, uint8_t iei, uint8_t value, uint32_t len);

#endif /* SERVICE TYPE_H_ */

