#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "EsmMessageContainer.h"

#ifndef ATTACH_COMPLETE_H_
#define ATTACH_COMPLETE_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ATTACH_COMPLETE_MINIMUM_LENGTH ( \
    ESM_MESSAGE_CONTAINER_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define ATTACH_COMPLETE_MAXIMUM_LENGTH ( \
    ESM_MESSAGE_CONTAINER_MAXIMUM_LENGTH )


/*
 * Message name: Attach complete
 * Description: This message is sent by the UE to the network in response to an ATTACH ACCEPT message. See table 8.2.2.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct attach_complete_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator         protocoldiscriminator:4;
  SecurityHeaderType            securityheadertype:4;
  MessageType                   messagetype;
  EsmMessageContainer           esmmessagecontainer;
} attach_complete_msg;

int decode_attach_complete(attach_complete_msg *attachcomplete, uint8_t *buffer, uint32_t len);

int encode_attach_complete(attach_complete_msg *attachcomplete, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ATTACH_COMPLETE_H_) */

