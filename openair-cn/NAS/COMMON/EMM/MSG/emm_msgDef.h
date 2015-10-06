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
Version   0.1

Date    2012/09/27

Product   NAS stack

Subsystem EPS Mobility Management

Author    Frederic Maurel, Sebastien Roux

Description Defines identifiers of the EPS Mobility Management messages.

*****************************************************************************/
#ifndef __EMM_MSGDEF_H__
#define __EMM_MSGDEF_H__

#include <stdint.h>
#include <asm/byteorder.h>

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Header length boundaries of EPS Mobility Management messages  */
#define EMM_HEADER_LENGTH   sizeof(emm_msg_header_t)
#define EMM_HEADER_MINIMUM_LENGTH EMM_HEADER_LENGTH
#define EMM_HEADER_MAXIMUM_LENGTH EMM_HEADER_LENGTH

/* EPS Mobility Management Security header type */
#define SECURITY_HEADER_TYPE_NOT_PROTECTED      0b0000
#define SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED    0b0001
#define SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_CYPHERED 0b0010
#define SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_NEW    0b0011
#define SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_CYPHERED_NEW 0b0100
#define SECURITY_HEADER_TYPE_SERVICE_REQUEST      0b1100

/* Message identifiers for EPS Mobility Management     */
# define ATTACH_REQUEST                0b01000001 /* 65 = 0x41 */
# define ATTACH_ACCEPT                 0b01000010 /* 66 = 0x42 */
# define ATTACH_COMPLETE               0b01000011 /* 67 = 0x43 */
# define ATTACH_REJECT                 0b01000100 /* 68 = 0x44 */
# define DETACH_REQUEST                0b01000101 /* 69 = 0x45 */
# define DETACH_ACCEPT                 0b01000110 /* 70 = 0x46 */
# define TRACKING_AREA_UPDATE_REQUEST  0b01001000 /* 72 = 0x48 */
# define TRACKING_AREA_UPDATE_ACCEPT   0b01001001 /* 73 = 0x49 */
# define TRACKING_AREA_UPDATE_COMPLETE 0b01001010 /* 74 = 0x4a */
# define TRACKING_AREA_UPDATE_REJECT   0b01001011 /* 75 = 0x4b */
# define EXTENDED_SERVICE_REQUEST      0b01001100 /* 76 = 0x4c */
# define SERVICE_REJECT                0b01001110 /* 78 = 0x4e */
# define GUTI_REALLOCATION_COMMAND     0b01010000 /* 80 = 0x50 */
# define GUTI_REALLOCATION_COMPLETE    0b01010001 /* 81 = 0x51 */
# define AUTHENTICATION_REQUEST        0b01010010 /* 82 = 0x52 */
# define AUTHENTICATION_RESPONSE       0b01010011 /* 83 = 0x53 */
# define AUTHENTICATION_REJECT         0b01010100 /* 84 = 0x54 */
# define AUTHENTICATION_FAILURE        0b01011100 /* 92 = 0x5c */
# define IDENTITY_REQUEST              0b01010101 /* 85 = 0x55 */
# define IDENTITY_RESPONSE             0b01010110 /* 86 = 0x56 */
# define SECURITY_MODE_COMMAND         0b01011101 /* 93 = 0x5d */
# define SECURITY_MODE_COMPLETE        0b01011110 /* 94 = 0x5e */
# define SECURITY_MODE_REJECT          0b01011111 /* 95 = 0x5f */
# define EMM_STATUS                    0b01100000 /* 96 = 0x60 */
# define EMM_INFORMATION               0b01100001 /* 97 = 0x61 */
# define DOWNLINK_NAS_TRANSPORT        0b01100010 /* 98 = 0x62 */
# define UPLINK_NAS_TRANSPORT          0b01100011 /* 99 = 0x63 */
# define CS_SERVICE_NOTIFICATION       0b01100100 /* 100 = 0x64 */

/*
 * Message identifiers for EMM messages that does not follow the structure
 * of a standard layer 3 message
 */
# define SERVICE_REQUEST               0b01001101 /* TODO: TBD - 77 = 0x4d */

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * Header of EPS Mobility Management plain NAS message
 * ---------------------------------------------------
 *   8     7      6      5     4      3      2      1
 *  +-----------------------+------------------------+
 *  | Security header type  | Protocol discriminator |
 *  +-----------------------+------------------------+
 *  |     Message type       |
 *  +-----------------------+------------------------+
 */
typedef struct {
#ifdef __LITTLE_ENDIAN_BITFIELD
  uint8_t protocol_discriminator:4;
  uint8_t security_header_type:4;
#endif
#ifdef __BIG_ENDIAN_BITFIELD
  uint8_t security_header_type:4;
  uint8_t protocol_discriminator:4;
#endif
  uint8_t message_type;
} __attribute__((__packed__)) emm_msg_header_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* __EMM_MSGDEF_H__ */

