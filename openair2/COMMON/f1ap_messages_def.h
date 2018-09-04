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

/* eNB application layer -> F1AP messages */
MESSAGE_DEF(F1AP_SETUP_REQ          , MESSAGE_PRIORITY_MED, f1ap_setup_req_t          , f1ap_setup_req)

/* F1AP -> eNB application layer messages */
MESSAGE_DEF(F1AP_SETUP_RESP         , MESSAGE_PRIORITY_MED, f1ap_setup_resp_t          , f1ap_setup_resp)
MESSAGE_DEF(F1AP_SETUP_FAILURE         , MESSAGE_PRIORITY_MED, f1ap_setup_failure_t          , f1ap_setup_failure)

/* MAC -> F1AP messages */
MESSAGE_DEF(F1AP_INITIAL_UL_RRC_MESSAGE           , MESSAGE_PRIORITY_MED, f1ap_initial_ul_rrc_message_t             , f1ap_initial_ul_rrc_message)
MESSAGE_DEF(F1AP_UL_RRC_MESSAGE                , MESSAGE_PRIORITY_MED, f1ap_ul_rrc_message_t                , f1ap_ul_rrc_message)
//MESSAGE_DEF(F1AP_INITIAL_CONTEXT_SETUP_RESP, MESSAGE_PRIORITY_MED, f1ap_initial_context_setup_resp_t, f1ap_initial_context_setup_resp)
//MESSAGE_DEF(F1AP_INITIAL_CONTEXT_SETUP_FAILURE, MESSAGE_PRIORITY_MED, f1ap_initial_context_setup_failure_t, f1ap_initial_context_setup_failure)


/* RRC -> F1AP  messages */
MESSAGE_DEF(F1AP_DL_RRC_MESSAGE              , MESSAGE_PRIORITY_MED, f1ap_dl_rrc_message_t              , f1ap_downlink_rrc_message )
//MESSAGE_DEF(F1AP_INITIAL_CONTEXT_SETUP_REQ , MESSAGE_PRIORITY_MED, f1ap_initial_context_setup_req_t , f1ap_initial_context_setup_req )


