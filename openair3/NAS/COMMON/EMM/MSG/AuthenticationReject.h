#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"

#ifndef AUTHENTICATION_REJECT_H_
#define AUTHENTICATION_REJECT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define AUTHENTICATION_REJECT_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define AUTHENTICATION_REJECT_MAXIMUM_LENGTH (0)

/*
 * Message name: Authentication reject
 * Description: This message is sent by the network to the UE to indicate that the authentication procedure has failed and that the UE shall abort all activities. See table 8.2.6.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct authentication_reject_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator           protocoldiscriminator:4;
  SecurityHeaderType              securityheadertype:4;
  MessageType                     messagetype;
} authentication_reject_msg;

int decode_authentication_reject(authentication_reject_msg *authenticationreject, uint8_t *buffer, uint32_t len);

int encode_authentication_reject(authentication_reject_msg *authenticationreject, uint8_t *buffer, uint32_t len);

#endif /* ! defined(AUTHENTICATION_REJECT_H_) */

