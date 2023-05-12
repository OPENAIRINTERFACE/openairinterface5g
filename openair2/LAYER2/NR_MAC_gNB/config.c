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
#include "common/platform_constants.h"
#include "common/ran_context.h"
#include "common/utils/nr/nr_common.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "NR_BCCH-BCH-Message.h"
#include "NR_ServingCellConfigCommon.h"
#include "uper_encoder.h"

#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "SCHED_NR/phy_frame_config_nr.h"
#include "openair1/PHY/defs_gNB.h"

#include "NR_MIB.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "../../../../nfapi/oai_integration/vendor_ext.h"
/* Softmodem params */
#include "executables/softmodem-common.h"

extern RAN_CONTEXT_t RC;
//extern int l2_init_gNB(void);
extern uint8_t nfapi_mode;

static void process_rlcBearerConfig(struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list,
                                    struct NR_CellGroupConfig__rlc_BearerToReleaseList *rlc_bearer2release_list,
                                    NR_UE_sched_ctrl_t *sched_ctrl)
{
  if (rlc_bearer2release_list) {
    for (int i = 0; i < rlc_bearer2release_list->list.count; i++) {
      for (int idx = 0; idx < sched_ctrl->dl_lc_num; idx++) {
        if (sched_ctrl->dl_lc_ids[idx] == *rlc_bearer2release_list->list.array[i]) {
          const int remaining_lcs = sched_ctrl->dl_lc_num - idx - 1;
          memmove(&sched_ctrl->dl_lc_ids[idx], &sched_ctrl->dl_lc_ids[idx + 1], sizeof(sched_ctrl->dl_lc_ids[idx]) * remaining_lcs);
          sched_ctrl->dl_lc_num--;
          break;
        }
      }
    }
  }

  if (rlc_bearer2add_list) {
    // keep lcids
    for (int i = 0; i < rlc_bearer2add_list->list.count; i++) {
      const int lcid = rlc_bearer2add_list->list.array[i]->logicalChannelIdentity;
      bool found = false;
      for (int idx = 0; idx < sched_ctrl->dl_lc_num; idx++) {
        if (sched_ctrl->dl_lc_ids[idx] == lcid) {
          found = true;
          break;
        }
      }

      if (!found) {
        sched_ctrl->dl_lc_num++;
        sched_ctrl->dl_lc_ids[sched_ctrl->dl_lc_num - 1] = lcid;
        LOG_D(NR_MAC, "Adding LCID %d (%s %d)\n", lcid, lcid < 4 ? "SRB" : "DRB", lcid);
      }
    }
  }

  LOG_D(NR_MAC, "In %s: total num of active bearers %d) \n",
      __FUNCTION__,
      sched_ctrl->dl_lc_num);

}

void process_CellGroup(NR_CellGroupConfig_t *CellGroup, NR_UE_info_t *UE)
{
  /* we assume that this function is mutex-protected from outside */
  NR_SCHED_ENSURE_LOCKED(&RC.nrmac[0]->sched_lock);

   AssertFatal(CellGroup, "CellGroup is null\n");
   NR_MAC_CellGroupConfig_t *mac_CellGroupConfig = CellGroup->mac_CellGroupConfig;

   if (mac_CellGroupConfig) {
     //process_drx_Config(sched_ctrl,mac_CellGroupConfig->drx_Config);
     //process_schedulingRequestConfig(sched_ctrl,mac_CellGroupConfig->schedulingRequestConfig);
     //process_bsrConfig(sched_ctrl,mac_CellGroupConfig->bsr_Config);
     //process_tag_Config(sched_ctrl,mac_CellGroupConfig->tag_Config);
     //process_phr_Config(sched_ctrl,mac_CellGroupConfig->phr_Config);
   }

   nr_mac_prepare_ra_ue(RC.nrmac[0], UE->rnti, CellGroup);
   process_rlcBearerConfig(CellGroup->rlc_BearerToAddModList, CellGroup->rlc_BearerToReleaseList, &UE->UE_sched_ctrl);
}

