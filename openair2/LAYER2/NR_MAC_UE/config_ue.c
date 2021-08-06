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
#include "NR_MAC_UE/mac_proto.h"
#include "NR_MAC-CellGroupConfig.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"

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
  cfg->tdd_table.tdd_period_in_slots = nb_slots_per_period;

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
  LOG_D(MAC, "Entering UE Config Common\n");

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

  uint32_t band = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
  frequency_range_t  frequency_range = band<100?FR1:FR2;
 
  lte_frame_type_t frame_type;
  get_frame_type(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing, &frame_type);

  // cell config

  cfg->cell_config.phy_cell_id = *scc->physCellId;
  cfg->cell_config.frame_duplex_type = frame_type;

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
        cfg->ssb_table.ssb_mask_list[0].ssb_mask += (scc->ssb_PositionsInBurst->choice.longBitmap.buf[3-i]<<i*8);
        cfg->ssb_table.ssb_mask_list[1].ssb_mask += (scc->ssb_PositionsInBurst->choice.longBitmap.buf[7-i]<<i*8);
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
  if(cfg->cell_config.frame_duplex_type == TDD){
    LOG_I(MAC,"Setting TDD configuration period to %d\n", cfg->tdd_table.tdd_period);
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

  // PRACH configuration

  uint8_t nb_preambles = 64;
  if(scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles != NULL)
     nb_preambles = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles;

  cfg->prach_config.prach_sequence_length = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present-1;

  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing)
    cfg->prach_config.prach_sub_c_spacing = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing;
  else 
    cfg->prach_config.prach_sub_c_spacing = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  cfg->prach_config.restricted_set_config = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig;

  switch (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM) {
    case 0 :
      cfg->prach_config.num_prach_fd_occasions = 1;
      break;
    case 1 :
      cfg->prach_config.num_prach_fd_occasions = 2;
      break;
    case 2 :
      cfg->prach_config.num_prach_fd_occasions = 4;
      break;
    case 3 :
      cfg->prach_config.num_prach_fd_occasions = 8;
      break;
    default:
      AssertFatal(1==0,"msg1 FDM identifier %ld undefined (0,1,2,3) \n", scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM);
  }

  cfg->prach_config.num_prach_fd_occasions_list = (fapi_nr_num_prach_fd_occasions_t *) malloc(cfg->prach_config.num_prach_fd_occasions*sizeof(fapi_nr_num_prach_fd_occasions_t));
  for (i=0; i<cfg->prach_config.num_prach_fd_occasions; i++) {
    cfg->prach_config.num_prach_fd_occasions_list[i].num_prach_fd_occasions = i;
    if (cfg->prach_config.prach_sequence_length)
      cfg->prach_config.num_prach_fd_occasions_list[i].prach_root_sequence_index = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l139; 
    else
      cfg->prach_config.num_prach_fd_occasions_list[i].prach_root_sequence_index = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l839;

    cfg->prach_config.num_prach_fd_occasions_list[i].k1 = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart;
    cfg->prach_config.num_prach_fd_occasions_list[i].prach_zero_corr_conf = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig;
    cfg->prach_config.num_prach_fd_occasions_list[i].num_root_sequences = compute_nr_root_seq(scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup, nb_preambles, frame_type,frequency_range);
    //cfg->prach_config.num_prach_fd_occasions_list[i].num_unused_root_sequences = ???
  }

  cfg->prach_config.ssb_per_rach = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present-1;

}

/** \brief This function performs some configuration routines according to clause 12 "Bandwidth part operation" 3GPP TS 38.213 version 16.3.0 Release 16
    @param NR_UE_MAC_INST_t mac: pointer to local MAC instance
    @returns void
    */
