/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
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

