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

static void config_common_ue_sa(NR_UE_MAC_INST_t *mac,
                                NR_ServingCellConfigCommonSIB_t *scc,
                                int cc_idP)
{
  fapi_nr_config_request_t *cfg = &mac->phy_config.config_req;
  mac->phy_config.Mod_id = mac->ue_id;
  mac->phy_config.CC_id = cc_idP;

  LOG_D(MAC, "Entering SA UE Config Common\n");

  // carrier config
  NR_FrequencyInfoDL_SIB_t *frequencyInfoDL = &scc->downlinkConfigCommon.frequencyInfoDL;
  AssertFatal(frequencyInfoDL->frequencyBandList.list.array[0]->freqBandIndicatorNR, "Field mandatory present for DL in SIB1\n");
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
  mac->p_Max = frequencyInfoUL->p_Max ? *frequencyInfoUL->p_Max : INT_MIN;
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
    set_tdd_config_nr_ue(&cfg->tdd_table_1, cfg->ssb_config.scs_common, &mac->tdd_UL_DL_ConfigurationCommon->pattern1);
    if (mac->tdd_UL_DL_ConfigurationCommon->pattern2) {
      cfg->tdd_table_2 = (fapi_nr_tdd_table_t *) malloc(sizeof(fapi_nr_tdd_table_t));
      set_tdd_config_nr_ue(cfg->tdd_table_2, cfg->ssb_config.scs_common, mac->tdd_UL_DL_ConfigurationCommon->pattern2);
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

  AssertFatal(rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM < 4,
              "msg1 FDM identifier %ld undefined (0,1,2,3)\n", rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM);
  cfg->prach_config.num_prach_fd_occasions = 1 << rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM;


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

static void config_common_ue(NR_UE_MAC_INST_t *mac,
                             NR_ServingCellConfigCommon_t *scc,
                             int cc_idP)
{
  fapi_nr_config_request_t *cfg = &mac->phy_config.config_req;

  mac->phy_config.Mod_id = mac->ue_id;
  mac->phy_config.CC_id = cc_idP;
  
  // carrier config
  LOG_D(MAC, "Entering UE Config Common\n");

  AssertFatal(scc->downlinkConfigCommon, "Not expecting downlinkConfigCommon to be NULL here\n");

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
                                                    frequencyInfoDL->absoluteFrequencyPointA)
                                       / 1000; // freq in kHz

    for (int i = 0; i < 5; i++) {
      if (i == frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
        cfg->carrier_config.dl_grid_size[i] = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
        cfg->carrier_config.dl_k0[i] = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
      } else {
        cfg->carrier_config.dl_grid_size[i] = 0;
        cfg->carrier_config.dl_k0[i] = 0;
      }
    }
  }

  if (scc->uplinkConfigCommon && scc->uplinkConfigCommon->frequencyInfoUL) {
    NR_FrequencyInfoUL_t *frequencyInfoUL = scc->uplinkConfigCommon->frequencyInfoUL;
    mac->p_Max = frequencyInfoUL->p_Max ? *frequencyInfoUL->p_Max : INT_MIN;

    int bw_index = get_supported_band_index(frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                            *frequencyInfoUL->frequencyBandList->list.array[0],
                                            frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
    cfg->carrier_config.uplink_bandwidth = get_supported_bw_mhz(mac->frequency_range, bw_index);

    long *UL_pointA = NULL;
    if (frequencyInfoUL->absoluteFrequencyPointA)
      UL_pointA = frequencyInfoUL->absoluteFrequencyPointA;
    else if (frequencyInfoDL)
      UL_pointA = &frequencyInfoDL->absoluteFrequencyPointA;

    if (UL_pointA)
      cfg->carrier_config.uplink_frequency = from_nrarfcn(*frequencyInfoUL->frequencyBandList->list.array[0],
                                                          *scc->ssbSubcarrierSpacing,
                                                          *UL_pointA)
                                             / 1000; // freq in kHz

    for (int i = 0; i < 5; i++) {
      if (i == frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
        cfg->carrier_config.ul_grid_size[i] = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
        cfg->carrier_config.ul_k0[i] = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
      } else {
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
    int scs_scaling = 1 << (cfg->ssb_config.scs_common);
    if (frequencyInfoDL->absoluteFrequencyPointA < 600000)
      scs_scaling = scs_scaling * 3;
    if (frequencyInfoDL->absoluteFrequencyPointA > 2016666)
      scs_scaling = scs_scaling >> 2;
    uint32_t absolute_diff = (*frequencyInfoDL->absoluteFrequencySSB - frequencyInfoDL->absoluteFrequencyPointA);
    cfg->ssb_table.ssb_offset_point_a = absolute_diff / (12 * scs_scaling) - 10;
    cfg->ssb_table.ssb_period = *scc->ssb_periodicityServingCell;
    // NSA -> take ssb offset from SCS
    cfg->ssb_table.ssb_subcarrier_offset = absolute_diff % (12 * scs_scaling);
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
    set_tdd_config_nr_ue(&cfg->tdd_table_1, cfg->ssb_config.scs_common, &mac->tdd_UL_DL_ConfigurationCommon->pattern1);
    if (mac->tdd_UL_DL_ConfigurationCommon->pattern2) {
      cfg->tdd_table_2 = (fapi_nr_tdd_table_t *) malloc(sizeof(fapi_nr_tdd_table_t));
      set_tdd_config_nr_ue(cfg->tdd_table_2, cfg->ssb_config.scs_common, mac->tdd_UL_DL_ConfigurationCommon->pattern2);
    }
  }

  // PRACH configuration
  uint8_t nb_preambles = 64;
  if (scc->uplinkConfigCommon && scc->uplinkConfigCommon->initialUplinkBWP
      && scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon) { // all NeedM

    NR_RACH_ConfigCommon_t *rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
    if (rach_ConfigCommon->totalNumberOfRA_Preambles != NULL)
      nb_preambles = *rach_ConfigCommon->totalNumberOfRA_Preambles;

    cfg->prach_config.prach_sequence_length = rach_ConfigCommon->prach_RootSequenceIndex.present - 1;

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

    AssertFatal(rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM < 4,
                "msg1 FDM identifier %ld undefined (0,1,2,3)\n", rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM);
    cfg->prach_config.num_prach_fd_occasions = 1 << rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM;

    cfg->prach_config.num_prach_fd_occasions_list = (fapi_nr_num_prach_fd_occasions_t *)malloc(
        cfg->prach_config.num_prach_fd_occasions * sizeof(fapi_nr_num_prach_fd_occasions_t));
    for (int i = 0; i < cfg->prach_config.num_prach_fd_occasions; i++) {
      fapi_nr_num_prach_fd_occasions_t *prach_fd_occasion = &cfg->prach_config.num_prach_fd_occasions_list[i];
      prach_fd_occasion->num_prach_fd_occasions = i;
      if (cfg->prach_config.prach_sequence_length)
        prach_fd_occasion->prach_root_sequence_index = rach_ConfigCommon->prach_RootSequenceIndex.choice.l139;
      else
        prach_fd_occasion->prach_root_sequence_index = rach_ConfigCommon->prach_RootSequenceIndex.choice.l839;

      prach_fd_occasion->k1 = rach_ConfigCommon->rach_ConfigGeneric.msg1_FrequencyStart;
      prach_fd_occasion->prach_zero_corr_conf = rach_ConfigCommon->rach_ConfigGeneric.zeroCorrelationZoneConfig;
      prach_fd_occasion->num_root_sequences =
          compute_nr_root_seq(rach_ConfigCommon, nb_preambles, mac->frame_type, mac->frequency_range);

      cfg->prach_config.ssb_per_rach = rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present - 1;
      // prach_fd_occasion->num_unused_root_sequences = ???
    }
  }
}

void release_common_ss_cset(NR_BWP_PDCCH_t *pdcch)
{
  asn1cFreeStruc(asn_DEF_NR_SearchSpace, pdcch->otherSI_SS);
  asn1cFreeStruc(asn_DEF_NR_SearchSpace, pdcch->ra_SS);
  asn1cFreeStruc(asn_DEF_NR_SearchSpace, pdcch->paging_SS);
  asn1cFreeStruc(asn_DEF_NR_SearchSpace, pdcch->search_space_zero);
  asn1cFreeStruc(asn_DEF_NR_ControlResourceSet, pdcch->commonControlResourceSet);
  asn1cFreeStruc(asn_DEF_NR_ControlResourceSet, pdcch->coreset0);
}

static void modlist_ss(NR_SearchSpace_t *source, NR_SearchSpace_t *target)
{
  target->searchSpaceId = source->searchSpaceId;
  if (source->controlResourceSetId)
    UPDATE_MAC_IE(target->controlResourceSetId, source->controlResourceSetId, NR_ControlResourceSetId_t);
  if (source->monitoringSlotPeriodicityAndOffset)
    UPDATE_MAC_IE(target->monitoringSlotPeriodicityAndOffset,
                source->monitoringSlotPeriodicityAndOffset,
                struct NR_SearchSpace__monitoringSlotPeriodicityAndOffset);
  UPDATE_MAC_IE(target->duration, source->duration, long);
  if (source->monitoringSymbolsWithinSlot)
    UPDATE_MAC_IE(target->monitoringSymbolsWithinSlot, source->monitoringSymbolsWithinSlot, BIT_STRING_t);
  if (source->nrofCandidates)
    UPDATE_MAC_IE(target->nrofCandidates, source->nrofCandidates, struct NR_SearchSpace__nrofCandidates);
  if (source->searchSpaceType)
    UPDATE_MAC_IE(target->searchSpaceType, source->searchSpaceType, struct NR_SearchSpace__searchSpaceType);
}

static NR_SearchSpace_t *get_common_search_space(const struct NR_PDCCH_ConfigCommon__commonSearchSpaceList *commonSearchSpaceList,
                                                 const NR_BWP_PDCCH_t *pdcch,
                                                 const NR_SearchSpaceId_t ss_id)
{
  if (ss_id == 0)
    return pdcch->search_space_zero;

  NR_SearchSpace_t *css = NULL;
  for (int i = 0; i < commonSearchSpaceList->list.count; i++) {
    if (commonSearchSpaceList->list.array[i]->searchSpaceId == ss_id) {
      css = calloc(1, sizeof(*css));
      modlist_ss(commonSearchSpaceList->list.array[i], css);
      break;
    }
  }
  AssertFatal(css, "Couldn't find CSS with Id %ld\n", ss_id);
  return css;
}

static void configure_common_ss_coreset(NR_BWP_PDCCH_t *pdcch, NR_PDCCH_ConfigCommon_t *pdcch_ConfigCommon)
{
  if (pdcch_ConfigCommon) {
    asn1cFreeStruc(asn_DEF_NR_SearchSpace, pdcch->otherSI_SS);
    if (pdcch_ConfigCommon->searchSpaceOtherSystemInformation)
      pdcch->otherSI_SS = get_common_search_space(pdcch_ConfigCommon->commonSearchSpaceList,
                                                  pdcch,
                                                  *pdcch_ConfigCommon->searchSpaceOtherSystemInformation);

    asn1cFreeStruc(asn_DEF_NR_SearchSpace, pdcch->ra_SS);
    if (pdcch_ConfigCommon->ra_SearchSpace) {
      if (pdcch->otherSI_SS && *pdcch_ConfigCommon->ra_SearchSpace == pdcch->otherSI_SS->searchSpaceId)
        pdcch->ra_SS = pdcch->otherSI_SS;
      else
        pdcch->ra_SS =
            get_common_search_space(pdcch_ConfigCommon->commonSearchSpaceList, pdcch, *pdcch_ConfigCommon->ra_SearchSpace);
    }

    asn1cFreeStruc(asn_DEF_NR_SearchSpace, pdcch->paging_SS);
    if (pdcch_ConfigCommon->pagingSearchSpace) {
      if (pdcch->otherSI_SS && *pdcch_ConfigCommon->pagingSearchSpace == pdcch->otherSI_SS->searchSpaceId)
        pdcch->paging_SS = pdcch->otherSI_SS;
      else if (pdcch->ra_SS && *pdcch_ConfigCommon->pagingSearchSpace == pdcch->ra_SS->searchSpaceId)
        pdcch->paging_SS = pdcch->ra_SS;
      if (!pdcch->paging_SS)
        pdcch->paging_SS =
            get_common_search_space(pdcch_ConfigCommon->commonSearchSpaceList, pdcch, *pdcch_ConfigCommon->pagingSearchSpace);
    }

    UPDATE_MAC_IE(pdcch->commonControlResourceSet, pdcch_ConfigCommon->commonControlResourceSet, NR_ControlResourceSet_t);
  }
}

static void modlist_coreset(NR_ControlResourceSet_t *source, NR_ControlResourceSet_t *target)
{
  target->controlResourceSetId = source->controlResourceSetId;
  target->frequencyDomainResources.size = source->frequencyDomainResources.size;
  if (!target->frequencyDomainResources.buf)
    target->frequencyDomainResources.buf =
        calloc(target->frequencyDomainResources.size, sizeof(*target->frequencyDomainResources.buf));
  for (int i = 0; i < source->frequencyDomainResources.size; i++)
    target->frequencyDomainResources.buf[i] = source->frequencyDomainResources.buf[i];
  target->duration = source->duration;
  target->precoderGranularity = source->precoderGranularity;
  long *shiftIndex = NULL;
  if (target->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved)
    shiftIndex = target->cce_REG_MappingType.choice.interleaved->shiftIndex;
  if (source->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved) {
    target->cce_REG_MappingType.present = NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved;
    target->cce_REG_MappingType.choice.interleaved->reg_BundleSize = source->cce_REG_MappingType.choice.interleaved->reg_BundleSize;
    target->cce_REG_MappingType.choice.interleaved->interleaverSize =
        source->cce_REG_MappingType.choice.interleaved->interleaverSize;
    UPDATE_MAC_IE(target->cce_REG_MappingType.choice.interleaved->shiftIndex,
                  source->cce_REG_MappingType.choice.interleaved->shiftIndex,
                  long);
  } else {
    free(shiftIndex);
    target->cce_REG_MappingType = source->cce_REG_MappingType;
  }
  UPDATE_MAC_IE(target->tci_PresentInDCI, source->tci_PresentInDCI, long);
  UPDATE_MAC_IE(target->pdcch_DMRS_ScramblingID, source->pdcch_DMRS_ScramblingID, long);
  // TCI States
  if (source->tci_StatesPDCCH_ToReleaseList) {
    for (int i = 0; i < source->tci_StatesPDCCH_ToReleaseList->list.count; i++) {
      long id = *source->tci_StatesPDCCH_ToReleaseList->list.array[i];
      int j;
      for (j = 0; j < target->tci_StatesPDCCH_ToAddList->list.count; j++) {
        if (id == *target->tci_StatesPDCCH_ToAddList->list.array[j])
          break;
      }
      if (j < target->tci_StatesPDCCH_ToAddList->list.count)
        asn_sequence_del(&target->tci_StatesPDCCH_ToAddList->list, j, 1);
      else
        LOG_E(NR_MAC, "Element not present in the list, impossible to release\n");
    }
  }
  if (source->tci_StatesPDCCH_ToAddList) {
    if (target->tci_StatesPDCCH_ToAddList) {
      for (int i = 0; i < source->tci_StatesPDCCH_ToAddList->list.count; i++) {
        long id = *source->tci_StatesPDCCH_ToAddList->list.array[i];
        int j;
        for (j = 0; j < target->tci_StatesPDCCH_ToAddList->list.count; j++) {
          if (id == *target->tci_StatesPDCCH_ToAddList->list.array[j])
            break;
        }
        if (j == target->tci_StatesPDCCH_ToAddList->list.count)
          ASN_SEQUENCE_ADD(&target->tci_StatesPDCCH_ToAddList->list, source->tci_StatesPDCCH_ToAddList->list.array[i]);
      }
    } else
      UPDATE_MAC_IE(target->tci_StatesPDCCH_ToAddList,
                    source->tci_StatesPDCCH_ToAddList,
                    struct NR_ControlResourceSet__tci_StatesPDCCH_ToAddList);
  }
  // end TCI States
}

static void configure_ss_coreset(NR_BWP_PDCCH_t *pdcch, NR_PDCCH_Config_t *pdcch_Config)
{
  if (!pdcch_Config)
    return;
  if (pdcch_Config->controlResourceSetToAddModList) {
    for (int i = 0; i < pdcch_Config->controlResourceSetToAddModList->list.count; i++) {
      NR_ControlResourceSet_t *source_coreset = pdcch_Config->controlResourceSetToAddModList->list.array[i];
      NR_ControlResourceSet_t *target_coreset = NULL;
      for (int j = 0; j < pdcch->list_Coreset.count; j++) {
        if (pdcch->list_Coreset.array[j]->controlResourceSetId == source_coreset->controlResourceSetId) {
          target_coreset = pdcch->list_Coreset.array[j];
          break;
        }
      }
      if (!target_coreset) {
        target_coreset = calloc(1, sizeof(*target_coreset));
        ASN_SEQUENCE_ADD(&pdcch->list_Coreset, target_coreset);
      }
      modlist_coreset(source_coreset, target_coreset);
    }
  }
  if (pdcch_Config->controlResourceSetToReleaseList) {
    for (int i = 0; i < pdcch_Config->controlResourceSetToReleaseList->list.count; i++) {
      NR_ControlResourceSetId_t id = *pdcch_Config->controlResourceSetToReleaseList->list.array[i];
      for (int j = 0; j < pdcch->list_Coreset.count; j++) {
        if (id == pdcch->list_Coreset.array[j]->controlResourceSetId) {
          asn_sequence_del(&pdcch->list_Coreset, j, 1);
          break;
        }
      }
    }
  }
  if (pdcch_Config->searchSpacesToAddModList) {
    for (int i = 0; i < pdcch_Config->searchSpacesToAddModList->list.count; i++) {
      NR_SearchSpace_t *source_ss = pdcch_Config->searchSpacesToAddModList->list.array[i];
      NR_SearchSpace_t *target_ss = NULL;
      for (int j = 0; j < pdcch->list_SS.count; j++) {
        if (pdcch->list_SS.array[j]->searchSpaceId == source_ss->searchSpaceId) {
          target_ss = pdcch->list_SS.array[j];
          break;
        }
      }
      if (!target_ss) {
        target_ss = calloc(1, sizeof(*target_ss));
        ASN_SEQUENCE_ADD(&pdcch->list_SS, target_ss);
      }
      modlist_ss(source_ss, target_ss);
    }
  }
  if (pdcch_Config->searchSpacesToReleaseList) {
    for (int i = 0; i < pdcch_Config->searchSpacesToReleaseList->list.count; i++) {
      NR_ControlResourceSetId_t id = *pdcch_Config->searchSpacesToReleaseList->list.array[i];
      for (int j = 0; j < pdcch->list_SS.count; j++) {
        if (id == pdcch->list_SS.array[j]->searchSpaceId) {
          asn_sequence_del(&pdcch->list_SS, j, 1);
          break;
        }
      }
    }
  }
}

static int lcid_cmp(const void *a, const void *b)
{
  long priority_a = (*((nr_lcordered_info_t**)a))->priority;
  AssertFatal(priority_a > 0 && priority_a < 17, "Invalid priority value %ld\n", priority_a);
  long priority_b = (*((nr_lcordered_info_t**)b))->priority;
  AssertFatal(priority_b > 0 && priority_b < 17, "Invalid priority value %ld\n", priority_b);
  return priority_a - priority_b;
}

static int nr_get_ms_bucketsizeduration(long bucketsizeduration)
{
  switch (bucketsizeduration) {
    case NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms5:
      return 5;
    case NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms10:
      return 10;
    case NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms20:
      return 20;
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
      AssertFatal(false, "Invalid bucketSizeDuration %ld\n", bucketsizeduration);
  }
}

static uint32_t get_lc_bucket_size(long prioritisedBitRate, long bucketSizeDuration)
{
  int pbr = nr_get_pbr(prioritisedBitRate);
  // in infinite pbr, the bucket is saturated by pbr
  int bsd = 0;
  if (prioritisedBitRate == NR_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity)
    bsd = 1;
  else
    bsd = nr_get_ms_bucketsizeduration(bucketSizeDuration);

  return pbr * bsd;
}

// default configuration as per 38.331 section 9.2.1
static void set_default_logicalchannelconfig(nr_lcordered_info_t *lc_info, NR_SRB_Identity_t srb_id)
{
  lc_info->lcid = srb_id;
  lc_info->priority = srb_id == 2 ? 3 : 1;
  lc_info->prioritisedBitRate = NR_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  lc_info->bucket_size = get_lc_bucket_size(lc_info->prioritisedBitRate, 0);
}

static void nr_configure_lc_config(NR_UE_MAC_INST_t *mac,
                                   nr_lcordered_info_t *lc_info,
                                   NR_LogicalChannelConfig_t *mac_lc_config,
                                   NR_SRB_Identity_t srb_id)
{
  if (srb_id > 0 && !mac_lc_config->ul_SpecificParameters) {
    // release configuration and reset to default
    set_default_logicalchannelconfig(lc_info, srb_id);
    mac->scheduling_info.lc_sched_info[lc_info->lcid - 1].LCGID = 0;
    return;
  }
  AssertFatal(mac_lc_config->ul_SpecificParameters, "UL parameters shouldn't be NULL for DRBs\n");
  struct NR_LogicalChannelConfig__ul_SpecificParameters *ul_parm = mac_lc_config->ul_SpecificParameters;
  lc_info->priority = ul_parm->priority;
  lc_info->prioritisedBitRate = ul_parm->prioritisedBitRate;
  // TODO Verify setting to 0 is ok, 331 just says need R (release if NULL)
  mac->scheduling_info.lc_sched_info[lc_info->lcid - 1].LCGID = ul_parm->logicalChannelGroup ? *ul_parm->logicalChannelGroup : 0;
  lc_info->bucket_size = get_lc_bucket_size(ul_parm->prioritisedBitRate, ul_parm->bucketSizeDuration);
}

static void configure_logicalChannelBearer(NR_UE_MAC_INST_t *mac,
                                           struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_toadd_list,
                                           struct NR_CellGroupConfig__rlc_BearerToReleaseList *rlc_torelease_list)
{
  if (rlc_torelease_list) {
    for (int i = 0; i < rlc_torelease_list->list.count; i++) {
      long id = *rlc_torelease_list->list.array[i];
      int j;
      for (j = 0; j < mac->lc_ordered_list.count; j++) {
        if (id == mac->lc_ordered_list.array[j]->lcid)
          break;
      }
      if (j < mac->lc_ordered_list.count) {
        nr_lcordered_info_t *lc_info = mac->lc_ordered_list.array[j];
        asn_sequence_del(&mac->lc_ordered_list, j, 0);
        free(lc_info);
      }
      else
        LOG_E(NR_MAC, "Element not present in the list, impossible to release\n");
    }
  }

  if (rlc_toadd_list) {
    for (int i = 0; i < rlc_toadd_list->list.count; i++) {
      NR_RLC_BearerConfig_t *rlc_bearer = rlc_toadd_list->list.array[i];
      int lc_identity = rlc_bearer->logicalChannelIdentity;
      NR_LogicalChannelConfig_t *mac_lc_config = rlc_bearer->mac_LogicalChannelConfig;
      int j;
      for (j = 0; j < mac->lc_ordered_list.count; j++) {
        if (lc_identity == mac->lc_ordered_list.array[j]->lcid)
          break;
      }
      if (j < mac->lc_ordered_list.count) {
        LOG_D(NR_MAC, "Logical channel %d is already established, Reconfiguring now\n", lc_identity);
        if (mac_lc_config != NULL) {
          NR_SRB_Identity_t srb_id = 0;
          if (rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity)
            srb_id = rlc_bearer->servedRadioBearer->choice.srb_Identity;
          nr_configure_lc_config(mac, mac->lc_ordered_list.array[j], mac_lc_config, srb_id);
        }
      }
      else {
        /* setup of new LCID*/
        nr_lcordered_info_t *lc_info = calloc(1, sizeof(*lc_info));
        lc_info->lcid = lc_identity;
        LOG_D(NR_MAC, "Establishing the logical channel %d\n", lc_identity);
        AssertFatal(rlc_bearer->servedRadioBearer, "servedRadioBearer should be present for LCID establishment\n");
        if (rlc_bearer->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity) { /* SRB */
          NR_SRB_Identity_t srb_id = rlc_bearer->servedRadioBearer->choice.srb_Identity;
          if (mac_lc_config != NULL)
            nr_configure_lc_config(mac, lc_info, mac_lc_config, srb_id);
          else
            set_default_logicalchannelconfig(lc_info, srb_id);
        } else { /* DRB */
          AssertFatal(mac_lc_config, "When establishing a DRB, LogicalChannelConfig should be mandatorily present\n");
          nr_configure_lc_config(mac, lc_info, mac_lc_config, 0);
        }
        ASN_SEQUENCE_ADD(&mac->lc_ordered_list, lc_info);
      }
    }

    // reorder the logical channels as per its priority
    qsort(mac->lc_ordered_list.array, mac->lc_ordered_list.count, sizeof(nr_lcordered_info_t*), lcid_cmp);
  }
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

static void update_mib_conf(NR_MIB_t *target, NR_MIB_t *source)
{
  target->systemFrameNumber.size = source->systemFrameNumber.size;
  target->systemFrameNumber.bits_unused = source->systemFrameNumber.bits_unused;
  if (!target->systemFrameNumber.buf)
    target->systemFrameNumber.buf = calloc(target->systemFrameNumber.size, sizeof(*target->systemFrameNumber.buf));
  for (int i = 0; i < target->systemFrameNumber.size; i++)
    target->systemFrameNumber.buf[i] = source->systemFrameNumber.buf[i];
  target->subCarrierSpacingCommon = source->subCarrierSpacingCommon;
  target->ssb_SubcarrierOffset = source->ssb_SubcarrierOffset;
  target->dmrs_TypeA_Position = source->dmrs_TypeA_Position;
  target->pdcch_ConfigSIB1 = source->pdcch_ConfigSIB1;
  target->cellBarred = source->cellBarred;
  target->intraFreqReselection = source->intraFreqReselection;
}

void nr_rrc_mac_config_req_mib(module_id_t module_id,
                               int cc_idP,
                               NR_MIB_t *mib,
                               int sched_sib)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  AssertFatal(mib, "MIB should not be NULL\n");
  if (!mac->mib)
    mac->mib = calloc(1, sizeof(*mac->mib));
  update_mib_conf(mac->mib, mib);
  mac->phy_config.Mod_id = module_id;
  mac->phy_config.CC_id = cc_idP;
  if (sched_sib == 1)
    mac->get_sib1 = true;
  else if (sched_sib == 2)
    mac->get_otherSI = true;
  nr_ue_decode_mib(mac, cc_idP);
}

static void setup_puschpowercontrol(NR_PUSCH_PowerControl_t *source, NR_PUSCH_PowerControl_t *target)
{
  UPDATE_MAC_IE(target->tpc_Accumulation, source->tpc_Accumulation, long);
  UPDATE_MAC_IE(target->msg3_Alpha, source->msg3_Alpha, NR_Alpha_t);
  if (source->p0_NominalWithoutGrant)
    UPDATE_MAC_IE(target->p0_NominalWithoutGrant, source->p0_NominalWithoutGrant, long);
  if (source->p0_AlphaSets)
    UPDATE_MAC_IE(target->p0_AlphaSets, source->p0_AlphaSets, struct NR_PUSCH_PowerControl__p0_AlphaSets);
  UPDATE_MAC_IE(target->twoPUSCH_PC_AdjustmentStates, source->twoPUSCH_PC_AdjustmentStates, long);
  UPDATE_MAC_IE(target->deltaMCS, source->deltaMCS, long);
  if (source->pathlossReferenceRSToReleaseList) {
    RELEASE_IE_FROMLIST(source->pathlossReferenceRSToReleaseList,
                        target->pathlossReferenceRSToAddModList,
                        pusch_PathlossReferenceRS_Id);
  }
  if (source->pathlossReferenceRSToAddModList) {
    if (!target->pathlossReferenceRSToAddModList)
      target->pathlossReferenceRSToAddModList = calloc(1, sizeof(*target->pathlossReferenceRSToAddModList));
    ADDMOD_IE_FROMLIST(source->pathlossReferenceRSToAddModList,
                       target->pathlossReferenceRSToAddModList,
                       pusch_PathlossReferenceRS_Id,
                       NR_PUSCH_PathlossReferenceRS_t);
  }
  if (source->sri_PUSCH_MappingToReleaseList) {
    RELEASE_IE_FROMLIST(source->sri_PUSCH_MappingToReleaseList,
                        target->sri_PUSCH_MappingToAddModList,
                        sri_PUSCH_PowerControlId);
  }
  if (source->sri_PUSCH_MappingToAddModList) {
    if (!target->sri_PUSCH_MappingToAddModList)
      target->sri_PUSCH_MappingToAddModList = calloc(1, sizeof(*target->sri_PUSCH_MappingToAddModList));
    ADDMOD_IE_FROMLIST(source->sri_PUSCH_MappingToAddModList,
                       target->sri_PUSCH_MappingToAddModList,
                       sri_PUSCH_PowerControlId,
                       NR_SRI_PUSCH_PowerControl_t);
  }
}

static void setup_puschconfig(NR_PUSCH_Config_t *source, NR_PUSCH_Config_t *target)
{
  UPDATE_MAC_IE(target->dataScramblingIdentityPUSCH, source->dataScramblingIdentityPUSCH, long);
  UPDATE_MAC_IE(target->txConfig, source->txConfig, long);
  if (source->dmrs_UplinkForPUSCH_MappingTypeA)
    HANDLE_SETUPRELEASE_IE(target->dmrs_UplinkForPUSCH_MappingTypeA,
                           source->dmrs_UplinkForPUSCH_MappingTypeA,
                           NR_DMRS_UplinkConfig_t,
                           asn_DEF_NR_SetupRelease_DMRS_UplinkConfig);
  if (source->dmrs_UplinkForPUSCH_MappingTypeB)
    HANDLE_SETUPRELEASE_IE(target->dmrs_UplinkForPUSCH_MappingTypeB,
                           source->dmrs_UplinkForPUSCH_MappingTypeB,
                           NR_DMRS_UplinkConfig_t,
                           asn_DEF_NR_SetupRelease_DMRS_UplinkConfig);
  if (source->pusch_PowerControl) {
    if (!target->pusch_PowerControl)
      target->pusch_PowerControl = calloc(1, sizeof(*target->pusch_PowerControl));
    setup_puschpowercontrol(source->pusch_PowerControl, target->pusch_PowerControl);
  }
  UPDATE_MAC_IE(target->frequencyHopping, source->frequencyHopping, long);
  if (source->frequencyHoppingOffsetLists)
    UPDATE_MAC_IE(target->frequencyHoppingOffsetLists,
                  source->frequencyHoppingOffsetLists,
                  struct NR_PUSCH_Config__frequencyHoppingOffsetLists);
  target->resourceAllocation = source->resourceAllocation;
  if (source->pusch_TimeDomainAllocationList)
    HANDLE_SETUPRELEASE_IE(target->pusch_TimeDomainAllocationList,
                           source->pusch_TimeDomainAllocationList,
                           NR_PUSCH_TimeDomainResourceAllocationList_t,
                           asn_DEF_NR_SetupRelease_PUSCH_TimeDomainResourceAllocationList);
  UPDATE_MAC_IE(target->pusch_AggregationFactor, source->pusch_AggregationFactor, long);
  UPDATE_MAC_IE(target->mcs_Table, source->mcs_Table, long);
  UPDATE_MAC_IE(target->mcs_TableTransformPrecoder, source->mcs_TableTransformPrecoder, long);
  UPDATE_MAC_IE(target->transformPrecoder, source->transformPrecoder, long);
  UPDATE_MAC_IE(target->codebookSubset, source->codebookSubset, long);
  UPDATE_MAC_IE(target->maxRank, source->maxRank, long);
  UPDATE_MAC_IE(target->rbg_Size, source->rbg_Size, long);
  UPDATE_MAC_IE(target->tp_pi2BPSK, source->tp_pi2BPSK, long);
  if (source->uci_OnPUSCH) {
    if (source->uci_OnPUSCH->present == NR_SetupRelease_UCI_OnPUSCH_PR_release)
      asn1cFreeStruc(asn_DEF_NR_UCI_OnPUSCH, target->uci_OnPUSCH);
    if (source->uci_OnPUSCH->present == NR_SetupRelease_UCI_OnPUSCH_PR_setup) {
      if (target->uci_OnPUSCH) {
        target->uci_OnPUSCH->choice.setup->scaling = source->uci_OnPUSCH->choice.setup->scaling;
        if (source->uci_OnPUSCH->choice.setup->betaOffsets)
          UPDATE_MAC_IE(target->uci_OnPUSCH->choice.setup->betaOffsets,
                        source->uci_OnPUSCH->choice.setup->betaOffsets,
                        struct NR_UCI_OnPUSCH__betaOffsets);
      }
    }
  }
}

static void setup_pdschconfig(NR_PDSCH_Config_t *source, NR_PDSCH_Config_t *target)
{
  UPDATE_MAC_IE(target->dataScramblingIdentityPDSCH, source->dataScramblingIdentityPDSCH, long);
  if (source->dmrs_DownlinkForPDSCH_MappingTypeA)
    HANDLE_SETUPRELEASE_IE(target->dmrs_DownlinkForPDSCH_MappingTypeA,
                           source->dmrs_DownlinkForPDSCH_MappingTypeA,
                           NR_DMRS_DownlinkConfig_t,
                           asn_DEF_NR_SetupRelease_DMRS_DownlinkConfig);
  if (source->dmrs_DownlinkForPDSCH_MappingTypeB)
    HANDLE_SETUPRELEASE_IE(target->dmrs_DownlinkForPDSCH_MappingTypeB,
                           source->dmrs_DownlinkForPDSCH_MappingTypeB,
                           NR_DMRS_DownlinkConfig_t,
                           asn_DEF_NR_SetupRelease_DMRS_DownlinkConfig);
  // TCI States
  if (source->tci_StatesToReleaseList) {
    RELEASE_IE_FROMLIST(source->tci_StatesToReleaseList,
                        target->tci_StatesToAddModList,
                        tci_StateId);
  }
  if (source->tci_StatesToAddModList) {
    if (!target->tci_StatesToAddModList)
      target->tci_StatesToAddModList = calloc(1, sizeof(*target->tci_StatesToAddModList));
    ADDMOD_IE_FROMLIST(source->tci_StatesToAddModList,
                       target->tci_StatesToAddModList,
                       tci_StateId,
                       NR_TCI_State_t);
  }
  // end TCI States
  UPDATE_MAC_IE(target->vrb_ToPRB_Interleaver, source->vrb_ToPRB_Interleaver, long);
  target->resourceAllocation = source->resourceAllocation;
  if (source->pdsch_TimeDomainAllocationList)
    HANDLE_SETUPRELEASE_IE(target->pdsch_TimeDomainAllocationList,
                           source->pdsch_TimeDomainAllocationList,
                           NR_PDSCH_TimeDomainResourceAllocationList_t,
                           asn_DEF_NR_SetupRelease_PDSCH_TimeDomainResourceAllocationList);
  UPDATE_MAC_IE(target->pdsch_AggregationFactor, source->pdsch_AggregationFactor, long);
  // rateMatchPattern
  if (source->rateMatchPatternToReleaseList) {
    RELEASE_IE_FROMLIST(source->rateMatchPatternToReleaseList,
                        target->rateMatchPatternToAddModList,
                        rateMatchPatternId);
  }
  if (source->rateMatchPatternToAddModList) {
    if (!target->rateMatchPatternToAddModList)
      target->rateMatchPatternToAddModList = calloc(1, sizeof(*target->rateMatchPatternToAddModList));
    ADDMOD_IE_FROMLIST(source->rateMatchPatternToAddModList,
                       target->rateMatchPatternToAddModList,
                       rateMatchPatternId,
                       NR_RateMatchPattern_t);
  }
  // end rateMatchPattern
  UPDATE_MAC_IE(target->rateMatchPatternGroup1, source->rateMatchPatternGroup1, NR_RateMatchPatternGroup_t);
  UPDATE_MAC_IE(target->rateMatchPatternGroup2, source->rateMatchPatternGroup2, NR_RateMatchPatternGroup_t);
  target->rbg_Size = source->rbg_Size;
  UPDATE_MAC_IE(target->mcs_Table, source->mcs_Table, long);
  UPDATE_MAC_IE(target->maxNrofCodeWordsScheduledByDCI, source->maxNrofCodeWordsScheduledByDCI, long);
  UPDATE_MAC_NP_IE(target->prb_BundlingType, source->prb_BundlingType, struct NR_PDSCH_Config__prb_BundlingType);
  AssertFatal(source->zp_CSI_RS_ResourceToAddModList == NULL, "Not handled\n");
  AssertFatal(source->aperiodic_ZP_CSI_RS_ResourceSetsToAddModList == NULL, "Not handled\n");
  AssertFatal(source->sp_ZP_CSI_RS_ResourceSetsToAddModList == NULL, "Not handled\n");
}

static void setup_sr_resource(NR_SchedulingRequestResourceConfig_t *target, NR_SchedulingRequestResourceConfig_t *source)
{
  target->schedulingRequestResourceId = source->schedulingRequestResourceId;
  target->schedulingRequestID = source->schedulingRequestID;
  if (source->periodicityAndOffset)
    UPDATE_MAC_IE(target->periodicityAndOffset,
                  source->periodicityAndOffset,
                  struct NR_SchedulingRequestResourceConfig__periodicityAndOffset);
  if (source->resource)
    UPDATE_MAC_IE(target->resource, source->resource, NR_PUCCH_ResourceId_t);
}

static void setup_pucchconfig(NR_PUCCH_Config_t *source, NR_PUCCH_Config_t *target)
{
  // PUCCH-ResourceSet
  if (source->resourceSetToAddModList) {
    if (!target->resourceSetToAddModList)
      target->resourceSetToAddModList = calloc(1, sizeof(*target->resourceSetToAddModList));
    ADDMOD_IE_FROMLIST(source->resourceSetToAddModList,
                       target->resourceSetToAddModList,
                       pucch_ResourceSetId,
                       NR_PUCCH_ResourceSet_t);
  }
  if (source->resourceSetToReleaseList) {
    RELEASE_IE_FROMLIST(source->resourceSetToReleaseList,
                        target->resourceSetToAddModList,
                        pucch_ResourceSetId);
  }
  // PUCCH-Resource
  if (source->resourceToAddModList) {
    if (!target->resourceToAddModList)
      target->resourceToAddModList = calloc(1, sizeof(*target->resourceToAddModList));
    ADDMOD_IE_FROMLIST(source->resourceToAddModList,
                       target->resourceToAddModList,
                       pucch_ResourceId,
                       NR_PUCCH_Resource_t);
  }
  if (source->resourceToReleaseList) {
    RELEASE_IE_FROMLIST(source->resourceToReleaseList,
                        target->resourceToAddModList,
                        pucch_ResourceId);
  }
  // PUCCH-FormatConfig
  if (source->format1)
    HANDLE_SETUPRELEASE_IE(target->format1,
                           source->format1,
                           NR_PUCCH_FormatConfig_t,
                           asn_DEF_NR_SetupRelease_PUCCH_FormatConfig);
  if (source->format2)
    HANDLE_SETUPRELEASE_IE(target->format2,
                           source->format2,
                           NR_PUCCH_FormatConfig_t,
                           asn_DEF_NR_SetupRelease_PUCCH_FormatConfig);
  if (source->format3)
    HANDLE_SETUPRELEASE_IE(target->format3,
                           source->format3,
                           NR_PUCCH_FormatConfig_t,
                           asn_DEF_NR_SetupRelease_PUCCH_FormatConfig);
  if (source->format4)
    HANDLE_SETUPRELEASE_IE(target->format4,
                           source->format4,
                           NR_PUCCH_FormatConfig_t,
                           asn_DEF_NR_SetupRelease_PUCCH_FormatConfig);
  // SchedulingRequestResourceConfig
  if (source->schedulingRequestResourceToAddModList) {
    if (!target->schedulingRequestResourceToAddModList)
      target->schedulingRequestResourceToAddModList = calloc(1, sizeof(*target->schedulingRequestResourceToAddModList));
    ADDMOD_IE_FROMLIST_WFUNCTION(source->schedulingRequestResourceToAddModList,
                                 target->schedulingRequestResourceToAddModList,
                                 schedulingRequestResourceId,
                                 NR_SchedulingRequestResourceConfig_t,
                                 setup_sr_resource);
  }
  if (source->schedulingRequestResourceToReleaseList) {
    RELEASE_IE_FROMLIST(source->schedulingRequestResourceToReleaseList,
                        target->schedulingRequestResourceToAddModList,
                        schedulingRequestResourceId);
  }

  if (source->multi_CSI_PUCCH_ResourceList)
    UPDATE_MAC_IE(target->multi_CSI_PUCCH_ResourceList,
                  source->multi_CSI_PUCCH_ResourceList,
                  struct NR_PUCCH_Config__multi_CSI_PUCCH_ResourceList);
  if (source->dl_DataToUL_ACK)
    UPDATE_MAC_IE(target->dl_DataToUL_ACK, source->dl_DataToUL_ACK, struct NR_PUCCH_Config__dl_DataToUL_ACK);
  // PUCCH-SpatialRelationInfo
  if (source->spatialRelationInfoToAddModList) {
    if (!target->spatialRelationInfoToAddModList)
      target->spatialRelationInfoToAddModList = calloc(1, sizeof(*target->spatialRelationInfoToAddModList));
    ADDMOD_IE_FROMLIST(source->spatialRelationInfoToAddModList,
                       target->spatialRelationInfoToAddModList,
                       pucch_SpatialRelationInfoId,
                       NR_PUCCH_SpatialRelationInfo_t);
  }
  if (source->spatialRelationInfoToReleaseList) {
    RELEASE_IE_FROMLIST(source->spatialRelationInfoToReleaseList,
                        target->spatialRelationInfoToAddModList,
                        pucch_SpatialRelationInfoId);
  }

  if (source->pucch_PowerControl) {
    if (!target->pucch_PowerControl)
      target->pucch_PowerControl = calloc(1, sizeof(*target->pucch_PowerControl));
    UPDATE_MAC_IE(target->pucch_PowerControl->deltaF_PUCCH_f0, source->pucch_PowerControl->deltaF_PUCCH_f0, long);
    UPDATE_MAC_IE(target->pucch_PowerControl->deltaF_PUCCH_f1, source->pucch_PowerControl->deltaF_PUCCH_f1, long);
    UPDATE_MAC_IE(target->pucch_PowerControl->deltaF_PUCCH_f2, source->pucch_PowerControl->deltaF_PUCCH_f2, long);
    UPDATE_MAC_IE(target->pucch_PowerControl->deltaF_PUCCH_f3, source->pucch_PowerControl->deltaF_PUCCH_f3, long);
    UPDATE_MAC_IE(target->pucch_PowerControl->deltaF_PUCCH_f4, source->pucch_PowerControl->deltaF_PUCCH_f4, long);
    if (source->pucch_PowerControl->p0_Set)
      UPDATE_MAC_IE(target->pucch_PowerControl->p0_Set, source->pucch_PowerControl->p0_Set, struct NR_PUCCH_PowerControl__p0_Set);
    if (source->pucch_PowerControl->pathlossReferenceRSs)
      UPDATE_MAC_IE(target->pucch_PowerControl->pathlossReferenceRSs,
                    source->pucch_PowerControl->pathlossReferenceRSs,
                    struct NR_PUCCH_PowerControl__pathlossReferenceRSs);
    UPDATE_MAC_IE(target->pucch_PowerControl->twoPUCCH_PC_AdjustmentStates,
                  source->pucch_PowerControl->twoPUCCH_PC_AdjustmentStates,
                  long);
  }
}

static void handle_aperiodic_srs_type(struct NR_SRS_ResourceSet__resourceType__aperiodic *source,
                                      struct NR_SRS_ResourceSet__resourceType__aperiodic *target)
{
  target->aperiodicSRS_ResourceTrigger = source->aperiodicSRS_ResourceTrigger;
  if (source->csi_RS)
    UPDATE_MAC_IE(target->csi_RS, source->csi_RS, NR_NZP_CSI_RS_ResourceId_t);
  UPDATE_MAC_IE(target->slotOffset, source->slotOffset, long);
  if (source->ext1 && source->ext1->aperiodicSRS_ResourceTriggerList)
    UPDATE_MAC_IE(target->ext1->aperiodicSRS_ResourceTriggerList,
                  source->ext1->aperiodicSRS_ResourceTriggerList,
                  struct NR_SRS_ResourceSet__resourceType__aperiodic__ext1__aperiodicSRS_ResourceTriggerList);
}

static void setup_srsresourceset(NR_SRS_ResourceSet_t *target, NR_SRS_ResourceSet_t *source)
{
  target->srs_ResourceSetId = source->srs_ResourceSetId;
  if (source->srs_ResourceIdList)
    UPDATE_MAC_IE(target->srs_ResourceIdList, source->srs_ResourceIdList, struct NR_SRS_ResourceSet__srs_ResourceIdList);

  if (target->resourceType.present != source->resourceType.present) {
    UPDATE_MAC_NP_IE(target->resourceType, source->resourceType, struct NR_SRS_ResourceSet__resourceType);
  }
  else {
    switch (source->resourceType.present) {
      case NR_SRS_ResourceSet__resourceType_PR_aperiodic:
        handle_aperiodic_srs_type(source->resourceType.choice.aperiodic, target->resourceType.choice.aperiodic);
        break;
      case NR_SRS_ResourceSet__resourceType_PR_periodic:
        if (source->resourceType.choice.periodic->associatedCSI_RS)
          UPDATE_MAC_IE(target->resourceType.choice.periodic->associatedCSI_RS,
                        source->resourceType.choice.periodic->associatedCSI_RS,
                        NR_NZP_CSI_RS_ResourceId_t);
        break;
      case NR_SRS_ResourceSet__resourceType_PR_semi_persistent:
        if (source->resourceType.choice.semi_persistent->associatedCSI_RS)
          UPDATE_MAC_IE(target->resourceType.choice.semi_persistent->associatedCSI_RS,
                        source->resourceType.choice.semi_persistent->associatedCSI_RS,
                        NR_NZP_CSI_RS_ResourceId_t);
        break;
      default:
        break;
    }
  }
  target->usage = source->usage;
  UPDATE_MAC_IE(target->alpha, source->alpha, NR_Alpha_t);
  if (source->p0)
    UPDATE_MAC_IE(target->p0, source->p0, long);
  if (source->pathlossReferenceRS)
    UPDATE_MAC_IE(target->pathlossReferenceRS, source->pathlossReferenceRS, struct NR_PathlossReferenceRS_Config);
  UPDATE_MAC_IE(target->srs_PowerControlAdjustmentStates, source->srs_PowerControlAdjustmentStates, long);
}

static void setup_srsconfig(NR_SRS_Config_t *source, NR_SRS_Config_t *target)
{
  UPDATE_MAC_IE(target->tpc_Accumulation, source->tpc_Accumulation, long);
  // SRS-Resource
  if (source->srs_ResourceToAddModList) {
    if (!target->srs_ResourceToAddModList)
      target->srs_ResourceToAddModList = calloc(1, sizeof(*target->srs_ResourceToAddModList));
    ADDMOD_IE_FROMLIST(source->srs_ResourceToAddModList,
                       target->srs_ResourceToAddModList,
                       srs_ResourceId,
                       NR_SRS_Resource_t);
  }
  if (source->srs_ResourceToReleaseList) {
    RELEASE_IE_FROMLIST(source->srs_ResourceToReleaseList,
                        target->srs_ResourceToAddModList,
                        srs_ResourceId);
  }
  // SRS-ResourceSet
  if (source->srs_ResourceSetToAddModList) {
    if (!target->srs_ResourceSetToAddModList)
      target->srs_ResourceSetToAddModList = calloc(1, sizeof(*target->srs_ResourceSetToAddModList));
    ADDMOD_IE_FROMLIST_WFUNCTION(source->srs_ResourceSetToAddModList,
                                 target->srs_ResourceSetToAddModList,
                                 srs_ResourceSetId,
                                 NR_SRS_ResourceSet_t,
                                 setup_srsresourceset);
  }
  if (source->srs_ResourceSetToReleaseList) {
    RELEASE_IE_FROMLIST(source->srs_ResourceSetToReleaseList,
                        target->srs_ResourceSetToAddModList,
                        srs_ResourceSetId);
  }
}

static NR_UE_DL_BWP_t *get_dl_bwp_structure(NR_UE_MAC_INST_t *mac, int bwp_id, bool setup)
{
  NR_UE_DL_BWP_t *bwp = NULL;
  for (int i = 0; i < mac->dl_BWPs.count; i++) {
    if (bwp_id == mac->dl_BWPs.array[i]->bwp_id) {
      bwp = mac->dl_BWPs.array[i];
      break;
    }
  }
  if (!bwp && setup) {
    bwp = calloc(1, sizeof(*bwp));
    ASN_SEQUENCE_ADD(&mac->dl_BWPs, bwp);
  }
  if (!setup) {
    mac->sc_info.n_dl_bwp = mac->dl_BWPs.count - 1;
    mac->sc_info.dl_bw_tbslbrm = 0;
    for (int i = 0; i < mac->dl_BWPs.count; i++) {
      if (mac->dl_BWPs.array[i]->BWPSize > mac->sc_info.dl_bw_tbslbrm)
        mac->sc_info.dl_bw_tbslbrm = mac->dl_BWPs.array[i]->BWPSize;
    }
  }
  return bwp;
}

static NR_UE_UL_BWP_t *get_ul_bwp_structure(NR_UE_MAC_INST_t *mac, int bwp_id, bool setup)
{
  NR_UE_UL_BWP_t *bwp = NULL;
  for (int i = 0; i < mac->ul_BWPs.count; i++) {
    if (bwp_id == mac->ul_BWPs.array[i]->bwp_id) {
      bwp = mac->ul_BWPs.array[i];
      break;
    }
  }
  if (!bwp && setup) {
    bwp = calloc(1, sizeof(*bwp));
    ASN_SEQUENCE_ADD(&mac->ul_BWPs, bwp);
  }
  if (!setup) {
    mac->sc_info.n_ul_bwp = mac->ul_BWPs.count - 1;
    mac->sc_info.ul_bw_tbslbrm = 0;
    for (int i = 0; i < mac->ul_BWPs.count; i++) {
      if (mac->ul_BWPs.array[i]->BWPSize > mac->sc_info.ul_bw_tbslbrm)
        mac->sc_info.ul_bw_tbslbrm = mac->ul_BWPs.array[i]->BWPSize;
    }
  }
  return bwp;
}

static void configure_dedicated_BWP_dl(NR_UE_MAC_INST_t *mac, int bwp_id, NR_BWP_DownlinkDedicated_t *dl_dedicated)
{
  if (dl_dedicated) {
    NR_UE_DL_BWP_t *bwp = get_dl_bwp_structure(mac, bwp_id, true);
    bwp->bwp_id = bwp_id;
    NR_BWP_PDCCH_t *pdcch = &mac->config_BWP_PDCCH[bwp_id];
    if(dl_dedicated->pdsch_Config) {
      if (dl_dedicated->pdsch_Config->present == NR_SetupRelease_PDSCH_Config_PR_release)
        asn1cFreeStruc(asn_DEF_NR_PDSCH_Config, bwp->pdsch_Config);
      if (dl_dedicated->pdsch_Config->present == NR_SetupRelease_PDSCH_Config_PR_setup) {
        if (!bwp->pdsch_Config)
          bwp->pdsch_Config = calloc(1, sizeof(*bwp->pdsch_Config));
        setup_pdschconfig(dl_dedicated->pdsch_Config->choice.setup, bwp->pdsch_Config);
      }
    }
    if (dl_dedicated->pdcch_Config) {
      if (dl_dedicated->pdcch_Config->present == NR_SetupRelease_PDCCH_Config_PR_release) {
        for (int i = 0; pdcch->list_Coreset.count; i++)
          asn_sequence_del(&pdcch->list_Coreset, i, 1);
        for (int i = 0; pdcch->list_SS.count; i++)
          asn_sequence_del(&pdcch->list_SS, i, 1);
      }
      if (dl_dedicated->pdcch_Config->present == NR_SetupRelease_PDCCH_Config_PR_setup)
        configure_ss_coreset(pdcch, dl_dedicated->pdcch_Config->choice.setup);
    }
  }
}

static void configure_dedicated_BWP_ul(NR_UE_MAC_INST_t *mac, int bwp_id, NR_BWP_UplinkDedicated_t *ul_dedicated)
{
  if (ul_dedicated) {
    NR_UE_UL_BWP_t *bwp = get_ul_bwp_structure(mac, bwp_id, true);
    bwp->bwp_id = bwp_id;
    if(ul_dedicated->pucch_Config) {
      if (ul_dedicated->pucch_Config->present == NR_SetupRelease_PUCCH_Config_PR_release)
        asn1cFreeStruc(asn_DEF_NR_PUCCH_Config, bwp->pucch_Config);
      if (ul_dedicated->pucch_Config->present == NR_SetupRelease_PUCCH_Config_PR_setup) {
        if (!bwp->pucch_Config)
          bwp->pucch_Config = calloc(1, sizeof(*bwp->pucch_Config));
        setup_pucchconfig(ul_dedicated->pucch_Config->choice.setup, bwp->pucch_Config);
      }
    }
    if(ul_dedicated->pusch_Config) {
      if (ul_dedicated->pusch_Config->present == NR_SetupRelease_PUSCH_Config_PR_release)
        asn1cFreeStruc(asn_DEF_NR_PUSCH_Config, bwp->pusch_Config);
      if (ul_dedicated->pusch_Config->present == NR_SetupRelease_PUSCH_Config_PR_setup) {
        if (!bwp->pusch_Config)
          bwp->pusch_Config = calloc(1, sizeof(*bwp->pusch_Config));
        setup_puschconfig(ul_dedicated->pusch_Config->choice.setup, bwp->pusch_Config);
      }
    }
    if(ul_dedicated->srs_Config) {
      if (ul_dedicated->srs_Config->present == NR_SetupRelease_SRS_Config_PR_release)
        asn1cFreeStruc(asn_DEF_NR_SRS_Config, bwp->srs_Config);
      if (ul_dedicated->srs_Config->present == NR_SetupRelease_SRS_Config_PR_setup) {
        if (!bwp->srs_Config)
          bwp->srs_Config = calloc(1, sizeof(*bwp->srs_Config));
        setup_srsconfig(ul_dedicated->srs_Config->choice.setup, bwp->srs_Config);
      }
    }
    AssertFatal(!ul_dedicated->configuredGrantConfig, "configuredGrantConfig not supported\n");
  }
}

static void configure_common_BWP_dl(NR_UE_MAC_INST_t *mac, int bwp_id, NR_BWP_DownlinkCommon_t *dl_common)
{
  if (dl_common) {
    NR_UE_DL_BWP_t *bwp = get_dl_bwp_structure(mac, bwp_id, true);
    bwp->bwp_id = bwp_id;
    NR_BWP_t *dl_genericParameters = &dl_common->genericParameters;
    bwp->scs = dl_genericParameters->subcarrierSpacing;
    bwp->cyclicprefix = dl_genericParameters->cyclicPrefix;
    bwp->BWPSize = NRRIV2BW(dl_genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
    bwp->BWPStart = NRRIV2PRBOFFSET(dl_genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
    if (bwp_id == 0) {
      mac->sc_info.initial_dl_BWPSize = bwp->BWPSize;
      mac->sc_info.initial_dl_BWPStart = bwp->BWPStart;
    }
    if (dl_common->pdsch_ConfigCommon) {
      if (dl_common->pdsch_ConfigCommon->present == NR_SetupRelease_PDSCH_ConfigCommon_PR_setup)
        UPDATE_MAC_IE(bwp->tdaList_Common,
                      dl_common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList,
                      NR_PDSCH_TimeDomainResourceAllocationList_t);
      if (dl_common->pdsch_ConfigCommon->present == NR_SetupRelease_PDSCH_ConfigCommon_PR_release)
        asn1cFreeStruc(asn_DEF_NR_PDSCH_TimeDomainResourceAllocationList, bwp->tdaList_Common);
    }
    NR_BWP_PDCCH_t *pdcch = &mac->config_BWP_PDCCH[bwp_id];
    if (dl_common->pdcch_ConfigCommon) {
      if (dl_common->pdcch_ConfigCommon->present == NR_SetupRelease_PDCCH_ConfigCommon_PR_setup)
        configure_common_ss_coreset(pdcch, dl_common->pdcch_ConfigCommon->choice.setup);
      if (dl_common->pdcch_ConfigCommon->present == NR_SetupRelease_PDCCH_ConfigCommon_PR_release)
        release_common_ss_cset(pdcch);
    }
  }
}

static void configure_common_BWP_ul(NR_UE_MAC_INST_t *mac, int bwp_id, NR_BWP_UplinkCommon_t *ul_common)
{
  if (ul_common) {
    NR_UE_UL_BWP_t *bwp = get_ul_bwp_structure(mac, bwp_id, true);
    bwp->bwp_id = bwp_id;
    NR_BWP_t *ul_genericParameters = &ul_common->genericParameters;
    bwp->scs = ul_genericParameters->subcarrierSpacing;
    bwp->cyclicprefix = ul_genericParameters->cyclicPrefix;
    bwp->BWPSize = NRRIV2BW(ul_genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
    bwp->BWPStart = NRRIV2PRBOFFSET(ul_genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
    if (bwp_id == 0) {
      mac->sc_info.initial_ul_BWPSize = bwp->BWPSize;
      mac->sc_info.initial_ul_BWPStart = bwp->BWPStart;
    }
    if (ul_common->rach_ConfigCommon) {
      HANDLE_SETUPRELEASE_DIRECT(bwp->rach_ConfigCommon,
                                 ul_common->rach_ConfigCommon,
                                 NR_RACH_ConfigCommon_t,
                                 asn_DEF_NR_RACH_ConfigCommon);
    }
    if (ul_common->pucch_ConfigCommon)
      HANDLE_SETUPRELEASE_DIRECT(bwp->pucch_ConfigCommon,
                                 ul_common->pucch_ConfigCommon,
                                 NR_PUCCH_ConfigCommon_t,
                                 asn_DEF_NR_PUCCH_ConfigCommon);
    if (ul_common->pusch_ConfigCommon) {
      if (ul_common->pusch_ConfigCommon->present == NR_SetupRelease_PUSCH_ConfigCommon_PR_setup) {
        UPDATE_MAC_IE(bwp->tdaList_Common,
                      ul_common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList,
                      NR_PUSCH_TimeDomainResourceAllocationList_t);
        UPDATE_MAC_IE(bwp->msg3_DeltaPreamble, ul_common->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble, long);
      }
      if (ul_common->pusch_ConfigCommon->present == NR_SetupRelease_PUSCH_ConfigCommon_PR_release) {
        asn1cFreeStruc(asn_DEF_NR_PUSCH_TimeDomainResourceAllocationList, bwp->tdaList_Common);
        free(bwp->msg3_DeltaPreamble);
        bwp->msg3_DeltaPreamble = NULL;
      }
    }
  }
}

void nr_rrc_mac_config_req_reset(module_id_t module_id,
                                 NR_UE_MAC_reset_cause_t reset_cause)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  reset_mac_inst(mac);
  reset_ra(&mac->ra);
  release_mac_configuration(mac);
  nr_ue_init_mac(mac);

  // Sending to PHY a request to resync
  // with no target cell ID
  if (reset_cause != DETACH) {
    mac->synch_request.Mod_id = module_id;
    mac->synch_request.CC_id = 0;
    mac->synch_request.synch_req.target_Nid_cell = -1;
    mac->if_module->synch_request(&mac->synch_request);
  }
}

void nr_rrc_mac_config_req_sib1(module_id_t module_id,
                                int cc_idP,
                                NR_SI_SchedulingInfo_t *si_SchedulingInfo,
                                NR_ServingCellConfigCommonSIB_t *scc)
{
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  AssertFatal(scc, "SIB1 SCC should not be NULL\n");

  UPDATE_MAC_IE(mac->tdd_UL_DL_ConfigurationCommon, scc->tdd_UL_DL_ConfigurationCommon, NR_TDD_UL_DL_ConfigCommon_t);
  UPDATE_MAC_IE(mac->si_SchedulingInfo, si_SchedulingInfo, NR_SI_SchedulingInfo_t);

  config_common_ue_sa(mac, scc, cc_idP);
  configure_common_BWP_dl(mac,
                          0, // bwp-id
                          &scc->downlinkConfigCommon.initialDownlinkBWP);
  if (scc->uplinkConfigCommon)
    configure_common_BWP_ul(mac,
                            0, // bwp-id
                            &scc->uplinkConfigCommon->initialUplinkBWP);
  // set current BWP only if coming from non-connected state
  // otherwise it is just a periodically update of the SIB1 content
  if (mac->state < UE_CONNECTED) {
    mac->current_DL_BWP = get_dl_bwp_structure(mac, 0, false);
    AssertFatal(mac->current_DL_BWP, "Couldn't find DL-BWP0\n");
    mac->current_UL_BWP = get_ul_bwp_structure(mac, 0, false);
    AssertFatal(mac->current_UL_BWP, "Couldn't find DL-BWP0\n");
  }

  // Setup the SSB to Rach Occasions mapping according to the config
  build_ssb_to_ro_map(mac);

  if (!get_softmodem_params()->emulate_l1)
    mac->if_module->phy_config_request(&mac->phy_config);
  mac->phy_config_request_sent = true;
}

static void handle_reconfiguration_with_sync(NR_UE_MAC_INST_t *mac,
                                             int cc_idP,
                                             const NR_ReconfigurationWithSync_t *reconfigurationWithSync)
{
  mac->crnti = reconfigurationWithSync->newUE_Identity;
  LOG_I(NR_MAC, "Configuring CRNTI %x\n", mac->crnti);

  RA_config_t *ra = &mac->ra;
  if (reconfigurationWithSync->rach_ConfigDedicated) {
    AssertFatal(
        reconfigurationWithSync->rach_ConfigDedicated->present == NR_ReconfigurationWithSync__rach_ConfigDedicated_PR_uplink,
        "RACH on supplementaryUplink not supported\n");
    UPDATE_MAC_IE(ra->rach_ConfigDedicated, reconfigurationWithSync->rach_ConfigDedicated->choice.uplink, NR_RACH_ConfigDedicated_t);
  }

  if (reconfigurationWithSync->spCellConfigCommon) {
    NR_ServingCellConfigCommon_t *scc = reconfigurationWithSync->spCellConfigCommon;
    if (scc->physCellId)
      mac->physCellId = *scc->physCellId;
    mac->dmrs_TypeA_Position = scc->dmrs_TypeA_Position;
    UPDATE_MAC_IE(mac->tdd_UL_DL_ConfigurationCommon, scc->tdd_UL_DL_ConfigurationCommon, NR_TDD_UL_DL_ConfigCommon_t);
    config_common_ue(mac, scc, cc_idP);
    if (scc->downlinkConfigCommon)
      configure_common_BWP_dl(mac,
                              0, // bwp-id
                              scc->downlinkConfigCommon->initialDownlinkBWP);
    if (scc->uplinkConfigCommon)
      configure_common_BWP_ul(mac,
                              0, // bwp-id
                              scc->uplinkConfigCommon->initialUplinkBWP);
  }

  mac->state = UE_NOT_SYNC;
  ra->ra_state = RA_UE_IDLE;
  nr_ue_mac_default_configs(mac);

  if (!get_softmodem_params()->emulate_l1) {
    mac->synch_request.Mod_id = mac->ue_id;
    mac->synch_request.CC_id = cc_idP;
    mac->synch_request.synch_req.target_Nid_cell = mac->physCellId;
    mac->if_module->synch_request(&mac->synch_request);
    mac->if_module->phy_config_request(&mac->phy_config);
    mac->phy_config_request_sent = true;
  }
}

static void configure_physicalcellgroup(NR_UE_MAC_INST_t *mac,
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

static void configure_maccellgroup(NR_UE_MAC_INST_t *mac, const NR_MAC_CellGroupConfig_t *mcg)
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

static void configure_csi_resourcemapping(NR_CSI_RS_ResourceMapping_t *target, NR_CSI_RS_ResourceMapping_t *source)
{
  if (target->frequencyDomainAllocation.present != source->frequencyDomainAllocation.present) {
    UPDATE_MAC_NP_IE(target->frequencyDomainAllocation,
                     source->frequencyDomainAllocation,
                     struct NR_CSI_RS_ResourceMapping__frequencyDomainAllocation);
  }
  else {
    switch (source->frequencyDomainAllocation.present) {
      case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row1:
        target->frequencyDomainAllocation.choice.row1.size = source->frequencyDomainAllocation.choice.row1.size;
        target->frequencyDomainAllocation.choice.row1.bits_unused = source->frequencyDomainAllocation.choice.row1.bits_unused;
        if (!target->frequencyDomainAllocation.choice.row1.buf)
          target->frequencyDomainAllocation.choice.row1.buf =
              calloc(target->frequencyDomainAllocation.choice.row1.size, sizeof(uint8_t));
        for (int i = 0; i < target->frequencyDomainAllocation.choice.row1.size; i++)
          target->frequencyDomainAllocation.choice.row1.buf[i] = source->frequencyDomainAllocation.choice.row1.buf[i];
        break;
      case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row2:
        target->frequencyDomainAllocation.choice.row2.size = source->frequencyDomainAllocation.choice.row2.size;
        target->frequencyDomainAllocation.choice.row2.bits_unused = source->frequencyDomainAllocation.choice.row2.bits_unused;
        if (!target->frequencyDomainAllocation.choice.row2.buf)
          target->frequencyDomainAllocation.choice.row2.buf =
              calloc(target->frequencyDomainAllocation.choice.row2.size, sizeof(uint8_t));
        for (int i = 0; i < target->frequencyDomainAllocation.choice.row2.size; i++)
          target->frequencyDomainAllocation.choice.row2.buf[i] = source->frequencyDomainAllocation.choice.row2.buf[i];
        break;
      case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row4:
        target->frequencyDomainAllocation.choice.row4.size = source->frequencyDomainAllocation.choice.row4.size;
        target->frequencyDomainAllocation.choice.row4.bits_unused = source->frequencyDomainAllocation.choice.row4.bits_unused;
        if (!target->frequencyDomainAllocation.choice.row4.buf)
          target->frequencyDomainAllocation.choice.row4.buf =
              calloc(target->frequencyDomainAllocation.choice.row4.size, sizeof(uint8_t));
        for (int i = 0; i < target->frequencyDomainAllocation.choice.row4.size; i++)
          target->frequencyDomainAllocation.choice.row4.buf[i] = source->frequencyDomainAllocation.choice.row4.buf[i];
        break;
      case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_other:
        target->frequencyDomainAllocation.choice.other.size = source->frequencyDomainAllocation.choice.other.size;
        target->frequencyDomainAllocation.choice.other.bits_unused = source->frequencyDomainAllocation.choice.other.bits_unused;
        if (!target->frequencyDomainAllocation.choice.other.buf)
          target->frequencyDomainAllocation.choice.other.buf =
              calloc(target->frequencyDomainAllocation.choice.other.size, sizeof(uint8_t));
        for (int i = 0; i < target->frequencyDomainAllocation.choice.other.size; i++)
          target->frequencyDomainAllocation.choice.other.buf[i] = source->frequencyDomainAllocation.choice.other.buf[i];
        break;
      default:
        AssertFatal(false, "Invalid entry\n");
    }
  }
  target->nrofPorts = source->nrofPorts;
  target->firstOFDMSymbolInTimeDomain = source->firstOFDMSymbolInTimeDomain;
  UPDATE_MAC_IE(target->firstOFDMSymbolInTimeDomain2, source->firstOFDMSymbolInTimeDomain2, long);
  target->cdm_Type = source->cdm_Type;
  target->density = source->density;
  target->freqBand = source->freqBand;
}

static void configure_csirs_resource(NR_NZP_CSI_RS_Resource_t *target, NR_NZP_CSI_RS_Resource_t *source)
{
  configure_csi_resourcemapping(&target->resourceMapping, &source->resourceMapping);
  target->powerControlOffset = source->powerControlOffset;
  UPDATE_MAC_IE(target->powerControlOffsetSS, source->powerControlOffsetSS, long);
  target->scramblingID = source->scramblingID;
  if (source->periodicityAndOffset)
    UPDATE_MAC_IE(target->periodicityAndOffset, source->periodicityAndOffset, NR_CSI_ResourcePeriodicityAndOffset_t);
  if (source->qcl_InfoPeriodicCSI_RS)
    UPDATE_MAC_IE(target->qcl_InfoPeriodicCSI_RS, source->qcl_InfoPeriodicCSI_RS, NR_TCI_StateId_t);
}

static void configure_csiim_resource(NR_CSI_IM_Resource_t *target, NR_CSI_IM_Resource_t *source)
{
  if (source->csi_IM_ResourceElementPattern)
    UPDATE_MAC_IE(target->csi_IM_ResourceElementPattern,
                  source->csi_IM_ResourceElementPattern,
                  struct NR_CSI_IM_Resource__csi_IM_ResourceElementPattern);
  if (source->freqBand)
    UPDATE_MAC_IE(target->freqBand, source->freqBand, NR_CSI_FrequencyOccupation_t);
  if (source->periodicityAndOffset)
    UPDATE_MAC_IE(target->periodicityAndOffset, source->periodicityAndOffset, NR_CSI_ResourcePeriodicityAndOffset_t);
}

static void configure_csiconfig(NR_UE_ServingCell_Info_t *sc_info, struct NR_SetupRelease_CSI_MeasConfig *csi_MeasConfig_sr)
{
  switch (csi_MeasConfig_sr->present) {
    case NR_SetupRelease_CSI_MeasConfig_PR_NOTHING:
      break;
    case NR_SetupRelease_CSI_MeasConfig_PR_release :
      asn1cFreeStruc(asn_DEF_NR_CSI_MeasConfig, sc_info->csi_MeasConfig);
      break;
    case NR_SetupRelease_CSI_MeasConfig_PR_setup:
      if (!sc_info->csi_MeasConfig) { // setup
        UPDATE_MAC_IE(sc_info->csi_MeasConfig, csi_MeasConfig_sr->choice.setup, NR_CSI_MeasConfig_t);
      } else { // modification
        NR_CSI_MeasConfig_t *target = sc_info->csi_MeasConfig;
        NR_CSI_MeasConfig_t *csi_MeasConfig = csi_MeasConfig_sr->choice.setup;
        if (csi_MeasConfig->reportTriggerSize)
          UPDATE_MAC_IE(target->reportTriggerSize, csi_MeasConfig->reportTriggerSize, long);
        if (csi_MeasConfig->aperiodicTriggerStateList)
          HANDLE_SETUPRELEASE_DIRECT(sc_info->aperiodicTriggerStateList,
                                     csi_MeasConfig->aperiodicTriggerStateList,
                                     NR_CSI_AperiodicTriggerStateList_t,
                                     asn_DEF_NR_CSI_AperiodicTriggerStateList);
        if (csi_MeasConfig->semiPersistentOnPUSCH_TriggerStateList)
          HANDLE_SETUPRELEASE_IE(target->semiPersistentOnPUSCH_TriggerStateList,
                                 csi_MeasConfig->semiPersistentOnPUSCH_TriggerStateList,
                                 NR_CSI_SemiPersistentOnPUSCH_TriggerStateList_t,
                                 asn_DEF_NR_SetupRelease_CSI_SemiPersistentOnPUSCH_TriggerStateList);
        // NZP-CSI-RS-Resources
        if (csi_MeasConfig->nzp_CSI_RS_ResourceToReleaseList) {
          RELEASE_IE_FROMLIST(csi_MeasConfig->nzp_CSI_RS_ResourceToReleaseList,
                              target->nzp_CSI_RS_ResourceToAddModList,
                              nzp_CSI_RS_ResourceId);
        }
        if (csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList) {
          if (!target->nzp_CSI_RS_ResourceToAddModList)
            target->nzp_CSI_RS_ResourceToAddModList = calloc(1, sizeof(*target->nzp_CSI_RS_ResourceToAddModList));
          ADDMOD_IE_FROMLIST_WFUNCTION(csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList,
                                       target->nzp_CSI_RS_ResourceToAddModList,
                                       nzp_CSI_RS_ResourceId,
                                       NR_NZP_CSI_RS_Resource_t,
                                       configure_csirs_resource);
        }
        // NZP-CSI-RS-ResourceSets
        if (csi_MeasConfig->nzp_CSI_RS_ResourceSetToReleaseList) {
          RELEASE_IE_FROMLIST(csi_MeasConfig->nzp_CSI_RS_ResourceSetToReleaseList,
                              target->nzp_CSI_RS_ResourceSetToAddModList,
                              nzp_CSI_ResourceSetId);
        }
        if (csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList) {
          if (!target->nzp_CSI_RS_ResourceSetToAddModList)
            target->nzp_CSI_RS_ResourceSetToAddModList = calloc(1, sizeof(*target->nzp_CSI_RS_ResourceSetToAddModList));
          ADDMOD_IE_FROMLIST(csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList,
                             target->nzp_CSI_RS_ResourceSetToAddModList,
                             nzp_CSI_ResourceSetId,
                             NR_NZP_CSI_RS_ResourceSet_t);
        }
        // CSI-IM-Resource
        if (csi_MeasConfig->csi_IM_ResourceToReleaseList) {
          RELEASE_IE_FROMLIST(csi_MeasConfig->csi_IM_ResourceToReleaseList,
                              target->csi_IM_ResourceToAddModList,
                              csi_IM_ResourceId);
        }
        if (csi_MeasConfig->csi_IM_ResourceToAddModList) {
          if (!target->csi_IM_ResourceToAddModList)
            target->csi_IM_ResourceToAddModList = calloc(1, sizeof(*target->csi_IM_ResourceToAddModList));
          ADDMOD_IE_FROMLIST_WFUNCTION(csi_MeasConfig->csi_IM_ResourceToAddModList,
                                       target->csi_IM_ResourceToAddModList,
                                       csi_IM_ResourceId,
                                       NR_CSI_IM_Resource_t,
                                       configure_csiim_resource);
        }
        // CSI-IM-ResourceSets
        if (csi_MeasConfig->csi_IM_ResourceSetToReleaseList) {
          RELEASE_IE_FROMLIST(csi_MeasConfig->csi_IM_ResourceSetToReleaseList,
                              target->csi_IM_ResourceSetToAddModList,
                              csi_IM_ResourceSetId);
        }
        if (csi_MeasConfig->csi_IM_ResourceSetToAddModList) {
          if (!target->csi_IM_ResourceSetToAddModList)
            target->csi_IM_ResourceSetToAddModList = calloc(1, sizeof(*target->csi_IM_ResourceSetToAddModList));
          ADDMOD_IE_FROMLIST(csi_MeasConfig->csi_IM_ResourceSetToAddModList,
                             target->csi_IM_ResourceSetToAddModList,
                             csi_IM_ResourceSetId,
                             NR_CSI_IM_ResourceSet_t);
        }
        // CSI-SSB-ResourceSets
        if (csi_MeasConfig->csi_SSB_ResourceSetToReleaseList) {
          RELEASE_IE_FROMLIST(csi_MeasConfig->csi_SSB_ResourceSetToReleaseList,
                              target->csi_SSB_ResourceSetToAddModList,
                              csi_SSB_ResourceSetId);
        }
        if (csi_MeasConfig->csi_SSB_ResourceSetToAddModList) {
          if (!target->csi_SSB_ResourceSetToAddModList)
            target->csi_SSB_ResourceSetToAddModList = calloc(1, sizeof(*target->csi_SSB_ResourceSetToAddModList));
          ADDMOD_IE_FROMLIST(csi_MeasConfig->csi_SSB_ResourceSetToAddModList,
                             target->csi_SSB_ResourceSetToAddModList,
                             csi_SSB_ResourceSetId,
                             NR_CSI_SSB_ResourceSet_t);
        }
        // CSI-ResourceConfigs
        if (csi_MeasConfig->csi_ResourceConfigToReleaseList) {
          RELEASE_IE_FROMLIST(csi_MeasConfig->csi_ResourceConfigToReleaseList,
                              target->csi_ResourceConfigToAddModList,
                              csi_ResourceConfigId);
        }
        if (csi_MeasConfig->csi_ResourceConfigToAddModList) {
          if (!target->csi_ResourceConfigToAddModList)
            target->csi_ResourceConfigToAddModList = calloc(1, sizeof(*target->csi_ResourceConfigToAddModList));
          ADDMOD_IE_FROMLIST(csi_MeasConfig->csi_ResourceConfigToAddModList,
                             target->csi_ResourceConfigToAddModList,
                             csi_ResourceConfigId,
                             NR_CSI_ResourceConfig_t);
        }
        // CSI-ReportConfigs
        if (csi_MeasConfig->csi_ReportConfigToReleaseList) {
          RELEASE_IE_FROMLIST(csi_MeasConfig->csi_ReportConfigToReleaseList,
                              target->csi_ReportConfigToAddModList,
                              reportConfigId);
        }
        if (csi_MeasConfig->csi_ReportConfigToAddModList) {
          if (!target->csi_ReportConfigToAddModList)
            target->csi_ReportConfigToAddModList = calloc(1, sizeof(*target->csi_ReportConfigToAddModList));
          ADDMOD_IE_FROMLIST(csi_MeasConfig->csi_ReportConfigToAddModList,
                             target->csi_ReportConfigToAddModList,
                             reportConfigId,
                             NR_CSI_ReportConfig_t);
        }
      }
      break;
    default:
      AssertFatal(false, "Invalid case\n");
  }
}

static void configure_servingcell_info(NR_UE_ServingCell_Info_t *sc_info, NR_ServingCellConfig_t *scd)
{
  if (scd->csi_MeasConfig)
    configure_csiconfig(sc_info, scd->csi_MeasConfig);

  if (scd->supplementaryUplink)
    UPDATE_MAC_IE(sc_info->supplementaryUplink, scd->supplementaryUplink, NR_UplinkConfig_t);
  if (scd->crossCarrierSchedulingConfig)
    UPDATE_MAC_IE(sc_info->crossCarrierSchedulingConfig, scd->crossCarrierSchedulingConfig, NR_CrossCarrierSchedulingConfig_t);
  if (scd->pdsch_ServingCellConfig) {
    switch (scd->pdsch_ServingCellConfig->present) {
      case NR_SetupRelease_PDSCH_ServingCellConfig_PR_NOTHING:
        break;
      case NR_SetupRelease_PDSCH_ServingCellConfig_PR_release:
        // release all configurations
        if (sc_info->pdsch_CGB_Transmission)
          asn1cFreeStruc(asn_DEF_NR_PDSCH_CodeBlockGroupTransmission, sc_info->pdsch_CGB_Transmission);
        if (sc_info->xOverhead_PDSCH) {
          free(sc_info->xOverhead_PDSCH);
          sc_info->xOverhead_PDSCH = NULL;
        }
        if (sc_info->maxMIMO_Layers_PDSCH) {
          free(sc_info->maxMIMO_Layers_PDSCH);
          sc_info->maxMIMO_Layers_PDSCH = NULL;
        }
        break;
      case NR_SetupRelease_PDSCH_ServingCellConfig_PR_setup: {
        NR_PDSCH_ServingCellConfig_t *pdsch_servingcellconfig = scd->pdsch_ServingCellConfig->choice.setup;
        if (pdsch_servingcellconfig->codeBlockGroupTransmission)
          HANDLE_SETUPRELEASE_DIRECT(sc_info->pdsch_CGB_Transmission,
                                     pdsch_servingcellconfig->codeBlockGroupTransmission,
                                     NR_PDSCH_CodeBlockGroupTransmission_t,
                                     asn_DEF_NR_PDSCH_CodeBlockGroupTransmission);
        UPDATE_MAC_IE(sc_info->xOverhead_PDSCH, pdsch_servingcellconfig->xOverhead, long);
        if (pdsch_servingcellconfig->ext1 && pdsch_servingcellconfig->ext1->maxMIMO_Layers)
          UPDATE_MAC_IE(sc_info->maxMIMO_Layers_PDSCH, pdsch_servingcellconfig->ext1->maxMIMO_Layers, long);
        break;
      }
      default:
        AssertFatal(false, "Invalid case\n");
    }
  }
  if (scd->uplinkConfig && scd->uplinkConfig->pusch_ServingCellConfig) {
    switch (scd->uplinkConfig->pusch_ServingCellConfig->present) {
      case NR_SetupRelease_PUSCH_ServingCellConfig_PR_NOTHING:
        break;
      case NR_SetupRelease_PUSCH_ServingCellConfig_PR_release:
        // release all configurations
        if (sc_info->pusch_CGB_Transmission)
          asn1cFreeStruc(asn_DEF_NR_PUSCH_CodeBlockGroupTransmission, sc_info->pusch_CGB_Transmission);
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
      case NR_SetupRelease_PUSCH_ServingCellConfig_PR_setup: {
        NR_PUSCH_ServingCellConfig_t *pusch_servingcellconfig = scd->uplinkConfig->pusch_ServingCellConfig->choice.setup;
        UPDATE_MAC_IE(sc_info->rateMatching_PUSCH, pusch_servingcellconfig->rateMatching, long);
        UPDATE_MAC_IE(sc_info->xOverhead_PUSCH, pusch_servingcellconfig->xOverhead, long);
        if (pusch_servingcellconfig->ext1 && pusch_servingcellconfig->ext1->maxMIMO_Layers)
          UPDATE_MAC_IE(sc_info->maxMIMO_Layers_PUSCH, pusch_servingcellconfig->ext1->maxMIMO_Layers, long);
        if (pusch_servingcellconfig->codeBlockGroupTransmission)
          HANDLE_SETUPRELEASE_DIRECT(sc_info->pusch_CGB_Transmission,
                                     pusch_servingcellconfig->codeBlockGroupTransmission,
                                     NR_PUSCH_CodeBlockGroupTransmission_t,
                                     asn_DEF_NR_PUSCH_CodeBlockGroupTransmission);
        break;
      }
      default:
        AssertFatal(false, "Invalid case\n");
    }
  }
}

void release_dl_BWP(NR_UE_MAC_INST_t *mac, int index)
{
  NR_UE_DL_BWP_t *bwp = mac->dl_BWPs.array[index];
  int bwp_id = bwp->bwp_id;
  asn_sequence_del(&mac->dl_BWPs, index, 0);

  free(bwp->cyclicprefix);
  asn1cFreeStruc(asn_DEF_NR_PDSCH_TimeDomainResourceAllocationList, bwp->tdaList_Common);
  asn1cFreeStruc(asn_DEF_NR_PDSCH_Config, bwp->pdsch_Config);
  free(bwp);

  NR_BWP_PDCCH_t *pdcch = &mac->config_BWP_PDCCH[bwp_id];
  release_common_ss_cset(pdcch);
  for (int i = 0; pdcch->list_Coreset.count; i++)
    asn_sequence_del(&pdcch->list_Coreset, i, 1);
  for (int i = 0; pdcch->list_SS.count; i++)
    asn_sequence_del(&pdcch->list_SS, i, 1);
}

void release_ul_BWP(NR_UE_MAC_INST_t *mac, int index)
{
  NR_UE_UL_BWP_t *bwp = mac->ul_BWPs.array[index];
  asn_sequence_del(&mac->ul_BWPs, index, 0);

  free(bwp->cyclicprefix);
  asn1cFreeStruc(asn_DEF_NR_RACH_ConfigCommon, bwp->rach_ConfigCommon);
  asn1cFreeStruc(asn_DEF_NR_PUSCH_TimeDomainResourceAllocationList, bwp->tdaList_Common);
  asn1cFreeStruc(asn_DEF_NR_ConfiguredGrantConfig, bwp->configuredGrantConfig);
  asn1cFreeStruc(asn_DEF_NR_PUSCH_Config, bwp->pusch_Config);
  asn1cFreeStruc(asn_DEF_NR_PUCCH_Config, bwp->pucch_Config);
  asn1cFreeStruc(asn_DEF_NR_PUCCH_ConfigCommon, bwp->pucch_ConfigCommon);
  asn1cFreeStruc(asn_DEF_NR_SRS_Config, bwp->srs_Config);
  free(bwp->msg3_DeltaPreamble);
  bwp->msg3_DeltaPreamble = NULL;
  free(bwp);
}

static void configure_BWPs(NR_UE_MAC_INST_t *mac, NR_ServingCellConfig_t *scd)
{
  configure_dedicated_BWP_dl(mac, 0, scd->initialDownlinkBWP);
  if (scd->downlinkBWP_ToReleaseList) {
    for (int i = 0; i < scd->downlinkBWP_ToReleaseList->list.count; i++) {
      for (int j = 0; j < mac->dl_BWPs.count; j++) {
        if (*scd->downlinkBWP_ToReleaseList->list.array[i] == mac->dl_BWPs.array[i]->bwp_id)
          release_dl_BWP(mac, i);
      }
    }
  }
  if (scd->downlinkBWP_ToAddModList) {
    for (int i = 0; i < scd->downlinkBWP_ToAddModList->list.count; i++) {
      NR_BWP_Downlink_t *bwp = scd->downlinkBWP_ToAddModList->list.array[i];
      configure_common_BWP_dl(mac, bwp->bwp_Id, bwp->bwp_Common);
      configure_dedicated_BWP_dl(mac, bwp->bwp_Id, bwp->bwp_Dedicated);
    }
  }
  if (scd->firstActiveDownlinkBWP_Id) {
    mac->current_DL_BWP = get_dl_bwp_structure(mac, *scd->firstActiveDownlinkBWP_Id, false);
    AssertFatal(mac->current_DL_BWP, "Couldn't find DL-BWP %ld\n", *scd->firstActiveDownlinkBWP_Id);
  }

  if (scd->uplinkConfig) {
    configure_dedicated_BWP_ul(mac, 0, scd->uplinkConfig->initialUplinkBWP);
    if (scd->uplinkConfig->uplinkBWP_ToReleaseList) {
      for (int i = 0; i < scd->uplinkConfig->uplinkBWP_ToReleaseList->list.count; i++) {
        for (int j = 0; j < mac->ul_BWPs.count; j++) {
          if (*scd->uplinkConfig->uplinkBWP_ToReleaseList->list.array[i] == mac->ul_BWPs.array[i]->bwp_id)
            release_ul_BWP(mac, i);
        }
      }
    }
    if (scd->uplinkConfig->uplinkBWP_ToAddModList) {
      for (int i = 0; i < scd->uplinkConfig->uplinkBWP_ToAddModList->list.count; i++) {
        NR_BWP_Uplink_t *bwp = scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[i];
        configure_common_BWP_ul(mac, bwp->bwp_Id, bwp->bwp_Common);
        configure_dedicated_BWP_ul(mac, bwp->bwp_Id, bwp->bwp_Dedicated);
      }
    }
    if (scd->uplinkConfig->firstActiveUplinkBWP_Id) {
      mac->current_UL_BWP = get_ul_bwp_structure(mac, *scd->uplinkConfig->firstActiveUplinkBWP_Id, false);
      AssertFatal(mac->current_UL_BWP, "Couldn't find UL-BWP %ld\n", *scd->uplinkConfig->firstActiveUplinkBWP_Id);
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
      handle_reconfiguration_with_sync(mac, cc_idP, spCellConfig->reconfigurationWithSync);
    }
    if (scd) {
      configure_servingcell_info(&mac->sc_info, scd);
      configure_BWPs(mac, scd);
    }
  }

  configure_logicalChannelBearer(mac,
                                 cell_group_config->rlc_BearerToAddModList,
                                 cell_group_config->rlc_BearerToReleaseList);

  // Setup the SSB to Rach Occasions mapping according to the config
  // Only if RACH is configured for current BWP
  if (mac->current_UL_BWP->rach_ConfigCommon)
    build_ssb_to_ro_map(mac);

  if (!mac->dl_config_request || !mac->ul_config_request)
    ue_init_config_request(mac, mac->current_DL_BWP->scs);
}
