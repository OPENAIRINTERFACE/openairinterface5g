#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef TIME_ZONE_AND_TIME_H_
#define TIME_ZONE_AND_TIME_H_

#define TIME_ZONE_AND_TIME_MINIMUM_LENGTH 8
#define TIME_ZONE_AND_TIME_MAXIMUM_LENGTH 8

typedef struct TimeZoneAndTime_tag {
  uint8_t  year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
  uint8_t  timezone;
} TimeZoneAndTime;

int encode_time_zone_and_time(TimeZoneAndTime *timezoneandtime, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_time_zone_and_time_xml(TimeZoneAndTime *timezoneandtime, uint8_t iei);

int decode_time_zone_and_time(TimeZoneAndTime *timezoneandtime, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* TIME ZONE AND TIME_H_ */

