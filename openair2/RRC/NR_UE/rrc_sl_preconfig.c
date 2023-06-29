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

/* \file rrc_sl_preconfig.c
 * \brief Preparation of Sidelink-Preconfiguration message
 * \author Raghavendra Dinavahi
 * \date 
 * \version 
 * \company Fraunhofer IIS
 * \email
 * \note
 * \warning
 */

#define RRC_SL_PRECONFIG
#define RRC_SL_PRECONFIG_C

#include "oai_asn1.h"
#include "NR_SL-PreconfigurationNR-r16.h"
#include "common/utils/LOG/log.h"

static void prepare_NR_SL_SyncConfig(NR_SL_SyncConfig_r16_t *sl_syncconfig) {

  // Hysteris when evaluating SyncRef UE
  sl_syncconfig->sl_SyncRefMinHyst_r16 = NULL;

  // Hysteris when evaluating SyncRef UE
  sl_syncconfig->sl_SyncRefDiffHyst_r16 = NULL;

  // Filtering for SL RSRP
  sl_syncconfig->sl_filterCoefficient_r16 = NULL;

  // SSB Periodicity within 16 frames.
  sl_syncconfig->sl_SSB_TimeAllocation1_r16 = calloc(1, sizeof(NR_SL_SSB_TimeAllocation_r16_t));
  sl_syncconfig->sl_SSB_TimeAllocation1_r16->sl_NumSSB_WithinPeriod_r16 = calloc(1, sizeof(long));
  sl_syncconfig->sl_SSB_TimeAllocation1_r16->sl_TimeOffsetSSB_r16 = calloc(1, sizeof(long));
  sl_syncconfig->sl_SSB_TimeAllocation1_r16->sl_TimeInterval_r16 = calloc(1, sizeof(long));
  *sl_syncconfig->sl_SSB_TimeAllocation1_r16->sl_NumSSB_WithinPeriod_r16 = NR_SL_SSB_TimeAllocation_r16__sl_NumSSB_WithinPeriod_r16_n2;
  *sl_syncconfig->sl_SSB_TimeAllocation1_r16->sl_TimeOffsetSSB_r16 = 8;
  *sl_syncconfig->sl_SSB_TimeAllocation1_r16->sl_TimeInterval_r16 = 120;

  sl_syncconfig->sl_SSB_TimeAllocation2_r16 = NULL;
  sl_syncconfig->sl_SSB_TimeAllocation3_r16 = NULL;

  //SLSS Id 
  sl_syncconfig->sl_SSID_r16 = NULL;

  // Threshold to be used in coverage
  sl_syncconfig->txParameters_r16.syncTxThreshIC_r16 = NULL;

  // Threshold to be used when Out of coverage
  sl_syncconfig->txParameters_r16.syncTxThreshOoC_r16 = NULL;

  // Syncconfig is used when UE is synced to GNSS if set, else if UE is synced to eNB/gNB
  sl_syncconfig->gnss_Sync_r16 = calloc(1, sizeof(long));
  *sl_syncconfig->gnss_Sync_r16 = 0; // GNSS
}

