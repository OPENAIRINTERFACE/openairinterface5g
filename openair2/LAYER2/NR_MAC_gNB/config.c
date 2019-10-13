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
 * \brief gNB configuration performed by RRC or as a consequence of RRC procedures
 * \author  Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \version 0.1
 * \company Eurecom, NTUST
 * \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
 * @ingroup _mac

 */

#include "COMMON/platform_types.h"
#include "COMMON/platform_constants.h"
#include "common/ran_context.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "NR_BCCH-BCH-Message.h"
#include "NR_ServingCellConfigCommon.h"

#include "LAYER2/NR_MAC_gNB/mac_proto.h"

#include "NR_MIB.h"

extern RAN_CONTEXT_t RC;
//extern int l2_init_gNB(void);
extern void mac_top_init_gNB(void);
extern uint8_t nfapi_mode;
 

void config_common(int Mod_idP, 
		   NR_ServingCellConfigCommon_t *scc
                  ){
  nfapi_nr_config_request_t *cfg = &RC.nrmac[Mod_idP]->config[0];

  int mu = 1;

  switch (*scc->ssb_periodicityServingCell) {
    case 0:
      cfg->sch_config.ssb_periodicity.value = 5;
      break;
    case 1:
      cfg->sch_config.ssb_periodicity.value = 10;
      break;
    case 2:
      cfg->sch_config.ssb_periodicity.value = 20;
      break;
    case 3:
      cfg->sch_config.ssb_periodicity.value = 40;
      break;
    case 4:
      cfg->sch_config.ssb_periodicity.value = 80;
      break;
    case 5:
      cfg->sch_config.ssb_periodicity.value = 160;
      break;
  }   


  cfg->sch_config.physical_cell_id.value = *scc->physCellId;
  cfg->sch_config.ssb_scg_position_in_burst.value = scc->ssb_PositionsInBurst->choice.mediumBitmap.buf[0];
 
  // TDD
  cfg->subframe_config.duplex_mode.value                          = 1;
  cfg->subframe_config.duplex_mode.tl.tag = NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG;
  cfg->num_tlv++;
  
  /// In NR DL and UL will be different band
  cfg->nfapi_config.rf_bands.number_rf_bands       = 1;
  cfg->nfapi_config.rf_bands.rf_band[0]            = *(long*)scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];  
  cfg->nfapi_config.rf_bands.tl.tag = NFAPI_PHY_RF_BANDS_TAG;
  cfg->num_tlv++;

  cfg->nfapi_config.nrarfcn.value                  = scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA;

  cfg->nfapi_config.nrarfcn.tl.tag = NFAPI_NR_NFAPI_NRARFCN_TAG;
  cfg->num_tlv++;

  cfg->subframe_config.numerology_index_mu.value = mu;
  //cfg->subframe_config.tl.tag = 
  //cfg->num_tlv++;

  cfg->rf_config.dl_carrier_bandwidth.value    = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
  cfg->rf_config.dl_carrier_bandwidth.tl.tag   = NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG; //temporary
  cfg->num_tlv++;
  LOG_I(PHY,"%s() dl_BandwidthP:%d\n", __FUNCTION__, cfg->rf_config.dl_carrier_bandwidth.value);

  cfg->rf_config.ul_carrier_bandwidth.value    = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
  cfg->rf_config.ul_carrier_bandwidth.tl.tag   = NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG; 
  cfg->num_tlv++;

  cfg->rf_config.dl_subcarrierspacing.value    = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
  cfg->rf_config.dl_subcarrierspacing.tl.tag   = NFAPI_NR_RF_CONFIG_DL_SUBCARRIERSPACING_TAG;
  cfg->num_tlv++;

  cfg->rf_config.ul_subcarrierspacing.value    = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
  cfg->rf_config.ul_subcarrierspacing.tl.tag   = NFAPI_NR_RF_CONFIG_UL_SUBCARRIERSPACING_TAG;
  cfg->num_tlv++;

  cfg->rf_config.dl_offsettocarrier.value    = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
  cfg->rf_config.dl_offsettocarrier.tl.tag   = NFAPI_NR_RF_CONFIG_DL_OFFSETTOCARRIER_TAG;
  cfg->num_tlv++;

  cfg->rf_config.ul_offsettocarrier.value    = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
  cfg->rf_config.ul_offsettocarrier.tl.tag   = NFAPI_NR_RF_CONFIG_UL_OFFSETTOCARRIER_TAG;
  cfg->num_tlv++;
 
  // InitialBWP configuration

  cfg->initialBWP_config.dl_bandwidth.value    = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,275);
  cfg->initialBWP_config.dl_bandwidth.tl.tag   = NFAPI_INITIALBWP_DL_BANDWIDTH_TAG; //temporary
  cfg->num_tlv++;
  cfg->initialBWP_config.dl_offset.value    = NRRIV2PRBOFFSET(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,275);
  cfg->initialBWP_config.dl_offset.tl.tag   = NFAPI_INITIALBWP_DL_OFFSET_TAG; //temporary
  cfg->num_tlv++;
  cfg->initialBWP_config.dl_subcarrierSpacing.value    = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing;
  cfg->initialBWP_config.dl_subcarrierSpacing.tl.tag   = NFAPI_INITIALBWP_DL_SUBCARRIERSPACING_TAG; //temporary
  cfg->num_tlv++;
  LOG_I(PHY,"%s() initialBWP_dl_Bandwidth.RBstart.SCS :%d.%d.%d\n", __FUNCTION__, cfg->initialBWP_config.dl_bandwidth.value,cfg->initialBWP_config.dl_offset.value,cfg->initialBWP_config.dl_subcarrierSpacing.value);

  cfg->initialBWP_config.ul_bandwidth.value    = NRRIV2BW(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth,275);
  cfg->initialBWP_config.ul_bandwidth.tl.tag   = NFAPI_INITIALBWP_UL_BANDWIDTH_TAG; 
  cfg->num_tlv++;
  cfg->initialBWP_config.ul_offset.value    = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth,275);
  cfg->initialBWP_config.ul_offset.tl.tag   = NFAPI_INITIALBWP_UL_OFFSET_TAG; //temporary
  cfg->num_tlv++;
  cfg->initialBWP_config.ul_subcarrierSpacing.value    = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;
  cfg->initialBWP_config.ul_subcarrierSpacing.tl.tag   = NFAPI_INITIALBWP_DL_SUBCARRIERSPACING_TAG; //temporary
  cfg->num_tlv++;
  LOG_I(PHY,"%s() initialBWP_ul_Bandwidth.RBstart.SCS :%d.%d.%d\n", __FUNCTION__, cfg->initialBWP_config.ul_bandwidth.value,cfg->initialBWP_config.ul_offset.value,cfg->initialBWP_config.ul_subcarrierSpacing.value);


  cfg->rach_config.prach_RootSequenceIndex.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l139;
  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing)
    cfg->rach_config.prach_msg1_SubcarrierSpacing.value = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing;
  else cfg->rach_config.prach_msg1_SubcarrierSpacing.value=cfg->rf_config.dl_subcarrierspacing.value;

  cfg->rach_config.restrictedSetConfig.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig;
  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder)
    cfg->rach_config.msg3_transformPrecoding.value = 1;
  else cfg->rach_config.msg3_transformPrecoding.value = 0;

  cfg->rach_config.prach_ConfigurationIndex.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex;  
  cfg->rach_config.prach_msg1_FDM.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM;            
  cfg->rach_config.prach_msg1_FrequencyStart.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart; 
  cfg->rach_config.zeroCorrelationZoneConfig.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig;
  cfg->rach_config.preambleReceivedTargetPower.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleReceivedTargetPower;

  // PDCCH-ConfigCommon
  cfg->pdcch_config.controlResourceSetZero.value = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero;
  cfg->pdcch_config.searchSpaceZero.value = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero;

  // PDSCH-ConfigCommon
  cfg->pdsch_config.num_PDSCHTimeDomainResourceAllocations.value = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count;
  cfg->pdsch_config.dmrs_TypeA_Position.value = scc->dmrs_TypeA_Position;
  AssertFatal(cfg->pdsch_config.num_PDSCHTimeDomainResourceAllocations.value<=NFAPI_NR_PDSCH_CONFIG_MAXALLOCATIONS,"illegal TimeDomainAllocation count %d\n",cfg->pdsch_config.num_PDSCHTimeDomainResourceAllocations.value);
  for (int i=0;i<cfg->pdsch_config.num_PDSCHTimeDomainResourceAllocations.value;i++) {
    cfg->pdsch_config.PDSCHTimeDomainResourceAllocation_k0[i].value=*scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0;
    cfg->pdsch_config.PDSCHTimeDomainResourceAllocation_mappingType[i].value=scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->mappingType;
    cfg->pdsch_config.PDSCHTimeDomainResourceAllocation_startSymbolAndLength[i].value=scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
  }

  // PUSCH-ConfigCommon
  cfg->pusch_config.num_PUSCHTimeDomainResourceAllocations.value = scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.count;
  cfg->pusch_config.dmrs_TypeA_Position.value = scc->dmrs_TypeA_Position+2;
  AssertFatal(cfg->pusch_config.num_PUSCHTimeDomainResourceAllocations.value<=NFAPI_NR_PUSCH_CONFIG_MAXALLOCATIONS,"illegal TimeDomainAllocation count %d\n",cfg->pusch_config.num_PUSCHTimeDomainResourceAllocations.value);
  for (int i=0;i<cfg->pusch_config.num_PUSCHTimeDomainResourceAllocations.value;i++) {
    cfg->pusch_config.PUSCHTimeDomainResourceAllocation_k2[i].value=*scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[0]->k2;
  }

  //cfg->sch_config.half_frame_index.value = 0; Fix in PHY
  //cfg->sch_config.n_ssb_crb.value = 86;       Fix in PHY

}