static void config_common(gNB_MAC_INST *nrmac, int pdsch_AntennaPorts, int pusch_AntennaPorts, NR_ServingCellConfigCommon_t *scc)
{
  nfapi_nr_config_request_scf_t *cfg = &nrmac->config[0];
  nrmac->common_channels[0].ServingCellConfigCommon = scc;

  // Carrier configuration
  struct NR_FrequencyInfoDL *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
  int bw_index = get_supported_band_index(frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                          *frequencyInfoDL->frequencyBandList.list.array[0],
                                          frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
  cfg->carrier_config.dl_bandwidth.value = get_supported_bw_mhz(*frequencyInfoDL->frequencyBandList.list.array[0] > 256 ? FR2 : FR1, bw_index);
  cfg->carrier_config.dl_bandwidth.tl.tag   = NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG; //temporary
  cfg->num_tlv++;
  LOG_I(NR_MAC,"%s() dl_BandwidthP:%d\n", __FUNCTION__, cfg->carrier_config.dl_bandwidth.value);

  cfg->carrier_config.dl_frequency.value = from_nrarfcn(*frequencyInfoDL->frequencyBandList.list.array[0],
                                                        *scc->ssbSubcarrierSpacing,
                                                        frequencyInfoDL->absoluteFrequencyPointA)/1000; // freq in kHz
  cfg->carrier_config.dl_frequency.tl.tag = NFAPI_NR_CONFIG_DL_FREQUENCY_TAG;
  cfg->num_tlv++;

  for (int i = 0; i < 5; i++) {
    if (i == frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.dl_grid_size[i].value = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.dl_k0[i].value = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
      cfg->carrier_config.dl_grid_size[i].tl.tag = NFAPI_NR_CONFIG_DL_GRID_SIZE_TAG;
      cfg->carrier_config.dl_k0[i].tl.tag = NFAPI_NR_CONFIG_DL_K0_TAG;
      cfg->num_tlv++;
      cfg->num_tlv++;
    }
    else {
      cfg->carrier_config.dl_grid_size[i].value = 0;
      cfg->carrier_config.dl_k0[i].value = 0;
    }
  }
  struct NR_FrequencyInfoUL *frequencyInfoUL = scc->uplinkConfigCommon->frequencyInfoUL;
  bw_index = get_supported_band_index(frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                      *frequencyInfoUL->frequencyBandList->list.array[0],
                                      frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
  cfg->carrier_config.uplink_bandwidth.value = get_supported_bw_mhz(*frequencyInfoUL->frequencyBandList->list.array[0] > 256 ? FR2 : FR1, bw_index);
  cfg->carrier_config.uplink_bandwidth.tl.tag   = NFAPI_NR_CONFIG_UPLINK_BANDWIDTH_TAG; //temporary
  cfg->num_tlv++;
  LOG_I(NR_MAC,"%s() dl_BandwidthP:%d\n", __FUNCTION__, cfg->carrier_config.uplink_bandwidth.value);

  int UL_pointA;
  if (frequencyInfoUL->absoluteFrequencyPointA == NULL)
    UL_pointA = frequencyInfoDL->absoluteFrequencyPointA;
  else
    UL_pointA = *frequencyInfoUL->absoluteFrequencyPointA;

  cfg->carrier_config.uplink_frequency.value = from_nrarfcn(*frequencyInfoUL->frequencyBandList->list.array[0],
                                                            *scc->ssbSubcarrierSpacing,
                                                            UL_pointA)/1000; // freq in kHz
  cfg->carrier_config.uplink_frequency.tl.tag = NFAPI_NR_CONFIG_UPLINK_FREQUENCY_TAG;
  cfg->num_tlv++;

  for (int i = 0; i < 5; i++) {
    if (i == frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.ul_grid_size[i].value = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.ul_k0[i].value = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
      cfg->carrier_config.ul_grid_size[i].tl.tag = NFAPI_NR_CONFIG_UL_GRID_SIZE_TAG;
      cfg->carrier_config.ul_k0[i].tl.tag = NFAPI_NR_CONFIG_UL_K0_TAG;
      cfg->num_tlv++;
      cfg->num_tlv++;
    }
    else {
      cfg->carrier_config.ul_grid_size[i].value = 0;
      cfg->carrier_config.ul_k0[i].value = 0;
    }
  }

  uint32_t band = *frequencyInfoDL->frequencyBandList.list.array[0];
  frequency_range_t frequency_range = band<100?FR1:FR2;

  frame_type_t frame_type = get_frame_type(*frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);
  nrmac->common_channels[0].frame_type = frame_type;

  // Cell configuration
  cfg->cell_config.phy_cell_id.value = *scc->physCellId;
  cfg->cell_config.phy_cell_id.tl.tag = NFAPI_NR_CONFIG_PHY_CELL_ID_TAG;
  cfg->num_tlv++;

  cfg->cell_config.frame_duplex_type.value = frame_type;
  cfg->cell_config.frame_duplex_type.tl.tag = NFAPI_NR_CONFIG_FRAME_DUPLEX_TYPE_TAG;
  cfg->num_tlv++;


  // SSB configuration
  cfg->ssb_config.ss_pbch_power.value = scc->ss_PBCH_BlockPower;
  cfg->ssb_config.ss_pbch_power.tl.tag = NFAPI_NR_CONFIG_SS_PBCH_POWER_TAG;
  cfg->num_tlv++;

  cfg->ssb_config.bch_payload.value = 1;
  cfg->ssb_config.bch_payload.tl.tag = NFAPI_NR_CONFIG_BCH_PAYLOAD_TAG;
  cfg->num_tlv++;

  cfg->ssb_config.scs_common.value = *scc->ssbSubcarrierSpacing;
  cfg->ssb_config.scs_common.tl.tag = NFAPI_NR_CONFIG_SCS_COMMON_TAG;
  cfg->num_tlv++;

  // PRACH configuration

  uint8_t nb_preambles = 64;
  NR_RACH_ConfigCommon_t *rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  if(rach_ConfigCommon->totalNumberOfRA_Preambles != NULL)
     nb_preambles = *rach_ConfigCommon->totalNumberOfRA_Preambles;

  cfg->prach_config.prach_sequence_length.value = rach_ConfigCommon->prach_RootSequenceIndex.present-1;
  cfg->prach_config.prach_sequence_length.tl.tag = NFAPI_NR_CONFIG_PRACH_SEQUENCE_LENGTH_TAG;
  cfg->num_tlv++;

  if (rach_ConfigCommon->msg1_SubcarrierSpacing)
    cfg->prach_config.prach_sub_c_spacing.value = *rach_ConfigCommon->msg1_SubcarrierSpacing;
  else
    cfg->prach_config.prach_sub_c_spacing.value = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
  cfg->prach_config.prach_sub_c_spacing.tl.tag = NFAPI_NR_CONFIG_PRACH_SUB_C_SPACING_TAG;
  cfg->num_tlv++;
  cfg->prach_config.restricted_set_config.value = rach_ConfigCommon->restrictedSetConfig;
  cfg->prach_config.restricted_set_config.tl.tag = NFAPI_NR_CONFIG_RESTRICTED_SET_CONFIG_TAG;
  cfg->num_tlv++;
  cfg->prach_config.prach_ConfigurationIndex.value = rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;
  cfg->prach_config.prach_ConfigurationIndex.tl.tag = NFAPI_NR_CONFIG_PRACH_CONFIG_INDEX_TAG;
  cfg->num_tlv++;

  switch (rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM) {
    case 0 :
      cfg->prach_config.num_prach_fd_occasions.value = 1;
      break;
    case 1 :
      cfg->prach_config.num_prach_fd_occasions.value = 2;
      break;
    case 2 :
      cfg->prach_config.num_prach_fd_occasions.value = 4;
      break;
    case 3 :
      cfg->prach_config.num_prach_fd_occasions.value = 8;
      break;
    default:
      AssertFatal(1==0,"msg1 FDM identifier %ld undefined (0,1,2,3) \n", rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM);
  } 
  cfg->prach_config.num_prach_fd_occasions.tl.tag = NFAPI_NR_CONFIG_NUM_PRACH_FD_OCCASIONS_TAG;
  cfg->num_tlv++;

  cfg->prach_config.prach_ConfigurationIndex.value =  rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;
  cfg->prach_config.prach_ConfigurationIndex.tl.tag = NFAPI_NR_CONFIG_PRACH_CONFIG_INDEX_TAG;
  cfg->num_tlv++;

  cfg->prach_config.num_prach_fd_occasions_list = (nfapi_nr_num_prach_fd_occasions_t *) malloc(cfg->prach_config.num_prach_fd_occasions.value*sizeof(nfapi_nr_num_prach_fd_occasions_t));
  for (int i=0; i<cfg->prach_config.num_prach_fd_occasions.value; i++) {
    nfapi_nr_num_prach_fd_occasions_t *prach_fd_occasion = &cfg->prach_config.num_prach_fd_occasions_list[i];
    // prach_fd_occasion->num_prach_fd_occasions = i;
    if (cfg->prach_config.prach_sequence_length.value)
      prach_fd_occasion->prach_root_sequence_index.value = rach_ConfigCommon->prach_RootSequenceIndex.choice.l139; 
    else
      prach_fd_occasion->prach_root_sequence_index.value = rach_ConfigCommon->prach_RootSequenceIndex.choice.l839;
    prach_fd_occasion->prach_root_sequence_index.tl.tag = NFAPI_NR_CONFIG_PRACH_ROOT_SEQUENCE_INDEX_TAG;
    cfg->num_tlv++;
    prach_fd_occasion->k1.value = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE) +
                                                  rach_ConfigCommon->rach_ConfigGeneric.msg1_FrequencyStart +
                                                  (get_N_RA_RB(cfg->prach_config.prach_sub_c_spacing.value, frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing ) * i);
    if (get_softmodem_params()->sa) {
      prach_fd_occasion->k1.value = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE) +
                                                    rach_ConfigCommon->rach_ConfigGeneric.msg1_FrequencyStart +
                                                    (get_N_RA_RB(cfg->prach_config.prach_sub_c_spacing.value, frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing ) * i);
    } else {
      prach_fd_occasion->k1.value = rach_ConfigCommon->rach_ConfigGeneric.msg1_FrequencyStart +
                                    (get_N_RA_RB(cfg->prach_config.prach_sub_c_spacing.value, frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing ) * i);
    }
    prach_fd_occasion->k1.tl.tag = NFAPI_NR_CONFIG_K1_TAG;
    cfg->num_tlv++;
    prach_fd_occasion->prach_zero_corr_conf.value = rach_ConfigCommon->rach_ConfigGeneric.zeroCorrelationZoneConfig;
    prach_fd_occasion->prach_zero_corr_conf.tl.tag = NFAPI_NR_CONFIG_PRACH_ZERO_CORR_CONF_TAG;
    cfg->num_tlv++;
    prach_fd_occasion->num_root_sequences.value = compute_nr_root_seq(rach_ConfigCommon,
                                                                      nb_preambles,
                                                                      frame_type,
                                                                      frequency_range);
    prach_fd_occasion->num_root_sequences.tl.tag = NFAPI_NR_CONFIG_NUM_ROOT_SEQUENCES_TAG;
    cfg->num_tlv++;
    prach_fd_occasion->num_unused_root_sequences.value = 1;
  }

  cfg->prach_config.ssb_per_rach.value = rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present-1;
  cfg->prach_config.ssb_per_rach.tl.tag = NFAPI_NR_CONFIG_SSB_PER_RACH_TAG;
  cfg->num_tlv++;

  // SSB Table Configuration
  
  cfg->ssb_table.ssb_offset_point_a.value =
      get_ssb_offset_to_pointA(*scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB,
                               scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,
                               *scc->ssbSubcarrierSpacing,
                               frequency_range);
  cfg->ssb_table.ssb_offset_point_a.tl.tag = NFAPI_NR_CONFIG_SSB_OFFSET_POINT_A_TAG;
  cfg->num_tlv++;
  cfg->ssb_table.ssb_period.value = *scc->ssb_periodicityServingCell;
  cfg->ssb_table.ssb_period.tl.tag = NFAPI_NR_CONFIG_SSB_PERIOD_TAG;
  cfg->num_tlv++;
  cfg->ssb_table.ssb_subcarrier_offset.value =
      get_ssb_subcarrier_offset(*scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB,
                                scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA);
  cfg->ssb_table.ssb_subcarrier_offset.tl.tag = NFAPI_NR_CONFIG_SSB_SUBCARRIER_OFFSET_TAG;
  cfg->num_tlv++;

  nrmac->ssb_SubcarrierOffset = cfg->ssb_table.ssb_subcarrier_offset.value;
  nrmac->ssb_OffsetPointA = cfg->ssb_table.ssb_offset_point_a.value;
  LOG_I(NR_MAC,
        "ssb_OffsetPointA %d, ssb_SubcarrierOffset %d\n",
        cfg->ssb_table.ssb_offset_point_a.value,
        cfg->ssb_table.ssb_subcarrier_offset.value);

  switch (scc->ssb_PositionsInBurst->present) {
    case 1 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask.value = scc->ssb_PositionsInBurst->choice.shortBitmap.buf[0]<<24;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask.value = 0;
      break;
    case 2 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask.value = ((uint32_t) scc->ssb_PositionsInBurst->choice.mediumBitmap.buf[0]) << 24;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask.value = 0;
      break;
    case 3 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask.value = 0;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask.value = 0;
      for (int i=0; i<4; i++) {
        cfg->ssb_table.ssb_mask_list[0].ssb_mask.value += (scc->ssb_PositionsInBurst->choice.longBitmap.buf[3-i]<<i*8);
        cfg->ssb_table.ssb_mask_list[1].ssb_mask.value += (scc->ssb_PositionsInBurst->choice.longBitmap.buf[7-i]<<i*8);
      }
      break;
    default:
      AssertFatal(1==0,"SSB bitmap size value %d undefined (allowed values 1,2,3) \n", scc->ssb_PositionsInBurst->present);
  }

  cfg->ssb_table.ssb_mask_list[0].ssb_mask.tl.tag = NFAPI_NR_CONFIG_SSB_MASK_TAG;
  cfg->ssb_table.ssb_mask_list[1].ssb_mask.tl.tag = NFAPI_NR_CONFIG_SSB_MASK_TAG;
  cfg->num_tlv+=2;

  // logical antenna ports
  cfg->carrier_config.num_tx_ant.value = pdsch_AntennaPorts;
  AssertFatal(pdsch_AntennaPorts > 0 && pdsch_AntennaPorts < 33, "pdsch_AntennaPorts in 1...32\n");
  cfg->carrier_config.num_tx_ant.tl.tag = NFAPI_NR_CONFIG_NUM_TX_ANT_TAG;

  int num_ssb=0;
  for (int i=0;i<32;i++) {
    cfg->ssb_table.ssb_beam_id_list[i].beam_id.tl.tag = NFAPI_NR_CONFIG_BEAM_ID_TAG;
    if ((cfg->ssb_table.ssb_mask_list[0].ssb_mask.value>>(31-i))&1) {
      cfg->ssb_table.ssb_beam_id_list[i].beam_id.value = num_ssb;
      num_ssb++;
    }
    cfg->num_tlv++;
  }
  for (int i=0;i<32;i++) {
    cfg->ssb_table.ssb_beam_id_list[32+i].beam_id.tl.tag = NFAPI_NR_CONFIG_BEAM_ID_TAG;
    if ((cfg->ssb_table.ssb_mask_list[1].ssb_mask.value>>(31-i))&1) {
      cfg->ssb_table.ssb_beam_id_list[32+i].beam_id.value = num_ssb;
      num_ssb++;
    }
    cfg->num_tlv++;
  } 

  cfg->carrier_config.num_rx_ant.value = pusch_AntennaPorts;
  AssertFatal(pusch_AntennaPorts > 0 && pusch_AntennaPorts < 13, "pusch_AntennaPorts in 1...12\n");
  cfg->carrier_config.num_rx_ant.tl.tag = NFAPI_NR_CONFIG_NUM_RX_ANT_TAG;
  LOG_I(NR_MAC,"Set RX antenna number to %d, Set TX antenna number to %d (num ssb %d: %x,%x)\n",
        cfg->carrier_config.num_tx_ant.value,cfg->carrier_config.num_rx_ant.value,num_ssb,cfg->ssb_table.ssb_mask_list[0].ssb_mask.value,cfg->ssb_table.ssb_mask_list[1].ssb_mask.value);
  AssertFatal(cfg->carrier_config.num_tx_ant.value > 0,"carrier_config.num_tx_ant.value %d !\n",cfg->carrier_config.num_tx_ant.value );
  cfg->num_tlv++;
  cfg->num_tlv++;

  // TDD Table Configuration
  if (cfg->cell_config.frame_duplex_type.value == TDD){
    cfg->tdd_table.tdd_period.tl.tag = NFAPI_NR_CONFIG_TDD_PERIOD_TAG;
    cfg->num_tlv++;
    if (scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1 == NULL) {
      cfg->tdd_table.tdd_period.value = scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity;
    } else {
      AssertFatal(scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530 != NULL,
                  "In %s: scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530 is null\n", __FUNCTION__);
      cfg->tdd_table.tdd_period.value = *scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530;
    }
    LOG_I(NR_MAC, "Setting TDD configuration period to %d\n", cfg->tdd_table.tdd_period.value);
    int periods_per_frame = set_tdd_config_nr(cfg,
                                              frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                              scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots,
                                              scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols,
                                              scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots,
                                              scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols);

    if (periods_per_frame < 0)
      LOG_E(NR_MAC,"TDD configuration can not be done\n");
    else {
      LOG_I(NR_MAC,"TDD has been properly configurated\n");
      nrmac->tdd_beam_association = (int16_t *)malloc16(periods_per_frame*sizeof(int16_t));
    }
  }
}

