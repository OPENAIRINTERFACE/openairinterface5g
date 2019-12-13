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

/* \file fapi_nr_ue_l1.c
 * \brief functions for NR UE FAPI-like interface
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include <stdio.h>

#include "fapi_nr_ue_interface.h"
#include "fapi_nr_ue_l1.h"
//#include "PHY/phy_vars_nr_ue.h"

#include "PHY/defs_nr_UE.h"
#include "PHY/impl_defs_nr.h"
extern PHY_VARS_NR_UE ***PHY_vars_UE_g;

int8_t nr_ue_scheduled_response(nr_scheduled_response_t *scheduled_response)
{
  /// module id
  module_id_t module_id = scheduled_response->module_id; 
  /// component carrier id
  uint8_t cc_id = scheduled_response->CC_id;
  uint32_t i;
  int slot = scheduled_response->slot;

  if(scheduled_response != NULL){
    // Note: we have to handle the thread IDs for this. To be revisited completely.
    uint8_t thread_id = PHY_vars_UE_g[module_id][cc_id]->current_thread_id[slot];
    NR_UE_PDCCH *pdcch_vars2 = PHY_vars_UE_g[module_id][cc_id]->pdcch_vars[thread_id][0];
    NR_UE_DLSCH_t *dlsch0 = PHY_vars_UE_g[module_id][cc_id]->dlsch[thread_id][0][0];
    NR_UE_ULSCH_t *ulsch0 = PHY_vars_UE_g[module_id][cc_id]->ulsch[thread_id][0][0];
    //NR_DL_FRAME_PARMS frame_parms = PHY_vars_UE_g[module_id][cc_id]->frame_parms;
    PRACH_RESOURCES_t *prach_resources = PHY_vars_UE_g[module_id][cc_id]->prach_resources[0];
        
    //        PUCCH_ConfigCommon_nr_t    *pucch_config_common = PHY_vars_UE_g[module_id][cc_id]->pucch_config_common_nr[0];
    //        PUCCH_Config_t             *pucch_config_dedicated = PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0];

    if(scheduled_response->dl_config != NULL){
      fapi_nr_dl_config_request_t *dl_config = scheduled_response->dl_config;

      for(i=0; i<dl_config->number_pdus; ++i){
	if(dl_config->dl_config_list[i].pdu_type == FAPI_NR_DL_CONFIG_TYPE_DCI){
	  pdcch_vars2->nb_search_space = pdcch_vars2->nb_search_space + 1;
	  fapi_nr_dl_config_dci_dl_pdu_rel15_t *dci_config = &dl_config->dl_config_list[i].dci_config_pdu.dci_config_rel15;
             
	  pdcch_vars2->n_RB_BWP[i] = dci_config->N_RB_BWP;
	  pdcch_vars2->searchSpace[i].monitoringSymbolWithinSlot = dci_config->monitoring_symbols_within_slot;
                    
	  pdcch_vars2->searchSpace[i].nrofCandidates_aggrlevel1  = dci_config->number_of_candidates[0];
	  pdcch_vars2->searchSpace[i].nrofCandidates_aggrlevel2  = dci_config->number_of_candidates[1];
	  pdcch_vars2->searchSpace[i].nrofCandidates_aggrlevel4  = dci_config->number_of_candidates[2];
	  pdcch_vars2->searchSpace[i].nrofCandidates_aggrlevel8  = dci_config->number_of_candidates[3];
	  pdcch_vars2->searchSpace[i].nrofCandidates_aggrlevel16 = dci_config->number_of_candidates[4];

	  pdcch_vars2->coreset[i].duration = dci_config->coreset.duration;
                    
	  pdcch_vars2->coreset[i].frequencyDomainResources = dci_config->coreset.frequency_domain_resource;
	  pdcch_vars2->coreset[i].rb_offset = dci_config->coreset.rb_offset;

	  if(dci_config->coreset.cce_reg_mapping_type == CCE_REG_MAPPING_TYPE_INTERLEAVED){
	    pdcch_vars2->coreset[i].cce_reg_mappingType.shiftIndex = dci_config->coreset.cce_reg_interleaved_shift_index;
	    pdcch_vars2->coreset[i].cce_reg_mappingType.reg_bundlesize = dci_config->coreset.cce_reg_interleaved_reg_bundle_size;
	    pdcch_vars2->coreset[i].cce_reg_mappingType.interleaversize = dci_config->coreset.cce_reg_interleaved_interleaver_size;
	  }else{  //CCE_REG_MAPPING_TYPE_NON_INTERLEAVED
	    pdcch_vars2->coreset[i].cce_reg_mappingType.shiftIndex = 0;
	    pdcch_vars2->coreset[i].cce_reg_mappingType.reg_bundlesize = 6;
	    pdcch_vars2->coreset[i].cce_reg_mappingType.interleaversize = 1;
	  }
                    
	  pdcch_vars2->coreset[i].precoderGranularity = dci_config->coreset.precoder_granularity;
	  //pdcch_vars2->coreset[i].tciStatesPDCCH;
	  //pdcch_vars2->coreset[i].tciPresentInDCI;
	  pdcch_vars2->coreset[i].pdcchDMRSScramblingID = dci_config->coreset.pdcch_dmrs_scrambling_id;

	}else{  //FAPI_NR_DL_CONFIG_TYPE_DLSCH
	  //  dlsch config pdu

	  fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu = &dl_config->dl_config_list[i].dlsch_config_pdu.dlsch_config_rel15;
	  uint8_t current_harq_pid = dlsch_config_pdu->harq_process_nbr;
	  dlsch0->current_harq_pid = current_harq_pid;
	  dlsch0->active = 1;
	  dlsch0->rnti = dl_config->dl_config_list[i].dlsch_config_pdu.rnti;
                    
	  //dlsch0->harq_processes[0]->mcs = &dlsch_config_pdu->mcs;
                    
	  NR_DL_UE_HARQ_t *dlsch0_harq = dlsch0->harq_processes[current_harq_pid];

                    
	  dlsch0_harq->nb_rb = dlsch_config_pdu->number_rbs;
	  dlsch0_harq->start_rb = dlsch_config_pdu->start_rb;
	  dlsch0_harq->nb_symbols = dlsch_config_pdu->number_symbols;
	  dlsch0_harq->start_symbol = dlsch_config_pdu->start_symbol;
	  dlsch0_harq->mcs = dlsch_config_pdu->mcs;
	  dlsch0_harq->DCINdi = dlsch_config_pdu->ndi;
	  dlsch0_harq->rvidx = dlsch_config_pdu->rv;
	  dlsch0->g_pucch = dlsch_config_pdu->accumulated_delta_PUCCH;
	  dlsch0_harq->harq_ack.pucch_resource_indicator = dlsch_config_pdu->pucch_resource_id;
	  dlsch0_harq->harq_ack.slot_for_feedback_ack = dlsch_config_pdu->pdsch_to_harq_feedback_time_ind;
	  dlsch0_harq->Nl=1;
	  dlsch0_harq->mcs_table=0;
	  dlsch0_harq->status = ACTIVE;
	  LOG_D(MAC,">>>> \tdlsch0->g_pucch=%d\tdlsch0_harq.mcs=%d\n",dlsch0->g_pucch,dlsch0_harq->mcs);

	}
      }
    }else{
      pdcch_vars2->nb_search_space = 0;
    }

    if(scheduled_response->ul_config != NULL){
      fapi_nr_ul_config_request_t *ul_config = scheduled_response->ul_config;
      for(i=0; i<ul_config->number_pdus; ++i){
	if(ul_config->ul_config_list[i].pdu_type == FAPI_NR_UL_CONFIG_TYPE_PUSCH){
	  // pusch config pdu
	  fapi_nr_ul_config_pusch_pdu_rel15_t *pusch_config_pdu = &ul_config->ul_config_list[i].ulsch_config_pdu.ulsch_pdu_rel15;
	  uint8_t current_harq_pid = pusch_config_pdu->harq_process_nbr;
	  ulsch0->harq_processes[current_harq_pid]->nb_rb = pusch_config_pdu->number_rbs;
	  ulsch0->harq_processes[current_harq_pid]->first_rb = pusch_config_pdu->start_rb;
	  ulsch0->harq_processes[current_harq_pid]->number_of_symbols = pusch_config_pdu->number_symbols;
	  ulsch0->harq_processes[current_harq_pid]->start_symbol = pusch_config_pdu->start_symbol;
	  ulsch0->harq_processes[current_harq_pid]->mcs = pusch_config_pdu->mcs;
	  ulsch0->harq_processes[current_harq_pid]->DCINdi = pusch_config_pdu->ndi;
	  ulsch0->harq_processes[current_harq_pid]->rvidx = pusch_config_pdu->rv;
	  ulsch0->harq_processes[current_harq_pid]->Nl = pusch_config_pdu->n_layers;
	  ulsch0->f_pusch = pusch_config_pdu->absolute_delta_PUSCH;
	}
	if(ul_config->ul_config_list[i].pdu_type == FAPI_NR_UL_CONFIG_TYPE_PUCCH){
	  // pucch config pdu
	  fapi_nr_ul_config_pucch_pdu *pucch_config_pdu = &ul_config->ul_config_list[i].pucch_config_pdu;
	  uint8_t pucch_resource_id = 0; //FIXME!!!
	  uint8_t format = 1; // FIXME!!!
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].PUCCH_Resource[pucch_resource_id]->format_parameters.initialCyclicShift = pucch_config_pdu->initialCyclicShift;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].PUCCH_Resource[pucch_resource_id]->format_parameters.nrofSymbols = pucch_config_pdu->nrofSymbols;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].PUCCH_Resource[pucch_resource_id]->format_parameters.startingSymbolIndex = pucch_config_pdu->startingSymbolIndex;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].PUCCH_Resource[pucch_resource_id]->format_parameters.nrofPRBs = pucch_config_pdu->nrofPRBs;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].PUCCH_Resource[pucch_resource_id]->startingPRB = pucch_config_pdu->startingPRB;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].PUCCH_Resource[pucch_resource_id]->format_parameters.timeDomainOCC = pucch_config_pdu->timeDomainOCC;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].PUCCH_Resource[pucch_resource_id]->format_parameters.occ_length = pucch_config_pdu->occ_length;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].PUCCH_Resource[pucch_resource_id]->format_parameters.occ_Index = pucch_config_pdu->occ_Index;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].PUCCH_Resource[pucch_resource_id]->intraSlotFrequencyHopping = pucch_config_pdu->intraSlotFrequencyHopping;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].PUCCH_Resource[pucch_resource_id]->secondHopPRB = pucch_config_pdu->secondHopPRB; // Not sure this parameter is used
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].formatConfig[format-1]->additionalDMRS = pucch_config_pdu->additionalDMRS; // At this point we need to know which format is going to be used
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0].formatConfig[format-1]->pi2PBSK = pucch_config_pdu->pi2PBSK;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_common_nr[0].pucch_GroupHopping = pucch_config_pdu->pucch_GroupHopping;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_common_nr[0].hoppingId = pucch_config_pdu->hoppingId;
	  PHY_vars_UE_g[module_id][cc_id]->pucch_config_common_nr[0].p0_nominal = pucch_config_pdu->p0_nominal;
	  /*                     pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->format_parameters.initialCyclicShift = pucch_config_pdu->initialCyclicShift;
				 pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->format_parameters.nrofSymbols = pucch_config_pdu->nrofSymbols;
				 pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->format_parameters.startingSymbolIndex = pucch_config_pdu->startingSymbolIndex;
				 pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->format_parameters.nrofPRBs = pucch_config_pdu->nrofPRBs;
				 pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->startingPRB = pucch_config_pdu->startingPRB;
				 pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->format_parameters.timeDomainOCC = pucch_config_pdu->timeDomainOCC;
				 pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->format_parameters.occ_length = pucch_config_pdu->occ_length;
				 pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->format_parameters.occ_Index = pucch_config_pdu->occ_Index;
				 pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->intraSlotFrequencyHopping = pucch_config_pdu->intraSlotFrequencyHopping;
				 pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->secondHopPRB = pucch_config_pdu->secondHopPRB; // Not sure this parameter is used
				 pucch_config_dedicated->formatConfig[format-1]->additionalDMRS = pucch_config_pdu->additionalDMRS; // At this point we need to know which format is going to be used
				 pucch_config_dedicated->formatConfig[format-1]->pi2PBSK = pucch_config_pdu->pi2PBSK;
				 pucch_config_common->pucch_GroupHopping = pucch_config_pdu->pucch_GroupHopping;
				 pucch_config_common->hoppingId = pucch_config_pdu->hoppingId;
				 pucch_config_common->p0_nominal = pucch_config_pdu->p0_nominal;*/
	}
	if(ul_config->ul_config_list[i].pdu_type == FAPI_NR_UL_CONFIG_TYPE_PRACH){
	  // prach config pdu
	  fapi_nr_ul_config_prach_pdu *prach_config_pdu = &ul_config->ul_config_list[i].prach_config_pdu;
	  /*frame_parms.prach_config_common.rootSequenceIndex = prach_config_pdu->root_sequence_index;
	  frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex = prach_config_pdu->prach_configuration_index;
	  frame_parms.prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig = prach_config_pdu->zero_correlation_zone_config;
	  frame_parms.prach_config_common.prach_ConfigInfo.highSpeedFlag = prach_config_pdu->restrictedset_config;
	  frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset = prach_config_pdu->prach_freq_offset;*/
	  prach_resources->ra_PreambleIndex = prach_config_pdu->preamble_index;
	}
      }
    }else{
            
    }

    if(scheduled_response->tx_request != NULL){

    }else{
            
    }
  }

  return 0;
}


