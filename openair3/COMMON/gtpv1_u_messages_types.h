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

#ifndef GTPV1_U_MESSAGES_TYPES_H_
#define GTPV1_U_MESSAGES_TYPES_H_

#include "../SGW-LITE/sgw_lite_ie_defs.h"

typedef struct {
  Teid_t           context_teid;               ///< Tunnel Endpoint Identifier
  ebi_t            eps_bearer_id;
} Gtpv1uCreateTunnelReq;

typedef struct {
  uint8_t  status;           ///< Status of S1U endpoint creation (Failed = 0xFF or Success = 0x0)
  Teid_t   context_teid;     ///< local SGW S11 Tunnel Endpoint Identifier
  Teid_t   S1u_teid;         ///< Tunnel Endpoint Identifier
  ebi_t    eps_bearer_id;
} Gtpv1uCreateTunnelResp;

typedef struct {
  Teid_t           context_teid;     ///< S11 Tunnel Endpoint Identifier
  Teid_t           sgw_S1u_teid;     ///< SGW S1U local Tunnel Endpoint Identifier
  Teid_t           enb_S1u_teid;     ///< eNB S1U Tunnel Endpoint Identifier
  ip_address_t     enb_ip_address_for_S1u;
  ebi_t            eps_bearer_id;
} Gtpv1uUpdateTunnelReq;

typedef struct {
  uint8_t          status;           ///< Status (Failed = 0xFF or Success = 0x0)
  Teid_t           context_teid;     ///< S11 Tunnel Endpoint Identifier
  Teid_t           sgw_S1u_teid;     ///< SGW S1U local Tunnel Endpoint Identifier
  Teid_t           enb_S1u_teid;     ///< eNB S1U Tunnel Endpoint Identifier
  ebi_t            eps_bearer_id;
} Gtpv1uUpdateTunnelResp;

typedef struct {
  Teid_t           context_teid;   ///< local SGW S11 Tunnel Endpoint Identifier
  Teid_t           S1u_teid;       ///< local S1U Tunnel Endpoint Identifier to be deleted
} Gtpv1uDeleteTunnelReq;

typedef struct {
  uint8_t  status;           ///< Status of S1U endpoint deleteion (Failed = 0xFF or Success = 0x0)
  Teid_t   context_teid;     ///< local SGW S11 Tunnel Endpoint Identifier
  Teid_t   S1u_teid;         ///< local S1U Tunnel Endpoint Identifier to be deleted
} Gtpv1uDeleteTunnelResp;

typedef struct {
  uint8_t  *buffer;
  uint32_t  length;
  uint32_t  offset;                       ///< start of message offset in buffer
  Teid_t    local_S1u_teid;               ///< Tunnel Endpoint Identifier
} Gtpv1uTunnelDataInd;

typedef struct {
  uint8_t  *buffer;
  uint32_t  length;
  uint32_t  offset;                       ///< start of message offset in buffer
  Teid_t    local_S1u_teid;               ///< Tunnel Endpoint Identifier
  Teid_t    S1u_enb_teid;                 ///< Tunnel Endpoint Identifier
} Gtpv1uTunnelDataReq;

#endif /* GTPV1_U_MESSAGES_TYPES_H_ */
