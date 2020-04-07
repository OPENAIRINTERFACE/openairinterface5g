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

/* \file nr_ue_dci_configuration.c
 * \brief functions for generating dci search procedures based on RRC Serving Cell Group Configuration
 * \author R. Knopp
 * \date 2020
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include "mac_proto.h"
#include "mac_defs.h"
#include "assertions.h"
#include "LAYER2/NR_MAC_UE/mac_extern.h"
#include "mac_defs.h"
#include <stdio.h>

#ifdef NR_PDCCH_DCI_TOOLS_DEBUG
#define LOG_DCI_D(a...) printf("\t\t<-NR_PDCCH_DCI_TOOLS_DEBUG (nr_extract_dci_info) ->" a)
#else 
#define LOG_DCI_D(a...)
#endif
#define LOG_DCI_PARM(a...) LOG_D(PHY,"\t<-NR_PDCCH_DCI_TOOLS_DEBUG (nr_generate_ue_ul_dlsch_params_from_dci)" a)


void fill_dci_search_candidates(NR_SearchSpace_t *ss,fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15) {

  LOG_D(MAC,"Filling search candidates for DCI\n");
  
  rel15->number_of_candidates=3;
  rel15->CCE[0]=0;
  rel15->L[0]=4;
  rel15->CCE[1]=4;
  rel15->L[1]=4;
  rel15->CCE[2]=8;
  rel15->L[2]=4;

}

void ue_dci_configuration(NR_UE_MAC_INST_t *mac,fapi_nr_dl_config_request_t *dl_config,int frame,int slot) {

  // check if DL slot
  if (is_nr_DL_slot(mac->scc,slot)==1) {
    
    // get BWP 1, Coreset 0, SearchSpace 0  
    if (mac->DLbwp[0]==NULL) {
      AssertFatal(mac->scd->downlinkBWP_ToAddModList!=NULL,"downlinkBWP_ToAddModList is null\n");
      AssertFatal(mac->scd->downlinkBWP_ToAddModList->list.count==1,"downlinkBWP_ToAddModList->list->count is %d\n",
		  mac->scd->downlinkBWP_ToAddModList->list.count);
      mac->DLbwp[0]      = mac->scd->downlinkBWP_ToAddModList->list.array[0];
      AssertFatal(mac->DLbwp[0]->bwp_Dedicated!=NULL,"bwp_Dedicated is null\n");
      AssertFatal(mac->DLbwp[0]->bwp_Dedicated->pdcch_Config!=NULL,"pdcch_Config is null\n");
      AssertFatal(mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList!=NULL,"controlResourceSetToAddModList is null\n");
      AssertFatal(mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.count==1,"controlResourceSetToAddModList->list.count=%d\n",
		  mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.count);
      mac->coreset[0][0] = mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.array[0];
      AssertFatal(mac->coreset[0][0]!=NULL,"coreset[0][0] is null\n");
      
      AssertFatal(mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList!=NULL,"searchPsacesToAddModList is null\n");
      AssertFatal(mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count>0,
		  "searchPsacesToAddModList is empty\n");
      NR_SearchSpace_t *ss;
      int ss_id=0;
      AssertFatal(mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count<FAPI_NR_MAX_SS_PER_CORESET,
		  "too many searchpaces per coreset %d\n",
		  mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count);
      for (int i=0;i<mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count;i++) {
	ss=mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.array[i];
	AssertFatal(ss->controlResourceSetId != NULL,"ss->controlResourceSetId is null\n");
	AssertFatal(ss->searchSpaceType != NULL,"ss->searchSpaceType is null\n");
	AssertFatal(*ss->controlResourceSetId == mac->coreset[0][0]->controlResourceSetId,"ss->controlResourceSetId is unknown\n");
	mac->SSpace[0][0][ss_id] = ss;
	ss_id++;
      }
    }
    if (mac->ULbwp[0]==NULL) {
      AssertFatal(mac->scd->uplinkConfig->uplinkBWP_ToAddModList!=NULL,"uplinkBWP_ToAddModList is null\n");
      AssertFatal(mac->scd->uplinkConfig->uplinkBWP_ToAddModList->list.count==1,"uplinkBWP_ToAddModList->list->count is %d\n",
		  mac->scd->uplinkConfig->uplinkBWP_ToAddModList->list.count);
      mac->ULbwp[0]      = mac->scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0];
      AssertFatal(mac->ULbwp[0]->bwp_Dedicated!=NULL,"bwp_Dedicated is null\n");
    }
    // check search spaces
	
    int ss_id=0;
    fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15;
    int sps =mac->DLbwp[0]->bwp_Common->genericParameters.cyclicPrefix == NULL ? 14 : 12;
    
    while(mac->SSpace[0][0][ss_id]!=NULL && 
	  ss_id < mac->DLbwp[0]->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count) {
      
      AssertFatal(mac->SSpace[0][0][ss_id]->monitoringSymbolsWithinSlot!=NULL,"ss->monitoringSymbolsWithinSlot is null\n");
      AssertFatal(mac->SSpace[0][0][ss_id]->monitoringSymbolsWithinSlot->buf!=NULL,"ss->monitoringSymbolsWithinSlot->buf is null\n");
      
      ss_id++;
    }
    if (mac->ra_state == WAIT_RAR) {
      // check for RAR
      rel15 = &dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15; 
      rel15->rnti = 2;//get_RA_RNTI(mac,frame,slot);
      dl_config->number_pdus = dl_config->number_pdus + 1;
    }
    else if (mac->ra_state == WAIT_CONTENTION_RESOLUTION) {
      rel15 = &dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15; 
      rel15->rnti = mac->t_crnti;
      dl_config->number_pdus = dl_config->number_pdus + 1;
    }
    if (mac->crnti>0) {
      rel15 = &dl_config->dl_config_list[dl_config->number_pdus].dci_config_pdu.dci_config_rel15; 
      rel15->rnti = mac->crnti;
      rel15->BWPSize = NRRIV2BW(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth,275);
      rel15->BWPStart = NRRIV2PRBOFFSET(mac->DLbwp[0]->bwp_Common->genericParameters.locationAndBandwidth,275);
      rel15->SubcarrierSpacing = mac->DLbwp[0]->bwp_Common->genericParameters.subcarrierSpacing;
      // get UE-specific search space
      for (ss_id=0;ss_id<FAPI_NR_MAX_SS_PER_CORESET && mac->SSpace[0][0][ss_id]!=NULL;ss_id++) 
	if (mac->SSpace[0][0][ss_id]->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific) break;
      AssertFatal(ss_id<FAPI_NR_MAX_SS_PER_CORESET,"couldn't find a UE-specific SS\n");
      // for SPS=14 8 MSBs in positions 13 downto 6,  
      uint16_t monitoringSymbolsWithinSlot = (mac->SSpace[0][0][ss_id]->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) | 
	(mac->SSpace[0][0][ss_id]->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));
      
      for (int i=0; i<sps; i++)
	if ((monitoringSymbolsWithinSlot>>(sps-1-i))&1) {
	  rel15->coreset.StartSymbolIndex=i;
	  break;
	}
      rel15->coreset.duration = mac->coreset[0][0]->duration;
      for (int i=0;i<6;i++)
	rel15->coreset.frequency_domain_resource[i] = mac->coreset[0][0]->frequencyDomainResources.buf[i];
      
      rel15->coreset.CceRegMappingType = mac->coreset[0][0]->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved?
	FAPI_NR_CCE_REG_MAPPING_TYPE_INTERLEAVED : FAPI_NR_CCE_REG_MAPPING_TYPE_NON_INTERLEAVED;
      if (rel15->coreset.CceRegMappingType == FAPI_NR_CCE_REG_MAPPING_TYPE_INTERLEAVED) {
	rel15->coreset.RegBundleSize = (mac->coreset[0][0]->cce_REG_MappingType.choice.interleaved->reg_BundleSize == NR_ControlResourceSet__cce_REG_MappingType__interleaved__reg_BundleSize_n6) ? 6 : (2+mac->coreset[0][0]->cce_REG_MappingType.choice.interleaved->reg_BundleSize);
	rel15->coreset.InterleaverSize = (mac->coreset[0][0]->cce_REG_MappingType.choice.interleaved->interleaverSize==NR_ControlResourceSet__cce_REG_MappingType__interleaved__interleaverSize_n6) ? 6 : (2+mac->coreset[0][0]->cce_REG_MappingType.choice.interleaved->interleaverSize);
	AssertFatal(mac->scc->physCellId != NULL,"mac->scc->physCellId is null\n");
	rel15->coreset.ShiftIndex = mac->coreset[0][0]->cce_REG_MappingType.choice.interleaved->shiftIndex != NULL ? *mac->coreset[0][0]->cce_REG_MappingType.choice.interleaved->shiftIndex : *mac->scc->physCellId;
      }
      else {
	rel15->coreset.RegBundleSize = 0;
	rel15->coreset.InterleaverSize = 0;
	rel15->coreset.ShiftIndex = 0;
      }
      rel15->coreset.CoreSetType = 1;
      rel15->coreset.precoder_granularity = mac->coreset[0][0]->precoderGranularity;
      if (mac->coreset[0][0]->pdcch_DMRS_ScramblingID)
	rel15->coreset.pdcch_dmrs_scrambling_id = *mac->coreset[0][0]->pdcch_DMRS_ScramblingID;
      else
	rel15->coreset.pdcch_dmrs_scrambling_id = *mac->scc->physCellId;
      fill_dci_search_candidates(mac->SSpace[0][0][ss_id],rel15);
      rel15->dci_format = NR_DL_DCI_FORMAT_1_0;
      rel15->dci_length = nr_dci_size(rel15->dci_format,NR_RNTI_C,rel15->BWPSize);
      dl_config->dl_config_list[dl_config->number_pdus].pdu_type = FAPI_NR_DL_CONFIG_TYPE_DCI;
      dl_config->number_pdus = dl_config->number_pdus + 1;
    }
  }
}




