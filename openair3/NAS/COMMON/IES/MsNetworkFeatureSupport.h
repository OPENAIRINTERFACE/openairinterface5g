#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef MS_NETWORK_FEATURE_SUPPORT_H_
#define MS_NETWORK_FEATURE_SUPPORT_H_

#define MS_NETWORK_FEATURE_SUPPORT_MINIMUM_LENGTH 3
#define MS_NETWORK_FEATURE_SUPPORT_MAXIMUM_LENGTH 10

typedef struct MsNetworkFeatureSupport_tag {
  uint8_t spare_bits:3;
  uint8_t extended_periodic_timers:1;
} MsNetworkFeatureSupport;

int encode_ms_network_feature_support(MsNetworkFeatureSupport *msnetworkfeaturesupport, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_ms_network_feature_support(MsNetworkFeatureSupport *msnetworkfeaturesupport, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_ms_network_feature_support_xml(MsNetworkFeatureSupport *msnetworkfeaturesupport, uint8_t iei);

#endif /* MS NETWORK CAPABILITY_H_ */

