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

#include "../NR_MAC_gNB/nr_mac_common.h"
#include "SCHED_NR/phy_frame_config_nr.h"

int set_tdd_config_nr_ue(fapi_nr_config_request_t *cfg,
                         int mu,
                         int nrofDownlinkSlots, int nrofDownlinkSymbols,
                         int nrofUplinkSlots,   int nrofUplinkSymbols) {

  int slot_number = 0;
  int nb_periods_per_frame;
  int nb_slots_to_set = TDD_CONFIG_NB_FRAMES*(1<<mu)*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;

  switch(cfg->tdd_table.tdd_period) {
    case 0:
      nb_periods_per_frame = 20; // 10ms/0p5ms
      break;

    case 1:
      nb_periods_per_frame = 16; // 10ms/0p625ms
      break;

    case 2:
      nb_periods_per_frame = 10; // 10ms/1ms
      break;

    case 3:
      nb_periods_per_frame = 8; // 10ms/1p25ms
      break;

    case 4:
      nb_periods_per_frame = 5; // 10ms/2ms
      break;

    case 5:
      nb_periods_per_frame = 4; // 10ms/2p5ms
      break;

    case 6:
      nb_periods_per_frame = 2; // 10ms/5ms
      break;

    case 7:
      nb_periods_per_frame = 1; // 10ms/10ms
      break;

    default:
      AssertFatal(1==0,"Undefined tdd period %d\n", cfg->tdd_table.tdd_period);
  }

  int nb_slots_per_period = ((1<<mu) * NR_NUMBER_OF_SUBFRAMES_PER_FRAME)/nb_periods_per_frame;

  if ( (nrofDownlinkSymbols + nrofUplinkSymbols) == 0 )
    AssertFatal(nb_slots_per_period == (nrofDownlinkSlots + nrofUplinkSlots),
                "set_tdd_configuration_nr: given period is inconsistent with current tdd configuration, nrofDownlinkSlots %d, nrofUplinkSlots %d, nb_slots_per_period %d \n",
                nrofDownlinkSlots,nrofUplinkSlots,nb_slots_per_period);
  else {
    AssertFatal(nrofDownlinkSymbols + nrofUplinkSymbols < 14,"illegal symbol configuration DL %d, UL %d\n",nrofDownlinkSymbols,nrofUplinkSymbols);
    AssertFatal(nb_slots_per_period == (nrofDownlinkSlots + nrofUplinkSlots + 1),
                "set_tdd_configuration_nr: given period is inconsistent with current tdd configuration, nrofDownlinkSlots %d, nrofUplinkSlots %d, nrofMixed slots 1, nb_slots_per_period %d \n",
                nrofDownlinkSlots,nrofUplinkSlots,nb_slots_per_period);
  }

  cfg->tdd_table.max_tdd_periodicity_list = (fapi_nr_max_tdd_periodicity_t *) malloc(nb_slots_to_set*sizeof(fapi_nr_max_tdd_periodicity_t));

  for(int memory_alloc =0 ; memory_alloc<nb_slots_to_set; memory_alloc++)
    cfg->tdd_table.max_tdd_periodicity_list[memory_alloc].max_num_of_symbol_per_slot_list = (fapi_nr_max_num_of_symbol_per_slot_t *) malloc(NR_NUMBER_OF_SYMBOLS_PER_SLOT*sizeof(
          fapi_nr_max_num_of_symbol_per_slot_t));

  while(slot_number != nb_slots_to_set) {
    if(nrofDownlinkSlots != 0) {
      for (int number_of_symbol = 0; number_of_symbol < nrofDownlinkSlots*NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
        cfg->tdd_table.max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol%NR_NUMBER_OF_SYMBOLS_PER_SLOT].slot_config= 0;

        if((number_of_symbol+1)%NR_NUMBER_OF_SYMBOLS_PER_SLOT == 0)
          slot_number++;
      }
    }

    if (nrofDownlinkSymbols != 0 || nrofUplinkSymbols != 0) {
      for(int number_of_symbol =0; number_of_symbol < nrofDownlinkSymbols; number_of_symbol++) {
        cfg->tdd_table.max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol].slot_config= 0;
      }

      for(int number_of_symbol = nrofDownlinkSymbols; number_of_symbol < NR_NUMBER_OF_SYMBOLS_PER_SLOT-nrofUplinkSymbols; number_of_symbol++) {
        cfg->tdd_table.max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol].slot_config= 2;
      }

      for(int number_of_symbol = NR_NUMBER_OF_SYMBOLS_PER_SLOT-nrofUplinkSymbols; number_of_symbol < NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
        cfg->tdd_table.max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol].slot_config= 1;
      }

      slot_number++;
    }

    if(nrofUplinkSlots != 0) {
      for (int number_of_symbol = 0; number_of_symbol < nrofUplinkSlots*NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
        cfg->tdd_table.max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol%NR_NUMBER_OF_SYMBOLS_PER_SLOT].slot_config= 1;

        if((number_of_symbol+1)%NR_NUMBER_OF_SYMBOLS_PER_SLOT == 0)
          slot_number++;
      }
    }
  }

  return (0);
}


