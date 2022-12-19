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
#include "UTIL/OSA/osa_defs.h"
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
                                  rrc_gNB_ue_context_t  *ue_context_p,
                                  e1ap_bearer_setup_req_t *const req,
                                  NR_DRB_ToAddModList_t *DRB_configList,
                                  NR_SRB_ToAddModList_t *SRB_configList,
                                  instance_t instance) {

  gtpv1u_gnb_create_tunnel_req_t  create_tunnel_req={0};
  gtpv1u_gnb_create_tunnel_resp_t create_tunnel_resp={0};

  for (int i=0; i < ue_context_p->ue_context.nb_of_pdusessions; i++) {
    pdu_session_param_t *pdu = ue_context_p->ue_context.pduSession + i;
    create_tunnel_req.pdusession_id[i] = pdu->param.pdusession_id;
    create_tunnel_req.incoming_rb_id[i] = i + 1;
    create_tunnel_req.outgoing_qfi[i] = req->pduSession[i].DRBnGRanList[0].qosFlows[0].id;
    memcpy(&create_tunnel_req.dst_addr[i].buffer,
           &pdu->param.upf_addr.buffer,
           sizeof(uint8_t)*20);
    create_tunnel_req.dst_addr[i].length = pdu->param.upf_addr.length;
    create_tunnel_req.outgoing_teid[i] = pdu->param.gtp_teid;
  }
  create_tunnel_req.num_tunnels = ue_context_p->ue_context.nb_of_pdusessions;
  create_tunnel_req.ue_id       = ue_context_p->ue_context.rnti;

  int ret = gtpv1u_create_ngu_tunnel(getCxtE1(instance)->gtpInstN3, &create_tunnel_req, &create_tunnel_resp);

  if (ret != 0) {
    LOG_E(NR_RRC,"rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ : gtpv1u_create_ngu_tunnel failed,start to release UE rnti %ld\n",
          create_tunnel_req.ue_id);
    return ret;
  }

  nr_rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(ctxt_p,
                                               &create_tunnel_resp);

  uint8_t *kRRCenc = NULL;
  uint8_t *kRRCint = NULL;
  uint8_t *kUPenc = NULL;
  uint8_t *kUPint = NULL;
  /* Derive the keys from kgnb */
  if (DRB_configList != NULL) {
    nr_derive_key_up_enc(ue_context_p->ue_context.ciphering_algorithm,
                         ue_context_p->ue_context.kgnb,
                         &kUPenc);
    nr_derive_key_up_int(ue_context_p->ue_context.integrity_algorithm,
                         ue_context_p->ue_context.kgnb,
                         &kUPint);
  }

  nr_derive_key_rrc_enc(ue_context_p->ue_context.ciphering_algorithm,
                        ue_context_p->ue_context.kgnb,
                        &kRRCenc);
  nr_derive_key_rrc_int(ue_context_p->ue_context.integrity_algorithm,
                        ue_context_p->ue_context.kgnb,
                        &kRRCint);
  /* Refresh SRBs/DRBs */

  LOG_D(NR_RRC,"Configuring PDCP DRBs/SRBs for UE %x\n",ue_context_p->ue_context.rnti);

  nr_pdcp_add_srbs(ctxt_p->enb_flag, ctxt_p->rntiMaybeUEid,
                   SRB_configList,
                   (ue_context_p->ue_context.integrity_algorithm << 4)
                   | ue_context_p->ue_context.ciphering_algorithm,
                   kRRCenc,
                   kRRCint);
                   
  nr_pdcp_add_drbs(ctxt_p->enb_flag, ctxt_p->rntiMaybeUEid, 0,
                   DRB_configList,
                   (ue_context_p->ue_context.integrity_algorithm << 4)
                   | ue_context_p->ue_context.ciphering_algorithm,
                   kUPenc,
                   kUPint,
                   get_softmodem_params()->sa ? ue_context_p->ue_context.masterCellGroup->rlc_BearerToAddModList : NULL);
  
  return ret;
}

static NR_SRB_ToAddModList_t **generateSRB2_confList(gNB_RRC_UE_t *ue, NR_SRB_ToAddModList_t *SRB_configList, uint8_t xid)
{
  NR_SRB_ToAddModList_t **SRB_configList2 = NULL;

  SRB_configList2 = &ue->SRB_configList2[xid];
  if (*SRB_configList2 == NULL) {
    *SRB_configList2 = CALLOC(1, sizeof(**SRB_configList2));
    NR_SRB_ToAddMod_t *SRB2_config = CALLOC(1, sizeof(*SRB2_config));
    SRB2_config->srb_Identity = 2;
    asn1cSeqAdd(&(*SRB_configList2)->list, SRB2_config);
    asn1cSeqAdd(&SRB_configList->list, SRB2_config);
  }

  return SRB_configList2;
}
static void cucp_cuup_bearer_context_setup_direct(e1ap_bearer_setup_req_t *const req, instance_t instance) {
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instance)], req->rnti);
  protocol_ctxt_t ctxt = {0};
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, GNB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0, 0);

  fill_DRB_configList(&ctxt, ue_context_p);

  gNB_RRC_INST *rrc = RC.nrrrc[ctxt.module_id];
  // Fixme: xid not random, but almost!
  NR_SRB_ToAddModList_t **SRB_configList2 = generateSRB2_confList(&ue_context_p->ue_context, ue_context_p->ue_context.SRB_configList, ue_context_p->ue_context.pduSession[0].xid);
  // GTP tunnel for UL
  int ret = drb_config_gtpu_create(&ctxt, ue_context_p, req, ue_context_p->ue_context.DRB_configList, *SRB_configList2, rrc->e1_inst);
  if (ret < 0) AssertFatal(false, "Unable to configure DRB or to create GTP Tunnel\n");

  if(!NODE_IS_CU(RC.nrrrc[ctxt.module_id]->node_type)) {
    rrc_gNB_generate_dedicatedRRCReconfiguration(&ctxt, ue_context_p, NULL);
  } else {
    e1ap_bearer_setup_resp_t resp; // Used to store teids
    int remote_port = RC.nrrrc[ctxt.module_id]->eth_params_s.remote_portd;
    in_addr_t my_addr = inet_addr(RC.nrrrc[ctxt.module_id]->eth_params_s.my_addr);
    instance_t gtpInst = getCxt(CUtype, instance)->gtpInst;
    // GTP tunnel for DL
    fill_e1ap_bearer_setup_resp(&resp, req, gtpInst, ue_context_p->ue_context.rnti, remote_port, my_addr);

    prepare_and_send_ue_context_modification_f1(ue_context_p, &resp);
  }
}

static void cucp_cuup_bearer_context_mod_direct(e1ap_bearer_setup_req_t *const req, instance_t instance) {
  instance_t gtpInst = getCxt(CUtype, instance)->gtpInst;
  CU_update_UP_DL_tunnel(req, gtpInst, req->rnti);
}

void cucp_cuup_message_transfer_direct_init(gNB_RRC_INST *rrc) {
  rrc->cucp_cuup.bearer_context_setup = cucp_cuup_bearer_context_setup_direct;
  rrc->cucp_cuup.bearer_context_mod = cucp_cuup_bearer_context_mod_direct;
}
