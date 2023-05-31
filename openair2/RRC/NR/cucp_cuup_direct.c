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
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_e1_api.h"
#include <openair2/RRC/NR/rrc_gNB_UE_context.h>
#include "openair3/ocp-gtpu/gtp_itf.h"
#include "rrc_gNB_GTPV1U.h"
#include "common/ran_context.h"
#include "openair2/F1AP/f1ap_common.h"
#include "openair2/E1AP/e1ap_common.h"

extern RAN_CONTEXT_t RC;

void fill_e1ap_bearer_setup_resp(e1ap_bearer_setup_resp_t *resp,
                                 e1ap_bearer_setup_req_t *const req,
                                 instance_t gtpInst,
                                 ue_id_t ue_id,
                                 int remote_port,
                                 in_addr_t my_addr) {

  resp->numPDUSessions = req->numPDUSessions;
  transport_layer_addr_t dummy_address = {0};
  dummy_address.length = 32; // IPv4
  for (int i=0; i < req->numPDUSessions; i++) {
    resp->pduSession[i].numDRBSetup = req->pduSession[i].numDRB2Setup;
    for (int j=0; j < req->pduSession[i].numDRB2Setup; j++) {
      DRB_nGRAN_to_setup_t *drb2Setup = req->pduSession[i].DRBnGRanList + j;
      DRB_nGRAN_setup_t *drbSetup = resp->pduSession[i].DRBnGRanList + j;

      drbSetup->numUpParam = 1;
      drbSetup->UpParamList[0].tlAddress = my_addr;
      drbSetup->UpParamList[0].teId = newGtpuCreateTunnel(gtpInst,
                                                          (ue_id & 0xFFFF),
                                                          drb2Setup->id,
                                                          drb2Setup->id,
                                                          0xFFFF, // We will set the right value from DU answer
                                                          -1, // no qfi
                                                          dummy_address, // We will set the right value from DU answer
                                                          remote_port,
                                                          cu_f1u_data_req,
                                                          NULL);
      drbSetup->id = drb2Setup->id;

      drbSetup->numQosFlowSetup = drb2Setup->numQosFlow2Setup;
      for (int k=0; k < drbSetup->numQosFlowSetup; k++) {
        drbSetup->qosFlows[k].id = drb2Setup->qosFlows[k].id;
      }
    }
  }
}

void CU_update_UP_DL_tunnel(e1ap_bearer_setup_req_t *const req, instance_t instance, ue_id_t ue_id) {
  for (int i=0; i < req->numPDUSessionsMod; i++) {
    for (int j=0; j < req->pduSessionMod[i].numDRB2Modify; j++) {
      DRB_nGRAN_to_setup_t *drb_p = req->pduSessionMod[i].DRBnGRanModList + j;

      transport_layer_addr_t newRemoteAddr;
      newRemoteAddr.length = 32; // IPv4
      memcpy(newRemoteAddr.buffer,
             &drb_p->DlUpParamList[0].tlAddress,
             sizeof(in_addr_t));

      GtpuUpdateTunnelOutgoingAddressAndTeid(instance,
                                             (ue_id & 0xFFFF),
                                             (ebi_t)drb_p->id,
                                             *(in_addr_t*)&newRemoteAddr.buffer,
                                             drb_p->DlUpParamList[0].teId);
    }
  }
}

static int drb_config_gtpu_create(const protocol_ctxt_t *const ctxt_p,
                                  rrc_gNB_ue_context_t *ue_context_p,
                                  e1ap_bearer_setup_req_t *const req,
                                  NR_DRB_ToAddModList_t *DRB_configList,
                                  instance_t instance)
{
  gtpv1u_gnb_create_tunnel_req_t  create_tunnel_req={0};
  gtpv1u_gnb_create_tunnel_resp_t create_tunnel_resp={0};
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  LOG_W(NR_RRC, "recreate existing tunnels, while adding new ones\n");
  for (int i = 0; i < UE->nb_of_pdusessions; i++) {
    rrc_pdu_session_param_t *pdu = UE->pduSession + i;
    create_tunnel_req.pdusession_id[i] = pdu->param.pdusession_id;
    create_tunnel_req.incoming_rb_id[i] = i + 1;
    create_tunnel_req.outgoing_qfi[i] = req->pduSession[i].DRBnGRanList[0].qosFlows[0].id;
    memcpy(&create_tunnel_req.dst_addr[i].buffer, &pdu->param.upf_addr.buffer, sizeof(create_tunnel_req.dst_addr[0].buffer));
    create_tunnel_req.dst_addr[i].length = pdu->param.upf_addr.length;
    create_tunnel_req.outgoing_teid[i] = pdu->param.gtp_teid;
  }
  create_tunnel_req.num_tunnels = UE->nb_of_pdusessions;
  create_tunnel_req.ue_id = UE->rnti;
  int ret = gtpv1u_create_ngu_tunnel(getCxtE1(instance)->gtpInstN3,
                                     &create_tunnel_req,
                                     &create_tunnel_resp,
                                     nr_pdcp_data_req_drb,
                                     sdap_data_req);

  if (ret != 0) {
    LOG_E(NR_RRC,"rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ : gtpv1u_create_ngu_tunnel failed,start to release UE rnti %ld\n",
          create_tunnel_req.ue_id);
    return ret;
  }

  nr_rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(ctxt_p, &create_tunnel_resp, 0);

  uint8_t kRRCenc[16] = {0};
  uint8_t kRRCint[16] = {0};
  uint8_t kUPenc[16] = {0};
  uint8_t kUPint[16] = {0};
  /* Derive the keys from kgnb */
  if (DRB_configList != NULL) {
    nr_derive_key(UP_ENC_ALG, UE->ciphering_algorithm, UE->kgnb, kUPenc);
    nr_derive_key(UP_INT_ALG, UE->integrity_algorithm, UE->kgnb, kUPint);
  }

  nr_derive_key(RRC_ENC_ALG, UE->ciphering_algorithm, UE->kgnb, kRRCenc);
  nr_derive_key(RRC_INT_ALG, UE->integrity_algorithm, UE->kgnb, kRRCint);

  /* Refresh SRBs/DRBs */

  LOG_D(NR_RRC, "Configuring PDCP DRBs for UE %x\n", UE->rnti);
  nr_pdcp_add_drbs(ctxt_p->enb_flag,
                   ctxt_p->rntiMaybeUEid,
                   0,
                   DRB_configList,
                   (UE->integrity_algorithm << 4) | UE->ciphering_algorithm,
                   kUPenc,
                   kUPint,
                   get_softmodem_params()->sa ? UE->masterCellGroup->rlc_BearerToAddModList : NULL);

  return ret;
}

