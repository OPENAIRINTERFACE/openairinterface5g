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

#include "cucp_cuup_handler.h"

#include "NR_DRB-ToAddModList.h"

#include "common/platform_types.h"
#include "intertask_interface.h"
#include "openair2/COMMON/e1ap_messages_types.h"
#include "openair3/ocp-gtpu/gtp_itf.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "nr_pdcp_oai_api.h"
#include "cuup_cucp_if.h"
#include "common/utils/oai_asn1.h"
#include "openair2/SDAP/nr_sdap/nr_sdap.h"
#include "openair2/E1AP/e1ap_common.h"
#include "openair2/F1AP/f1ap_common.h"

static void fill_DRB_configList_e1(NR_DRB_ToAddModList_t *DRB_configList, const pdu_session_to_setup_t *pdu)
{
  for (int i=0; i < pdu->numDRB2Setup; i++) {
    const DRB_nGRAN_to_setup_t *drb = pdu->DRBnGRanList + i;
    asn1cSequenceAdd(DRB_configList->list, struct NR_DRB_ToAddMod, ie);
    ie->drb_Identity = drb->id;
    ie->cnAssociation = CALLOC(1, sizeof(*ie->cnAssociation));
    ie->cnAssociation->present = NR_DRB_ToAddMod__cnAssociation_PR_sdap_Config;

    // sdap_Config
    asn1cCalloc(ie->cnAssociation->choice.sdap_Config, sdap_config);
    sdap_config->pdu_Session = pdu->sessionId;
    sdap_config->sdap_HeaderDL = drb->sDAP_Header_DL;
    sdap_config->sdap_HeaderUL = drb->sDAP_Header_UL;
    sdap_config->defaultDRB = drb->defaultDRB;

    asn1cCalloc(sdap_config->mappedQoS_FlowsToAdd, FlowsToAdd);
    for (int j=0; j < drb->numQosFlow2Setup; j++) {
      asn1cSequenceAdd(FlowsToAdd->list, NR_QFI_t, qfi);
      *qfi = drb->qosFlows[j].qfi;
    }
    sdap_config->mappedQoS_FlowsToRelease = NULL;

    // pdcp_Config
    ie->reestablishPDCP = NULL;
    ie->recoverPDCP = NULL;
    asn1cCalloc(ie->pdcp_Config, pdcp_config);
    asn1cCalloc(pdcp_config->drb, drbCfg);
    asn1cCallocOne(drbCfg->discardTimer, drb->discardTimer);
    asn1cCallocOne(drbCfg->pdcp_SN_SizeUL, drb->pDCP_SN_Size_UL);
    asn1cCallocOne(drbCfg->pdcp_SN_SizeDL, drb->pDCP_SN_Size_DL);
    drbCfg->headerCompression.present = NR_PDCP_Config__drb__headerCompression_PR_notUsed;
    drbCfg->headerCompression.choice.notUsed = 0;

    drbCfg->integrityProtection = NULL;
    drbCfg->statusReportRequired = NULL;
    drbCfg->outOfOrderDelivery = NULL;
    pdcp_config->moreThanOneRLC = NULL;

    pdcp_config->t_Reordering = calloc(1, sizeof(*pdcp_config->t_Reordering));
    *pdcp_config->t_Reordering = drb->reorderingTimer;
    pdcp_config->ext1 = NULL;

    if (pdu->integrityProtectionIndication == 0 || // Required
        pdu->integrityProtectionIndication == 1) { // Preferred
      asn1cCallocOne(drbCfg->integrityProtection, NR_PDCP_Config__drb__integrityProtection_enabled);
    }

    if (pdu->confidentialityProtectionIndication == 2) { // Not Needed
      asn1cCalloc(pdcp_config->ext1, ext1);
      asn1cCallocOne(ext1->cipheringDisabled, NR_PDCP_Config__ext1__cipheringDisabled_true);
    }
  }
}

static int drb_gtpu_create(instance_t instance,
                           uint32_t ue_id,
                           int incoming_id,
                           int outgoing_id,
                           int qfi,
                           in_addr_t tlAddress, // only IPv4 now
                           teid_t outgoing_teid,
                           gtpCallback callBack,
                           gtpCallbackSDAP callBackSDAP,
                           gtpv1u_gnb_create_tunnel_resp_t *create_tunnel_resp)
{
  gtpv1u_gnb_create_tunnel_req_t create_tunnel_req = {0};
  create_tunnel_req.incoming_rb_id[0] = incoming_id;
  create_tunnel_req.pdusession_id[0] = outgoing_id;
  memcpy(&create_tunnel_req.dst_addr[0].buffer, &tlAddress, sizeof(uint8_t) * 4);
  create_tunnel_req.dst_addr[0].length = 32;
  create_tunnel_req.outgoing_teid[0] = outgoing_teid;
  create_tunnel_req.outgoing_qfi[0] = qfi;
  create_tunnel_req.num_tunnels = 1;
  create_tunnel_req.ue_id = ue_id;

  // we use gtpv1u_create_ngu_tunnel because it returns the interface
  // address and port of the interface; apart from that, we also might call
  // newGtpuCreateTunnel() directly
  return gtpv1u_create_ngu_tunnel(instance, &create_tunnel_req, create_tunnel_resp, callBack, callBackSDAP);
}

