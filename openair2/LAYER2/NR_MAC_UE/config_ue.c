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

#define _GNU_SOURCE

//#include "mac_defs.h"
#include <NR_MAC_gNB/mac_proto.h>
#include "NR_MAC_UE/mac_proto.h"
#include "NR_MAC-CellGroupConfig.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "common/utils/nr/nr_common.h"
#include "executables/softmodem-common.h"
#include "SCHED_NR/phy_frame_config_nr.h"
#include "oai_asn1.h"

const long logicalChannelGroup0_NR = 0;
typedef struct NR_LogicalChannelConfig__ul_SpecificParameters LcConfig_UlParamas_t;

const LcConfig_UlParamas_t NR_LCSRB1 = {
    .priority = 1,
    .prioritisedBitRate = NR_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity,
    .logicalChannelGroup = (long *)&logicalChannelGroup0_NR};

const LcConfig_UlParamas_t NR_LCSRB2 = {
    .priority = 3,
    .prioritisedBitRate = NR_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity,
    .logicalChannelGroup = (long *)&logicalChannelGroup0_NR};

const LcConfig_UlParamas_t NR_LCSRB3 = {
    .priority = 1,
    .prioritisedBitRate = NR_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity,
    .logicalChannelGroup = (long *)&logicalChannelGroup0_NR};

// these are the default values for SRB configurations(SRB1 and SRB2) as mentioned in 36.331 pg 258-259
const NR_LogicalChannelConfig_t NR_SRB1_logicalChannelConfig_defaultValue = {.ul_SpecificParameters =
                                                                                 (LcConfig_UlParamas_t *)&NR_LCSRB1};
const NR_LogicalChannelConfig_t NR_SRB2_logicalChannelConfig_defaultValue = {.ul_SpecificParameters =
                                                                                 (LcConfig_UlParamas_t *)&NR_LCSRB2};
const NR_LogicalChannelConfig_t NR_SRB3_logicalChannelConfig_defaultValue = {.ul_SpecificParameters =
                                                                                 (LcConfig_UlParamas_t *)&NR_LCSRB3};

void set_tdd_config_nr_ue(fapi_nr_tdd_table_t *tdd_table,
                          int mu,
                          NR_TDD_UL_DL_Pattern_t *pattern)
{
  const int nrofDownlinkSlots = pattern->nrofDownlinkSlots;
  const int nrofDownlinkSymbols = pattern->nrofDownlinkSymbols;
  const int nrofUplinkSlots = pattern->nrofUplinkSlots;
  const int nrofUplinkSymbols = pattern->nrofUplinkSymbols;
  const int nb_periods_per_frame = get_nb_periods_per_frame(pattern->dl_UL_TransmissionPeriodicity);
  const int nb_slots_per_period = ((1 << mu) * NR_NUMBER_OF_SUBFRAMES_PER_FRAME) / nb_periods_per_frame;
  tdd_table->tdd_period_in_slots = nb_slots_per_period;

  if ((nrofDownlinkSymbols + nrofUplinkSymbols) == 0)
    AssertFatal(nb_slots_per_period == (nrofDownlinkSlots + nrofUplinkSlots),
                "set_tdd_configuration_nr: given period is inconsistent with current tdd configuration, nrofDownlinkSlots %d, nrofUplinkSlots %d, nb_slots_per_period %d \n",
                nrofDownlinkSlots,nrofUplinkSlots,nb_slots_per_period);
  else {
    AssertFatal(nrofDownlinkSymbols + nrofUplinkSymbols < 14,"illegal symbol configuration DL %d, UL %d\n",nrofDownlinkSymbols,nrofUplinkSymbols);
    AssertFatal(nb_slots_per_period == (nrofDownlinkSlots + nrofUplinkSlots + 1),
                "set_tdd_configuration_nr: given period is inconsistent with current tdd configuration, nrofDownlinkSlots %d, nrofUplinkSlots %d, nrofMixed slots 1, nb_slots_per_period %d \n",
                nrofDownlinkSlots,nrofUplinkSlots,nb_slots_per_period);
  }

  tdd_table->max_tdd_periodicity_list = (fapi_nr_max_tdd_periodicity_t *) malloc(nb_slots_per_period * sizeof(fapi_nr_max_tdd_periodicity_t));

  for(int memory_alloc = 0 ; memory_alloc < nb_slots_per_period; memory_alloc++)
    tdd_table->max_tdd_periodicity_list[memory_alloc].max_num_of_symbol_per_slot_list =
      (fapi_nr_max_num_of_symbol_per_slot_t *) malloc(NR_NUMBER_OF_SYMBOLS_PER_SLOT*sizeof(fapi_nr_max_num_of_symbol_per_slot_t));

  int slot_number = 0;
  while(slot_number != nb_slots_per_period) {
    if(nrofDownlinkSlots != 0) {
      for (int number_of_symbol = 0; number_of_symbol < nrofDownlinkSlots * NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
        tdd_table->max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol % NR_NUMBER_OF_SYMBOLS_PER_SLOT].slot_config = 0;
        if((number_of_symbol + 1) % NR_NUMBER_OF_SYMBOLS_PER_SLOT == 0)
          slot_number++;
      }
    }

    if (nrofDownlinkSymbols != 0 || nrofUplinkSymbols != 0) {
      for(int number_of_symbol = 0; number_of_symbol < nrofDownlinkSymbols; number_of_symbol++) {
        tdd_table->max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol].slot_config = 0;
      }
      for(int number_of_symbol = nrofDownlinkSymbols; number_of_symbol < NR_NUMBER_OF_SYMBOLS_PER_SLOT - nrofUplinkSymbols; number_of_symbol++) {
        tdd_table->max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol].slot_config = 2;
      }
      for(int number_of_symbol = NR_NUMBER_OF_SYMBOLS_PER_SLOT - nrofUplinkSymbols; number_of_symbol < NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
        tdd_table->max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol].slot_config = 1;
      }
      slot_number++;
    }

    if(nrofUplinkSlots != 0) {
      for (int number_of_symbol = 0; number_of_symbol < nrofUplinkSlots * NR_NUMBER_OF_SYMBOLS_PER_SLOT; number_of_symbol++) {
        tdd_table->max_tdd_periodicity_list[slot_number].max_num_of_symbol_per_slot_list[number_of_symbol%NR_NUMBER_OF_SYMBOLS_PER_SLOT].slot_config = 1;
        if((number_of_symbol + 1) % NR_NUMBER_OF_SYMBOLS_PER_SLOT == 0)
          slot_number++;
      }
    }
  }
}

