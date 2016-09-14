#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "ServiceType.h"
#include "NasKeySetIdentifier.h"
#include "MobileIdentity.h"
#include "CsfbResponse.h"

#ifndef EXTENDED_SERVICE_REQUEST_H_
#define EXTENDED_SERVICE_REQUEST_H_

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define EXTENDED_SERVICE_REQUEST_MINIMUM_LENGTH ( \
    SERVICE_TYPE_MINIMUM_LENGTH + \
    NAS_KEY_SET_IDENTIFIER_MINIMUM_LENGTH + \
    MOBILE_IDENTITY_MINIMUM_LENGTH )

/* Maximum length macro. Formed by maximum length of each field */
#define EXTENDED_SERVICE_REQUEST_MAXIMUM_LENGTH ( \
    SERVICE_TYPE_MAXIMUM_LENGTH + \
    NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH + \
    MOBILE_IDENTITY_MAXIMUM_LENGTH + \
    CSFB_RESPONSE_MAXIMUM_LENGTH )


/*
 * Message name: Extended service request
 * Description: This message is sent by the UE to the network to initiate a CS fallback call or respond to a mobile terminated CS fallback request from the network. See table 8.2.15.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct extended_service_request_msg_tag {
  /* Mandatory fields */
  ProtocolDiscriminator                 protocoldiscriminator:4;
  SecurityHeaderType                    securityheadertype:4;
  MessageType                           messagetype;
  ServiceType                           servicetype;
  NasKeySetIdentifier                   naskeysetidentifier;
  MobileIdentity                        mtmsi;
  /* Optional fields */
  uint32_t                              presencemask;
  CsfbResponse                          csfbresponse;
} extended_service_request_msg;

int decode_extended_service_request(extended_service_request_msg *extendedservicerequest, uint8_t *buffer, uint32_t len);

int encode_extended_service_request(extended_service_request_msg *extendedservicerequest, uint8_t *buffer, uint32_t len);

#endif /* ! defined(EXTENDED_SERVICE_REQUEST_H_) */

