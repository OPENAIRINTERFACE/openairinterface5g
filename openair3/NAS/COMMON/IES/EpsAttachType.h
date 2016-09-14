#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EPS_ATTACH_TYPE_H_
#define EPS_ATTACH_TYPE_H_

#define EPS_ATTACH_TYPE_MINIMUM_LENGTH 1
#define EPS_ATTACH_TYPE_MAXIMUM_LENGTH 1

#define EPS_ATTACH_TYPE_EPS   0b0001
#define EPS_ATTACH_TYPE_IMSI    0b0010
#define EPS_ATTACH_TYPE_EMERGENCY 0b0110
#define EPS_ATTACH_TYPE_RESERVED  0b0111
typedef uint8_t EpsAttachType;

int encode_eps_attach_type(EpsAttachType *epsattachtype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_attach_type_xml(EpsAttachType *epsattachtype, uint8_t iei);

uint8_t encode_u8_eps_attach_type(EpsAttachType *epsattachtype);

int decode_eps_attach_type(EpsAttachType *epsattachtype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_eps_attach_type(EpsAttachType *epsattachtype, uint8_t iei, uint8_t value, uint32_t len);

#endif /* EPS ATTACH TYPE_H_ */

