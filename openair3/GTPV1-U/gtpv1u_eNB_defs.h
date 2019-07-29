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

/*! \file gtpv1u_eNB_defs.h
 * \brief
 * \author Sebastien ROUX, Lionel GAUTHIER
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */

#include "hashtable.h"
#include "LTE_asn_constant.h"

#ifndef GTPV1U_ENB_DEFS_H_
#define GTPV1U_ENB_DEFS_H_

#include "NwGtpv1u.h"

#define GTPV1U_UDP_PORT (2152)
#define GTPV1U_BEARER_OFFSET 3

#define GTPV1U_MAX_BEARERS_ID     (max_val_LTE_DRB_Identity - GTPV1U_BEARER_OFFSET)

#define GTPV1U_SOURCE_ENB (0)
#define GTPV1U_TARGET_ENB (1)
#define GTPV1U_MSG_FROM_SOURCE_ENB (0)
#define GTPV1U_MSG_FROM_SPGW (1)

typedef enum {
  BEARER_DOWN = 0,
  BEARER_IN_CONFIG,
  BEARER_UP,
  BEARER_DL_HANDOVER,
  BEARER_UL_HANDOVER,
  BEARER_MAX,
} bearer_state_t;


typedef struct gtpv1u_teid_data_s {
  /* UE identifier for oaisim stack */
  module_id_t  enb_id;
  rnti_t       ue_id;
  ebi_t        eps_bearer_id;
} gtpv1u_teid_data_t;


typedef struct gtpv1u_bearer_s {
  /* TEID used in dl and ul */
  teid_t          teid_eNB;                ///< eNB TEID
  uintptr_t       teid_eNB_stack_session;  ///< eNB TEID
  teid_t          teid_sgw;                ///< Remote TEID
  in_addr_t       sgw_ip_addr;
  struct in6_addr sgw_ip6_addr;
  teid_t          teid_teNB;
  in_addr_t       tenb_ip_addr;				///< target eNB ipv4
  struct in6_addr tenb_ip6_addr;				///< target eNB ipv6
  tcp_udp_port_t  port;
  //NwGtpv1uStackSessionHandleT stack_session;
  bearer_state_t state;
} gtpv1u_bearer_t;

typedef struct gtpv1u_ue_data_s {
  /* UE identifier for oaisim stack */
  rnti_t   ue_id;

  /* Unique identifier used between PDCP and GTP-U to distinguish UEs */
  uint32_t instance_id;
  int      num_bearers;
  /* Bearer related data.
   * Note that the first LCID available for data is 3 and we fixed the maximum
   * number of e-rab per UE to be (32 [id range]), max RB is 11. The real rb id will 3 + rab_id (3..32).
   */
  gtpv1u_bearer_t bearers[GTPV1U_MAX_BEARERS_ID];

  //RB_ENTRY(gtpv1u_ue_data_s) gtpv1u_ue_node;
} gtpv1u_ue_data_t;

typedef struct gtpv1u_data_s {
  /* nwgtpv1u stack internal data */
  NwGtpv1uStackHandleT  gtpv1u_stack;

  /* RB tree of UEs */
  hash_table_t         *ue_mapping;   // PDCP->GTPV1U
  hash_table_t         *teid_mapping; // GTPV1U -> PDCP

  //RB_HEAD(gtpv1u_ue_map, gtpv1u_ue_data_s) gtpv1u_ue_map_head;
  /* Local IP address to use */
  in_addr_t             enb_ip_address_for_S1u_S12_S4_up;
  /* UDP internal data */
  //udp_data_t            udp_data;

  uint16_t              seq_num;
  uint8_t               restart_counter;

#ifdef GTPU_IN_KERNEL
  char                 *interface_name;
  int                   interface_index;

  struct iovec         *malloc_ring;
  void                 *sock_mmap_ring[16];
  int                   sock_desc[16]; // indexed by marking
#endif
} gtpv1u_data_t;

int
gtpv1u_new_data_req(
  uint8_t  enb_module_idP,
  rnti_t   ue_rntiP,
  uint8_t  rab_idP,
  uint8_t *buffer_pP,
  uint32_t buf_lenP,
  uint32_t buf_offsetP
);

int
gtpv1u_initial_req(
  gtpv1u_data_t *gtpv1u_data_p,
  uint32_t teid,
  uint16_t port,
  uint32_t address);

#endif /* GTPV1U_ENB_DEFS_H_ */
