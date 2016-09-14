#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef ESM_CAUSE_H_
#define ESM_CAUSE_H_

#define ESM_CAUSE_MINIMUM_LENGTH 1
#define ESM_CAUSE_MAXIMUM_LENGTH 1

typedef uint8_t EsmCause;

int encode_esm_cause(EsmCause *esmcause, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_esm_cause_xml(EsmCause *esmcause, uint8_t iei);

int decode_esm_cause(EsmCause *esmcause, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* ESM CAUSE_H_ */