void config_common_ue_sa(NR_UE_MAC_INST_t *mac,
                         NR_ServingCellConfigCommonSIB_t *scc,
		         module_id_t module_id,
		         int cc_idP)
{
  fapi_nr_config_request_t *cfg = &mac->phy_config.config_req;
  mac->phy_config.Mod_id = module_id;
  mac->phy_config.CC_id = cc_idP;

  LOG_D(MAC, "Entering SA UE Config Common\n");

  // carrier config
  NR_FrequencyInfoDL_SIB_t *frequencyInfoDL = &scc->downlinkConfigCommon.frequencyInfoDL;
  AssertFatal(frequencyInfoDL->frequencyBandList.list.array[0]->freqBandIndicatorNR,
              "Field mandatory present for DL in SIB1\n");
  mac->nr_band = *frequencyInfoDL->frequencyBandList.list.array[0]->freqBandIndicatorNR;
  int bw_index = get_supported_band_index(frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                          mac->nr_band,
                                          frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
  cfg->carrier_config.dl_bandwidth = get_supported_bw_mhz(mac->frequency_range, bw_index);

  uint64_t dl_bw_khz = (12 * frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth) *
                       (15 << frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing);
  cfg->carrier_config.dl_frequency = (downlink_frequency[cc_idP][0]/1000) - (dl_bw_khz>>1);

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

  NR_FrequencyInfoUL_SIB_t *frequencyInfoUL = &scc->uplinkConfigCommon->frequencyInfoUL;
  mac->p_Max = frequencyInfoUL->p_Max ?
               *frequencyInfoUL->p_Max :
               INT_MIN;
  bw_index = get_supported_band_index(frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                      mac->nr_band,
                                      frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
  cfg->carrier_config.uplink_bandwidth = get_supported_bw_mhz(mac->frequency_range, bw_index);

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
    set_tdd_config_nr_ue(&cfg->tdd_table_1,
                         cfg->ssb_config.scs_common,
                         &mac->tdd_UL_DL_ConfigurationCommon->pattern1);
    if (mac->tdd_UL_DL_ConfigurationCommon->pattern2) {
      cfg->tdd_table_2 = (fapi_nr_tdd_table_t *) malloc(sizeof(fapi_nr_tdd_table_t));
      set_tdd_config_nr_ue(cfg->tdd_table_2,
                           cfg->ssb_config.scs_common,
                           mac->tdd_UL_DL_ConfigurationCommon->pattern2);
    }
  }

  // PRACH configuration

  uint8_t nb_preambles = 64;
  NR_RACH_ConfigCommon_t *rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup;
  if(rach_ConfigCommon->totalNumberOfRA_Preambles != NULL)
     nb_preambles = *rach_ConfigCommon->totalNumberOfRA_Preambles;

  cfg->prach_config.prach_sequence_length = rach_ConfigCommon->prach_RootSequenceIndex.present-1;

  if (rach_ConfigCommon->msg1_SubcarrierSpacing)
    cfg->prach_config.prach_sub_c_spacing = *rach_ConfigCommon->msg1_SubcarrierSpacing;
  else {
    // If absent, the UE applies the SCS as derived from the prach-ConfigurationIndex (for 839)
    int config_index = rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;
    const int64_t *prach_config_info_p = get_prach_config_info(mac->frequency_range, config_index, mac->frame_type);
    int format = prach_config_info_p[0];
    cfg->prach_config.prach_sub_c_spacing = format == 3 ? 5 : 4;
  }

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
                      NR_ServingCellConfigCommon_t *scc,
		      module_id_t module_id,
		      int cc_idP)
{
  fapi_nr_config_request_t *cfg = &mac->phy_config.config_req;

  mac->phy_config.Mod_id = module_id;
  mac->phy_config.CC_id = cc_idP;
  
  // carrier config
  LOG_D(MAC, "Entering UE Config Common\n");

  AssertFatal(scc->downlinkConfigCommon,
              "Not expecting downlinkConfigCommon to be NULL here\n");

  NR_FrequencyInfoDL_t *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
  if (frequencyInfoDL) { // NeedM for inter-freq handover
    mac->nr_band = *frequencyInfoDL->frequencyBandList.list.array[0];
    mac->frame_type = get_frame_type(mac->nr_band, get_softmodem_params()->numerology);
    mac->frequency_range = mac->nr_band < 256 ? FR1 : FR2;

    int bw_index = get_supported_band_index(frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                            mac->nr_band,
                                            frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
    cfg->carrier_config.dl_bandwidth = get_supported_bw_mhz(mac->frequency_range, bw_index);
    
    cfg->carrier_config.dl_frequency = from_nrarfcn(mac->nr_band,
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
  }

  if (scc->uplinkConfigCommon && scc->uplinkConfigCommon->frequencyInfoUL) {
    NR_FrequencyInfoUL_t *frequencyInfoUL = scc->uplinkConfigCommon->frequencyInfoUL;
    mac->p_Max = frequencyInfoUL->p_Max ?
                 *frequencyInfoUL->p_Max :
                 INT_MIN;

    int bw_index = get_supported_band_index(frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                            *frequencyInfoUL->frequencyBandList->list.array[0],
                                            frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
    cfg->carrier_config.uplink_bandwidth = get_supported_bw_mhz(mac->frequency_range, bw_index);

    long *UL_pointA = NULL;
    if (frequencyInfoUL->absoluteFrequencyPointA)
      UL_pointA = frequencyInfoUL->absoluteFrequencyPointA;
    else if (frequencyInfoDL)
      UL_pointA = &frequencyInfoDL->absoluteFrequencyPointA;

    if(UL_pointA)
      cfg->carrier_config.uplink_frequency = from_nrarfcn(*frequencyInfoUL->frequencyBandList->list.array[0],
                                                          *scc->ssbSubcarrierSpacing,
                                                          *UL_pointA) / 1000; // freq in kHz

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
  }

  // cell config
  cfg->cell_config.phy_cell_id = *scc->physCellId;
  cfg->cell_config.frame_duplex_type = mac->frame_type;

  // SSB config
  cfg->ssb_config.ss_pbch_power = scc->ss_PBCH_BlockPower;
  cfg->ssb_config.scs_common = *scc->ssbSubcarrierSpacing;

  // SSB Table config
  if (frequencyInfoDL && frequencyInfoDL->absoluteFrequencySSB) {
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
  }

  switch (scc->ssb_PositionsInBurst->present) {
  case 1 :
    cfg->ssb_table.ssb_mask_list[0].ssb_mask = scc->ssb_PositionsInBurst->choice.shortBitmap.buf[0] << 24;
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
      cfg->ssb_table.ssb_mask_list[0].ssb_mask += (uint32_t) scc->ssb_PositionsInBurst->choice.longBitmap.buf[3 - i] << i * 8;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask += (uint32_t) scc->ssb_PositionsInBurst->choice.longBitmap.buf[7 - i] << i * 8;
    }
    break;
  default:
    AssertFatal(1==0,"SSB bitmap size value %d undefined (allowed values 1,2,3) \n", scc->ssb_PositionsInBurst->present);
  }

  // TDD Table Configuration
  if (cfg->cell_config.frame_duplex_type == TDD){
    set_tdd_config_nr_ue(&cfg->tdd_table_1,
                         cfg->ssb_config.scs_common,
                         &mac->tdd_UL_DL_ConfigurationCommon->pattern1);
    if (mac->tdd_UL_DL_ConfigurationCommon->pattern2) {
      cfg->tdd_table_2 = (fapi_nr_tdd_table_t *) malloc(sizeof(fapi_nr_tdd_table_t));
      set_tdd_config_nr_ue(cfg->tdd_table_2,
                           cfg->ssb_config.scs_common,
                           mac->tdd_UL_DL_ConfigurationCommon->pattern2);
    }
  }

  // PRACH configuration
  uint8_t nb_preambles = 64;
  if (scc->uplinkConfigCommon &&
      scc->uplinkConfigCommon->initialUplinkBWP &&
      scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon) { // all NeedM

    NR_RACH_ConfigCommon_t *rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
    if(rach_ConfigCommon->totalNumberOfRA_Preambles != NULL)
      nb_preambles = *rach_ConfigCommon->totalNumberOfRA_Preambles;

    cfg->prach_config.prach_sequence_length = rach_ConfigCommon->prach_RootSequenceIndex.present-1;

    if (rach_ConfigCommon->msg1_SubcarrierSpacing)
      cfg->prach_config.prach_sub_c_spacing = *rach_ConfigCommon->msg1_SubcarrierSpacing;
    else {
      // If absent, the UE applies the SCS as derived from the prach-ConfigurationIndex (for 839)
      int config_index = rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;
      const int64_t *prach_config_info_p = get_prach_config_info(mac->frequency_range, config_index, mac->frame_type);
      int format = prach_config_info_p[0];
      cfg->prach_config.prach_sub_c_spacing = format == 3 ? 5 : 4;
    }

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
                                                                  mac->frame_type,
                                                                  mac->frequency_range);

      cfg->prach_config.ssb_per_rach = rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present-1;
      //prach_fd_occasion->num_unused_root_sequences = ???
    }
  }
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

static int lcid_cmp(const void *lc1, const void *lc2, void *mac_inst)
{
  uint8_t id1 = ((nr_lcordered_info_t *)lc1)->lcids_ordered;
  uint8_t id2 = ((nr_lcordered_info_t *)lc2)->lcids_ordered;
  NR_UE_MAC_INST_t *mac = (NR_UE_MAC_INST_t *)mac_inst;

  NR_LogicalChannelConfig_t **lc_config = &mac->logicalChannelConfig[0];

  AssertFatal(id1 > 0 && id2 > 0, "undefined logical channel identity\n");
  AssertFatal(lc_config[id1 - 1] != NULL || lc_config[id2 - 1] != NULL, "logical channel configuration should be available\n");

  return (lc_config[id1 - 1]->ul_SpecificParameters->priority - lc_config[id2 - 1]->ul_SpecificParameters->priority);
}

void nr_release_mac_config_logicalChannelBearer(module_id_t module_id, long channel_identity)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  if (mac->logicalChannelConfig[channel_identity - 1] != NULL) {
    mac->logicalChannelConfig[channel_identity - 1] = NULL;
    memset(&mac->scheduling_info.lc_sched_info[channel_identity - 1], 0, sizeof(NR_LC_SCHEDULING_INFO));
  } else {
    LOG_E(NR_MAC, "Trying to release a non configured logical channel bearer %li\n", channel_identity);
  }
}

static uint16_t nr_get_ms_bucketsizeduration(uint8_t bucketsizeduration)
{
  switch (bucketsizeduration) {
    case NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50:
      return 50;

    case NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms100:
      return 100;

    case NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms150:
      return 150;

    case NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms300:
      return 300;

    case NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms500:
      return 500;

    case NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms1000:
      return 1000;

    default:
      return 0;
  }
}

void nr_configure_mac_config_logicalChannelBearer(module_id_t module_id,
                                                  long channel_identity,
                                                  NR_LogicalChannelConfig_t *lc_config)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

  LOG_I(NR_MAC, "[MACLogicalChannelConfig]Applying RRC Logical Channel Config %d to lcid %li\n", module_id, channel_identity);
  mac->logicalChannelConfig[channel_identity - 1] = lc_config;

  // initialize the variable Bj for every LCID
  mac->scheduling_info.lc_sched_info[channel_identity - 1].Bj = 0;

  // store the bucket size
  int pbr = nr_get_pbr(lc_config->ul_SpecificParameters->prioritisedBitRate);
  int bsd = nr_get_ms_bucketsizeduration(lc_config->ul_SpecificParameters->bucketSizeDuration);

  // in infinite pbr, the bucket is saturated by pbr
  if (lc_config->ul_SpecificParameters->prioritisedBitRate
      == NR_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity) {
    bsd = 1;
  }
  mac->scheduling_info.lc_sched_info[channel_identity - 1].bucket_size = pbr * bsd;

  if (lc_config->ul_SpecificParameters->logicalChannelGroup != NULL)
    mac->scheduling_info.lc_sched_info[channel_identity - 1].LCGID = *lc_config->ul_SpecificParameters->logicalChannelGroup;
  else
    mac->scheduling_info.lc_sched_info[channel_identity - 1].LCGID = 0;
}

