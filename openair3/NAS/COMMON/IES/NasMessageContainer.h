#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef NAS_MESSAGE_CONTAINER_H_
#define NAS_MESSAGE_CONTAINER_H_

#define NAS_MESSAGE_CONTAINER_MINIMUM_LENGTH 4
#define NAS_MESSAGE_CONTAINER_MAXIMUM_LENGTH 253

typedef struct NasMessageContainer_tag {
  OctetString nasmessagecontainercontents;
} NasMessageContainer;

int encode_nas_message_container(NasMessageContainer *nasmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_nas_message_container(NasMessageContainer *nasmessagecontainer, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_nas_message_container_xml(NasMessageContainer *nasmessagecontainer, uint8_t iei);

#endif /* NAS MESSAGE CONTAINER_H_ */

