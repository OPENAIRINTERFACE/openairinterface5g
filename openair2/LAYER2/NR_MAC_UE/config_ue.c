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
#include <NR_MAC_gNB/mac_proto.h>
#include "NR_MAC_UE/mac_proto.h"
#include "NR_MAC-CellGroupConfig.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "common/utils/nr/nr_common.h"
#include "executables/softmodem-common.h"

void set_tdd_config_nr_ue(fapi_nr_config_request_t *cfg,
                          int mu,
                          NR_TDD_UL_DL_ConfigCommon_t *tdd_config) {

  const int nrofDownlinkSlots   = tdd_config->pattern1.nrofDownlinkSlots;
  const int nrofDownlinkSymbols = tdd_config->pattern1.nrofDownlinkSymbols;
  const int nrofUplinkSlots     = tdd_config->pattern1.nrofUplinkSlots;
  const int nrofUplinkSymbols   = tdd_config->pattern1.nrofUplinkSymbols;
  int slot_number = 0;
  const int nb_periods_per_frame = get_nb_periods_per_frame(tdd_config->pattern1.dl_UL_TransmissionPeriodicity);
  const int nb_slots_to_set = TDD_CONFIG_NB_FRAMES*(1<<mu)*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
  const int nb_slots_per_period = ((1<<mu) * NR_NUMBER_OF_SUBFRAMES_PER_FRAME)/nb_periods_per_frame;
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

  if (tdd_config->pattern1.ext1 == NULL) {
    cfg->tdd_table.tdd_period = tdd_config->pattern1.dl_UL_TransmissionPeriodicity;
  } else {
    AssertFatal(tdd_config->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530 != NULL, "scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530 is null\n");
    cfg->tdd_table.tdd_period = *tdd_config->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530;
  }

  LOG_I(NR_MAC, "TDD has been properly configured\n");

}


