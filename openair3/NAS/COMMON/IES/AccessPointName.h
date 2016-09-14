#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef ACCESS_POINT_NAME_H_
#define ACCESS_POINT_NAME_H_

#define ACCESS_POINT_NAME_MINIMUM_LENGTH 3
#define ACCESS_POINT_NAME_MAXIMUM_LENGTH 102

typedef struct AccessPointName_tag {
  OctetString accesspointnamevalue;
} AccessPointName;

int encode_access_point_name(AccessPointName *accesspointname, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_access_point_name(AccessPointName *accesspointname, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_access_point_name_xml(AccessPointName *accesspointname, uint8_t iei);

#endif /* ACCESS POINT NAME_H_ */

