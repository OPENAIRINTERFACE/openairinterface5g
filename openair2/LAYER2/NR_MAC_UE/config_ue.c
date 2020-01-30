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

/* \file config_ue.c
 * \brief UE and eNB configuration performed by RRC or as a consequence of RRC procedures
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

//#include "mac_defs.h"
#include "mac_proto.h"

#include "NR_MAC-CellGroupConfig.h"

int nr_rrc_mac_config_req_ue(
    module_id_t                     module_id,
    int                             cc_idP,
    uint8_t                         gNB_index,
    NR_MIB_t                        *mibP,
    NR_MAC_CellGroupConfig_t        *mac_cell_group_configP,
    NR_PhysicalCellGroupConfig_t    *phy_cell_group_configP,
    NR_SpCellConfig_t               *spcell_configP ){

    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

//    NR_ServingCellConfig_t *serving_cell_config = spcell_configP->spCellConfigDedicated;
//  TODO do something FAPI-like P5 L1/L2 config interface in config_si, config_mib, etc.

    if(mibP != NULL){
        mac->mib = mibP;    //  update by every reception
    }

    if(mac_cell_group_configP != NULL){
        if(mac_cell_group_configP->drx_Config != NULL ){
            switch(mac_cell_group_configP->drx_Config->present){
                case NR_SetupRelease_DRX_Config_PR_NOTHING:
                    break;
                case NR_SetupRelease_DRX_Config_PR_release:
                    mac->drx_Config = NULL;
                    break;
                case NR_SetupRelease_DRX_Config_PR_setup:
                    mac->drx_Config = mac_cell_group_configP->drx_Config->choice.setup;
                    break;
                default:
                    break;
            }
        }

        if(mac_cell_group_configP->schedulingRequestConfig != NULL ){
            mac->schedulingRequestConfig = mac_cell_group_configP->schedulingRequestConfig;
        }

        if(mac_cell_group_configP->bsr_Config != NULL ){
            mac->bsr_Config = mac_cell_group_configP->bsr_Config;
        }

        if(mac_cell_group_configP->tag_Config != NULL ){
            mac->tag_Config = mac_cell_group_configP->tag_Config;
        }

        if(mac_cell_group_configP->phr_Config != NULL ){
            switch(mac_cell_group_configP->phr_Config->present){
                case NR_SetupRelease_PHR_Config_PR_NOTHING:
                    break;
                case NR_SetupRelease_PHR_Config_PR_release:
                    mac->phr_Config = NULL;
                    break;
                case NR_SetupRelease_PHR_Config_PR_setup:
                    mac->phr_Config = mac_cell_group_configP->phr_Config->choice.setup;
                    break;
                default:
                    break;
            }
            
        }

        if(phy_cell_group_configP->cs_RNTI != NULL ){
            switch(phy_cell_group_configP->cs_RNTI->present){
                case NR_SetupRelease_RNTI_Value_PR_NOTHING:
                    break;
                case NR_SetupRelease_RNTI_Value_PR_release:
                    mac->cs_RNTI = NULL;
                    break;
                case NR_SetupRelease_RNTI_Value_PR_setup:
                    mac->cs_RNTI = &phy_cell_group_configP->cs_RNTI->choice.setup;
                    break;
                default:
                    break;
            }
            
        }
	
    }
    
    if(phy_cell_group_configP != NULL ){
        //config_phy(phy_cell_group_config, NULL);
    }

//  TODO check
#if 0
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
#endif
    //scell config not yet

    return 0;
}
