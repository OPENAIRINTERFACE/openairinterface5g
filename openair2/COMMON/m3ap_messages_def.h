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

/* eNB application layer -> M3AP messages */
/* ITTI LOG messages */
/* ENCODER */
MESSAGE_DEF(M3AP_RESET_REQUST_LOG               , MESSAGE_PRIORITY_MED, IttiMsgText                      , m3ap_reset_request_log)
MESSAGE_DEF(M3AP_RESOURCE_STATUS_RESPONSE_LOG   , MESSAGE_PRIORITY_MED, IttiMsgText                      , m3ap_resource_status_response_log)
MESSAGE_DEF(M3AP_RESOURCE_STATUS_FAILURE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , m3ap_resource_status_failure_log)

/* Messages for M3AP logging */
MESSAGE_DEF(M3AP_SETUP_REQUEST_LOG              , MESSAGE_PRIORITY_MED, IttiMsgText                      , m3ap_setup_request_log)


/* eNB application layer -> M3AP messages */
MESSAGE_DEF(M3AP_REGISTER_MCE_REQ               , MESSAGE_PRIORITY_MED, m3ap_register_mce_req_t          , m3ap_register_mce_req)
MESSAGE_DEF(M3AP_SUBFRAME_PROCESS               , MESSAGE_PRIORITY_MED, m3ap_subframe_process_t          , m3ap_subframe_process)

/* M3AP -> eNB application layer messages */
MESSAGE_DEF(M3AP_REGISTER_MCE_CNF               , MESSAGE_PRIORITY_MED, m3ap_register_mce_cnf_t          , m3ap_register_mce_cnf)
MESSAGE_DEF(M3AP_DEREGISTERED_MCE_IND           , MESSAGE_PRIORITY_MED, m3ap_deregistered_mce_ind_t      , m3ap_deregistered_mce_ind)

/* handover messages M3AP <-> RRC */
//MESSAGE_DEF(M3AP_HANDOVER_REQ                   , MESSAGE_PRIORITY_MED, m3ap_handover_req_t              , m3ap_handover_req)
//MESSAGE_DEF(M3AP_HANDOVER_REQ_ACK               , MESSAGE_PRIORITY_MED, m3ap_handover_req_ack_t          , m3ap_handover_req_ack)
//MESSAGE_DEF(M3AP_HANDOVER_CANCEL                , MESSAGE_PRIORITY_MED, m3ap_handover_cancel_t           , m3ap_handover_cancel)

/* handover messages M3AP <-> S1AP */
//MESSAGE_DEF(M3AP_UE_CONTEXT_RELEASE             , MESSAGE_PRIORITY_MED, m3ap_ue_context_release_t        , m3ap_ue_context_release)

MESSAGE_DEF(M3AP_MME_SCTP_REQ        , MESSAGE_PRIORITY_MED, m3ap_mme_sctp_req_t       , m3ap_mme_sctp_req)


MESSAGE_DEF(M3AP_MBMS_SESSION_START_REQ      , MESSAGE_PRIORITY_MED  ,m3ap_session_start_req_t         ,m3ap_session_start_req)
MESSAGE_DEF(M3AP_MBMS_SESSION_START_RESP     , MESSAGE_PRIORITY_MED  ,m3ap_session_start_resp_t        ,m3ap_session_start_resp)
MESSAGE_DEF(M3AP_MBMS_SESSION_START_FAILURE  , MESSAGE_PRIORITY_MED  ,m3ap_session_start_failure_t     ,m3ap_session_start_failure)
MESSAGE_DEF(M3AP_MBMS_SESSION_STOP_REQ       , MESSAGE_PRIORITY_MED  ,m3ap_session_stop_req_t          ,m3ap_session_stop_req)
MESSAGE_DEF(M3AP_MBMS_SESSION_STOP_RESP      , MESSAGE_PRIORITY_MED  ,m3ap_session_stop_resp_t         ,m3ap_session_stop_resp)
MESSAGE_DEF(M3AP_MBMS_SESSION_STOP_FAILURE   , MESSAGE_PRIORITY_MED  ,m3ap_session_stop_failure_t      ,m3ap_session_stop_failure)
MESSAGE_DEF(M3AP_ERROR_INDICATION            , MESSAGE_PRIORITY_MED  ,m3ap_error_indication_t               ,m3ap_error_indication)
MESSAGE_DEF(M3AP_RESET          	     , MESSAGE_PRIORITY_MED  ,m3ap_reset_t                          ,m3ap_reset)
MESSAGE_DEF(M3AP_RESET_ACK                   , MESSAGE_PRIORITY_MED  ,m3ap_reset_ack_t                      ,m3ap_reset_ack)
MESSAGE_DEF(M3AP_MBMS_SESSION_UPDATE_REQ     , MESSAGE_PRIORITY_MED  ,m3ap_mbms_session_update_req_t        ,m3ap_mbms_session_update_req)
MESSAGE_DEF(M3AP_MBMS_SESSION_UPDATE_RESP    , MESSAGE_PRIORITY_MED  ,m3ap_mbms_session_update_resp_t       ,m3ap_mbms_session_update_resp)
MESSAGE_DEF(M3AP_MBMS_SESSION_UPDATE_FAILURE , MESSAGE_PRIORITY_MED  ,m3ap_mbms_session_update_failure_t    ,m3ap_mbms_session_update_failure)
MESSAGE_DEF(M3AP_SETUP_REQ		     , MESSAGE_PRIORITY_MED  ,m3ap_setup_req_t                      ,m3ap_setup_req)
MESSAGE_DEF(M3AP_SETUP_RESP	             , MESSAGE_PRIORITY_MED  ,m3ap_setup_resp_t                     ,m3ap_setup_resp)
MESSAGE_DEF(M3AP_SETUP_FAILURE		     , MESSAGE_PRIORITY_MED  ,m3ap_setup_failure_t                  ,m3ap_setup_failure)
MESSAGE_DEF(M3AP_MCE_CONFIGURATION_UPDATE        , MESSAGE_PRIORITY_MED  ,m3ap_mce_configuration_update_t           ,m3ap_mce_configuration_update)
MESSAGE_DEF(M3AP_MCE_CONFIGURATION_UPDATE_ACK    , MESSAGE_PRIORITY_MED  ,m3ap_mce_configuration_update_ack_t       ,m3ap_mce_configuration_update_ack)
MESSAGE_DEF(M3AP_MCE_CONFIGURATION_UPDATE_FAILURE, MESSAGE_PRIORITY_MED  ,m3ap_mce_configuration_update_failure_t   ,m3ap_mce_configuration_update_failure)


