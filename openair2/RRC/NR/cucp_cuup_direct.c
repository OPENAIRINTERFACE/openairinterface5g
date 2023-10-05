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

#include <arpa/inet.h>

#include "cucp_cuup_if.h"
#include "platform_types.h"
#include "nr_rrc_defs.h"

#include "softmodem-common.h"
#include "nr_rrc_proto.h"
#include "nr_rrc_extern.h"
#include "openair2/COMMON/e1ap_messages_types.h"
#include "openair3/SECU/key_nas_deriver.h"

#include "nr_pdcp/nr_pdcp_entity.h"
#include "openair2/LAYER2/nr_pdcp/cucp_cuup_handler.h"
#include <openair2/RRC/NR/rrc_gNB_UE_context.h>
#include "openair3/ocp-gtpu/gtp_itf.h"
#include "rrc_gNB_GTPV1U.h"
#include "common/ran_context.h"
#include "openair2/F1AP/f1ap_common.h"
#include "openair2/E1AP/e1ap_common.h"

extern RAN_CONTEXT_t RC;

static void cucp_cuup_bearer_context_setup_direct(sctp_assoc_t assoc_id, const e1ap_bearer_setup_req_t *req)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d, impossible for integrated CU\n", assoc_id);
  e1_bearer_context_setup(req);
}

void CU_update_UP_DL_tunnel(e1ap_bearer_setup_req_t *const req, instance_t instance, ue_id_t ue_id) {
  for (int i=0; i < req->numPDUSessionsMod; i++) {
    for (int j=0; j < req->pduSessionMod[i].numDRB2Modify; j++) {
      DRB_nGRAN_to_setup_t *drb_p = req->pduSessionMod[i].DRBnGRanModList + j;

      in_addr_t addr = {0};
      memcpy(&addr, &drb_p->DlUpParamList[0].tlAddress, sizeof(in_addr_t));

      GtpuUpdateTunnelOutgoingAddressAndTeid(instance,
                                             (ue_id & 0xFFFF),
                                             (ebi_t)drb_p->id,
                                             addr,
                                             drb_p->DlUpParamList[0].teId);
    }
  }
}

static void cucp_cuup_bearer_context_mod_direct(sctp_assoc_t assoc_id, e1ap_bearer_setup_req_t *const req)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d, impossible for integrated CU\n", assoc_id);
  // only update GTP tunnels if it is really a CU
  if (!NODE_IS_CU(RC.nrrrc[0]->node_type))
    return;
  instance_t gtpInst = getCxt(0)->gtpInst;
  CU_update_UP_DL_tunnel(req, gtpInst, req->gNB_cu_cp_ue_id);
}

void cucp_cuup_message_transfer_direct_init(gNB_RRC_INST *rrc) {
  rrc->cucp_cuup.bearer_context_setup = cucp_cuup_bearer_context_setup_direct;
  rrc->cucp_cuup.bearer_context_mod = cucp_cuup_bearer_context_mod_direct;
}