void config_bwp_ue(NR_UE_MAC_INST_t *mac, uint16_t *bwp_ind, uint8_t *dci_format){

  NR_ServingCellConfig_t *scd = mac->scg->spCellConfig->spCellConfigDedicated;

  if (bwp_ind && dci_format){

    switch(*dci_format){
    case NR_UL_DCI_FORMAT_0_1:
      mac->UL_BWP_Id = *bwp_ind;
      break;
    case NR_DL_DCI_FORMAT_1_1:
      mac->DL_BWP_Id = *bwp_ind;
      break;
    default:
      LOG_E(MAC, "In %s: failed to configure BWP Id from DCI with format %d \n", __FUNCTION__, *dci_format);
    }

  } else {

    if (scd->firstActiveDownlinkBWP_Id)
      mac->DL_BWP_Id = *scd->firstActiveDownlinkBWP_Id;
    else if (scd->defaultDownlinkBWP_Id)
      mac->DL_BWP_Id = *scd->defaultDownlinkBWP_Id;
    else
      mac->DL_BWP_Id = 1;

    if (scd->uplinkConfig && scd->uplinkConfig->firstActiveUplinkBWP_Id)
      mac->UL_BWP_Id = *scd->uplinkConfig->firstActiveUplinkBWP_Id;
    else
      mac->UL_BWP_Id = 1;

  }

  LOG_D(MAC, "In %s setting DL_BWP_Id %ld UL_BWP_Id %ld \n", __FUNCTION__, mac->DL_BWP_Id, mac->UL_BWP_Id);

}

/** \brief This function is relavant for the UE procedures for control. It loads the search spaces, the BWPs and the CORESETs into the MAC instance and
    \brief performs assert checks on the relevant RRC configuration.
    @param NR_UE_MAC_INST_t mac: pointer to local MAC instance
    @returns void
    */
