/*! \file FGSAuthenticationResponse.h

\brief authentication response procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ExtendedProtocolDiscriminator.h"
#include "SecurityHeaderType.h"
#include "SpareHalfOctet.h"
#include "MessageType.h"
#include "AuthenticationResponseParameter.h"

#ifndef FGS_AUTHENTICATION_RESPONSE_H_
#define FGS_AUTHENTICATION_RESPONSE_H_

#define AUTHENTICATION_RESPONSE_PARAMETER_IEI     0x2d

/*
 * Message name: Identity response
 * Description: This message is sent by the UE to the AMF to provide the requested identity. See table 8.2.22.1.
 * Significance: dual
 * Direction: UE to AMF
 */

typedef struct fgs_authentication_response_msg_tag {
  /* Mandatory fields */
  ExtendedProtocolDiscriminator           protocoldiscriminator;
  SecurityHeaderType                      securityheadertype:4;
  SpareHalfOctet                          sparehalfoctet:4;
  MessageType                             messagetype;
  AuthenticationResponseParameter         authenticationresponseparameter;
} fgs_authentication_response_msg;

int encode_fgs_authentication_response(fgs_authentication_response_msg *authentication_response, uint8_t *buffer, uint32_t len);

#endif /* ! defined(FGS_AUTHENTICATION_RESPONSE_H_) */