int8_t nr_ue_phy_config_request(nr_phy_config_t *phy_config){

  fapi_nr_config_request_t *nrUE_config = &PHY_vars_UE_g[phy_config->Mod_id][phy_config->CC_id]->nrUE_config;
  
  if(phy_config != NULL){
    if(phy_config->config_req.config_mask & FAPI_NR_CONFIG_REQUEST_MASK_PBCH){
      LOG_I(MAC,"[L1][IF module][PHY CONFIG]\n");
      LOG_I(MAC,"subcarrier spacing:          %d\n", phy_config->config_req.pbch_config.subcarrier_spacing_common);
      LOG_I(MAC,"ssb carrier offset:          %d\n", phy_config->config_req.pbch_config.ssb_subcarrier_offset);
      LOG_I(MAC,"dmrs type A position:        %d\n", phy_config->config_req.pbch_config.dmrs_type_a_position);
      LOG_I(MAC,"pdcch config sib1:           %d\n", phy_config->config_req.pbch_config.pdcch_config_sib1);
      LOG_I(MAC,"cell barred:                 %d\n", phy_config->config_req.pbch_config.cell_barred);
      LOG_I(MAC,"intra frequency reselection: %d\n", phy_config->config_req.pbch_config.intra_frequency_reselection);
      LOG_I(MAC,"system frame number:         %d\n", phy_config->config_req.pbch_config.system_frame_number);
      LOG_I(MAC,"ssb index:                   %d\n", phy_config->config_req.pbch_config.ssb_index);
      LOG_I(MAC,"half frame bit:              %d\n", phy_config->config_req.pbch_config.half_frame_bit);
      LOG_I(MAC,"-------------------------------\n");

      memcpy(&nrUE_config->pbch_config,&phy_config->config_req.pbch_config,sizeof(fapi_nr_pbch_config_t));
      
    }
        
    if(phy_config->config_req.config_mask & FAPI_NR_CONFIG_REQUEST_MASK_DL_BWP_COMMON){
            
    }

    if(phy_config->config_req.config_mask & FAPI_NR_CONFIG_REQUEST_MASK_UL_BWP_COMMON){
            
    }

    if(phy_config->config_req.config_mask & FAPI_NR_CONFIG_REQUEST_MASK_DL_BWP_DEDICATED){
            
    }

    if(phy_config->config_req.config_mask & FAPI_NR_CONFIG_REQUEST_MASK_UL_BWP_DEDICATED){
            
    }
  }
    
  

  return 0;
}