void config_control_ue(NR_UE_MAC_INST_t *mac){

  uint8_t coreset_id = 1, ss_id;

  NR_ServingCellConfig_t *scd = mac->scg->spCellConfig->spCellConfigDedicated;
  AssertFatal(scd->downlinkBWP_ToAddModList != NULL, "downlinkBWP_ToAddModList is null\n");
  AssertFatal(scd->downlinkBWP_ToAddModList->list.count == 1, "downlinkBWP_ToAddModList->list->count is %d\n", scd->downlinkBWP_ToAddModList->list.count);

  config_bwp_ue(mac, NULL, NULL);
  NR_BWP_Id_t dl_bwp_id = mac->DL_BWP_Id;
  AssertFatal(dl_bwp_id != 0, "DL_BWP_Id is 0!");

  NR_BWP_DownlinkCommon_t *bwp_Common = scd->downlinkBWP_ToAddModList->list.array[dl_bwp_id - 1]->bwp_Common;
  AssertFatal(bwp_Common != NULL, "bwp_Common is null\n");

  NR_BWP_DownlinkDedicated_t *dl_bwp_Dedicated = scd->downlinkBWP_ToAddModList->list.array[dl_bwp_id - 1]->bwp_Dedicated;
  AssertFatal(dl_bwp_Dedicated != NULL, "dl_bwp_Dedicated is null\n");

  NR_SetupRelease_PDCCH_Config_t *pdcch_Config = dl_bwp_Dedicated->pdcch_Config;
  AssertFatal(pdcch_Config != NULL, "pdcch_Config is null\n");

  NR_SetupRelease_PDCCH_ConfigCommon_t *pdcch_ConfigCommon = bwp_Common->pdcch_ConfigCommon;
  AssertFatal(pdcch_ConfigCommon != NULL, "pdcch_ConfigCommon is null\n");
  AssertFatal(pdcch_ConfigCommon->choice.setup->ra_SearchSpace != NULL, "ra_SearchSpace must be available in DL BWP\n");

  struct NR_PDCCH_ConfigCommon__commonSearchSpaceList *commonSearchSpaceList = pdcch_ConfigCommon->choice.setup->commonSearchSpaceList;
  AssertFatal(commonSearchSpaceList != NULL, "commonSearchSpaceList is null\n");
  AssertFatal(commonSearchSpaceList->list.count > 0, "PDCCH CSS list has 0 elements\n");

  struct NR_PDCCH_Config__controlResourceSetToAddModList *controlResourceSetToAddModList = pdcch_Config->choice.setup->controlResourceSetToAddModList;
  AssertFatal(controlResourceSetToAddModList != NULL, "controlResourceSetToAddModList is null\n");
  AssertFatal(controlResourceSetToAddModList->list.count == 1, "controlResourceSetToAddModList->list.count=%d\n", controlResourceSetToAddModList->list.count);
  AssertFatal(controlResourceSetToAddModList->list.array[0] != NULL, "coreset[0][0] is null\n");

  struct NR_PDCCH_Config__searchSpacesToAddModList *searchSpacesToAddModList = pdcch_Config->choice.setup->searchSpacesToAddModList;
  AssertFatal(searchSpacesToAddModList != NULL, "searchSpacesToAddModList is null\n");
  AssertFatal(searchSpacesToAddModList->list.count > 0, "list of UE specifically configured Search Spaces is empty\n");
  AssertFatal(searchSpacesToAddModList->list.count < FAPI_NR_MAX_SS_PER_CORESET, "too many searchpaces per coreset %d\n", searchSpacesToAddModList->list.count);

  struct NR_UplinkConfig__uplinkBWP_ToAddModList *uplinkBWP_ToAddModList = scd->uplinkConfig->uplinkBWP_ToAddModList;
  AssertFatal(uplinkBWP_ToAddModList != NULL, "uplinkBWP_ToAddModList is null\n");
  AssertFatal(uplinkBWP_ToAddModList->list.count == 1, "uplinkBWP_ToAddModList->list->count is %d\n", uplinkBWP_ToAddModList->list.count);

  // check pdcch_Config, pdcch_ConfigCommon and DL BWP
  mac->DLbwp[0] = scd->downlinkBWP_ToAddModList->list.array[dl_bwp_id - 1];
  mac->coreset[dl_bwp_id - 1][coreset_id - 1] = controlResourceSetToAddModList->list.array[0];

  // Check dedicated UL BWP and pass to MAC
  mac->ULbwp[0] = uplinkBWP_ToAddModList->list.array[0];
  AssertFatal(mac->ULbwp[0]->bwp_Dedicated != NULL, "UL bwp_Dedicated is null\n");

  // check available Search Spaces in the searchSpacesToAddModList and pass to MAC
  // note: the network configures at most 10 Search Spaces per BWP per cell (including UE-specific and common Search Spaces).
  for (ss_id = 0; ss_id < searchSpacesToAddModList->list.count; ss_id++) {
    NR_SearchSpace_t *ss = searchSpacesToAddModList->list.array[ss_id];
    AssertFatal(ss->controlResourceSetId != NULL, "ss->controlResourceSetId is null\n");
    AssertFatal(ss->searchSpaceType != NULL, "ss->searchSpaceType is null\n");
    AssertFatal(*ss->controlResourceSetId == mac->coreset[dl_bwp_id - 1][coreset_id - 1]->controlResourceSetId, "ss->controlResourceSetId is unknown\n");
    AssertFatal(ss->monitoringSymbolsWithinSlot != NULL, "NR_SearchSpace->monitoringSymbolsWithinSlot is null\n");
    AssertFatal(ss->monitoringSymbolsWithinSlot->buf != NULL, "NR_SearchSpace->monitoringSymbolsWithinSlot->buf is null\n");
    mac->SSpace[0][0][ss_id] = ss;
  }

  // Check available CSSs in the commonSearchSpaceList (list of additional common search spaces)
  // note: commonSearchSpaceList SIZE(1..4)
  for (int css_id = 0; css_id < commonSearchSpaceList->list.count; css_id++) {
    NR_SearchSpace_t *css = commonSearchSpaceList->list.array[css_id];
    AssertFatal(css->controlResourceSetId != NULL, "ss->controlResourceSetId is null\n");
    AssertFatal(*css->controlResourceSetId == mac->coreset[dl_bwp_id - 1][coreset_id - 1]->controlResourceSetId, "ss->controlResourceSetId is unknown\n");
    AssertFatal(css->searchSpaceType != NULL, "css->searchSpaceType is null\n");
    AssertFatal(css->monitoringSymbolsWithinSlot != NULL, "css->monitoringSymbolsWithinSlot is null\n");
    AssertFatal(css->monitoringSymbolsWithinSlot->buf != NULL, "css->monitoringSymbolsWithinSlot->buf is null\n");
    mac->SSpace[0][0][ss_id] = css;
    ss_id++;
  }

  // TODO: Merge this code in a single function as fill_default_searchSpaceZero() in rrc_gNB_reconfig.c
  // Search space zero
  if(mac->search_space_zero == NULL) mac->search_space_zero=calloc(1,sizeof(*mac->search_space_zero));
  if(mac->search_space_zero->controlResourceSetId == NULL) mac->search_space_zero->controlResourceSetId=calloc(1,sizeof(*mac->search_space_zero->controlResourceSetId));
  if(mac->search_space_zero->monitoringSymbolsWithinSlot == NULL) mac->search_space_zero->monitoringSymbolsWithinSlot = calloc(1,sizeof(*mac->search_space_zero->monitoringSymbolsWithinSlot));
  if(mac->search_space_zero->monitoringSymbolsWithinSlot->buf == NULL) mac->search_space_zero->monitoringSymbolsWithinSlot->buf = calloc(1,2);
  if(mac->search_space_zero->nrofCandidates == NULL) mac->search_space_zero->nrofCandidates = calloc(1,sizeof(*mac->search_space_zero->nrofCandidates));
  if(mac->search_space_zero->searchSpaceType == NULL) mac->search_space_zero->searchSpaceType = calloc(1,sizeof(*mac->search_space_zero->searchSpaceType));
  if(mac->search_space_zero->searchSpaceType->choice.common == NULL) mac->search_space_zero->searchSpaceType->choice.common=calloc(1,sizeof(*mac->search_space_zero->searchSpaceType->choice.common));
  if(mac->search_space_zero->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0 == NULL) mac->search_space_zero->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0 = calloc(1,sizeof(*mac->search_space_zero->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0));
  mac->search_space_zero->searchSpaceId = 0;
  *mac->search_space_zero->controlResourceSetId = 0;
  mac->search_space_zero->monitoringSlotPeriodicityAndOffset = calloc(1,sizeof(*mac->search_space_zero->monitoringSlotPeriodicityAndOffset));
  mac->search_space_zero->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
  mac->search_space_zero->duration=NULL;
  // should be '1100 0000 0000 00'B (LSB first!), first two symbols in slot, adjust if needed
  mac->search_space_zero->monitoringSymbolsWithinSlot->buf[1] = 0;
  mac->search_space_zero->monitoringSymbolsWithinSlot->buf[0] = (1<<7);
  mac->search_space_zero->monitoringSymbolsWithinSlot->size = 2;
  mac->search_space_zero->monitoringSymbolsWithinSlot->bits_unused = 2;

  // FIXME: update values from TS38.213 Section 10.1 Table 10.1-1: CCE aggregation levels and maximum number of PDCCH candidates per CCE aggregation level for CSS sets configured by searchSpaceSIB1
  mac->search_space_zero->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
  mac->search_space_zero->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n0;
  mac->search_space_zero->nrofCandidates->aggregationLevel4 = NR_SearchSpace__nrofCandidates__aggregationLevel4_n2;
  mac->search_space_zero->nrofCandidates->aggregationLevel8 = NR_SearchSpace__nrofCandidates__aggregationLevel8_n0;
  mac->search_space_zero->nrofCandidates->aggregationLevel16 = NR_SearchSpace__nrofCandidates__aggregationLevel16_n0;
  mac->search_space_zero->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_common;

  // Coreset0
  if(mac->coreset0 == NULL) mac->coreset0 = calloc(1,sizeof(*mac->coreset0));
  mac->coreset0->controlResourceSetId = 0;
  // frequencyDomainResources '11111111 00000000 00000000 00000000 00000000 00000'B,
  if(mac->coreset0->frequencyDomainResources.buf == NULL) mac->coreset0->frequencyDomainResources.buf = calloc(1,6);
  mac->coreset0->frequencyDomainResources.buf[0] = 0xff;
  mac->coreset0->frequencyDomainResources.buf[1] = 0;
  mac->coreset0->frequencyDomainResources.buf[2] = 0;
  mac->coreset0->frequencyDomainResources.buf[3] = 0;
  mac->coreset0->frequencyDomainResources.buf[4] = 0;
  mac->coreset0->frequencyDomainResources.buf[5] = 0;
  mac->coreset0->frequencyDomainResources.size = 6;
  mac->coreset0->frequencyDomainResources.bits_unused = 3;
  mac->coreset0->duration = 1;
  mac->coreset0->cce_REG_MappingType.present=NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved;
  mac->coreset0->cce_REG_MappingType.choice.interleaved=calloc(1,sizeof(*mac->coreset0->cce_REG_MappingType.choice.interleaved));
  mac->coreset0->cce_REG_MappingType.choice.interleaved->reg_BundleSize = NR_ControlResourceSet__cce_REG_MappingType__interleaved__reg_BundleSize_n6;
  mac->coreset0->cce_REG_MappingType.choice.interleaved->interleaverSize = NR_ControlResourceSet__cce_REG_MappingType__interleaved__interleaverSize_n2;
  mac->coreset0->cce_REG_MappingType.choice.interleaved->shiftIndex = NULL;
  mac->coreset0->precoderGranularity = NR_ControlResourceSet__precoderGranularity_sameAsREG_bundle;
  if(mac->coreset0->tci_StatesPDCCH_ToAddList == NULL) mac->coreset0->tci_StatesPDCCH_ToAddList = calloc(1,sizeof(*mac->coreset0->tci_StatesPDCCH_ToAddList));
  NR_TCI_StateId_t *tci[8];
  for (int i=0;i<8;i++) {
    tci[i]=calloc(1,sizeof(*tci[i]));
    *tci[i] = i;
    ASN_SEQUENCE_ADD(&mac->coreset0->tci_StatesPDCCH_ToAddList->list,tci[i]);
  }
  mac->coreset0->tci_StatesPDCCH_ToReleaseList = NULL;
  mac->coreset0->tci_PresentInDCI = NULL;
  mac->coreset0->pdcch_DMRS_ScramblingID = NULL;

}