void nr_rrc_mac_config_req_ue_logicalChannelBearer(module_id_t module_id,
                                                   struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_toadd_list,
                                                   struct NR_CellGroupConfig__rlc_BearerToReleaseList *rlc_torelease_list)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  if (rlc_torelease_list) {
    for (int i = 0; i < rlc_torelease_list->list.count; i++) {
      if (rlc_torelease_list->list.array[i]) {
        int lc_identity = *rlc_torelease_list->list.array[i];
        nr_release_mac_config_logicalChannelBearer(module_id, lc_identity);
      }
    }
  }
  if (rlc_toadd_list) {
    for (int i = 0; i < rlc_toadd_list->list.count; i++) {
      NR_RLC_BearerConfig_t *rlc_bearer = rlc_toadd_list->list.array[i];
      int lc_identity = rlc_bearer->logicalChannelIdentity;
      mac->lc_ordered_info[i].lcids_ordered = lc_identity;
      NR_LogicalChannelConfig_t *mac_lc_config;
      if (mac->logicalChannelConfig[lc_identity - 1] == NULL) {
        /* setup of new LCID*/
        LOG_D(NR_MAC, "Establishing the logical channel %d\n", lc_identity);
        AssertFatal(rlc_bearer->servedRadioBearer, "servedRadioBearer should be present for LCID establishment\n");
        if (rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity) { /* SRB */
          NR_SRB_Identity_t srb_id = rlc_bearer->servedRadioBearer->choice.srb_Identity;
          if (rlc_bearer->mac_LogicalChannelConfig != NULL) {
            mac_lc_config = rlc_bearer->mac_LogicalChannelConfig;
          } else {
            LOG_I(NR_RRC, "Applying the default logicalChannelConfig for SRB\n");
            if (srb_id == 1)
              mac_lc_config = (NR_LogicalChannelConfig_t *)&NR_SRB1_logicalChannelConfig_defaultValue;
            else if (srb_id == 2)
              mac_lc_config = (NR_LogicalChannelConfig_t *)&NR_SRB2_logicalChannelConfig_defaultValue;
            else if (srb_id == 3)
              mac_lc_config = (NR_LogicalChannelConfig_t *)&NR_SRB3_logicalChannelConfig_defaultValue;
            else
              AssertFatal(1 == 0, "The logical id %d is not a valid SRB id %li\n", lc_identity, srb_id);
          }
        } else { /* DRB */
          mac_lc_config = rlc_bearer->mac_LogicalChannelConfig;
          AssertFatal(mac_lc_config != NULL, "For DRB, it should be mandatorily present\n");
        }
      } else {
        /* LC is already established, reconfiguring the LC */
        LOG_D(NR_MAC, "Logical channel %d is already established, Reconfiguring now\n", lc_identity);
        if (rlc_bearer->mac_LogicalChannelConfig != NULL) {
          mac_lc_config = rlc_bearer->mac_LogicalChannelConfig;
        } else {
          /* Need M - Maintains current value */
          continue;
        }
      }
      mac->lc_ordered_info[i].logicalChannelConfig_ordered = mac_lc_config;
      nr_configure_mac_config_logicalChannelBearer(module_id, lc_identity, mac_lc_config);
    }

    // reorder the logical channels as per its priority
    qsort_r(mac->lc_ordered_info, rlc_toadd_list->list.count, sizeof(nr_lcordered_info_t), lcid_cmp, mac);
  }
}

