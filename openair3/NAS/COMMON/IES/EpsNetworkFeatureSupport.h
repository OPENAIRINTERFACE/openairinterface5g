#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EPS_NETWORK_FEATURE_SUPPORT_H_
#define EPS_NETWORK_FEATURE_SUPPORT_H_

#define EPS_NETWORK_FEATURE_SUPPORT_MINIMUM_LENGTH 3
#define EPS_NETWORK_FEATURE_SUPPORT_MAXIMUM_LENGTH 3

typedef uint8_t EpsNetworkFeatureSupport;

int encode_eps_network_feature_support(EpsNetworkFeatureSupport *epsnetworkfeaturesupport, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_eps_network_feature_support(EpsNetworkFeatureSupport *epsnetworkfeaturesupport, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_network_feature_support_xml(EpsNetworkFeatureSupport *epsnetworkfeaturesupport, uint8_t iei);

#endif /* EPS NETWORK FEATURE SUPPORT_H_ */

