#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef ADDITIONAL_UPDATE_TYPE_H_
#define ADDITIONAL_UPDATE_TYPE_H_

#define ADDITIONAL_UPDATE_TYPE_MINIMUM_LENGTH 1
#define ADDITIONAL_UPDATE_TYPE_MAXIMUM_LENGTH 1

typedef uint8_t AdditionalUpdateType;

int encode_additional_update_type(AdditionalUpdateType *additionalupdatetype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_additional_update_type(AdditionalUpdateType *additionalupdatetype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_additional_update_type_xml(AdditionalUpdateType *additionalupdatetype, uint8_t iei);

#endif /* ADDITIONAL UPDATE TYPE_H_ */

