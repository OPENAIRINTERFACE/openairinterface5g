#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef TIME_ZONE_H_
#define TIME_ZONE_H_

#define TIME_ZONE_MINIMUM_LENGTH 2
#define TIME_ZONE_MAXIMUM_LENGTH 2

typedef uint8_t TimeZone;

int encode_time_zone(TimeZone *timezone, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_time_zone_xml(TimeZone *timezone, uint8_t iei);

int decode_time_zone(TimeZone *timezone, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* TIME ZONE_H_ */

