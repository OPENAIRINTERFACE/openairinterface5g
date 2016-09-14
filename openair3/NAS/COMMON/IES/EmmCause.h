#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EMM_CAUSE_H_
#define EMM_CAUSE_H_

#define EMM_CAUSE_MINIMUM_LENGTH 1
#define EMM_CAUSE_MAXIMUM_LENGTH 1

typedef uint8_t EmmCause;

int encode_emm_cause(EmmCause *emmcause, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_emm_cause_xml(EmmCause *emmcause, uint8_t iei);

int decode_emm_cause(EmmCause *emmcause, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* EMM CAUSE_H_ */