static void prepare_NR_SL_ResourcePool(NR_SL_ResourcePool_r16_t *sl_res_pool,
                                       uint16_t is_txpool,
                                       uint16_t is_sl_syncsource) {

  // PSCCH configuration
  sl_res_pool->sl_PSCCH_Config_r16 = calloc(1, sizeof(*sl_res_pool->sl_PSCCH_Config_r16));
  sl_res_pool->sl_PSCCH_Config_r16->present = NR_SetupRelease_SL_PSCCH_Config_r16_PR_setup;
  sl_res_pool->sl_PSCCH_Config_r16->choice.setup = calloc(1, sizeof(NR_SL_PSCCH_Config_r16_t));
  // Indicates number of symbols for PSCCH in a resource pool
  sl_res_pool->sl_PSCCH_Config_r16->choice.setup->sl_TimeResourcePSCCH_r16 = calloc(1, sizeof(long));
  *sl_res_pool->sl_PSCCH_Config_r16->choice.setup->sl_TimeResourcePSCCH_r16 = NR_SL_PSCCH_Config_r16__sl_TimeResourcePSCCH_r16_n3;

  // Indicates number of PRBs for PSCCH in a resource pool
  sl_res_pool->sl_PSCCH_Config_r16->choice.setup->sl_FreqResourcePSCCH_r16 = calloc(1, sizeof(long));
  *sl_res_pool->sl_PSCCH_Config_r16->choice.setup->sl_FreqResourcePSCCH_r16 = NR_SL_PSCCH_Config_r16__sl_FreqResourcePSCCH_r16_n25;

  // Inititation during PSCCH DMRS Sequence generation
  sl_res_pool->sl_PSCCH_Config_r16->choice.setup->sl_DMRS_ScrambleID_r16 = calloc(1, sizeof(long));
  *sl_res_pool->sl_PSCCH_Config_r16->choice.setup->sl_DMRS_ScrambleID_r16 = 0;

  // num reserve bits used for first stage SCI
  sl_res_pool->sl_PSCCH_Config_r16->choice.setup->sl_NumReservedBits_r16 = calloc(1, sizeof(long));
  *sl_res_pool->sl_PSCCH_Config_r16->choice.setup->sl_NumReservedBits_r16 = 2;

  //PSSCH Configuration
  sl_res_pool->sl_PSSCH_Config_r16 = calloc(1, sizeof(NR_SetupRelease_SL_PSSCH_Config_r16_t));
  sl_res_pool->sl_PSSCH_Config_r16->present = NR_SetupRelease_SL_PSSCH_Config_r16_PR_setup;
  sl_res_pool->sl_PSSCH_Config_r16->choice.setup = calloc(1, sizeof(NR_SL_PSSCH_Config_r16_t));
  sl_res_pool->sl_PSSCH_Config_r16->choice.setup->sl_PSSCH_DMRS_TimePatternList_r16 =
                      calloc(1, sizeof(*sl_res_pool->sl_PSSCH_Config_r16->choice.setup->sl_PSSCH_DMRS_TimePatternList_r16));
  for(int i=0; i<3; i++) {
    long *p = calloc(1, sizeof(long));
    *p = 2+i; // valid values: 2..4
    ASN_SEQUENCE_ADD(&sl_res_pool->sl_PSSCH_Config_r16->choice.setup->sl_PSSCH_DMRS_TimePatternList_r16->list, p);
  }

  //PSFCH configuration
  sl_res_pool->sl_PSFCH_Config_r16 = NULL;

  // indicates allowed sync sources which are allowed to use this resource pool
  sl_res_pool->sl_SyncAllowed_r16 = calloc(1, sizeof(NR_SL_SyncAllowed_r16_t));

  //configured resources can be used if UE is directly/indirectly synced to network.
  sl_res_pool->sl_SyncAllowed_r16->gnbEnb_Sync_r16 = NULL;

  //configured resources can be used if UE is directly/indirectly synced to GNSS.
  sl_res_pool->sl_SyncAllowed_r16->gnss_Sync_r16 = calloc(1, sizeof(long));
  *sl_res_pool->sl_SyncAllowed_r16->gnss_Sync_r16 = NR_SL_SyncAllowed_r16__gnss_Sync_r16_true;

  //configured resources can be used if UE is directly/indirectly synced to SYNC REF UE.
  sl_res_pool->sl_SyncAllowed_r16->ue_Sync_r16 = calloc(1, sizeof(long));
  *sl_res_pool->sl_SyncAllowed_r16->ue_Sync_r16 = NR_SL_SyncAllowed_r16__ue_Sync_r16_true;

  //Min freq domain resources used for resource sensing. Size of Subchannels
  sl_res_pool->sl_SubchannelSize_r16 = calloc(1, sizeof(long));
  *sl_res_pool->sl_SubchannelSize_r16 = NR_SL_ResourcePool_r16__sl_SubchannelSize_r16_n50;

  sl_res_pool->dummy = NULL;

  // lowest RB index of lowest subch in this resource pool
  sl_res_pool->sl_StartRB_Subchannel_r16 = calloc(1, sizeof(long));
  *sl_res_pool->sl_StartRB_Subchannel_r16 = 0; // STARTs from RB0

  //number of subchannels in this res pool. contiguous PRBs
  sl_res_pool->sl_NumSubchannel_r16 = calloc(1, sizeof(long));
  *sl_res_pool->sl_NumSubchannel_r16 = 1;


  // 64QAM table is default. in case other MCS tables needs tobe used.
  sl_res_pool->sl_Additional_MCS_Table_r16 = NULL;

  sl_res_pool->sl_ThreshS_RSSI_CBR_r16 = NULL;
  sl_res_pool->sl_TimeWindowSizeCBR_r16 = NULL;
  sl_res_pool->sl_TimeWindowSizeCR_r16 = NULL;
  sl_res_pool->sl_PTRS_Config_r16 = NULL;
  sl_res_pool->sl_UE_SelectedConfigRP_r16 = NULL;
  sl_res_pool->sl_RxParametersNcell_r16 = NULL;
  sl_res_pool->sl_ZoneConfigMCR_List_r16 = NULL;
  sl_res_pool->sl_FilterCoefficient_r16 = NULL;

  //number of contiguous PRBS in this res pool.
  sl_res_pool->sl_RB_Number_r16 = calloc(1, sizeof(long));
  *sl_res_pool->sl_RB_Number_r16 = 50;

  sl_res_pool->sl_PreemptionEnable_r16 = NULL;
  sl_res_pool->sl_PriorityThreshold_UL_URLLC_r16 = NULL;
  sl_res_pool->sl_PriorityThreshold_r16 = NULL;
  sl_res_pool->sl_X_Overhead_r16 = NULL;
  sl_res_pool->sl_PowerControl_r16 = NULL;
  sl_res_pool->sl_TxPercentageList_r16 = NULL;
  sl_res_pool->sl_MinMaxMCS_List_r16 = NULL;

  sl_res_pool->ext1 = calloc(1, sizeof(*sl_res_pool->ext1));
  sl_res_pool->ext1->sl_TimeResource_r16 = calloc(1, sizeof(*sl_res_pool->ext1->sl_TimeResource_r16));
  sl_res_pool->ext1->sl_TimeResource_r16->size = 8;
  sl_res_pool->ext1->sl_TimeResource_r16->bits_unused = 4;
  sl_res_pool->ext1->sl_TimeResource_r16->buf = calloc(sl_res_pool->ext1->sl_TimeResource_r16->size, sizeof(uint8_t));
  // EX: BITMAP 10101010.. indicating every alternating slot supported for sidelink
  for (int i=0;i<sl_res_pool->ext1->sl_TimeResource_r16->size;i++) {
    if (is_txpool) {
        sl_res_pool->ext1->sl_TimeResource_r16->buf[i] = (is_sl_syncsource) ? 0xAA //0x88;//0xAA;
                                                                            : 0x55;//0x11;//0x55;
    } else {
        sl_res_pool->ext1->sl_TimeResource_r16->buf[i] = (is_sl_syncsource) ? 0x55 //0x88;//0xAA;
                                                                            : 0xAA;//0x11;//0x55;
    }
  }

  // mask out unused bits
  sl_res_pool->ext1->sl_TimeResource_r16->buf[sl_res_pool->ext1->sl_TimeResource_r16->size - 1] &= (0 - (1 << (sl_res_pool->ext1->sl_TimeResource_r16->bits_unused)));

}

