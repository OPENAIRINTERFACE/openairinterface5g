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

/*! \file gtpv1u_eNB_task.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef GTPV1U_ENB_TASK_H_
#define GTPV1U_ENB_TASK_H_

typedef struct gtpv1u_data_s {
  /* RB tree of UEs */
  hash_table_t         *ue_mapping;
} gtpv1u_data_t;

#define GTPV1U_BEARER_OFFSET 3
#define GTPV1U_MAX_BEARERS_ID     (max_val_LTE_DRB_Identity - GTPV1U_BEARER_OFFSET)
typedef enum {
  BEARER_DOWN = 0,
  BEARER_IN_CONFIG,
  BEARER_UP,
  BEARER_DL_HANDOVER,
  BEARER_UL_HANDOVER,
  BEARER_MAX,
} bearer_state_t;

typedef struct fixMe_gtpv1u_bearer_s {
  /* TEID used in dl and ul */
  teid_t          teid_eNB;                ///< eNB TEID
  uintptr_t       teid_eNB_stack_session;  ///< eNB TEID
  teid_t          teid_sgw;                ///< Remote TEID
  in_addr_t       sgw_ip_addr;
  struct in6_addr sgw_ip6_addr;
  teid_t          teid_teNB;
  in_addr_t       tenb_ip_addr;       ///< target eNB ipv4
  struct in6_addr tenb_ip6_addr;        ///< target eNB ipv6
  tcp_udp_port_t  port;
  //NwGtpv1uStackSessionHandleT stack_session;
  bearer_state_t state;
} fixMe_gtpv1u_bearer_t;

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
  fixMe_gtpv1u_bearer_t bearers[GTPV1U_MAX_BEARERS_ID];

  //RB_ENTRY(gtpv1u_ue_data_s) gtpv1u_ue_node;
} gtpv1u_ue_data_t;
/*
int
gtpv1u_new_data_req(
  uint8_t enb_id,
  uint8_t ue_id,
  uint8_t rab_id,
  uint8_t *buffer,
  uint32_t buf_len,
  uint32_t buf_offset);*/

int   gtpv1u_eNB_init(void);
void *gtpv1u_eNB_process_itti_msg(void *);
void *gtpv1u_eNB_task(void *args);

int
gtpv1u_create_x2u_tunnel(
  const instance_t instanceP,
  const gtpv1u_enb_create_x2u_tunnel_req_t   *const create_tunnel_req_pP,
  gtpv1u_enb_create_x2u_tunnel_resp_t *const create_tunnel_resp_pP);

int
gtpv1u_create_s1u_tunnel(
  const instance_t instanceP,
  const gtpv1u_enb_create_tunnel_req_t   *const create_tunnel_req_pP,
  gtpv1u_enb_create_tunnel_resp_t *const create_tunnel_resp_pP);

int
gtpv1u_update_s1u_tunnel(
  const instance_t                              instanceP,
  const gtpv1u_enb_create_tunnel_req_t *const  create_tunnel_req_pP,
  const rnti_t                                  prior_rnti);

int gtpv1u_delete_x2u_tunnel(
  const instance_t                             instanceP,
  const gtpv1u_enb_delete_tunnel_req_t *const req_pP);
#endif /* GTPV1U_ENB_TASK_H_ */
