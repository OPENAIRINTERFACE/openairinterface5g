#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef TMSI_STATUS_H_
#define TMSI_STATUS_H_

#define TMSI_STATUS_MINIMUM_LENGTH 1
#define TMSI_STATUS_MAXIMUM_LENGTH 1

typedef uint8_t TmsiStatus;

int encode_tmsi_status(TmsiStatus *tmsistatus, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_tmsi_status_xml(TmsiStatus *tmsistatus, uint8_t iei);

uint8_t encode_u8_tmsi_status(TmsiStatus *tmsistatus);

int decode_tmsi_status(TmsiStatus *tmsistatus, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_tmsi_status(TmsiStatus *tmsistatus, uint8_t iei, uint8_t value, uint32_t len);

#endif /* TMSI STATUS_H_ */