int nr_mac_enable_ue_rrc_processing_timer(module_id_t Mod_idP, rnti_t rnti, NR_SubcarrierSpacing_t subcarrierSpacing, uint32_t rrc_reconfiguration_delay)
{
  if (rrc_reconfiguration_delay == 0) {
    return -1;
  }

  gNB_MAC_INST *nrmac = RC.nrmac[Mod_idP];
  NR_SCHED_LOCK(&nrmac->sched_lock);

  NR_UE_info_t *UE_info = find_nr_UE(&nrmac->UE_info,rnti);
  if (!UE_info) {
    LOG_W(NR_MAC, "Could not find UE for RNTI 0x%04x\n", rnti);
    NR_SCHED_UNLOCK(&nrmac->sched_lock);
    return -1;
  }
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl;
  const uint16_t sl_ahead = nrmac->if_inst->sl_ahead;
  sched_ctrl->rrc_processing_timer = (rrc_reconfiguration_delay<<subcarrierSpacing) + sl_ahead;
  LOG_I(NR_MAC, "Activating RRC processing timer for UE %04x with %d ms\n", UE_info->rnti, rrc_reconfiguration_delay);

  // it might happen that timing advance command should be sent during the RRC
  // processing timer. To prevent this, set a variable as if we would have just
  // sent it. This way, another TA command will for sure be sent in some
  // frames, after RRC processing timer.
  sched_ctrl->ta_frame = (nrmac->frame - 1 + 1024) % 1024;

  NR_SCHED_UNLOCK(&nrmac->sched_lock);
  return 0;
}

