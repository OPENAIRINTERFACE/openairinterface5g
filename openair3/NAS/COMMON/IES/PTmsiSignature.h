#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef P_TMSI_SIGNATURE_H_
#define P_TMSI_SIGNATURE_H_

#define P_TMSI_SIGNATURE_MINIMUM_LENGTH 4
#define P_TMSI_SIGNATURE_MAXIMUM_LENGTH 4

typedef struct PTmsiSignature_tag {
  OctetString ptmsisignaturevalue;
} PTmsiSignature;

int encode_p_tmsi_signature(PTmsiSignature *ptmsisignature, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_p_tmsi_signature_xml(PTmsiSignature *ptmsisignature, uint8_t iei);

int decode_p_tmsi_signature(PTmsiSignature *ptmsisignature, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* P TMSI SIGNATURE_H_ */

