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
#include "common/utils/nr/nr_common.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "NR_BCCH-BCH-Message.h"
#include "NR_ServingCellConfigCommon.h"

#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "SCHED_NR/phy_frame_config_nr.h"

#include "NR_MIB.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "../../../../nfapi/oai_integration/vendor_ext.h"
/* Softmodem params */
#include "executables/softmodem-common.h"

extern RAN_CONTEXT_t RC;
//extern int l2_init_gNB(void);
extern void mac_top_init_gNB(void);
extern uint8_t nfapi_mode;

void process_rlcBearerConfig(struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list,
                             struct NR_CellGroupConfig__rlc_BearerToReleaseList *rlc_bearer2release_list,
                             NR_UE_sched_ctrl_t *sched_ctrl) {

  if (rlc_bearer2add_list)
  // keep lcids
    for (int i=0;i<rlc_bearer2add_list->list.count;i++) {
      sched_ctrl->lcid_mask |= (1<<rlc_bearer2add_list->list.array[i]->logicalChannelIdentity);
      LOG_I(NR_MAC,"Adding LCID %d (%s %d)\n",
            (int)rlc_bearer2add_list->list.array[i]->logicalChannelIdentity,
            rlc_bearer2add_list->list.array[i]->logicalChannelIdentity<4 ? "SRB" : "DRB",
            (int)rlc_bearer2add_list->list.array[i]->logicalChannelIdentity);
    }
  if (rlc_bearer2release_list)
    for (int i=0;i<rlc_bearer2release_list->list.count;i++)
      sched_ctrl->lcid_mask |= (1<<*rlc_bearer2release_list->list.array[i]);

}


void process_drx_Config(NR_UE_sched_ctrl_t *sched_ctrl,NR_SetupRelease_DRX_Config_t *drx_Config) {
 if (!drx_Config) return;
 AssertFatal(drx_Config->present != NR_SetupRelease_DRX_Config_PR_NOTHING, "Cannot have NR_SetupRelease_DRX_Config_PR_NOTHING\n");

 if (drx_Config->present == NR_SetupRelease_DRX_Config_PR_setup) {
   LOG_I(NR_MAC,"Adding DRX config\n");
 }
 else {
   LOG_I(NR_MAC,"Removing DRX config\n");
 }
}

void process_schedulingRequestConfig(NR_UE_sched_ctrl_t *sched_ctrl,NR_SchedulingRequestConfig_t *schedulingRequestConfig) {
 if (!schedulingRequestConfig) return;

   LOG_I(NR_MAC,"Adding SchedulingRequestconfig\n");
}

void process_bsrConfig(NR_UE_sched_ctrl_t *sched_ctrl,NR_BSR_Config_t *bsr_Config) {
  if (!bsr_Config) return;
  LOG_I(NR_MAC,"Adding BSR config\n");
}

void process_tag_Config(NR_UE_sched_ctrl_t *sched_ctrl,NR_TAG_Config_t *tag_Config) {
  if (!tag_Config) return;
  LOG_I(NR_MAC,"Adding TAG config\n");
}

void process_phr_Config(NR_UE_sched_ctrl_t *sched_ctrl,NR_SetupRelease_PHR_Config_t *phr_Config) {
   if (!phr_Config) return;
   AssertFatal(phr_Config->present != NR_SetupRelease_PHR_Config_PR_NOTHING, "Cannot have NR_SetupRelease_PHR_Config_PR_NOTHING\n");

   if (phr_Config->present == NR_SetupRelease_PHR_Config_PR_setup) {
     LOG_I(NR_MAC,"Adding PHR config\n");
   }
   else {
     LOG_I(NR_MAC,"Removing PHR config\n");
   }
}

