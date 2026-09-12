/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "cuup_cucp_if.h"
#include "e1ap_bearer_context_management.h"
#include "e1ap_messages_types.h"
#include "intertask_interface.h"

static void bearer_setup_response_direct(const e1ap_bearer_setup_resp_t *resp)
{
  MessageDef *msg = itti_alloc_new_message(TASK_MAC_GNB, 0, E1AP_BEARER_CONTEXT_SETUP_RESP);
  e1ap_bearer_setup_resp_t *msg_resp = &E1AP_BEARER_CONTEXT_SETUP_RESP(msg);
  *msg_resp = cp_bearer_context_setup_response(resp);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
}

static void bearer_modif_response_direct(const e1ap_bearer_modif_resp_t *resp)
{
  MessageDef *msg = itti_alloc_new_message(TASK_MAC_GNB, 0, E1AP_BEARER_CONTEXT_MODIFICATION_RESP);
  e1ap_bearer_modif_resp_t *msg_resp = &E1AP_BEARER_CONTEXT_MODIFICATION_RESP(msg);
  *msg_resp = cp_bearer_context_mod_response(resp);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
}

static void bearer_mod_required_direct(const e1ap_bearer_mod_required_t *req)
{
  MessageDef *msg = itti_alloc_new_message(TASK_MAC_GNB, 0, E1AP_BEARER_CONTEXT_MODIFICATION_REQUIRED);
  msg->ittiMsgHeader.originInstance = -1; // means monolithic
  e1ap_bearer_mod_required_t *msg_req = &E1AP_BEARER_CONTEXT_MODIFICATION_REQUIRED(msg);
  *msg_req = cp_bearer_context_mod_required(req);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
}

static void bearer_release_complete_direct(const e1ap_bearer_release_cplt_t *cplt)
{
  MessageDef *msg = itti_alloc_new_message(TASK_MAC_GNB, 0, E1AP_BEARER_CONTEXT_RELEASE_CPLT);
  e1ap_bearer_release_cplt_t *msg_cplt = &E1AP_BEARER_CONTEXT_RELEASE_CPLT(msg);
  *msg_cplt = *cplt;
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
}

void cuup_cucp_init_direct(e1_if_t *iface)
{
  iface->bearer_setup_response = bearer_setup_response_direct;
  iface->bearer_modif_response = bearer_modif_response_direct;
  iface->bearer_mod_required = bearer_mod_required_direct;
  iface->bearer_release_complete = bearer_release_complete_direct;
}
