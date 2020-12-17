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

void config_common(int Mod_idP, int pdsch_AntennaPorts, NR_ServingCellConfigCommon_t *scc) {

  nfapi_nr_config_request_scf_t *cfg = &RC.nrmac[Mod_idP]->config[0];
  RC.nrmac[Mod_idP]->common_channels[0].ServingCellConfigCommon = scc;
  int i;

  // Carrier configuration

  cfg->carrier_config.dl_bandwidth.value = config_bandwidth(scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                                            scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,
                                                            *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0]);
  cfg->carrier_config.dl_bandwidth.tl.tag   = NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG; //temporary
  cfg->num_tlv++;
  LOG_I(PHY,"%s() dl_BandwidthP:%d\n", __FUNCTION__, cfg->carrier_config.dl_bandwidth.value);

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
  LOG_I(PHY,"%s() dl_BandwidthP:%d\n", __FUNCTION__, cfg->carrier_config.uplink_bandwidth.value);

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

  lte_frame_type_t frame_type;
  get_frame_type(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing, &frame_type);
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
    cfg->prach_config.num_prach_fd_occasions_list[i].k1.value = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart + (get_N_RA_RB( cfg->prach_config.prach_sub_c_spacing.value, scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing ) * i);
//k1= msg1_FrequencyStart + 12 (no. of FDM)(RB for PRACH occasion);
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
  AssertFatal(sco==0,"absoluteFrequencySSB has a subcarrier offset of %d while it should be alligned with CRBs\n",sco);
  cfg->ssb_table.ssb_offset_point_a.value = absolute_diff/(12*scs_scaling) - 10; //absoluteFrequencySSB is the central frequency of SSB which is made by 20RBs in total
  cfg->ssb_table.ssb_offset_point_a.tl.tag = NFAPI_NR_CONFIG_SSB_OFFSET_POINT_A_TAG;
  cfg->num_tlv++;
  cfg->ssb_table.ssb_period.value = *scc->ssb_periodicityServingCell;
  cfg->ssb_table.ssb_period.tl.tag = NFAPI_NR_CONFIG_SSB_PERIOD_TAG;
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
  cfg->num_tlv++;

  cfg->carrier_config.num_tx_ant.value = pdsch_AntennaPorts;
  AssertFatal(pdsch_AntennaPorts > 0 && pdsch_AntennaPorts < 13, "pdsch_AntennaPorts in 1...12\n");
  cfg->carrier_config.num_tx_ant.tl.tag = NFAPI_NR_CONFIG_NUM_TX_ANT_TAG;
  int num_ssb=0;
  for (int i=0;i<32;i++) {
    num_ssb += (cfg->ssb_table.ssb_mask_list[0].ssb_mask.value>>i)&1;
    num_ssb += (cfg->ssb_table.ssb_mask_list[1].ssb_mask.value>>i)&1;
  } 

  cfg->carrier_config.num_rx_ant.value = cfg->carrier_config.num_tx_ant.value;
  cfg->carrier_config.num_rx_ant.tl.tag = NFAPI_NR_CONFIG_NUM_RX_ANT_TAG;
  LOG_I(MAC,"Set TX/RX antenna number to %d (num ssb %d: %x,%x)\n",cfg->carrier_config.num_tx_ant.value,num_ssb,cfg->ssb_table.ssb_mask_list[0].ssb_mask.value,cfg->ssb_table.ssb_mask_list[1].ssb_mask.value);
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
    LOG_I(MAC,"Setting TDD configuration period to %d\n",cfg->tdd_table.tdd_period.value);
    int return_tdd = set_tdd_config_nr(cfg,
		    scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                    scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots,
                    scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols,
                    scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots,
                    scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols);

    if (return_tdd != 0)
      LOG_E(MAC,"TDD configuration can not be done\n");
    else 
      LOG_I(MAC,"TDD has been properly configurated\n");    
  }

}

//! Calculating number of bits set
uint8_t number_of_bits_set (uint8_t buf,uint8_t * max_ri){
  uint8_t nb_of_bits_set = 0;
  uint8_t mask = 0xff;
  uint8_t index = 0;

  for (index=7; (buf & mask) && (index>=0)  ; index--){
    if (buf & (1<<index))
      nb_of_bits_set++;

    mask>>=1;
  }
  *max_ri = 8-index;
  return nb_of_bits_set;
}

