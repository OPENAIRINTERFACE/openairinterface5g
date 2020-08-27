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
#include "harq_nr.h"
//#include "PHY/phy_vars_nr_ue.h"

#include "PHY/defs_nr_UE.h"
#include "PHY/impl_defs_nr.h"

extern PHY_VARS_NR_UE ***PHY_vars_UE_g;

int8_t nr_ue_scheduled_response(nr_scheduled_response_t *scheduled_response){

  if(scheduled_response != NULL){

    module_id_t module_id = scheduled_response->module_id;
    uint8_t cc_id = scheduled_response->CC_id, thread_id;
    uint32_t i;
    int slot = scheduled_response->slot;

    // Note: we have to handle the thread IDs for this. To be revisited completely.
    thread_id = PHY_vars_UE_g[module_id][cc_id]->current_thread_id[slot];
    NR_UE_DLSCH_t *dlsch0 = NULL;
    NR_UE_PDCCH *pdcch_vars = PHY_vars_UE_g[module_id][cc_id]->pdcch_vars[thread_id][0];
    NR_UE_ULSCH_t *ulsch0 = PHY_vars_UE_g[module_id][cc_id]->ulsch[thread_id][0][0];
    NR_DL_FRAME_PARMS frame_parms = PHY_vars_UE_g[module_id][cc_id]->frame_parms;
    //PRACH_RESOURCES_t *prach_resources = PHY_vars_UE_g[module_id][cc_id]->prach_resources[0];
        
    //        PUCCH_ConfigCommon_nr_t    *pucch_config_common = PHY_vars_UE_g[module_id][cc_id]->pucch_config_common_nr[0];
    //        PUCCH_Config_t             *pucch_config_dedicated = PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0];

    if(scheduled_response->dl_config != NULL){
      fapi_nr_dl_config_request_t *dl_config = scheduled_response->dl_config;

      LOG_D(PHY,"Received %d DL pdus\n",dl_config->number_pdus);
      pdcch_vars->nb_search_space = 0;

      for (i = 0; i < dl_config->number_pdus; ++i){

        if (dl_config->dl_config_list[i].pdu_type == FAPI_NR_DL_CONFIG_TYPE_DCI) {

          fapi_nr_dl_config_dci_dl_pdu_rel15_t *pdcch_config = &dl_config->dl_config_list[i].dci_config_pdu.dci_config_rel15;
          memcpy((void*)&pdcch_vars->pdcch_config[pdcch_vars->nb_search_space],(void*)pdcch_config,sizeof(*pdcch_config));
          pdcch_vars->nb_search_space = pdcch_vars->nb_search_space + 1;
          LOG_D(PHY,"Number of DCI SearchSpaces %d\n",pdcch_vars->nb_search_space);

        } else {

          if (dl_config->dl_config_list[i].pdu_type == FAPI_NR_DL_CONFIG_TYPE_DLSCH){
            dlsch0 = PHY_vars_UE_g[module_id][cc_id]->dlsch[thread_id][0][0];
          }
          else if (dl_config->dl_config_list[i].pdu_type == FAPI_NR_DL_CONFIG_TYPE_RA_DLSCH){
            dlsch0 = PHY_vars_UE_g[module_id][cc_id]->dlsch_ra[0];
          }

          fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config_pdu = &dl_config->dl_config_list[i].dlsch_config_pdu.dlsch_config_rel15;
          uint8_t current_harq_pid = dlsch_config_pdu->harq_process_nbr;
          NR_DL_UE_HARQ_t *dlsch0_harq;

          dlsch0->current_harq_pid = current_harq_pid;
          dlsch0->active = 1;
          dlsch0->rnti = dl_config->dl_config_list[i].dlsch_config_pdu.rnti;
          //dlsch0->harq_processes[0]->mcs = &dlsch_config_pdu->mcs;
          dlsch0_harq = dlsch0->harq_processes[current_harq_pid];

          if (dlsch0_harq){

            dlsch0_harq->BWPStart = dlsch_config_pdu->BWPStart;
            dlsch0_harq->BWPSize = dlsch_config_pdu->BWPSize;
            dlsch0_harq->nb_rb = dlsch_config_pdu->number_rbs;
            dlsch0_harq->start_rb = dlsch_config_pdu->start_rb;
            dlsch0_harq->nb_symbols = dlsch_config_pdu->number_symbols;
            dlsch0_harq->start_symbol = dlsch_config_pdu->start_symbol;
            dlsch0_harq->dlDmrsSymbPos = dlsch_config_pdu->dlDmrsSymbPos;
            dlsch0_harq->dmrsConfigType = dlsch_config_pdu->dmrsConfigType;
            dlsch0_harq->n_dmrs_cdm_groups = dlsch_config_pdu->n_dmrs_cdm_groups;
            dlsch0_harq->mcs = dlsch_config_pdu->mcs;
            dlsch0_harq->rvidx = dlsch_config_pdu->rv;
            dlsch0->g_pucch = dlsch_config_pdu->accumulated_delta_PUCCH;
            dlsch0_harq->harq_ack.pucch_resource_indicator = dlsch_config_pdu->pucch_resource_id;
            dlsch0_harq->harq_ack.slot_for_feedback_ack = (slot+dlsch_config_pdu->pdsch_to_harq_feedback_time_ind)%frame_parms.slots_per_frame;
            dlsch0_harq->Nl=1;
            dlsch0_harq->mcs_table=0;
            dlsch0_harq->harq_ack.rx_status = downlink_harq_process(dlsch0_harq, dlsch0->current_harq_pid, dlsch_config_pdu->ndi, dlsch0->rnti_type);
            dlsch0_harq->harq_ack.vDAI_DL = dlsch_config_pdu->dai;
            LOG_D(MAC, ">>>> \tdlsch0->g_pucch = %d\tdlsch0_harq.mcs = %d\n", dlsch0->g_pucch, dlsch0_harq->mcs);
          }
        }
      }
    } else {
      pdcch_vars->nb_search_space = 0;
    }

    if (scheduled_response->ul_config != NULL){

      fapi_nr_ul_config_request_t *ul_config = scheduled_response->ul_config;

      for (i = 0; i < ul_config->number_pdus; ++i){

        uint8_t pdu_type = ul_config->ul_config_list[i].pdu_type, pucch_resource_id, current_harq_pid, format, gNB_id = 0;
        /* PRACH */
        //NR_PRACH_RESOURCES_t *prach_resources;
        fapi_nr_ul_config_prach_pdu *prach_config_pdu;
        /* PUSCH */
        nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu;
        /* PUCCH */
        fapi_nr_ul_config_pucch_pdu *pucch_config_pdu;
        PUCCH_ConfigCommon_nr_t *pucch_config_common_nr;
        PUCCH_Config_t *pucch_config_dedicated_nr;
        PUCCH_format_t *format_params;

        switch (pdu_type){

        case (FAPI_NR_UL_CONFIG_TYPE_PUSCH):
          // pusch config pdu
          pusch_config_pdu = &ul_config->ul_config_list[i].pusch_config_pdu;
          current_harq_pid = pusch_config_pdu->pusch_data.harq_process_id;
          NR_UL_UE_HARQ_t *harq_process_ul_ue = ulsch0->harq_processes[current_harq_pid];

          if (harq_process_ul_ue){

            nfapi_nr_ue_pusch_pdu_t *pusch_pdu = &harq_process_ul_ue->pusch_pdu;

            memcpy(pusch_pdu, pusch_config_pdu, sizeof(nfapi_nr_ue_pusch_pdu_t));

            ulsch0->f_pusch = pusch_config_pdu->absolute_delta_PUSCH;

            if (scheduled_response->tx_request){
              fapi_nr_tx_request_body_t *tx_req_body = scheduled_response->tx_request->tx_request_body;

              //harq_process_ul_ue->a = (unsigned char*)calloc(TBS/8, sizeof(unsigned char));
              memcpy(harq_process_ul_ue->a, tx_req_body->pdu, tx_req_body->pdu_length);

              harq_process_ul_ue->status = ACTIVE;
            }

          } else {

            LOG_E(PHY, "[phy_procedures_nrUE_TX] harq_process_ul_ue is NULL !!\n");
            return -1;

          }

        break;

        case (FAPI_NR_UL_CONFIG_TYPE_PUCCH):
          // pucch config pdu
          pucch_config_pdu = &ul_config->ul_config_list[i].pucch_config_pdu;
          pucch_resource_id = 0; //FIXME!!!
          format = 1; // FIXME!!!
          pucch_config_common_nr = &PHY_vars_UE_g[module_id][cc_id]->pucch_config_common_nr[0];
          pucch_config_dedicated_nr = &PHY_vars_UE_g[module_id][cc_id]->pucch_config_dedicated_nr[0];
          format_params = &pucch_config_dedicated_nr->PUCCH_Resource[pucch_resource_id]->format_parameters;

          format_params->initialCyclicShift = pucch_config_pdu->initialCyclicShift;
          format_params->nrofSymbols = pucch_config_pdu->nrofSymbols;
          format_params->startingSymbolIndex = pucch_config_pdu->startingSymbolIndex;
          format_params->nrofPRBs = pucch_config_pdu->nrofPRBs;
          format_params->timeDomainOCC = pucch_config_pdu->timeDomainOCC;
          format_params->occ_length = pucch_config_pdu->occ_length;
          format_params->occ_Index = pucch_config_pdu->occ_Index;

          pucch_config_dedicated_nr->PUCCH_Resource[pucch_resource_id]->startingPRB = pucch_config_pdu->startingPRB;
          pucch_config_dedicated_nr->PUCCH_Resource[pucch_resource_id]->intraSlotFrequencyHopping = pucch_config_pdu->intraSlotFrequencyHopping;
          pucch_config_dedicated_nr->PUCCH_Resource[pucch_resource_id]->secondHopPRB = pucch_config_pdu->secondHopPRB; // Not sure this parameter is used
          pucch_config_dedicated_nr->formatConfig[format - 1]->additionalDMRS = pucch_config_pdu->additionalDMRS; // At this point we need to know which format is going to be used
          pucch_config_dedicated_nr->formatConfig[format - 1]->pi2PBSK = pucch_config_pdu->pi2PBSK;

          pucch_config_common_nr->pucch_GroupHopping = pucch_config_pdu->pucch_GroupHopping;
          pucch_config_common_nr->hoppingId = pucch_config_pdu->hoppingId;
          pucch_config_common_nr->p0_nominal = pucch_config_pdu->p0_nominal;

          /* pucch_config_dedicated->PUCCH_Resource[pucch_resource_id]->format_parameters.initialCyclicShift = pucch_config_pdu->initialCyclicShift;
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
        break;

        case (FAPI_NR_UL_CONFIG_TYPE_PRACH):
          // prach config pdu
          //prach_resources = PHY_vars_UE_g[module_id][cc_id]->prach_resources[gNB_id];
          prach_config_pdu = &ul_config->ul_config_list[i].prach_config_pdu;
          memcpy((void*)&(PHY_vars_UE_g[module_id][cc_id]->prach_vars[gNB_id]->prach_pdu), (void*)prach_config_pdu, sizeof(fapi_nr_ul_config_prach_pdu));
          PHY_vars_UE_g[module_id][cc_id]->prach_vars[gNB_id]->prach_Config_enabled = 1;
        break;

        default:
        break;
        }
      }
    } else {
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


