#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "NasMessageContainer.h"

#ifndef UPLINK_NAS_TRANSPORT_H_
#define UPLINK_NAS_TRANSPORT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define UPLINK_NAS_TRANSPORT_MINIMUM_LENGTH ( \
    NAS_MESSAGE_CONTAINER_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define UPLINK_NAS_TRANSPORT_MAXIMUM_LENGTH ( \
    NAS_MESSAGE_CONTAINER_MAXIMUM_LENGTH )


/*
 * Message name: Uplink NAS Transport
 * Description: This message is sent by the UE to the network in order to carry an SMS message in encapsulated format. See table 8.2.30.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct uplink_nas_transport_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator             protocoldiscriminator:4;
  SecurityHeaderType                securityheadertype:4;
  MessageType                       messagetype;
  NasMessageContainer               nasmessagecontainer;
} uplink_nas_transport_msg;

int decode_uplink_nas_transport(uplink_nas_transport_msg *uplinknastransport, uint8_t *buffer, uint32_t len);

int encode_uplink_nas_transport(uplink_nas_transport_msg *uplinknastransport, uint8_t *buffer, uint32_t len);

#endif /* ! defined(UPLINK_NAS_TRANSPORT_H_) */