void nr_mac_config_scc(gNB_MAC_INST *nrmac,
                       rrc_pdsch_AntennaPorts_t pdsch_AntennaPorts,
                       int pusch_AntennaPorts,
                       int sib1_tda,
                       int minRXTXTIMEpdsch,
                       NR_ServingCellConfigCommon_t *scc)
{
  DevAssert(nrmac != NULL);
  AssertFatal(nrmac->common_channels[0].ServingCellConfigCommon == NULL, "logic error: multiple configurations of SCC\n");
  NR_SCHED_LOCK(&nrmac->sched_lock);

  DevAssert(scc != NULL);
  AssertFatal(scc->ssb_PositionsInBurst->present > 0 && scc->ssb_PositionsInBurst->present < 4,
              "SSB Bitmap type %d is not valid\n",
              scc->ssb_PositionsInBurst->present);

  int n = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
  if (*scc->ssbSubcarrierSpacing == 0)
    n <<= 1; // to have enough room for feedback possibly beyond the frame we need a larger array at 15kHz SCS
  nrmac->common_channels[0].vrb_map_UL = calloc(n * MAX_BWP_SIZE, sizeof(uint16_t));
  nrmac->vrb_map_UL_size = n;
  AssertFatal(nrmac->common_channels[0].vrb_map_UL,
              "could not allocate memory for RC.nrmac[]->common_channels[0].vrb_map_UL\n");

  LOG_I(NR_MAC, "Configuring common parameters from NR ServingCellConfig\n");

  int num_pdsch_antenna_ports = pdsch_AntennaPorts.N1 * pdsch_AntennaPorts.N2 * pdsch_AntennaPorts.XP;
  nrmac->xp_pdsch_antenna_ports = pdsch_AntennaPorts.XP;
  config_common(nrmac, num_pdsch_antenna_ports, pusch_AntennaPorts, scc);

  if (NFAPI_MODE == NFAPI_MODE_PNF || NFAPI_MODE == NFAPI_MODE_VNF) {
    // fake that the gNB is configured in nFAPI mode, which would normally be
    // done in a NR_PHY_config_req, but in this mode, there is no PHY
    RC.gNB[0]->configured = 1;
  } else {
    NR_PHY_Config_t phycfg = {.Mod_id = 0, .CC_id = 0, .cfg = &nrmac->config[0]};
    DevAssert(nrmac->if_inst->NR_PHY_config_req);
    nrmac->if_inst->NR_PHY_config_req(&phycfg);
  }

  nrmac->minRXTXTIMEpdsch = minRXTXTIMEpdsch;
  find_SSB_and_RO_available(nrmac);

  const NR_TDD_UL_DL_Pattern_t *tdd = scc->tdd_UL_DL_ConfigurationCommon ? &scc->tdd_UL_DL_ConfigurationCommon->pattern1 : NULL;

  int nr_slots_period = n;
  int nr_dl_slots = n;
  int nr_ulstart_slot = 0;
  if (tdd) {
    nr_dl_slots = tdd->nrofDownlinkSlots + (tdd->nrofDownlinkSymbols != 0);
    nr_ulstart_slot = get_first_ul_slot(tdd->nrofDownlinkSlots, tdd->nrofDownlinkSymbols, tdd->nrofUplinkSymbols);
    nr_slots_period /= get_nb_periods_per_frame(tdd->dl_UL_TransmissionPeriodicity);
  } else {
    // if TDD configuration is not present and the band is not FDD, it means it is a dynamic TDD configuration
    AssertFatal(nrmac->common_channels[0].frame_type == FDD,"Dynamic TDD not handled yet\n");
  }

  for (int slot = 0; slot < n; ++slot) {
    nrmac->dlsch_slot_bitmap[slot / 64] |= (uint64_t)((slot % nr_slots_period) < nr_dl_slots) << (slot % 64);
    nrmac->ulsch_slot_bitmap[slot / 64] |= (uint64_t)((slot % nr_slots_period) >= nr_ulstart_slot) << (slot % 64);

    LOG_I(NR_MAC,
          "slot %d DL %d UL %d\n",
          slot,
          (nrmac->dlsch_slot_bitmap[slot / 64] & ((uint64_t)1 << (slot % 64))) != 0,
          (nrmac->ulsch_slot_bitmap[slot / 64] & ((uint64_t)1 << (slot % 64))) != 0);
  }

  if (get_softmodem_params()->phy_test) {
    nrmac->pre_processor_dl = nr_preprocessor_phytest;
    nrmac->pre_processor_ul = nr_ul_preprocessor_phytest;
  } else {
    nrmac->pre_processor_dl = nr_init_fr1_dlsch_preprocessor(0);
    nrmac->pre_processor_ul = nr_init_fr1_ulsch_preprocessor(0);
  }

  if (get_softmodem_params()->sa > 0) {
    NR_COMMON_channels_t *cc = &nrmac->common_channels[0];
    nrmac->sib1_tda = sib1_tda;
    for (int n = 0; n < NR_NB_RA_PROC_MAX; n++) {
      NR_RA_t *ra = &cc->ra[n];
      ra->cfra = false;
      ra->rnti = 0;
      ra->preambles.num_preambles = MAX_NUM_NR_PRACH_PREAMBLES;
      ra->preambles.preamble_list = malloc(MAX_NUM_NR_PRACH_PREAMBLES * sizeof(*ra->preambles.preamble_list));
      for (int i = 0; i < MAX_NUM_NR_PRACH_PREAMBLES; i++)
        ra->preambles.preamble_list[i] = i;
    }
  }
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
}

