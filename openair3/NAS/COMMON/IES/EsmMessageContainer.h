#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef ESM_MESSAGE_CONTAINER_H_
#define ESM_MESSAGE_CONTAINER_H_

#define ESM_MESSAGE_CONTAINER_MINIMUM_LENGTH 2 // [length]+[length]
#define ESM_MESSAGE_CONTAINER_MAXIMUM_LENGTH 65538 // [IEI]+[length]+[length]+[ESM msg]

typedef struct EsmMessageContainer_tag {
  OctetString esmmessagecontainercontents;
} EsmMessageContainer;

int encode_esm_message_container(EsmMessageContainer *esmmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_esm_message_container(EsmMessageContainer *esmmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_esm_message_container_xml(EsmMessageContainer *esmmessagecontainer, uint8_t iei);

#endif /* ESM MESSAGE CONTAINER_H_ */

