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
 *      conmnc_digit_lengtht@openairinterface.org
 */

#include "cucp_cuup_if.h"
#include <arpa/inet.h>

#include "platform_types.h"
#include "nr_rrc_defs.h"

#include "nr_rrc_proto.h"
#include "nr_rrc_extern.h"
#include "cucp_cuup_if.h"
#include "openair2/COMMON/e1ap_messages_types.h"
#include "UTIL/OSA/osa_defs.h"
#include <openair2/RRC/NR/rrc_gNB_UE_context.h>
#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;

static void cucp_cuup_bearer_context_setup_e1ap(e1ap_bearer_setup_req_t *const req, instance_t instance) {
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instance)], req->rnti);
  protocol_ctxt_t ctxt = {0};
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, GNB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0, 0);

  fill_DRB_configList(&ctxt, ue_context_p);
  MessageDef *msg_p = itti_alloc_new_message(TASK_CUCP_E1, instance, E1AP_BEARER_CONTEXT_SETUP_REQ);
  e1ap_bearer_setup_req_t *bearer_req = &E1AP_BEARER_CONTEXT_SETUP_REQ(msg_p);
  memcpy(bearer_req, req, sizeof(e1ap_bearer_setup_req_t));

  itti_send_msg_to_task (TASK_CUCP_E1, instance, msg_p);
}

static void cucp_cuup_bearer_context_mod_e1ap(e1ap_bearer_setup_req_t *const req, instance_t instance) {
  MessageDef *msg = itti_alloc_new_message(TASK_CUCP_E1, instance, E1AP_BEARER_CONTEXT_MODIFICATION_REQ);
  e1ap_bearer_setup_req_t *req_msg = &E1AP_BEARER_CONTEXT_SETUP_REQ(msg);
  memcpy(req_msg, req, sizeof(*req));
  itti_send_msg_to_task(TASK_CUCP_E1, instance, msg);
}

void cucp_cuup_message_transfer_e1ap_init(gNB_RRC_INST *rrc) {
  rrc->cucp_cuup.bearer_context_setup = cucp_cuup_bearer_context_setup_e1ap;
  rrc->cucp_cuup.bearer_context_mod = cucp_cuup_bearer_context_mod_e1ap;
}
