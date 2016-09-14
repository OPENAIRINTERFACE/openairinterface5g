#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef LOCATION_AREA_IDENTIFICATION_H_
#define LOCATION_AREA_IDENTIFICATION_H_

#define LOCATION_AREA_IDENTIFICATION_MINIMUM_LENGTH 6
#define LOCATION_AREA_IDENTIFICATION_MAXIMUM_LENGTH 6

typedef struct LocationAreaIdentification_tag {
  uint8_t  mccdigit2:4;
  uint8_t  mccdigit1:4;
  uint8_t  mncdigit3:4;
  uint8_t  mccdigit3:4;
  uint8_t  mncdigit2:4;
  uint8_t  mncdigit1:4;
  uint16_t lac;
} LocationAreaIdentification;

int encode_location_area_identification(LocationAreaIdentification *locationareaidentification, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_location_area_identification_xml(LocationAreaIdentification *locationareaidentification, uint8_t iei);

int decode_location_area_identification(LocationAreaIdentification *locationareaidentification, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* LOCATION AREA IDENTIFICATION_H_ */

