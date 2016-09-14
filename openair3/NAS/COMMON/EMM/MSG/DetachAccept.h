#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"

#ifndef DETACH_ACCEPT_H_
#define DETACH_ACCEPT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define DETACH_ACCEPT_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define DETACH_ACCEPT_MAXIMUM_LENGTH (0)

/*
 * Message name: Detach accept
 * Description: This message is sent by the network to indicate that the detach procedure has been completed. See table 8.2.10.1.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct detach_accept_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator       protocoldiscriminator:4;
  SecurityHeaderType          securityheadertype:4;
  MessageType                 messagetype;
} detach_accept_msg;

int decode_detach_accept(detach_accept_msg *detachaccept, uint8_t *buffer, uint32_t len);

int encode_detach_accept(detach_accept_msg *detachaccept, uint8_t *buffer, uint32_t len);

#endif /* ! defined(DETACH_ACCEPT_H_) */

