#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"

#ifndef GUTI_REALLOCATION_COMPLETE_H_
#define GUTI_REALLOCATION_COMPLETE_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define GUTI_REALLOCATION_COMPLETE_MINIMUM_LENGTH (0)

/* Maximum length macro. Formed by maximum length of each field */
#define GUTI_REALLOCATION_COMPLETE_MAXIMUM_LENGTH (0)

/*
 * Message name: GUTI reallocation complete
 * Description: This message is sent by the UE to the network to indicate that reallocation of a GUTI has taken place. See table 8.2.17.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct guti_reallocation_complete_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                   protocoldiscriminator:4;
  SecurityHeaderType                      securityheadertype:4;
  MessageType                             messagetype;
} guti_reallocation_complete_msg;

int decode_guti_reallocation_complete(guti_reallocation_complete_msg *gutireallocationcomplete, uint8_t *buffer, uint32_t len);

int encode_guti_reallocation_complete(guti_reallocation_complete_msg *gutireallocationcomplete, uint8_t *buffer, uint32_t len);

#endif /* ! defined(GUTI_REALLOCATION_COMPLETE_H_) */