static instance_t get_n3_gtp_instance(void)
{
  const e1ap_upcp_inst_t *inst = getCxtE1(0);
  AssertFatal(inst != NULL, "need to have E1 instance\n");
  return inst->gtpInstN3;
}

static instance_t get_f1_gtp_instance(void)
{
  const f1ap_cudu_inst_t *inst = getCxt(0);
  if (!inst)
    return -1; // means no F1
  return inst->gtpInst;
}

void e1_bearer_context_setup(const e1ap_bearer_setup_req_t *req)
{
  bool need_ue_id_mgmt = e1_used();

  /* mirror the CU-CP UE ID for CU-UP */
  uint32_t cu_up_ue_id = req->gNB_cu_cp_ue_id;
  f1_ue_data_t ued = {.secondary_ue = req->gNB_cu_cp_ue_id};
  if (need_ue_id_mgmt) {
    cu_add_f1_ue_data(cu_up_ue_id, &ued);
    LOG_I(E1AP, "adding UE with CU-CP UE ID %d and CU-UP UE ID %d\n", req->gNB_cu_cp_ue_id, cu_up_ue_id);
  }

  instance_t n3inst = get_n3_gtp_instance();
  instance_t f1inst = get_f1_gtp_instance();

  e1ap_bearer_setup_resp_t resp = {
    .gNB_cu_cp_ue_id = req->gNB_cu_cp_ue_id,
    .gNB_cu_up_ue_id = cu_up_ue_id,
  };
  resp.numPDUSessions = req->numPDUSessions;
  for (int i = 0; i < resp.numPDUSessions; ++i) {
    pdu_session_setup_t *resp_pdu = resp.pduSession + i;
    const pdu_session_to_setup_t *req_pdu = req->pduSession + i;
    resp_pdu->id = req_pdu->sessionId;

    AssertFatal(req_pdu->numDRB2Modify == 0, "DRB modification not implemented\n");
    AssertFatal(req_pdu->numDRB2Setup == 1, "can only handle one DRB per PDU session\n");
    resp_pdu->numDRBSetup = req_pdu->numDRB2Setup;
    const DRB_nGRAN_to_setup_t *req_drb = &req_pdu->DRBnGRanList[0];
    DRB_nGRAN_setup_t *resp_drb = &resp_pdu->DRBnGRanList[0];
    resp_drb->id = req_drb->id;
    resp_drb->numQosFlowSetup = req_drb->numQosFlow2Setup;
    for (int k = 0; k < resp_drb->numQosFlowSetup; k++) {
      const qos_flow_to_setup_t *qosflow2Setup = &req_drb->qosFlows[k];
      qos_flow_setup_t *qosflowSetup = &resp_drb->qosFlows[k];
      qosflowSetup->qfi = qosflow2Setup->qfi;
    }

    // GTP tunnel for N3/to core
    gtpv1u_gnb_create_tunnel_resp_t resp_n3 = {0};
    int qfi = 0; // put PDU session marker
    int ret = drb_gtpu_create(n3inst,
                              cu_up_ue_id,
                              req_drb->id,
                              req_pdu->sessionId,
                              qfi,
                              req_pdu->tlAddress,
                              req_pdu->teId,
                              nr_pdcp_data_req_drb,
                              sdap_data_req,
                              &resp_n3);
    AssertFatal(ret >= 0, "Unable to create GTP Tunnel for NG-U\n");
    AssertFatal(resp_n3.num_tunnels == req_pdu->numDRB2Setup, "could not create all tunnels\n");
    resp_pdu->teId = resp_n3.gnb_NGu_teid[0];
    memcpy(&resp_pdu->tlAddress, &resp_n3.gnb_addr.buffer, 4);

    // create PDCP bearers. This will also create SDAP bearers
    NR_DRB_ToAddModList_t DRB_configList = {0};
    fill_DRB_configList_e1(&DRB_configList, req_pdu);
    nr_pdcp_add_drbs(true, // set this to notify PDCP that his not UE
                     cu_up_ue_id,
                     &DRB_configList,
                     (req->integrityProtectionAlgorithm << 4) | req->cipheringAlgorithm,
                     (uint8_t *)req->encryptionKey,
                     (uint8_t *)req->integrityProtectionKey);

    if (f1inst >= 0) { /* we have F1(-U) */
      teid_t dummy_teid = 0xffff; // we will update later with answer from DU
      in_addr_t dummy_address = {0}; // IPv4, updated later with answer from DU
      gtpv1u_gnb_create_tunnel_resp_t resp_f1 = {0};
      int qfi = -1; // don't put PDU session marker in GTP
      int ret = drb_gtpu_create(f1inst,
                                cu_up_ue_id,
                                req_drb->id,
                                req_drb->id,
                                qfi,
                                dummy_address,
                                dummy_teid,
                                cu_f1u_data_req,
                                NULL,
                                &resp_f1);
      resp_drb->numUpParam = 1;
      AssertFatal(ret >= 0, "Unable to create GTP Tunnel for F1-U\n");
      memcpy(&resp_drb->UpParamList[0].tlAddress, &resp_f1.gnb_addr.buffer, 4);
      resp_drb->UpParamList[0].teId = resp_f1.gnb_NGu_teid[0];
    }

    // We assume all DRBs to setup have been setup successfully, so we always
    // send successful outcome in response and no failed DRBs
    resp_pdu->numDRBFailed = 0;
  }

  get_e1_if()->bearer_setup_response(&resp);
}

