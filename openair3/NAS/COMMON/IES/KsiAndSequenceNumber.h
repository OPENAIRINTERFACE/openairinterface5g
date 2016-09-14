#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef KSI_AND_SEQUENCE_NUMBER_H_
#define KSI_AND_SEQUENCE_NUMBER_H_

#define KSI_AND_SEQUENCE_NUMBER_MINIMUM_LENGTH 2
#define KSI_AND_SEQUENCE_NUMBER_MAXIMUM_LENGTH 2

typedef struct KsiAndSequenceNumber_tag {
  uint8_t  ksi:3;
  uint8_t  sequencenumber:5;
} KsiAndSequenceNumber;

int encode_ksi_and_sequence_number(KsiAndSequenceNumber *ksiandsequencenumber, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_ksi_and_sequence_number_xml(KsiAndSequenceNumber *ksiandsequencenumber, uint8_t iei);

int decode_ksi_and_sequence_number(KsiAndSequenceNumber *ksiandsequencenumber, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* KSI AND SEQUENCE NUMBER_H_ */