static void prepare_NR_SL_BWPConfigCommon(NR_SL_BWP_ConfigCommon_r16_t *sl_bwp,
                                   uint16_t num_tx_pools,
                                   uint16_t num_rx_pools,
                                   uint16_t sl_syncsource) {

  sl_bwp->sl_BWP_Generic_r16 = calloc(1, sizeof(NR_SL_BWP_Generic_r16_t));
  sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16 = calloc(1, sizeof(NR_BWP_t));
  // if Cyclicprefix is NULL, then default value Normal cyclic prefix is configured. else EXT CP. 
  sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16->cyclicPrefix = NULL;

  //30Khz and 40Mhz - 106 RBs
  sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16->locationAndBandwidth = 28875;
  sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16->subcarrierSpacing = NR_SubcarrierSpacing_kHz30;

  sl_bwp->sl_BWP_Generic_r16->sl_LengthSymbols_r16 = calloc(1, sizeof(long));
  // Value can be between symbols 7 to 14
  *sl_bwp->sl_BWP_Generic_r16->sl_LengthSymbols_r16 = NR_SL_BWP_Generic_r16__sl_LengthSymbols_r16_sym14;

  sl_bwp->sl_BWP_Generic_r16->sl_StartSymbol_r16 = calloc(1, sizeof(long));
  // Value can be between symbols 0 to 7
  *sl_bwp->sl_BWP_Generic_r16->sl_StartSymbol_r16 = NR_SL_BWP_Generic_r16__sl_StartSymbol_r16_sym0;

  sl_bwp->sl_BWP_Generic_r16->sl_PSBCH_Config_r16 = calloc(1,sizeof(NR_SL_PSBCH_Config_r16_t));
  // PSBCH CONFIG contains PO and alpha for PSBCH powercontrol.
  sl_bwp->sl_BWP_Generic_r16->sl_PSBCH_Config_r16->present = NR_SetupRelease_SL_PSBCH_Config_r16_PR_release;

  sl_bwp->sl_BWP_Generic_r16->sl_TxDirectCurrentLocation_r16 = NULL;

  sl_bwp->sl_BWP_PoolConfigCommon_r16 = calloc(1, sizeof(NR_SL_BWP_PoolConfigCommon_r16_t));

  if (num_rx_pools) {
    AssertFatal(num_rx_pools >= 1, "Currently supports only 1 RX pool\n");
    // Receiving resource pool.
    NR_SL_ResourcePool_r16_t *respool = calloc(1, sizeof(*respool));
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16 = calloc(1, sizeof(*sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16));
    ASN_SEQUENCE_ADD(&sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16->list, respool);
    // Fill RX resource pool
    prepare_NR_SL_ResourcePool(sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16->list.array[0], 0, sl_syncsource);
  } else 
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16 = NULL;

  if (num_tx_pools) {
    AssertFatal(num_tx_pools >= 1, "Currently supports only 1 TX pool\n");
    //resource pool(s) to transmit NR SL 
    NR_SL_ResourcePoolConfig_r16_t *respoolcfg = calloc(1, sizeof(*respoolcfg));
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16 = calloc(1, sizeof(*sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16));
    ASN_SEQUENCE_ADD(&sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list, respoolcfg);
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list.array[0]->sl_ResourcePoolID_r16 = 1;
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list.array[0]->sl_ResourcePool_r16 = calloc(1, sizeof(NR_SL_ResourcePool_r16_t));
    // Fill tx resource pool
    prepare_NR_SL_ResourcePool(sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list.array[0]->sl_ResourcePool_r16, num_tx_pools, sl_syncsource);
  } else 
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16 = NULL;

  sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolExceptional_r16 = NULL;
}

