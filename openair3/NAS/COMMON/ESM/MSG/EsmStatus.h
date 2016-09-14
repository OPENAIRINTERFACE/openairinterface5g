#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "EpsBearerIdentity.h"
#include "ProcedureTransactionIdentity.h"
#include "MessageType.h"
#include "EsmCause.h"

#ifndef ESM_STATUS_H_
#define ESM_STATUS_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define ESM_STATUS_MINIMUM_LENGTH ( \
    ESM_CAUSE_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define ESM_STATUS_MAXIMUM_LENGTH ( \
    ESM_CAUSE_MAXIMUM_LENGTH )


/*
 * Message name: ESM status
 * Description: This message is sent by the network or the UE to pass information on the status of the indicated EPS bearer context and report certain error conditions (e.g. as listed in clause 7). See table 8.3.15.1.
 * Significance: dual
 * Direction: both
 */

typedef struct esm_status_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator        protocoldiscriminator:4;
  EpsBearerIdentity            epsbeareridentity:4;
  ProcedureTransactionIdentity proceduretransactionidentity;
  MessageType                  messagetype;
  EsmCause                     esmcause;
} esm_status_msg;

int decode_esm_status(esm_status_msg *esmstatus, uint8_t *buffer, uint32_t len);

int encode_esm_status(esm_status_msg *esmstatus, uint8_t *buffer, uint32_t len);

#endif /* ! defined(ESM_STATUS_H_) */

