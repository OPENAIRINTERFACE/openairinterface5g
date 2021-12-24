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

/*! \file gtpv1u_gNB_defs.h
 * \brief 
 * \author Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \version 0.1
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 *         (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com) 
 */

#include "hashtable.h"
#include "NR_asn_constant.h"

#ifndef GTPV1U_GNB_DEFS_H_
#define GTPV1U_GNB_DEFS_H_

#include "NwGtpv1u.h"

#define GTPV1U_UDP_PORT (2152)

#define NR_GTPV1U_MAX_BEARERS_ID     (max_val_NR_DRB_Identity - 3)

#define GTPV1U_SOURCE_GNB (0)
#define GTPV1U_TARGET_GNB (1)
#define GTPV1U_MSG_FROM_SOURCE_GNB (0)
#define GTPV1U_MSG_FROM_UPF (1)


typedef struct nr_gtpv1u_teid_data_s {
  /* UE identifier for oaisim stack */
  module_id_t     gnb_id;
  rnti_t          ue_id;
  pdusessionid_t  pdu_session_id;
} nr_gtpv1u_teid_data_t;


typedef struct nr_gtpv1u_bearer_s {
  /* TEID used in dl and ul */
  teid_t          teid_gNB;                ///< gNB TEID
  uintptr_t       teid_gNB_stack_session;  ///< gNB TEID
  teid_t          teid_upf;                ///< Remote TEID
  in_addr_t       upf_ip_addr;
  struct in6_addr upf_ip6_addr;
  teid_t          teid_tgNB;
  in_addr_t       tgnb_ip_addr;				///< target gNB ipv4
  struct in6_addr tgnb_ip6_addr;				///< target gNB ipv6
  tcp_udp_port_t  port;
  //NwGtpv1uStackSessionHandleT stack_session;
  bearer_state_t state;
} nr_gtpv1u_bearer_t;

typedef struct nr_gtpv1u_ue_data_s {
  /* UE identifier for oaisim stack */
  rnti_t   ue_id;

  /* Unique identifier used between PDCP and GTP-U to distinguish UEs */
  uint32_t instance_id;
  int      num_bearers;
  /* Bearer related data.
   * Note that the first LCID available for data is 3 and we fixed the maximum
   * number of e-rab per UE to be (32 [id range]), max RB is 11. The real rb id will 3 + rab_id (3..32).
   */
  nr_gtpv1u_bearer_t bearers[NR_GTPV1U_MAX_BEARERS_ID];

  //RB_ENTRY(gtpv1u_ue_data_s) gtpv1u_ue_node;
} nr_gtpv1u_ue_data_t;

typedef struct nr_gtpv1u_data_s {
  /* nwgtpv1u stack internal data */
  NwGtpv1uStackHandleT  gtpv1u_stack;

  /* RB tree of UEs */
  hash_table_t         *ue_mapping;   // PDCP->GTPV1U
  hash_table_t         *teid_mapping; // GTPV1U -> PDCP

  //RB_HEAD(gtpv1u_ue_map, gtpv1u_ue_data_s) gtpv1u_ue_map_head;
  /* Local IP address to use */
  in_addr_t             gnb_ip_address_for_NGu_up;
  /* UDP internal data */
  //udp_data_t            udp_data;

  uint16_t              seq_num;
  uint8_t               restart_counter;

} nr_gtpv1u_data_t;


#endif /* GTPV1U_GNB_DEFS_H_ */
