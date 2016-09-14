#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef TRAFFIC_FLOW_AGGREGATE_DESCRIPTION_H_
#define TRAFFIC_FLOW_AGGREGATE_DESCRIPTION_H_

#define TRAFFIC_FLOW_AGGREGATE_DESCRIPTION_MINIMUM_LENGTH 1
#define TRAFFIC_FLOW_AGGREGATE_DESCRIPTION_MAXIMUM_LENGTH 1

typedef struct {
  uint8_t field;
} TrafficFlowAggregateDescription;

int encode_traffic_flow_aggregate_description(TrafficFlowAggregateDescription *trafficflowaggregatedescription, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_traffic_flow_aggregate_description_xml(TrafficFlowAggregateDescription *trafficflowaggregatedescription, uint8_t iei);

int decode_traffic_flow_aggregate_description(TrafficFlowAggregateDescription *trafficflowaggregatedescription, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* TRAFFIC FLOW AGGREGATE DESCRIPTION_H_ */

