/*
 * intertask_messages_types.h
 *
 *  Created on: Jan 14, 2014
 *      Author: laurent winckel
 */

#ifndef INTERTASK_MESSAGES_TYPES_H_
#define INTERTASK_MESSAGES_TYPES_H_

typedef struct IttiMsgEmpty_s {
} IttiMsgEmpty;

typedef struct IttiMsgText_s {
  uint32_t  size;
  char      text[];
} IttiMsgText;

#endif /* INTERTASK_MESSAGES_TYPES_H_ */