void configure_current_BWP(NR_UE_MAC_INST_t *mac,
                           NR_ServingCellConfigCommonSIB_t *scc,
                           const NR_ServingCellConfig_t *spCellConfigDedicated)
{
  NR_UE_DL_BWP_t *DL_BWP = &mac->current_DL_BWP;
  NR_UE_UL_BWP_t *UL_BWP = &mac->current_UL_BWP;
  NR_BWP_t dl_genericParameters = {0};
  NR_BWP_t ul_genericParameters = {0};

  if(scc) {
    DL_BWP->bwp_id = 0;
    UL_BWP->bwp_id = 0;
    mac->bwp_dlcommon = &scc->downlinkConfigCommon.initialDownlinkBWP;
    mac->bwp_ulcommon = &scc->uplinkConfigCommon->initialUplinkBWP;
    dl_genericParameters = mac->bwp_dlcommon->genericParameters;
    if(mac->bwp_ulcommon)
      ul_genericParameters = mac->bwp_ulcommon->genericParameters;
    else
      ul_genericParameters = mac->bwp_dlcommon->genericParameters;

    if (mac->bwp_dlcommon->pdsch_ConfigCommon)
      DL_BWP->tdaList_Common = mac->bwp_dlcommon->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    if (mac->bwp_ulcommon->pusch_ConfigCommon) {
      UL_BWP->tdaList_Common = mac->bwp_ulcommon->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
      UL_BWP->msg3_DeltaPreamble = mac->bwp_ulcommon->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble;
    }
    if (mac->bwp_ulcommon->pucch_ConfigCommon)
      UL_BWP->pucch_ConfigCommon = mac->bwp_ulcommon->pucch_ConfigCommon->choice.setup;
    if (mac->bwp_ulcommon->rach_ConfigCommon) {
      UL_BWP->rach_ConfigCommon = mac->bwp_ulcommon->rach_ConfigCommon->choice.setup;
      // Setup the SSB to Rach Occasions mapping according to the config
      build_ssb_to_ro_map(mac);
    }
    if (mac->bwp_dlcommon->pdcch_ConfigCommon)
      configure_ss_coreset(mac, mac->bwp_dlcommon->pdcch_ConfigCommon->choice.setup, NULL);
  }

  if(spCellConfigDedicated) {
    if (spCellConfigDedicated->firstActiveDownlinkBWP_Id)
      DL_BWP->bwp_id = *spCellConfigDedicated->firstActiveDownlinkBWP_Id;
    if (spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id)
      UL_BWP->bwp_id = *spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id;

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
      dl_genericParameters = mac->bwp_dlcommon->genericParameters;
      DL_BWP->pdsch_Config = spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup;
      DL_BWP->tdaList_Common = mac->bwp_dlcommon->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
      configure_ss_coreset(mac,
                           mac->bwp_dlcommon->pdcch_ConfigCommon ? mac->bwp_dlcommon->pdcch_ConfigCommon->choice.setup : NULL,
                           spCellConfigDedicated->initialDownlinkBWP->pdcch_Config ? spCellConfigDedicated->initialDownlinkBWP->pdcch_Config->choice.setup : NULL);
    }

    UL_BWP->msg3_DeltaPreamble = mac->bwp_ulcommon->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble;

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
      if (bwp_uplink->bwp_Common->rach_ConfigCommon) {
        UL_BWP->rach_ConfigCommon = bwp_uplink->bwp_Common->rach_ConfigCommon->choice.setup;
        // Setup the SSB to Rach Occasions mapping according to the config
        build_ssb_to_ro_map(mac);
      }
    }
    else {
      UL_BWP->tdaList_Common = mac->bwp_ulcommon->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
      UL_BWP->pusch_Config = spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pusch_Config->choice.setup;
      UL_BWP->pucch_Config = spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup;
      UL_BWP->srs_Config = spCellConfigDedicated->uplinkConfig->initialUplinkBWP->srs_Config->choice.setup;
      UL_BWP->configuredGrantConfig =
          spCellConfigDedicated->uplinkConfig->initialUplinkBWP->configuredGrantConfig ? spCellConfigDedicated->uplinkConfig->initialUplinkBWP->configuredGrantConfig->choice.setup : NULL;
      ul_genericParameters = mac->bwp_ulcommon->genericParameters;
      if (mac->bwp_ulcommon->pucch_ConfigCommon)
        UL_BWP->pucch_ConfigCommon = mac->bwp_ulcommon->pucch_ConfigCommon->choice.setup;
      if (mac->bwp_ulcommon->rach_ConfigCommon)
        UL_BWP->rach_ConfigCommon = mac->bwp_ulcommon->rach_ConfigCommon->choice.setup;
    }
  }

  DL_BWP->scs = dl_genericParameters.subcarrierSpacing;
  DL_BWP->cyclicprefix = dl_genericParameters.cyclicPrefix;
  DL_BWP->BWPSize = NRRIV2BW(dl_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  DL_BWP->BWPStart = NRRIV2PRBOFFSET(dl_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  UL_BWP->scs = ul_genericParameters.subcarrierSpacing;
  UL_BWP->cyclicprefix = ul_genericParameters.cyclicPrefix;
  UL_BWP->BWPSize = NRRIV2BW(ul_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  UL_BWP->BWPStart = NRRIV2PRBOFFSET(ul_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  DL_BWP->initial_BWPSize = NRRIV2BW(mac->bwp_dlcommon->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  UL_BWP->initial_BWPSize = NRRIV2BW(mac->bwp_ulcommon->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  DL_BWP->initial_BWPStart = NRRIV2PRBOFFSET(mac->bwp_dlcommon->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  UL_BWP->initial_BWPStart = NRRIV2PRBOFFSET(mac->bwp_ulcommon->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  DL_BWP->bw_tbslbrm = get_dlbw_tbslbrm(DL_BWP->initial_BWPSize, spCellConfigDedicated);
  UL_BWP->bw_tbslbrm = get_ulbw_tbslbrm(UL_BWP->initial_BWPSize, spCellConfigDedicated);
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
                               int sched_sib)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  AssertFatal(mib, "MIB should not be NULL\n");
  // initialize dl and ul config_request upon first reception of MIB
  mac->mib = mib;    //  update by every reception
  mac->phy_config.Mod_id = module_id;
  mac->phy_config.CC_id = cc_idP;
  if (sched_sib == 1)
    mac->get_sib1 = true;
  else if (sched_sib == 2)
    mac->get_otherSI = true;
  nr_ue_decode_mib(module_id,
                   cc_idP);
}

void nr_rrc_mac_config_req_sib1(module_id_t module_id,
                                int cc_idP,
                                NR_SI_SchedulingInfo_t *si_SchedulingInfo,
                                NR_ServingCellConfigCommonSIB_t *scc)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  AssertFatal(scc, "SIB1 SCC should not be NULL\n");

  updateMACie(mac->tdd_UL_DL_ConfigurationCommon,
              scc->tdd_UL_DL_ConfigurationCommon,
              NR_TDD_UL_DL_ConfigCommon_t);
  updateMACie(mac->si_SchedulingInfo,
              si_SchedulingInfo,
              NR_SI_SchedulingInfo_t);

  config_common_ue_sa(mac, scc, module_id, cc_idP);
  // configure BWP only if it is a SIB1 detection in non connected state (after sync)
  // not if it is a periodical update of SIB1 (no change of BWP in that case)
  if(mac->state < UE_CONNECTED)
    configure_current_BWP(mac, scc, NULL);

  if (!get_softmodem_params()->emulate_l1)
    mac->if_module->phy_config_request(&mac->phy_config);
  mac->phy_config_request_sent = true;
}

void handle_reconfiguration_with_sync(NR_UE_MAC_INST_t *mac,
                                      module_id_t module_id,
                                      int cc_idP,
                                      const NR_ReconfigurationWithSync_t *reconfigurationWithSync)
{
  mac->crnti = reconfigurationWithSync->newUE_Identity;
  LOG_I(NR_MAC, "Configuring CRNTI %x\n", mac->crnti);

  RA_config_t *ra = &mac->ra;
  if (reconfigurationWithSync->rach_ConfigDedicated) {
    AssertFatal(reconfigurationWithSync->rach_ConfigDedicated->present ==
                NR_ReconfigurationWithSync__rach_ConfigDedicated_PR_uplink,
                "RACH on supplementaryUplink not supported\n");
    ra->rach_ConfigDedicated = reconfigurationWithSync->rach_ConfigDedicated->choice.uplink;
  }

  if (reconfigurationWithSync->spCellConfigCommon) {
    NR_ServingCellConfigCommon_t *scc = reconfigurationWithSync->spCellConfigCommon;
    mac->bwp_dlcommon = scc->downlinkConfigCommon->initialDownlinkBWP;
    mac->bwp_ulcommon = scc->uplinkConfigCommon->initialUplinkBWP;
    if (scc->physCellId)
      mac->physCellId = *scc->physCellId;
    mac->dmrs_TypeA_Position = scc->dmrs_TypeA_Position;
    updateMACie(mac->tdd_UL_DL_ConfigurationCommon,
                scc->tdd_UL_DL_ConfigurationCommon,
                NR_TDD_UL_DL_ConfigCommon_t);
    config_common_ue(mac, scc, module_id, cc_idP);
  }

  mac->state = UE_NOT_SYNC;
  ra->ra_state = RA_UE_IDLE;
  nr_ue_mac_default_configs(mac);

  if (!get_softmodem_params()->emulate_l1) {
    mac->synch_request.Mod_id = module_id;
    mac->synch_request.CC_id = cc_idP;
    mac->synch_request.synch_req.target_Nid_cell = mac->physCellId;
    mac->if_module->synch_request(&mac->synch_request);
    mac->if_module->phy_config_request(&mac->phy_config);
    mac->phy_config_request_sent = true;
  }
}

void configure_physicalcellgroup(NR_UE_MAC_INST_t *mac,
                                 const NR_PhysicalCellGroupConfig_t *phyConfig)
{
  mac->pdsch_HARQ_ACK_Codebook = phyConfig->pdsch_HARQ_ACK_Codebook;
  mac->harq_ACK_SpatialBundlingPUCCH = phyConfig->harq_ACK_SpatialBundlingPUCCH ? true : false;
  mac->harq_ACK_SpatialBundlingPUSCH = phyConfig->harq_ACK_SpatialBundlingPUSCH ? true : false;

  NR_P_Max_t *p_NR_FR1 = phyConfig->p_NR_FR1;
  NR_P_Max_t *p_UE_FR1 = phyConfig->ext1 ?
                         phyConfig->ext1->p_UE_FR1 :
                         NULL;
  if (p_NR_FR1 == NULL)
    mac->p_Max_alt = p_UE_FR1 == NULL ? INT_MIN : *p_UE_FR1;
  else
    mac->p_Max_alt = p_UE_FR1 == NULL ? *p_NR_FR1 :
                                        (*p_UE_FR1 < *p_NR_FR1 ?
                                        *p_UE_FR1 : *p_NR_FR1);
}

void configure_maccellgroup(NR_UE_MAC_INST_t *mac, const NR_MAC_CellGroupConfig_t *mcg)
{
  NR_UE_SCHEDULING_INFO *si = &mac->scheduling_info;
  if (mcg->drx_Config)
    LOG_E(NR_MAC, "DRX not implemented! Configuration not handled!\n");
  if (mcg->schedulingRequestConfig) {
    const NR_SchedulingRequestConfig_t *src = mcg->schedulingRequestConfig;
    if (src->schedulingRequestToReleaseList) {
      for (int i = 0; i < src->schedulingRequestToReleaseList->list.count; i++) {
        if (*src->schedulingRequestToReleaseList->list.array[i] == si->sr_id) {
          si->SR_COUNTER = 0;
          si->sr_ProhibitTimer = 0;
          si->sr_ProhibitTimer_Running = 0;
          si->sr_id = -1; // invalid init value
        }
        else
          LOG_E(NR_MAC, "Cannot release SchedulingRequestConfig. Not configured.\n");
      }
    }
    if (src->schedulingRequestToAddModList) {
      for (int i = 0; i < src->schedulingRequestToAddModList->list.count; i++) {
        NR_SchedulingRequestToAddMod_t *sr = src->schedulingRequestToAddModList->list.array[i];
        AssertFatal(si->sr_id == -1 ||
                    si->sr_id == sr->schedulingRequestId,
                    "Current implementation cannot handle more than 1 SR configuration\n");
        si->sr_id = sr->schedulingRequestId;
        si->sr_TransMax = sr->sr_TransMax;
        if (sr->sr_ProhibitTimer)
          LOG_E(NR_MAC, "SR prohibit timer not properly implemented\n");
      }
    }
  }
  if (mcg->bsr_Config) {
    si->periodicBSR_Timer = mcg->bsr_Config->periodicBSR_Timer;
    si->retxBSR_Timer = mcg->bsr_Config->retxBSR_Timer;
    if (mcg->bsr_Config->logicalChannelSR_DelayTimer)
      LOG_E(NR_MAC, "Handling of logicalChannelSR_DelayTimer not implemented\n");
  }
  if (mcg->tag_Config) {
    // TODO TAG not handled
    if(mcg->tag_Config->tag_ToAddModList) {
      for (int i = 0; i < mcg->tag_Config->tag_ToAddModList->list.count; i++) {
        if (mcg->tag_Config->tag_ToAddModList->list.array[i]->timeAlignmentTimer !=
            NR_TimeAlignmentTimer_infinity)
          LOG_E(NR_MAC, "TimeAlignmentTimer not handled\n");
      }
    }
  }
  if (mcg->phr_Config) {
    // TODO configuration when PHR is implemented
  }
}

static void configure_csiconfig(NR_UE_ServingCell_Info_t *sc_info,
                                struct NR_SetupRelease_CSI_MeasConfig *csi_MeasConfig_sr)
{
  switch (csi_MeasConfig_sr->present) {
    case NR_SetupRelease_CSI_MeasConfig_PR_NOTHING :
      break;
    case NR_SetupRelease_CSI_MeasConfig_PR_release :
      ASN_STRUCT_FREE(asn_DEF_NR_CSI_MeasConfig,
                      sc_info->csi_MeasConfig);
      sc_info->csi_MeasConfig = NULL;
      break;
    case NR_SetupRelease_CSI_MeasConfig_PR_setup :
      if (!sc_info->csi_MeasConfig) {  // setup
        updateMACie(sc_info->csi_MeasConfig,
                    csi_MeasConfig_sr->choice.setup,
                    NR_CSI_MeasConfig_t);
      }
      else { // modification
        NR_CSI_MeasConfig_t *csi_MeasConfig = csi_MeasConfig_sr->choice.setup;
        if (csi_MeasConfig->reportTriggerSize)
          updateMACie(sc_info->csi_MeasConfig->reportTriggerSize,
                      csi_MeasConfig->reportTriggerSize,
                      long);
        if (csi_MeasConfig->aperiodicTriggerStateList)
          handleMACsetuprelease(sc_info->aperiodicTriggerStateList,
                                csi_MeasConfig->aperiodicTriggerStateList,
                                NR_CSI_AperiodicTriggerStateList_t,
                                asn_DEF_NR_CSI_AperiodicTriggerStateList);
        if (csi_MeasConfig->semiPersistentOnPUSCH_TriggerStateList)
          handleMACsetuprelease(sc_info->csi_MeasConfig->semiPersistentOnPUSCH_TriggerStateList->choice.setup,
                                csi_MeasConfig->semiPersistentOnPUSCH_TriggerStateList,
                                NR_CSI_SemiPersistentOnPUSCH_TriggerStateList_t,
                                asn_DEF_NR_CSI_SemiPersistentOnPUSCH_TriggerStateList);
        // NZP-CSI-RS-Resources
        if (csi_MeasConfig->nzp_CSI_RS_ResourceToReleaseList) {
          for (int i = 0; i < csi_MeasConfig->nzp_CSI_RS_ResourceToReleaseList->list.count; i++) {
            long id = *csi_MeasConfig->nzp_CSI_RS_ResourceToReleaseList->list.array[i];
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.array[j]->nzp_CSI_RS_ResourceId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.count)
              asn_sequence_del(&sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list, j, 1);
            else
              LOG_E(NR_MAC, "Element not present in the list, impossible to release\n");
          }
        }
        if (csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList) {
          for (int i = 0; i < csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.count; i++) {
            NR_NZP_CSI_RS_Resource_t *res = csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.array[i];
            long id = res->nzp_CSI_RS_ResourceId;
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.array[j]->nzp_CSI_RS_ResourceId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.count) { // modify
              NR_NZP_CSI_RS_Resource_t *mac_res = sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.array[j];
              mac_res->resourceMapping = res->resourceMapping;
              mac_res->powerControlOffset = res->powerControlOffset;
              updateMACie(mac_res->powerControlOffsetSS,
                          res->powerControlOffsetSS,
                          long);
              mac_res->scramblingID = res->scramblingID;
              if (res->periodicityAndOffset)
                updateMACie(mac_res->periodicityAndOffset,
                            res->periodicityAndOffset,
                            NR_CSI_ResourcePeriodicityAndOffset_t);
              if (res->qcl_InfoPeriodicCSI_RS)
                updateMACie(mac_res->qcl_InfoPeriodicCSI_RS,
                            res->qcl_InfoPeriodicCSI_RS,
                            NR_TCI_StateId_t);
            }
            else { // add
              asn1cSequenceAdd(sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list,
                               NR_NZP_CSI_RS_Resource_t,
                               nzp_csi_rs);
              updateMACie(nzp_csi_rs, res, NR_NZP_CSI_RS_Resource_t);
            }
          }
        }
        // NZP-CSI-RS-ResourceSets
        if (csi_MeasConfig->nzp_CSI_RS_ResourceSetToReleaseList) {
          for (int i = 0; i < csi_MeasConfig->nzp_CSI_RS_ResourceSetToReleaseList->list.count; i++) {
            long id = *csi_MeasConfig->nzp_CSI_RS_ResourceSetToReleaseList->list.array[i];
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[j]->nzp_CSI_ResourceSetId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.count)
              asn_sequence_del(&sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list, j, 1);
            else
              LOG_E(NR_MAC, "Element not present in the list, impossible to release\n");
          }
        }
        if (csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList) {
          for (int i = 0; i < csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.count; i++) {
            NR_NZP_CSI_RS_ResourceSet_t *res = csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[i];
            long id = res->nzp_CSI_ResourceSetId;
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[j]->nzp_CSI_ResourceSetId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.count) { // modify
              NR_NZP_CSI_RS_ResourceSet_t *mac_res = sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[j];
              updateMACie(mac_res, res, NR_NZP_CSI_RS_ResourceSet_t);
            }
            else { // add
              asn1cSequenceAdd(sc_info->csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list,
                               NR_NZP_CSI_RS_ResourceSet_t,
                               nzp_csi_rsset);
              updateMACie(nzp_csi_rsset, res, NR_NZP_CSI_RS_ResourceSet_t);
            }
          }
        }
        // CSI-IM-Resource
        if (csi_MeasConfig->csi_IM_ResourceToReleaseList) {
          for (int i = 0; i < csi_MeasConfig->csi_IM_ResourceToReleaseList->list.count; i++) {
            long id = *csi_MeasConfig->csi_IM_ResourceToReleaseList->list.array[i];
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->csi_IM_ResourceToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->csi_IM_ResourceToAddModList->list.array[j]->csi_IM_ResourceId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->csi_IM_ResourceToAddModList->list.count)
              asn_sequence_del(&sc_info->csi_MeasConfig->csi_IM_ResourceToAddModList->list, j, 1);
            else
              LOG_E(NR_MAC, "Element not present in the list, impossible to release\n");
          }
        }
        if (csi_MeasConfig->csi_IM_ResourceToAddModList) {
          for (int i = 0; i < csi_MeasConfig->csi_IM_ResourceToAddModList->list.count; i++) {
            NR_CSI_IM_Resource_t *res = csi_MeasConfig->csi_IM_ResourceToAddModList->list.array[i];
            long id = res->csi_IM_ResourceId;
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->csi_IM_ResourceToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->csi_IM_ResourceToAddModList->list.array[j]->csi_IM_ResourceId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->csi_IM_ResourceToAddModList->list.count) { // modify
              NR_CSI_IM_Resource_t *mac_res = sc_info->csi_MeasConfig->csi_IM_ResourceToAddModList->list.array[j];
              if (res->csi_IM_ResourceElementPattern)
                updateMACie(mac_res->csi_IM_ResourceElementPattern,
                            res->csi_IM_ResourceElementPattern,
                            struct NR_CSI_IM_Resource__csi_IM_ResourceElementPattern);
              if (res->freqBand)
                updateMACie(mac_res->freqBand,
                            res->freqBand,
                            NR_CSI_FrequencyOccupation_t);
              if (res->periodicityAndOffset)
                updateMACie(mac_res->periodicityAndOffset,
                            res->periodicityAndOffset,
                            NR_CSI_ResourcePeriodicityAndOffset_t);
            }
            else { // add
              asn1cSequenceAdd(sc_info->csi_MeasConfig->csi_IM_ResourceToAddModList->list,
                               NR_CSI_IM_Resource_t,
                               csi_im);
              updateMACie(csi_im, res, NR_CSI_IM_Resource_t);
            }
          }
        }
        // CSI-IM-ResourceSets
        if (csi_MeasConfig->csi_IM_ResourceSetToReleaseList) {
          for (int i = 0; i < csi_MeasConfig->csi_IM_ResourceSetToReleaseList->list.count; i++) {
            long id = *csi_MeasConfig->csi_IM_ResourceSetToReleaseList->list.array[i];
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->csi_IM_ResourceSetToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->csi_IM_ResourceSetToAddModList->list.array[j]->csi_IM_ResourceSetId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->csi_IM_ResourceSetToAddModList->list.count)
              asn_sequence_del(&sc_info->csi_MeasConfig->csi_IM_ResourceSetToAddModList->list, j, 1);
            else
              LOG_E(NR_MAC, "Element not present in the list, impossible to release\n");
          }
        }
        if (csi_MeasConfig->csi_IM_ResourceSetToAddModList) {
          for (int i = 0; i < csi_MeasConfig->csi_IM_ResourceSetToAddModList->list.count; i++) {
            NR_CSI_IM_ResourceSet_t *res = csi_MeasConfig->csi_IM_ResourceSetToAddModList->list.array[i];
            long id = res->csi_IM_ResourceSetId;
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->csi_IM_ResourceSetToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->csi_IM_ResourceSetToAddModList->list.array[j]->csi_IM_ResourceSetId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->csi_IM_ResourceSetToAddModList->list.count) { // modify
              NR_CSI_IM_ResourceSet_t *mac_res = sc_info->csi_MeasConfig->csi_IM_ResourceSetToAddModList->list.array[j];
              updateMACie(mac_res, res, NR_CSI_IM_ResourceSet_t);
            }
            else { // add
              asn1cSequenceAdd(sc_info->csi_MeasConfig->csi_IM_ResourceSetToAddModList->list,
                               NR_CSI_IM_ResourceSet_t,
                               csi_im_set);
              updateMACie(csi_im_set, res, NR_CSI_IM_ResourceSet_t);
            }
          }
        }
        // CSI-SSB-ResourceSets
        if (csi_MeasConfig->csi_SSB_ResourceSetToReleaseList) {
          for (int i = 0; i < csi_MeasConfig->csi_SSB_ResourceSetToReleaseList->list.count; i++) {
            long id = *csi_MeasConfig->csi_SSB_ResourceSetToReleaseList->list.array[i];
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[j]->csi_SSB_ResourceSetId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.count)
              asn_sequence_del(&sc_info->csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list, j, 1);
            else
              LOG_E(NR_MAC, "Element not present in the list, impossible to release\n");
          }
        }
        if (csi_MeasConfig->csi_SSB_ResourceSetToAddModList) {
          for (int i = 0; i < csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.count; i++) {
            NR_CSI_SSB_ResourceSet_t *res = csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[i];
            long id = res->csi_SSB_ResourceSetId;
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[j]->csi_SSB_ResourceSetId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.count) { // modify
              NR_CSI_SSB_ResourceSet_t *mac_res = sc_info->csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[j];
              updateMACie(mac_res, res, NR_CSI_SSB_ResourceSet_t);
            }
            else { // add
              asn1cSequenceAdd(sc_info->csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list,
                               NR_CSI_SSB_ResourceSet_t,
                               csi_ssb_set);
              updateMACie(csi_ssb_set, res, NR_CSI_SSB_ResourceSet_t);
            }
          }
        }
        // CSI-ResourceConfigs
        if (csi_MeasConfig->csi_ResourceConfigToReleaseList) {
          for (int i = 0; i < csi_MeasConfig->csi_ResourceConfigToReleaseList->list.count; i++) {
            long id = *csi_MeasConfig->csi_ResourceConfigToReleaseList->list.array[i];
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->csi_ResourceConfigToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[j]->csi_ResourceConfigId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->csi_ResourceConfigToAddModList->list.count)
              asn_sequence_del(&sc_info->csi_MeasConfig->csi_ResourceConfigToAddModList->list, j, 1);
            else
              LOG_E(NR_MAC, "Element not present in the list, impossible to release\n");
          }
        }
        if (csi_MeasConfig->csi_ResourceConfigToAddModList) {
          for (int i = 0; i < csi_MeasConfig->csi_ResourceConfigToAddModList->list.count; i++) {
            NR_CSI_ResourceConfig_t *res = csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[i];
            long id = res->csi_ResourceConfigId;
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->csi_ResourceConfigToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[j]->csi_ResourceConfigId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->csi_ResourceConfigToAddModList->list.count) { // modify
              NR_CSI_ResourceConfig_t *mac_res = sc_info->csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[j];
              updateMACie(mac_res, res, NR_CSI_ResourceConfig_t);
            }
            else { // add
              asn1cSequenceAdd(sc_info->csi_MeasConfig->csi_ResourceConfigToAddModList->list,
                               NR_CSI_ResourceConfig_t,
                               csi_config);
              updateMACie(csi_config, res, NR_CSI_ResourceConfig_t);
            }
          }
        }
        // CSI-ReportConfigs
        if (csi_MeasConfig->csi_ReportConfigToReleaseList) {
          for (int i = 0; i < csi_MeasConfig->csi_ReportConfigToReleaseList->list.count; i++) {
            long id = *csi_MeasConfig->csi_ReportConfigToReleaseList->list.array[i];
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->csi_ReportConfigToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->csi_ReportConfigToAddModList->list.array[j]->reportConfigId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->csi_ReportConfigToAddModList->list.count)
              asn_sequence_del(&sc_info->csi_MeasConfig->csi_ReportConfigToAddModList->list, j, 1);
            else
              LOG_E(NR_MAC, "Element not present in the list, impossible to release\n");
          }
        }
        if (csi_MeasConfig->csi_ReportConfigToAddModList) {
          for (int i = 0; i < csi_MeasConfig->csi_ReportConfigToAddModList->list.count; i++) {
            NR_CSI_ReportConfig_t *res = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[i];
            long id = res->reportConfigId;
            int j;
            for (j = 0; j < sc_info->csi_MeasConfig->csi_ReportConfigToAddModList->list.count; j++) {
              if(id == sc_info->csi_MeasConfig->csi_ReportConfigToAddModList->list.array[j]->reportConfigId)
                break;
            }
            if (j < sc_info->csi_MeasConfig->csi_ReportConfigToAddModList->list.count) { // modify
              NR_CSI_ReportConfig_t *mac_res = sc_info->csi_MeasConfig->csi_ReportConfigToAddModList->list.array[j];
              updateMACie(mac_res, res, NR_CSI_ReportConfig_t);
            }
            else { // add
              asn1cSequenceAdd(sc_info->csi_MeasConfig->csi_ReportConfigToAddModList->list,
                               NR_CSI_ReportConfig_t,
                               csi_rep);
              updateMACie(csi_rep, res, NR_CSI_ReportConfig_t);
            }
          }
        }
      }
      break;
    default :
      AssertFatal(false, "Invalid case\n");
  }
}

