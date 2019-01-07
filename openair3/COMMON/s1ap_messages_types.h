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

#ifndef S1AP_MESSAGES_TYPES_H_
#define S1AP_MESSAGES_TYPES_H_

#define S1AP_ENB_DEREGISTERED_IND(mSGpTR)   (mSGpTR)->ittiMsg.s1ap_eNB_deregistered_ind
#define S1AP_DEREGISTER_UE_REQ(mSGpTR)      (mSGpTR)->ittiMsg.s1ap_deregister_ue_req
#define S1AP_UE_CONTEXT_RELEASE_REQ(mSGpTR) (mSGpTR)->ittiMsg.s1ap_ue_context_release_req
#define S1AP_UE_CONTEXT_RELEASE_COMMAND(mSGpTR) (mSGpTR)->ittiMsg.s1ap_ue_context_release_command
#define S1AP_UE_CONTEXT_RELEASE_COMPLETE(mSGpTR) (mSGpTR)->ittiMsg.s1ap_ue_context_release_complete

typedef struct s1ap_initial_ue_message_s {
  unsigned     eNB_ue_s1ap_id:24;
  uint32_t     mme_ue_s1ap_id;
  cgi_t        e_utran_cgi;
} s1ap_initial_ue_message_t;

typedef struct s1ap_initial_ctxt_setup_req_s {
  unsigned                eNB_ue_s1ap_id:24;
  uint32_t                mme_ue_s1ap_id;

  /* Key eNB */
  uint8_t                 keNB[32];

  ambr_t                  ambr;
  ambr_t                  apn_ambr;

  /* EPS bearer ID */
  unsigned                ebi:4;

  /* QoS */
  qci_t                   qci;
  priority_level_t        prio_level;
  pre_emp_vulnerability_t pre_emp_vulnerability;
  pre_emp_capability_t    pre_emp_capability;

  /* S-GW TEID for user-plane */
  Teid_t                  teid;
  /* S-GW IP address for User-Plane */
  ip_address_t            s_gw_address;
} s1ap_initial_ctxt_setup_req_t;

typedef struct s1ap_ue_cap_ind_s {
  unsigned  eNB_ue_s1ap_id:24;
  uint32_t  mme_ue_s1ap_id;
  uint8_t   radio_capabilities[100];
  uint32_t  radio_capabilities_length;
} s1ap_ue_cap_ind_t;

#define S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE 20
typedef struct s1ap_eNB_deregistered_ind_s {
  uint8_t  nb_ue_to_deregister;
  uint32_t mme_ue_s1ap_id[S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE];
} s1ap_eNB_deregistered_ind_t;

typedef struct s1ap_deregister_ue_req_s {
  uint32_t mme_ue_s1ap_id;
} s1ap_deregister_ue_req_t;

typedef struct s1ap_ue_context_release_req_s {
  uint32_t mme_ue_s1ap_id;
} s1ap_ue_context_release_req_t;

typedef struct s1ap_ue_context_release_command_s {
  uint32_t mme_ue_s1ap_id;
} s1ap_ue_context_release_command_t;

typedef struct s1ap_ue_context_release_complete_s {
  uint32_t mme_ue_s1ap_id;
} s1ap_ue_context_release_complete_t;

#endif /* S1AP_MESSAGES_TYPES_H_ */
