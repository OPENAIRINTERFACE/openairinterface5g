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

/*! \file nr_ rrc_common.c
 * \brief rrc common procedures for gNB
 * \author Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2011 - 2014, 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email:  navid.nikaein@eurecom.fr and raymond.knopp@eurecom.fr, kroempa@gmail.com
 */

#include "nr_rrc_extern.h"
#include "LAYER2/MAC/mac_extern.h"
#include "COMMON/openair_defs.h"
#include "COMMON/platform_types.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "LAYER2/RLC/rlc.h"
#include "COMMON/mac_rrc_primitives.h"
#include "UTIL/LOG/log.h"
#include "asn1_msg.h"
#include "pdcp.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "rrc_eNB_UE_context.h"
#include "common/ran_context.h"

#define DEBUG_NR_RRC 1

extern RAN_CONTEXT_t RC;
extern UE_MAC_INST *UE_mac_inst;
extern mui_t rrc_gNB_mui;

//-----------------------------------------------------------------------------
int rrc_init_nr_global_param(void){

  rrc_rlc_register_rrc (rrc_data_ind, NULL); //register with rlc

  DCCH_LCHAN_DESC.transport_block_size = 4;
  DCCH_LCHAN_DESC.max_transport_blocks = 16;
  DCCH_LCHAN_DESC.Delay_class = 1;
  DTCH_DL_LCHAN_DESC.transport_block_size = 52;
  DTCH_DL_LCHAN_DESC.max_transport_blocks = 20;
  DTCH_DL_LCHAN_DESC.Delay_class = 1;
  DTCH_UL_LCHAN_DESC.transport_block_size = 52;
  DTCH_UL_LCHAN_DESC.max_transport_blocks = 20;
  DTCH_UL_LCHAN_DESC.Delay_class = 1;

  Rlc_info_um.rlc_mode = RLC_MODE_UM;
  Rlc_info_um.rlc.rlc_um_info.timer_reordering = 5;
  Rlc_info_um.rlc.rlc_um_info.sn_field_length = 10;
  Rlc_info_um.rlc.rlc_um_info.is_mXch = 0;
  //Rlc_info_um.rlc.rlc_um_info.sdu_discard_mode=16;

  Rlc_info_am_config.rlc_mode = RLC_MODE_AM;
  Rlc_info_am_config.rlc.rlc_am_info.max_retx_threshold = 50;
  Rlc_info_am_config.rlc.rlc_am_info.poll_pdu = 8;
  Rlc_info_am_config.rlc.rlc_am_info.poll_byte = 1000;
  Rlc_info_am_config.rlc.rlc_am_info.t_poll_retransmit = 15;
  Rlc_info_am_config.rlc.rlc_am_info.t_reordering = 50;
  Rlc_info_am_config.rlc.rlc_am_info.t_status_prohibit = 10;

  return 0;
}

//-----------------------------------------------------------------------------
void rrc_config_nr_buffer(NR_SRB_INFO* Srb_info,
                          uint8_t Lchan_type,
                          uint8_t Role
                         ){

  Srb_info->Rx_buffer.payload_size = 0;
  Srb_info->Tx_buffer.payload_size = 0;
}

//-----------------------------------------------------------------------------

