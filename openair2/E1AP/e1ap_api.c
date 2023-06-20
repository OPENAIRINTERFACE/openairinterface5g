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
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_e1_api.h"
#include "openair2/RRC/NR/cucp_cuup_if.h"
#include "openair2/RRC/LTE/MESSAGES/asn1_msg.h"
#include "openair3/SECU/key_nas_deriver.h"
#include "openair3/ocp-gtpu/gtp_itf.h"
#include "e1ap_asnc.h"
#include "e1ap_common.h"
#include "e1ap.h"

struct NR_DRB_ToAddMod;
static void fill_DRB_configList_e1(NR_DRB_ToAddModList_t *DRB_configList, pdu_session_to_setup_t *pdu) {

  for (int i=0; i < pdu->numDRB2Setup; i++) {
    DRB_nGRAN_to_setup_t *drb = pdu->DRBnGRanList + i;
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
      *qfi = drb->qosFlows[j].fiveQI;
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

static int drb_config_N3gtpu_create(e1ap_bearer_setup_req_t * const req,
                                    gtpv1u_gnb_create_tunnel_resp_t *create_tunnel_resp,
                                    instance_t instance) {

  gtpv1u_gnb_create_tunnel_req_t create_tunnel_req={0};

  NR_DRB_ToAddModList_t DRB_configList = {0};
  for (int i=0; i < req->numPDUSessions; i++) {
    pdu_session_to_setup_t *const pdu = &req->pduSession[i];
    create_tunnel_req.pdusession_id[i] = pdu->sessionId;
    create_tunnel_req.incoming_rb_id[i] = pdu->DRBnGRanList[0].id; // taking only the first DRB. TODO:change this
    memcpy(&create_tunnel_req.dst_addr[i].buffer,
           &pdu->tlAddress,
           sizeof(uint8_t)*4);
    create_tunnel_req.dst_addr[i].length = 32; // 8bits * 4bytes
    create_tunnel_req.outgoing_teid[i] = pdu->teId;
    fill_DRB_configList_e1(&DRB_configList, pdu);
  }
  create_tunnel_req.num_tunnels = req->numPDUSessions;
  create_tunnel_req.ue_id = (req->gNB_cu_cp_ue_id & 0xFFFF);

  // Create N3 tunnel
  int ret = gtpv1u_create_ngu_tunnel(instance, &create_tunnel_req, create_tunnel_resp, nr_pdcp_data_req_drb, sdap_data_req);
  if (ret != 0) {
    LOG_E(NR_RRC,"rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ : gtpv1u_create_ngu_tunnel failed,start to release UE id %ld\n",
          create_tunnel_req.ue_id);
    return ret;
  }

  // Configure DRBs
  nr_pdcp_e1_add_drbs(true, // set this to notify PDCP that his not UE
                      create_tunnel_req.ue_id,
                      &DRB_configList,
                      (req->integrityProtectionAlgorithm << 4) | req->cipheringAlgorithm,
                      (uint8_t *)req->encryptionKey,
                      (uint8_t *)req->integrityProtectionKey);
  return ret;
}

void process_e1_bearer_context_setup_req(instance_t instance, e1ap_bearer_setup_req_t *const req)
{
  e1ap_upcp_inst_t *inst = getCxtE1(instance);
  AssertFatal(inst, "");
  gtpv1u_gnb_create_tunnel_resp_t create_tunnel_resp_N3={0};

  // GTP tunnel for UL
  drb_config_N3gtpu_create(req, &create_tunnel_resp_N3, inst->gtpInstN3);

  MessageDef *msg = itti_alloc_new_message(TASK_CUCP_E1, 0, E1AP_BEARER_CONTEXT_SETUP_RESP);
  e1ap_bearer_setup_resp_t *resp = &E1AP_BEARER_CONTEXT_SETUP_RESP(msg);

  in_addr_t my_addr;
  if (inet_pton(AF_INET, inst->setupReq.localAddressF1U, &my_addr) != 1)
    LOG_E(E1AP, "can't use the F1-U local interface: %s\n", inst->setupReq.localAddressF1U);
  fill_e1ap_bearer_setup_resp(resp, req, inst->gtpInstF1U, req->gNB_cu_cp_ue_id, inst->setupReq.remotePortF1U, my_addr);

  resp->gNB_cu_cp_ue_id = req->gNB_cu_cp_ue_id;
  resp->numPDUSessions = req->numPDUSessions;
  for (int i=0; i < req->numPDUSessions; i++) {
    pdu_session_setup_t *pduSetup = resp->pduSession + i;
    pdu_session_to_setup_t *pdu2Setup = req->pduSession + i;

    pduSetup->id = pdu2Setup->sessionId;
    if (inet_pton(AF_INET, inst->setupReq.localAddressN3, &pduSetup->tlAddress) != 1)
      LOG_E(E1AP, "can't use the N3 local interface: %s\n", inst->setupReq.localAddressN3);
    pduSetup->teId = create_tunnel_resp_N3.gnb_NGu_teid[i];
    pduSetup->numDRBSetup = pdu2Setup->numDRB2Setup;

    // At this point we don't have a way to know the DRBs that failed to setup
    // We assume all DRBs to setup have are setup successfully so we always send successful outcome in response
    // TODO: Modify nr_pdcp_add_drbs() to return DRB list that failed to setup to support E1AP
    pduSetup->numDRBFailed = 0;
  }

  e1apCUUP_send_BEARER_CONTEXT_SETUP_RESPONSE(inst, resp);
}

void CUUP_process_bearer_context_mod_req(instance_t instance, e1ap_bearer_setup_req_t *const req)
{
  e1ap_upcp_inst_t *inst = getCxtE1(instance);
  AssertFatal(inst, "");
  // assume we receive modification of F1-U but it is wrong, we can also get modification of N3 when HO will occur
  CU_update_UP_DL_tunnel(req, inst->gtpInstF1U, req->gNB_cu_cp_ue_id);
  // TODO: send bearer cxt mod response
}

void CUUP_process_bearer_release_command(instance_t instance, e1ap_bearer_release_cmd_t *const cmd)
{
  e1ap_upcp_inst_t *inst = getCxtE1(instance);
  AssertFatal(inst, "");
  newGtpuDeleteAllTunnels(inst->gtpInstN3, cmd->gNB_cu_up_ue_id);
  newGtpuDeleteAllTunnels(inst->gtpInstF1U, cmd->gNB_cu_up_ue_id);
  e1apCUUP_send_BEARER_CONTEXT_RELEASE_COMPLETE(inst, cmd);
}
