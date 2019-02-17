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

/* Messages for S1AP logging */
MESSAGE_DEF(S1AP_UPLINK_NAS_LOG            , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_uplink_nas_log)
MESSAGE_DEF(S1AP_UE_CAPABILITY_IND_LOG     , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_ue_capability_ind_log)
MESSAGE_DEF(S1AP_INITIAL_CONTEXT_SETUP_LOG , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_initial_context_setup_log)
MESSAGE_DEF(S1AP_NAS_NON_DELIVERY_IND_LOG  , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_nas_non_delivery_ind_log)
MESSAGE_DEF(S1AP_DOWNLINK_NAS_LOG          , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_downlink_nas_log)
MESSAGE_DEF(S1AP_S1_SETUP_LOG              , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_s1_setup_log)
MESSAGE_DEF(S1AP_INITIAL_UE_MESSAGE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_initial_ue_message_log)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_REQ_LOG, MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_ue_context_release_req_log)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMMAND_LOG, MESSAGE_PRIORITY_MED, IttiMsgText                  , s1ap_ue_context_release_command_log)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMPLETE_LOG, MESSAGE_PRIORITY_MED, IttiMsgText                 , s1ap_ue_context_release_complete_log)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_ue_context_release_log)
MESSAGE_DEF(S1AP_E_RAB_SETUP_REQUEST_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_e_rab_setup_request_log)
MESSAGE_DEF(S1AP_E_RAB_SETUP_RESPONSE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_e_rab_setup_response_log)
MESSAGE_DEF(S1AP_E_RAB_MODIFY_REQUEST_LOG     , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_e_rab_modify_request_log)
MESSAGE_DEF(S1AP_E_RAB_MODIFY_RESPONSE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_e_rab_modify_response_log)
MESSAGE_DEF(S1AP_PAGING_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_paging_log)
MESSAGE_DEF(S1AP_E_RAB_RELEASE_REQUEST_LOG   , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_e_rab_release_request_log)
MESSAGE_DEF(S1AP_E_RAB_RELEASE_RESPONSE_LOG  , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_e_rab_release_response_log)
MESSAGE_DEF(S1AP_ERROR_INDICATION_LOG        , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_error_indication_log)
MESSAGE_DEF(S1AP_PATH_SWITCH_REQ_LOG         , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_path_switch_req_log)
MESSAGE_DEF(S1AP_PATH_SWITCH_REQ_ACK_LOG     , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_path_switch_req_ack_log)

/* eNB application layer -> S1AP messages */
MESSAGE_DEF(S1AP_REGISTER_ENB_REQ          , MESSAGE_PRIORITY_MED, s1ap_register_enb_req_t          , s1ap_register_enb_req)

/* S1AP -> eNB application layer messages */
MESSAGE_DEF(S1AP_REGISTER_ENB_CNF          , MESSAGE_PRIORITY_MED, s1ap_register_enb_cnf_t          , s1ap_register_enb_cnf)
MESSAGE_DEF(S1AP_DEREGISTERED_ENB_IND      , MESSAGE_PRIORITY_MED, s1ap_deregistered_enb_ind_t      , s1ap_deregistered_enb_ind)