static void configure_servingcell_info(NR_UE_ServingCell_Info_t *sc_info,
                                       NR_ServingCellConfig_t *scd)
{
  if (scd->csi_MeasConfig)
    configure_csiconfig(sc_info, scd->csi_MeasConfig);

  if (scd->supplementaryUplink)
    updateMACie(sc_info->supplementaryUplink,
                scd->supplementaryUplink,
                NR_UplinkConfig_t);
  if (scd->crossCarrierSchedulingConfig)
    updateMACie(sc_info->crossCarrierSchedulingConfig,
                scd->crossCarrierSchedulingConfig,
                NR_CrossCarrierSchedulingConfig_t);
  if (scd->pdsch_ServingCellConfig) {
    switch (scd->pdsch_ServingCellConfig->present) {
      case NR_SetupRelease_PDSCH_ServingCellConfig_PR_NOTHING :
        break;
      case NR_SetupRelease_PDSCH_ServingCellConfig_PR_release :
        // release all configurations
        if (sc_info->pdsch_CGB_Transmission) {
          ASN_STRUCT_FREE(asn_DEF_NR_PDSCH_CodeBlockGroupTransmission,
                          sc_info->pdsch_CGB_Transmission);
          sc_info->pdsch_CGB_Transmission = NULL;
        }
        if (sc_info->xOverhead_PDSCH) {
          free(sc_info->xOverhead_PDSCH);
          sc_info->xOverhead_PDSCH = NULL;
        }
        if (sc_info->maxMIMO_Layers_PDSCH) {
          free(sc_info->maxMIMO_Layers_PDSCH);
          sc_info->maxMIMO_Layers_PDSCH = NULL;
        }
        break;
      case NR_SetupRelease_PDSCH_ServingCellConfig_PR_setup : {
        NR_PDSCH_ServingCellConfig_t *pdsch_servingcellconfig = scd->pdsch_ServingCellConfig->choice.setup;
        if (pdsch_servingcellconfig->codeBlockGroupTransmission)
          handleMACsetuprelease(sc_info->pdsch_CGB_Transmission,
                                pdsch_servingcellconfig->codeBlockGroupTransmission,
                                NR_PDSCH_CodeBlockGroupTransmission_t,
                                asn_DEF_NR_PDSCH_CodeBlockGroupTransmission);
        updateMACie(sc_info->xOverhead_PDSCH,
                    pdsch_servingcellconfig->xOverhead,
                    long);
        if (pdsch_servingcellconfig->ext1 &&
            pdsch_servingcellconfig->ext1->maxMIMO_Layers)
          updateMACie(sc_info->maxMIMO_Layers_PDSCH,
                      pdsch_servingcellconfig->ext1->maxMIMO_Layers,
                      long);
        break;
      }
      default :
        AssertFatal(false, "Invalid case\n");
    }
  }
  if (scd->uplinkConfig &&
      scd->uplinkConfig->pusch_ServingCellConfig) {
    switch (scd->uplinkConfig->pusch_ServingCellConfig->present) {
      case NR_SetupRelease_PUSCH_ServingCellConfig_PR_NOTHING :
        break;
      case NR_SetupRelease_PUSCH_ServingCellConfig_PR_release :
        // release all configurations
        if (sc_info->pusch_CGB_Transmission) {
          ASN_STRUCT_FREE(asn_DEF_NR_PUSCH_CodeBlockGroupTransmission,
                          sc_info->pusch_CGB_Transmission);
          sc_info->pdsch_CGB_Transmission = NULL;
        }
        if (sc_info->rateMatching_PUSCH) {
          free(sc_info->rateMatching_PUSCH);
          sc_info->rateMatching_PUSCH = NULL;
        }
        if (sc_info->xOverhead_PUSCH) {
          free(sc_info->xOverhead_PUSCH);
          sc_info->xOverhead_PUSCH = NULL;
        }
        if (sc_info->maxMIMO_Layers_PUSCH) {
          free(sc_info->maxMIMO_Layers_PUSCH);
          sc_info->maxMIMO_Layers_PUSCH = NULL;
        }
        break;
      case NR_SetupRelease_PUSCH_ServingCellConfig_PR_setup : {
        NR_PUSCH_ServingCellConfig_t *pusch_servingcellconfig = scd->uplinkConfig->pusch_ServingCellConfig->choice.setup;
        updateMACie(sc_info->rateMatching_PUSCH,
                    pusch_servingcellconfig->rateMatching,
                    long);
        updateMACie(sc_info->xOverhead_PUSCH,
                    pusch_servingcellconfig->xOverhead,
                    long);
        if (pusch_servingcellconfig->ext1 &&
            pusch_servingcellconfig->ext1->maxMIMO_Layers)
          updateMACie(sc_info->maxMIMO_Layers_PUSCH,
                      pusch_servingcellconfig->ext1->maxMIMO_Layers,
                      long);
        if (pusch_servingcellconfig->codeBlockGroupTransmission)
          handleMACsetuprelease(sc_info->pusch_CGB_Transmission,
                                pusch_servingcellconfig->codeBlockGroupTransmission,
                                NR_PUSCH_CodeBlockGroupTransmission_t,
                                asn_DEF_NR_PUSCH_CodeBlockGroupTransmission);
        break;
      }
      default :
        AssertFatal(false, "Invalid case\n");
    }
  }
}

