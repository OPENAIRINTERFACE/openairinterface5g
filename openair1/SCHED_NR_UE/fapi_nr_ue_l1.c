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


  if(scheduled_response != NULL){
    /// module id
    module_id_t module_id = scheduled_response->module_id; 
    /// component carrier id
    uint8_t cc_id = scheduled_response->CC_id;
    uint32_t i;
    int slot = scheduled_response->slot; 	
    // Note: we have to handle the thread IDs for this. To be revisited completely.
    uint8_t thread_id = PHY_vars_UE_g[module_id][cc_id]->current_thread_id[slot];
    NR_UE_PDCCH *pdcch_vars = PHY_vars_UE_g[module_id][cc_id]->pdcch_vars[thread_id][0];
    NR_UE_DLSCH_t *dlsch0 = PHY_vars_UE_g[module_id][cc_id]->dlsch[thread_id][0][0];
    NR_UE_ULSCH_t *ulsch0 = PHY_vars_UE_g[module_id][cc_id]->ulsch[thread_id][0][0];
    //NR_DL_FRAME_PARMS frame_parms = PHY_vars_UE_g[module_id][cc_id]->frame_parms;
    PRACH_RESOURCES_t *prach_resources = PHY_vars_UE_g[module_id][cc_id]->prach_resources[0];
        
    //        PUCCH_ConfigCommon_nr_t    *pucch_config_common = PHY_vars_UE_g[module_id][cc_id]->pucch_config_common_nr[0];
    //        PUCCH_Config_t             *pucch_config_dedicated = PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0];

    if(scheduled_response->dl_config != NULL){
      fapi_nr_dl_config_request_t *dl_config = scheduled_response->dl_config;

      LOG_D(PHY,"Received %d DL pdus\n",dl_config->number_pdus);
      pdcch_vars->nb_search_space = 0;
      for(i=0; i<dl_config->number_pdus; ++i){
	if(dl_config->dl_config_list[i].pdu_type == FAPI_NR_DL_CONFIG_TYPE_DCI){
	  fapi_nr_dl_config_dci_dl_pdu_rel15_t *pdcch_config = &dl_config->dl_config_list[i].dci_config_pdu.dci_config_rel15;
	  memcpy((void*)&pdcch_vars->pdcch_config[pdcch_vars->nb_search_space],(void*)pdcch_config,sizeof(*pdcch_config));
	  pdcch_vars->nb_search_space = pdcch_vars->nb_search_space + 1;
	  LOG_D(PHY,"Number of DCI SearchSpaces %d\n",pdcch_vars->nb_search_space);
	}else{  //FAPI_NR_DL_CONFIG_TYPE_DLSCH
	  //  dlsch config pdu

	  fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu = &dl_config->dl_config_list[i].dlsch_config_pdu.dlsch_config_rel15;
	  uint8_t current_harq_pid = dlsch_config_pdu->harq_process_nbr;
	  dlsch0->current_harq_pid = current_harq_pid;
	  dlsch0->active = 1;
	  dlsch0->rnti = dl_config->dl_config_list[i].dlsch_config_pdu.rnti;
                    
	  //dlsch0->harq_processes[0]->mcs = &dlsch_config_pdu->mcs;
                    
	  NR_DL_UE_HARQ_t *dlsch0_harq = dlsch0->harq_processes[current_harq_pid];

	  dlsch0_harq->BWPStart = dlsch_config_pdu->BWPStart;
	  dlsch0_harq->BWPSize = dlsch_config_pdu->BWPSize;
	  dlsch0_harq->nb_rb = dlsch_config_pdu->number_rbs;
	  dlsch0_harq->start_rb = dlsch_config_pdu->start_rb;
	  dlsch0_harq->nb_symbols = dlsch_config_pdu->number_symbols;
	  dlsch0_harq->start_symbol = dlsch_config_pdu->start_symbol;
	  dlsch0_harq->dlDmrsSymbPos = dlsch_config_pdu->dlDmrsSymbPos;
	  dlsch0_harq->dmrsConfigType = dlsch_config_pdu->dmrsConfigType;
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
      pdcch_vars->nb_search_space = 0;
    }

    if(scheduled_response->ul_config != NULL){
      fapi_nr_ul_config_request_t *ul_config = scheduled_response->ul_config;
      for(i=0; i<ul_config->number_pdus; ++i){
	if(ul_config->ul_config_list[i].pdu_type == FAPI_NR_UL_CONFIG_TYPE_PUSCH){
	  // pusch config pdu
	  nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu = &ul_config->ul_config_list[i].pusch_config_pdu;
	  uint8_t current_harq_pid = pusch_config_pdu->pusch_data.harq_process_id;
	  nfapi_nr_ue_pusch_pdu_t *pusch_pdu = &ulsch0->harq_processes[current_harq_pid]->pusch_pdu;

	  memcpy(pusch_pdu, pusch_config_pdu, sizeof(nfapi_nr_ue_pusch_pdu_t));

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
  
  if(phy_config != NULL)
      memcpy(nrUE_config,&phy_config->config_req,sizeof(fapi_nr_config_request_t));
    
  return 0;
}