static void prepare_NR_SL_FreqConfigCommon(NR_SL_FreqConfigCommon_r16_t *sl_fcfg,
                                    uint16_t num_tx_pools,
                                    uint16_t num_rx_pools,
                                    uint16_t sl_syncsource) {

  // Sub carrier spacing used on this frequency configured.
  NR_SCS_SpecificCarrier_t *scs_specific = calloc(1, sizeof(*scs_specific));
  ASN_SEQUENCE_ADD(&sl_fcfg->sl_SCS_SpecificCarrierList_r16.list, scs_specific);
  sl_fcfg->sl_SCS_SpecificCarrierList_r16.list.array[0]->offsetToCarrier = 0;
  sl_fcfg->sl_SCS_SpecificCarrierList_r16.list.array[0]->subcarrierSpacing = NR_SubcarrierSpacing_kHz30;
  sl_fcfg->sl_SCS_SpecificCarrierList_r16.list.array[0]->carrierBandwidth = 106; //40Mhz

  // NR bands for Sidelink n47, n38. 
  // N47band - 5855Mhz - 5925Mhz
  sl_fcfg->sl_AbsoluteFrequencyPointA_r16 = 792000; //freq 5880Mhz

  //SL SSB chosen to be located from RB10 to RB21. points to the middle of the SSB block.
  //SSB location should be within Sidelink BWP
  //792000 + 10*12*2 + 66*2. channel raster is 15Khz for band47
  sl_fcfg->sl_AbsoluteFrequencySSB_r16 = calloc(1, sizeof(NR_ARFCN_ValueNR_t));
  *sl_fcfg->sl_AbsoluteFrequencySSB_r16 =  792372; 

  //NR SL transmission with a 7.5 Khz shift to the LTE raster. if absent, freq shift is disabled.
  //Required if carrier freq configured for NR SL shared by LTE SL
  sl_fcfg->frequencyShift7p5khzSL_r16 = NULL;

  //NR SL transmission with valueN*5Khz shift to LTE raster.
  sl_fcfg->valueN_r16 = 0;

  // Sidelink BWP configuration. 
  // In REL16, 17 SUPPORTS only 1 SIDELINK Bandwidth part
  NR_SL_BWP_ConfigCommon_r16_t *bwpcfgcommon = calloc(1, sizeof(*bwpcfgcommon));
  sl_fcfg->sl_BWP_List_r16 = calloc(1, sizeof(*sl_fcfg->sl_BWP_List_r16));
  ASN_SEQUENCE_ADD(&sl_fcfg->sl_BWP_List_r16->list, bwpcfgcommon);
  prepare_NR_SL_BWPConfigCommon(sl_fcfg->sl_BWP_List_r16->list.array[0], num_tx_pools, num_rx_pools, sl_syncsource);

  // sync prio between GNSS and gNB/eNB
  sl_fcfg->sl_SyncPriority_r16 = calloc(1, sizeof(long));
  *sl_fcfg->sl_SyncPriority_r16 = 0; // Set to GNSS

  // If TRUE/1 - Network can be selected as sync source directly/indirectly in case syncprio = GNSS.
  sl_fcfg->sl_NbAsSync_r16 = calloc(1, sizeof(long));
  *sl_fcfg->sl_NbAsSync_r16 = 1;

  // config info related to rx and tx of SL SYNC SIGNALS (SLSS)
  NR_SL_SyncConfig_r16_t *synccfg = calloc(1, sizeof(*synccfg));
  sl_fcfg->sl_SyncConfigList_r16 = calloc(1, sizeof(*sl_fcfg->sl_SyncConfigList_r16));
  ASN_SEQUENCE_ADD(&sl_fcfg->sl_SyncConfigList_r16->list, synccfg);
  prepare_NR_SL_SyncConfig(sl_fcfg->sl_SyncConfigList_r16->list.array[0]);

}

