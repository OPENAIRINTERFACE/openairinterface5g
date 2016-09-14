#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef LCS_INDICATOR_H_
#define LCS_INDICATOR_H_

#define LCS_INDICATOR_MINIMUM_LENGTH 2
#define LCS_INDICATOR_MAXIMUM_LENGTH 2

typedef uint8_t LcsIndicator;

int encode_lcs_indicator(LcsIndicator *lcsindicator, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_lcs_indicator_xml(LcsIndicator *lcsindicator, uint8_t iei);

int decode_lcs_indicator(LcsIndicator *lcsindicator, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* LCS INDICATOR_H_ */

