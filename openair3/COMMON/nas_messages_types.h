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

#include "as_message.h"
#include "nas_message.h"

#ifndef NAS_MESSAGES_TYPES_H_
#define NAS_MESSAGES_TYPES_H_

#define NAS_DL_EMM_RAW_MSG(mSGpTR)                  (mSGpTR)->ittiMsg.nas_dl_emm_raw_msg
#define NAS_UL_EMM_RAW_MSG(mSGpTR)                  (mSGpTR)->ittiMsg.nas_ul_emm_raw_msg

#define NAS_DL_EMM_PROTECTED_MSG(mSGpTR)            (mSGpTR)->ittiMsg.nas_dl_emm_protected_msg
#define NAS_UL_EMM_PROTECTED_MSG(mSGpTR)            (mSGpTR)->ittiMsg.nas_ul_emm_protected_msg
#define NAS_DL_EMM_PLAIN_MSG(mSGpTR)                (mSGpTR)->ittiMsg.nas_dl_emm_plain_msg
#define NAS_UL_EMM_PLAIN_MSG(mSGpTR)                (mSGpTR)->ittiMsg.nas_ul_emm_plain_msg

#define NAS_DL_ESM_RAW_MSG(mSGpTR)                  (mSGpTR)->ittiMsg.nas_dl_esm_raw_msg
#define NAS_UL_ESM_RAW_MSG(mSGpTR)                  (mSGpTR)->ittiMsg.nas_ul_esm_raw_msg

#define NAS_DL_ESM_PROTECTED_MSG(mSGpTR)            (mSGpTR)->ittiMsg.nas_dl_esm_protected_msg
#define NAS_UL_ESM_PROTECTED_MSG(mSGpTR)            (mSGpTR)->ittiMsg.nas_ul_esm_protected_msg
#define NAS_DL_ESM_PLAIN_MSG(mSGpTR)                (mSGpTR)->ittiMsg.nas_dl_esm_plain_msg
#define NAS_UL_ESM_PLAIN_MSG(mSGpTR)                (mSGpTR)->ittiMsg.nas_ul_esm_plain_msg

#define NAS_UL_DATA_IND(mSGpTR)                     (mSGpTR)->ittiMsg.nas_ul_data_ind
#define NAS_DL_DATA_REQ(mSGpTR)                     (mSGpTR)->ittiMsg.nas_dl_data_req
#define NAS_DL_DATA_CNF(mSGpTR)                     (mSGpTR)->ittiMsg.nas_dl_data_cnf
#define NAS_DL_DATA_REJ(mSGpTR)                     (mSGpTR)->ittiMsg.nas_dl_data_rej
#define NAS_PDN_CONNECTIVITY_REQ(mSGpTR)            (mSGpTR)->ittiMsg.nas_pdn_connectivity_req
#define NAS_PDN_CONNECTIVITY_RSP(mSGpTR)            (mSGpTR)->ittiMsg.nas_pdn_connectivity_rsp
#define NAS_PDN_CONNECTIVITY_FAIL(mSGpTR)           (mSGpTR)->ittiMsg.nas_pdn_connectivity_fail
#define NAS_CONN_EST_IND(mSGpTR)                    (mSGpTR)->ittiMsg.nas_conn_est_ind
#define NAS_CONNECTION_ESTABLISHMENT_CNF(mSGpTR)    (mSGpTR)->ittiMsg.nas_conn_est_cnf
#define NAS_BEARER_PARAM(mSGpTR)                    (mSGpTR)->ittiMsg.nas_bearer_param
#define NAS_AUTHENTICATION_REQ(mSGpTR)              (mSGpTR)->ittiMsg.nas_auth_req
#define NAS_AUTHENTICATION_PARAM_REQ(mSGpTR)        (mSGpTR)->ittiMsg.nas_auth_param_req
#define NAS_AUTHENTICATION_PARAM_RSP(mSGpTR)        (mSGpTR)->ittiMsg.nas_auth_param_rsp
#define NAS_AUTHENTICATION_PARAM_FAIL(mSGpTR)       (mSGpTR)->ittiMsg.nas_auth_param_fail

#define NAS_DATA_LENGHT_MAX     256