int nr_rrc_mac_config_req_ue(
    module_id_t                     module_id,
    int                             cc_idP,
    uint8_t                         gNB_index,
    NR_MIB_t                        *mibP,
    //    NR_ServingCellConfigCommon_t    *sccP,
    //    NR_MAC_CellGroupConfig_t        *mac_cell_group_configP,
    //    NR_PhysicalCellGroupConfig_t    *phy_cell_group_configP,
    NR_CellGroupConfig_t            *cell_group_config ){

    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
    RA_config_t *ra = &mac->ra;

    //  TODO do something FAPI-like P5 L1/L2 config interface in config_si, config_mib, etc.

    if(mibP != NULL){
      mac->mib = mibP;    //  update by every reception
    }

    if(cell_group_config != NULL ){
      mac->scg = cell_group_config;
      mac->servCellIndex = *cell_group_config->spCellConfig->servCellIndex;
      config_control_ue(mac);
      if (cell_group_config->spCellConfig->reconfigurationWithSync) {
        ra->rach_ConfigDedicated = cell_group_config->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink;
	mac->scc = cell_group_config->spCellConfig->reconfigurationWithSync->spCellConfigCommon;
        int num_slots = mac->scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots;
        if (mac->scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols > 0) {
          num_slots++;
        }
        mac->ul_config_request = calloc(num_slots, sizeof(*mac->ul_config_request));
	config_common_ue(mac,module_id,cc_idP);
	mac->crnti = cell_group_config->spCellConfig->reconfigurationWithSync->newUE_Identity;
	LOG_I(MAC,"Configuring CRNTI %x\n",mac->crnti);
      }

      // Setup the SSB to Rach Occasions mapping according to the config
      build_ssb_to_ro_map(mac->scc, mac->phy_config.config_req.cell_config.frame_duplex_type);

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
