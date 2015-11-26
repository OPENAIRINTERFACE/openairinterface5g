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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

#ifndef GTPV1_U_MESSAGES_TYPES_H_
#define GTPV1_U_MESSAGES_TYPES_H_

#include "asn1_constants.h"


#define GTPV1U_MAX_BEARERS_PER_UE max_val_DRB_Identity

#define GTPV1U_ENB_UPDATE_TUNNEL_REQ(mSGpTR)  (mSGpTR)->ittiMsg.Gtpv1uUpdateTunnelReq
#define GTPV1U_ENB_UPDATE_TUNNEL_RESP(mSGpTR) (mSGpTR)->ittiMsg.Gtpv1uUpdateTunnelResp
#define GTPV1U_ENB_DELETE_TUNNEL_REQ(mSGpTR)  (mSGpTR)->ittiMsg.Gtpv1uDeleteTunnelReq
#define GTPV1U_ENB_DELETE_TUNNEL_RESP(mSGpTR) (mSGpTR)->ittiMsg.Gtpv1uDeleteTunnelResp
#define GTPV1U_ENB_TUNNEL_DATA_IND(mSGpTR)    (mSGpTR)->ittiMsg.Gtpv1uTunnelDataInd
#define GTPV1U_ENB_TUNNEL_DATA_REQ(mSGpTR)    (mSGpTR)->ittiMsg.Gtpv1uTunnelDataReq

#define GTPV1U_ALL_TUNNELS_TEID (teid_t)0xFFFFFFFF

typedef struct gtpv1u_enb_create_tunnel_req_s {
  rnti_t                 rnti;
  int                    num_tunnels;
  teid_t                 sgw_S1u_teid[GTPV1U_MAX_BEARERS_PER_UE];  ///< Tunnel Endpoint Identifier
  ebi_t                  eps_bearer_id[GTPV1U_MAX_BEARERS_PER_UE];
  transport_layer_addr_t sgw_addr[GTPV1U_MAX_BEARERS_PER_UE];
} gtpv1u_enb_create_tunnel_req_t;

typedef struct gtpv1u_enb_create_tunnel_resp_s {
  uint8_t                status;               ///< Status of S1U endpoint creation (Failed = 0xFF or Success = 0x0)
  rnti_t                 rnti;
  int                    num_tunnels;
  teid_t                 enb_S1u_teid[GTPV1U_MAX_BEARERS_PER_UE];  ///< Tunnel Endpoint Identifier
  ebi_t                  eps_bearer_id[GTPV1U_MAX_BEARERS_PER_UE];
  transport_layer_addr_t enb_addr;
} gtpv1u_enb_create_tunnel_resp_t;

typedef struct gtpv1u_enb_update_tunnel_req_s {
  rnti_t                 rnti;
  teid_t                 enb_S1u_teid;         ///< eNB S1U Tunnel Endpoint Identifier
  teid_t                 sgw_S1u_teid;         ///< SGW S1U local Tunnel Endpoint Identifier
  transport_layer_addr_t sgw_addr;
  ebi_t                  eps_bearer_id;
} gtpv1u_enb_update_tunnel_req_t;

typedef struct gtpv1u_enb_update_tunnel_resp_s {
  rnti_t                 rnti;
  uint8_t                status;               ///< Status (Failed = 0xFF or Success = 0x0)
  teid_t                 enb_S1u_teid;         ///< eNB S1U Tunnel Endpoint Identifier
  teid_t                 sgw_S1u_teid;         ///< SGW S1U local Tunnel Endpoint Identifier
  ebi_t                  eps_bearer_id;
} gtpv1u_enb_update_tunnel_resp_t;

typedef struct gtpv1u_enb_delete_tunnel_req_s {
  rnti_t                 rnti;
  uint8_t                num_erab;
  ebi_t                  eps_bearer_id[GTPV1U_MAX_BEARERS_PER_UE];
  //teid_t                 enb_S1u_teid;         ///< local SGW S11 Tunnel Endpoint Identifier
} gtpv1u_enb_delete_tunnel_req_t;

typedef struct gtpv1u_enb_delete_tunnel_resp_s {
  rnti_t                 rnti;
  uint8_t                status;               ///< Status of S1U endpoint deleteion (Failed = 0xFF or Success = 0x0)
  teid_t                 enb_S1u_teid;         ///< local S1U Tunnel Endpoint Identifier to be deleted
} gtpv1u_enb_delete_tunnel_resp_t;

typedef struct gtpv1u_enb_tunnel_data_ind_s {
  rnti_t                 rnti;
  uint8_t               *buffer;
  uint32_t               length;
  uint32_t               offset;               ///< start of message offset in buffer
  teid_t                 enb_S1u_teid;         ///< Tunnel Endpoint Identifier
} gtpv1u_enb_tunnel_data_ind_t;

typedef struct gtpv1u_enb_tunnel_data_req_s {
  uint8_t               *buffer;
  uint32_t               length;
  uint32_t               offset;               ///< start of message offset in buffer
  rnti_t                 rnti;
  rb_id_t                rab_id;
} gtpv1u_enb_tunnel_data_req_t;

#endif /* GTPV1_U_MESSAGES_TYPES_H_ */
