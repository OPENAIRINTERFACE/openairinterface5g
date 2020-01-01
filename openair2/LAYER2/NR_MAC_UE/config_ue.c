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




void config_common_ue(NR_UE_MAC_INST_t *mac) {

  fapi_nr_config_request_t        *cfg = &mac->phy_config.config_req;
  NR_ServingCellConfigCommon_t    *scc = mac->scc;
/*
  mac->if_module->phy_config_request(&mac->phy_config);
  cfg->sch_config.physical_cell_id = *scc->physCellId;
  cfg->sch_config.ssb_scg_position_in_burst = scc->ssb_PositionsInBurst->choice.mediumBitmap.buf[0];

  cfg->subframe_config.duplex_mode                          = 1;

  cfg->fapi_config.rf_bands.number_rf_bands       = 1;
  cfg->fapi_config.rf_bands.rf_band[0]            = *(long*)scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];  

  cfg->fapi_config.nrarfcn                  = scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA;

  //  cfg->subframe_config.numerology_index_mu = 1;

  cfg->rf_config.dl_carrier_bandwidth    = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
  LOG_I(PHY,"%s() dl_BandwidthP:%d\n", __FUNCTION__, cfg->rf_config.dl_carrier_bandwidth);

  cfg->rf_config.ul_carrier_bandwidth    = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;

  cfg->rf_config.dl_subcarrierspacing    = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  cfg->rf_config.ul_subcarrierspacing    = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;


  cfg->rf_config.dl_offsettocarrier    = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;

  cfg->rf_config.ul_offsettocarrier    = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
 
  // InitialBWP configuration

  cfg->initialBWP_config.dl_bandwidth    = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,275);

  cfg->initialBWP_config.dl_offset    = NRRIV2PRBOFFSET(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,275);

  cfg->initialBWP_config.dl_subcarrierSpacing    = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing;

  LOG_I(PHY,"%s() initialBWP_dl_Bandwidth.RBstart.SCS :%d.%d.%d\n", __FUNCTION__, cfg->initialBWP_config.dl_bandwidth,cfg->initialBWP_config.dl_offset,cfg->initialBWP_config.dl_subcarrierSpacing);

  cfg->initialBWP_config.ul_bandwidth    = NRRIV2BW(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth,275);

  cfg->initialBWP_config.ul_offset    = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth,275);

  cfg->initialBWP_config.ul_subcarrierSpacing    = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;

  LOG_I(PHY,"%s() initialBWP_ul_Bandwidth.RBstart.SCS :%d.%d.%d\n", __FUNCTION__, cfg->initialBWP_config.ul_bandwidth,cfg->initialBWP_config.ul_offset,cfg->initialBWP_config.ul_subcarrierSpacing);


  cfg->rach_config.prach_RootSequenceIndex = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l139;
  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing)
    cfg->rach_config.prach_msg1_SubcarrierSpacing = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing;
  else cfg->rach_config.prach_msg1_SubcarrierSpacing=cfg->rf_config.dl_subcarrierspacing;

  cfg->rach_config.restrictedSetConfig = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig;
  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder)
    cfg->rach_config.msg3_transformPrecoding = 1;
  else cfg->rach_config.msg3_transformPrecoding = 0;

  cfg->rach_config.prach_ConfigurationIndex = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex;  
  cfg->rach_config.prach_msg1_FDM = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM;            
  cfg->rach_config.prach_msg1_FrequencyStart = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart; 
  cfg->rach_config.zeroCorrelationZoneConfig = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig;
  cfg->rach_config.preambleReceivedTargetPower = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleReceivedTargetPower;

  // PDCCH-ConfigCommon
  cfg->pdcch_config.controlResourceSetZero = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero;
  cfg->pdcch_config.searchSpaceZero = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero;

  // PDSCH-ConfigCommon
  cfg->pdsch_config.num_PDSCHTimeDomainResourceAllocations = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count;
  cfg->pdsch_config.dmrs_TypeA_Position = scc->dmrs_TypeA_Position;
  AssertFatal(cfg->pdsch_config.num_PDSCHTimeDomainResourceAllocations<=NFAPI_NR_PDSCH_CONFIG_MAXALLOCATIONS,"illegal TimeDomainAllocation count %d\n",cfg->pdsch_config.num_PDSCHTimeDomainResourceAllocations);
  for (int i=0;i<cfg->pdsch_config.num_PDSCHTimeDomainResourceAllocations;i++) {
    cfg->pdsch_config.PDSCHTimeDomainResourceAllocation_k0[i]=*scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0;
    cfg->pdsch_config.PDSCHTimeDomainResourceAllocation_mappingType[i]=scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->mappingType;
    cfg->pdsch_config.PDSCHTimeDomainResourceAllocation_startSymbolAndLength[i]=scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
  }

  // PUSCH-ConfigCommon
  cfg->pusch_config.num_PUSCHTimeDomainResourceAllocations = scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.count;
  cfg->pusch_config.dmrs_TypeA_Position = scc->dmrs_TypeA_Position+2;
  AssertFatal(cfg->pusch_config.num_PUSCHTimeDomainResourceAllocations<=NFAPI_NR_PUSCH_CONFIG_MAXALLOCATIONS,"illegal TimeDomainAllocation count %d\n",cfg->pusch_config.num_PUSCHTimeDomainResourceAllocations);
  for (int i=0;i<cfg->pusch_config.num_PUSCHTimeDomainResourceAllocations;i++) {
    cfg->pusch_config.PUSCHTimeDomainResourceAllocation_k2[i]=*scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[0]->k2;
  }
*/
}

int nr_rrc_mac_config_req_ue(
    module_id_t                     module_id,
    int                             cc_idP,
    uint8_t                         gNB_index,
    NR_MIB_t                        *mibP,
    NR_ServingCellConfigCommon_t    *sccP,
    //    NR_MAC_CellGroupConfig_t        *mac_cell_group_configP,
    //    NR_PhysicalCellGroupConfig_t    *phy_cell_group_configP,
    NR_SpCellConfig_t               *spCell_ConfigP ){

    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

//    NR_ServingCellConfig_t *serving_cell_config = spcell_configP->spCellConfigDedicated;
//  TODO do something FAPI-like P5 L1/L2 config interface in config_si, config_mib, etc.

    NR_ServingCellConfigCommon_t    *scc;

    if(mibP != NULL){
      mac->mib = mibP;    //  update by every reception
    }


    
    
    if(spCell_ConfigP != NULL ){
      mac->servCellIndex = spCell_ConfigP->servCellIndex;
      if (spCell_ConfigP->reconfigurationWithSync) {
	mac->scc = spCell_ConfigP->reconfigurationWithSync->spCellConfigCommon;
	config_common_ue(mac);
	mac->crnti = spCell_ConfigP->reconfigurationWithSync->newUE_Identity;
	LOG_I(MAC,"Configuring CRNTI %x\n",mac->crnti);
      }
      mac->scd = spCell_ConfigP->spCellConfigDedicated;

      /*      
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
      }
      
      
      if(phy_cell_group_configP != NULL ){
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
      */
    }   
    
    return 0;

}
