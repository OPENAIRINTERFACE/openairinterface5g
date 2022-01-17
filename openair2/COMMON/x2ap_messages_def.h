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

/* eNB application layer -> X2AP messages */
/* ITTI LOG messages */
/* ENCODER */
MESSAGE_DEF(X2AP_RESET_REQUST_LOG               , MESSAGE_PRIORITY_MED, IttiMsgText                      , x2ap_reset_request_log)
MESSAGE_DEF(X2AP_RESOURCE_STATUS_RESPONSE_LOG   , MESSAGE_PRIORITY_MED, IttiMsgText                      , x2ap_resource_status_response_log)
MESSAGE_DEF(X2AP_RESOURCE_STATUS_FAILURE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , x2ap_resource_status_failure_log)

/* Messages for X2AP logging */
MESSAGE_DEF(X2AP_SETUP_REQUEST_LOG              , MESSAGE_PRIORITY_MED, IttiMsgText                      , x2ap_setup_request_log)


/* eNB application layer -> X2AP messages */
MESSAGE_DEF(X2AP_REGISTER_ENB_REQ               , MESSAGE_PRIORITY_MED, x2ap_register_enb_req_t          , x2ap_register_enb_req)
MESSAGE_DEF(X2AP_SUBFRAME_PROCESS               , MESSAGE_PRIORITY_MED, x2ap_subframe_process_t          , x2ap_subframe_process)
MESSAGE_DEF(X2AP_RESET_REQ                      , MESSAGE_PRIORITY_MED, x2ap_reset_req_t                 , x2ap_reset_req)
/* X2AP -> eNB application layer messages */
MESSAGE_DEF(X2AP_REGISTER_ENB_CNF               , MESSAGE_PRIORITY_MED, x2ap_register_enb_cnf_t          , x2ap_register_enb_cnf)
MESSAGE_DEF(X2AP_DEREGISTERED_ENB_IND           , MESSAGE_PRIORITY_MED, x2ap_deregistered_enb_ind_t      , x2ap_deregistered_enb_ind)

/* handover messages X2AP <-> RRC */
MESSAGE_DEF(X2AP_SETUP_REQ                      , MESSAGE_PRIORITY_MED, x2ap_setup_req_t                 , x2ap_setup_req)
MESSAGE_DEF(X2AP_SETUP_RESP                     , MESSAGE_PRIORITY_MED, x2ap_setup_resp_t                , x2ap_setup_resp)
MESSAGE_DEF(X2AP_HANDOVER_REQ                   , MESSAGE_PRIORITY_MED, x2ap_handover_req_t              , x2ap_handover_req)
MESSAGE_DEF(X2AP_HANDOVER_REQ_ACK               , MESSAGE_PRIORITY_MED, x2ap_handover_req_ack_t          , x2ap_handover_req_ack)
MESSAGE_DEF(X2AP_HANDOVER_CANCEL                , MESSAGE_PRIORITY_MED, x2ap_handover_cancel_t           , x2ap_handover_cancel)

/* handover messages X2AP <-> S1AP */
MESSAGE_DEF(X2AP_UE_CONTEXT_RELEASE             , MESSAGE_PRIORITY_MED, x2ap_ue_context_release_t        , x2ap_ue_context_release)

/* Senb bearer addition messages X2AP <-> RRC */
MESSAGE_DEF(X2AP_SENB_ADDITION_REQ              , MESSAGE_PRIORITY_MED, x2ap_senb_addition_req_t         , x2ap_senb_addition_req)

/* SGnb bearer addition messages X2AP <-> RRC */
MESSAGE_DEF(X2AP_ENDC_SETUP_REQ                 , MESSAGE_PRIORITY_MED, x2ap_ENDC_setup_req_t            , x2ap_ENDC_setup_req)
MESSAGE_DEF(X2AP_ENDC_SGNB_ADDITION_REQ         , MESSAGE_PRIORITY_MED, x2ap_ENDC_sgnb_addition_req_t    , x2ap_ENDC_sgnb_addition_req)
MESSAGE_DEF(X2AP_ENDC_SGNB_ADDITION_REQ_ACK     , MESSAGE_PRIORITY_MED, x2ap_ENDC_sgnb_addition_req_ACK_t, x2ap_ENDC_sgnb_addition_req_ACK)
MESSAGE_DEF(X2AP_ENDC_SGNB_RECONF_COMPLETE      , MESSAGE_PRIORITY_MED, x2ap_ENDC_reconf_complete_t      , x2ap_ENDC_sgnb_reconf_complete)

/* SGnb UE releases ('request' is for enb starting the process, 'required' for gnb) */
MESSAGE_DEF(X2AP_ENDC_SGNB_RELEASE_REQUEST      , MESSAGE_PRIORITY_MED, x2ap_ENDC_sgnb_release_request_t , x2ap_ENDC_sgnb_release_request)
MESSAGE_DEF(X2AP_ENDC_SGNB_RELEASE_REQUIRED     , MESSAGE_PRIORITY_MED, x2ap_ENDC_sgnb_release_required_t, x2ap_ENDC_sgnb_release_required)

/* ENDC timers' timeout X2AP <-> RRC */
MESSAGE_DEF(X2AP_ENDC_DC_PREP_TIMEOUT           , MESSAGE_PRIORITY_MED, x2ap_ENDC_dc_prep_timeout_t      , x2ap_ENDC_dc_prep_timeout)
MESSAGE_DEF(X2AP_ENDC_DC_OVERALL_TIMEOUT        , MESSAGE_PRIORITY_MED, x2ap_ENDC_dc_overall_timeout_t   , x2ap_ENDC_dc_overall_timeout)
