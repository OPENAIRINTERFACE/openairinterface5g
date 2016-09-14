#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef PACKET_FLOW_IDENTIFIER_H_
#define PACKET_FLOW_IDENTIFIER_H_

#define PACKET_FLOW_IDENTIFIER_MINIMUM_LENGTH 3
#define PACKET_FLOW_IDENTIFIER_MAXIMUM_LENGTH 3

typedef uint8_t PacketFlowIdentifier;

int encode_packet_flow_identifier(PacketFlowIdentifier *packetflowidentifier, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_packet_flow_identifier(PacketFlowIdentifier *packetflowidentifier, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_packet_flow_identifier_xml(PacketFlowIdentifier *packetflowidentifier, uint8_t iei);

#endif /* PACKET FLOW IDENTIFIER_H_ */

