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

//WARNING: Do not include this header directly. Use intertask_interface.h instead.

// Messages for NAS logging
MESSAGE_DEF(NAS_DL_EMM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_dl_emm_raw_msg)
MESSAGE_DEF(NAS_UL_EMM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_ul_emm_raw_msg)

MESSAGE_DEF(NAS_DL_EMM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_emm_plain_msg_t,        nas_dl_emm_plain_msg)
MESSAGE_DEF(NAS_UL_EMM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_emm_plain_msg_t,        nas_ul_emm_plain_msg)
MESSAGE_DEF(NAS_DL_EMM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_emm_protected_msg_t,    nas_dl_emm_protected_msg)
MESSAGE_DEF(NAS_UL_EMM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_emm_protected_msg_t,    nas_ul_emm_protected_msg)

MESSAGE_DEF(NAS_DL_ESM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_dl_esm_raw_msg)
MESSAGE_DEF(NAS_UL_ESM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_ul_esm_raw_msg)

MESSAGE_DEF(NAS_DL_ESM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_esm_plain_msg_t,        nas_dl_esm_plain_msg)
MESSAGE_DEF(NAS_UL_ESM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_esm_plain_msg_t,        nas_ul_esm_plain_msg)
MESSAGE_DEF(NAS_DL_ESM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_esm_protected_msg_t,    nas_dl_esm_protected_msg)
MESSAGE_DEF(NAS_UL_ESM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_esm_protected_msg_t,    nas_ul_esm_protected_msg)

/* */
MESSAGE_DEF(NAS_PAGING_IND,                     MESSAGE_PRIORITY_MED,   nas_paging_ind_t,           nas_paging_ind)
MESSAGE_DEF(NAS_PDN_CONNECTIVITY_REQ,           MESSAGE_PRIORITY_MED,   nas_pdn_connectivity_req_t, nas_pdn_connectivity_req)
MESSAGE_DEF(NAS_CONNECTION_ESTABLISHMENT_IND,   MESSAGE_PRIORITY_MED,   nas_conn_est_ind_t,         nas_conn_est_ind)
MESSAGE_DEF(NAS_CONNECTION_ESTABLISHMENT_CNF,   MESSAGE_PRIORITY_MED,   nas_conn_est_cnf_t,         nas_conn_est_cnf)
MESSAGE_DEF(NAS_CONNECTION_RELEASE_IND,         MESSAGE_PRIORITY_MED,   nas_conn_rel_ind_t,         nas_conn_rel_ind)
MESSAGE_DEF(NAS_UPLINK_DATA_IND,                MESSAGE_PRIORITY_MED,   nas_ul_data_ind_t,          nas_ul_data_ind)
MESSAGE_DEF(NAS_DOWNLINK_DATA_REQ,              MESSAGE_PRIORITY_MED,   nas_dl_data_req_t,          nas_dl_data_req)
MESSAGE_DEF(NAS_DOWNLINK_DATA_CNF,              MESSAGE_PRIORITY_MED,   nas_dl_data_cnf_t,          nas_dl_data_cnf)
MESSAGE_DEF(NAS_DOWNLINK_DATA_REJ,              MESSAGE_PRIORITY_MED,   nas_dl_data_rej_t,          nas_dl_data_rej)
MESSAGE_DEF(NAS_RAB_ESTABLISHMENT_REQ,          MESSAGE_PRIORITY_MED,   nas_rab_est_req_t,          nas_rab_est_req)
MESSAGE_DEF(NAS_RAB_ESTABLISHMENT_RESP,         MESSAGE_PRIORITY_MED,   nas_rab_est_rsp_t,          nas_rab_est_rsp)
MESSAGE_DEF(NAS_RAB_RELEASE_REQ,                MESSAGE_PRIORITY_MED,   nas_rab_rel_req_t,          nas_rab_rel_req)

/* NAS layer -> MME app messages */
MESSAGE_DEF(NAS_AUTHENTICATION_PARAM_REQ,       MESSAGE_PRIORITY_MED,   nas_auth_param_req_t,       nas_auth_param_req)

/* MME app -> NAS layer messages */
MESSAGE_DEF(NAS_PDN_CONNECTIVITY_RSP,           MESSAGE_PRIORITY_MED,   nas_pdn_connectivity_rsp_t,  nas_pdn_connectivity_rsp)
MESSAGE_DEF(NAS_PDN_CONNECTIVITY_FAIL,          MESSAGE_PRIORITY_MED,   nas_pdn_connectivity_fail_t, nas_pdn_connectivity_fail)
MESSAGE_DEF(NAS_AUTHENTICATION_PARAM_RSP,       MESSAGE_PRIORITY_MED,   nas_auth_param_rsp_t,        nas_auth_param_rsp)
MESSAGE_DEF(NAS_AUTHENTICATION_PARAM_FAIL,      MESSAGE_PRIORITY_MED,   nas_auth_param_fail_t,       nas_auth_param_fail)

