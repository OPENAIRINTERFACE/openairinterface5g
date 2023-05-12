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
#include "SCHED_NR/phy_frame_config_nr.h"

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
  NR_FrequencyInfoDL_SIB_t *frequencyInfoDL = &scc->downlinkConfigCommon.frequencyInfoDL;
  int bw_index = get_supported_band_index(frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                          *frequencyInfoDL->frequencyBandList.list.array[0]->freqBandIndicatorNR,
                                          frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
  cfg->carrier_config.dl_bandwidth = get_supported_bw_mhz(*frequencyInfoDL->frequencyBandList.list.array[0]->freqBandIndicatorNR > 256 ? FR2 : FR1, bw_index);

  uint64_t dl_bw_khz = (12 * frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth) *
                       (15 << frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing);
  cfg->carrier_config.dl_frequency = (downlink_frequency[cc_idP][0]/1000) - (dl_bw_khz>>1);

  for (int i=0; i<5; i++) {
    if (i == frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.dl_grid_size[i] = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.dl_k0[i] = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
    }
    else {
      cfg->carrier_config.dl_grid_size[i] = 0;
      cfg->carrier_config.dl_k0[i] = 0;
    }
  }

  NR_FrequencyInfoUL_SIB_t *frequencyInfoUL = &scc->uplinkConfigCommon->frequencyInfoUL;
  int UL_band_ind = frequencyInfoUL->frequencyBandList == NULL ?
                    *frequencyInfoDL->frequencyBandList.list.array[0]->freqBandIndicatorNR :
                    *frequencyInfoUL->frequencyBandList->list.array[0]->freqBandIndicatorNR;
  bw_index = get_supported_band_index(frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                      UL_band_ind,
                                      frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
  cfg->carrier_config.uplink_bandwidth = get_supported_bw_mhz(UL_band_ind > 256 ? FR2 : FR1, bw_index);

  if (frequencyInfoUL->absoluteFrequencyPointA == NULL)
    cfg->carrier_config.uplink_frequency = cfg->carrier_config.dl_frequency;
  else
    // TODO check if corresponds to what reported in SIB1
    cfg->carrier_config.uplink_frequency = (downlink_frequency[cc_idP][0]/1000) + uplink_frequency_offset[cc_idP][0];

  for (int i = 0; i < 5; i++) {
    if (i == frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.ul_grid_size[i] = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.ul_k0[i] = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
    }
    else {
      cfg->carrier_config.ul_grid_size[i] = 0;
      cfg->carrier_config.ul_k0[i] = 0;
    }
  }

  mac->frame_type = get_frame_type(mac->nr_band, get_softmodem_params()->numerology);
  // cell config

  cfg->cell_config.phy_cell_id = mac->physCellId;
  cfg->cell_config.frame_duplex_type = mac->frame_type;

  // SSB config
  cfg->ssb_config.ss_pbch_power = scc->ss_PBCH_BlockPower;
  cfg->ssb_config.scs_common = get_softmodem_params()->numerology;

  // SSB Table config
  cfg->ssb_table.ssb_offset_point_a = frequencyInfoDL->offsetToPointA;
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
                         frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                         scc->tdd_UL_DL_ConfigurationCommon);
  }

  // PRACH configuration

  uint8_t nb_preambles = 64;
  NR_RACH_ConfigCommon_t *rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup;
  if(rach_ConfigCommon->totalNumberOfRA_Preambles != NULL)
     nb_preambles = *rach_ConfigCommon->totalNumberOfRA_Preambles;

  cfg->prach_config.prach_sequence_length = rach_ConfigCommon->prach_RootSequenceIndex.present-1;

  if (rach_ConfigCommon->msg1_SubcarrierSpacing)
    cfg->prach_config.prach_sub_c_spacing = *rach_ConfigCommon->msg1_SubcarrierSpacing;
  else
    cfg->prach_config.prach_sub_c_spacing = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  cfg->prach_config.restricted_set_config = rach_ConfigCommon->restrictedSetConfig;

  switch (rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM) {
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
      AssertFatal(1==0,"msg1 FDM identifier %ld undefined (0,1,2,3) \n", rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM);
  }

  cfg->prach_config.num_prach_fd_occasions_list = (fapi_nr_num_prach_fd_occasions_t *) malloc(cfg->prach_config.num_prach_fd_occasions*sizeof(fapi_nr_num_prach_fd_occasions_t));
  for (int i=0; i<cfg->prach_config.num_prach_fd_occasions; i++) {
    fapi_nr_num_prach_fd_occasions_t *prach_fd_occasion = &cfg->prach_config.num_prach_fd_occasions_list[i];
    prach_fd_occasion->num_prach_fd_occasions = i;
    if (cfg->prach_config.prach_sequence_length)
      prach_fd_occasion->prach_root_sequence_index = rach_ConfigCommon->prach_RootSequenceIndex.choice.l139;
    else
      prach_fd_occasion->prach_root_sequence_index = rach_ConfigCommon->prach_RootSequenceIndex.choice.l839;
    prach_fd_occasion->k1 = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP.genericParameters.locationAndBandwidth, MAX_BWP_SIZE) +
                                            rach_ConfigCommon->rach_ConfigGeneric.msg1_FrequencyStart +
                                            (get_N_RA_RB(cfg->prach_config.prach_sub_c_spacing, frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing ) * i);
    prach_fd_occasion->prach_zero_corr_conf = rach_ConfigCommon->rach_ConfigGeneric.zeroCorrelationZoneConfig;
    prach_fd_occasion->num_root_sequences = compute_nr_root_seq(rach_ConfigCommon,
                                                                nb_preambles, mac->frame_type, mac->frequency_range);
    //prach_fd_occasion->num_unused_root_sequences = ???
  }
  cfg->prach_config.ssb_per_rach = rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present-1;

}