void e1_bearer_context_modif(const e1ap_bearer_setup_req_t *req)
{
  AssertFatal(req->numPDUSessionsMod > 0, "need at least one PDU session to modify\n");

  e1ap_bearer_modif_resp_t modif = {
      .gNB_cu_cp_ue_id = req->gNB_cu_cp_ue_id,
      .gNB_cu_up_ue_id = req->gNB_cu_up_ue_id,
      .numPDUSessionsMod = req->numPDUSessionsMod,
  };

  instance_t f1inst = get_f1_gtp_instance();

  for (int i=0; i < req->numPDUSessionsMod; i++) {
    DevAssert(req->pduSessionMod[i].sessionId > 0);
    LOG_I(E1AP,
          "UE %d: updating PDU session ID %ld (%ld bearers)\n",
          req->gNB_cu_up_ue_id,
          req->pduSessionMod[i].sessionId,
          req->pduSessionMod[i].numDRB2Modify);
    modif.pduSessionMod[i].id = req->pduSessionMod[i].sessionId;
    modif.pduSessionMod[i].numDRBModified = req->pduSessionMod[i].numDRB2Modify;
    for (int j=0; j < req->pduSessionMod[i].numDRB2Modify; j++) {
      const DRB_nGRAN_to_setup_t *to_modif = &req->pduSessionMod[i].DRBnGRanModList[j];
      DRB_nGRAN_modified_t *modified = &modif.pduSessionMod[i].DRBnGRanModList[j];
      modified->id = to_modif->id;

      if (f1inst < 0) // no F1-U?
        continue; // nothing to do

      in_addr_t addr = {0};
      memcpy(&addr, &to_modif->DlUpParamList[0].tlAddress, sizeof(in_addr_t));

      GtpuUpdateTunnelOutgoingAddressAndTeid(f1inst, req->gNB_cu_cp_ue_id, to_modif->id, addr, to_modif->DlUpParamList[0].teId);
    }
  }

  get_e1_if()->bearer_modif_response(&modif);
}

void e1_bearer_release_cmd(const e1ap_bearer_release_cmd_t *cmd)
{
  bool need_ue_id_mgmt = e1_used();

  instance_t n3inst = get_n3_gtp_instance();
  instance_t f1inst = get_f1_gtp_instance();

  LOG_I(E1AP, "releasing UE %d\n", cmd->gNB_cu_up_ue_id);

  newGtpuDeleteAllTunnels(n3inst, cmd->gNB_cu_up_ue_id);
  if (f1inst >= 0)  // is there F1-U?
    newGtpuDeleteAllTunnels(f1inst, cmd->gNB_cu_up_ue_id);
  nr_pdcp_remove_UE(cmd->gNB_cu_up_ue_id);
  nr_sdap_delete_ue_entities(cmd->gNB_cu_up_ue_id);
  if (need_ue_id_mgmt) {
    cu_remove_f1_ue_data(cmd->gNB_cu_up_ue_id);
  }

  e1ap_bearer_release_cplt_t cplt = {
    .gNB_cu_cp_ue_id = cmd->gNB_cu_cp_ue_id,
    .gNB_cu_up_ue_id = cmd->gNB_cu_up_ue_id,
  };

  get_e1_if()->bearer_release_complete(&cplt);
}
