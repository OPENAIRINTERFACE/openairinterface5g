#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef MOBILE_STATION_CLASSMARK_3_H_
#define MOBILE_STATION_CLASSMARK_3_H_

#define MOBILE_STATION_CLASSMARK_3_MINIMUM_LENGTH 1
#define MOBILE_STATION_CLASSMARK_3_MAXIMUM_LENGTH 1

typedef struct {
  uint8_t field;
} MobileStationClassmark3;

int encode_mobile_station_classmark_3(MobileStationClassmark3 *mobilestationclassmark3, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_mobile_station_classmark_3_xml(MobileStationClassmark3 *mobilestationclassmark3, uint8_t iei);

int decode_mobile_station_classmark_3(MobileStationClassmark3 *mobilestationclassmark3, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* MOBILE STATION CLASSMARK 3_H_ */

