
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

/*! \file config.c
 * \brief UE and eNB configuration performed by RRC or as a consequence of RRC procedures
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \version 0.1
 * \email: navid.nikaein@eurecom.fr
 * @ingroup _mac

 */

#include "mac_proto.h"

int
nr_rrc_mac_config_req_ue(
    module_id_t                     module_id,
	int                             CC_idP,
	uint8_t                         gNB_index,
    NR_MIB_t                        *mibP,
    NR_MAC_CellGroupConfig_t        *mac_cell_group_configP,
    NR_PhysicalCellGroupConfig_t    *phy_cell_group_configP,
    NR_SpCellConfig_t               *spcell_configP ){

    NR_UE_MAC_INST *mac = get_mac_inst(module_id);

    ServingCellConfig_t *serving_cell_config = spcell_config->spCellConfigDedicated;
//  TODO do something FAPI-like P5 L1/L2 config interface in config_si, config_mib, etc.

    if(mib != NULL){
        
    }

    if(mac_cell_group_configP != NULL){
        if(mac_cell_group_configP->drx_Config != NULL ){
            mac->drx_Config = mac_cell_group_configP->drx_Config;
        }

        if(mac_cell_group_configP->SchedulingRequestConfig != NULL ){
            mac->SchedulingRequestConfig = mac_cell_group_configP->SchedulingRequestConfig;
        }

        if(mac_cell_group_configP->BSR_Config != NULL ){
            mac->BSR_Config = mac_cell_group_configP->BSR_Config;
        }

        if(mac_cell_group_configP->TAG_Config != NULL ){
            mac->TAG_Config = mac_cell_group_configP->TAG_Config;
        }

        if(mac_cell_group_configP->phr_Config != NULL ){
            mac->phr_Config = mac_cell_group_configP->phr_Config;
        }

        if(mac_cell_group_configP->cs_RNTI != NULL ){
            mac->cs_RNTI = mac_cell_group_configP->cs_RNTI;
        }
    }
    
    if(phy_cell_group_configP != NULL ){
        //config_phy(phy_cell_group_config, NULL);
    }

//  TODO check
/*
    if(serving_cell_config_configP != NULL ){
        //config_phy(NULL, spcell_config);
        mac->servCellIndex = spcell_config->servCellIndex;
    }


    if(serving_cell_config != NULL ){
        if(serving_cell_config->tdd_UL_DL_ConfigurationDedicated != NULL ){
            mac->tdd_UL_DL_ConfigurationDedicated = serving_cell_config->tdd_UL_DL_ConfigurationDedicated;
        }
        
        if(spcell_config->initialDownlinkBWP != NULL ){
            mac->init_DL_BWP = spcell_config->initialDownlinkBWP;
        }
        
        //  storage list of DL BWP config. TODO should be modify to maintain(add/release) a list inside MAC instance, this implementation just use for one-shot RRC configuration setting.
        if(spcell_config->downlinkBWP_ToAddModList != NULL ){
            mac->BWP_Downlink_list = spcell_config->downlinkBWP_ToAddModList->list;
            mac->BWP_Downlink_count = spcell_config->downlinkBWP_ToAddModList->count;
        }
        
        if(spcell_config->bwp_InactivityTimer != NULL ){
            mac->bwp_InactivityTimer = spcell_config->bwp_InactivityTimer;
        } 

        if(spcell_config->defaultDownlinkBWP_Id != NULL ){
            mac->defaultDownlinkBWP_Id = spcell_config->defaultDownlinkBWP_Id;
        }

        if(spcell_config->pdsch_ServingCellConfig != NULL ){
            mac->pdsch_ServingCellConfig = spcell_config->pdsch_ServingCellConfig;
        }

        if(spcell_config->csi_MeasConfig != NULL ){
            mac->csi_MeasConfig = spcell_config->csi_MeasConfig;
        }

        spcell_config->tag_Id = spcell_config.tag_Id;
    }
*/
    //scell config not yet

    return (0);
}
