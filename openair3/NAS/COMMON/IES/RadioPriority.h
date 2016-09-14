#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef RADIO_PRIORITY_H_
#define RADIO_PRIORITY_H_

#define RADIO_PRIORITY_MINIMUM_LENGTH 1
#define RADIO_PRIORITY_MAXIMUM_LENGTH 1

typedef uint8_t RadioPriority;

int encode_radio_priority(RadioPriority *radiopriority, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_radio_priority_xml(RadioPriority *radiopriority, uint8_t iei);

uint8_t encode_u8_radio_priority(RadioPriority *radiopriority);

int decode_radio_priority(RadioPriority *radiopriority, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_radio_priority(RadioPriority *radiopriority, uint8_t iei, uint8_t value, uint32_t len);

#endif /* RADIO PRIORITY_H_ */

