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
extern PHY_VARS_NR_UE ***PHY_vars_UE_g;

int8_t nr_ue_scheduled_response(nr_scheduled_response_t *scheduled_response){

    /// module id
    module_id_t module_id = scheduled_response->module_id; 
    /// component carrier id
    uint8_t cc_id = scheduled_response->CC_id;
    uint32_t i;

    if(scheduled_response != NULL){
        PHY_VARS_NR_UE *ue = PHY_vars_UE_g[module_id][cc_id];
        NR_UE_PDCCH *pdcch_vars2 = ue->pdcch_vars[0][0];
        
        if(scheduled_response->dl_config != NULL){
            fapi_nr_dl_config_request_t *dl_config = scheduled_response->dl_config;
            

            for(i=0; i<dl_config->number_pdus; ++i){
                if(dl_config->dl_config_list[i].pdu_type == FAPI_NR_DL_CONFIG_TYPE_DCI){
                    pdcch_vars2->nb_search_space = pdcch_vars2->nb_search_space + 1;
                    fapi_nr_dl_config_dci_dl_pdu_rel15_t *dci_config = &dl_config->dl_config_list[i].dci_config_pdu.dci_config_rel15;
                    
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
                        pdcch_vars2->coreset[i].cce_reg_mappingType.reg_bundlesize = 0;
                        pdcch_vars2->coreset[i].cce_reg_mappingType.interleaversize = 0;
                    }
                    
                    pdcch_vars2->coreset[i].precoderGranularity = dci_config->coreset.precoder_granularity;
                    //pdcch_vars2->coreset[i].tciStatesPDCCH;
                    //pdcch_vars2->coreset[i].tciPresentInDCI;
                    pdcch_vars2->coreset[i].pdcchDMRSScramblingID = dci_config->coreset.pdcch_dmrs_scrambling_id;
                }else{  //FAPI_NR_DL_CONFIG_TYPE_DLSCH
                    //  dlsch config pdu

                    fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_pdu = &dl_config->dl_config_list[i].dlsch_config_pdu.dlsch_config_rel15;
                    NR_UE_DLSCH_t **dlsch = ue->dlsch[ue->current_thread_id[0]][0]; //nr_tti_rx
                    NR_UE_DLSCH_t *dlsch0 = dlsch[0];
                    NR_DL_UE_HARQ_t *dlsch0_harq = dlsch[0]->harq_processes[dlsch_pdu->harq_pid];

                    dlsch0->rnti = dl_config->dl_config_list[i].dlsch_config_pdu.rnti; 
                    dlsch0_harq->start_rb = dlsch_pdu->start_rb;
                    dlsch0_harq->nb_rb = dlsch_pdu->number_rbs;    
                    dlsch0_harq->nb_symbols = dlsch_pdu->number_symbols;
                    dlsch0_harq->nb_symbols = dlsch_pdu->number_symbols;
                    dlsch0_harq->start_symbol = dlsch_pdu->start_symbol;
                    dlsch0->current_harq_pid = dlsch_pdu->harq_pid;
                    dlsch0->active           = 1;
                    dlsch0_harq->mcs = dlsch_pdu->mcs;
                    dlsch0_harq->DCINdi = dlsch_pdu->ndi;
                }
            }
        }else{
            pdcch_vars2->nb_search_space = 0;
        }

        if(scheduled_response->ul_config != NULL){

        }else{
            
        }

        if(scheduled_response->tx_request != NULL){

        }else{
            
        }
    }

    return 0;
}


int8_t nr_ue_phy_config_request(nr_phy_config_t *phy_config){

    if(phy_config != NULL){
        if(phy_config->config_req.config_mask & FAPI_NR_CONFIG_REQUEST_MASK_PBCH){
            printf("[L1][IF module][PHY CONFIG]\n");
            printf("subcarrier spacing:          %d\n", phy_config->config_req.pbch_config.subcarrier_spacing_common);
            printf("ssb carrier offset:          %d\n", phy_config->config_req.pbch_config.ssb_subcarrier_offset);
            printf("dmrs type A position:        %d\n", phy_config->config_req.pbch_config.dmrs_type_a_position);
            printf("pdcch config sib1:           %d\n", phy_config->config_req.pbch_config.pdcch_config_sib1);
            printf("cell barred:                 %d\n", phy_config->config_req.pbch_config.cell_barred);
            printf("intra frequcney reselection: %d\n", phy_config->config_req.pbch_config.intra_frequency_reselection);
            printf("system frame number:         %d\n", phy_config->config_req.pbch_config.system_frame_number);
            printf("ssb index:                   %d\n", phy_config->config_req.pbch_config.ssb_index);
            printf("half frame bit:              %d\n", phy_config->config_req.pbch_config.half_frame_bit);
            printf("-------------------------------\n");

            PHY_vars_UE_g[0][0]->proc.proc_rxtx[0].frame_rx = phy_config->config_req.pbch_config.system_frame_number;
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