typedef enum {
  EMM_MSG_HEADER = 1,
  EMM_MSG_ATTACH_REQUEST,
  EMM_MSG_ATTACH_ACCEPT,
  EMM_MSG_ATTACH_COMPLETE,
  EMM_MSG_ATTACH_REJECT,
  EMM_MSG_DETACH_REQUEST,
  EMM_MSG_DETACH_ACCEPT,
  EMM_MSG_TRACKING_AREA_UPDATE_REQUEST,
  EMM_MSG_TRACKING_AREA_UPDATE_ACCEPT,
  EMM_MSG_TRACKING_AREA_UPDATE_COMPLETE,
  EMM_MSG_TRACKING_AREA_UPDATE_REJECT,
  EMM_MSG_EXTENDED_SERVICE_REQUEST,
  EMM_MSG_SERVICE_REQUEST,
  EMM_MSG_SERVICE_REJECT,
  EMM_MSG_GUTI_REALLOCATION_COMMAND,
  EMM_MSG_GUTI_REALLOCATION_COMPLETE,
  EMM_MSG_AUTHENTICATION_REQUEST,
  EMM_MSG_AUTHENTICATION_RESPONSE,
  EMM_MSG_AUTHENTICATION_REJECT,
  EMM_MSG_AUTHENTICATION_FAILURE,
  EMM_MSG_IDENTITY_REQUEST,
  EMM_MSG_IDENTITY_RESPONSE,
  EMM_MSG_SECURITY_MODE_COMMAND,
  EMM_MSG_SECURITY_MODE_COMPLETE,
  EMM_MSG_SECURITY_MODE_REJECT,
  EMM_MSG_EMM_STATUS,
  EMM_MSG_EMM_INFORMATION,
  EMM_MSG_DOWNLINK_NAS_TRANSPORT,
  EMM_MSG_UPLINK_NAS_TRANSPORT,
  EMM_MSG_CS_SERVICE_NOTIFICATION,
} emm_message_ids_t;



typedef enum {
  ESM_MSG_HEADER = 1,
  ESM_MSG_ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST,
  ESM_MSG_ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT,
  ESM_MSG_ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT,
  ESM_MSG_ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST,
  ESM_MSG_ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT,
  ESM_MSG_ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT,
  ESM_MSG_MODIFY_EPS_BEARER_CONTEXT_REQUEST,
  ESM_MSG_MODIFY_EPS_BEARER_CONTEXT_ACCEPT,
  ESM_MSG_MODIFY_EPS_BEARER_CONTEXT_REJECT,
  ESM_MSG_DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST,
  ESM_MSG_DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT,
  ESM_MSG_PDN_CONNECTIVITY_REQUEST,
  ESM_MSG_PDN_CONNECTIVITY_REJECT,
  ESM_MSG_PDN_DISCONNECT_REQUEST,
  ESM_MSG_PDN_DISCONNECT_REJECT,
  ESM_MSG_BEARER_RESOURCE_ALLOCATION_REQUEST,
  ESM_MSG_BEARER_RESOURCE_ALLOCATION_REJECT,
  ESM_MSG_BEARER_RESOURCE_MODIFICATION_REQUEST,
  ESM_MSG_BEARER_RESOURCE_MODIFICATION_REJECT,
  ESM_MSG_ESM_INFORMATION_REQUEST,
  ESM_MSG_ESM_INFORMATION_RESPONSE,
  ESM_MSG_ESM_STATUS,
} esm_message_ids_t;



typedef struct nas_raw_msg_s {
  uint32_t                        lenght;
  uint8_t                         data[NAS_DATA_LENGHT_MAX];
} nas_raw_msg_t;



typedef struct nas_emm_plain_msg_s {
  emm_message_ids_t               present;
  EMM_msg                         choice;

} nas_emm_plain_msg_t;



typedef struct nas_emm_protected_msg_s {
  nas_message_security_header_t   header;
  emm_message_ids_t               present;
  EMM_msg                         choice;
} nas_emm_protected_msg_t;


typedef struct nas_esm_plain_msg_s {
  esm_message_ids_t               present;
  ESM_msg                         choice;

} nas_esm_plain_msg_t;


typedef struct nas_esm_protected_msg_s {
  nas_message_security_header_t   header;
  esm_message_ids_t               present;
  ESM_msg                         choice;
} nas_esm_protected_msg_t;


typedef struct nas_paging_ind_s {

} nas_paging_ind_t;


