/*! \file RegistrationRequest.h

\brief registration request procedures for gNB
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
#include "SORTransparentContainer.h"

#ifndef REGISTRATION_COMPLETE_H_
#define REGISTRATION_COMPLETE_H_


typedef struct registration_complete_msg_tag {
  /* Mandatory fields */
  ExtendedProtocolDiscriminator           protocoldiscriminator;
  SecurityHeaderType                      securityheadertype:4;
  SpareHalfOctet                          sparehalfoctet:4;
  MessageType                             messagetype;

  /* Optional fields */
  SORTransparentContainer                 sortransparentcontainer;
} registration_complete_msg;


int decode_registration_complete(registration_complete_msg *registrationcomplete, uint8_t *buffer, uint32_t len);

int encode_registration_complete(registration_complete_msg *registrationcomplete, uint8_t *buffer, uint32_t len);

#endif /* ! defined(REGISTRATION_COMPLETE_H_) */