static void cucp_cuup_bearer_context_setup_direct(e1ap_bearer_setup_req_t *const req, instance_t instance, uint8_t xid)
{
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context_by_rnti(RC.nrrrc[instance], req->rnti);
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  protocol_ctxt_t ctxt = {0};
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, GNB_FLAG_YES, UE->rnti, 0, 0, 0);

  fill_DRB_configList(&ctxt, ue_context_p, xid);
  e1ap_bearer_setup_resp_t resp = {0};
  resp.numPDUSessions = req->numPDUSessions;
  for (int i = 0; i < resp.numPDUSessions; ++i) {
    resp.pduSession[i].numDRBSetup = req->pduSession[i].numDRB2Setup;
    for (int j = 0; j < req->pduSession[i].numDRB2Setup; j++) {
      DRB_nGRAN_to_setup_t *req_drb = req->pduSession[i].DRBnGRanList + j;
      DRB_nGRAN_setup_t *resp_drb = resp.pduSession[i].DRBnGRanList + j;
      resp_drb->id = req_drb->id;
      resp_drb->numQosFlowSetup = req_drb->numQosFlow2Setup;
      for (int k = 0; k < resp_drb->numQosFlowSetup; k++)
        resp_drb->qosFlows[k].id = req_drb->qosFlows[k].id;
    }
  }

  gNB_RRC_INST *rrc = RC.nrrrc[ctxt.module_id];
  // GTP tunnel for UL
  int ret = drb_config_gtpu_create(&ctxt, ue_context_p, req, UE->DRB_configList, rrc->e1_inst);
  if (ret < 0) AssertFatal(false, "Unable to configure DRB or to create GTP Tunnel\n");

  // Used to store teids: if monolithic, will simply be NULL
  if(!NODE_IS_CU(RC.nrrrc[ctxt.module_id]->node_type)) {
    // intentionally empty
  } else {
    int remote_port = RC.nrrrc[ctxt.module_id]->eth_params_s.remote_portd;
    in_addr_t my_addr = inet_addr(RC.nrrrc[ctxt.module_id]->eth_params_s.my_addr);
    instance_t gtpInst = getCxt(CUtype, instance)->gtpInst;
    // GTP tunnel for DL
    fill_e1ap_bearer_setup_resp(&resp, req, gtpInst, UE->rnti, remote_port, my_addr);
  }
  // actually, we should receive the corresponding context setup response
  // message at the RRC and always react to this one. So in the following, we
  // just call the corresponding message handler
  prepare_and_send_ue_context_modification_f1(ue_context_p, &resp);
}

static void cucp_cuup_bearer_context_mod_direct(e1ap_bearer_setup_req_t *const req, instance_t instance, uint8_t xid) {
  // only update GTP tunnels if it is really a CU
  if (!NODE_IS_CU(RC.nrrrc[0]->node_type))
    return;
  instance_t gtpInst = getCxt(CUtype, instance)->gtpInst;
  CU_update_UP_DL_tunnel(req, gtpInst, req->rnti);
}

void cucp_cuup_message_transfer_direct_init(gNB_RRC_INST *rrc) {
  rrc->cucp_cuup.bearer_context_setup = cucp_cuup_bearer_context_setup_direct;
  rrc->cucp_cuup.bearer_context_mod = cucp_cuup_bearer_context_mod_direct;
}