typedef struct nas_pdn_connectivity_req_s {
  int                    pti;   // nas ref  Identity of the procedure transaction executed to activate the PDN connection entry
  unsigned               ue_id; // nas ref
  char                   imsi[16];
  uint8_t                imsi_length;
  network_qos_t          qos;
  pco_flat_t             pco;
  OctetString            apn;
  OctetString            pdn_addr;
  int                    pdn_type;
  void                  *proc_data;
  int                    request_type;
} nas_pdn_connectivity_req_t;


typedef struct nas_pdn_connectivity_rsp_s {
  int                     pti;   // nas ref  Identity of the procedure transaction executed to activate the PDN connection entry
  unsigned                ue_id; // nas ref
  network_qos_t           qos;
  pco_flat_t              pco;
  OctetString             apn;
  OctetString             pdn_addr;
  int                     pdn_type;
  void                   *proc_data;
  int                     request_type;

  unsigned                eNB_ue_s1ap_id:24;
  uint32_t                mme_ue_s1ap_id;

  /* Key eNB */
  //uint8_t                 keNB[32];

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
  Teid_t                  sgw_s1u_teid;
  /* S-GW IP address for User-Plane */
  ip_address_t            sgw_s1u_address;
} nas_pdn_connectivity_rsp_t;


typedef struct nas_pdn_connectivity_fail_s {
  unsigned                ue_id; // nas ref
} nas_pdn_connectivity_fail_t;


typedef struct nas_conn_est_ind_s {
  nas_establish_ind_t nas;

  /* Transparent message from s1ap to be forwarded to MME_APP or
   * to S1AP if connection establishment is rejected by NAS.
   */
  s1ap_initial_ue_message_t transparent;
} nas_conn_est_ind_t;


typedef nas_establish_rsp_t nas_conn_est_rej_t;


typedef nas_establish_cnf_t nas_conn_est_cnf_t;


typedef struct nas_conn_rel_ind_s {

} nas_conn_rel_ind_t;

typedef ul_info_transfer_ind_t nas_ul_data_ind_t;
typedef dl_info_transfer_req_t nas_dl_data_req_t;
typedef dl_info_transfer_cnf_t nas_dl_data_cnf_t;


typedef struct nas_dl_data_rej_s {
	uint32_t UEid;          /* UE lower layer identifier        */
	  as_nas_info_t nasMsg;   /* Uplink NAS message           */
} nas_dl_data_rej_t;

typedef struct nas_rab_est_req_s {

} nas_rab_est_req_t;


typedef struct nas_rab_est_rsp_s {

} nas_rab_est_rsp_t;


typedef struct nas_rab_rel_req_s {

} nas_rab_rel_req_t;


typedef struct nas_attach_req_s {
  /* TODO: Set the correct size */
  char apn[100];
  char imsi[16];
#define INITIAL_REQUEST (0x1)
  unsigned initial:1;
  s1ap_initial_ue_message_t transparent;
} nas_attach_req_t;


typedef struct nas_auth_req_s {
  /* UE imsi */
  char imsi[16];

#define NAS_FAILURE_OK  0x0
#define NAS_FAILURE_IND 0x1
  unsigned failure:1;
  int cause;
} nas_auth_req_t;


typedef struct nas_auth_resp_s {
  char imsi[16];
} nas_auth_resp_t;

typedef struct nas_auth_param_req_s {
  /* UE identifier */
  uint32_t ue_id;

  /* Imsi of the UE (In case of initial request) */
  char     imsi[16];
  uint8_t  imsi_length;

  /* Indicates whether the procedure corresponds to a new connection or not */
  uint8_t  initial_req:1;

  uint8_t  re_synchronization:1;
  uint8_t  auts[14];
} nas_auth_param_req_t;


typedef struct nas_auth_param_rsp_s {
  /* UE identifier */
  uint32_t ue_id;

  /* For future use: nb of vectors provided */
  uint8_t nb_vectors;

  /* Consider only one E-UTRAN vector for the moment... */
  eutran_vector_t vector;
} nas_auth_param_rsp_t;

typedef struct nas_auth_param_fail_s {
  /* UE identifier */
  uint32_t    ue_id;

  /* S6A mapped to NAS cause */
  nas_cause_t cause;
} nas_auth_param_fail_t;


#endif /* NAS_MESSAGES_TYPES_H_ */
