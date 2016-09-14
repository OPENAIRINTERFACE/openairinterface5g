#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "MobileIdentity.h"

#ifndef SECURITY_MODE_COMPLETE_H_
#define SECURITY_MODE_COMPLETE_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define SECURITY_MODE_COMPLETE_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define SECURITY_MODE_COMPLETE_MAXIMUM_LENGTH ( \
    MOBILE_IDENTITY_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define SECURITY_MODE_COMPLETE_IMEISV_PRESENT (1<<0)

typedef enum security_mode_complete_iei_tag {
  SECURITY_MODE_COMPLETE_IMEISV_IEI  = 0x23, /* 0x23 = 35 */
} security_mode_complete_iei;

/*
 * Message name: Security mode complete
 * Description: This message is sent by the UE to the network in response to a SECURITY MODE COMMAND message. See table 8.2.21.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct security_mode_complete_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator               protocoldiscriminator:4;
  SecurityHeaderType                  securityheadertype:4;
  MessageType                         messagetype;
  /* Optional fields */
  uint32_t                            presencemask;
  MobileIdentity                      imeisv;
} security_mode_complete_msg;

int decode_security_mode_complete(security_mode_complete_msg *securitymodecomplete, uint8_t *buffer, uint32_t len);

int encode_security_mode_complete(security_mode_complete_msg *securitymodecomplete, uint8_t *buffer, uint32_t len);

#endif /* ! defined(SECURITY_MODE_COMPLETE_H_) */

