/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <netinet/in.h>
#include <netinet/sctp.h>
#include "assertions.h"
#include "cucp_cuup_if.h"
#include "e1ap_messages_types.h"
#include "intertask_interface.h"
#include "nr_rrc_defs.h"
#include "E1AP/lib/e1ap_bearer_context_management.h"

static void cucp_cuup_bearer_context_setup_e1ap(sctp_assoc_t assoc_id, const e1ap_bearer_setup_req_t *req)
{
  AssertFatal(assoc_id > 0, "illegal assoc_id %d\n", assoc_id);
  MessageDef *msg_p = itti_alloc_new_message(TASK_CUCP_E1, 0, E1AP_BEARER_CONTEXT_SETUP_REQ);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  e1ap_bearer_setup_req_t *bearer_req = &E1AP_BEARER_CONTEXT_SETUP_REQ(msg_p);
  *bearer_req = cp_bearer_context_setup_request(req);
  itti_send_msg_to_task (TASK_CUCP_E1, 0, msg_p);
}

static void cucp_cuup_bearer_context_mod_e1ap(sctp_assoc_t assoc_id, const e1ap_bearer_mod_req_t *req)
{
  AssertFatal(assoc_id > 0, "illegal assoc_id %d\n", assoc_id);
  MessageDef *msg = itti_alloc_new_message(TASK_CUCP_E1, 0, E1AP_BEARER_CONTEXT_MODIFICATION_REQ);
  msg->ittiMsgHeader.originInstance = assoc_id;
  e1ap_bearer_mod_req_t *req_msg = &E1AP_BEARER_CONTEXT_MODIFICATION_REQ(msg);
  *req_msg = cp_bearer_context_mod_request(req);
  itti_send_msg_to_task(TASK_CUCP_E1, 0, msg);
}

static void cucp_cuup_bearer_context_mod_confirm_e1ap(sctp_assoc_t assoc_id, const e1ap_bearer_mod_confirm_t *conf)
{
  AssertFatal(assoc_id > 0, "illegal assoc_id %d\n", assoc_id);
  MessageDef *msg = itti_alloc_new_message(TASK_CUCP_E1, 0, E1AP_BEARER_CONTEXT_MODIFICATION_CONFIRM);
  msg->ittiMsgHeader.originInstance = assoc_id;
  e1ap_bearer_mod_confirm_t *conf_msg = &E1AP_BEARER_CONTEXT_MODIFICATION_CONFIRM(msg);
  *conf_msg = cp_bearer_context_mod_confirm(conf);
  itti_send_msg_to_task(TASK_CUCP_E1, 0, msg);
}

static void cucp_cuup_bearer_context_release_cmd_e1ap(sctp_assoc_t assoc_id, const e1ap_bearer_release_cmd_t *cmd)
{
  AssertFatal(assoc_id > 0, "illegal assoc_id %d\n", assoc_id);
  MessageDef *msg = itti_alloc_new_message(TASK_CUCP_E1, 0, E1AP_BEARER_CONTEXT_RELEASE_CMD);
  msg->ittiMsgHeader.originInstance = assoc_id;
  e1ap_bearer_release_cmd_t *cmd_msg = &E1AP_BEARER_CONTEXT_RELEASE_CMD(msg);
  memcpy(cmd_msg, cmd, sizeof(*cmd));
  itti_send_msg_to_task(TASK_CUCP_E1, 0, msg);
}

void cucp_cuup_message_transfer_e1ap_init(gNB_RRC_INST *rrc) {
  rrc->cucp_cuup.bearer_context_setup = cucp_cuup_bearer_context_setup_e1ap;
  rrc->cucp_cuup.bearer_context_mod = cucp_cuup_bearer_context_mod_e1ap;
  rrc->cucp_cuup.bearer_context_mod_confirm = cucp_cuup_bearer_context_mod_confirm_e1ap;
  rrc->cucp_cuup.bearer_context_release = cucp_cuup_bearer_context_release_cmd_e1ap;
}
