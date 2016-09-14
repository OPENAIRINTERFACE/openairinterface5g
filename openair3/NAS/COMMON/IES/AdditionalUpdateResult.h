#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef ADDITIONAL_UPDATE_RESULT_H_
#define ADDITIONAL_UPDATE_RESULT_H_

#define ADDITIONAL_UPDATE_RESULT_MINIMUM_LENGTH 1
#define ADDITIONAL_UPDATE_RESULT_MAXIMUM_LENGTH 1

typedef uint8_t AdditionalUpdateResult;

int encode_additional_update_result(AdditionalUpdateResult *additionalupdateresult, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_additional_update_result_xml(AdditionalUpdateResult *additionalupdateresult, uint8_t iei);

uint8_t encode_u8_additional_update_result(AdditionalUpdateResult *additionalupdateresult);

int decode_additional_update_result(AdditionalUpdateResult *additionalupdateresult, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_additional_update_result(AdditionalUpdateResult *additionalupdateresult, uint8_t iei, uint8_t value, uint32_t len);

#endif /* ADDITIONAL UPDATE RESULT_H_ */