//!TODO : smae function can be written to handle csi_resources
void update_csi_bitlen (NR_CSI_MeasConfig_t *csi_MeasConfig, NR_UE_list_t *UE_list, int UE_id, module_id_t Mod_idP) {
  uint8_t csi_report_id = 0;
  uint8_t csi_resourceidx =0;
  uint8_t csi_ssb_idx =0;
  uint16_t period, offset;
  NR_CSI_ReportPeriodicityAndOffset_PR p_and_o;
  NR_CSI_ReportConfig__reportQuantity_PR reportQuantity_type;
  NR_CSI_ResourceConfigId_t csi_ResourceConfigId;

  for (csi_report_id=0; csi_report_id < csi_MeasConfig->csi_ReportConfigToAddModList->list.count; csi_report_id++){
    csi_ResourceConfigId=csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->resourcesForChannelMeasurement;
    reportQuantity_type = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportQuantity.present;
    UE_list->csi_report_template[UE_id][csi_report_id].reportQuantity_type = reportQuantity_type;
    switch(p_and_o){
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots4:
        period = 4;
        offset = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots4;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots5:
        period = 5;
        offset = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots5;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots8:
        period = 8;
        offset = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots8;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots10:
        period = 10;
        offset = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots10;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots16:
        period = 16;
        offset = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots16;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots20:
        period = 20;
        offset = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots20;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots40:
        period = 40;
        offset = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots40;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots80:
        period = 80;
        offset = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots80;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots160:
        period = 160;
        offset = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots160;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots320:
        period = 320;
        offset = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportConfigType.choice.periodic->reportSlotConfig.choice.slots320;
        break;
    default:
      AssertFatal(1==0,"No periodicity and offset resource found in CSI report");
    }
    UE_list->csi_report_template[UE_id][csi_report_id].periodicity=period;
    UE_list->csi_report_template[UE_id][csi_report_id].offset=offset;

    for ( csi_resourceidx = 0; csi_resourceidx < csi_MeasConfig->csi_ResourceConfigToAddModList->list.count; csi_resourceidx++) {
      if ( csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[csi_resourceidx]->csi_ResourceConfigId != csi_ResourceConfigId) 
	continue;
      else {
	uint8_t nb_ssb_resources =0;
      //Finding the CSI_RS or SSB Resources
        if ( NR_CSI_ReportConfig__reportQuantity_PR_cri_RSRP == reportQuantity_type || 
			NR_CSI_ReportConfig__reportQuantity_PR_ssb_Index_RSRP == reportQuantity_type) {
 
          if (NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled ==
              csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->groupBasedBeamReporting.present ) {
	    if (NULL != csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->groupBasedBeamReporting.choice.disabled->nrofReportedRS)
              UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.nb_ssbri_cri = *(csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->groupBasedBeamReporting.choice.disabled->nrofReportedRS)+1;
            else
		/*! From Spec 38.331
		 * nrofReportedRS
		 * The number (N) of measured RS resources to be reported per report setting in a non-group-based report. N <= N_max, where N_max is either 2 or 4 depending on UE
		 * capability. FFS: The signaling mechanism for the gNB to select a subset of N beams for the UE to measure and report.  
		 * When the field is absent the UE applies the value 1
		 */
	      UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.nb_ssbri_cri= 1;
	  }else 
	     UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.nb_ssbri_cri= 2;

	  if (NR_CSI_ReportConfig__reportQuantity_PR_ssb_Index_RSRP == UE_list->csi_report_template[UE_id][csi_report_id].reportQuantity_type) {
            for ( csi_ssb_idx = 0; csi_ssb_idx < csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.count; csi_ssb_idx++) {
              if (csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[csi_ssb_idx]->csi_SSB_ResourceSetId ==
                  *(csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[csi_resourceidx]->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList->list.array[0])){  ///We can configure only one SSB resource set from spec 38.331 IE CSI-ResourceConfig
              
	        nb_ssb_resources=  csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[csi_ssb_idx]->csi_SSB_ResourceList.list.count;
	        UE_list->csi_report_template[UE_id][csi_report_id].SSB_Index_list = csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[csi_ssb_idx]->csi_SSB_ResourceList.list.array;
		UE_list->csi_report_template[UE_id][csi_report_id].CSI_Index_list = NULL;
	      }
	      break ;
	    }
	  } else /*if (NR_CSI_ReportConfig__reportQuantity_PR_cri_RSRP == UE_list->csi_report_template[UE_id][csi_report_id].reportQuantity_type)*/{
            for ( csi_ssb_idx = 0; csi_ssb_idx < csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.count; csi_ssb_idx++) {
              if (csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[csi_ssb_idx]->nzp_CSI_ResourceSetId ==
                  *(csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[csi_resourceidx]->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList->list.array[0])) {  ///For periodic and semi-persistent CSI Resource Settings, the number of CSI-RS Resource Sets configured is limited to S=1 for spec 38.212
              
	        nb_ssb_resources=  csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[csi_ssb_idx]->nzp_CSI_RS_Resources.list.count;
	        UE_list->csi_report_template[UE_id][csi_report_id].CSI_Index_list = csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[csi_ssb_idx]->nzp_CSI_RS_Resources.list.array;
		UE_list->csi_report_template[UE_id][csi_report_id].SSB_Index_list = NULL;
	      }
	      break ;
	    }
	  }

	  if (nb_ssb_resources) {
	    UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.cri_ssbri_bitlen =ceil(log2 (nb_ssb_resources));
	    UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.rsrp_bitlen = 7; //From spec 38.212 Table 6.3.1.1.2-6: CRI, SSBRI, and RSRP 
	    UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.diff_rsrp_bitlen =4; //From spec 38.212 Table 6.3.1.1.2-6: CRI, SSBRI, and RSRP
	  } else { 
	    UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.cri_ssbri_bitlen =0;
	    UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.rsrp_bitlen = 0;  
	    UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.diff_rsrp_bitlen =0; 
	  }
		 
	   LOG_I (MAC, "UCI: CSI_bit len : ssbri %d, rsrp: %d, diff_rsrp: %d",
	      UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.cri_ssbri_bitlen,
	      UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.rsrp_bitlen,
	      UE_list->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.diff_rsrp_bitlen);
        } 

        uint8_t ri_restriction;
        uint8_t ri_bitlen;
	uint8_t nb_allowed_ri;
	uint8_t max_ri;

	if (NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_PMI_CQI == reportQuantity_type ||
	    NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_LI_PMI_CQI==reportQuantity_type ||
	    NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_CQI==reportQuantity_type ||
	    NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_i1_CQI==reportQuantity_type||
	    NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_i1==reportQuantity_type){
	  
          for ( csi_ssb_idx = 0; csi_ssb_idx < csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.count; csi_ssb_idx++) {
            if (csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[csi_ssb_idx]->nzp_CSI_ResourceSetId ==
                *(csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[csi_resourceidx]->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList->list.array[0])) {  ///For periodic and semi-persistent CSI Resource Settings, the number of CSI-RS Resource Sets configured is limited to S=1 for spec 38.212
            
	      nb_ssb_resources=  csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[csi_ssb_idx]->nzp_CSI_RS_Resources.list.count;
	      UE_list->csi_report_template[UE_id][csi_report_id].CSI_Index_list = csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[csi_ssb_idx]->nzp_CSI_RS_Resources.list.array;
	      UE_list->csi_report_template[UE_id][csi_report_id].SSB_Index_list = NULL;
	    }
	    break ;
	  }
	  UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.cri_bitlen=ceil(log2 (nb_ssb_resources));

	  if (NR_CodebookConfig__codebookType__type1__subType_PR_typeI_SinglePanel==
			  csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->codebookConfig->codebookType.choice.type1->subType.present){
	    
	    switch (RC.nrrrc[Mod_idP]->carrier.pdsch_AntennaPorts) {
	      case 1:;
	        UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.ri_bitlen=0;
	        break;
	      case 2:
		/*  From Spec 38.212 
		 *  If the higher layer parameter nrofCQIsPerReport=1, nRI in Table 6.3.1.1.2-3 is the number of allowed rank indicator
		 *  values in the 4 LSBs of the higher layer parameter typeI-SinglePanel-ri-Restriction according to Subclause 5.2.2.2.1 [6,
		 *  TS 38.214]; otherwise nRI in Table 6.3.1.1.2-3 is the number of allowed rank indicator values according to Subclause
		 *  5.2.2.2.1 [6, TS 38.214].
		 *
		 *  But from Current RRC ASN structures nrofCQIsPerReport is not present. Present a dummy variable is present so using it to
		 *  calculate RI for antennas equal or more than two.
		 * */
		AssertFatal (NULL!=csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->dummy, "nrofCQIsPerReport is not present");
		 
		ri_restriction = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel->typeI_SinglePanel_ri_Restriction.buf[0];
		if (0==*(csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->dummy)){ /* Replace dummy with the nrofCQIsPerReport from the CSIreport 
		                                                                                             config when equalent ASN structure present */
		  nb_allowed_ri = number_of_bits_set((ri_restriction & 0xf0), &max_ri);
		  ri_bitlen = ceil(log2(nb_allowed_ri));
		}
		else{
		  nb_allowed_ri = number_of_bits_set(ri_restriction, &max_ri);
		  ri_bitlen = ceil(log2(nb_allowed_ri));
		}
		ri_bitlen = ri_bitlen<1?ri_bitlen:1; //from the spec 38.212 and table  6.3.1.1.2-3: RI, LI, CQI, and CRI of codebookType=typeI-SinglePanel 
	        UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.ri_bitlen=ri_bitlen;
		break;
	      case 4:
		AssertFatal (NULL!=csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->dummy, "nrofCQIsPerReport is not present");
		 
		ri_restriction = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel->typeI_SinglePanel_ri_Restriction.buf[0];
		if (0==*(csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->dummy)){ /* Replace dummy with the nrofCQIsPerReport from the CSIreport 
		                                                                                             config when equalent ASN structure present */
		  nb_allowed_ri = number_of_bits_set((ri_restriction & 0xf0), &max_ri);
		  ri_bitlen = ceil(log2(nb_allowed_ri));
		}
		else{
		  nb_allowed_ri = number_of_bits_set(ri_restriction,&max_ri);
		  ri_bitlen = ceil(log2(nb_allowed_ri));
		}
		ri_bitlen = ri_bitlen<2?ri_bitlen:2; //from the spec 38.212 and table  6.3.1.1.2-3: RI, LI, CQI, and CRI of codebookType=typeI-SinglePanel 
	        UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.ri_bitlen=ri_bitlen;
		break;
	      case 6:
	      case 8:
		AssertFatal (NULL!=csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->dummy, "nrofCQIsPerReport is not present");
		 
		ri_restriction = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel->typeI_SinglePanel_ri_Restriction.buf[0];
		if (0==*(csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->dummy)){ /* Replace dummy with the nrofCQIsPerReport from the CSIreport 
		                                                                                             config when equalent ASN structure present */
		  nb_allowed_ri = number_of_bits_set((ri_restriction & 0xf0),&max_ri);
		  ri_bitlen = ceil(log2(nb_allowed_ri));
		}
		else{
		  nb_allowed_ri = number_of_bits_set(ri_restriction, &max_ri);
		  ri_bitlen = ceil(log2(nb_allowed_ri));
		}
		//ri_bitlen = ri_bitlen<1?ri_bitlen:1; //from the spec 38.212 and table  6.3.1.1.2-3: RI, LI, CQI, and CRI of codebookType=typeI-SinglePanel 
	        UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.ri_bitlen=ri_bitlen;
		break;
	      default:
		//UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.ri_bitlen=0;
		AssertFatal(RC.nrrrc[Mod_idP]->carrier.pdsch_AntennaPorts>8,"Number of antennas %d are out of range", RC.nrrrc[Mod_idP]->carrier.pdsch_AntennaPorts);
                break;
	    }
	  }
	  UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.li_bitlen=0;
	  UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.cqi_bitlen=0;
	  UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.pmi_x1_bitlen=0;
	  UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.pmi_x2_bitlen=0;
	}
	
	if( NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_LI_PMI_CQI==reportQuantity_type ){
	  if (NR_CodebookConfig__codebookType__type1__subType_PR_typeI_SinglePanel==
			  csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->codebookConfig->codebookType.choice.type1->subType.present){
	    
	    switch (RC.nrrrc[Mod_idP]->carrier.pdsch_AntennaPorts) {
	      case 1:;
		UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.li_bitlen=0;
	        break;
	      case 2:
	      case 4:
	      case 6:
	      case 8:
		/*  From Spec 38.212 
		 *  If the higher layer parameter nrofCQIsPerReport=1, nRI in Table 6.3.1.1.2-3 is the number of allowed rank indicator
		 *  values in the 4 LSBs of the higher layer parameter typeI-SinglePanel-ri-Restriction according to Subclause 5.2.2.2.1 [6,
		 *  TS 38.214]; otherwise nRI in Table 6.3.1.1.2-3 is the number of allowed rank indicator values according to Subclause
		 *  5.2.2.2.1 [6, TS 38.214].
		 *
		 *  But from Current RRC ASN structures nrofCQIsPerReport is not present. Present a dummy variable is present so using it to
		 *  calculate RI for antennas equal or more than two.
		 * */
		 //! TODO: The bit length of LI is as follows LI = log2(RI), Need to confirm wheather we should consider maximum RI can be reported from ri_restricted
		 //        or we should consider reported RI. If we need to consider reported RI for calculating LI bit length then we need to modify the code.
	        UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.li_bitlen=ceil(log2(max_ri))<2?ceil(log2(max_ri)):2;
		break;
	      default:
		AssertFatal(RC.nrrrc[Mod_idP]->carrier.pdsch_AntennaPorts>8,"Number of antennas %d are out of range", RC.nrrrc[Mod_idP]->carrier.pdsch_AntennaPorts);
                break;
	    }
	  }
	}

	if (NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_PMI_CQI == reportQuantity_type ||
	    NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_LI_PMI_CQI==reportQuantity_type ||
	    NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_CQI==reportQuantity_type ||
	    NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_i1_CQI==reportQuantity_type){

	    
	  switch (RC.nrrrc[Mod_idP]->carrier.pdsch_AntennaPorts){
	    case 1:;
	    case 2:
	    case 4:
	    case 6:
	    case 8:
	        /*  From Spec 38.212 
		 *  If the higher layer parameter nrofCQIsPerReport=1, nRI in Table 6.3.1.1.2-3 is the number of allowed rank indicator
		 *  values in the 4 LSBs of the higher layer parameter typeI-SinglePanel-ri-Restriction according to Subclause 5.2.2.2.1 [6,
		 *  TS 38.214]; otherwise nRI in Table 6.3.1.1.2-3 is the number of allowed rank indicator values according to Subclause
		 *  5.2.2.2.1 [6, TS 38.214].
		 *
		 *  But from Current RRC ASN structures nrofCQIsPerReport is not present. Present a dummy variable is present so using it to
		 *  calculate RI for antennas equal or more than two.
		 * */
		 
	      if (max_ri > 4 && max_ri < 8){
	        if (NR_CodebookConfig__codebookType__type1__subType_PR_typeI_SinglePanel==
	 		  csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->codebookConfig->codebookType.choice.type1->subType.present){
	          if (NR_CSI_ReportConfig__reportFreqConfiguration__cqi_FormatIndicator_widebandCQI==csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportFreqConfiguration->cqi_FormatIndicator)
	  	    UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.cqi_bitlen = 8;
	          else 
		    UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.cqi_bitlen = 4;
	        }
	      }else{ //This condition will work even for type1-multipanel.
	        if (NR_CSI_ReportConfig__reportFreqConfiguration__cqi_FormatIndicator_widebandCQI==csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportFreqConfiguration->cqi_FormatIndicator)
	          UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.cqi_bitlen = 4;
	        else 
	          UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.cqi_bitlen = 2;
	      }
	      break;
	    default:
	      AssertFatal(RC.nrrrc[Mod_idP]->carrier.pdsch_AntennaPorts>8,"Number of antennas %d are out of range", RC.nrrrc[Mod_idP]->carrier.pdsch_AntennaPorts);
              break;
	  }
	  
	}

	if (NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_PMI_CQI == reportQuantity_type ||
            NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_LI_PMI_CQI==reportQuantity_type){
	  if (NR_CodebookConfig__codebookType__type1__subType_PR_typeI_SinglePanel==
	 		  csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->codebookConfig->codebookType.choice.type1->subType.present){
            switch (csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel->nrOfAntennaPorts.present){
              case NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts_PR_two:
		if (max_ri ==1)
		  UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.pmi_x1_bitlen = 2;
		else if (max_ri ==2)
		  UE_list->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.pmi_x1_bitlen = 1;
		break;
	      default:
		AssertFatal(csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel->nrOfAntennaPorts.present!=NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts_PR_two,
				"Not handled Yet %d", csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel->nrOfAntennaPorts.present);
		break;
	    }
	  }
	}
        break;
      }
    }
  }
}
#endif

