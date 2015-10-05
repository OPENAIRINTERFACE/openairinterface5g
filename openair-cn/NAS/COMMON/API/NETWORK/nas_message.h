/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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