void nr_mac_config_mib(gNB_MAC_INST *nrmac, NR_BCCH_BCH_Message_t *mib)
{
  DevAssert(nrmac != NULL);
  DevAssert(mib != NULL);
  NR_SCHED_LOCK(&nrmac->sched_lock);
  NR_COMMON_channels_t *cc = &nrmac->common_channels[0];

  AssertFatal(cc->mib == NULL, "logic bug: updated MIB multiple times\n");
  cc->mib = mib;
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
}

void nr_mac_config_sib1(gNB_MAC_INST *nrmac, NR_BCCH_DL_SCH_Message_t *sib1)
{
  DevAssert(nrmac != NULL);
  DevAssert(sib1 != NULL);
  NR_SCHED_LOCK(&nrmac->sched_lock);
  NR_COMMON_channels_t *cc = &nrmac->common_channels[0];

  AssertFatal(cc->sib1 == NULL, "logic bug: updated SIB1 multiple times\n");
  cc->sib1 = sib1;
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
}

bool nr_mac_add_test_ue(gNB_MAC_INST *nrmac, uint32_t rnti, NR_CellGroupConfig_t *CellGroup)
{
  DevAssert(nrmac != NULL);
  DevAssert(CellGroup != NULL);
  DevAssert(get_softmodem_params()->phy_test);
  NR_SCHED_LOCK(&nrmac->sched_lock);

  NR_UE_info_t* UE = add_new_nr_ue(nrmac, rnti, CellGroup);
  if (UE) {
    LOG_I(NR_MAC,"Force-added new UE %x with initial CellGroup\n", rnti);
    process_CellGroup(CellGroup, UE);
  } else {
    LOG_E(NR_MAC,"Error adding UE %04x\n", rnti);
  }
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
  return UE != NULL;
}

