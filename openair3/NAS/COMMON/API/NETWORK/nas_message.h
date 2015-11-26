/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
/*****************************************************************************

Source      nas_message.h

Version     0.1

Date        2012/26/09

Product     NAS stack

Subsystem   Application Programming Interface

Author      Frederic Maurel

Description Defines the layer 3 messages supported by the NAS sublayer
        protocol and functions used to encode and decode

*****************************************************************************/
#ifndef __NAS_MESSAGE_H__
#define __NAS_MESSAGE_H__

#include "commonDef.h"
#include "emm_msg.h"
#if defined(NAS_BUILT_IN_EPC)
#include "emmData.h"
#endif
#include "esm_msg.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

#define NAS_MESSAGE_SECURITY_HEADER_SIZE    6

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/* Structure of security protected header */
typedef struct {
#ifdef __LITTLE_ENDIAN_BITFIELD
  eps_protocol_discriminator_t    protocol_discriminator:4;
  uint8_t                         security_header_type:4;
#endif
#ifdef __BIG_ENDIAN_BITFIELD
  uint8_t security_header_type:4;
  uint8_t protocol_discriminator:4;
#endif
  uint32_t message_authentication_code;
  uint8_t sequence_number;
} nas_message_security_header_t;

/* Structure of plain NAS message */
typedef union {
  EMM_msg emm;    /* EPS Mobility Management messages */
  ESM_msg esm;    /* EPS Session Management messages  */
} nas_message_plain_t;

/* Structure of security protected NAS message */
typedef struct {
  nas_message_security_header_t header;
  nas_message_plain_t plain;
} nas_message_security_protected_t;

/*
 * Structure of a layer 3 NAS message
 */
typedef union {
  nas_message_security_header_t header;
  nas_message_security_protected_t security_protected;
  nas_message_plain_t plain;
} nas_message_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int nas_message_encrypt(
  const char                          *inbuf,
  char                                *outbuf,
  const nas_message_security_header_t *header,
  int                                  length,
  void                                *security);

int nas_message_decrypt(const char *inbuf,
                        char                           *outbuf,
                        nas_message_security_header_t  *header,
                        int                             length,
                        void                           *security);

int nas_message_decode(
  const char * const  buffer,
  nas_message_t      *msg,
  int                 length,
  void               *security);

int nas_message_encode(
  char                       *buffer,
  const nas_message_t * const msg,
  int                         length,
  void                       *security);

#endif /* __NAS_MESSAGE_H__*/