void process_CellGroup(NR_CellGroupConfig_t *CellGroup, NR_UE_sched_ctrl_t *sched_ctrl) {

   AssertFatal(CellGroup, "CellGroup is null\n");
   NR_MAC_CellGroupConfig_t   *mac_CellGroupConfig = CellGroup->mac_CellGroupConfig;


   if (mac_CellGroupConfig) {
     process_drx_Config(sched_ctrl,mac_CellGroupConfig->drx_Config);
     process_schedulingRequestConfig(sched_ctrl,mac_CellGroupConfig->schedulingRequestConfig);
     process_bsrConfig(sched_ctrl,mac_CellGroupConfig->bsr_Config);
     process_tag_Config(sched_ctrl,mac_CellGroupConfig->tag_Config);
     process_phr_Config(sched_ctrl,mac_CellGroupConfig->phr_Config);
   }
   else {
     // apply defaults

   }

   process_rlcBearerConfig(CellGroup->rlc_BearerToAddModList,CellGroup->rlc_BearerToReleaseList,sched_ctrl);

}
void config_common(int Mod_idP, int ssb_SubcarrierOffset, int pdsch_AntennaPorts, int pusch_AntennaPorts, NR_ServingCellConfigCommon_t *scc) {

  nfapi_nr_config_request_scf_t *cfg = &RC.nrmac[Mod_idP]->config[0];
  RC.nrmac[Mod_idP]->common_channels[0].ServingCellConfigCommon = scc;
  int i;

  // Carrier configuration

  cfg->carrier_config.dl_bandwidth.value = config_bandwidth(scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                                            scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,
                                                            *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0]);
  cfg->carrier_config.dl_bandwidth.tl.tag   = NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG; //temporary
  cfg->num_tlv++;
  LOG_I(NR_MAC,"%s() dl_BandwidthP:%d\n", __FUNCTION__, cfg->carrier_config.dl_bandwidth.value);

  cfg->carrier_config.dl_frequency.value = from_nrarfcn(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0],
                                                        *scc->ssbSubcarrierSpacing,
                                                        scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA)/1000; // freq in kHz
  cfg->carrier_config.dl_frequency.tl.tag = NFAPI_NR_CONFIG_DL_FREQUENCY_TAG;
  cfg->num_tlv++;

  for (i=0; i<5; i++) {
    if (i==scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.dl_grid_size[i].value = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.dl_k0[i].value = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
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

  cfg->carrier_config.uplink_bandwidth.value = config_bandwidth(scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                                                scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,
                                                                *scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list.array[0]);
  cfg->carrier_config.uplink_bandwidth.tl.tag   = NFAPI_NR_CONFIG_UPLINK_BANDWIDTH_TAG; //temporary
  cfg->num_tlv++;
  LOG_I(NR_MAC,"%s() dl_BandwidthP:%d\n", __FUNCTION__, cfg->carrier_config.uplink_bandwidth.value);

  int UL_pointA;
  if (scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA == NULL)
    UL_pointA = scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA;
  else
    UL_pointA = *scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA; 

  cfg->carrier_config.uplink_frequency.value = from_nrarfcn(*scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list.array[0],
                                                            *scc->ssbSubcarrierSpacing,
                                                            UL_pointA)/1000; // freq in kHz
  cfg->carrier_config.uplink_frequency.tl.tag = NFAPI_NR_CONFIG_UPLINK_FREQUENCY_TAG;
  cfg->num_tlv++;

  for (i=0; i<5; i++) {
    if (i==scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.ul_grid_size[i].value = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.ul_k0[i].value = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
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

  uint32_t band = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
  frequency_range_t frequency_range = band<100?FR1:FR2;

  lte_frame_type_t frame_type = get_frame_type(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);
  RC.nrmac[Mod_idP]->common_channels[0].frame_type = frame_type;

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
  if(scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles != NULL)
     nb_preambles = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles;

  cfg->prach_config.prach_sequence_length.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present-1;
  cfg->prach_config.prach_sequence_length.tl.tag = NFAPI_NR_CONFIG_PRACH_SEQUENCE_LENGTH_TAG;
  cfg->num_tlv++;  

  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing)
    cfg->prach_config.prach_sub_c_spacing.value = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing;
  else 
    cfg->prach_config.prach_sub_c_spacing.value = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
  cfg->prach_config.prach_sub_c_spacing.tl.tag = NFAPI_NR_CONFIG_PRACH_SUB_C_SPACING_TAG;
  cfg->num_tlv++;
  cfg->prach_config.restricted_set_config.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig;
  cfg->prach_config.restricted_set_config.tl.tag = NFAPI_NR_CONFIG_RESTRICTED_SET_CONFIG_TAG;
  cfg->num_tlv++;
  cfg->prach_config.prach_ConfigurationIndex.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex;
  cfg->prach_config.prach_ConfigurationIndex.tl.tag = NFAPI_NR_CONFIG_PRACH_CONFIG_INDEX_TAG;
  cfg->num_tlv++;

  switch (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM) {
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
      AssertFatal(1==0,"msg1 FDM identifier %ld undefined (0,1,2,3) \n", scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM);
  } 
  cfg->prach_config.num_prach_fd_occasions.tl.tag = NFAPI_NR_CONFIG_NUM_PRACH_FD_OCCASIONS_TAG;
  cfg->num_tlv++;

  cfg->prach_config.prach_ConfigurationIndex.value =  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex;
  cfg->prach_config.prach_ConfigurationIndex.tl.tag = NFAPI_NR_CONFIG_PRACH_CONFIG_INDEX_TAG;
  cfg->num_tlv++;

  cfg->prach_config.num_prach_fd_occasions_list = (nfapi_nr_num_prach_fd_occasions_t *) malloc(cfg->prach_config.num_prach_fd_occasions.value*sizeof(nfapi_nr_num_prach_fd_occasions_t));
  for (i=0; i<cfg->prach_config.num_prach_fd_occasions.value; i++) {
//    cfg->prach_config.num_prach_fd_occasions_list[i].num_prach_fd_occasions = i;
    if (cfg->prach_config.prach_sequence_length.value)
      cfg->prach_config.num_prach_fd_occasions_list[i].prach_root_sequence_index.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l139; 
    else
      cfg->prach_config.num_prach_fd_occasions_list[i].prach_root_sequence_index.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l839;
    cfg->prach_config.num_prach_fd_occasions_list[i].prach_root_sequence_index.tl.tag = NFAPI_NR_CONFIG_PRACH_ROOT_SEQUENCE_INDEX_TAG;
    cfg->num_tlv++;
    cfg->prach_config.num_prach_fd_occasions_list[i].k1.value = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE) + scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart + (get_N_RA_RB( cfg->prach_config.prach_sub_c_spacing.value, scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing ) * i);
    if (get_softmodem_params()->sa) {
      cfg->prach_config.num_prach_fd_occasions_list[i].k1.value = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE) + scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart + (get_N_RA_RB( cfg->prach_config.prach_sub_c_spacing.value, scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing ) * i);
    } else {
      cfg->prach_config.num_prach_fd_occasions_list[i].k1.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart + (get_N_RA_RB( cfg->prach_config.prach_sub_c_spacing.value, scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing ) * i);
    }
    cfg->prach_config.num_prach_fd_occasions_list[i].k1.tl.tag = NFAPI_NR_CONFIG_K1_TAG;
    cfg->num_tlv++;
    cfg->prach_config.num_prach_fd_occasions_list[i].prach_zero_corr_conf.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig;
    cfg->prach_config.num_prach_fd_occasions_list[i].prach_zero_corr_conf.tl.tag = NFAPI_NR_CONFIG_PRACH_ZERO_CORR_CONF_TAG;
    cfg->num_tlv++;
    cfg->prach_config.num_prach_fd_occasions_list[i].num_root_sequences.value = compute_nr_root_seq(scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup,nb_preambles, frame_type, frequency_range);
    cfg->prach_config.num_prach_fd_occasions_list[i].num_root_sequences.tl.tag = NFAPI_NR_CONFIG_NUM_ROOT_SEQUENCES_TAG;
    cfg->num_tlv++;
    cfg->prach_config.num_prach_fd_occasions_list[i].num_unused_root_sequences.value = 1;
  }

  cfg->prach_config.ssb_per_rach.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present-1;
  cfg->prach_config.ssb_per_rach.tl.tag = NFAPI_NR_CONFIG_SSB_PER_RACH_TAG;
  cfg->num_tlv++;

  // SSB Table Configuration
  int scs_scaling = 1<<(cfg->ssb_config.scs_common.value);
  if (scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA < 600000)
    scs_scaling = scs_scaling*3;
  if (scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA > 2016666)
    scs_scaling = scs_scaling>>2;
  uint32_t absolute_diff = (*scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB - scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA);
  uint16_t sco = absolute_diff%(12*scs_scaling);
  // values of subcarrier offset larger than the limit only indicates CORESET for Type0-PDCCH CSS set is not present
  uint8_t ssb_SubcarrierOffset_limit = 0;
  if(frequency_range == FR1) {
    ssb_SubcarrierOffset_limit = 24;
  } else {
    ssb_SubcarrierOffset_limit = 12;
  }
  if (ssb_SubcarrierOffset<ssb_SubcarrierOffset_limit)
    AssertFatal(sco==(scs_scaling * ssb_SubcarrierOffset),"absoluteFrequencySSB has a subcarrier offset of %d while it should be %d\n",sco/scs_scaling,ssb_SubcarrierOffset);

  cfg->ssb_table.ssb_offset_point_a.value = absolute_diff/(12*scs_scaling) - 10; //absoluteFrequencySSB is the central frequency of SSB which is made by 20RBs in total
  cfg->ssb_table.ssb_offset_point_a.tl.tag = NFAPI_NR_CONFIG_SSB_OFFSET_POINT_A_TAG;
  cfg->num_tlv++;
  cfg->ssb_table.ssb_period.value = *scc->ssb_periodicityServingCell;
  cfg->ssb_table.ssb_period.tl.tag = NFAPI_NR_CONFIG_SSB_PERIOD_TAG;
  cfg->num_tlv++;
  cfg->ssb_table.ssb_subcarrier_offset.value = ssb_SubcarrierOffset;
  cfg->ssb_table.ssb_subcarrier_offset.tl.tag = NFAPI_NR_CONFIG_SSB_SUBCARRIER_OFFSET_TAG;
  cfg->num_tlv++;

  switch (scc->ssb_PositionsInBurst->present) {
    case 1 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask.value = scc->ssb_PositionsInBurst->choice.shortBitmap.buf[0]<<24;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask.value = 0;
      break;
    case 2 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask.value = scc->ssb_PositionsInBurst->choice.mediumBitmap.buf[0]<<24;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask.value = 0;
      break;
    case 3 :
      cfg->ssb_table.ssb_mask_list[0].ssb_mask.value = 0;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask.value = 0;
      for (i=0; i<4; i++) {
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

  cfg->carrier_config.num_tx_ant.value = pdsch_AntennaPorts;
  AssertFatal(pdsch_AntennaPorts > 0 && pdsch_AntennaPorts < 13, "pdsch_AntennaPorts in 1...12\n");
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
  LOG_I(NR_MAC,"Set TX/RX antenna number to %d (num ssb %d: %x,%x)\n",cfg->carrier_config.num_tx_ant.value,num_ssb,cfg->ssb_table.ssb_mask_list[0].ssb_mask.value,cfg->ssb_table.ssb_mask_list[1].ssb_mask.value);
  AssertFatal(cfg->carrier_config.num_tx_ant.value > 0,"carrier_config.num_tx_ant.value %d !\n",cfg->carrier_config.num_tx_ant.value );
  cfg->num_tlv++;
  cfg->num_tlv++;

  // TDD Table Configuration
  //cfg->tdd_table.tdd_period.value = scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity;
  cfg->tdd_table.tdd_period.tl.tag = NFAPI_NR_CONFIG_TDD_PERIOD_TAG;
  cfg->num_tlv++;
  if (scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1 == NULL)
    cfg->tdd_table.tdd_period.value = scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity;
  else {
    AssertFatal(scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530 != NULL,
		"scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530 is null\n");
    cfg->tdd_table.tdd_period.value = *scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530;
  }
  if(cfg->cell_config.frame_duplex_type.value == TDD){
    LOG_I(NR_MAC,"Setting TDD configuration period to %d\n",cfg->tdd_table.tdd_period.value);
    int periods_per_frame = set_tdd_config_nr(cfg,
                                              scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                              scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots,
                                              scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols,
                                              scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots,
                                              scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols);

    if (periods_per_frame < 0)
      LOG_E(NR_MAC,"TDD configuration can not be done\n");
    else {
      LOG_I(NR_MAC,"TDD has been properly configurated\n");
      RC.nrmac[Mod_idP]->tdd_beam_association = (int16_t *)malloc16(periods_per_frame*sizeof(int16_t));
    }
  }

}

extern uint16_t sl_ahead;
int rrc_mac_config_req_gNB(module_id_t Mod_idP,
                           int ssb_SubcarrierOffset,
                           int pdsch_AntennaPorts,
                           int pusch_AntennaPorts,
                           int sib1_tda,
                           NR_ServingCellConfigCommon_t *scc,
                           NR_BCCH_BCH_Message_t *mib,
	                   int add_ue,
                           uint32_t rnti,
	                   NR_CellGroupConfig_t *CellGroup) {

  if (scc != NULL ) {
    AssertFatal((scc->ssb_PositionsInBurst->present > 0) && (scc->ssb_PositionsInBurst->present < 4), "SSB Bitmap type %d is not valid\n",scc->ssb_PositionsInBurst->present);

    /* dimension UL_tti_req_ahead for number of slots in frame */
    const uint8_t slots_per_frame[5] = {10, 20, 40, 80, 160};
    const int n = slots_per_frame[*scc->ssbSubcarrierSpacing];
    RC.nrmac[Mod_idP]->UL_tti_req_ahead[0] = calloc(n, sizeof(nfapi_nr_ul_tti_request_t));
    AssertFatal(RC.nrmac[Mod_idP]->UL_tti_req_ahead[0],
                "could not allocate memory for RC.nrmac[]->UL_tti_req_ahead[]\n");
    /* fill in slot/frame numbers: slot is fixed, frame will be updated by
     * scheduler */
    for (int i = 0; i < n; ++i) {
      nfapi_nr_ul_tti_request_t *req = &RC.nrmac[Mod_idP]->UL_tti_req_ahead[0][i];
      /* consider that scheduler runs sl_ahead: the first sl_ahead slots are
       * already "in the past" and thus we put frame 1 instead of 0!  Note that
       * variable sl_ahead seems to not be correctly initialized, but I leave
       * it for information purposes here (the fix would always put 0, what
       * happens now, too) */
      req->SFN = i < sl_ahead;
      req->Slot = i;
    }

    RC.nrmac[Mod_idP]->common_channels[0].vrb_map_UL =
        calloc(n * MAX_BWP_SIZE, sizeof(uint16_t));
    AssertFatal(RC.nrmac[Mod_idP]->common_channels[0].vrb_map_UL,
                "could not allocate memory for RC.nrmac[]->common_channels[0].vrb_map_UL\n");

    LOG_I(NR_MAC,"Configuring common parameters from NR ServingCellConfig\n");

    config_common(Mod_idP,
                  ssb_SubcarrierOffset,
                  pdsch_AntennaPorts,
                  pusch_AntennaPorts,
		  scc);
    LOG_D(NR_MAC, "%s() %s:%d RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req:%p\n", __FUNCTION__, __FILE__, __LINE__, RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req);
  
    // if in nFAPI mode 
    if ( (NFAPI_MODE == NFAPI_MODE_PNF || NFAPI_MODE == NFAPI_MODE_VNF) && (RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req == NULL) ){
      while(RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req == NULL) {
        // DJP AssertFatal(RC.nrmac[Mod_idP]->if_inst->PHY_config_req != NULL,"if_inst->phy_config_request is null\n");
        usleep(100 * 1000);
        printf("Waiting for PHY_config_req\n");
      }
    }
    RC.nrmac[Mod_idP]->ssb_SubcarrierOffset = ssb_SubcarrierOffset;

    NR_PHY_Config_t phycfg;
    phycfg.Mod_id = Mod_idP;
    phycfg.CC_id  = 0;
    phycfg.cfg    = &RC.nrmac[Mod_idP]->config[0];

    if (RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req) RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req(&phycfg);

    find_SSB_and_RO_available(Mod_idP);

    const NR_TDD_UL_DL_Pattern_t *tdd = &scc->tdd_UL_DL_ConfigurationCommon->pattern1;
    const int nr_mix_slots = tdd->nrofDownlinkSymbols != 0 || tdd->nrofUplinkSymbols != 0;
    const int nr_slots_period = tdd->nrofDownlinkSlots + tdd->nrofUplinkSlots + nr_mix_slots;
    const int nr_dlmix_slots = tdd->nrofDownlinkSlots + (tdd->nrofDownlinkSymbols != 0);
    const int nr_ulstart_slot = tdd->nrofDownlinkSlots + (tdd->nrofUplinkSymbols == 0);

    for (int slot = 0; slot < n; ++slot) {
      /* FIXME: it seems there is a problem with slot 0/10/slots right after UL:
       * we just get retransmissions. Thus, do not schedule such slots in DL */
      if (slot % nr_slots_period != 0)
        RC.nrmac[Mod_idP]->dlsch_slot_bitmap[slot / 64] |= (uint64_t)((slot % nr_slots_period) < nr_dlmix_slots) << (slot % 64);
      RC.nrmac[Mod_idP]->ulsch_slot_bitmap[slot / 64] |= (uint64_t)((slot % nr_slots_period) >= nr_ulstart_slot) << (slot % 64);
      LOG_D(NR_MAC, "slot %d DL %d UL %d\n",
            slot,
            (RC.nrmac[Mod_idP]->dlsch_slot_bitmap[slot / 64] & ((uint64_t)1 << (slot % 64))) != 0,
            (RC.nrmac[Mod_idP]->ulsch_slot_bitmap[slot / 64] & ((uint64_t)1 << (slot % 64))) != 0);
    }

    if (get_softmodem_params()->phy_test) {
      RC.nrmac[Mod_idP]->pre_processor_dl = nr_preprocessor_phytest;
      RC.nrmac[Mod_idP]->pre_processor_ul = nr_ul_preprocessor_phytest;
    } else {
      RC.nrmac[Mod_idP]->pre_processor_dl = nr_init_fr1_dlsch_preprocessor(Mod_idP, 0);
      RC.nrmac[Mod_idP]->pre_processor_ul = nr_init_fr1_ulsch_preprocessor(Mod_idP, 0);
    }

    if (get_softmodem_params()->sa > 0) {
      NR_COMMON_channels_t *cc = &RC.nrmac[Mod_idP]->common_channels[0];
      RC.nrmac[Mod_idP]->sib1_tda = sib1_tda;
      for (int n=0;n<NR_NB_RA_PROC_MAX;n++ ) {
	       cc->ra[n].cfra = false;
	       cc->ra[n].rnti = 0;
	       cc->ra[n].preambles.num_preambles = MAX_NUM_NR_PRACH_PREAMBLES;
	       cc->ra[n].preambles.preamble_list = (uint8_t *) malloc(MAX_NUM_NR_PRACH_PREAMBLES*sizeof(uint8_t));
	       for (int i = 0; i < MAX_NUM_NR_PRACH_PREAMBLES; i++)
	          cc->ra[n].preambles.preamble_list[i] = i;
      }
    }
  }
 
  if (mib) RC.nrmac[Mod_idP]->common_channels[0].mib = mib; 
 
  if (CellGroup) {

    const NR_ServingCellConfig_t *servingCellConfig = CellGroup->spCellConfig->spCellConfigDedicated;
    const struct NR_ServingCellConfig__downlinkBWP_ToAddModList *bwpList = servingCellConfig->downlinkBWP_ToAddModList;
    if(bwpList) {
      AssertFatal(bwpList->list.count > 0, "downlinkBWP_ToAddModList has no BWPs!\n");
      for (int i = 0; i < bwpList->list.count; ++i) {
        const NR_BWP_Downlink_t *bwp = bwpList->list.array[i];
        calculate_preferred_dl_tda(Mod_idP, bwp);
      }
    } else {
      calculate_preferred_dl_tda(Mod_idP, NULL);
    }

    const struct NR_UplinkConfig__uplinkBWP_ToAddModList *ubwpList = servingCellConfig->uplinkConfig->uplinkBWP_ToAddModList;
    if(ubwpList) {
      AssertFatal(ubwpList->list.count > 0, "uplinkBWP_ToAddModList no BWPs!\n");
      for (int i = 0; i < ubwpList->list.count; ++i) {
        const NR_BWP_Uplink_t *ubwp = ubwpList->list.array[i];
        calculate_preferred_ul_tda(Mod_idP, ubwp);
      }
    }

    NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
    if (add_ue == 1 && get_softmodem_params()->phy_test) {
      const int UE_id = add_new_nr_ue(Mod_idP, rnti, CellGroup);
      LOG_I(NR_MAC,"Added new UE_id %d/%x with initial CellGroup\n",UE_id,rnti);
      process_CellGroup(CellGroup,&UE_info->UE_sched_ctrl[UE_id]);
    } else if (add_ue == 1 && !get_softmodem_params()->phy_test) {
      const int CC_id = 0;
      NR_COMMON_channels_t *cc = &RC.nrmac[Mod_idP]->common_channels[CC_id];
      uint8_t ra_index = 0;
      /* checking for free RA process */
      for(; ra_index < NR_NB_RA_PROC_MAX; ra_index++) {
        if((cc->ra[ra_index].state == RA_IDLE) && (!cc->ra[ra_index].cfra)) break;
      }
      if (ra_index == NR_NB_RA_PROC_MAX) {
        LOG_E(NR_MAC, "%s() %s:%d RA processes are not available for CFRA RNTI :%x\n", __FUNCTION__, __FILE__, __LINE__, rnti);
        return -1;
      }	
      NR_RA_t *ra = &cc->ra[ra_index];
      ra->CellGroup = CellGroup;
      if (CellGroup->spCellConfig && CellGroup->spCellConfig->reconfigurationWithSync &&
	        CellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated!=NULL) {
        if (CellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra != NULL) {
          ra->cfra = true;
          ra->rnti = rnti;
          struct NR_CFRA *cfra = CellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra;
          uint8_t num_preamble = cfra->resources.choice.ssb->ssb_ResourceList.list.count;
          ra->preambles.num_preambles = num_preamble;
          ra->preambles.preamble_list = (uint8_t *) malloc(num_preamble*sizeof(uint8_t));
          for(int i=0; i<cc->num_active_ssb; i++) {
            for(int j=0; j<num_preamble; j++) {
              if (cc->ssb_index[i] == cfra->resources.choice.ssb->ssb_ResourceList.list.array[j]->ssb) {
                // one dedicated preamble for each beam
                ra->preambles.preamble_list[i] =
                    cfra->resources.choice.ssb->ssb_ResourceList.list.array[j]->ra_PreambleIndex;
                break;
              }
            }
          }
        }
      } else {
        ra->cfra = false;
        ra->rnti = 0;
        ra->preambles.num_preambles = MAX_NUM_NR_PRACH_PREAMBLES;
        ra->preambles.preamble_list = (uint8_t *) malloc(MAX_NUM_NR_PRACH_PREAMBLES*sizeof(uint8_t));
        for (int i = 0; i < MAX_NUM_NR_PRACH_PREAMBLES; i++)
          ra->preambles.preamble_list[i] = i;
      }
      LOG_I(NR_MAC,"Added new RA process for UE RNTI %04x with initial CellGroup\n", rnti);
    } else { // CellGroup has been updated
      NR_ServingCellConfigCommon_t *scc = RC.nrmac[Mod_idP]->common_channels[0].ServingCellConfigCommon;
      const int UE_id = find_nr_UE_id(Mod_idP,rnti);
      int target_ss;
      UE_info->CellGroup[UE_id] = CellGroup;
      LOG_I(NR_MAC,"Modified UE_id %d/%x with CellGroup\n",UE_id,rnti);
      process_CellGroup(CellGroup,&UE_info->UE_sched_ctrl[UE_id]);
      // update coreset/searchspace
      void *bwpd = NULL;
      NR_BWP_t *genericParameters = NULL;
      target_ss = NR_SearchSpace__searchSpaceType_PR_common;
      if ((UE_info->UE_sched_ctrl[UE_id].active_bwp)) {
        target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
        bwpd = (void*)UE_info->UE_sched_ctrl[UE_id].active_bwp->bwp_Dedicated;
        genericParameters = &UE_info->UE_sched_ctrl[UE_id].active_bwp->bwp_Common->genericParameters;
      }
      else if (CellGroup->spCellConfig &&
                 CellGroup->spCellConfig->spCellConfigDedicated &&
                 (CellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP)) {
        target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
        bwpd = (void*)CellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP;
        genericParameters = &scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters;
      }
      UE_info->UE_sched_ctrl[UE_id].search_space = get_searchspace(scc, bwpd, target_ss);
      UE_info->UE_sched_ctrl[UE_id].coreset = get_coreset(Mod_idP, scc, bwpd, UE_info->UE_sched_ctrl[UE_id].search_space, target_ss);
      UE_info->UE_sched_ctrl[UE_id].sched_pdcch = set_pdcch_structure(RC.nrmac[Mod_idP],
                                                                      UE_info->UE_sched_ctrl[UE_id].search_space,
                                                                      UE_info->UE_sched_ctrl[UE_id].coreset,
                                                                      scc,
                                                                      genericParameters,
                                                                      NULL);
      UE_info->UE_sched_ctrl[UE_id].maxL = 2;
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_OUT);
  
    
  return(0);

}// END rrc_mac_config_req_gNB