bool nr_mac_prepare_ra_ue(gNB_MAC_INST *nrmac, uint32_t rnti, NR_CellGroupConfig_t *CellGroup)
{
  DevAssert(nrmac != NULL);
  DevAssert(CellGroup != NULL);
  NR_SCHED_ENSURE_LOCKED(&nrmac->sched_lock);

  // NSA case: need to pre-configure CFRA
  const int CC_id = 0;
  NR_COMMON_channels_t *cc = &nrmac->common_channels[CC_id];
  uint8_t ra_index = 0;
  /* checking for free RA process */
  for(; ra_index < NR_NB_RA_PROC_MAX; ra_index++) {
    if((cc->ra[ra_index].state == RA_IDLE) && (!cc->ra[ra_index].cfra)) break;
  }
  if (ra_index == NR_NB_RA_PROC_MAX) {
    LOG_E(NR_MAC, "RA processes are not available for CFRA RNTI %04x\n", rnti);
    NR_SCHED_UNLOCK(&nrmac->sched_lock);
    return false;
  }
  NR_RA_t *ra = &cc->ra[ra_index];
  if (CellGroup->spCellConfig && CellGroup->spCellConfig->reconfigurationWithSync
      && CellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated != NULL) {
    if (CellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra != NULL) {
      ra->cfra = true;
      ra->rnti = rnti;
      ra->CellGroup = CellGroup;
      struct NR_CFRA *cfra = CellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra;
      uint8_t num_preamble = cfra->resources.choice.ssb->ssb_ResourceList.list.count;
      ra->preambles.num_preambles = num_preamble;
      ra->preambles.preamble_list = calloc(ra->preambles.num_preambles, sizeof(*ra->preambles.preamble_list));
      for (int i = 0; i < cc->num_active_ssb; i++) {
        for (int j = 0; j < num_preamble; j++) {
          if (cc->ssb_index[i] == cfra->resources.choice.ssb->ssb_ResourceList.list.array[j]->ssb) {
            // one dedicated preamble for each beam
            ra->preambles.preamble_list[i] = cfra->resources.choice.ssb->ssb_ResourceList.list.array[j]->ra_PreambleIndex;
            break;
          }
        }
      }
    }
  } else {
    ra->cfra = false;
    ra->rnti = 0;
    if (ra->preambles.preamble_list == NULL) {
      ra->preambles.num_preambles = MAX_NUM_NR_PRACH_PREAMBLES;
      ra->preambles.preamble_list = (uint8_t *)malloc(MAX_NUM_NR_PRACH_PREAMBLES * sizeof(uint8_t));
      for (int i = 0; i < MAX_NUM_NR_PRACH_PREAMBLES; i++)
        ra->preambles.preamble_list[i] = i;
    }
  }
  LOG_I(NR_MAC, "Added new %s process for UE RNTI %04x with initial CellGroup\n", ra->cfra ? "CFRA" : "CBRA", rnti);
  return true;
}

bool nr_mac_update_cellgroup(gNB_MAC_INST *nrmac, uint32_t rnti, NR_CellGroupConfig_t *CellGroup)
{
  DevAssert(nrmac != NULL);
  /* we assume that this function is mutex-protected from outside */
  NR_SCHED_ENSURE_LOCKED(&nrmac->sched_lock);

  DevAssert(CellGroup != NULL);

  NR_UE_info_t *UE = find_nr_UE(&nrmac->UE_info, rnti);
  AssertFatal(UE != NULL, "Can't find UE %04x for CellGroup update\n", rnti);

  /* copy CellGroup by calling asn1c encode this is a temporary hack to avoid the gNB having a pointer to RRC CellGroup structure
   * (otherwise it would be applied to early)
   * TODO remove once we have a proper implementation */
  UE->enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig, NULL, (void *)CellGroup, UE->cg_buf, 32768);

  if (UE->enc_rval.encoded == -1) {
    LOG_E(NR_MAC, "ASN1 message CellGroupConfig encoding failed (%s, %lu)!\n", UE->enc_rval.failed_type->name, UE->enc_rval.encoded);
    exit(1);
  }

  process_CellGroup(CellGroup, UE);

  return true;
}
