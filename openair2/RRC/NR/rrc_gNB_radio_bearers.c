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

NR_SRB_ToAddMod_t *generateSRB2() {
  NR_SRB_ToAddMod_t *SRB2_config = NULL;

  SRB2_config = CALLOC(1, sizeof(*SRB2_config));
  SRB2_config->srb_Identity = 2;

  return SRB2_config;
}

NR_SRB_ToAddModList_t **generateSRB2_confList(gNB_RRC_UE_t *ue, 
                                              NR_SRB_ToAddModList_t *SRB_configList, 
                                              uint8_t xid) {
  NR_SRB_ToAddModList_t **SRB_configList2 = NULL;

  SRB_configList2 = &ue->SRB_configList2[xid];
  if (*SRB_configList2 == NULL) {
    *SRB_configList2 = CALLOC(1, sizeof(**SRB_configList2));
    memset(*SRB_configList2, 0, sizeof(**SRB_configList2));
    NR_SRB_ToAddMod_t *SRB2_config = generateSRB2();
    asn1cSeqAdd(&(*SRB_configList2)->list, SRB2_config);
    asn1cSeqAdd(&SRB_configList->list, SRB2_config);
  }

  return SRB_configList2;
}

NR_DRB_ToAddMod_t *generateDRB(gNB_RRC_UE_t *ue,
                               uint8_t drb_id,
                               const pdu_session_param_t *pduSession,
                               bool enable_sdap,
                               int do_drb_integrity,
                               int do_drb_ciphering) {
  NR_DRB_ToAddMod_t *DRB_config  = NULL;
  NR_SDAP_Config_t  *SDAP_config = NULL;

  DRB_config = CALLOC(1, sizeof(*DRB_config));
  DRB_config->drb_Identity = drb_id;
  DRB_config->cnAssociation = CALLOC(1, sizeof(*DRB_config->cnAssociation));
  DRB_config->cnAssociation->present = NR_DRB_ToAddMod__cnAssociation_PR_sdap_Config;
  
  /* SDAP Configuration */
  SDAP_config = CALLOC(1, sizeof(NR_SDAP_Config_t));
  memset(SDAP_config, 0, sizeof(NR_SDAP_Config_t));
  SDAP_config->mappedQoS_FlowsToAdd = calloc(1, sizeof(struct NR_SDAP_Config__mappedQoS_FlowsToAdd));
  memset(SDAP_config->mappedQoS_FlowsToAdd, 0, sizeof(struct NR_SDAP_Config__mappedQoS_FlowsToAdd));
  
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
    NR_QFI_t *qfi = calloc(1, sizeof(NR_QFI_t));
    *qfi = pduSession->param.qos[qos_flow_index].qfi;
    asn1cSeqAdd(&SDAP_config->mappedQoS_FlowsToAdd->list, qfi);

    if(pduSession->param.qos[qos_flow_index].fiveQI > 5)
      ue->pduSession[pduSession->param.pdusession_id].param.used_drbs[drb_id-1] = DRB_ACTIVE_NONGBR;
    else
      ue->pduSession[pduSession->param.pdusession_id].param.used_drbs[drb_id-1] = DRB_ACTIVE;
  }
  
  SDAP_config->mappedQoS_FlowsToRelease = NULL;
  DRB_config->cnAssociation->choice.sdap_Config = SDAP_config;
  
  /* PDCP Configuration */
  DRB_config->reestablishPDCP  = NULL;
  DRB_config->recoverPDCP      = NULL;
  DRB_config->pdcp_Config      = calloc(1, sizeof(*DRB_config->pdcp_Config));
  DRB_config->pdcp_Config->drb = calloc(1,sizeof(*DRB_config->pdcp_Config->drb));

  DRB_config->pdcp_Config->drb->discardTimer    = calloc(1, sizeof(*DRB_config->pdcp_Config->drb->discardTimer));
  *DRB_config->pdcp_Config->drb->discardTimer   = NR_PDCP_Config__drb__discardTimer_infinity;
  DRB_config->pdcp_Config->drb->pdcp_SN_SizeUL  = calloc(1, sizeof(*DRB_config->pdcp_Config->drb->pdcp_SN_SizeUL));
  *DRB_config->pdcp_Config->drb->pdcp_SN_SizeUL = NR_PDCP_Config__drb__pdcp_SN_SizeUL_len18bits;
  DRB_config->pdcp_Config->drb->pdcp_SN_SizeDL  = calloc(1, sizeof(*DRB_config->pdcp_Config->drb->pdcp_SN_SizeDL));
  *DRB_config->pdcp_Config->drb->pdcp_SN_SizeDL = NR_PDCP_Config__drb__pdcp_SN_SizeDL_len18bits;

  DRB_config->pdcp_Config->drb->headerCompression.present = NR_PDCP_Config__drb__headerCompression_PR_notUsed;
  DRB_config->pdcp_Config->drb->headerCompression.choice.notUsed = 0;
  
  DRB_config->pdcp_Config->drb->integrityProtection  = NULL;
  DRB_config->pdcp_Config->drb->statusReportRequired = NULL;
  DRB_config->pdcp_Config->drb->outOfOrderDelivery   = NULL;
  DRB_config->pdcp_Config->moreThanOneRLC            = NULL;
  
  DRB_config->pdcp_Config->t_Reordering  = calloc(1, sizeof(*DRB_config->pdcp_Config->t_Reordering));
  *DRB_config->pdcp_Config->t_Reordering = NR_PDCP_Config__t_Reordering_ms0;
  DRB_config->pdcp_Config->ext1          = NULL;
  
  if (do_drb_integrity) {
    DRB_config->pdcp_Config->drb->integrityProtection = calloc(1, sizeof(*DRB_config->pdcp_Config->drb->integrityProtection));
    *DRB_config->pdcp_Config->drb->integrityProtection = NR_PDCP_Config__drb__integrityProtection_enabled;
  }
  
  if (!do_drb_ciphering) {
    DRB_config->pdcp_Config->ext1 = calloc(1, sizeof(*DRB_config->pdcp_Config->ext1));
    DRB_config->pdcp_Config->ext1->cipheringDisabled = calloc(1, sizeof(*DRB_config->pdcp_Config->ext1->cipheringDisabled));
    *DRB_config->pdcp_Config->ext1->cipheringDisabled = NR_PDCP_Config__ext1__cipheringDisabled_true;
  }

  ue->DRB_active[drb_id-1] = DRB_ACTIVE;

  return DRB_config;
}

uint8_t next_available_drb(gNB_RRC_UE_t *ue, uint8_t pdusession_id, bool is_gbr) {
  uint8_t drb_id;

  if(!is_gbr) { /* Find if Non-GBR DRB exists in the same PDU Session */
    for (drb_id = 0; drb_id < NGAP_MAX_DRBS_PER_UE; drb_id++)
      if(ue->pduSession[pdusession_id].param.used_drbs[drb_id] == DRB_ACTIVE_NONGBR)
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
  if(ue->DRB_active[drb_id-1] == DRB_ACTIVE)
    return true;
  return false;
}
