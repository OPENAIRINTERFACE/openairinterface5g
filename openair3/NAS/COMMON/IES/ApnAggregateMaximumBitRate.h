#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef APN_AGGREGATE_MAXIMUM_BIT_RATE_H_
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_H_

#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MINIMUM_LENGTH 4
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_LENGTH 8

#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION_PRESENT  (1<<0)
#define APN_AGGREGATE_MAXIMUM_BIT_RATE_MAXIMUM_EXTENSION2_PRESENT (1<<1)

typedef struct ApnAggregateMaximumBitRate_tag {
  uint8_t  apnambrfordownlink;
  uint8_t  apnambrforuplink;
  uint8_t  apnambrfordownlink_extended;
  uint8_t  apnambrforuplink_extended;
  uint8_t  apnambrfordownlink_extended2;
  uint8_t  apnambrforuplink_extended2;
  uint8_t  extensions;
} ApnAggregateMaximumBitRate;

int encode_apn_aggregate_maximum_bit_rate(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_apn_aggregate_maximum_bit_rate(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_apn_aggregate_maximum_bit_rate_xml(ApnAggregateMaximumBitRate *apnaggregatemaximumbitrate, uint8_t iei);

#endif /* APN AGGREGATE MAXIMUM BIT RATE_H_ */

