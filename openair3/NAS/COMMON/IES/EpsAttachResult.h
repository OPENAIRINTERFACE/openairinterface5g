#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EPS_ATTACH_RESULT_H_
#define EPS_ATTACH_RESULT_H_

#define EPS_ATTACH_RESULT_MINIMUM_LENGTH 1
#define EPS_ATTACH_RESULT_MAXIMUM_LENGTH 1

#define EPS_ATTACH_RESULT_EPS   0b001
#define EPS_ATTACH_RESULT_EPS_IMSI  0b010
typedef uint8_t EpsAttachResult;

int encode_eps_attach_result(EpsAttachResult *epsattachresult, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_attach_result_xml(EpsAttachResult *epsattachresult, uint8_t iei);

uint8_t encode_u8_eps_attach_result(EpsAttachResult *epsattachresult);

int decode_eps_attach_result(EpsAttachResult *epsattachresult, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_eps_attach_result(EpsAttachResult *epsattachresult, uint8_t iei, uint8_t value, uint32_t len);

#endif /* EPS ATTACH RESULT_H_ */

