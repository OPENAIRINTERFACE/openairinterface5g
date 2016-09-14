#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef LLC_SERVICE_ACCESS_POINT_IDENTIFIER_H_
#define LLC_SERVICE_ACCESS_POINT_IDENTIFIER_H_

#define LLC_SERVICE_ACCESS_POINT_IDENTIFIER_MINIMUM_LENGTH 2
#define LLC_SERVICE_ACCESS_POINT_IDENTIFIER_MAXIMUM_LENGTH 2

typedef uint8_t LlcServiceAccessPointIdentifier;

int encode_llc_service_access_point_identifier(LlcServiceAccessPointIdentifier *llcserviceaccesspointidentifier, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_llc_service_access_point_identifier_xml(LlcServiceAccessPointIdentifier *llcserviceaccesspointidentifier, uint8_t iei);

int decode_llc_service_access_point_identifier(LlcServiceAccessPointIdentifier *llcserviceaccesspointidentifier, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* LLC SERVICE ACCESS POINT IDENTIFIER_H_ */