void config_common_ue_sa(NR_UE_MAC_INST_t *mac,
		         module_id_t module_id,
		         int cc_idP) {

  fapi_nr_config_request_t *cfg = &mac->phy_config.config_req;
  NR_ServingCellConfigCommonSIB_t *scc = mac->scc_SIB;

  mac->phy_config.Mod_id = module_id;
  mac->phy_config.CC_id = cc_idP;

  LOG_D(MAC, "Entering SA UE Config Common\n");

  // carrier config
  cfg->carrier_config.dl_bandwidth = config_bandwidth(scc->downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                                      scc->downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list.array[0]->carrierBandwidth,
                                                      *scc->downlinkConfigCommon.frequencyInfoDL.frequencyBandList.list.array[0]->freqBandIndicatorNR);

  uint64_t dl_bw_khz = (12*scc->downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list.array[0]->carrierBandwidth)*
                       (15<<scc->downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list.array[0]->subcarrierSpacing);
  cfg->carrier_config.dl_frequency = (downlink_frequency[cc_idP][0]/1000) - (dl_bw_khz>>1);

  for (int i=0; i<5; i++) {
    if (i==scc->downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.dl_grid_size[i] = scc->downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.dl_k0[i] = scc->downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
    }
    else {
      cfg->carrier_config.dl_grid_size[i] = 0;
      cfg->carrier_config.dl_k0[i] = 0;
    }
  }

  cfg->carrier_config.uplink_bandwidth = config_bandwidth(scc->uplinkConfigCommon->frequencyInfoUL.scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                                          scc->uplinkConfigCommon->frequencyInfoUL.scs_SpecificCarrierList.list.array[0]->carrierBandwidth,
                                                          scc->uplinkConfigCommon->frequencyInfoUL.frequencyBandList==NULL ?
                                                          *scc->downlinkConfigCommon.frequencyInfoDL.frequencyBandList.list.array[0]->freqBandIndicatorNR :
                                                          *scc->uplinkConfigCommon->frequencyInfoUL.frequencyBandList->list.array[0]->freqBandIndicatorNR);


  if (scc->uplinkConfigCommon->frequencyInfoUL.absoluteFrequencyPointA == NULL)
    cfg->carrier_config.uplink_frequency = cfg->carrier_config.dl_frequency;
  else
    // TODO check if corresponds to what reported in SIB1
    cfg->carrier_config.uplink_frequency = (downlink_frequency[cc_idP][0]/1000) + uplink_frequency_offset[cc_idP][0];

  for (int i=0; i<5; i++) {
    if (i==scc->uplinkConfigCommon->frequencyInfoUL.scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.ul_grid_size[i] = scc->uplinkConfigCommon->frequencyInfoUL.scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.ul_k0[i] = scc->uplinkConfigCommon->frequencyInfoUL.scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
    }
    else {
      cfg->carrier_config.ul_grid_size[i] = 0;
      cfg->carrier_config.ul_k0[i] = 0;
    }
  }

  mac->nr_band = *scc->downlinkConfigCommon.frequencyInfoDL.frequencyBandList.list.array[0]->freqBandIndicatorNR;
  mac->frame_type = get_frame_type(mac->nr_band, get_softmodem_params()->numerology);
  // cell config

  cfg->cell_config.phy_cell_id = mac->physCellId;
  cfg->cell_config.frame_duplex_type = mac->frame_type;

  // SSB config
  cfg->ssb_config.ss_pbch_power = scc->ss_PBCH_BlockPower;
  cfg->ssb_config.scs_common = get_softmodem_params()->numerology;

  // SSB Table config
  cfg->ssb_table.ssb_offset_point_a = scc->downlinkConfigCommon.frequencyInfoDL.offsetToPointA;
  cfg->ssb_table.ssb_period = scc->ssb_PeriodicityServingCell;
  cfg->ssb_table.ssb_subcarrier_offset = mac->ssb_subcarrier_offset;

  if (mac->frequency_range == FR1){
    cfg->ssb_table.ssb_mask_list[0].ssb_mask = ((uint32_t) scc->ssb_PositionsInBurst.inOneGroup.buf[0]) << 24;
    cfg->ssb_table.ssb_mask_list[1].ssb_mask = 0;
  }
  else{
    for (int i=0; i<8; i++){
      if ((scc->ssb_PositionsInBurst.groupPresence->buf[0]>>(7-i))&0x01)
        cfg->ssb_table.ssb_mask_list[i>>2].ssb_mask |= scc->ssb_PositionsInBurst.inOneGroup.buf[0]<<(24-8*(i%4));
    }
  }

  // TDD Table Configuration
  if (cfg->cell_config.frame_duplex_type == TDD){
    set_tdd_config_nr_ue(cfg,
                         scc->uplinkConfigCommon->frequencyInfoUL.scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                         scc->tdd_UL_DL_ConfigurationCommon);
  }

  // PRACH configuration

  uint8_t nb_preambles = 64;
  if(scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles != NULL)
     nb_preambles = *scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles;

  cfg->prach_config.prach_sequence_length = scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present-1;

  if (scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing)
    cfg->prach_config.prach_sub_c_spacing = *scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing;
  else
    cfg->prach_config.prach_sub_c_spacing = scc->downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  cfg->prach_config.restricted_set_config = scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->restrictedSetConfig;

  switch (scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM) {
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
      AssertFatal(1==0,"msg1 FDM identifier %ld undefined (0,1,2,3) \n", scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM);
  }

  cfg->prach_config.num_prach_fd_occasions_list = (fapi_nr_num_prach_fd_occasions_t *) malloc(cfg->prach_config.num_prach_fd_occasions*sizeof(fapi_nr_num_prach_fd_occasions_t));
  for (int i=0; i<cfg->prach_config.num_prach_fd_occasions; i++) {
    cfg->prach_config.num_prach_fd_occasions_list[i].num_prach_fd_occasions = i;
    if (cfg->prach_config.prach_sequence_length)
      cfg->prach_config.num_prach_fd_occasions_list[i].prach_root_sequence_index = scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l139;
    else
      cfg->prach_config.num_prach_fd_occasions_list[i].prach_root_sequence_index = scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l839;
    cfg->prach_config.num_prach_fd_occasions_list[i].k1 = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE) +
      scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart +
      (get_N_RA_RB(cfg->prach_config.prach_sub_c_spacing, scc->uplinkConfigCommon->frequencyInfoUL.scs_SpecificCarrierList.list.array[0]->subcarrierSpacing ) * i);
    cfg->prach_config.num_prach_fd_occasions_list[i].prach_zero_corr_conf = scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig;
    cfg->prach_config.num_prach_fd_occasions_list[i].num_root_sequences = compute_nr_root_seq(scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup,
                                                                                              nb_preambles, mac->frame_type, mac->frequency_range);
    //cfg->prach_config.num_prach_fd_occasions_list[i].num_unused_root_sequences = ???
  }
  cfg->prach_config.ssb_per_rach = scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present-1;

}

