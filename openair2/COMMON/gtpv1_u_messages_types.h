/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef GTPV1_U_MESSAGES_TYPES_H_
#define GTPV1_U_MESSAGES_TYPES_H_

#include <stdint.h>
#include <netinet/in.h>
#include "common/platform_types.h"

#define LTE_GTPV1U_MAX_BEARERS_PER_UE 32 // same as LTE ASN.1 def max_val_LTE_DRB_Identity
#define NR_GTPV1U_MAX_BEARERS_PER_UE 32 // same as RRC ASN.1 def max_val_NR_DRB_Identity

#define GTPV1U_ENB_TUNNEL_DATA_IND(mSGpTR)    (mSGpTR)->ittiMsg.Gtpv1uTunnelDataInd
#define GTPV1U_ENB_DATA_FORWARDING_REQ(mSGpTR)    (mSGpTR)->ittiMsg.Gtpv1uDataForwardingReq
#define GTPV1U_ENB_DATA_FORWARDING_IND(mSGpTR)    (mSGpTR)->ittiMsg.Gtpv1uDataForwardingInd
#define GTPV1U_ENB_END_MARKER_REQ(mSGpTR)     (mSGpTR)->ittiMsg.Gtpv1uEndMarkerReq
#define GTPV1U_ENB_END_MARKER_IND(mSGpTR)     (mSGpTR)->ittiMsg.Gtpv1uEndMarkerInd

#define GTPV1U_REQ(mSGpTR)    (mSGpTR)->ittiMsg.gtpv1uReq

#define GTPV1U_ALL_TUNNELS_TEID (teid_t)0xFFFFFFFF

typedef struct gtpv1u_enb_create_x2u_tunnel_req_s {
  rnti_t                 rnti;
  int                    num_tunnels;
  teid_t                 tenb_X2u_teid[LTE_GTPV1U_MAX_BEARERS_PER_UE];
  ebi_t                  eps_bearer_id[LTE_GTPV1U_MAX_BEARERS_PER_UE];
  transport_layer_addr_t enb_addr[LTE_GTPV1U_MAX_BEARERS_PER_UE];
} gtpv1u_enb_create_x2u_tunnel_req_t;

typedef struct gtpv1u_enb_create_x2u_tunnel_resp_s {
  uint8_t                status;               ///< Status of S1U endpoint creation (Failed = 0xFF or Success = 0x0)
  rnti_t                 rnti;
  int                    num_tunnels;
  teid_t                 enb_X2u_teid[LTE_GTPV1U_MAX_BEARERS_PER_UE];
  ebi_t                  eps_bearer_id[LTE_GTPV1U_MAX_BEARERS_PER_UE];
  transport_layer_addr_t enb_addr;
} gtpv1u_enb_create_x2u_tunnel_resp_t;


typedef struct gtpv1u_enb_create_tunnel_req_s {
  rnti_t                 rnti;
  int                    num_tunnels;
  teid_t                 sgw_S1u_teid[LTE_GTPV1U_MAX_BEARERS_PER_UE];
  ebi_t                  eps_bearer_id[LTE_GTPV1U_MAX_BEARERS_PER_UE];
  transport_layer_addr_t sgw_addr[LTE_GTPV1U_MAX_BEARERS_PER_UE];
} gtpv1u_enb_create_tunnel_req_t;

typedef struct gtpv1u_enb_create_tunnel_resp_s {
  uint8_t                status;               ///< Status of S1U endpoint creation (Failed = 0xFF or Success = 0x0)
  rnti_t                 rnti;
  int                    num_tunnels;
  teid_t                 enb_S1u_teid[LTE_GTPV1U_MAX_BEARERS_PER_UE];
  ebi_t                  eps_bearer_id[LTE_GTPV1U_MAX_BEARERS_PER_UE];
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
  ebi_t                  eps_bearer_id[LTE_GTPV1U_MAX_BEARERS_PER_UE];
  //teid_t                 enb_S1u_teid;         ///< local SGW S11 Tunnel Endpoint Identifier
  int                    from_gnb;             ///< Indicates if the message comes from gNB or eNB (1 = comes from gNB, 0 from eNB)
} gtpv1u_enb_delete_tunnel_req_t;

