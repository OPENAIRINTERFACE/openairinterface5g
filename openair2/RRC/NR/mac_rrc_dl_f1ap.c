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

#include <stdlib.h>

#include "mac_rrc_dl.h"
#include "nr_rrc_defs.h"

static void ue_context_setup_request_f1ap(const f1ap_ue_context_setup_t *req)
{
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, F1AP_UE_CONTEXT_SETUP_REQ);
  f1ap_ue_context_setup_t *f1ap_msg = &F1AP_UE_CONTEXT_SETUP_REQ(msg);
  *f1ap_msg = *req;
  AssertFatal(req->cu_to_du_rrc_information == NULL, "cu_to_du_rrc_information not supported yet\n");
  AssertFatal(req->drbs_to_be_setup == NULL, "drbs_to_be_setup not supported yet\n");
  AssertFatal(req->srbs_to_be_setup == NULL, "drbs_to_be_setup not supported yet\n");
  if (req->rrc_container_length > 0) {
    f1ap_msg->rrc_container = calloc(req->rrc_container_length, sizeof(*f1ap_msg->rrc_container));
    AssertFatal(f1ap_msg->rrc_container != NULL, "out of memory\n");
    f1ap_msg->rrc_container_length = req->rrc_container_length;
    memcpy(f1ap_msg->rrc_container, req->rrc_container, req->rrc_container_length);
  }
  itti_send_msg_to_task(TASK_CU_F1, 0, msg);
}

static void ue_context_modification_request_f1ap(const f1ap_ue_context_modif_req_t *req)
{
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, F1AP_UE_CONTEXT_MODIFICATION_REQ);
  f1ap_ue_context_modif_req_t *f1ap_msg = &F1AP_UE_CONTEXT_MODIFICATION_REQ(msg);
  *f1ap_msg = *req;
  AssertFatal(req->cu_to_du_rrc_information == NULL, "cu_to_du_rrc_information not supported yet\n");
  AssertFatal(req->drbs_to_be_modified_length == 0, "drbs_to_be_modified not supported yet\n");
  if (req->drbs_to_be_setup_length > 0) {
    int n = req->drbs_to_be_setup_length;
    f1ap_msg->drbs_to_be_setup_length = n;
    f1ap_msg->drbs_to_be_setup = calloc(n, sizeof(*f1ap_msg->drbs_to_be_setup));
    AssertFatal(f1ap_msg->drbs_to_be_setup != NULL, "out of memory\n");
    memcpy(f1ap_msg->drbs_to_be_setup, req->drbs_to_be_setup, n * sizeof(*f1ap_msg->drbs_to_be_setup));
  }
  if (req->srbs_to_be_setup_length > 0) {
    int n = req->srbs_to_be_setup_length;
    f1ap_msg->srbs_to_be_setup_length = n;
    f1ap_msg->srbs_to_be_setup = calloc(n, sizeof(*f1ap_msg->srbs_to_be_setup));
    AssertFatal(f1ap_msg->srbs_to_be_setup != NULL, "out of memory\n");
    memcpy(f1ap_msg->srbs_to_be_setup, req->srbs_to_be_setup, n * sizeof(*f1ap_msg->srbs_to_be_setup));
  }
  if (req->rrc_container_length > 0) {
    f1ap_msg->rrc_container = calloc(req->rrc_container_length, sizeof(*f1ap_msg->rrc_container));
    AssertFatal(f1ap_msg->rrc_container != NULL, "out of memory\n");
    f1ap_msg->rrc_container_length = req->rrc_container_length;
    memcpy(f1ap_msg->rrc_container, req->rrc_container, req->rrc_container_length);
  }
  itti_send_msg_to_task(TASK_CU_F1, 0, msg);
}

static void ue_context_release_command_f1ap(const f1ap_ue_context_release_cmd_t *cmd)
{
  MessageDef *message_p = itti_alloc_new_message (TASK_RRC_GNB, 0, F1AP_UE_CONTEXT_RELEASE_CMD);
  f1ap_ue_context_release_cmd_t *msg = &F1AP_UE_CONTEXT_RELEASE_CMD(message_p);
  *msg = *cmd;
  itti_send_msg_to_task (TASK_CU_F1, 0, message_p);
}

static void dl_rrc_message_transfer_f1ap(module_id_t module_id, const f1ap_dl_rrc_message_t *dl_rrc)
{
  /* TODO call F1AP function directly? no real-time constraint here */

  MessageDef *message_p = itti_alloc_new_message (TASK_RRC_GNB, 0, F1AP_DL_RRC_MESSAGE);
  f1ap_dl_rrc_message_t *msg = &F1AP_DL_RRC_MESSAGE(message_p);
  *msg = *dl_rrc;
  if (dl_rrc->rrc_container) {
    msg->rrc_container = malloc(dl_rrc->rrc_container_length);
    AssertFatal(msg->rrc_container != NULL, "out of memory\n");
    msg->rrc_container_length = dl_rrc->rrc_container_length;
    memcpy(msg->rrc_container, dl_rrc->rrc_container, dl_rrc->rrc_container_length);
  }
  itti_send_msg_to_task (TASK_CU_F1, module_id, message_p);
}

void mac_rrc_dl_f1ap_init(nr_mac_rrc_dl_if_t *mac_rrc)
{
  mac_rrc->ue_context_setup_request = ue_context_setup_request_f1ap;
  mac_rrc->ue_context_modification_request = ue_context_modification_request_f1ap;
  mac_rrc->ue_context_release_command = ue_context_release_command_f1ap;
  mac_rrc->dl_rrc_message_transfer = dl_rrc_message_transfer_f1ap;
}