/* RRC -> S1AP messages */
MESSAGE_DEF(S1AP_NAS_FIRST_REQ             , MESSAGE_PRIORITY_MED, s1ap_nas_first_req_t             , s1ap_nas_first_req)
MESSAGE_DEF(S1AP_UPLINK_NAS                , MESSAGE_PRIORITY_MED, s1ap_uplink_nas_t                , s1ap_uplink_nas)
MESSAGE_DEF(S1AP_UE_CAPABILITIES_IND       , MESSAGE_PRIORITY_MED, s1ap_ue_cap_info_ind_t           , s1ap_ue_cap_info_ind)
MESSAGE_DEF(S1AP_INITIAL_CONTEXT_SETUP_RESP, MESSAGE_PRIORITY_MED, s1ap_initial_context_setup_resp_t, s1ap_initial_context_setup_resp)
MESSAGE_DEF(S1AP_INITIAL_CONTEXT_SETUP_FAIL, MESSAGE_PRIORITY_MED, s1ap_initial_context_setup_fail_t, s1ap_initial_context_setup_fail)
MESSAGE_DEF(S1AP_NAS_NON_DELIVERY_IND      , MESSAGE_PRIORITY_MED, s1ap_nas_non_delivery_ind_t      , s1ap_nas_non_delivery_ind)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_RESP   , MESSAGE_PRIORITY_MED, s1ap_ue_release_resp_t           , s1ap_ue_release_resp)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMPLETE, MESSAGE_PRIORITY_MED, s1ap_ue_release_complete_t      , s1ap_ue_release_complete)
MESSAGE_DEF(S1AP_UE_CTXT_MODIFICATION_RESP , MESSAGE_PRIORITY_MED, s1ap_ue_ctxt_modification_resp_t , s1ap_ue_ctxt_modification_resp)
MESSAGE_DEF(S1AP_UE_CTXT_MODIFICATION_FAIL , MESSAGE_PRIORITY_MED, s1ap_ue_ctxt_modification_fail_t , s1ap_ue_ctxt_modification_fail)
MESSAGE_DEF(S1AP_E_RAB_SETUP_RESP          , MESSAGE_PRIORITY_MED, s1ap_e_rab_setup_resp_t          , s1ap_e_rab_setup_resp)
MESSAGE_DEF(S1AP_E_RAB_SETUP_REQUEST_FAIL  , MESSAGE_PRIORITY_MED, s1ap_e_rab_setup_req_fail_t      , s1ap_e_rab_setup_request_fail)
MESSAGE_DEF(S1AP_E_RAB_MODIFY_RESP          , MESSAGE_PRIORITY_MED, s1ap_e_rab_modify_resp_t          , s1ap_e_rab_modify_resp)
MESSAGE_DEF(S1AP_E_RAB_RELEASE_RESPONSE    , MESSAGE_PRIORITY_MED, s1ap_e_rab_release_resp_t        , s1ap_e_rab_release_resp)
MESSAGE_DEF(S1AP_PATH_SWITCH_REQ           , MESSAGE_PRIORITY_MED, s1ap_path_switch_req_t           , s1ap_path_switch_req)
MESSAGE_DEF(S1AP_PATH_SWITCH_REQ_ACK       , MESSAGE_PRIORITY_MED, s1ap_path_switch_req_ack_t       , s1ap_path_switch_req_ack)

/* S1AP -> RRC messages */
MESSAGE_DEF(S1AP_DOWNLINK_NAS              , MESSAGE_PRIORITY_MED, s1ap_downlink_nas_t              , s1ap_downlink_nas )
MESSAGE_DEF(S1AP_INITIAL_CONTEXT_SETUP_REQ , MESSAGE_PRIORITY_MED, s1ap_initial_context_setup_req_t , s1ap_initial_context_setup_req )
MESSAGE_DEF(S1AP_UE_CTXT_MODIFICATION_REQ  , MESSAGE_PRIORITY_MED, s1ap_ue_ctxt_modification_req_t  , s1ap_ue_ctxt_modification_req)
MESSAGE_DEF(S1AP_PAGING_IND                , MESSAGE_PRIORITY_MED, s1ap_paging_ind_t                , s1ap_paging_ind )
MESSAGE_DEF(S1AP_E_RAB_SETUP_REQ            , MESSAGE_PRIORITY_MED, s1ap_e_rab_setup_req_t        , s1ap_e_rab_setup_req )
MESSAGE_DEF(S1AP_E_RAB_MODIFY_REQ           , MESSAGE_PRIORITY_MED, s1ap_e_rab_modify_req_t        , s1ap_e_rab_modify_req )
MESSAGE_DEF(S1AP_E_RAB_RELEASE_COMMAND     , MESSAGE_PRIORITY_MED, s1ap_e_rab_release_command_t     , s1ap_e_rab_release_command)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMMAND, MESSAGE_PRIORITY_MED, s1ap_ue_release_command_t        , s1ap_ue_release_command)

/* S1AP <-> RRC messages (can be initiated either by MME or eNB) */
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_REQ    , MESSAGE_PRIORITY_MED, s1ap_ue_release_req_t            , s1ap_ue_release_req)
