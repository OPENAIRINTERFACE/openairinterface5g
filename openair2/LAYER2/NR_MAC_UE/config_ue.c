
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

int
nr_rrc_mac_config_req_ue(
    module_id_t Mod_idP,
	int CC_idP,
	uint8_t eNB_index,
    MAC_CellGroupConfig_t *mac_cell_group_config,
    PhysicalCellGroupConfig_t *phy_cell_group_config,
    SpCellConfig_t *spcell_config){

    NR_UE_MAC_INST *mac = get_mac_inst(Mod_idP);

    ServingCellConfig_t *serving_cell_config = spcell_config->spCellConfigDedicated;
//  TODO do something FAPI-like P5 L1/L2 config interface in config_si, config_mib, etc.

    if(mac_cell_group_config != (MAC_CellGroupConfig_t *)0){
        if(mac_cell_group_config->drx_Config != (drx_Config_t *)0 ){
            mac->drx_Config = mac_cell_group_config->drx_Config;
        }

        if(mac_cell_group_config->SchedulingRequestConfig != (SchedulingRequestConfig_t *)0 ){
            mac->SchedulingRequestConfig = mac_cell_group_config->SchedulingRequestConfig;
        }

        if(mac_cell_group_config->BSR_Config != (BSR_Config_t *)0 ){
            mac->BSR_Config = mac_cell_group_config->BSR_Config;
        }

        if(mac_cell_group_config->TAG_Config != (TAG_Config_t *)0 ){
            mac->TAG_Config = mac_cell_group_config->TAG_Config;
        }

        if(mac_cell_group_config->phr_Config != (phr_Config_t *)0 ){
            mac->phr_Config = mac_cell_group_config->phr_Config;
        }

        if(mac_cell_group_config->cs_RNTI != (cs_RNTI_t *)0 ){
            mac->cs_RNTI = mac_cell_group_config->cs_RNTI;
        }
    }
    
    if(phy_cell_group_config != (PhysicalCellGroupConfig_t *)0){
        config_phy(phy_cell_group_config, NULL);
    }

    if(serving_cell_config_config != (SpCellConfig_t *)0){
        config_phy(NULL, spcell_config);
        mac->servCellIndex = spcell_config->servCellIndex;
    }


    if(serving_cell_config != (spCellConfigDedicated_t *)0){
        if(serving_cell_config->tdd_UL_DL_ConfigurationDedicated != (TDD_UL_DL_ConfigDedicated_t *)0){
            mac->tdd_UL_DL_ConfigurationDedicated = serving_cell_config->tdd_UL_DL_ConfigurationDedicated;
        }
        
        if(spcell_config->initialDownlinkBWP != (BWP_DownlinkDedicated_t *)0){
            mac->init_DL_BWP = spcell_config->initialDownlinkBWP;
        }
        
        //  storage list of DL BWP config. TODO should be modify to maintain(add/release) a list inside MAC instance, this implementation just use for one-shot RRC configuration setting.
        if(spcell_config->downlinkBWP_ToAddModList != (struct ServingCellConfig__downlinkBWP_ToAddModList *)0){
            mac->BWP_Downlink_list = spcell_config->downlinkBWP_ToAddModList->list;
            mac->BWP_Downlink_count = spcell_config->downlinkBWP_ToAddModList->count;
        }
        
        if(spcell_config->bwp_InactivityTimer != (long *)0){
            mac->bwp_InactivityTimer = spcell_config->bwp_InactivityTimer;
        } 

        if(spcell_config->defaultDownlinkBWP_Id != (BWP_Id_t *)0){
            mac->defaultDownlinkBWP_Id = spcell_config->defaultDownlinkBWP_Id;
        }

        if(spcell_config->pdsch_ServingCellConfig != (PDSCH_ServingCellConfig_t *)0){
            mac->pdsch_ServingCellConfig = spcell_config->pdsch_ServingCellConfig;
        }

        if(spcell_config->csi_MeasConfig != (CSI_MeasConfig_t *)0){
            mac->csi_MeasConfig = spcell_config->csi_MeasConfig;
        }

        spcell_config->tag_Id = spcell_config.tag_Id;
    }

    //scell config not yet







    return (0);
}