void config_common_ue(NR_UE_MAC_INST_t *mac,
		      module_id_t module_id,
		      int cc_idP)
{
  fapi_nr_config_request_t        *cfg = &mac->phy_config.config_req;
  NR_ServingCellConfigCommon_t    *scc = mac->scc;

  mac->phy_config.Mod_id = module_id;
  mac->phy_config.CC_id = cc_idP;
  
  // carrier config
  LOG_D(MAC, "Entering UE Config Common\n");

  AssertFatal(scc != NULL, "scc cannot be null\n");

  struct NR_FrequencyInfoDL *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
  int bw_index = get_supported_band_index(frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                          *frequencyInfoDL->frequencyBandList.list.array[0],
                                          frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
  cfg->carrier_config.dl_bandwidth = get_supported_bw_mhz(*frequencyInfoDL->frequencyBandList.list.array[0] > 256 ? FR2 : FR1, bw_index);
    
  cfg->carrier_config.dl_frequency = from_nrarfcn(*frequencyInfoDL->frequencyBandList.list.array[0],
                                                  *scc->ssbSubcarrierSpacing,
                                                  frequencyInfoDL->absoluteFrequencyPointA)/1000; // freq in kHz
    
  for (int i = 0; i < 5; i++) {
    if (i == frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.dl_grid_size[i] = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.dl_k0[i] = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
    }
    else {
      cfg->carrier_config.dl_grid_size[i] = 0;
      cfg->carrier_config.dl_k0[i] = 0;
    }
  }

  struct NR_FrequencyInfoUL *frequencyInfoUL = scc->uplinkConfigCommon->frequencyInfoUL;
  bw_index = get_supported_band_index(frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                      *frequencyInfoUL->frequencyBandList->list.array[0],
                                      frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
  cfg->carrier_config.uplink_bandwidth = get_supported_bw_mhz(*frequencyInfoUL->frequencyBandList->list.array[0] > 256 ? FR2 : FR1, bw_index);

  int UL_pointA;
  if (frequencyInfoUL->absoluteFrequencyPointA == NULL)
    UL_pointA = frequencyInfoDL->absoluteFrequencyPointA;
  else
    UL_pointA = *frequencyInfoUL->absoluteFrequencyPointA;

  cfg->carrier_config.uplink_frequency = from_nrarfcn(*frequencyInfoUL->frequencyBandList->list.array[0],
                                                      *scc->ssbSubcarrierSpacing,
                                                      UL_pointA) / 1000; // freq in kHz

  for (int i = 0; i < 5; i++) {
    if (i == frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.ul_grid_size[i] = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.ul_k0[i] = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
    }
    else {
      cfg->carrier_config.ul_grid_size[i] = 0;
      cfg->carrier_config.ul_k0[i] = 0;
    }
  }

  uint32_t band = *frequencyInfoDL->frequencyBandList.list.array[0];
  mac->frequency_range = band<100?FR1:FR2;

  frame_type_t frame_type = get_frame_type(*frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);

  // cell config

  cfg->cell_config.phy_cell_id = *scc->physCellId;
  cfg->cell_config.frame_duplex_type = frame_type;

  // SSB config
  cfg->ssb_config.ss_pbch_power = scc->ss_PBCH_BlockPower;
  cfg->ssb_config.scs_common = *scc->ssbSubcarrierSpacing;

  // SSB Table config
  int scs_scaling = 1<<(cfg->ssb_config.scs_common);
  if (frequencyInfoDL->absoluteFrequencyPointA < 600000)
    scs_scaling = scs_scaling*3;
  if (frequencyInfoDL->absoluteFrequencyPointA > 2016666)
    scs_scaling = scs_scaling>>2;
  uint32_t absolute_diff = (*frequencyInfoDL->absoluteFrequencySSB - frequencyInfoDL->absoluteFrequencyPointA);
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
    for (int i = 0; i < 4; i++) {
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
                         frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                         scc->tdd_UL_DL_ConfigurationCommon);
  }

  // PRACH configuration
  uint8_t nb_preambles = 64;
  NR_RACH_ConfigCommon_t *rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  if(rach_ConfigCommon->totalNumberOfRA_Preambles != NULL)
    nb_preambles = *rach_ConfigCommon->totalNumberOfRA_Preambles;

  cfg->prach_config.prach_sequence_length = rach_ConfigCommon->prach_RootSequenceIndex.present-1;

  if (rach_ConfigCommon->msg1_SubcarrierSpacing)
    cfg->prach_config.prach_sub_c_spacing = *rach_ConfigCommon->msg1_SubcarrierSpacing;
  else 
    cfg->prach_config.prach_sub_c_spacing = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  cfg->prach_config.restricted_set_config = rach_ConfigCommon->restrictedSetConfig;
    
  switch (rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM) {
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
      AssertFatal(1==0,"msg1 FDM identifier %ld undefined (0,1,2,3) \n", rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM);
  }

  cfg->prach_config.num_prach_fd_occasions_list = (fapi_nr_num_prach_fd_occasions_t *) malloc(cfg->prach_config.num_prach_fd_occasions*sizeof(fapi_nr_num_prach_fd_occasions_t));
  for (int i = 0; i < cfg->prach_config.num_prach_fd_occasions; i++) {
    fapi_nr_num_prach_fd_occasions_t *prach_fd_occasion = &cfg->prach_config.num_prach_fd_occasions_list[i];
    prach_fd_occasion->num_prach_fd_occasions = i;
    if (cfg->prach_config.prach_sequence_length)
      prach_fd_occasion->prach_root_sequence_index = rach_ConfigCommon->prach_RootSequenceIndex.choice.l139;
    else
      prach_fd_occasion->prach_root_sequence_index = rach_ConfigCommon->prach_RootSequenceIndex.choice.l839;

    prach_fd_occasion->k1 = rach_ConfigCommon->rach_ConfigGeneric.msg1_FrequencyStart;
    prach_fd_occasion->prach_zero_corr_conf = rach_ConfigCommon->rach_ConfigGeneric.zeroCorrelationZoneConfig;
    prach_fd_occasion->num_root_sequences = compute_nr_root_seq(rach_ConfigCommon,
                                                                nb_preambles,
                                                                frame_type,
                                                                mac->frequency_range);
    //prach_fd_occasion->num_unused_root_sequences = ???
  }

  cfg->prach_config.ssb_per_rach = rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present-1;
}


NR_SearchSpace_t *get_common_search_space(const struct NR_PDCCH_ConfigCommon__commonSearchSpaceList *commonSearchSpaceList,
                                          const NR_UE_MAC_INST_t *mac,
                                          const NR_SearchSpaceId_t ss_id)
{
  if (ss_id == 0)
    return mac->search_space_zero;

  NR_SearchSpace_t *css = NULL;
  for (int i = 0; i < commonSearchSpaceList->list.count; i++) {
    if (commonSearchSpaceList->list.array[i]->searchSpaceId == ss_id) {
      css = commonSearchSpaceList->list.array[i];
      break;
    }
  }
  AssertFatal(css, "Couldn't find CSS with Id %ld\n", ss_id);
  return css;
}

void configure_ss_coreset(NR_UE_MAC_INST_t *mac,
                          const NR_PDCCH_ConfigCommon_t *pdcch_ConfigCommon,
                          const NR_PDCCH_Config_t *pdcch_Config)
{

  // configuration of search spaces
  if (pdcch_ConfigCommon) {
    mac->otherSI_SS = pdcch_ConfigCommon->searchSpaceOtherSystemInformation ?
                      get_common_search_space(pdcch_ConfigCommon->commonSearchSpaceList, mac,
                                              *pdcch_ConfigCommon->searchSpaceOtherSystemInformation) :
                      NULL;
    mac->ra_SS = pdcch_ConfigCommon->ra_SearchSpace ?
                 get_common_search_space(pdcch_ConfigCommon->commonSearchSpaceList, mac,
                                         *pdcch_ConfigCommon->ra_SearchSpace) :
                 NULL;
    mac->paging_SS = pdcch_ConfigCommon->pagingSearchSpace ?
                     get_common_search_space(pdcch_ConfigCommon->commonSearchSpaceList, mac,
                                             *pdcch_ConfigCommon->pagingSearchSpace) :
                     NULL;
  }
  if(pdcch_Config &&
     pdcch_Config->searchSpacesToAddModList) {
    int ss_configured = 0;
    struct NR_PDCCH_Config__searchSpacesToAddModList *searchSpacesToAddModList = pdcch_Config->searchSpacesToAddModList;
    for (int i = 0; i < searchSpacesToAddModList->list.count; i++) {
      AssertFatal(ss_configured < FAPI_NR_MAX_SS, "Attempting to configure %d SS but only %d per BWP are allowed",
                  ss_configured + 1, FAPI_NR_MAX_SS);
      mac->BWP_searchspaces[ss_configured] = searchSpacesToAddModList->list.array[i];
      ss_configured++;
    }
    for (int i = ss_configured; i < FAPI_NR_MAX_SS; i++)
      mac->BWP_searchspaces[i] = NULL;
  }

  // configuration of coresets
  int cset_configured = 0;
  int common_cset_id = -1;
  if (pdcch_ConfigCommon &&
      pdcch_ConfigCommon->commonControlResourceSet) {
    mac->BWP_coresets[cset_configured] = pdcch_ConfigCommon->commonControlResourceSet;
    common_cset_id = pdcch_ConfigCommon->commonControlResourceSet->controlResourceSetId;
    cset_configured++;
  }
  if(pdcch_Config &&
     pdcch_Config->controlResourceSetToAddModList) {
    struct NR_PDCCH_Config__controlResourceSetToAddModList *controlResourceSetToAddModList = pdcch_Config->controlResourceSetToAddModList;
    for (int i = 0; i < controlResourceSetToAddModList->list.count; i++) {
      AssertFatal(cset_configured < FAPI_NR_MAX_CORESET_PER_BWP, "Attempting to configure %d CORESET but only %d per BWP are allowed",
                  cset_configured + 1, FAPI_NR_MAX_CORESET_PER_BWP);
      // In case network reconfigures control resource set with the same ControlResourceSetId as used for commonControlResourceSet
      // configured via PDCCH-ConfigCommon, the configuration from PDCCH-Config always takes precedence
      if (controlResourceSetToAddModList->list.array[i]->controlResourceSetId == common_cset_id)
        mac->BWP_coresets[0] = controlResourceSetToAddModList->list.array[i];
      else {
        mac->BWP_coresets[cset_configured] = controlResourceSetToAddModList->list.array[i];
        cset_configured++;
      }
    }
  }
  for (int i = cset_configured; i < FAPI_NR_MAX_CORESET_PER_BWP; i++)
    mac->BWP_coresets[i] = NULL;
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
  NR_BWP_UplinkCommon_t *bwp_ulcommon = NULL;
  DL_BWP->n_dl_bwp = 0;
  UL_BWP->n_ul_bwp = 0;

  if(scc) {
    DL_BWP->bwp_id = 0;
    UL_BWP->bwp_id = 0;
    bwp_dlcommon = &scc->downlinkConfigCommon.initialDownlinkBWP;
    bwp_ulcommon = &scc->uplinkConfigCommon->initialUplinkBWP;
    dl_genericParameters = bwp_dlcommon->genericParameters;
    if(scc->uplinkConfigCommon)
      ul_genericParameters = scc->uplinkConfigCommon->initialUplinkBWP.genericParameters;
    else
      ul_genericParameters = bwp_dlcommon->genericParameters;

    DL_BWP->pdsch_Config = NULL;
    if (bwp_dlcommon->pdsch_ConfigCommon)
      DL_BWP->tdaList_Common = bwp_dlcommon->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    if (bwp_ulcommon->pusch_ConfigCommon) {
      UL_BWP->tdaList_Common = bwp_ulcommon->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
      UL_BWP->msg3_DeltaPreamble = bwp_ulcommon->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble;
    }
    if (bwp_ulcommon->pucch_ConfigCommon)
      UL_BWP->pucch_ConfigCommon = bwp_ulcommon->pucch_ConfigCommon->choice.setup;
    if (bwp_ulcommon->rach_ConfigCommon)
      UL_BWP->rach_ConfigCommon = bwp_ulcommon->rach_ConfigCommon->choice.setup;
    if (bwp_dlcommon->pdcch_ConfigCommon)
      configure_ss_coreset(mac, bwp_dlcommon->pdcch_ConfigCommon->choice.setup, NULL);
  }

  if(cell_group_config) {
    if (cell_group_config->physicalCellGroupConfig) {
      DL_BWP->pdsch_HARQ_ACK_Codebook = &cell_group_config->physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook;
      UL_BWP->harq_ACK_SpatialBundlingPUCCH = cell_group_config->physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH;
    }
    if (cell_group_config->spCellConfig &&
        cell_group_config->spCellConfig->spCellConfigDedicated) {
      struct NR_ServingCellConfig *spCellConfigDedicated = cell_group_config->spCellConfig->spCellConfigDedicated;
      UL_BWP->csi_MeasConfig = spCellConfigDedicated->csi_MeasConfig ? spCellConfigDedicated->csi_MeasConfig->choice.setup : NULL;
      UL_BWP->pusch_servingcellconfig =
          spCellConfigDedicated->uplinkConfig && spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig ? spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup : NULL;
      DL_BWP->pdsch_servingcellconfig = spCellConfigDedicated->pdsch_ServingCellConfig ? spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup : NULL;

      if (spCellConfigDedicated->firstActiveDownlinkBWP_Id)
        DL_BWP->bwp_id = *spCellConfigDedicated->firstActiveDownlinkBWP_Id;
      if (spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id)
        UL_BWP->bwp_id = *spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id;

      if (mac->scc) {
        bwp_dlcommon = mac->scc->downlinkConfigCommon->initialDownlinkBWP;
        bwp_ulcommon = mac->scc->uplinkConfigCommon->initialUplinkBWP;
      }
      else if (mac->scc_SIB) {
        bwp_dlcommon = &mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP;
        bwp_ulcommon = &mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP;
      }
      else
        AssertFatal(false, "Either SCC or SCC SIB should be non-NULL\n");

      NR_BWP_Downlink_t *bwp_downlink = NULL;
      const struct NR_ServingCellConfig__downlinkBWP_ToAddModList *bwpList = spCellConfigDedicated->downlinkBWP_ToAddModList;
      if (bwpList)
        DL_BWP->n_dl_bwp = bwpList->list.count;
      if (bwpList && DL_BWP->bwp_id > 0) {
        for (int i = 0; i < bwpList->list.count; i++) {
          bwp_downlink = bwpList->list.array[i];
          if(bwp_downlink->bwp_Id == DL_BWP->bwp_id)
            break;
        }
        AssertFatal(bwp_downlink != NULL,"Couldn't find DLBWP corresponding to BWP ID %ld\n", DL_BWP->bwp_id);
        dl_genericParameters = bwp_downlink->bwp_Common->genericParameters;
        DL_BWP->pdsch_Config = bwp_downlink->bwp_Dedicated->pdsch_Config->choice.setup;
        DL_BWP->tdaList_Common = bwp_downlink->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
        configure_ss_coreset(mac,
                             bwp_downlink->bwp_Common->pdcch_ConfigCommon ? bwp_downlink->bwp_Common->pdcch_ConfigCommon->choice.setup : NULL,
                             bwp_downlink->bwp_Dedicated->pdcch_Config ? bwp_downlink->bwp_Dedicated->pdcch_Config->choice.setup : NULL);

      }
      else {
        dl_genericParameters = bwp_dlcommon->genericParameters;
        DL_BWP->pdsch_Config = spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup;
        DL_BWP->tdaList_Common = bwp_dlcommon->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
        configure_ss_coreset(mac,
                             bwp_dlcommon->pdcch_ConfigCommon ? bwp_dlcommon->pdcch_ConfigCommon->choice.setup : NULL,
                             spCellConfigDedicated->initialDownlinkBWP->pdcch_Config ? spCellConfigDedicated->initialDownlinkBWP->pdcch_Config->choice.setup : NULL);

      }

      UL_BWP->msg3_DeltaPreamble = bwp_ulcommon->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble;

      NR_BWP_Uplink_t *bwp_uplink = NULL;
      const struct NR_UplinkConfig__uplinkBWP_ToAddModList *ubwpList = spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList;
      if (ubwpList)
        UL_BWP->n_ul_bwp = ubwpList->list.count;
      if (ubwpList && UL_BWP->bwp_id > 0) {
        for (int i = 0; i < ubwpList->list.count; i++) {
          bwp_uplink = ubwpList->list.array[i];
          if(bwp_uplink->bwp_Id == UL_BWP->bwp_id)
            break;
        }
        AssertFatal(bwp_uplink != NULL,"Couldn't find ULBWP corresponding to BWP ID %ld\n",UL_BWP->bwp_id);
        ul_genericParameters = bwp_uplink->bwp_Common->genericParameters;
        UL_BWP->tdaList_Common = bwp_uplink->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
        UL_BWP->pusch_Config = bwp_uplink->bwp_Dedicated->pusch_Config->choice.setup;
        UL_BWP->pucch_Config = bwp_uplink->bwp_Dedicated->pucch_Config->choice.setup;
        UL_BWP->srs_Config = bwp_uplink->bwp_Dedicated->srs_Config->choice.setup;
        UL_BWP->configuredGrantConfig = bwp_uplink->bwp_Dedicated->configuredGrantConfig ? bwp_uplink->bwp_Dedicated->configuredGrantConfig->choice.setup : NULL;
        if (bwp_uplink->bwp_Common->pucch_ConfigCommon)
          UL_BWP->pucch_ConfigCommon = bwp_uplink->bwp_Common->pucch_ConfigCommon->choice.setup;
        if (bwp_uplink->bwp_Common->rach_ConfigCommon)
          UL_BWP->rach_ConfigCommon = bwp_uplink->bwp_Common->rach_ConfigCommon->choice.setup;
      }
      else {
        UL_BWP->tdaList_Common = bwp_ulcommon->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
        UL_BWP->pusch_Config = spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pusch_Config->choice.setup;
        UL_BWP->pucch_Config = spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup;
        UL_BWP->srs_Config = spCellConfigDedicated->uplinkConfig->initialUplinkBWP->srs_Config->choice.setup;
        UL_BWP->configuredGrantConfig =
            spCellConfigDedicated->uplinkConfig->initialUplinkBWP->configuredGrantConfig ? spCellConfigDedicated->uplinkConfig->initialUplinkBWP->configuredGrantConfig->choice.setup : NULL;
        ul_genericParameters = bwp_ulcommon->genericParameters;
        if (bwp_ulcommon->pucch_ConfigCommon)
          UL_BWP->pucch_ConfigCommon = bwp_ulcommon->pucch_ConfigCommon->choice.setup;
        if (bwp_ulcommon->rach_ConfigCommon)
          UL_BWP->rach_ConfigCommon = bwp_ulcommon->rach_ConfigCommon->choice.setup;
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

void ue_init_config_request(NR_UE_MAC_INST_t *mac, int scs)
{
  int slots_per_frame = nr_slots_per_frame[scs];
  LOG_I(NR_MAC, "Initializing dl and ul config_request. num_slots = %d\n", slots_per_frame);
  mac->dl_config_request = calloc(slots_per_frame, sizeof(*mac->dl_config_request));
  mac->ul_config_request = calloc(slots_per_frame, sizeof(*mac->ul_config_request));
  for (int i = 0; i < slots_per_frame; i++)
    pthread_mutex_init(&(mac->ul_config_request[i].mutex_ul_config), NULL);
}

void nr_rrc_mac_config_req_mib(module_id_t module_id,
                               int cc_idP,
                               NR_MIB_t *mib,
                               bool sched_sib1)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  AssertFatal(mib, "MIB should not be NULL\n");
  // initialize dl and ul config_request upon first reception of MIB
  mac->mib = mib;    //  update by every reception
  mac->phy_config.Mod_id = module_id;
  mac->phy_config.CC_id = cc_idP;
  mac->get_sib1 = sched_sib1;
}

void nr_rrc_mac_config_req_sib1(module_id_t module_id,
                                int cc_idP,
                                NR_ServingCellConfigCommonSIB_t *scc)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  AssertFatal(scc, "SIB1 SCC should not be NULL\n");
  mac->scc_SIB = scc;
  mac->nr_band = *scc->downlinkConfigCommon.frequencyInfoDL.frequencyBandList.list.array[0]->freqBandIndicatorNR;
  config_common_ue_sa(mac, module_id, cc_idP);
  configure_current_BWP(mac, scc, NULL);

  // Setup the SSB to Rach Occasionsif (cell_group_config->spCellConfig) { mapping according to the config
  build_ssb_to_ro_map(mac);
  if (!get_softmodem_params()->emulate_l1)
    mac->if_module->phy_config_request(&mac->phy_config);
  mac->phy_config_request_sent = true;
}

void nr_rrc_mac_config_req_mcg(module_id_t module_id,
                               int cc_idP,
                               NR_CellGroupConfig_t *cell_group_config)
{
  LOG_I(MAC,"Applying CellGroupConfig from gNodeB\n");
  AssertFatal(cell_group_config, "CellGroupConfig should not be NULL\n");
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
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

  RA_config_t *ra = &mac->ra;
  if (cell_group_config->spCellConfig && cell_group_config->spCellConfig->reconfigurationWithSync) {
    LOG_A(NR_MAC, "Received the reconfigurationWithSync in %s\n", __FUNCTION__);
    if (cell_group_config->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated) {
      ra->rach_ConfigDedicated = cell_group_config->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink;
    }
    mac->scc = cell_group_config->spCellConfig->reconfigurationWithSync->spCellConfigCommon;
    mac->nr_band = *mac->scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
    if (mac->scc_SIB) {
      ASN_STRUCT_FREE(asn_DEF_NR_ServingCellConfigCommonSIB, mac->scc_SIB);
      mac->scc_SIB = NULL;
    }
    mac->state = UE_NOT_SYNC;
    mac->ra.ra_state = RA_UE_IDLE;
    mac->physCellId = *mac->scc->physCellId;
    if (!get_softmodem_params()->emulate_l1) {
      mac->synch_request.Mod_id = module_id;
      mac->synch_request.CC_id = cc_idP;
      mac->synch_request.synch_req.target_Nid_cell = mac->physCellId;
      mac->if_module->synch_request(&mac->synch_request);
    }
    mac->crnti = cell_group_config->spCellConfig->reconfigurationWithSync->newUE_Identity;
    LOG_I(NR_MAC, "Configuring CRNTI %x\n", mac->crnti);

    configure_current_BWP(mac, NULL, cell_group_config);
    config_common_ue(mac, module_id, cc_idP);
    nr_ue_mac_default_configs(mac);

    if (!get_softmodem_params()->emulate_l1) {
      mac->if_module->phy_config_request(&mac->phy_config);
      mac->phy_config_request_sent = true;
    }

    // Setup the SSB to Rach Occasions mapping according to the config
    build_ssb_to_ro_map(mac);
  } else {
    configure_current_BWP(mac, NULL, cell_group_config);
  }
}

void nr_rrc_mac_config_req_scg(module_id_t module_id,
                               int cc_idP,
                               NR_CellGroupConfig_t *scell_group_config)
{

  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  RA_config_t *ra = &mac->ra;

  AssertFatal(scell_group_config, "scell_group_config cannot be NULL\n");

  mac->cg = scell_group_config;
  mac->servCellIndex = *scell_group_config->spCellConfig->servCellIndex;
  if (scell_group_config->spCellConfig->reconfigurationWithSync) {
    if (scell_group_config->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated) {
      ra->rach_ConfigDedicated = scell_group_config->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink;
    }
    mac->scc = scell_group_config->spCellConfig->reconfigurationWithSync->spCellConfigCommon;
    mac->nr_band = *mac->scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
    mac->physCellId = *mac->scc->physCellId;
    config_common_ue(mac,module_id,cc_idP);
    mac->crnti = scell_group_config->spCellConfig->reconfigurationWithSync->newUE_Identity;
    LOG_I(MAC,"Configuring CRNTI %x\n",mac->crnti);
  }
  configure_current_BWP(mac, NULL, scell_group_config);
  // Setup the SSB to Rach Occasions mapping according to the config
  build_ssb_to_ro_map(mac);
}
