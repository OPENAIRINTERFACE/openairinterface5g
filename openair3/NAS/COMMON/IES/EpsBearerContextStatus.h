#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EPS_BEARER_CONTEXT_STATUS_H_
#define EPS_BEARER_CONTEXT_STATUS_H_

#define EPS_BEARER_CONTEXT_STATUS_MINIMUM_LENGTH 4
#define EPS_BEARER_CONTEXT_STATUS_MAXIMUM_LENGTH 4

typedef uint16_t EpsBearerContextStatus;

int encode_eps_bearer_context_status(EpsBearerContextStatus *epsbearercontextstatus, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_eps_bearer_context_status(EpsBearerContextStatus *epsbearercontextstatus, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_bearer_context_status_xml(EpsBearerContextStatus *epsbearercontextstatus, uint8_t iei);

#endif /* EPS BEARER CONTEXT STATUS_H_ */

