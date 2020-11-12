/*! \file PduSessionEstablishRequest.h

\brief pdu session establishment request procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ExtendedProtocolDiscriminator.h"
#include "MessageType.h"

#ifndef PDU_SESSION_ESTABLISHMENT_REQUEST_H_
#define PDU_SESSION_ESTABLISHMENT_REQUEST_H_


/*
 * Message name: pdu session establishment request
 * Description: The PDU SESSION ESTABLISHMENT REQUEST message is sent by the UE to the SMF to initiate establishment of a PDU session. See table 8.3.1.1.1.
 * Significance: dual
 * Direction: UE to network
 */

typedef struct pdu_session_establishment_request_msg_tag {
    /* Mandatory fields */
    ExtendedProtocolDiscriminator           protocoldiscriminator;
    uint8_t                                 pdusessionid;
    uint8_t                                 pti;
    MessageType                             pdusessionestblishmsgtype;
    uint16_t                                maxdatarate;
    /* Optional fields */
} pdu_session_establishment_request_msg;


int encode_pdu_session_establishment_request(pdu_session_establishment_request_msg *pdusessionestablishrequest, uint8_t *buffer);

#endif /* ! defined(PDU_SESSION_ESTABLISHMENT_REQUEST_H_) */


