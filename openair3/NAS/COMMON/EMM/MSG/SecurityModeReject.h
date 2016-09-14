#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "EmmCause.h"

#ifndef SECURITY_MODE_REJECT_H_
#define SECURITY_MODE_REJECT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define SECURITY_MODE_REJECT_MINIMUM_LENGTH ( \
    EMM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define SECURITY_MODE_REJECT_MAXIMUM_LENGTH ( \
    EMM_CAUSE_MAXIMUM_LENGTH )


/*
 * Message name: Security mode reject
 * Description: This message is sent by the UE to the network to indicate that the corresponding security mode command has been rejected. See table 8.2.22.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct security_mode_reject_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator             protocoldiscriminator:4;
  SecurityHeaderType                securityheadertype:4;
  MessageType                       messagetype;
  EmmCause                          emmcause;
} security_mode_reject_msg;

int decode_security_mode_reject(security_mode_reject_msg *securitymodereject, uint8_t *buffer, uint32_t len);

int encode_security_mode_reject(security_mode_reject_msg *securitymodereject, uint8_t *buffer, uint32_t len);

#endif /* ! defined(SECURITY_MODE_REJECT_H_) */

