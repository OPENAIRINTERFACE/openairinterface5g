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
  if (j == ue->nb_of_pdusessions && create)
    ue->nb_of_pdusessions++;
  else
    return NULL;
  AssertFatal(ue->nb_of_pdusessions < NGAP_MAX_PDU_SESSION, "");
  return ue->pduSession + j;
}

NR_DRB_ToAddMod_t *generateDRB(gNB_RRC_UE_t *ue, uint8_t drb_id, rrc_pdu_session_param_t *pduSession, bool enable_sdap, int do_drb_integrity, int do_drb_ciphering)
{
  NR_DRB_ToAddMod_t *DRB_config = CALLOC(1, sizeof(*DRB_config));
  DRB_config->drb_Identity = drb_id;
  asn1cCalloc(DRB_config->cnAssociation, association);
  association->present = NR_DRB_ToAddMod__cnAssociation_PR_sdap_Config;

  /* SDAP Configuration */
  NR_SDAP_Config_t *SDAP_config = CALLOC(1, sizeof(NR_SDAP_Config_t));
  asn1cCalloc(SDAP_config->mappedQoS_FlowsToAdd, sdapFlows);

  SDAP_config->pdu_Session = pduSession->param.pdusession_id;
  
  if (enable_sdap) {
    SDAP_config->sdap_HeaderDL = NR_SDAP_Config__sdap_HeaderDL_present;
    SDAP_config->sdap_HeaderUL = NR_SDAP_Config__sdap_HeaderUL_present;
  } else {
    SDAP_config->sdap_HeaderDL = NR_SDAP_Config__sdap_HeaderDL_absent;
    SDAP_config->sdap_HeaderUL = NR_SDAP_Config__sdap_HeaderUL_absent;
  }
  
  SDAP_config->defaultDRB = true;
  
  for (int qos_flow_index = 0; qos_flow_index < pduSession->param.nb_qos; qos_flow_index++) 
  {
    asn1cSequenceAdd(sdapFlows->list, NR_QFI_t, qfi);
    *qfi = pduSession->param.qos[qos_flow_index].qfi;
    if(pduSession->param.qos[qos_flow_index].fiveQI > 5)
      pduSession->param.used_drbs[drb_id - 1] = DRB_ACTIVE_NONGBR;
    else
      pduSession->param.used_drbs[drb_id - 1] = DRB_ACTIVE;
  }

  association->choice.sdap_Config = SDAP_config;

  /* PDCP Configuration */
  asn1cCalloc(DRB_config->pdcp_Config, pdcpConfig);
  asn1cCalloc(pdcpConfig->drb, drb);

  asn1cCallocOne(drb->discardTimer, NR_PDCP_Config__drb__discardTimer_infinity);

  asn1cCallocOne(drb->pdcp_SN_SizeUL, NR_PDCP_Config__drb__pdcp_SN_SizeUL_len18bits);
  asn1cCallocOne(drb->pdcp_SN_SizeDL, NR_PDCP_Config__drb__pdcp_SN_SizeDL_len18bits);

  drb->headerCompression.present = NR_PDCP_Config__drb__headerCompression_PR_notUsed;
  drb->headerCompression.choice.notUsed = 0;

  asn1cCallocOne(pdcpConfig->t_Reordering, NR_PDCP_Config__t_Reordering_ms100);
  
  if (do_drb_integrity) {
    asn1cCallocOne(drb->integrityProtection, NR_PDCP_Config__drb__integrityProtection_enabled);
  }
  if (!do_drb_ciphering) {
    asn1cCalloc(pdcpConfig->ext1, ext1);
    asn1cCallocOne(ext1->cipheringDisabled, NR_PDCP_Config__ext1__cipheringDisabled_true);
  }

  ue->DRB_active[drb_id-1] = DRB_ACTIVE;

  return DRB_config;
}

uint8_t next_available_drb(gNB_RRC_UE_t *ue, rrc_pdu_session_param_t *pdusession, bool is_gbr)
{
  uint8_t drb_id;

  if(!is_gbr) { /* Find if Non-GBR DRB exists in the same PDU Session */
    for (drb_id = 0; drb_id < NGAP_MAX_DRBS_PER_UE; drb_id++)
      if (pdusession->param.used_drbs[drb_id] == DRB_ACTIVE_NONGBR)
        return drb_id+1;
  }
  /* GBR Flow  or a Non-GBR DRB does not exist in the same PDU Session, find an available DRB */
  for (drb_id = 0; drb_id < NGAP_MAX_DRBS_PER_UE; drb_id++)
    if(ue->DRB_active[drb_id] == DRB_INACTIVE)
      return drb_id+1;
  /* From this point, we need to handle the case that all DRBs are already used by the UE. */
  LOG_E(RRC, "Error - All the DRBs are used - Handle this\n");
  return DRB_INACTIVE;
}

bool drb_is_active(gNB_RRC_UE_t *ue, uint8_t drb_id) {
  DevAssert(drb_id > 0);
  if(ue->DRB_active[drb_id-1] == DRB_ACTIVE)
    return true;
  return false;
}
