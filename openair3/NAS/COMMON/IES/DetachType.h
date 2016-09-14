#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef DETACH_TYPE_H_
#define DETACH_TYPE_H_

#define DETACH_TYPE_MINIMUM_LENGTH 1
#define DETACH_TYPE_MAXIMUM_LENGTH 1

typedef struct DetachType_tag {
#define DETACH_TYPE_NORMAL_DETACH 0
#define DETACH_TYPE_SWITCH_OFF    1
  uint8_t  switchoff:1;
#define DETACH_TYPE_EPS     0b001
#define DETACH_TYPE_IMSI    0b010
#define DETACH_TYPE_EPS_IMSI    0b011
#define DETACH_TYPE_RESERVED_1    0b110
#define DETACH_TYPE_RESERVED_2    0b111
  uint8_t  typeofdetach:3;
} DetachType;

int encode_detach_type(DetachType *detachtype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_detach_type_xml(DetachType *detachtype, uint8_t iei);

uint8_t encode_u8_detach_type(DetachType *detachtype);

int decode_detach_type(DetachType *detachtype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_detach_type(DetachType *detachtype, uint8_t iei, uint8_t value, uint32_t len);

#endif /* DETACH TYPE_H_ */

