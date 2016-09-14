#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef MESSAGE_TYPE_H_
#define MESSAGE_TYPE_H_

#define MESSAGE_TYPE_MINIMUM_LENGTH 1
#define MESSAGE_TYPE_MAXIMUM_LENGTH 1

typedef uint8_t MessageType;

int encode_message_type(MessageType *messagetype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_message_type_xml(MessageType *messagetype, uint8_t iei);

int decode_message_type(MessageType *messagetype, uint8_t iei, uint8_t *buffer, uint32_t len);

#endif /* MESSAGE TYPE_H_ */