void config_common_ue(NR_UE_MAC_INST_t *mac,
		      module_id_t       module_id,
		      int               cc_idP) {

  fapi_nr_config_request_t        *cfg = &mac->phy_config.config_req;
  NR_ServingCellConfigCommon_t    *scc = mac->scc;
  int i;

    mac->phy_config.Mod_id = module_id;
    mac->phy_config.CC_id = cc_idP;    
  
  // carrier config

  LOG_I(MAC,"UE Config Common\n");  

  cfg->carrier_config.dl_bandwidth = config_bandwidth(scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                                      scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,
                                                      *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0]);

  cfg->carrier_config.dl_frequency = from_nrarfcn(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0],
                                                  *scc->ssbSubcarrierSpacing,
                                                  scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA)/1000; // freq in kHz

  for (i=0; i<5; i++) {
    if (i==scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.dl_grid_size[i] = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.dl_k0[i] = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
    }
    else {
      cfg->carrier_config.dl_grid_size[i] = 0;
      cfg->carrier_config.dl_k0[i] = 0;
    }
  }

  cfg->carrier_config.uplink_bandwidth = config_bandwidth(scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                                          scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,
                                                          *scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list.array[0]);

  int UL_pointA;
  if (scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA == NULL)
    UL_pointA = scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA;
  else
    UL_pointA = *scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA; 

  cfg->carrier_config.uplink_frequency = from_nrarfcn(*scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list.array[0],
                                                      *scc->ssbSubcarrierSpacing,
                                                      UL_pointA)/1000; // freq in kHz


  for (i=0; i<5; i++) {
    if (i==scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.ul_grid_size[i] = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.ul_k0[i] = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
    }
    else {
      cfg->carrier_config.ul_grid_size[i] = 0;
      cfg->carrier_config.ul_k0[i] = 0;
    }
  }


  // cell config

  cfg->cell_config.phy_cell_id = *scc->physCellId;
  cfg->cell_config.frame_duplex_type = 1;


  // SSB config
  cfg->ssb_config.ss_pbch_power = scc->ss_PBCH_BlockPower;
  cfg->ssb_config.scs_common = *scc->ssbSubcarrierSpacing;

  // SSB Table config
  int scs_scaling = 1<<(cfg->ssb_config.scs_common);
  if (scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA < 600000)
    scs_scaling = scs_scaling*3;
  if (scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA > 2016666)
    scs_scaling = scs_scaling>>2;
  uint32_t absolute_diff = (*scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB - scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA);
  cfg->ssb_table.ssb_offset_point_a = absolute_diff/(12*scs_scaling) - 10;
  cfg->ssb_table.ssb_period = *scc->ssb_periodicityServingCell;
  cfg->ssb_table.ssb_subcarrier_offset = 0; // TODO currently not in RRC?

  switch (scc->ssb_PositionsInBurst->present) {
    case 1 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask = scc->ssb_PositionsInBurst->choice.shortBitmap.buf[0]<<24;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask = 0;
      break;
    case 2 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask = scc->ssb_PositionsInBurst->choice.mediumBitmap.buf[0]<<24;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask = 0;
      break;
    case 3 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask = 0;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask = 0;
      for (i=0; i<4; i++) {
        cfg->ssb_table.ssb_mask_list[0].ssb_mask += (scc->ssb_PositionsInBurst->choice.longBitmap.buf[i+4]<<i*8);
        cfg->ssb_table.ssb_mask_list[1].ssb_mask += (scc->ssb_PositionsInBurst->choice.longBitmap.buf[i]<<i*8);
      }
      break;
    default:
      AssertFatal(1==0,"SSB bitmap size value %d undefined (allowed values 1,2,3) \n", scc->ssb_PositionsInBurst->present);
  }

  // TDD Table Configuration
  if (scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1 == NULL)
    cfg->tdd_table.tdd_period = scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity;
  else {
    AssertFatal(scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530 != NULL,
		"scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530 is null\n");
    cfg->tdd_table.tdd_period = *scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530;
  }
  LOG_I(MAC,"Setting TDD configuration period to %d\n",cfg->tdd_table.tdd_period);
  if(cfg->cell_config.frame_duplex_type == TDD){
    int return_tdd = set_tdd_config_nr_ue(cfg,
		     scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                     scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots,
                     scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols,
                     scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots,
                     scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols
                     );

    if (return_tdd !=0)
      LOG_E(PHY,"TDD configuration can not be done\n");
    else
      LOG_I(PHY,"TDD has been properly configurated\n");
  }


}

int nr_rrc_mac_config_req_ue(
    module_id_t                     module_id,
    int                             cc_idP,
    uint8_t                         gNB_index,
    NR_MIB_t                        *mibP,
    //    NR_ServingCellConfigCommon_t    *sccP,
    //    NR_MAC_CellGroupConfig_t        *mac_cell_group_configP,
    //    NR_PhysicalCellGroupConfig_t    *phy_cell_group_configP,
    NR_SpCellConfig_t               *spCell_ConfigP ){

    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

    //    NR_ServingCellConfig_t *serving_cell_config = spcell_configP->spCellConfigDedicated;
    //  TODO do something FAPI-like P5 L1/L2 config interface in config_si, config_mib, etc.


    if(mibP != NULL){
      mac->mib = mibP;    //  update by every reception
    }
    
    
    if(spCell_ConfigP != NULL ){
      mac->servCellIndex = *spCell_ConfigP->servCellIndex;
      if (spCell_ConfigP->reconfigurationWithSync) {
	mac->scc = spCell_ConfigP->reconfigurationWithSync->spCellConfigCommon;
	config_common_ue(mac,module_id,cc_idP);
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