void config_common_ue(NR_UE_MAC_INST_t *mac,
		      module_id_t module_id,
		      int cc_idP) {

  fapi_nr_config_request_t        *cfg = &mac->phy_config.config_req;
  NR_ServingCellConfigCommon_t    *scc = mac->scc;
  int i;

  mac->phy_config.Mod_id = module_id;
  mac->phy_config.CC_id = cc_idP;
  
  // carrier config
  LOG_D(MAC, "Entering UE Config Common\n");

  AssertFatal(scc!=NULL,"scc cannot be null\n");

  if (scc) {
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
    mac->frequency_range = band<100?FR1:FR2;
    
    frame_type_t frame_type = get_frame_type(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);
    
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

    // NSA -> take ssb offset from SCS
    cfg->ssb_table.ssb_subcarrier_offset = absolute_diff%(12*scs_scaling);
    
    switch (scc->ssb_PositionsInBurst->present) {
    case 1 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask = scc->ssb_PositionsInBurst->choice.shortBitmap.buf[0]<<24;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask = 0;
      break;
    case 2 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask = ((uint32_t) scc->ssb_PositionsInBurst->choice.mediumBitmap.buf[0]) << 24;
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
    if (cfg->cell_config.frame_duplex_type == TDD){
      set_tdd_config_nr_ue(cfg,
                           scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                           scc->tdd_UL_DL_ConfigurationCommon);
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
      cfg->prach_config.num_prach_fd_occasions_list[i].num_root_sequences = compute_nr_root_seq(scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup,
                                                                                                nb_preambles, frame_type,mac->frequency_range);
      //cfg->prach_config.num_prach_fd_occasions_list[i].num_unused_root_sequences = ???
    }

    cfg->prach_config.ssb_per_rach = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present-1;
    
  } // scc
    
}

/** \brief This function is relavant for the UE procedures for control. It loads the search spaces, the BWPs and the CORESETs into the MAC instance and
    \brief performs assert checks on the relevant RRC configuration.
    @param NR_UE_MAC_INST_t mac: pointer to local MAC instance
    @returns void
    */
void config_control_ue(NR_UE_MAC_INST_t *mac){

  int bwp_id;
  NR_ServingCellConfig_t *scd = mac->cg->spCellConfig->spCellConfigDedicated;
  NR_BWP_Id_t dl_bwp_id = mac->current_DL_BWP.bwp_id;

  // configure DLbwp
  if (scd->downlinkBWP_ToAddModList) {
    for (int i = 0; i < scd->downlinkBWP_ToAddModList->list.count; i++) {
      bwp_id = scd->downlinkBWP_ToAddModList->list.array[i]->bwp_Id;
      mac->DLbwp[bwp_id-1] = scd->downlinkBWP_ToAddModList->list.array[i];
    }
  }

  // configure ULbwp
  if (scd->uplinkConfig->uplinkBWP_ToAddModList) {
    for (int i = 0; i < scd->uplinkConfig->uplinkBWP_ToAddModList->list.count; i++) {
      bwp_id = scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[i]->bwp_Id;
      mac->ULbwp[bwp_id-1] = scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[i];
    }
  }

  configure_ss_coreset(mac, scd, dl_bwp_id);
}


void configure_ss_coreset(NR_UE_MAC_INST_t *mac,
                          NR_ServingCellConfig_t *scd,
                          NR_BWP_Id_t dl_bwp_id) {

  NR_BWP_DownlinkCommon_t *bwp_Common = get_bwp_downlink_common(mac, dl_bwp_id);
  AssertFatal(bwp_Common != NULL, "bwp_Common is null\n");

  NR_SetupRelease_PDCCH_ConfigCommon_t *pdcch_ConfigCommon = bwp_Common->pdcch_ConfigCommon;
  AssertFatal(pdcch_ConfigCommon != NULL, "pdcch_ConfigCommon is null\n");

  // configuring eventual common coreset
  NR_ControlResourceSet_t *coreset = pdcch_ConfigCommon->choice.setup->commonControlResourceSet;
  if (coreset)
    mac->coreset[dl_bwp_id][coreset->controlResourceSetId - 1] = coreset;

  NR_BWP_DownlinkDedicated_t *dl_bwp_Dedicated = dl_bwp_id>0 ? scd->downlinkBWP_ToAddModList->list.array[dl_bwp_id - 1]->bwp_Dedicated:
                                                               scd->initialDownlinkBWP;

  AssertFatal(dl_bwp_Dedicated != NULL, "dl_bwp_Dedicated is null\n");

  NR_SetupRelease_PDCCH_Config_t *pdcch_Config = dl_bwp_Dedicated->pdcch_Config;
  AssertFatal(pdcch_Config != NULL, "pdcch_Config is null\n");

  struct NR_PDCCH_Config__controlResourceSetToAddModList *controlResourceSetToAddModList = pdcch_Config->choice.setup->controlResourceSetToAddModList;
  AssertFatal(controlResourceSetToAddModList != NULL, "controlResourceSetToAddModList is null\n");

  // configuring dedicated coreset
  // In case network reconfigures control resource set with the same ControlResourceSetId as used for commonControlResourceSet configured via PDCCH-ConfigCommon,
  // the configuration from PDCCH-Config always takes precedence
  for (int i=0; i<controlResourceSetToAddModList->list.count; i++) {
    coreset = controlResourceSetToAddModList->list.array[i];
    mac->coreset[dl_bwp_id][coreset->controlResourceSetId - 1] = coreset;
  }

  struct NR_PDCCH_Config__searchSpacesToAddModList *searchSpacesToAddModList = pdcch_Config->choice.setup->searchSpacesToAddModList;
  AssertFatal(searchSpacesToAddModList != NULL, "searchSpacesToAddModList is null\n");
  AssertFatal(searchSpacesToAddModList->list.count > 0, "list of UE specifically configured Search Spaces is empty\n");
  AssertFatal(searchSpacesToAddModList->list.count < FAPI_NR_MAX_SS_PER_BWP, "too many searchpaces per coreset %d\n", searchSpacesToAddModList->list.count);

  // check available Search Spaces in the searchSpacesToAddModList and pass to MAC
  // note: the network configures at most 10 Search Spaces per BWP per cell (including UE-specific and common Search Spaces).
  for (int ss_id = 0; ss_id < searchSpacesToAddModList->list.count; ss_id++) {
    NR_SearchSpace_t *ss = searchSpacesToAddModList->list.array[ss_id];
    AssertFatal(ss->controlResourceSetId != NULL, "ss->controlResourceSetId is null\n");
    AssertFatal(ss->searchSpaceType != NULL, "ss->searchSpaceType is null\n");
    AssertFatal(ss->monitoringSymbolsWithinSlot != NULL, "NR_SearchSpace->monitoringSymbolsWithinSlot is null\n");
    AssertFatal(ss->monitoringSymbolsWithinSlot->buf != NULL, "NR_SearchSpace->monitoringSymbolsWithinSlot->buf is null\n");
    AssertFatal(ss->searchSpaceId <= FAPI_NR_MAX_SS, "Invalid searchSpaceId\n");
    mac->SSpace[dl_bwp_id][ss->searchSpaceId - 1] = ss;
  }

  struct NR_PDCCH_ConfigCommon__commonSearchSpaceList *commonSearchSpaceList = pdcch_ConfigCommon->choice.setup->commonSearchSpaceList;
  AssertFatal(commonSearchSpaceList != NULL, "commonSearchSpaceList is null\n");
  AssertFatal(commonSearchSpaceList->list.count > 0, "PDCCH CSS list has 0 elements\n");

  // Check available CSSs in the commonSearchSpaceList (list of additional common search spaces)
  // note: commonSearchSpaceList SIZE(1..4)
  for (int css_id = 0; css_id < commonSearchSpaceList->list.count; css_id++) {
    NR_SearchSpace_t *css = commonSearchSpaceList->list.array[css_id];
    AssertFatal(css->controlResourceSetId != NULL, "ss->controlResourceSetId is null\n");
    AssertFatal(css->searchSpaceType != NULL, "css->searchSpaceType is null\n");
    AssertFatal(css->monitoringSymbolsWithinSlot != NULL, "css->monitoringSymbolsWithinSlot is null\n");
    AssertFatal(css->monitoringSymbolsWithinSlot->buf != NULL, "css->monitoringSymbolsWithinSlot->buf is null\n");
    AssertFatal(css->searchSpaceId <= FAPI_NR_MAX_SS, "Invalid searchSpaceId\n");
    mac->SSpace[dl_bwp_id][css->searchSpaceId - 1] = css;
  }
}

// todo handle mac_LogicalChannelConfig
int nr_rrc_mac_config_req_ue_logicalChannelBearer(
    module_id_t                     module_id,
    int                             cc_idP,
    uint8_t                         gNB_index,
    long                            logicalChannelIdentity,
    bool                            status){
    NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
    mac->logicalChannelBearer_exist[logicalChannelIdentity] = status;
    return 0;
}


void configure_current_BWP(NR_UE_MAC_INST_t *mac,
                           NR_ServingCellConfigCommonSIB_t *scc,
                           NR_CellGroupConfig_t *cell_group_config)
{
  NR_UE_DL_BWP_t *DL_BWP = &mac->current_DL_BWP;
  NR_UE_UL_BWP_t *UL_BWP = &mac->current_UL_BWP;
  NR_BWP_t dl_genericParameters = {0};
  NR_BWP_t ul_genericParameters = {0};
  NR_BWP_DownlinkCommon_t *bwp_dlcommon = NULL;

  if(scc) {
    DL_BWP->bwp_id = 0;
    UL_BWP->bwp_id = 0;
    bwp_dlcommon = &scc->downlinkConfigCommon.initialDownlinkBWP;
    dl_genericParameters = bwp_dlcommon->genericParameters;
    if(scc->uplinkConfigCommon)
      ul_genericParameters = scc->uplinkConfigCommon->initialUplinkBWP.genericParameters;
    else
      ul_genericParameters = bwp_dlcommon->genericParameters;

    DL_BWP->pdsch_Config = NULL;
    if(bwp_dlcommon->pdsch_ConfigCommon)
      DL_BWP->tdaList = bwp_dlcommon->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
  }

  if(cell_group_config) {
    if (cell_group_config->spCellConfig &&
        cell_group_config->spCellConfig->spCellConfigDedicated) {
      struct NR_ServingCellConfig *spCellConfigDedicated = cell_group_config->spCellConfig->spCellConfigDedicated;
      if (spCellConfigDedicated->firstActiveDownlinkBWP_Id)
        DL_BWP->bwp_id = *spCellConfigDedicated->firstActiveDownlinkBWP_Id;
      if (spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id)
        UL_BWP->bwp_id = *spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id;

      if(mac->scc) {
        bwp_dlcommon = mac->scc->downlinkConfigCommon->initialDownlinkBWP;
      }
      if(mac->scc_SIB) {
        bwp_dlcommon = &mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP;
      }
      NR_BWP_Downlink_t *bwp_downlink = NULL;
      const struct NR_ServingCellConfig__downlinkBWP_ToAddModList *bwpList = spCellConfigDedicated->downlinkBWP_ToAddModList;
      if (bwpList && DL_BWP->bwp_id > 0) {
        for (int i = 0; i < bwpList->list.count; i++) {
          bwp_downlink = bwpList->list.array[i];
          if(bwp_downlink->bwp_Id == DL_BWP->bwp_id)
            break;
        }
        AssertFatal(bwp_downlink != NULL,"Couldn't find DLBWP corresponding to BWP ID %ld\n", DL_BWP->bwp_id);
        dl_genericParameters = bwp_downlink->bwp_Common->genericParameters;
        DL_BWP->pdsch_Config = bwp_downlink->bwp_Dedicated->pdsch_Config->choice.setup;
      }
      else {
        dl_genericParameters = bwp_dlcommon->genericParameters;
        DL_BWP->pdsch_Config = spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup;
      }
      if(DL_BWP->pdsch_Config &&
         DL_BWP->pdsch_Config->pdsch_TimeDomainAllocationList)
        DL_BWP->tdaList = DL_BWP->pdsch_Config->pdsch_TimeDomainAllocationList->choice.setup;
      else
        DL_BWP->tdaList = bwp_dlcommon->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;

      NR_BWP_Uplink_t *bwp_uplink = NULL;
      const struct NR_UplinkConfig__uplinkBWP_ToAddModList *ubwpList = spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList;
      if (ubwpList && UL_BWP->bwp_id > 0) {
        for (int i = 0; i < ubwpList->list.count; i++) {
          bwp_uplink = ubwpList->list.array[i];
          if(bwp_uplink->bwp_Id == UL_BWP->bwp_id)
            break;
        }
        AssertFatal(bwp_uplink != NULL,"Couldn't find ULBWP corresponding to BWP ID %ld\n",UL_BWP->bwp_id);
        ul_genericParameters = bwp_uplink->bwp_Common->genericParameters;
      }
      else {
        if(mac->scc) {
          ul_genericParameters = mac->scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;
        }
        if(mac->scc_SIB) {
          ul_genericParameters = mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters;
        }
      }
    }
    else
      AssertFatal(1==0,"We shouldn't end here in configuring BWP\n");
  }

  DL_BWP->scs = dl_genericParameters.subcarrierSpacing;
  DL_BWP->cyclicprefix = dl_genericParameters.cyclicPrefix;
  DL_BWP->BWPSize = NRRIV2BW(dl_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  DL_BWP->BWPStart = NRRIV2PRBOFFSET(dl_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  UL_BWP->scs = ul_genericParameters.subcarrierSpacing;
  UL_BWP->cyclicprefix = ul_genericParameters.cyclicPrefix;
  UL_BWP->BWPSize = NRRIV2BW(ul_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  UL_BWP->BWPStart = NRRIV2PRBOFFSET(ul_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  DL_BWP->initial_BWPSize = mac->scc ? NRRIV2BW(mac->scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE) :
                            NRRIV2BW(mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  UL_BWP->initial_BWPSize = mac->scc ? NRRIV2BW(mac->scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE) :
                            NRRIV2BW(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  DL_BWP->initial_BWPStart = mac->scc ? NRRIV2PRBOFFSET(mac->scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE) :
                            NRRIV2PRBOFFSET(mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  UL_BWP->initial_BWPStart = mac->scc ? NRRIV2PRBOFFSET(mac->scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE) :
                            NRRIV2PRBOFFSET(mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
}


int nr_rrc_mac_config_req_ue(module_id_t module_id,
                             int cc_idP,
                             uint8_t gNB_index,
                             NR_MIB_t *mibP,
                             NR_ServingCellConfigCommonSIB_t *sccP,
                             NR_CellGroupConfig_t *cell_group_config,
                             NR_CellGroupConfig_t *scell_group_config)
{

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  RA_config_t *ra = &mac->ra;
  fapi_nr_config_request_t *cfg = &mac->phy_config.config_req;

  //  TODO do something FAPI-like P5 L1/L2 config interface in config_si, config_mib, etc.

  if(mibP != NULL){
    mac->mib = mibP;    //  update by every reception
    mac->phy_config.Mod_id = module_id;
    mac->phy_config.CC_id = cc_idP;
  }
  AssertFatal(scell_group_config == NULL || cell_group_config == NULL,
              "both scell_group_config and cell_group_config cannot be non-NULL\n");

  if (sccP != NULL) {

    mac->scc_SIB = sccP;
    LOG_D(NR_MAC, "In %s: Keeping ServingCellConfigCommonSIB\n", __FUNCTION__);
    config_common_ue_sa(mac, module_id, cc_idP);
    configure_current_BWP(mac, sccP, NULL);

    int num_slots_ul = nr_slots_per_frame[mac->mib->subCarrierSpacingCommon];
    if(cfg->cell_config.frame_duplex_type == TDD){
      num_slots_ul = mac->scc_SIB->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots;
      if (mac->scc_SIB->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols > 0) {
        num_slots_ul++;
      }
    }
    LOG_I(NR_MAC, "Initializing ul_config_request. num_slots_ul = %d\n", num_slots_ul);
    mac->ul_config_request = (fapi_nr_ul_config_request_t *)calloc(num_slots_ul, sizeof(fapi_nr_ul_config_request_t));
    for (int i=0; i<num_slots_ul; i++)
      pthread_mutex_init(&(mac->ul_config_request[i].mutex_ul_config), NULL);
    // Setup the SSB to Rach Occasionsif (cell_group_config->spCellConfig) { mapping according to the config
    build_ssb_to_ro_map(mac);//->scc, mac->phy_config.config_req.cell_config.frame_duplex_type);
    if (!get_softmodem_params()->emulate_l1)
      mac->if_module->phy_config_request(&mac->phy_config);
  }
  if(scell_group_config != NULL ){
    mac->cg = scell_group_config;
    mac->servCellIndex = *scell_group_config->spCellConfig->servCellIndex;
    if (scell_group_config->spCellConfig->reconfigurationWithSync) {
      if (scell_group_config->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated) {
        ra->rach_ConfigDedicated = scell_group_config->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink;
      }
      mac->scc = scell_group_config->spCellConfig->reconfigurationWithSync->spCellConfigCommon;
      mac->physCellId = *mac->scc->physCellId;
      config_common_ue(mac,module_id,cc_idP);
      mac->crnti = scell_group_config->spCellConfig->reconfigurationWithSync->newUE_Identity;
      LOG_I(MAC,"Configuring CRNTI %x\n",mac->crnti);
    }
    configure_current_BWP(mac, NULL, scell_group_config);
    config_control_ue(mac);
    // Setup the SSB to Rach Occasions mapping according to the config
    build_ssb_to_ro_map(mac);
  }
  else if (cell_group_config != NULL ){
    LOG_I(MAC,"Applying CellGroupConfig from gNodeB\n");
    mac->cg = cell_group_config;
    if (cell_group_config->spCellConfig)
      mac->servCellIndex = cell_group_config->spCellConfig->servCellIndex ? *cell_group_config->spCellConfig->servCellIndex : 0;
    else
      mac->servCellIndex = 0;

    mac->scheduling_info.periodicBSR_SF = MAC_UE_BSR_TIMER_NOT_RUNNING;
    mac->scheduling_info.retxBSR_SF = MAC_UE_BSR_TIMER_NOT_RUNNING;
    mac->BSR_reporting_active = NR_BSR_TRIGGER_NONE;
    LOG_D(MAC, "[UE %d]: periodic BSR %d (SF), retx BSR %d (SF)\n",
          module_id,
          mac->scheduling_info.periodicBSR_SF,
          mac->scheduling_info.retxBSR_SF);

    configure_current_BWP(mac, NULL, cell_group_config);
    config_control_ue(mac);

    if (get_softmodem_params()->nsa) {
      if (cell_group_config->spCellConfig && cell_group_config->spCellConfig->reconfigurationWithSync) {
        if (cell_group_config->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated) {
          ra->rach_ConfigDedicated = cell_group_config->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink;
        }
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
      build_ssb_to_ro_map(mac);
    }
  }
  return 0;
}
