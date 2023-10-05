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
 * Author and copyright: Laurent Thomas, open-cells.com
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

#include <arpa/inet.h>
#include "e1ap_api.h"

#include "nr_pdcp/nr_pdcp_entity.h"
#include "openair2/RRC/NR/cucp_cuup_if.h"
#include "openair2/RRC/LTE/MESSAGES/asn1_msg.h"
#include "openair3/SECU/key_nas_deriver.h"
#include "openair3/ocp-gtpu/gtp_itf.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "e1ap_asnc.h"
#include "e1ap_common.h"
#include "e1ap.h"

void CUUP_process_bearer_release_command(instance_t instance, e1ap_bearer_release_cmd_t *const cmd)
{
  e1ap_upcp_inst_t *inst = getCxtE1(instance);
  AssertFatal(inst, "");
  newGtpuDeleteAllTunnels(inst->gtpInstN3, cmd->gNB_cu_up_ue_id);
  newGtpuDeleteAllTunnels(inst->gtpInstF1U, cmd->gNB_cu_up_ue_id);
  e1apCUUP_send_BEARER_CONTEXT_RELEASE_COMPLETE(inst->cuup.assoc_id, cmd);
}