typedef struct gtpv1u_enb_delete_tunnel_resp_s {
  rnti_t                 rnti;
  uint8_t                status;               ///< Status of S1U endpoint deleteion (Failed = 0xFF or Success = 0x0)
  teid_t                 enb_S1u_teid;         ///< local S1U Tunnel Endpoint Identifier to be deleted
} gtpv1u_enb_delete_tunnel_resp_t;

typedef struct gtpv1u_enb_data_forwarding_req_s {
  uint8_t               *buffer;
  uint32_t               length;
  uint32_t               offset;               ///< start of message offset in buffer
  rnti_t                 rnti;
  rb_id_t                rab_id;
} gtpv1u_enb_data_forwarding_req_t;

typedef struct gtpv1u_enb_data_forwarding_ind_s {
  uint32_t         frame;
  uint8_t          enb_flag;
  rb_id_t          rb_id;
  uint32_t         muip;
  uint32_t         confirmp;
  uint32_t         sdu_size;
  uint8_t          *sdu_p;
  uint8_t          mode;
  uint16_t         rnti;
  uint8_t          module_id;
  uint8_t          eNB_index;
} gtpv1u_enb_data_forwarding_ind_t;

typedef struct gtpv1u_enb_end_marker_req_s {
  uint8_t               *buffer;
  uint32_t               length;
  uint32_t               offset;               ///< start of message offset in buffer
  rnti_t                 rnti;
  rb_id_t                rab_id;
} gtpv1u_enb_end_marker_req_t;

typedef struct gtpv1u_enb_end_marker_ind_s {
  uint32_t       frame;
  uint8_t        enb_flag;
  rb_id_t        rb_id;
  uint32_t       muip;
  uint32_t       confirmp;
  uint32_t       sdu_size;
  uint8_t        *sdu_p;
  uint8_t        mode;
  uint16_t       rnti;
  uint8_t        module_id;
  uint8_t        eNB_index;
} gtpv1u_enb_end_marker_ind_t;

typedef struct {
  in_addr_t             localAddr;
  tcp_udp_port_t        localPort;
  char                  localAddrStr[256];
  char                  localPortStr[256];
} Gtpv1uReq;

/** @brief GTP-U Create Tunnel Request */
typedef struct gtpv1u_gnb_create_tunnel_req_s {
  // UE ID
  ue_id_t ue_id;
  // Outgoing TEID for the GTP tunnel
  teid_t outgoing_teid;
  // PDU Session ID (1..255) identifies the PDU session
  int pdusession_id;
  // Incoming TEID maps to this bearer ID:
  // for F1-U, DRB ID; for N3-U, PDU session ID (tunnel key).
  uint16_t incoming_rb_id;
  // Destination TL address
  transport_layer_addr_t dst_addr;
} gtpv1u_gnb_create_tunnel_req_t;

/** @brief GTP-U Create Tunnel Response */
typedef struct gtpv1u_gnb_create_tunnel_resp_s {
  uint8_t                status;               ///< Status of S1U endpoint creation (Failed = 0xFF or Success = 0x0)
  ue_id_t                ue_id;
  // GTP-U Tunnel Endpoint Identifier
  teid_t gnb_NGu_teid;
  // PDU Session ID
  int pdusession_id;
  // Destination TL address
  transport_layer_addr_t gnb_addr;
} gtpv1u_gnb_create_tunnel_resp_t;

typedef struct gtpv1u_gnb_delete_tunnel_req_s {
  ue_id_t                ue_id;
  uint8_t                num_pdusession;
  int pdusession_id[NR_GTPV1U_MAX_BEARERS_PER_UE];
} gtpv1u_gnb_delete_tunnel_req_t;

typedef struct gtpv1u_gnb_delete_tunnel_resp_s {
  ue_id_t                ue_id;
  uint8_t                status;               ///< Status of NGU endpoint deleteion (Failed = 0xFF or Success = 0x0)
  teid_t                 gnb_NGu_teid;         ///< local NGU Tunnel Endpoint Identifier to be deleted
} gtpv1u_gnb_delete_tunnel_resp_t;

#endif /* GTPV1_U_MESSAGES_TYPES_H_ */
