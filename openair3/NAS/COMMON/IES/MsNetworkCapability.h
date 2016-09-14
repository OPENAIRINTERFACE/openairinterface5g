#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef MS_NETWORK_CAPABILITY_H_
#define MS_NETWORK_CAPABILITY_H_

#define MS_NETWORK_CAPABILITY_MINIMUM_LENGTH 3
#define MS_NETWORK_CAPABILITY_MAXIMUM_LENGTH 10

typedef struct MsNetworkCapability_tag {
  OctetString msnetworkcapabilityvalue;
} MsNetworkCapability;

int encode_ms_network_capability(MsNetworkCapability *msnetworkcapability, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_ms_network_capability(MsNetworkCapability *msnetworkcapability, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_ms_network_capability_xml(MsNetworkCapability *msnetworkcapability, uint8_t iei);

#endif /* MS NETWORK CAPABILITY_H_ */

