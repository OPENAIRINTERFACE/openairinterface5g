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

#include "rrc_gNB_radio_bearers.h"
#include "oai_asn1.h"

rrc_pdu_session_param_t *find_pduSession(gNB_RRC_UE_t *ue, int id, bool create)
{
  int j;
  for (j = 0; j < ue->nb_of_pdusessions; j++)
    if (id == ue->pduSession[j].param.pdusession_id)
      break;
  if (j == ue->nb_of_pdusessions) {
    if (create)
      ue->nb_of_pdusessions++;
    else
      return NULL;
  }
  return ue->pduSession + j;
}

rrc_pdu_session_param_t *find_pduSession_from_drbId(gNB_RRC_UE_t *ue, int drb_id)
{
  const drb_t *drb = &ue->established_drbs[drb_id - 1];
  if (drb->status == DRB_INACTIVE) {
    LOG_E(NR_RRC, "UE %d: DRB %d inactive\n", ue->rrc_ue_id, drb_id);
    return NULL;
  }
  int id = drb->cnAssociation.sdap_config.pdusession_id;
  return find_pduSession(ue, id, false);
}

drb_t *get_drb(gNB_RRC_UE_t *ue, uint8_t drb_id)
{
  DevAssert(drb_id > 0 && drb_id < 32);
  DevAssert(ue != NULL);

  return &ue->established_drbs[drb_id - 1];
}

drb_t *generateDRB(gNB_RRC_UE_t *ue,
                   uint8_t drb_id,
                   const rrc_pdu_session_param_t *pduSession,
                   bool enable_sdap,
                   int do_drb_integrity,
                   int do_drb_ciphering)
{
  DevAssert(ue != NULL);

  LOG_I(NR_RRC, "UE %d: configure DRB ID %d for PDU session ID %d\n", ue->rrc_ue_id, drb_id, pduSession->param.pdusession_id);

  drb_t *est_drb = &ue->established_drbs[drb_id - 1];
  DevAssert(est_drb->status == DRB_INACTIVE);

  est_drb->status = DRB_ACTIVE;
  est_drb->drb_id = drb_id;
  est_drb->reestablishPDCP = -1;
  est_drb->recoverPDCP = -1;
  est_drb->cnAssociation.sdap_config.defaultDRB = true;
  est_drb->defaultDRBid = drb_id;

  /* SDAP Configuration */
  est_drb->cnAssociation.present = NR_DRB_ToAddMod__cnAssociation_PR_sdap_Config;
  est_drb->cnAssociation.sdap_config.pdusession_id = pduSession->param.pdusession_id;
  if (enable_sdap) {
    est_drb->cnAssociation.sdap_config.sdap_HeaderDL = NR_SDAP_Config__sdap_HeaderDL_present;
    est_drb->cnAssociation.sdap_config.sdap_HeaderUL = NR_SDAP_Config__sdap_HeaderUL_present;
  } else {
    est_drb->cnAssociation.sdap_config.sdap_HeaderDL = NR_SDAP_Config__sdap_HeaderDL_absent;
    est_drb->cnAssociation.sdap_config.sdap_HeaderUL = NR_SDAP_Config__sdap_HeaderUL_absent;
  }
  for (int qos_flow_index = 0; qos_flow_index < pduSession->param.nb_qos; qos_flow_index++) {
    est_drb->cnAssociation.sdap_config.mappedQoS_FlowsToAdd[qos_flow_index] = pduSession->param.qos[qos_flow_index].qfi;
    if (pduSession->param.qos[qos_flow_index].fiveQI > 5)
      est_drb->status = DRB_ACTIVE_NONGBR;
    else
      est_drb->status = DRB_ACTIVE;
  }
  /* PDCP Configuration */
  est_drb->pdcp_config.discardTimer = NR_PDCP_Config__drb__discardTimer_infinity;
  est_drb->pdcp_config.pdcp_SN_SizeDL = NR_PDCP_Config__drb__pdcp_SN_SizeDL_len18bits;
  est_drb->pdcp_config.pdcp_SN_SizeUL = NR_PDCP_Config__drb__pdcp_SN_SizeUL_len18bits;
  est_drb->pdcp_config.t_Reordering = NR_PDCP_Config__t_Reordering_ms100;
  est_drb->pdcp_config.headerCompression.present = NR_PDCP_Config__drb__headerCompression_PR_notUsed;
  est_drb->pdcp_config.headerCompression.NotUsed = 0;
  if (do_drb_integrity)
    est_drb->pdcp_config.integrityProtection = NR_PDCP_Config__drb__integrityProtection_enabled;
  else
    est_drb->pdcp_config.integrityProtection = 1;
  if (do_drb_ciphering)
    est_drb->pdcp_config.ext1.cipheringDisabled = 1;
  else
    est_drb->pdcp_config.ext1.cipheringDisabled = NR_PDCP_Config__ext1__cipheringDisabled_true;

  drb_t *rrc_drb = get_drb(ue, drb_id);
  DevAssert(rrc_drb == est_drb); /* to double check that we create the same which we would retrieve */
  return rrc_drb;
}

