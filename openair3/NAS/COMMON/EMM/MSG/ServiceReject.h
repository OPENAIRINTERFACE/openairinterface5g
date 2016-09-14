#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "EmmCause.h"
#include "GprsTimer.h"

#ifndef SERVICE_REJECT_H_
#define SERVICE_REJECT_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define SERVICE_REJECT_MINIMUM_LENGTH ( \
    EMM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define SERVICE_REJECT_MAXIMUM_LENGTH ( \
    EMM_CAUSE_MAXIMUM_LENGTH + \
    GPRS_TIMER_MAXIMUM_LENGTH )


/*
 * Message name: Service reject
 * Description: This message is sent by the network to the UE in order to reject the service request procedure. See table 8.2.24.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct service_reject_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator        protocoldiscriminator:4;
  SecurityHeaderType           securityheadertype:4;
  MessageType                  messagetype;
  EmmCause                     emmcause;
  /* Optional fields */
  uint32_t                     presencemask;
  GprsTimer                    t3442value;
} service_reject_msg;

int decode_service_reject(service_reject_msg *servicereject, uint8_t *buffer, uint32_t len);

int encode_service_reject(service_reject_msg *servicereject, uint8_t *buffer, uint32_t len);

#endif /* ! defined(SERVICE_REJECT_H_) */

