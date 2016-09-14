#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EPS_UPDATE_RESULT_H_
#define EPS_UPDATE_RESULT_H_

#define EPS_UPDATE_RESULT_MINIMUM_LENGTH 1
#define EPS_UPDATE_RESULT_MAXIMUM_LENGTH 1

typedef uint8_t EpsUpdateResult;

int encode_eps_update_result(EpsUpdateResult *epsupdateresult, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_update_result_xml(EpsUpdateResult *epsupdateresult, uint8_t iei);

uint8_t encode_u8_eps_update_result(EpsUpdateResult *epsupdateresult);

int decode_eps_update_result(EpsUpdateResult *epsupdateresult, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_eps_update_result(EpsUpdateResult *epsupdateresult, uint8_t iei, uint8_t value, uint32_t len);

#endif /* EPS UPDATE RESULT_H_ */

