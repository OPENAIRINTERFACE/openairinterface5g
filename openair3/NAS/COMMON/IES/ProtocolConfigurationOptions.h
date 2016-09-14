#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef PROTOCOL_CONFIGURATION_OPTIONS_H_
#define PROTOCOL_CONFIGURATION_OPTIONS_H_

#define PROTOCOL_CONFIGURATION_OPTIONS_MINIMUM_LENGTH 3
#define PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH 253

// arbitrary value, theoricaly can be greater than defined (250/3)
#define PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID 16


/* 3GPP TS 24.008 Table 10.5.154
 * MS to network table
 */
typedef enum ProtocolConfigurationOptionsList_ids_tag {
  PCO_UNKNOWN                         = 0,
  PCO_P_CSCF_IPV6_ADDRESS_REQ         = 1,
  PCO_IM_CN_SUBSYSTEM_SIGNALING_FLAG  = 2,
  PCO_DNS_SERVER_IPV6_ADDRESS_REQ     = 3,
  PCO_NOT_SUPPORTED                   = 4,
  PCO_MS_SUPPORTED_OF_NETWORK_REQUESTED_BEARER_CONTROL_INDICATOR = 5,
  PCO_RESERVED                        = 6,
  /* TODO: complete me */
} ProtocolConfigurationOptionsList_ids;

/* 3GPP TS 24.008 Table 10.5.154
 * network to MS table
 * TODO
 */

typedef struct ProtocolConfigurationOptions_tag {
  uint8_t     configurationprotol:3;
  uint8_t     num_protocol_id_or_container_id;
  uint16_t    protocolid[PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID];
  uint8_t     lengthofprotocolid[PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID];
  OctetString protocolidcontents[PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID];
} ProtocolConfigurationOptions;

int encode_protocol_configuration_options(ProtocolConfigurationOptions *protocolconfigurationoptions, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_protocol_configuration_options(ProtocolConfigurationOptions *protocolconfigurationoptions, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_protocol_configuration_options_xml(ProtocolConfigurationOptions *protocolconfigurationoptions, uint8_t iei);

#endif /* PROTOCOL CONFIGURATION OPTIONS_H_ */