NR_SL_PreconfigurationNR_r16_t *prepare_NR_SL_PRECONFIGURATION(uint16_t num_tx_pools,
                                                               uint16_t num_rx_pools,
                                                               uint16_t sl_syncsource) {

  NR_SL_PreconfigurationNR_r16_t *sl_preconfiguration = CALLOC(1, sizeof(NR_SL_PreconfigurationNR_r16_t));
  NR_SidelinkPreconfigNR_r16_t *sl_preconfig = &sl_preconfiguration->sidelinkPreconfigNR_r16;

  //FILL in Frequency config common
  NR_SL_FreqConfigCommon_r16_t *freqcfgcommon = calloc(1, sizeof(*freqcfgcommon));
  sl_preconfig->sl_PreconfigFreqInfoList_r16 = calloc(1, sizeof(*sl_preconfig->sl_PreconfigFreqInfoList_r16));
  //Supported only 1 FREQs for NR SL communication.
  ASN_SEQUENCE_ADD(&sl_preconfig->sl_PreconfigFreqInfoList_r16->list, freqcfgcommon);

  prepare_NR_SL_FreqConfigCommon(sl_preconfig->sl_PreconfigFreqInfoList_r16->list.array[0],
                                 num_tx_pools, num_rx_pools, sl_syncsource);

  // NR Frequency list
  sl_preconfig->sl_PreconfigNR_AnchorCarrierFreqList_r16 = NULL;

  // EUTRA Frequency list
  sl_preconfig->sl_PreconfigEUTRA_AnchorCarrierFreqList_r16 = NULL;

  // NR sidelink radio bearer(s) configuration(s)
  sl_preconfig->sl_RadioBearerPreConfigList_r16 = NULL; // fill later

  // NR sidelink RLC bearer(s) configuration(s)
  sl_preconfig->sl_RLC_BearerPreConfigList_r16 = NULL; // fill later

  //Measurement and reporting configuration
  sl_preconfig->sl_MeasPreConfig_r16 = NULL;

  //DFN timing offset used if GNSS is used as sync source.
  //1-.001ms, 2 - .002ms so on. value 0 if absent.
  sl_preconfig->sl_OffsetDFN_r16 = NULL;

  // t400 started upon txn of RRCreconfSidelink.
  sl_preconfig->t400_r16 = NULL;

  //Max num consecutive HARQ DTX before triggering SL RLF.
  sl_preconfig->sl_MaxNumConsecutiveDTX_r16 = NULL;

  //Priority of SSB transmission and reception. used in comparison to UL rxns/txns
  sl_preconfig->sl_SSB_PriorityNR_r16 = NULL;

  //Contains TDD ULDL confiuguration to be used by the sync source UE. 
  //Currently set to the default used in OAI 5G. Changes TBD..
  //For the UE with sync reference as another UE, TDD ULDL config is determined from SL-MIB
  sl_preconfig->sl_PreconfigGeneral_r16 = calloc(1, sizeof(NR_SL_PreconfigGeneral_r16_t));
  sl_preconfig->sl_PreconfigGeneral_r16->sl_TDD_Configuration_r16 = calloc(1, sizeof(NR_TDD_UL_DL_ConfigCommon_t));
  NR_TDD_UL_DL_ConfigCommon_t *tdd_ul_dl_cfg = sl_preconfig->sl_PreconfigGeneral_r16->sl_TDD_Configuration_r16;
  tdd_ul_dl_cfg->pattern1.dl_UL_TransmissionPeriodicity = NR_TDD_UL_DL_Pattern__dl_UL_TransmissionPeriodicity_ms2p5;
  tdd_ul_dl_cfg->pattern1.nrofDownlinkSlots = 7;
  tdd_ul_dl_cfg->pattern1.nrofDownlinkSymbols = 10;
  tdd_ul_dl_cfg->pattern1.nrofUplinkSlots = 2;
  tdd_ul_dl_cfg->pattern1.nrofUplinkSymbols = 4;
  tdd_ul_dl_cfg->pattern1.ext1 = NULL;
  tdd_ul_dl_cfg->pattern2 = NULL;

  // Configurations used for UE autonomous resource selection
  sl_preconfig->sl_UE_SelectedPreConfig_r16 = NULL;

  // indicates if CSI reporting supported in SL unicast.
  sl_preconfig->sl_CSI_Acquisition_r16 = NULL;

  // ROHC profiles for NR SL
  sl_preconfig->sl_RoHC_Profiles_r16 = NULL;

  // MaxCID value for PDCP as specified in 38.323
  sl_preconfig->sl_MaxCID_r16 = NULL;


  return sl_preconfiguration;
}


