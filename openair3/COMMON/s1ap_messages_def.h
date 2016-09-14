//WARNING: Do not include this header directly. Use intertask_interface.h instead.

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
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_LOG    , MESSAGE_PRIORITY_MED, IttiMsgText                      , s1ap_ue_context_release_log)

MESSAGE_DEF(S1AP_UE_CAPABILITIES_IND       , MESSAGE_PRIORITY_MED, s1ap_ue_cap_ind_t                , s1ap_ue_cap_ind)
MESSAGE_DEF(S1AP_ENB_DEREGISTERED_IND      , MESSAGE_PRIORITY_MED, s1ap_eNB_deregistered_ind_t      , s1ap_eNB_deregistered_ind)
MESSAGE_DEF(S1AP_DEREGISTER_UE_REQ         , MESSAGE_PRIORITY_MED, s1ap_deregister_ue_req_t         , s1ap_deregister_ue_req)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_REQ    , MESSAGE_PRIORITY_MED, s1ap_ue_context_release_req_t    , s1ap_ue_context_release_req)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMMAND, MESSAGE_PRIORITY_MED, s1ap_ue_context_release_command_t, s1ap_ue_context_release_command)
MESSAGE_DEF(S1AP_UE_CONTEXT_RELEASE_COMPLETE, MESSAGE_PRIORITY_MED, s1ap_ue_context_release_complete_t, s1ap_ue_context_release_complete)
