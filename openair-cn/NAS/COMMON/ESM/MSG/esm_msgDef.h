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

Version   0.1

Date    2012/09/27

Product   NAS stack

Subsystem EPS Session Management

Author    Frederic Maurel, Sebastien Roux

Description Defines identifiers of the EPS Session Management messages

*****************************************************************************/
#ifndef __ESM_MSGDEF_H__
#define __ESM_MSGDEF_H__

#include <stdint.h>
#include <asm/byteorder.h>

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Header length boundaries of EPS Session Management messages  */
#define ESM_HEADER_LENGTH   sizeof(esm_msg_header_t)
#define ESM_HEADER_MINIMUM_LENGTH ESM_HEADER_LENGTH
#define ESM_HEADER_MAXIMUM_LENGTH ESM_HEADER_LENGTH

/* Message identifiers for EPS Session Management   */
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST   0b11000001 /* 193 = 0xc1 */
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT    0b11000010 /* 194 = 0xc2 */
# define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT    0b11000011 /* 195 = 0xc3 */
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST 0b11000101 /* 197 = 0xc5 */
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT  0b11000110 /* 198 = 0xc6 */
# define ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT  0b11000111 /* 199 = 0xc7 */
# define MODIFY_EPS_BEARER_CONTEXT_REQUEST             0b11001001 /* 201 = 0xc9 */
# define MODIFY_EPS_BEARER_CONTEXT_ACCEPT              0b11001010 /* 202 = 0xca */
# define MODIFY_EPS_BEARER_CONTEXT_REJECT              0b11001011 /* 203 = 0xcb */
# define DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST         0b11001101 /* 205 = 0xcd */
# define DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT          0b11001110 /* 206 = 0xce */
# define PDN_CONNECTIVITY_REQUEST                      0b11010000 /* 208 = 0xd0 */
# define PDN_CONNECTIVITY_REJECT                       0b11010001 /* 209 = 0xd1 */
# define PDN_DISCONNECT_REQUEST                        0b11010010 /* 210 = 0xd2 */
# define PDN_DISCONNECT_REJECT                         0b11010011 /* 211 = 0xd3 */
# define BEARER_RESOURCE_ALLOCATION_REQUEST            0b11010100 /* 212 = 0xd4 */
# define BEARER_RESOURCE_ALLOCATION_REJECT             0b11010101 /* 213 = 0xd5 */
# define BEARER_RESOURCE_MODIFICATION_REQUEST          0b11010110 /* 214 = 0xd6 */
# define BEARER_RESOURCE_MODIFICATION_REJECT           0b11010111 /* 215 = 0xd7 */
# define ESM_INFORMATION_REQUEST                       0b11011001 /* 217 = 0xd9 */
# define ESM_INFORMATION_RESPONSE                      0b11011010 /* 218 = 0xda */
# define ESM_STATUS                                    0b11101000 /* 232 = 0xe8 */

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * Header of EPS Session Management plain NAS message
 * --------------------------------------------------
 *   8     7      6      5     4      3      2      1
 *  +-----------------------+------------------------+
 *  | EPS bearer identity | Protocol discriminator |
 *  +-----------------------+------------------------+
 *  | Procedure transaction identity     |
 *  +-----------------------+------------------------+
 *  |     Message type       |
 *  +-----------------------+------------------------+
 */
typedef struct {
#ifdef __LITTLE_ENDIAN_BITFIELD
  uint8_t protocol_discriminator:4;
  uint8_t eps_bearer_identity:4;
#endif
#ifdef __BIG_ENDIAN_BITFIELD
  uint8_t eps_bearer_identity:4;
  uint8_t protocol_discriminator:4;
#endif
  uint8_t procedure_transaction_identity;
  uint8_t message_type;
} __attribute__((__packed__)) esm_msg_header_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* __ESM_MSGDEF_H__ */

