#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "EpsBearerIdentity.h"
#include "ProcedureTransactionIdentity.h"
#include "MessageType.h"
#include "LinkedEpsBearerIdentity.h"
#include "ProtocolConfigurationOptions.h"

#ifndef PDN_DISCONNECT_REQUEST_H_
#define PDN_DISCONNECT_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define PDN_DISCONNECT_REQUEST_MINIMUM_LENGTH ( \
    LINKED_EPS_BEARER_IDENTITY_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define PDN_DISCONNECT_REQUEST_MAXIMUM_LENGTH ( \
    LINKED_EPS_BEARER_IDENTITY_MAXIMUM_LENGTH + \
    PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH )

/* If an optional value is present and should be encoded, the corresponding
 * Bit mask should be set to 1.
 */
# define PDN_DISCONNECT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT (1<<0)

typedef enum pdn_disconnect_request_iei_tag {
  PDN_DISCONNECT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI  = 0x27, /* 0x27 = 39 */
} pdn_disconnect_request_iei;

/*
 * Message name: PDN disconnect request
 * Description: This message is sent by the UE to the network to initiate release of a PDN connection. See table 8.3.22.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct pdn_disconnect_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator               protocoldiscriminator:4;
  EpsBearerIdentity                   epsbeareridentity:4;
  ProcedureTransactionIdentity        proceduretransactionidentity;
  MessageType                         messagetype;
  LinkedEpsBearerIdentity             linkedepsbeareridentity;
  /* Optional fields */
  uint32_t                            presencemask;
  ProtocolConfigurationOptions        protocolconfigurationoptions;
} pdn_disconnect_request_msg;

int decode_pdn_disconnect_request(pdn_disconnect_request_msg *pdndisconnectrequest, uint8_t *buffer, uint32_t len);

int encode_pdn_disconnect_request(pdn_disconnect_request_msg *pdndisconnectrequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(PDN_DISCONNECT_REQUEST_H_) */