extern uint16_t sl_ahead;
int rrc_mac_config_req_gNB(module_id_t Mod_idP, 
			   int ssb_SubcarrierOffset,
                           int pdsch_AntennaPorts,
                           int pusch_tgt_snrx10,
                           int pucch_tgt_snrx10,
                           NR_ServingCellConfigCommon_t *scc,
			   int add_ue,
			   uint32_t rnti,
			   NR_CellGroupConfig_t *secondaryCellGroup){

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

    for (int i = 0; i < MAX_NUM_BWP; ++i) {
      RC.nrmac[Mod_idP]->pucch_index_used[i] =
        calloc(n, sizeof(*RC.nrmac[Mod_idP]->pucch_index_used));
      AssertFatal(RC.nrmac[Mod_idP]->pucch_index_used[i],
                  "could not allocate memory for RC.nrmac[]->pucch_index_used[%d]\n",
                  i);
    }

    LOG_I(MAC,"Configuring common parameters from NR ServingCellConfig\n");

    config_common(Mod_idP,
                  pdsch_AntennaPorts, 
		  scc);
    LOG_E(MAC, "%s() %s:%d RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req:%p\n", __FUNCTION__, __FILE__, __LINE__, RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req);
  
    // if in nFAPI mode 
    if ( (NFAPI_MODE == NFAPI_MODE_PNF || NFAPI_MODE == NFAPI_MODE_VNF) && (RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req == NULL) ){
      while(RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req == NULL) {
        // DJP AssertFatal(RC.nrmac[Mod_idP]->if_inst->PHY_config_req != NULL,"if_inst->phy_config_request is null\n");
        usleep(100 * 1000);
        printf("Waiting for PHY_config_req\n");
      }
    }
  
    RC.nrmac[Mod_idP]->pusch_target_snrx10 = pusch_tgt_snrx10;
    RC.nrmac[Mod_idP]->pucch_target_snrx10 = pucch_tgt_snrx10;
    NR_PHY_Config_t phycfg;
    phycfg.Mod_id = Mod_idP;
    phycfg.CC_id  = 0;
    phycfg.cfg    = &RC.nrmac[Mod_idP]->config[0];

    phycfg.cfg->ssb_table.ssb_subcarrier_offset.value = ssb_SubcarrierOffset;
    phycfg.cfg->ssb_table.ssb_subcarrier_offset.tl.tag = NFAPI_NR_CONFIG_SSB_SUBCARRIER_OFFSET_TAG;
    phycfg.cfg->num_tlv++;

    if (RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req) RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req(&phycfg);

    find_SSB_and_RO_available(Mod_idP);

  }
  
  if (secondaryCellGroup) {

    RC.nrmac[Mod_idP]->secondaryCellGroupCommon = secondaryCellGroup;

    NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
    if (add_ue == 1 && get_softmodem_params()->phy_test) {
      const int UE_id = add_new_nr_ue(Mod_idP, rnti, secondaryCellGroup);
      LOG_I(PHY,"Added new UE_id %d/%x with initial secondaryCellGroup\n",UE_id,rnti);
    } else if (add_ue == 1 && !get_softmodem_params()->phy_test) {
      const int CC_id = 0;
      NR_COMMON_channels_t *cc = &RC.nrmac[Mod_idP]->common_channels[CC_id];
      uint8_t ra_index = 0;
      /* checking for free RA process */
      for(; ra_index < NR_NB_RA_PROC_MAX; ra_index++) {
        if((cc->ra[ra_index].state == RA_IDLE) && (!cc->ra[ra_index].cfra)) break;
      }
      if (ra_index == NR_NB_RA_PROC_MAX) {
        LOG_E(MAC, "%s() %s:%d RA processes are not available for CFRA RNTI :%x\n", __FUNCTION__, __FILE__, __LINE__, rnti);
        return -1;
      }	
      NR_RA_t *ra = &cc->ra[ra_index];
      ra->secondaryCellGroup = secondaryCellGroup;
      if (secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated!=NULL) {
        if (secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra != NULL) {
          ra->cfra = true;
          ra->rnti = rnti;
          struct NR_CFRA *cfra = secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra;
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
      }
      LOG_I(PHY,"Added new RA process for UE RNTI %04x with initial secondaryCellGroup\n", rnti);
    } else { // secondaryCellGroup has been updated
      const int UE_id = find_nr_UE_id(Mod_idP,rnti);
      UE_info->secondaryCellGroup[UE_id] = secondaryCellGroup;
      LOG_I(PHY,"Modified UE_id %d/%x with secondaryCellGroup\n",UE_id,rnti);
    }
    //update_csi_bitlen (secondaryCellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup, UE_list, UE_id, Mod_idP);
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_OUT);
  
    
  return(0);

}// END rrc_mac_config_req_gNB