void nr_rrc_mac_config_req_cg(module_id_t module_id,
                              int cc_idP,
                              NR_CellGroupConfig_t *cell_group_config)
{
  LOG_I(MAC,"Applying CellGroupConfig from gNodeB\n");
  AssertFatal(cell_group_config, "CellGroupConfig should not be NULL\n");
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

  if (cell_group_config->mac_CellGroupConfig)
    configure_maccellgroup(mac, cell_group_config->mac_CellGroupConfig);

  if (cell_group_config->physicalCellGroupConfig)
    configure_physicalcellgroup(mac, cell_group_config->physicalCellGroupConfig);

  if (cell_group_config->spCellConfig) {
    NR_SpCellConfig_t *spCellConfig = cell_group_config->spCellConfig;
    NR_ServingCellConfig_t *scd = spCellConfig->spCellConfigDedicated;
    mac->servCellIndex = spCellConfig->servCellIndex ? *spCellConfig->servCellIndex : 0;
    if (spCellConfig->reconfigurationWithSync) {
      LOG_A(NR_MAC, "Received reconfigurationWithSync\n");
      handle_reconfiguration_with_sync(mac, module_id, cc_idP, spCellConfig->reconfigurationWithSync);
    }
    if (scd) {
      configure_servingcell_info(&mac->sc_info, scd);
      configure_current_BWP(mac, NULL, scd);
    }
  }

  if (!mac->dl_config_request || !mac->ul_config_request)
    ue_init_config_request(mac, mac->current_DL_BWP.scs);
}
