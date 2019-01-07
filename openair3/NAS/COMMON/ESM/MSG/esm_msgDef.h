/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

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

