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

/*
 * ngap_messages_def.h
 *
 *  Created on: 2020
 *      Author: Yoshio INOUE, Masayuki HARADA
 *      Email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 */

/* Messages for NGAP logging */
MESSAGE_DEF(NGAP_UPLINK_NAS_LOG            , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_uplink_nas_log)
MESSAGE_DEF(NGAP_UE_CAPABILITY_IND_LOG     , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_ue_capability_ind_log)
MESSAGE_DEF(NGAP_INITIAL_CONTEXT_SETUP_LOG , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_initial_context_setup_log)
MESSAGE_DEF(NGAP_NAS_NON_DELIVERY_IND_LOG  , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_nas_non_delivery_ind_log)
MESSAGE_DEF(NGAP_DOWNLINK_NAS_LOG          , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_downlink_nas_log)
MESSAGE_DEF(NGAP_S1_SETUP_LOG              , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_s1_setup_log)
MESSAGE_DEF(NGAP_INITIAL_UE_MESSAGE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_initial_ue_message_log)
MESSAGE_DEF(NGAP_UE_CONTEXT_RELEASE_REQ_LOG, MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_ue_context_release_req_log)
MESSAGE_DEF(NGAP_UE_CONTEXT_RELEASE_COMMAND_LOG, MESSAGE_PRIORITY_MED, IttiMsgText                  , ngap_ue_context_release_command_log)
MESSAGE_DEF(NGAP_UE_CONTEXT_RELEASE_COMPLETE_LOG, MESSAGE_PRIORITY_MED, IttiMsgText                 , ngap_ue_context_release_complete_log)
MESSAGE_DEF(NGAP_UE_CONTEXT_RELEASE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_ue_context_release_log)
MESSAGE_DEF(NGAP_PDUSESSION_SETUP_REQUEST_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_pdusession_setup_request_log)
MESSAGE_DEF(NGAP_PDUSESSION_SETUP_RESPONSE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_pdusession_setup_response_log)
MESSAGE_DEF(NGAP_PDUSESSION_MODIFY_REQUEST_LOG     , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_pdusession_modify_request_log)
MESSAGE_DEF(NGAP_PDUSESSION_MODIFY_RESPONSE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_pdusession_modify_response_log)
MESSAGE_DEF(NGAP_PAGING_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_paging_log)
MESSAGE_DEF(NGAP_PDUSESSION_RELEASE_REQUEST_LOG   , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_pdusession_release_request_log)
MESSAGE_DEF(NGAP_PDUSESSION_RELEASE_RESPONSE_LOG  , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_pdusession_release_response_log)
MESSAGE_DEF(NGAP_ERROR_INDICATION_LOG        , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_error_indication_log)
MESSAGE_DEF(NGAP_PATH_SWITCH_REQ_LOG         , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_path_switch_req_log)
MESSAGE_DEF(NGAP_PATH_SWITCH_REQ_ACK_LOG     , MESSAGE_PRIORITY_MED, IttiMsgText                      , ngap_path_switch_req_ack_log)

/* gNB application layer -> NGAP messages */
MESSAGE_DEF(NGAP_REGISTER_GNB_REQ          , MESSAGE_PRIORITY_MED, ngap_register_gnb_req_t          , ngap_register_gnb_req)

/* NGAP -> gNB application layer messages */
MESSAGE_DEF(NGAP_REGISTER_GNB_CNF          , MESSAGE_PRIORITY_MED, ngap_register_gnb_cnf_t          , ngap_register_gnb_cnf)
MESSAGE_DEF(NGAP_DEREGISTERED_GNB_IND      , MESSAGE_PRIORITY_MED, ngap_deregistered_gnb_ind_t      , ngap_deregistered_gnb_ind)

