
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

//  TODO do something FAPI-like P5 L1/L2 config interface in config_si, config_mib, etc.

    if(mac_cell_group_config != (MAC_CellGroupConfig_t *)0){
        if(mac_cell_group_config->drx_Config != (drx_Config_t *)0 ){
            NR_UE_mac_inst->drx_Config = mac_cell_group_config->drx_Config;
        }

        if(mac_cell_group_config->SchedulingRequestConfig != (SchedulingRequestConfig_t *)0 ){
            NR_UE_mac_inst->SchedulingRequestConfig = mac_cell_group_config->SchedulingRequestConfig;
        }

        if(mac_cell_group_config->BSR_Config != (BSR_Config_t *)0 ){
            NR_UE_mac_inst->BSR_Config = mac_cell_group_config->BSR_Config;
        }

        if(mac_cell_group_config->TAG_Config != (TAG_Config_t *)0 ){
            NR_UE_mac_inst->TAG_Config = mac_cell_group_config->TAG_Config;
        }

        if(mac_cell_group_config->phr_Config != (phr_Config_t *)0 ){
            NR_UE_mac_inst->phr_Config = mac_cell_group_config->phr_Config;
        }

        if(mac_cell_group_config->cs_RNTI != (cs_RNTI_t *)0 ){
            NR_UE_mac_inst->cs_RNTI = mac_cell_group_config->cs_RNTI;
        }


    }
    
    if(phy_cell_group_config != (PhysicalCellGroupConfig_t *)0){
    
    }

    if(spcell_config != (SpCellConfig_t *)0){
    
    }

    return (0);
}
