/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/* gNB_CUUP application layer -> E1AP messages */
MESSAGE_DEF(E1AP_REGISTER_REQ, MESSAGE_PRIORITY_MED, e1ap_register_req_t, e1ap_register_req)

/* E1AP -> RRC to inform about lost connection */
MESSAGE_DEF(E1AP_LOST_CONNECTION, MESSAGE_PRIORITY_MED, e1ap_lost_connection_t, e1ap_lost_connection)

/* E1AP Interface Management Messages */
/* GNB-CU-UP E1 Setup Request: gNB-CU-UP -> gNB-CU-CP */
MESSAGE_DEF(E1AP_SETUP_REQ  , MESSAGE_PRIORITY_MED , e1ap_setup_req_t , e1ap_setup_req)
/* GNB-CU-UP E1 Setup Response: gNB-CU-CP -> gNB-CU-UP */
MESSAGE_DEF(E1AP_SETUP_RESP , MESSAGE_PRIORITY_MED, e1ap_setup_resp_t , e1ap_setup_resp)
/* GNB-CU-UP E1 Setup Failure: gNB-CU-CP -> gNB-CU-UP */
MESSAGE_DEF(E1AP_SETUP_FAIL, MESSAGE_PRIORITY_MED, e1ap_setup_fail_t, e1ap_setup_fail)

/* E1AP Bearer Context Management Procedures */
/* E1AP Bearer Context Setup Request: gNB-CU-CP -> gNB-CU-UP */
MESSAGE_DEF(E1AP_BEARER_CONTEXT_SETUP_REQ , MESSAGE_PRIORITY_MED , e1ap_bearer_setup_req_t , e1ap_bearer_setup_req)
/* E1AP Bearer Context Setup Response: gNB-CU-UP -> gNB-CU-CP */
MESSAGE_DEF(E1AP_BEARER_CONTEXT_SETUP_RESP , MESSAGE_PRIORITY_MED , e1ap_bearer_setup_resp_t , e1ap_bearer_setup_resp)
/* E1AP Bearer Context Setup Failure: gNB-CU-UP -> gNB-CU-CP */
MESSAGE_DEF(E1AP_BEARER_CONTEXT_SETUP_FAILURE , MESSAGE_PRIORITY_MED , e1ap_bearer_context_setup_failure_t , e1ap_bearer_setup_fail)
/* E1AP Bearer Context Modification Request: gNB-CU-CP -> gNB-CU-UP */
MESSAGE_DEF(E1AP_BEARER_CONTEXT_MODIFICATION_REQ , MESSAGE_PRIORITY_MED , e1ap_bearer_mod_req_t , e1ap_bearer_mod_req)
/* E1AP Bearer Context Modification Response: gNB-CU-UP -> gNB-CU-CP */
MESSAGE_DEF(E1AP_BEARER_CONTEXT_MODIFICATION_RESP, MESSAGE_PRIORITY_MED, e1ap_bearer_modif_resp_t, e1ap_bearer_modif_resp)
/* E1AP Bearer Context Modification Failure: gNB-CU-UP -> gNB-CU-CP */
MESSAGE_DEF(E1AP_BEARER_CONTEXT_MODIFICATION_FAIL, MESSAGE_PRIORITY_MED, e1ap_bearer_context_mod_failure_t, e1ap_bearer_modif_fail)
/* E1AP Bearer Context Modification Required: gNB-CU-UP -> gNB-CU-CP */
MESSAGE_DEF(E1AP_BEARER_CONTEXT_MODIFICATION_REQUIRED, MESSAGE_PRIORITY_MED, e1ap_bearer_mod_required_t, e1ap_bearer_mod_required)
/* E1AP Bearer Context Modification Confirm: gNB-CU-CP -> gNB-CU-UP */
MESSAGE_DEF(E1AP_BEARER_CONTEXT_MODIFICATION_CONFIRM, MESSAGE_PRIORITY_MED, e1ap_bearer_mod_confirm_t, e1ap_bearer_mod_confirm)
/* E1AP Bearer Context Release Request: gNB-CU-CP -> gNB-CU-UP */
MESSAGE_DEF(E1AP_BEARER_CONTEXT_RELEASE_CMD, MESSAGE_PRIORITY_MED, e1ap_bearer_release_cmd_t, e1ap_bearer_release_cmd)
/* E1AP Bearer Context Release Response: gNB-CU-UP -> gNB-CU-CP */
MESSAGE_DEF(E1AP_BEARER_CONTEXT_RELEASE_CPLT, MESSAGE_PRIORITY_MED, e1ap_bearer_release_cplt_t, e1ap_bearer_release_cplt)