/* RRC -> NGAP messages */
MESSAGE_DEF(NGAP_NAS_FIRST_REQ             , MESSAGE_PRIORITY_MED, ngap_nas_first_req_t             , ngap_nas_first_req)
MESSAGE_DEF(NGAP_UPLINK_NAS                , MESSAGE_PRIORITY_MED, ngap_uplink_nas_t                , ngap_uplink_nas)
MESSAGE_DEF(NGAP_UE_CAPABILITIES_IND       , MESSAGE_PRIORITY_MED, ngap_ue_cap_info_ind_t           , ngap_ue_cap_info_ind)
MESSAGE_DEF(NGAP_INITIAL_CONTEXT_SETUP_RESP, MESSAGE_PRIORITY_MED, ngap_initial_context_setup_resp_t, ngap_initial_context_setup_resp)
MESSAGE_DEF(NGAP_INITIAL_CONTEXT_SETUP_FAIL, MESSAGE_PRIORITY_MED, ngap_initial_context_setup_fail_t, ngap_initial_context_setup_fail)
MESSAGE_DEF(NGAP_NAS_NON_DELIVERY_IND      , MESSAGE_PRIORITY_MED, ngap_nas_non_delivery_ind_t      , ngap_nas_non_delivery_ind)
MESSAGE_DEF(NGAP_UE_CONTEXT_RELEASE_RESP   , MESSAGE_PRIORITY_MED, ngap_ue_release_resp_t           , ngap_ue_release_resp)
MESSAGE_DEF(NGAP_UE_CONTEXT_RELEASE_COMPLETE, MESSAGE_PRIORITY_MED, ngap_ue_release_complete_t      , ngap_ue_release_complete)
MESSAGE_DEF(NGAP_UE_CTXT_MODIFICATION_RESP , MESSAGE_PRIORITY_MED, ngap_ue_ctxt_modification_resp_t , ngap_ue_ctxt_modification_resp)
MESSAGE_DEF(NGAP_UE_CTXT_MODIFICATION_FAIL , MESSAGE_PRIORITY_MED, ngap_ue_ctxt_modification_fail_t , ngap_ue_ctxt_modification_fail)
MESSAGE_DEF(NGAP_PDUSESSION_SETUP_RESP          , MESSAGE_PRIORITY_MED, ngap_pdusession_setup_resp_t          , ngap_pdusession_setup_resp)
MESSAGE_DEF(NGAP_PDUSESSION_SETUP_REQUEST_FAIL  , MESSAGE_PRIORITY_MED, ngap_pdusession_setup_req_fail_t      , ngap_pdusession_setup_request_fail)
MESSAGE_DEF(NGAP_PDUSESSION_MODIFY_RESP          , MESSAGE_PRIORITY_MED, ngap_pdusession_modify_resp_t          , ngap_pdusession_modify_resp)
MESSAGE_DEF(NGAP_PDUSESSION_RELEASE_RESPONSE    , MESSAGE_PRIORITY_MED, ngap_pdusession_release_resp_t        , ngap_pdusession_release_resp)
MESSAGE_DEF(NGAP_PATH_SWITCH_REQ           , MESSAGE_PRIORITY_MED, ngap_path_switch_req_t           , ngap_path_switch_req)
MESSAGE_DEF(NGAP_PATH_SWITCH_REQ_ACK       , MESSAGE_PRIORITY_MED, ngap_path_switch_req_ack_t       , ngap_path_switch_req_ack)
MESSAGE_DEF(NGAP_PDUSESSION_MODIFICATION_IND    , MESSAGE_PRIORITY_MED, ngap_pdusession_modification_ind_t    , ngap_pdusession_modification_ind)

/* NGAP -> RRC messages */
MESSAGE_DEF(NGAP_DOWNLINK_NAS              , MESSAGE_PRIORITY_MED, ngap_downlink_nas_t              , ngap_downlink_nas )
MESSAGE_DEF(NGAP_INITIAL_CONTEXT_SETUP_REQ , MESSAGE_PRIORITY_MED, ngap_initial_context_setup_req_t , ngap_initial_context_setup_req )
MESSAGE_DEF(NGAP_UE_CTXT_MODIFICATION_REQ  , MESSAGE_PRIORITY_MED, ngap_ue_ctxt_modification_req_t  , ngap_ue_ctxt_modification_req)
MESSAGE_DEF(NGAP_PAGING_IND                , MESSAGE_PRIORITY_MED, ngap_paging_ind_t                , ngap_paging_ind )
MESSAGE_DEF(NGAP_PDUSESSION_SETUP_REQ            , MESSAGE_PRIORITY_MED, ngap_pdusession_setup_req_t        , ngap_pdusession_setup_req )
MESSAGE_DEF(NGAP_PDUSESSION_MODIFY_REQ           , MESSAGE_PRIORITY_MED, ngap_pdusession_modify_req_t        , ngap_pdusession_modify_req )
MESSAGE_DEF(NGAP_PDUSESSION_RELEASE_COMMAND     , MESSAGE_PRIORITY_MED, ngap_pdusession_release_command_t     , ngap_pdusession_release_command)
MESSAGE_DEF(NGAP_UE_CONTEXT_RELEASE_COMMAND, MESSAGE_PRIORITY_MED, ngap_ue_release_command_t        , ngap_ue_release_command)

/* NGAP <-> RRC messages (can be initiated either by MME or gNB) */
MESSAGE_DEF(NGAP_UE_CONTEXT_RELEASE_REQ    , MESSAGE_PRIORITY_MED, ngap_ue_release_req_t            , ngap_ue_release_req)
