/*! \file RegistrationAccept.h

\brief 5GS registration accept procedures
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
#include "FGSRegistrationResult.h"

#ifndef REGISTRATION_ACCEPT_H_
#define REGISTRATION_ACCEPT_H_

/*
 * Message name: Registration accept
 * Description: The REGISTRATION ACCEPT message is sent by the AMF to the UE. See table 8.2.7.1.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct registration_accept_msg_tag {
  /* Mandatory fields */
  ExtendedProtocolDiscriminator           protocoldiscriminator;
  SecurityHeaderType                      securityheadertype:4;
  SpareHalfOctet                          sparehalfoctet:4;
  MessageType                             messagetype;
  FGSRegistrationResult                   fgsregistrationresult;
} registration_accept_msg;

int decode_registration_accept(registration_accept_msg *registrationaccept, uint8_t *buffer, uint32_t len);

int encode_registration_accept(registration_accept_msg *registrationaccept, uint8_t *buffer, uint32_t len);

#endif /* ! defined(REGISTRATION_ACCEPT_H_) */

