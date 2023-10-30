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
#include "openair2/COMMON/e1ap_messages_types.h"
#include "openair2/LAYER2/nr_pdcp/cucp_cuup_handler.h"

static void cucp_cuup_bearer_context_setup_direct(sctp_assoc_t assoc_id, const e1ap_bearer_setup_req_t *req)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d, impossible for integrated CU\n", assoc_id);
  e1_bearer_context_setup(req);
}

static void cucp_cuup_bearer_context_modif_direct(sctp_assoc_t assoc_id, const e1ap_bearer_setup_req_t *req)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d, impossible for integrated CU\n", assoc_id);
  e1_bearer_context_modif(req);
}

static void cucp_cuup_bearer_context_release_cmd_direct(sctp_assoc_t assoc_id, const e1ap_bearer_release_cmd_t *cmd)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d, impossible for integrated CU\n", assoc_id);
  e1_bearer_release_cmd(cmd);
}

void cucp_cuup_message_transfer_direct_init(gNB_RRC_INST *rrc) {
  rrc->cucp_cuup.bearer_context_setup = cucp_cuup_bearer_context_setup_direct;
  rrc->cucp_cuup.bearer_context_mod = cucp_cuup_bearer_context_modif_direct;
  rrc->cucp_cuup.bearer_context_release = cucp_cuup_bearer_context_release_cmd_direct;
}