static void dump_NR_SL_ResourcePoolParams(NR_SL_ResourcePool_r16_t *respool) {

  if (respool->sl_PSCCH_Config_r16 &&
      respool->sl_PSCCH_Config_r16->present == NR_SetupRelease_SL_PSCCH_Config_r16_PR_setup) {

    NR_SL_PSCCH_Config_r16_t *pscch_cfg = respool->sl_PSCCH_Config_r16->choice.setup;
    LOG_I(NR_RRC,"PSCCH config: sl_TimeResourcePSCCH:%ld, sl_FreqResourcePSCCH:%ld, sl_DMRS_ScrambleID:%ld, sl_NumReservedBits:%ld\n",
                (pscch_cfg->sl_TimeResourcePSCCH_r16)? *pscch_cfg->sl_TimeResourcePSCCH_r16 : 0,
                (pscch_cfg->sl_FreqResourcePSCCH_r16)? *pscch_cfg->sl_FreqResourcePSCCH_r16 : 0,
                (pscch_cfg->sl_DMRS_ScrambleID_r16)? *pscch_cfg->sl_DMRS_ScrambleID_r16 : 0,
                (pscch_cfg->sl_NumReservedBits_r16)? *pscch_cfg->sl_NumReservedBits_r16 : 0);
  } else {
    LOG_I(NR_RRC,"PSCCH CONFIG: not present\n");
  }

  if (respool->sl_PSSCH_Config_r16 &&
      respool->sl_PSSCH_Config_r16->present == NR_SetupRelease_SL_PSSCH_Config_r16_PR_setup) {

    LOG_I(NR_RRC,"PSSCH config: present\n");
  } else {
    LOG_I(NR_RRC,"PSSCH CONFIG: not present\n");
  }
  if (respool->sl_PSFCH_Config_r16 &&
      respool->sl_PSFCH_Config_r16->present == NR_SetupRelease_SL_PSFCH_Config_r16_PR_setup) {

    LOG_I(NR_RRC,"PSFCH config: present\n");
  } else {
    LOG_I(NR_RRC,"PSFCH CONFIG: not present\n");
  }

  LOG_I(NR_RRC,"Subchannels info: sl_SubchannelSize:%ld, sl_StartRB_Subchannel:%ld, sl_NumSubchannel:%ld, sl_RB_Number:%ld\n",
                (respool->sl_SubchannelSize_r16)? *respool->sl_SubchannelSize_r16 : 0,
                (respool->sl_StartRB_Subchannel_r16)? *respool->sl_StartRB_Subchannel_r16 : 0,
                (respool->sl_NumSubchannel_r16)? *respool->sl_NumSubchannel_r16 : 0,
                (respool->sl_RB_Number_r16)? *respool->sl_RB_Number_r16 : 0);

  if (respool->ext1 &&
      respool->ext1->sl_TimeResource_r16) {
    int size = respool->ext1->sl_TimeResource_r16->size;
    int unused = respool->ext1->sl_TimeResource_r16->bits_unused;
    LOG_I(NR_RRC, "sl_TimeResource bitmap len:%d\n",size*8-unused);
    for (int i=0;i<size;i++) {
      LOG_I(NR_RRC, "sl_TimeResource bitmap buf[%d]:%x\n",i,respool->ext1->sl_TimeResource_r16->buf[i]);
    }
  }

}

