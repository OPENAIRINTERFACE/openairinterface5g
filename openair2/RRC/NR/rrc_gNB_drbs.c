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

#include "rrc_gNB_drbs.h"

typedef struct {
  nr_ue_t *list; /* UE Linked List, every element is a UE*/
} nr_ue_list_t;

static nr_ue_list_t ues;

NR_DRB_ToAddMod_t *generateDRB(rnti_t rnti,
                               const pdu_session_param_t *pduSession,
                               bool enable_sdap,
                               int do_drb_integrity,
                               int do_drb_ciphering) {
  uint8_t drb_id = next_available_drb(rnti, pduSession->param.pdusession_id);
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
    ASN_SEQUENCE_ADD(&SDAP_config->mappedQoS_FlowsToAdd->list, qfi);
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
  return DRB_config;
}

uint8_t next_available_drb(rnti_t rnti, uint8_t pdusession_id) {
  nr_ue_t *ue;
  if(nr_ue_get(rnti))
    ue = nr_ue_get(rnti);
  else
    ue = nr_ue_new(rnti);

  uint8_t drb_id;
  for (drb_id = 0; drb_id < MAX_DRBS_PER_UE; drb_id++) {
    if(ue->used_drbs[drb_id] == NULL_DRB && ue->pdus->drbs_established < MAX_DRBS_PER_PDUSESSION) {
      ue->pdus->pdu_drbs[ue->pdus->drbs_established] = drb_id;  /* Store drb_id to pdusession */
      ue->used_drbs[drb_id] = pdusession_id;                    /* Store the pdusession id that is using that drb */
      ue->pdus->drbs_established++;                             /* Increment the index for drbs */
      return ++drb_id;
    }
  }
  return NULL_DRB;
}

nr_ue_t *nr_ue_new(rnti_t rnti) {
  if(nr_ue_get(rnti)) {
    LOG_D(RRC, "UE already exists with rnti: %u\n", rnti);
    return nr_ue_get(rnti);
  }

  nr_ue_t *ue;
  ue = calloc(1, sizeof(nr_ue_t));
  ue->ue_id = rnti;
  ue->next_ue = ues.list;
  ues.list = ue;
  return ues.list;
}

nr_ue_t *nr_ue_get(rnti_t rnti) {
  nr_ue_t *ue;
  ue = ues.list;

  if(ue == NULL)
    return NULL;

  while(ue->ue_id != rnti && ue->next_ue != NULL)
    ue = ue->next_ue;

  if(ue->ue_id == rnti)
    return ue;

  return NULL;
}

void nr_ue_delete(rnti_t rnti) {
  nr_ue_t *ue;
  ue = ues.list;

  if(ue->ue_id == rnti) {
    ues.list = ues.list->next_ue;
    free(ue);
  } else {
    nr_ue_t *uePrev = NULL;

    while(ue->ue_id != rnti && ue->next_ue != NULL) {
      uePrev = ue;
      ue = ue->next_ue;
    }

    if(ue->ue_id != rnti) {
      uePrev->next_ue = ue->next_ue;
      free(ue);
    }
  }
}