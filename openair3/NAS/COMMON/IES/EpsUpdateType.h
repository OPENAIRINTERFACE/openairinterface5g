#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EPS_UPDATE_TYPE_H_
#define EPS_UPDATE_TYPE_H_

#define EPS_UPDATE_TYPE_MINIMUM_LENGTH 1
#define EPS_UPDATE_TYPE_MAXIMUM_LENGTH 1

typedef struct EpsUpdateType_tag {
  uint8_t  activeflag:1;
  uint8_t  epsupdatetypevalue:3;
} EpsUpdateType;

int encode_eps_update_type(EpsUpdateType *epsupdatetype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_update_type_xml(EpsUpdateType *epsupdatetype, uint8_t iei);

uint8_t encode_u8_eps_update_type(EpsUpdateType *epsupdatetype);

int decode_eps_update_type(EpsUpdateType *epsupdatetype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_eps_update_type(EpsUpdateType *epsupdatetype, uint8_t iei, uint8_t value, uint32_t len);

#endif /* EPS UPDATE TYPE_H_ */