void rrc_config_servingcellconfigcommon(uint8_t Mod_id,
                                        int CC_id
                                        #if defined(ENABLE_ITTI)
                                        ,gNB_RrcConfigurationReq *common_configuration
                                        #endif
                                       ){

  common_configuration->MIB_subCarrierSpacingCommon[CC_id]                                = 0;
  common_configuration->MIB_ssb_SubcarrierOffset[CC_id]                                   = 0;
  common_configuration->MIB_dmrs_TypeA_Position[CC_id]                                    = 0;
  common_configuration->pdcch_ConfigSIB1[CC_id]                                           = 0;
  common_configuration->absoluteFrequencySSB[CC_id]                                       = 0;
  common_configuration->DL_FreqBandIndicatorNR[CC_id]                                     = 0;
  common_configuration->DL_absoluteFrequencyPointA[CC_id]                                 = 0;
  common_configuration->DL_offsetToCarrier[CC_id]                                         = 0;
  common_configuration->DL_SCS_SubcarrierSpacing[CC_id]                                   = 0;
  common_configuration->DL_carrierBandwidth[CC_id]                                        = 0;
  common_configuration->DL_locationAndBandwidth[CC_id]                                    = 0;
  common_configuration->DL_BWP_SubcarrierSpacing[CC_id]                                   = 0;
  common_configuration->DL_BWP_prefix_type[CC_id]                                         = 0;
  common_configuration->UL_FreqBandIndicatorNR[CC_id]                                     = 0;
  common_configuration->UL_absoluteFrequencyPointA[CC_id]                                 = 0;
  common_configuration->UL_additionalSpectrumEmission[CC_id]                              = 0;
  common_configuration->UL_p_Max[CC_id]                                                   = 0;
  common_configuration->UL_frequencyShift7p5khz[CC_id]                                    = 0;
  common_configuration->UL_offsetToCarrier[CC_id]                                         = 0;
  common_configuration->UL_SCS_SubcarrierSpacing[CC_id]                                   = 0;
  common_configuration->UL_carrierBandwidth[CC_id]                                        = 0;
  common_configuration->UL_locationAndBandwidth[CC_id]                                    = 0;
  common_configuration->UL_BWP_SubcarrierSpacing[CC_id]                                   = 0;
  common_configuration->UL_BWP_prefix_type[CC_id]                                         = 0;
  common_configuration->UL_timeAlignmentTimerCommon[CC_id]                                = 0;
  common_configuration->ServingCellConfigCommon_ssb_PositionsInBurst_PR[CC_id]            = 0;
  common_configuration->ServingCellConfigCommon_ssb_periodicityServingCell[CC_id]         = 0;
  common_configuration->ServingCellConfigCommon_dmrs_TypeA_Position[CC_id]                = 0;
  common_configuration->NIA_SubcarrierSpacing[CC_id]                                      = 0;
  common_configuration->ServingCellConfigCommon_ss_PBCH_BlockPower[CC_id]                 = 0;
  common_configuration->referenceSubcarrierSpacing[CC_id]                                 = 0;
  common_configuration->dl_UL_TransmissionPeriodicity[CC_id]                              = 0;
  common_configuration->nrofDownlinkSlots[CC_id]                                          = 0;
  common_configuration->nrofDownlinkSymbols[CC_id]                                        = 0;
  common_configuration->nrofUplinkSlots[CC_id]                                            = 0;
  common_configuration->nrofUplinkSymbols[CC_id]                                          = 0;
  common_configuration->rach_totalNumberOfRA_Preambles[CC_id]                             = 0;
  common_configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_choice[CC_id]      = 0;
  common_configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneEighth[CC_id]   = 0;
  common_configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneFourth[CC_id]   = 0;
  common_configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneHalf[CC_id]     = 0;
  common_configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_one[CC_id]         = 0;
  common_configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_two[CC_id]         = 0;
  common_configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_four[CC_id]        = 0;
  common_configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_eight[CC_id]       = 0;
  common_configuration->rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_sixteen[CC_id]     = 0;
  common_configuration->rach_groupBconfigured[CC_id]                                      = 0;
  common_configuration->rach_ra_Msg3SizeGroupA[CC_id]                                     = 0;
  common_configuration->rach_messagePowerOffsetGroupB[CC_id]                              = 0;
  common_configuration->rach_numberOfRA_PreamblesGroupA[CC_id]                            = 0;
  common_configuration->rach_ra_ContentionResolutionTimer[CC_id]                          = 0;
  common_configuration->rsrp_ThresholdSSB[CC_id]                                          = 0;
  common_configuration->rsrp_ThresholdSSB_SUL[CC_id]                                      = 0;
  common_configuration->prach_RootSequenceIndex_choice[CC_id]                             = 0;
  common_configuration->prach_RootSequenceIndex_l839[CC_id]                               = 0;
  common_configuration->prach_RootSequenceIndex_l139[CC_id]                               = 0;
  common_configuration->prach_msg1_SubcarrierSpacing[CC_id]                               = 0;
  common_configuration->restrictedSetConfig[CC_id]                                        = 0;
  common_configuration->msg3_transformPrecoding[CC_id]                                    = 0;
  common_configuration->prach_ConfigurationIndex[CC_id]                                   = 0;
  common_configuration->prach_msg1_FDM[CC_id]                                             = 0;
  common_configuration->prach_msg1_FrequencyStart[CC_id]                                  = 0;
  common_configuration->zeroCorrelationZoneConfig[CC_id]                                  = 0;
  common_configuration->preambleReceivedTargetPower[CC_id]                                = 0;
  common_configuration->preambleTransMax[CC_id]                                           = 0;
  common_configuration->powerRampingStep[CC_id]                                           = 0;
  common_configuration->ra_ResponseWindow[CC_id]                                          = 0;
  common_configuration->groupHoppingEnabledTransformPrecoding[CC_id]                      = 0;
  common_configuration->msg3_DeltaPreamble[CC_id]                                         = 0;
  common_configuration->p0_NominalWithGrant[CC_id]                                        = 0;
  common_configuration->PUSCH_TimeDomainResourceAllocation_k2[CC_id]                      = 0;
  common_configuration->PUSCH_TimeDomainResourceAllocation_mappingType[CC_id]             = 0;
  common_configuration->PUSCH_TimeDomainResourceAllocation_startSymbolAndLength[CC_id]    = 0;
  common_configuration->pucch_ResourceCommon[CC_id]                                       = 0;
  common_configuration->pucch_GroupHopping[CC_id]                                         = 0;
  common_configuration->hoppingId[CC_id]                                                  = 0;
  common_configuration->p0_nominal[CC_id]                                                 = 0;
  common_configuration->PDSCH_TimeDomainResourceAllocation_k0[CC_id]                      = 0;
  common_configuration->PDSCH_TimeDomainResourceAllocation_mappingType[CC_id]             = 0;
  common_configuration->PDSCH_TimeDomainResourceAllocation_startSymbolAndLength[CC_id]    = 0;
  common_configuration->rateMatchPatternId[CC_id]                                         = 0;
  common_configuration->RateMatchPattern_patternType[CC_id]                               = 0;
  common_configuration->symbolsInResourceBlock[CC_id]                                     = 0;
  common_configuration->periodicityAndPattern[CC_id]                                      = 0;
  common_configuration->RateMatchPattern_controlResourceSet[CC_id]                        = 0;
  common_configuration->RateMatchPattern_subcarrierSpacing[CC_id]                         = 0;
  common_configuration->RateMatchPattern_mode[CC_id]                                      = 0;
  common_configuration->controlResourceSetZero[CC_id]                                     = 0;
  common_configuration->searchSpaceZero[CC_id]                                            = 0;
  common_configuration->searchSpaceSIB1[CC_id]                                            = 0;
  common_configuration->searchSpaceOtherSystemInformation[CC_id]                          = 0;
  common_configuration->pagingSearchSpace[CC_id]                                          = 0;
  common_configuration->ra_SearchSpace[CC_id]                                             = 0;
  common_configuration->PDCCH_common_controlResourceSetId[CC_id]                          = 0;
  common_configuration->PDCCH_common_ControlResourceSet_duration[CC_id]                   = 0;
  common_configuration->PDCCH_cce_REG_MappingType[CC_id]                                  = 0;
  common_configuration->PDCCH_reg_BundleSize[CC_id]                                       = 0;
  common_configuration->PDCCH_interleaverSize[CC_id]                                      = 0;
  common_configuration->PDCCH_shiftIndex[CC_id]                                           = 0;
  common_configuration->PDCCH_precoderGranularity[CC_id]                                  = 0;
  common_configuration->PDCCH_TCI_StateId[CC_id]                                          = 0;
  common_configuration->tci_PresentInDCI[CC_id]                                           = 0;
  common_configuration->PDCCH_DMRS_ScramblingID[CC_id]                                    = 0;
  common_configuration->SearchSpaceId[CC_id]                                              = 0;
  common_configuration->commonSearchSpaces_controlResourceSetId[CC_id]                    = 0;
  common_configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_choice[CC_id]      = 0;
  common_configuration->SearchSpace_monitoringSlotPeriodicityAndOffset_value[CC_id]       = 0;
  common_configuration->SearchSpace_duration[CC_id]                                       = 0;
  common_configuration->SearchSpace_nrofCandidates_aggregationLevel1[CC_id]               = 0;
  common_configuration->SearchSpace_nrofCandidates_aggregationLevel2[CC_id]               = 0;
  common_configuration->SearchSpace_nrofCandidates_aggregationLevel4[CC_id]               = 0;
  common_configuration->SearchSpace_nrofCandidates_aggregationLevel8[CC_id]               = 0;
  common_configuration->SearchSpace_nrofCandidates_aggregationLevel16[CC_id]              = 0;
  common_configuration->SearchSpace_searchSpaceType[CC_id]                                = 0;
  common_configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel1[CC_id]  = 0;
  common_configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel2[CC_id]  = 0;
  common_configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel4[CC_id]  = 0;
  common_configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel8[CC_id]  = 0;
  common_configuration->Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel16[CC_id] = 0;
  common_configuration->Common_dci_Format2_3_monitoringPeriodicity[CC_id]                 = 0;
  common_configuration->Common_dci_Format2_3_nrofPDCCH_Candidates[CC_id]                  = 0;
  common_configuration->ue_Specific__dci_Formats[CC_id]                                   = 0;
  common_configuration->RateMatchPatternLTE_CRS_carrierFreqDL[CC_id]                      = 0;
  common_configuration->RateMatchPatternLTE_CRS_carrierBandwidthDL[CC_id]                 = 0;
  common_configuration->RateMatchPatternLTE_CRS_nrofCRS_Ports[CC_id]                      = 0;
  common_configuration->RateMatchPatternLTE_CRS_v_Shift[CC_id]                            = 0;
  common_configuration->RateMatchPatternLTE_CRS_radioframeAllocationPeriod[CC_id]         = 0;
  common_configuration->RateMatchPatternLTE_CRS_radioframeAllocationOffset[CC_id]         = 0;
  common_configuration->RateMatchPatternLTE_CRS_subframeAllocation_choice[CC_id]          = 0;

}