static void dump_NR_SL_FreqConfigCommonParams(NR_SL_FreqConfigCommon_r16_t *sl_fcfg) {

  LOG_I(NR_RRC, "NR_SL_FreqConfigCommon_r16 IEs.............\n");

  LOG_I(NR_RRC, "sl_AbsoluteFrequencyPointA:%ld \n", sl_fcfg->sl_AbsoluteFrequencyPointA_r16);

  if (sl_fcfg->sl_AbsoluteFrequencySSB_r16) {
    LOG_I(NR_RRC, "*sl_AbsoluteFrequencySSB_r16:%ld \n", *sl_fcfg->sl_AbsoluteFrequencySSB_r16);
  }

  int num = sl_fcfg->sl_SCS_SpecificCarrierList_r16.list.count;
  for (int i = 0; i < num; i++) {
    LOG_I(NR_RRC," SCS entry[%d]: offsetToCarrier:%ld,  subcarrierSpacing:%ld, carrierBw:%ld \n", 
      i, sl_fcfg->sl_SCS_SpecificCarrierList_r16.list.array[i]->offsetToCarrier,
         sl_fcfg->sl_SCS_SpecificCarrierList_r16.list.array[i]->subcarrierSpacing,
         sl_fcfg->sl_SCS_SpecificCarrierList_r16.list.array[i]->carrierBandwidth);
  }

  LOG_I(NR_RRC, "valueN_r16:%ld \n", sl_fcfg->valueN_r16);

  if (sl_fcfg->sl_BWP_List_r16) {
    num = sl_fcfg->sl_BWP_List_r16->list.count;
    LOG_I(NR_RRC, "Sidelink BWPs configured:%d\n", num);
    for (int i = 0; i < num; i++) {
      NR_SL_BWP_ConfigCommon_r16_t *sl_bwp = sl_fcfg->sl_BWP_List_r16->list.array[i];
      if (sl_bwp->sl_BWP_Generic_r16 &&
          sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16) {
            LOG_I(NR_RRC," SL-BWP[%d]: CyclicPrefix:%s, scs:%ld, locandBw:%ld\n",
                            i, (sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16->cyclicPrefix) ? "EXTENDED" : "NORMAL",
                            sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16->subcarrierSpacing,
                            sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16->locationAndBandwidth);

            LOG_I(NR_RRC," SL-BWP[%d]: sl_LengthSymbols:%ld, sl_StartSymbol:%ld\n",
                            i, *sl_bwp->sl_BWP_Generic_r16->sl_LengthSymbols_r16,
                            *sl_bwp->sl_BWP_Generic_r16->sl_StartSymbol_r16);


            if (sl_bwp->sl_BWP_PoolConfigCommon_r16 &&
                sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16) {

              int num_rxpools = sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16->list.count;
              LOG_I(NR_RRC, "NUM RX RESOURCE POOLs:%d\n", num_rxpools);
              for (int i=0; i<num_rxpools;i++) {
                LOG_I(NR_RRC, "RX RESOURCE POOL[%d]..... \n", i);
                dump_NR_SL_ResourcePoolParams(sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16->list.array[i]);
              }
            }
            if (sl_bwp->sl_BWP_PoolConfigCommon_r16 &&
                sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16) {

              int num_txpools = sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list.count;
              LOG_I(NR_RRC, "NUM TX RESOURCE POOLs:%d\n", num_txpools);
              for (int i=0; i<num_txpools;i++) {
                LOG_I(NR_RRC, "TX RESOURCE POOL[%d]..... \n", i);
                dump_NR_SL_ResourcePoolParams(sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list.array[i]->sl_ResourcePool_r16);
              }
            }
          }
    }
  }

  if (sl_fcfg->sl_SyncConfigList_r16) {
    int num = sl_fcfg->sl_SyncConfigList_r16->list.count;
    LOG_I(NR_RRC, "NUM SyncConfig entries:%d\n", num);
    for (int i=0;i<num;i++) {
      NR_SL_SyncConfig_r16_t *synccfg = sl_fcfg->sl_SyncConfigList_r16->list.array[i];
      if (synccfg->sl_SSB_TimeAllocation1_r16) {
        NR_SL_SSB_TimeAllocation_r16_t *ssb_ta = synccfg->sl_SSB_TimeAllocation1_r16;
        LOG_I(NR_RRC, "syncconfig[%d]: Timealloc1, sl_NumSSB_WithinPeriod:%ld, sl_TimeOffsetSSB:%ld,sl_TimeInterval:%ld\n",
                      i, (ssb_ta->sl_NumSSB_WithinPeriod_r16) ? *ssb_ta->sl_NumSSB_WithinPeriod_r16 : 0,
                      (ssb_ta->sl_TimeOffsetSSB_r16) ? *ssb_ta->sl_TimeOffsetSSB_r16 : 0,
                      (ssb_ta->sl_TimeInterval_r16) ? *ssb_ta->sl_TimeInterval_r16 : 0);
      } else {
        LOG_I(NR_RRC, "syncconfig[%d]: Timealloc1 not present\n",i);
      }
      if (synccfg->sl_SSB_TimeAllocation2_r16) {
        NR_SL_SSB_TimeAllocation_r16_t *ssb_ta = synccfg->sl_SSB_TimeAllocation2_r16;
        LOG_I(NR_RRC, "syncconfig[%d]: Timealloc2, sl_NumSSB_WithinPeriod:%ld, sl_TimeOffsetSSB:%ld,sl_TimeInterval:%ld\n",
                      i, (ssb_ta->sl_NumSSB_WithinPeriod_r16) ? *ssb_ta->sl_NumSSB_WithinPeriod_r16 : 0,
                      (ssb_ta->sl_TimeOffsetSSB_r16) ? *ssb_ta->sl_TimeOffsetSSB_r16 : 0,
                      (ssb_ta->sl_TimeInterval_r16) ? *ssb_ta->sl_TimeInterval_r16 : 0);
      } else {
        LOG_I(NR_RRC, "syncconfig[%d]: Timealloc2 not present\n",i);
      }
      if (synccfg->sl_SSB_TimeAllocation3_r16) {
        NR_SL_SSB_TimeAllocation_r16_t *ssb_ta = synccfg->sl_SSB_TimeAllocation3_r16;
        LOG_I(NR_RRC, "syncconfig[%d]: Timealloc3, sl_NumSSB_WithinPeriod:%ld, sl_TimeOffsetSSB:%ld,sl_TimeInterval:%ld\n",
                      i, (ssb_ta->sl_NumSSB_WithinPeriod_r16) ? *ssb_ta->sl_NumSSB_WithinPeriod_r16 : 0,
                      (ssb_ta->sl_TimeOffsetSSB_r16) ? *ssb_ta->sl_TimeOffsetSSB_r16 : 0,
                      (ssb_ta->sl_TimeInterval_r16) ? *ssb_ta->sl_TimeInterval_r16 : 0);
      } else {
        LOG_I(NR_RRC, "syncconfig[%d]: Timealloc3 not present\n",i);
      }
    }
  }
}