NR_DRB_ToAddMod_t *generateDRB_ASN1(const drb_t *drb_asn1)
{
  NR_DRB_ToAddMod_t *DRB_config = CALLOC(1, sizeof(*DRB_config));
  NR_SDAP_Config_t *SDAP_config = CALLOC(1, sizeof(NR_SDAP_Config_t));

  asn1cCalloc(DRB_config->cnAssociation, association);
  asn1cCalloc(SDAP_config->mappedQoS_FlowsToAdd, sdapFlows);
  asn1cCalloc(DRB_config->pdcp_Config, pdcpConfig);
  asn1cCalloc(pdcpConfig->drb, drb);

  DRB_config->drb_Identity = drb_asn1->drb_id;
  association->present = drb_asn1->cnAssociation.present;

  /* SDAP Configuration */
  SDAP_config->pdu_Session = drb_asn1->cnAssociation.sdap_config.pdusession_id;
  SDAP_config->sdap_HeaderDL = drb_asn1->cnAssociation.sdap_config.sdap_HeaderDL;
  SDAP_config->sdap_HeaderUL = drb_asn1->cnAssociation.sdap_config.sdap_HeaderUL;
  SDAP_config->defaultDRB = drb_asn1->cnAssociation.sdap_config.defaultDRB;

  for (int qos_flow_index = 0; qos_flow_index < QOSFLOW_MAX_VALUE; qos_flow_index++) {
    if (drb_asn1->cnAssociation.sdap_config.mappedQoS_FlowsToAdd[qos_flow_index] != 0) {
      asn1cSequenceAdd(sdapFlows->list, NR_QFI_t, qfi);
      *qfi = drb_asn1->cnAssociation.sdap_config.mappedQoS_FlowsToAdd[qos_flow_index];
    }
  }

  association->choice.sdap_Config = SDAP_config;

  /* PDCP Configuration */
  asn1cCallocOne(drb->discardTimer, drb_asn1->pdcp_config.discardTimer);
  asn1cCallocOne(drb->pdcp_SN_SizeUL, drb_asn1->pdcp_config.pdcp_SN_SizeUL);
  asn1cCallocOne(drb->pdcp_SN_SizeDL, drb_asn1->pdcp_config.pdcp_SN_SizeDL);
  asn1cCallocOne(pdcpConfig->t_Reordering, drb_asn1->pdcp_config.t_Reordering);

  drb->headerCompression.present = drb_asn1->pdcp_config.headerCompression.present;
  drb->headerCompression.choice.notUsed = drb_asn1->pdcp_config.headerCompression.NotUsed;

  if (!drb_asn1->pdcp_config.integrityProtection) {
    asn1cCallocOne(drb->integrityProtection, drb_asn1->pdcp_config.integrityProtection);
  }
  if (!drb_asn1->pdcp_config.ext1.cipheringDisabled) {
    asn1cCalloc(pdcpConfig->ext1, ext1);
    asn1cCallocOne(ext1->cipheringDisabled, drb_asn1->pdcp_config.ext1.cipheringDisabled);
  }

  return DRB_config;
}

uint8_t get_next_available_drb_id(gNB_RRC_UE_t *ue)
{
  for (uint8_t drb_id = 0; drb_id < MAX_DRBS_PER_UE; drb_id++)
    if (ue->established_drbs[drb_id].status == DRB_INACTIVE)
      return drb_id + 1;
  /* From this point, we need to handle the case that all DRBs are already used by the UE. */
  LOG_E(RRC, "Error - All the DRBs are used - Handle this\n");
  return DRB_INACTIVE;
}

bool drb_is_active(gNB_RRC_UE_t *ue, uint8_t drb_id)
{
  drb_t *drb = get_drb(ue, drb_id);
  if (drb == NULL)
    return DRB_INACTIVE;
  return drb->status != DRB_INACTIVE;
}
