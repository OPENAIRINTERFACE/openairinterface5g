#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef DAYLIGHT_SAVING_TIME_H_
#define DAYLIGHT_SAVING_TIME_H_

#define DAYLIGHT_SAVING_TIME_MINIMUM_LENGTH 3
#define DAYLIGHT_SAVING_TIME_MAXIMUM_LENGTH 3

typedef uint8_t DaylightSavingTime;

int encode_daylight_saving_time(DaylightSavingTime *daylightsavingtime, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_daylight_saving_time(DaylightSavingTime *daylightsavingtime, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_daylight_saving_time_xml(DaylightSavingTime *daylightsavingtime, uint8_t iei);

#endif /* DAYLIGHT SAVING TIME_H_ */