void dump_NR_SL_Preconfiguration(NR_SL_PreconfigurationNR_r16_t *sl_preconfiguration) {

  NR_SidelinkPreconfigNR_r16_t *sl_preconfig = &sl_preconfiguration->sidelinkPreconfigNR_r16;

  AssertFatal(sl_preconfiguration || sl_preconfig ,"Sl_preconf cannot be NULL\n");

  LOG_I(NR_RRC, "-------------START of NR_SL_Preconfiguration IEs............\n");

  if (sl_preconfig->sl_PreconfigFreqInfoList_r16) {
    int num = sl_preconfig->sl_PreconfigFreqInfoList_r16->list.count;
    LOG_I(NR_RRC, "Number of Sidelink Frequencies configured:%d\n", num);

    for (int i=0;i<num;i++) {
      dump_NR_SL_FreqConfigCommonParams(sl_preconfig->sl_PreconfigFreqInfoList_r16->list.array[0]);
    }
  }

  if (sl_preconfig->sl_PreconfigGeneral_r16 &&
      sl_preconfig->sl_PreconfigGeneral_r16->sl_TDD_Configuration_r16) {

    NR_TDD_UL_DL_ConfigCommon_t *tdd_ul_dl_cfg = sl_preconfig->sl_PreconfigGeneral_r16->sl_TDD_Configuration_r16;
    LOG_I(NR_RRC,"SL-TDD ULDL config: Transmissionperiod:%ld,DL-Slots:%ld,Sym:%ld, UL-Slots:%ld,Symbols:%ld\n",
                          tdd_ul_dl_cfg->pattern1.dl_UL_TransmissionPeriodicity,
                          tdd_ul_dl_cfg->pattern1.nrofDownlinkSlots, tdd_ul_dl_cfg->pattern1.nrofDownlinkSymbols,
                          tdd_ul_dl_cfg->pattern1.nrofUplinkSlots, tdd_ul_dl_cfg->pattern1.nrofUplinkSymbols);

    if (tdd_ul_dl_cfg->pattern1.ext1 &&
        tdd_ul_dl_cfg->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530) {
      LOG_I(NR_RRC, "SL-TDD ULDL config: Transmissionperiod_v1530:%ld\n",
                *tdd_ul_dl_cfg->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530);
    }
  
    AssertFatal(tdd_ul_dl_cfg->pattern2 == NULL ,"pattern2 not supported\n");
  }

  LOG_I(NR_RRC, "-------------END OF NR_SL_Preconfiguration IEs............\n\n");
}

int configure_NR_SL_Preconfig() {

  //Example configurations to check and test
  //UE needs to be set with only 1 PREconfiguration. TBD.. Remove these later.

  //SL-Preconfiguration with 1 txpool, 1 rxpool and not syncsource
  NR_SL_PreconfigurationNR_r16_t *sl_preconfig = prepare_NR_SL_PRECONFIGURATION(1,1,0);
  dump_NR_SL_Preconfiguration(sl_preconfig);

  ASN_STRUCT_FREE(asn_DEF_NR_SL_PreconfigurationNR_r16, sl_preconfig);
  sl_preconfig = NULL;
  //END.......

  //SL-Preconfiguration with 1 txpool, 0 rxpool and UE is syncsource
  sl_preconfig = prepare_NR_SL_PRECONFIGURATION(1,0,1);
  dump_NR_SL_Preconfiguration(sl_preconfig);

  ASN_STRUCT_FREE(asn_DEF_NR_SL_PreconfigurationNR_r16, sl_preconfig);
  sl_preconfig = NULL;
  //END

  //SL-Preconfiguration with 0 txpool, 1 rxpool and UE is not a syncsource
  sl_preconfig = prepare_NR_SL_PRECONFIGURATION(0,1,0);
  dump_NR_SL_Preconfiguration(sl_preconfig);

  ASN_STRUCT_FREE(asn_DEF_NR_SL_PreconfigurationNR_r16, sl_preconfig);
  sl_preconfig = NULL;

  return 0;
}