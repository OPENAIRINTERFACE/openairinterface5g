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

#define RRC_SL_PRECONFIG
#define RRC_SL_PRECONFIG_C

#include "oai_asn1.h"
#include "NR_SL-PreconfigurationNR-r16.h"
#include "common/utils/LOG/log.h"
#include "sl_preconfig_paramvalues.h"
#include "common/config/config_userapi.h"
#include "rrc_defs.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"
#include "nr-uesoftmodem.h"

void free_sl_rrc(NR_UE_RRC_INST_t *rrc)
{

  if (rrc->sl_preconfig) {
    ASN_STRUCT_FREE(asn_DEF_NR_SL_PreconfigurationNR_r16, rrc->sl_preconfig);
  }
}

static void prepare_NR_SL_SyncConfig(NR_SL_SyncConfig_r16_t *sl_syncconfig)
{

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

  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  paramdef_t SL_SYNCCFGPARAMS[] = SL_SYNCPARAMS_DESC(sl_syncconfig);
  paramlist_def_t SL_SYNCFGParamList = {SL_CONFIG_STRING_SL_SYNCCONFIG_LIST, NULL, 0};
  sprintf(aprefix, "%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION, 0);
  config_getlist(config_get_if(), &SL_SYNCFGParamList, NULL, 0, aprefix);
  LOG_I(RRC, "NUM SL-SYNCCFG elem in cfg file:%d\n", SL_SYNCFGParamList.numelt);
  sprintf(aprefix, "%s.[%i].%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION, 0,SL_CONFIG_STRING_SL_SYNCCONFIG_LIST, 0);
  config_get(config_get_if(), SL_SYNCCFGPARAMS, sizeofArray(SL_SYNCCFGPARAMS), aprefix);
}

static void prepare_NR_SL_ResourcePool(NR_SL_ResourcePool_r16_t *sl_res_pool,
                                       uint16_t is_txpool,
                                       uint16_t is_sl_syncsource)
{

  // PSCCH configuration
  sl_res_pool->sl_PSCCH_Config_r16 = calloc(1, sizeof(*sl_res_pool->sl_PSCCH_Config_r16));
  sl_res_pool->sl_PSCCH_Config_r16->present = NR_SetupRelease_SL_PSCCH_Config_r16_PR_setup;
  sl_res_pool->sl_PSCCH_Config_r16->choice.setup = calloc(1, sizeof(NR_SL_PSCCH_Config_r16_t));
  // Indicates number of symbols for PSCCH in a resource pool
  sl_res_pool->sl_PSCCH_Config_r16->choice.setup->sl_TimeResourcePSCCH_r16 = calloc(1, sizeof(long));

  // Indicates number of PRBs for PSCCH in a resource pool
  sl_res_pool->sl_PSCCH_Config_r16->choice.setup->sl_FreqResourcePSCCH_r16 = calloc(1, sizeof(long));

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

  sl_res_pool->dummy = NULL;

  // lowest RB index of lowest subch in this resource pool
  sl_res_pool->sl_StartRB_Subchannel_r16 = calloc(1, sizeof(long));

  //number of subchannels in this res pool. contiguous PRBs
  sl_res_pool->sl_NumSubchannel_r16 = calloc(1, sizeof(long));


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

  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  paramdef_t SL_POOLPARAMS[] = SL_RESPOOLPARAMS_DESC(sl_res_pool);
  if (is_txpool)
    sprintf(aprefix, "%s.[%i].%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION, 0,SL_CONFIG_STRING_SL_TX_RPOOL_LIST, 0);
  else
    sprintf(aprefix, "%s.[%i].%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION, 0,SL_CONFIG_STRING_SL_RX_RPOOL_LIST, 0);

  config_get(config_get_if(), SL_POOLPARAMS, sizeofArray(SL_POOLPARAMS), aprefix);
}

static void prepare_NR_SL_BWPConfigCommon(NR_SL_BWP_ConfigCommon_r16_t *sl_bwp,
                                   uint16_t num_tx_pools,
                                   uint16_t num_rx_pools,
                                   uint16_t sl_syncsource)
{

  sl_bwp->sl_BWP_Generic_r16 = calloc(1, sizeof(NR_SL_BWP_Generic_r16_t));
  sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16 = calloc(1, sizeof(NR_BWP_t));
  // if Cyclicprefix is NULL, then default value Normal cyclic prefix is configured. else EXT CP. 
  sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16->cyclicPrefix = NULL;

  // Value can be between symbols 7 to 14
  sl_bwp->sl_BWP_Generic_r16->sl_LengthSymbols_r16 = calloc(1, sizeof(long));

  // Value can be between symbols 0 to 7
  sl_bwp->sl_BWP_Generic_r16->sl_StartSymbol_r16 = calloc(1, sizeof(long));

  sl_bwp->sl_BWP_Generic_r16->sl_PSBCH_Config_r16 = calloc(1,sizeof(NR_SL_PSBCH_Config_r16_t));
  // PSBCH CONFIG contains PO and alpha for PSBCH powercontrol.
  sl_bwp->sl_BWP_Generic_r16->sl_PSBCH_Config_r16->present = NR_SetupRelease_SL_PSBCH_Config_r16_PR_release;

  sl_bwp->sl_BWP_Generic_r16->sl_TxDirectCurrentLocation_r16 = NULL;

  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  paramdef_t SL_BWPPARAMS[] = SL_BWPPARAMS_DESC(sl_bwp);
  paramlist_def_t SL_BWPParamList = {SL_CONFIG_STRING_SL_BWP_LIST, NULL, 0};
  sprintf(aprefix, "%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION, 0);
  config_getlist(config_get_if(), &SL_BWPParamList, NULL, 0, aprefix);
  LOG_I(RRC, "NUM SL-BWP elem in cfg file:%d\n", SL_BWPParamList.numelt);
  sprintf(aprefix, "%s.[%i].%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION, 0, SL_CONFIG_STRING_SL_BWP_LIST, 0);
  config_get(config_get_if(), SL_BWPPARAMS, sizeofArray(SL_BWPPARAMS), aprefix);

  sl_bwp->sl_BWP_PoolConfigCommon_r16 = calloc(1, sizeof(NR_SL_BWP_PoolConfigCommon_r16_t));

  paramlist_def_t SL_RxPoolParamList = {SL_CONFIG_STRING_SL_RX_RPOOL_LIST, NULL, 0};
  sprintf(aprefix, "%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION, 0);
  config_getlist(config_get_if(), &SL_RxPoolParamList, NULL, 0, aprefix);
  LOG_I(RRC, "NUM Rx RPOOLs in cfg file:%d\n", SL_RxPoolParamList.numelt);
  AssertFatal(SL_RxPoolParamList.numelt <= 1 && num_rx_pools <= 1, "Only Max 1 RX Respool Supported now\n");

  if (num_rx_pools || SL_RxPoolParamList.numelt) {
    // Receiving resource pool.
    NR_SL_ResourcePool_r16_t *respool = calloc(1, sizeof(*respool));
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16 = calloc(1, sizeof(*sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16));
    ASN_SEQUENCE_ADD(&sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16->list, respool);
    // Fill RX resource pool
    prepare_NR_SL_ResourcePool(sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16->list.array[0], 0, sl_syncsource);
  } else 
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_RxPool_r16 = NULL;

  paramlist_def_t SL_TxPoolParamList = {SL_CONFIG_STRING_SL_TX_RPOOL_LIST, NULL, 0};
  sprintf(aprefix, "%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION, 0);
  config_getlist(config_get_if(), &SL_TxPoolParamList, NULL, 0, aprefix);
  LOG_I(RRC, "NUM Tx RPOOL in cfg file:%d\n", SL_TxPoolParamList.numelt);
  AssertFatal(SL_TxPoolParamList.numelt <= 1 && num_tx_pools <= 1, "Only Max 1 TX Respool Supported now\n");

  if (num_tx_pools || SL_TxPoolParamList.numelt) {
    //resource pool(s) to transmit NR SL 
    NR_SL_ResourcePoolConfig_r16_t *respoolcfg = calloc(1, sizeof(*respoolcfg));
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16 =
                  calloc(1, sizeof(*sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16));
    ASN_SEQUENCE_ADD(&sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list, respoolcfg);
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list.array[0]->sl_ResourcePoolID_r16 = 0;
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list.array[0]->sl_ResourcePool_r16 = calloc(1, sizeof(NR_SL_ResourcePool_r16_t));
    // Fill tx resource pool
    prepare_NR_SL_ResourcePool(sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16->list.array[0]->sl_ResourcePool_r16, 1, sl_syncsource);
  } else 
    sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolSelectedNormal_r16 = NULL;

  sl_bwp->sl_BWP_PoolConfigCommon_r16->sl_TxPoolExceptional_r16 = NULL;
}

static void prepare_NR_SL_FreqConfigCommon(NR_SL_FreqConfigCommon_r16_t *sl_fcfg,
                                    uint16_t num_tx_pools,
                                    uint16_t num_rx_pools,
                                    uint16_t sl_syncsource)
{

  // Sub carrier spacing used on this frequency configured.
  NR_SCS_SpecificCarrier_t *scs_specific = calloc(1, sizeof(*scs_specific));
  ASN_SEQUENCE_ADD(&sl_fcfg->sl_SCS_SpecificCarrierList_r16.list, scs_specific);

  // NR bands for Sidelink n47, n38. 
  // N47band - 5855Mhz - 5925Mhz
  //sl_fcfg->sl_AbsoluteFrequencyPointA_r16 = 792000; //freq 5880Mhz

  //SL SSB chosen to be located from RB10 to RB21. points to the middle of the SSB block.
  //SSB location should be within Sidelink BWP
  //792000 + 10*12*2 + 66*2. channel raster is 15Khz for band47
  sl_fcfg->sl_AbsoluteFrequencySSB_r16 = calloc(1, sizeof(NR_ARFCN_ValueNR_t));

  //NR SL transmission with a 7.5 Khz shift to the LTE raster. if absent, freq shift is disabled.
  //Required if carrier freq configured for NR SL shared by LTE SL
  sl_fcfg->frequencyShift7p5khzSL_r16 = NULL;

  //NR SL transmission with valueN*5Khz shift to LTE raster.
  sl_fcfg->valueN_r16 = 0;

  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  paramdef_t SL_FCCPARAMS[] = SL_FCCPARAMS_DESC(sl_fcfg);
  paramlist_def_t SL_FCCParamList = {SL_CONFIG_STRING_SL_FCC_LIST, NULL, 0};
  sprintf(aprefix, "%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION,0);
  config_getlist(config_get_if(), &SL_FCCParamList, NULL, 0, aprefix);
  LOG_I(RRC, "NUM SL-FCC elem in cfg file:%d\n", SL_FCCParamList.numelt);
  sprintf(aprefix, "%s.[%i].%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION,0, SL_CONFIG_STRING_SL_FCC_LIST, 0);
  config_get(config_get_if(), SL_FCCPARAMS, sizeofArray(SL_FCCPARAMS), aprefix);

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
                                                               uint16_t sl_syncsource)
{

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
  NR_TDD_UL_DL_ConfigCommon_t *tdd_uldl_cfg = sl_preconfig->sl_PreconfigGeneral_r16->sl_TDD_Configuration_r16;
  tdd_uldl_cfg->pattern1.ext1 = NULL;
  tdd_uldl_cfg->pattern2 = NULL;

  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  sprintf(aprefix, "%s.[%i]", SL_CONFIG_STRING_SL_PRECONFIGURATION,0);
  paramdef_t SLTDDCFG_PARAMS[] = SL_TDDCONFIGPARAMS_DESC(tdd_uldl_cfg);
  config_get(config_get_if(), SLTDDCFG_PARAMS, sizeofArray(SLTDDCFG_PARAMS), aprefix);

  NR_SL_FreqConfigCommon_r16_t *fcc =  sl_preconfig->sl_PreconfigFreqInfoList_r16->list.array[0];
  tdd_uldl_cfg->referenceSubcarrierSpacing =
                          fcc->sl_SCS_SpecificCarrierList_r16.list.array[0]->subcarrierSpacing;
  NR_SL_BWP_ConfigCommon_r16_t *sl_bwp = fcc->sl_BWP_List_r16->list.array[0];
  sl_bwp->sl_BWP_Generic_r16->sl_BWP_r16->subcarrierSpacing =
                          fcc->sl_SCS_SpecificCarrierList_r16.list.array[0]->subcarrierSpacing;

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

int configure_NR_SL_Preconfig(NR_UE_RRC_INST_t *rrc,int sync_source)
{

  NR_SL_PreconfigurationNR_r16_t *sl_preconfig = NULL;
  int num_txpools = 0, num_rxpools = 0;

  if (sync_source) {
    //SL-Preconfiguration with 1 txpool, 0 rxpool if UE is a syncsource
    num_txpools = 1;
    sl_preconfig = prepare_NR_SL_PRECONFIGURATION(num_txpools,num_rxpools,sync_source);

    if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
      xer_fprint(stdout, &asn_DEF_NR_SL_PreconfigurationNR_r16, sl_preconfig);
    }
  } else {
    //SL-Preconfiguration with 0 txpool, 1 rxpool if UE is not a syncsource
    num_rxpools = 1;
    sl_preconfig = prepare_NR_SL_PRECONFIGURATION(num_txpools,num_rxpools,sync_source);

    if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
      xer_fprint(stdout, &asn_DEF_NR_SL_PreconfigurationNR_r16, sl_preconfig);
    }
  }

  rrc->sl_preconfig = sl_preconfig;

  return 0;
}

/*decode SL-BCH (SL-MIB) message*/
static int8_t nr_sl_rrc_ue_decode_SL_MIB(const uint8_t gNB_index,
                                         uint8_t *const bufferP,
                                         const uint8_t buffer_len)
{
  NR_MasterInformationBlockSidelink_t *sl_mib = NULL;

  asn_dec_rval_t dec_rval = uper_decode_complete(NULL, &asn_DEF_NR_MasterInformationBlockSidelink,
                                                 (void **)&sl_mib,
                                                 (const void *)bufferP, buffer_len);

  int ret = 0;
  if ((dec_rval.code != RC_OK) || (dec_rval.consumed == 0)) {
    LOG_E(NR_RRC, "SL-MIB decode error\n");
    ret = -1;
  } else  {

    int bits_unused = sl_mib->directFrameNumber_r16.bits_unused;
    uint16_t val_fn = sl_mib->directFrameNumber_r16.buf[0];
    val_fn = (val_fn << (8 - bits_unused)) + (sl_mib->directFrameNumber_r16.buf[1] >> bits_unused);

    uint8_t val_slot = sl_mib->slotIndex_r16.buf[0];

    LOG_D(NR_RRC, "SL-RRC - Received MIB\n");
    LOG_D(NR_RRC, "SL-MIB Contents - DFN:%d\n" , val_fn);
    LOG_D(NR_RRC, "SL-MIB Contents - SLOT:%d\n" , val_slot >> 1);
    LOG_D(NR_RRC, "SL-MIB Contents - Incoverage:%d\n", sl_mib->inCoverage_r16);
    LOG_D(NR_RRC, "SL-MIB Contents - sl-TDD-Config:%x\n" , *((uint16_t *)(sl_mib->sl_TDD_Config_r16.buf)));

    ASN_STRUCT_FREE(asn_DEF_NR_MasterInformationBlockSidelink, sl_mib);

  }

  return ret;
}


void nr_rrc_ue_decode_NR_SBCCH_SL_BCH_Message(NR_UE_RRC_INST_t *rrc,
                                              const uint8_t gNB_index,
                                              const frame_t frame,
                                              const int slot,
                                              uint8_t* pduP,
                                              const sdu_size_t pdu_len,
                                              const uint16_t rx_slss_id)
{

  nr_sl_rrc_ue_decode_SL_MIB(gNB_index, (uint8_t*)pduP, pdu_len);

  DevAssert(rrc->sl_preconfig);

  NR_SL_FreqConfigCommon_r16_t *fcfg = NULL;
  if (rrc->sl_preconfig->sidelinkPreconfigNR_r16.sl_PreconfigFreqInfoList_r16)
    fcfg = rrc->sl_preconfig->sidelinkPreconfigNR_r16.sl_PreconfigFreqInfoList_r16->list.array[0];
  DevAssert(fcfg);

  NR_SL_SSB_TimeAllocation_r16_t *sl_SSB_TimeAllocation = NULL;

  //Current implementation only supports one SSB Timeallocation
  //Extend RRC to use multiple SSB Time allocations TBD....
  if (fcfg->sl_SyncConfigList_r16)
    sl_SSB_TimeAllocation = fcfg->sl_SyncConfigList_r16->list.array[0]->sl_SSB_TimeAllocation1_r16;
  DevAssert(sl_SSB_TimeAllocation);

  nr_rrc_mac_config_req_sl_mib(rrc->ue_id,
                               sl_SSB_TimeAllocation,
                               rx_slss_id,
                               pduP);

  return;
}

void rrc_ue_process_sidelink_Preconfiguration(NR_UE_RRC_INST_t *rrc_inst,
                                              sl_sync_source_enum_t sync_source)
{

  AssertFatal(rrc_inst, "RRC instance not created.\n");

  NR_SL_PreconfigurationNR_r16_t *sl_preconfig = rrc_inst->sl_preconfig;
  AssertFatal(rrc_inst->sl_preconfig, "Check if SL-preconfig was created");

  AssertFatal(sync_source != SL_SYNC_SOURCE_GNBENB, "Sync source GNB not supported\n");

  nr_rrc_mac_config_req_sl_preconfig(rrc_inst->ue_id, sl_preconfig, sync_source);

  //TBD.. These should be chosen by RRC according to 3GPP 38.331 RRC specification.
  //Currently hardcoding the values to these
  uint16_t slss_id = 671, ssb_ta_index = 1;
  //12 bits -sl-TDD-config will be filled by MAC
  //Incoverage 1bit is FALSE as this is mode 2
  //DFN, sfn will be filled by PHY
  uint8_t sl_mib_payload[4] = {0,0,0,0};

  NR_SL_SSB_TimeAllocation_r16_t *ssb_ta = NULL;
  NR_SL_FreqConfigCommon_r16_t *fcfg = NULL;
  NR_SL_SyncConfig_r16_t *synccfg = NULL;
  if (sl_preconfig->sidelinkPreconfigNR_r16.sl_PreconfigFreqInfoList_r16)
    fcfg = sl_preconfig->sidelinkPreconfigNR_r16.sl_PreconfigFreqInfoList_r16->list.array[0];
  AssertFatal(fcfg, "Fcfg cannot be NULL\n");
  if (fcfg->sl_SyncConfigList_r16)
    synccfg = fcfg->sl_SyncConfigList_r16->list.array[0];
  AssertFatal(synccfg, "Synccfg cannot be NULL\n");

  if (ssb_ta_index == 1)
    ssb_ta = synccfg->sl_SSB_TimeAllocation1_r16;
  else if (ssb_ta_index == 2)
    ssb_ta = synccfg->sl_SSB_TimeAllocation2_r16;
  else if (ssb_ta_index == 3)
    ssb_ta = synccfg->sl_SSB_TimeAllocation3_r16;
  else DevAssert(0);

  AssertFatal(ssb_ta, "SSB_timeallocation cannot be NULL\n");

  if (sync_source == SL_SYNC_SOURCE_LOCAL_TIMING || sync_source == SL_SYNC_SOURCE_GNSS)
    nr_rrc_mac_transmit_slss_req(rrc_inst->ue_id,sl_mib_payload, slss_id, ssb_ta);

}

//For Sidelink mode 2 operation this prepares the sidelink preconfiguration
void init_sidelink(NR_UE_RRC_INST_t *rrc)
{
  int sync_ref = get_softmodem_params()->sync_ref;

  if (get_softmodem_params()->sl_mode == 2) {
    //Preparation of the Sidelink PRE-Configuration message
    configure_NR_SL_Preconfig(rrc, sync_ref);

  }
}
