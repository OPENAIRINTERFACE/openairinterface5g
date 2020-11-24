/*! \file FGSIdentityResponse.h

\brief identity response procedures for gNB
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
#include "FGSMobileIdentity.h"
#include "MessageType.h"

#ifndef FGS_IDENTITY_RESPONSE_H_
#define FGS_IDENTITY_RESPONSE_H_

/*
 * Message name: Identity response
 * Description: This message is sent by the UE to the AMF to provide the requested identity. See table 8.2.22.1.
 * Significance: dual
 * Direction: UE to AMF
 */

typedef struct fgs_identiy_response_msg_tag {
  /* Mandatory fields */
  ExtendedProtocolDiscriminator           protocoldiscriminator;
  SecurityHeaderType                      securityheadertype:4;
  SpareHalfOctet                          sparehalfoctet:4;
  MessageType                             messagetype;
  FGSMobileIdentity                       fgsmobileidentity;

} fgs_identiy_response_msg;

int encode_identiy_response(fgs_identiy_response_msg *fgs_identity_reps, uint8_t *buffer, uint32_t len);

#endif /* ! defined(FGS_IDENTITY_RESPONSE_H_) */


