#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef SUPPORTED_CODEC_LIST_H_
#define SUPPORTED_CODEC_LIST_H_

#define SUPPORTED_CODEC_LIST_MINIMUM_LENGTH 5
#define SUPPORTED_CODEC_LIST_MAXIMUM_LENGTH 5

typedef struct SupportedCodecList_tag {
  uint8_t  systemidentification;
  uint8_t  lengthofbitmap;
  uint16_t codecbitmap;
} SupportedCodecList;

int encode_supported_codec_list(SupportedCodecList *supportedcodeclist, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_supported_codec_list(SupportedCodecList *supportedcodeclist, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_supported_codec_list_xml(SupportedCodecList *supportedcodeclist, uint8_t iei);

#endif /* SUPPORTED CODEC LIST_H_ */

