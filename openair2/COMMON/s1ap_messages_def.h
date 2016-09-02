/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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

/* S1AP -> RRC messages */
MESSAGE_DEF(S1AP_DOWNLINK_NAS              , MESSAGE_PRIORITY_MED, s1ap_downlink_nas_t              , s1ap_downlink_nas )
MESSAGE_DEF(S1AP_INITIAL_CONTEXT_SETUP_REQ , MESSAGE_PRIORITY_MED, s1ap_initial_context_setup_req_t , s1ap_initial_context_setup_req )
MESSAGE_DEF(S1AP_UE_CTXT_MODIFICATION_REQ  , MESSAGE_PRIORITY_MED, s1ap_ue_ctxt_modification_req_t  , s1ap_ue_ctxt_modification_req)
MESSAGE_DEF(S1AP_PAGING_IND                , MESSAGE_PRIORITY_MED, s1ap_paging_ind_t                , s1ap_paging_ind )
MESSAGE_DEF(S1AP_E_RAB_SETUP_REQ            , MESSAGE_PRIORITY_MED, s1ap_e_rab_setup_req_t        , s1ap_e_rab_setup_req )
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMMAND, MESSAGE_PRIORITY_MED, s1ap_ue_release_command_t        , s1ap_ue_release_command)

/* S1AP <-> RRC messages (can be initiated either by MME or eNB) */
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_REQ    , MESSAGE_PRIORITY_MED, s1ap_ue_release_req_t            , s1ap_ue_release_req)
