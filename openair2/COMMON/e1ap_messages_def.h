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

/* gNB_CUUP application layer -> E1AP messages */
MESSAGE_DEF(E1AP_REGISTER_REQ, MESSAGE_PRIORITY_MED, e1ap_register_req_t, e1ap_register_req)

/* E1AP messages -> RRC (CU-CP) */
MESSAGE_DEF(E1AP_SETUP_REQ  , MESSAGE_PRIORITY_MED , e1ap_setup_req_t , e1ap_setup_req)

/* E1AP -> eNB_DU or eNB_CU_RRC -> E1AP application layer messages */
MESSAGE_DEF(E1AP_SETUP_RESP , MESSAGE_PRIORITY_MED, e1ap_setup_resp_t , e1ap_setup_resp)

MESSAGE_DEF(E1AP_BEARER_CONTEXT_SETUP_REQ , MESSAGE_PRIORITY_MED , e1ap_bearer_setup_req_t , e1ap_bearer_setup_req)

MESSAGE_DEF(E1AP_BEARER_CONTEXT_SETUP_RESP , MESSAGE_PRIORITY_MED , e1ap_bearer_setup_resp_t , e1ap_bearer_setup_resp)

MESSAGE_DEF(E1AP_BEARER_CONTEXT_MODIFICATION_REQ , MESSAGE_PRIORITY_MED , e1ap_bearer_setup_req_t , e1ap_bearer_mod_req)
MESSAGE_DEF(E1AP_BEARER_CONTEXT_MODIFICATION_RESP, MESSAGE_PRIORITY_MED, e1ap_bearer_modif_resp_t, e1ap_bearer_modif_resp)

MESSAGE_DEF(E1AP_BEARER_CONTEXT_RELEASE_CMD, MESSAGE_PRIORITY_MED, e1ap_bearer_release_cmd_t, e1ap_bearer_release_cmd)
MESSAGE_DEF(E1AP_BEARER_CONTEXT_RELEASE_CPLT, MESSAGE_PRIORITY_MED, e1ap_bearer_release_cplt_t, e1ap_bearer_release_cplt)
