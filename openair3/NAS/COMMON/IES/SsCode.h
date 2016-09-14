#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef SS_CODE_H_
#define SS_CODE_H_

#define SS_CODE_MINIMUM_LENGTH 2
#define SS_CODE_MAXIMUM_LENGTH 2

typedef uint8_t SsCode;

int encode_ss_code(SsCode *sscode, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_ss_code_xml(SsCode *sscode, uint8_t iei);

int decode_ss_code(SsCode *sscode, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* SS CODE_H_ */

