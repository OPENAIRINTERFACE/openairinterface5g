#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "EmmCause.h"

#ifndef TRACKING_AREA_UPDATE_REJECT_H_
#define TRACKING_AREA_UPDATE_REJECT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define TRACKING_AREA_UPDATE_REJECT_MINIMUM_LENGTH ( \
    EMM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define TRACKING_AREA_UPDATE_REJECT_MAXIMUM_LENGTH ( \
    EMM_CAUSE_MAXIMUM_LENGTH )


/*
 * Message name: Tracking area update reject
 * Description: This message is sent by the network to the UE in order to reject the tracking area updating procedure. See table 8.2.28.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct tracking_area_update_reject_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                   protocoldiscriminator:4;
  SecurityHeaderType                      securityheadertype:4;
  MessageType                             messagetype;
  EmmCause                                emmcause;
} tracking_area_update_reject_msg;

int decode_tracking_area_update_reject(tracking_area_update_reject_msg *trackingareaupdatereject, uint8_t *buffer, uint32_t len);

int encode_tracking_area_update_reject(tracking_area_update_reject_msg *trackingareaupdatereject, uint8_t *buffer, uint32_t len);

#endif /* ! defined(TRACKING_AREA_UPDATE_REJECT_H_) */