/*void config_servingcellconfigcommon(){

}*/

int rrc_mac_config_req_gNB(module_id_t Mod_idP, 
			   int ssb_SubcarrierOffset,
                           NR_ServingCellConfigCommon_t *scc,
			   int add_ue,
			   uint32_t rnti,
			   NR_CellGroupConfig_t *secondaryCellGroup
                           ){

  if (scc != NULL ) {
    AssertFatal(scc->ssb_PositionsInBurst->present == NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap, "SSB Bitmap is not 8-bits!\n");

    LOG_I(MAC,"Configuring common parameters from NR ServingCellConfig\n");

    config_common(Mod_idP, 
		  scc);
    LOG_E(MAC, "%s() %s:%d RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req:%p\n", __FUNCTION__, __FILE__, __LINE__, RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req);
  
    // if in nFAPI mode 
    if ( (nfapi_mode == 1 || nfapi_mode == 2) && (RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req == NULL) ){
      while(RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req == NULL) {
	// DJP AssertFatal(RC.nrmac[Mod_idP]->if_inst->PHY_config_req != NULL,"if_inst->phy_config_request is null\n");
	usleep(100 * 1000);
	printf("Waiting for PHY_config_req\n");
      }
    }
  
  
    NR_PHY_Config_t phycfg;
    phycfg.Mod_id = Mod_idP;
    phycfg.CC_id  = 0;
    phycfg.cfg    = &RC.nrmac[Mod_idP]->config[0];
    
    if (RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req) RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req(&phycfg); 
  }
  
  if (secondaryCellGroup) {

    NR_UE_list_t *UE_list = &RC.nrmac[Mod_idP]->UE_list;
    int UE_id;
    if (add_ue == 1) {
      UE_id = add_new_nr_ue(Mod_idP,rnti);
      UE_list->secondaryCellGroup[UE_id] = secondaryCellGroup;
      LOG_I(PHY,"Added new UE_id %d/%x with initial secondaryCellGroup\n",UE_id,rnti);
    }
    else { // secondaryCellGroup has been updated
      UE_id = find_nr_UE_id(Mod_idP,rnti);
      UE_list->secondaryCellGroup[UE_id] = secondaryCellGroup;
      LOG_I(PHY,"Modified UE_id %d/%x with secondaryCellGroup\n",UE_id,rnti);
    }
    fill_nfapi_coresets_and_searchspaces(secondaryCellGroup,
					 UE_list->coreset[UE_id],
					 UE_list->search_space[UE_id]);
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_OUT);
  
    
  return(0);

}// END rrc_mac_config_req_gNB
