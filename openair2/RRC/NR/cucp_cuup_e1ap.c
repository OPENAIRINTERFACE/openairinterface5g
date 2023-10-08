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

#include "cucp_cuup_if.h"
#include "nr_rrc_defs.h"

#include "nr_rrc_proto.h"
#include "nr_rrc_extern.h"
#include "cucp_cuup_if.h"

static void cucp_cuup_bearer_context_setup_e1ap(sctp_assoc_t assoc_id, const e1ap_bearer_setup_req_t *req)
{
  AssertFatal(assoc_id > 0, "illegal assoc_id %d\n", assoc_id);
  MessageDef *msg_p = itti_alloc_new_message(TASK_CUCP_E1, 0, E1AP_BEARER_CONTEXT_SETUP_REQ);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  e1ap_bearer_setup_req_t *bearer_req = &E1AP_BEARER_CONTEXT_SETUP_REQ(msg_p);
  memcpy(bearer_req, req, sizeof(e1ap_bearer_setup_req_t));

  itti_send_msg_to_task (TASK_CUCP_E1, 0, msg_p);
}

static void cucp_cuup_bearer_context_mod_e1ap(sctp_assoc_t assoc_id, const e1ap_bearer_setup_req_t *req)
{
  AssertFatal(assoc_id > 0, "illegal assoc_id %d\n", assoc_id);
  MessageDef *msg = itti_alloc_new_message(TASK_CUCP_E1, 0, E1AP_BEARER_CONTEXT_MODIFICATION_REQ);
  msg->ittiMsgHeader.originInstance = assoc_id;
  e1ap_bearer_setup_req_t *req_msg = &E1AP_BEARER_CONTEXT_SETUP_REQ(msg);
  memcpy(req_msg, req, sizeof(*req));
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
  rrc->cucp_cuup.bearer_context_release = cucp_cuup_bearer_context_release_cmd_e1ap;
}
