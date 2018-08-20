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

	if(scheduled_response != NULL){
		if(scheduled_response->dl_config != NULL){

		}

		if(scheduled_response->ul_config != NULL){

		}

		if(scheduled_response->tx_request != NULL){

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
