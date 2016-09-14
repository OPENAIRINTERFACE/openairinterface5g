#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "AuthenticationResponseParameter.h"

#ifndef AUTHENTICATION_RESPONSE_H_
#define AUTHENTICATION_RESPONSE_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define AUTHENTICATION_RESPONSE_MINIMUM_LENGTH ( \
    AUTHENTICATION_RESPONSE_PARAMETER_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define AUTHENTICATION_RESPONSE_MAXIMUM_LENGTH ( \
    AUTHENTICATION_RESPONSE_PARAMETER_MAXIMUM_LENGTH )


/*
 * Message name: Authentication response
 * Description: This message is sent by the UE to the network to deliver a calculated authentication response to the network. See table 8.2.8.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct authentication_response_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator             protocoldiscriminator:4;
  SecurityHeaderType                securityheadertype:4;
  MessageType                       messagetype;
  AuthenticationResponseParameter   authenticationresponseparameter;
} authentication_response_msg;

int decode_authentication_response(authentication_response_msg *authenticationresponse, uint8_t *buffer, uint32_t len);

int encode_authentication_response(authentication_response_msg *authenticationresponse, uint8_t *buffer, uint32_t len);

#endif /* ! defined(AUTHENTICATION_RESPONSE_H_) */

