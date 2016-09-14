#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef ESM_INFORMATION_TRANSFER_FLAG_H_
#define ESM_INFORMATION_TRANSFER_FLAG_H_

#define ESM_INFORMATION_TRANSFER_FLAG_MINIMUM_LENGTH 1
#define ESM_INFORMATION_TRANSFER_FLAG_MAXIMUM_LENGTH 1

typedef uint8_t EsmInformationTransferFlag;

int encode_esm_information_transfer_flag(EsmInformationTransferFlag *esminformationtransferflag, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_esm_information_transfer_flag_xml(EsmInformationTransferFlag *esminformationtransferflag, uint8_t iei);

uint8_t encode_u8_esm_information_transfer_flag(EsmInformationTransferFlag *esminformationtransferflag);

int decode_esm_information_transfer_flag(EsmInformationTransferFlag *esminformationtransferflag, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_esm_information_transfer_flag(EsmInformationTransferFlag *esminformationtransferflag, uint8_t iei, uint8_t value, uint32_t len);

#endif /* ESM INFORMATION TRANSFER FLAG_H_ */

