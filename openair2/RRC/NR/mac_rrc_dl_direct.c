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

#include "nr_rrc_defs.h"

#include "mac_rrc_dl.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_rrc_dl_handler.h"

static void f1_setup_response_direct(sctp_assoc_t assoc_id, const f1ap_setup_resp_t *resp)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d\n", assoc_id);
  f1_setup_response(resp);
}

static void f1_setup_failure_direct(sctp_assoc_t assoc_id, const f1ap_setup_failure_t *fail)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d\n", assoc_id);
  f1_setup_failure(fail);
}

static void ue_context_setup_request_direct(sctp_assoc_t assoc_id, const f1ap_ue_context_setup_t *req)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d\n", assoc_id);
  ue_context_setup_request(req);
}

static void ue_context_modification_request_direct(sctp_assoc_t assoc_id, const f1ap_ue_context_modif_req_t *req)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d\n", assoc_id);
  ue_context_modification_request(req);
}

static void ue_context_modification_confirm_direct(sctp_assoc_t assoc_id, const f1ap_ue_context_modif_confirm_t *confirm)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d\n", assoc_id);
  ue_context_modification_confirm(confirm);
}

static void ue_context_modification_refuse_direct(sctp_assoc_t assoc_id, const f1ap_ue_context_modif_refuse_t *refuse)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d\n", assoc_id);
  ue_context_modification_refuse(refuse);
}

static void ue_context_release_command_direct(sctp_assoc_t assoc_id, const f1ap_ue_context_release_cmd_t *cmd)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d\n", assoc_id);
  ue_context_release_command(cmd);
}

static void dl_rrc_message_transfer_direct(sctp_assoc_t assoc_id, const f1ap_dl_rrc_message_t *dl_rrc)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d\n", assoc_id);
  dl_rrc_message_transfer(dl_rrc);
}

void mac_rrc_dl_direct_init(nr_mac_rrc_dl_if_t *mac_rrc)
{
  mac_rrc->f1_setup_response = f1_setup_response_direct;
  mac_rrc->f1_setup_failure = f1_setup_failure_direct;
  mac_rrc->ue_context_setup_request = ue_context_setup_request_direct;
  mac_rrc->ue_context_modification_request = ue_context_modification_request_direct;
  mac_rrc->ue_context_modification_confirm = ue_context_modification_confirm_direct;
  mac_rrc->ue_context_modification_refuse = ue_context_modification_refuse_direct;
  mac_rrc->ue_context_release_command = ue_context_release_command_direct;
  mac_rrc->dl_rrc_message_transfer = dl_rrc_message_transfer_direct;
}
