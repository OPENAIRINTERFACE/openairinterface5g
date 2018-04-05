
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
 * \brief UE and eNB configuration performed by RRC or as a consequence of RRC procedures
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \version 0.1
 * \email: navid.nikaein@eurecom.fr
 * @ingroup _mac

 */

#include "COMMON/platform_types.h"
#include "COMMON/platform_constants.h"
#include "SCHED/defs.h"
#include "SystemInformationBlockType2.h"
//#include "RadioResourceConfigCommonSIB.h"
#include "RadioResourceConfigDedicated.h"
#ifdef Rel14
#include "PRACH-ConfigSIB-v1310.h"
#endif
#include "MeasGapConfig.h"
#include "MeasObjectToAddModList.h"
#include "TDD-Config.h"
#include "MAC-MainConfig.h"
#include "defs.h"
#include "proto.h"
#include "extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

#include "common/ran_context.h"
#if defined(Rel10) || defined(Rel14)
#include "MBSFN-AreaInfoList-r9.h"
#include "MBSFN-AreaInfo-r9.h"
#include "MBSFN-SubframeConfigList.h"
#include "PMCH-InfoList-r9.h"
#endif

extern RAN_CONTEXT_t RC;
extern int l2_init_eNB(void);
extern void mac_top_init_eNB(void);
extern void mac_init_cell_params(int Mod_idP,int CC_idP);

extern uint8_t nfapi_mode;

int32_t **rxdata;
int32_t **txdata;


typedef struct eutra_bandentry_s {
  int16_t band;
  uint32_t ul_min;
  uint32_t ul_max;
  uint32_t dl_min;
  uint32_t dl_max;
  uint32_t N_OFFs_DL;
} eutra_bandentry_t;

typedef struct band_info_s {
  int nbands;
  eutra_bandentry_t band_info[100];
} band_info_t;



static const eutra_bandentry_t eutra_bandtable[] = {
  {1, 19200, 19800, 21100, 21700, 0},
  {2, 18500, 19100, 19300, 19900, 6000},
  {3, 17100, 17850, 18050, 18800, 12000},
  {4, 17100, 17550, 21100, 21550, 19500},
  {5, 8240, 8490, 8690, 8940, 24000},
  {6, 8300, 8400, 8750, 8850, 26500},
  {7, 25000, 25700, 26200, 26900, 27500},
  {8, 8800, 9150, 9250, 9600, 34500},
  {9, 17499, 17849, 18449, 18799, 38000},
  {10, 17100, 17700, 21100, 21700, 41500},
  {11, 14279, 14529, 14759, 15009, 47500},
  {12, 6980, 7160, 7280, 7460, 50100},
  {13, 7770, 7870, 7460, 7560, 51800},
  {14, 7880, 7980, 7580, 7680, 52800},
  {17, 7040, 7160, 7340, 7460, 57300},
  {18, 8150, 9650, 8600, 10100, 58500},
  {19, 8300, 8450, 8750, 8900, 60000},
  {20, 8320, 8620, 7910, 8210, 61500},
  {21, 14479, 14629, 14959, 15109, 64500},
  {22, 34100, 34900, 35100, 35900, 66000},
  {23, 20000, 20200, 21800, 22000, 75000},
  {24, 16126, 16605, 15250, 15590, 77000},
  {25, 18500, 19150, 19300, 19950, 80400},
  {26, 8140, 8490, 8590, 8940, 86900},
  {27, 8070, 8240, 8520, 8690, 90400},
  {28, 7030, 7580, 7580, 8130, 92100},
  {29, 0, 0, 7170, 7280, 96600},
  {30, 23050, 23250, 23500, 23600, 97700},
  {31, 45250, 34900, 46250, 35900, 98700},
  {32, 0, 0, 14520, 14960, 99200},
  {33, 19000, 19200, 19000, 19200, 36000},
  {34, 20100, 20250, 20100, 20250, 36200},
  {35, 18500, 19100, 18500, 19100, 36350},
  {36, 19300, 19900, 19300, 19900, 36950},
  {37, 19100, 19300, 19100, 19300, 37550},
  {38, 25700, 26200, 25700, 26300, 37750},
  {39, 18800, 19200, 18800, 19200, 38250},
  {40, 23000, 24000, 23000, 24000, 38650},
  {41, 24960, 26900, 24960, 26900, 39650},
  {42, 34000, 36000, 34000, 36000, 41590},
  {43, 36000, 38000, 36000, 38000, 43590},
  {44, 7030, 8030, 7030, 8030, 45590},
  {45, 14470, 14670, 14470, 14670, 46590},
  {46, 51500, 59250, 51500, 59250, 46790},
  {65, 19200, 20100, 21100, 22000, 65536},
  {66, 17100, 18000, 21100, 22000, 66436},
  {67, 0, 0, 7380, 7580, 67336},
  {68, 6980, 7280, 7530, 7830, 67536}
};

uint32_t to_earfcn(int eutra_bandP, uint32_t dl_CarrierFreq, uint32_t bw)
{

  uint32_t dl_CarrierFreq_by_100k = dl_CarrierFreq / 100000;
  int bw_by_100 = bw / 100;

  int i;

  AssertFatal(eutra_bandP < 69, "eutra_band %d > 68\n", eutra_bandP);
  for (i = 0; i < 69 && eutra_bandtable[i].band != eutra_bandP; i++);

  AssertFatal(dl_CarrierFreq_by_100k >= eutra_bandtable[i].dl_min,
	      "Band %d, bw %u : DL carrier frequency %u Hz < %u\n",
	      eutra_bandP, bw, dl_CarrierFreq,
	      eutra_bandtable[i].dl_min);
  AssertFatal(dl_CarrierFreq_by_100k <=
	      (eutra_bandtable[i].dl_max - bw_by_100),
	      "Band %d, bw %u: DL carrier frequency %u Hz > %d\n",
	      eutra_bandP, bw, dl_CarrierFreq,
	      eutra_bandtable[i].dl_max - bw_by_100);


  return (dl_CarrierFreq_by_100k - eutra_bandtable[i].dl_min +
	  (eutra_bandtable[i].N_OFFs_DL / 10));
}

uint32_t from_earfcn(int eutra_bandP, uint32_t dl_earfcn)
{

  int i;

  AssertFatal(eutra_bandP < 69, "eutra_band %d > 68\n", eutra_bandP);
  for (i = 0; i < 69 && eutra_bandtable[i].band != eutra_bandP; i++);

  return (eutra_bandtable[i].dl_min +
	  (dl_earfcn - (eutra_bandtable[i].N_OFFs_DL / 10))) * 100000;
}


int32_t get_uldl_offset(int eutra_bandP)
{
  int i;

  for (i = 0; i < 69 && eutra_bandtable[i].band != eutra_bandP; i++);
  return (eutra_bandtable[i].dl_min - eutra_bandtable[i].ul_min);
}

uint32_t bw_table[6] = {6*180,15*180,25*180,50*180,75*180,100*180};

void config_mib(int                 Mod_idP,
		int                 CC_idP,
		int                 eutra_bandP,  
		int                 dl_BandwidthP,
		PHICH_Config_t      *phich_configP,
		int                 Nid_cellP,
		int                 NcpP,
		int                 p_eNBP,
		uint32_t            dl_CarrierFreqP,
		uint32_t            ul_CarrierFreqP
#ifdef Rel14
                ,
		uint32_t            pbch_repetitionP
#endif
                ) {

  nfapi_config_request_t *cfg = &RC.mac[Mod_idP]->config[CC_idP];

  cfg->num_tlv=0;
		
  cfg->subframe_config.pcfich_power_offset.value   = 6000;  // 0dB
  cfg->subframe_config.pcfich_power_offset.tl.tag = NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG;
  cfg->num_tlv++;

  cfg->subframe_config.dl_cyclic_prefix_type.value = NcpP;
  cfg->subframe_config.dl_cyclic_prefix_type.tl.tag = NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG;
  cfg->num_tlv++;

  cfg->subframe_config.ul_cyclic_prefix_type.value = NcpP;
  cfg->subframe_config.ul_cyclic_prefix_type.tl.tag = NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG;
  cfg->num_tlv++;
 
  cfg->rf_config.dl_channel_bandwidth.value        = to_prb(dl_BandwidthP);
  cfg->rf_config.dl_channel_bandwidth.tl.tag = NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG;
  cfg->num_tlv++;
  LOG_E(PHY,"%s() dl_BandwidthP:%d\n", __FUNCTION__, dl_BandwidthP);

  cfg->rf_config.ul_channel_bandwidth.value        = to_prb(dl_BandwidthP);
  cfg->rf_config.ul_channel_bandwidth.tl.tag = NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG;
  cfg->num_tlv++;

  cfg->rf_config.tx_antenna_ports.value            = p_eNBP;
  cfg->rf_config.tx_antenna_ports.tl.tag = NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG;
  cfg->num_tlv++;

  cfg->rf_config.rx_antenna_ports.value            = 2;
  cfg->rf_config.rx_antenna_ports.tl.tag = NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG;
  cfg->num_tlv++;

  cfg->nfapi_config.earfcn.value                   = to_earfcn(eutra_bandP,dl_CarrierFreqP,bw_table[dl_BandwidthP]/100);
  cfg->nfapi_config.earfcn.tl.tag = NFAPI_NFAPI_EARFCN_TAG;
  cfg->num_tlv++;

  cfg->nfapi_config.rf_bands.number_rf_bands       = 1;
  cfg->nfapi_config.rf_bands.rf_band[0]            = eutra_bandP;  
  cfg->nfapi_config.rf_bands.tl.tag = NFAPI_PHY_RF_BANDS_TAG;
  cfg->num_tlv++;

  cfg->phich_config.phich_resource.value           = phich_configP->phich_Resource;
  cfg->phich_config.phich_resource.tl.tag = NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG;
  cfg->num_tlv++;

  cfg->phich_config.phich_duration.value           = phich_configP->phich_Duration;
  cfg->phich_config.phich_duration.tl.tag = NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG;
  cfg->num_tlv++;

  cfg->phich_config.phich_power_offset.value       = 6000;  // 0dB
  cfg->phich_config.phich_power_offset.tl.tag = NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG;
  cfg->num_tlv++;

  cfg->sch_config.primary_synchronization_signal_epre_eprers.value   = 6000; // 0dB
  cfg->sch_config.primary_synchronization_signal_epre_eprers.tl.tag = NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG;
  cfg->num_tlv++;

  cfg->sch_config.secondary_synchronization_signal_epre_eprers.value = 6000; // 0dB
  cfg->sch_config.secondary_synchronization_signal_epre_eprers.tl.tag = NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG;
  cfg->num_tlv++;

  cfg->sch_config.physical_cell_id.value                             = Nid_cellP;
  cfg->sch_config.physical_cell_id.tl.tag = NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG;
  cfg->num_tlv++;

#ifdef Rel14
  cfg->emtc_config.pbch_repetitions_enable_r13.value                 = pbch_repetitionP;
  cfg->emtc_config.pbch_repetitions_enable_r13.tl.tag = NFAPI_EMTC_CONFIG_PBCH_REPETITIONS_ENABLE_R13_TAG;
  cfg->num_tlv++;
#endif  
  LOG_I(MAC,
	"%s() NFAPI_CONFIG_REQUEST(num_tlv:%u) DL_BW:%u UL_BW:%u Ncp %d,p_eNB %d,earfcn %d,band %d,phich_resource %u phich_duration %u phich_power_offset %u PSS %d SSS %d PCI %d"
#ifdef Rel14
	
	" PBCH repetition %d"
#endif  
	"\n"
	,__FUNCTION__
	,cfg->num_tlv
	,cfg->rf_config.dl_channel_bandwidth.value
	,cfg->rf_config.ul_channel_bandwidth.value
	,NcpP,p_eNBP
	,cfg->nfapi_config.earfcn.value
	,cfg->nfapi_config.rf_bands.rf_band[0]
	,cfg->phich_config.phich_resource.value
	,cfg->phich_config.phich_duration.value
	,cfg->phich_config.phich_power_offset.value
	,cfg->sch_config.primary_synchronization_signal_epre_eprers.value
	,cfg->sch_config.secondary_synchronization_signal_epre_eprers.value
	,cfg->sch_config.physical_cell_id.value
#ifdef Rel14
	,cfg->emtc_config.pbch_repetitions_enable_r13.value
#endif  
      );
}

void config_sib1(int Mod_idP, int CC_idP, TDD_Config_t * tdd_ConfigP)
{


  nfapi_config_request_t *cfg = &RC.mac[Mod_idP]->config[CC_idP];

  if (tdd_ConfigP)   { //TDD
    cfg->subframe_config.duplex_mode.value                          = 0;
    cfg->subframe_config.duplex_mode.tl.tag = NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG;
    cfg->num_tlv++;

    cfg->tdd_frame_structure_config.subframe_assignment.value       = tdd_ConfigP->subframeAssignment;
    cfg->tdd_frame_structure_config.subframe_assignment.tl.tag = NFAPI_TDD_FRAME_STRUCTURE_SUBFRAME_ASSIGNMENT_TAG;
    cfg->num_tlv++;

    cfg->tdd_frame_structure_config.special_subframe_patterns.value = tdd_ConfigP->specialSubframePatterns;
    cfg->tdd_frame_structure_config.special_subframe_patterns.tl.tag = NFAPI_TDD_FRAME_STRUCTURE_SPECIAL_SUBFRAME_PATTERNS_TAG;
    cfg->num_tlv++;
  }
  else { // FDD
    cfg->subframe_config.duplex_mode.value                          = 1;
    cfg->subframe_config.duplex_mode.tl.tag = NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG;
    cfg->num_tlv++;
    // Note no half-duplex here
  }
}

int power_off_dB[6] = { 78, 118, 140, 170, 188, 200 };

void
config_sib2(int Mod_idP,
	    int CC_idP,
	    RadioResourceConfigCommonSIB_t * radioResourceConfigCommonP,
#ifdef Rel14
	    RadioResourceConfigCommonSIB_t * radioResourceConfigCommon_BRP,
#endif
            ARFCN_ValueEUTRA_t *ul_CArrierFreqP,
            long *ul_BandwidthP,
            AdditionalSpectrumEmission_t *additionalSpectrumEmissionP,
            struct MBSFN_SubframeConfigList  *mbsfn_SubframeConfigListP) {

  nfapi_config_request_t *cfg = &RC.mac[Mod_idP]->config[CC_idP];

  cfg->subframe_config.pb.value               = radioResourceConfigCommonP->pdsch_ConfigCommon.p_b;
  cfg->subframe_config.pb.tl.tag = NFAPI_SUBFRAME_CONFIG_PB_TAG;
  cfg->num_tlv++;

  cfg->rf_config.reference_signal_power.value = radioResourceConfigCommonP->pdsch_ConfigCommon.referenceSignalPower;
  cfg->rf_config.reference_signal_power.tl.tag = NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG;
  cfg->num_tlv++;

  cfg->nfapi_config.max_transmit_power.value  = cfg->rf_config.reference_signal_power.value + power_off_dB[cfg->rf_config.dl_channel_bandwidth.value];
  cfg->nfapi_config.max_transmit_power.tl.tag = NFAPI_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG;
  cfg->num_tlv++;

  cfg->prach_config.configuration_index.value                 = radioResourceConfigCommonP->prach_Config.prach_ConfigInfo.prach_ConfigIndex;
  cfg->prach_config.configuration_index.tl.tag = NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG;
  cfg->num_tlv++;

  cfg->prach_config.root_sequence_index.value                 = radioResourceConfigCommonP->prach_Config.rootSequenceIndex;
  cfg->prach_config.root_sequence_index.tl.tag = NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG;
  cfg->num_tlv++;

  cfg->prach_config.zero_correlation_zone_configuration.value = radioResourceConfigCommonP->prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig;
  cfg->prach_config.zero_correlation_zone_configuration.tl.tag = NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG;
  cfg->num_tlv++;

  cfg->prach_config.high_speed_flag.value                     = radioResourceConfigCommonP->prach_Config.prach_ConfigInfo.highSpeedFlag;
  cfg->prach_config.high_speed_flag.tl.tag = NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG;
  cfg->num_tlv++;

  cfg->prach_config.frequency_offset.value                    = radioResourceConfigCommonP->prach_Config.prach_ConfigInfo.prach_FreqOffset;
  cfg->prach_config.frequency_offset.tl.tag = NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG;
  cfg->num_tlv++;


  cfg->pusch_config.hopping_mode.value                        = radioResourceConfigCommonP->pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode;
  cfg->pusch_config.hopping_mode.tl.tag = NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG;
  cfg->num_tlv++;

  cfg->pusch_config.number_of_subbands.value                  = radioResourceConfigCommonP->pusch_ConfigCommon.pusch_ConfigBasic.n_SB;
  cfg->pusch_config.number_of_subbands.tl.tag = NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG;
  cfg->num_tlv++;

  cfg->pusch_config.hopping_offset.value                      = radioResourceConfigCommonP->pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset;
  cfg->pusch_config.hopping_offset.tl.tag = NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG;
  cfg->num_tlv++;

  cfg->pucch_config.delta_pucch_shift.value                         = radioResourceConfigCommonP->pucch_ConfigCommon.deltaPUCCH_Shift;
  cfg->pucch_config.delta_pucch_shift.tl.tag = NFAPI_PUCCH_CONFIG_DELTA_PUCCH_SHIFT_TAG;
  cfg->num_tlv++;

  cfg->pucch_config.n_cqi_rb.value                                  = radioResourceConfigCommonP->pucch_ConfigCommon.nRB_CQI;
  cfg->pucch_config.n_cqi_rb.tl.tag = NFAPI_PUCCH_CONFIG_N_CQI_RB_TAG;
  cfg->num_tlv++;

  cfg->pucch_config.n_an_cs.value                                   = radioResourceConfigCommonP->pucch_ConfigCommon.nCS_AN;
  cfg->pucch_config.n_an_cs.tl.tag = NFAPI_PUCCH_CONFIG_N_AN_CS_TAG;
  cfg->num_tlv++;

  cfg->pucch_config.n1_pucch_an.value                               = radioResourceConfigCommonP->pucch_ConfigCommon.n1PUCCH_AN;
  cfg->pucch_config.n1_pucch_an.tl.tag = NFAPI_PUCCH_CONFIG_N1_PUCCH_AN_TAG;
  cfg->num_tlv++;

  if (radioResourceConfigCommonP->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled == true) {
    cfg->uplink_reference_signal_config.uplink_rs_hopping.value     = 1;
  }
  else if (radioResourceConfigCommonP->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled == true) {
    cfg->uplink_reference_signal_config.uplink_rs_hopping.value     = 2;
  }
  else {
    cfg->uplink_reference_signal_config.uplink_rs_hopping.value     = 0;
  }
  cfg->uplink_reference_signal_config.uplink_rs_hopping.tl.tag = NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG;
  cfg->num_tlv++;

  cfg->uplink_reference_signal_config.group_assignment.value        = radioResourceConfigCommonP->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH;
  cfg->uplink_reference_signal_config.group_assignment.tl.tag = NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG;
  cfg->num_tlv++;

  cfg->uplink_reference_signal_config.cyclic_shift_1_for_drms.value = radioResourceConfigCommonP->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift;
  cfg->uplink_reference_signal_config.cyclic_shift_1_for_drms.tl.tag = NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG;
  cfg->num_tlv++;


  // how to enable/disable SRS?
  if (radioResourceConfigCommonP->soundingRS_UL_ConfigCommon.present==SoundingRS_UL_ConfigCommon_PR_setup) {
    cfg->srs_config.bandwidth_configuration.value                       = radioResourceConfigCommonP->soundingRS_UL_ConfigCommon.choice.setup.srs_BandwidthConfig;
    cfg->srs_config.bandwidth_configuration.tl.tag = NFAPI_SRS_CONFIG_BANDWIDTH_CONFIGURATION_TAG;
    cfg->num_tlv++;

    cfg->srs_config.srs_subframe_configuration.value                    = radioResourceConfigCommonP->soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig;
    cfg->srs_config.srs_subframe_configuration.tl.tag = NFAPI_SRS_CONFIG_SRS_SUBFRAME_CONFIGURATION_TAG;
    cfg->num_tlv++;

    cfg->srs_config.srs_acknack_srs_simultaneous_transmission.value     = radioResourceConfigCommonP->soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission;
    cfg->srs_config.srs_acknack_srs_simultaneous_transmission.tl.tag = NFAPI_SRS_CONFIG_SRS_ACKNACK_SRS_SIMULTANEOUS_TRANSMISSION_TAG;
    cfg->num_tlv++;
    
    if (radioResourceConfigCommonP->soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts) {
       cfg->srs_config.max_up_pts.value                                 = 1;
    }
    else {
       cfg->srs_config.max_up_pts.value                                 = 0;
    }
    cfg->srs_config.max_up_pts.tl.tag = NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG;
    cfg->num_tlv++;
  }

#ifdef Rel14
  if (RC.mac[Mod_idP]->common_channels[CC_idP].mib->message.schedulingInfoSIB1_BR_r13 > 0) {
    AssertFatal(radioResourceConfigCommon_BRP != NULL, "radioResource rou is missing\n");
    AssertFatal(radioResourceConfigCommon_BRP->ext4 != NULL, "ext4 is missing\n");
    cfg->emtc_config.prach_catm_root_sequence_index.value = radioResourceConfigCommon_BRP->prach_Config.rootSequenceIndex;
    cfg->emtc_config.prach_catm_root_sequence_index.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CATM_ROOT_SEQUENCE_INDEX_TAG;
    cfg->num_tlv++;

    cfg->emtc_config.prach_catm_zero_correlation_zone_configuration.value = radioResourceConfigCommon_BRP->prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig;
    cfg->emtc_config.prach_catm_zero_correlation_zone_configuration.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CATM_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG;
    cfg->num_tlv++;

    cfg->emtc_config.prach_catm_high_speed_flag.value = radioResourceConfigCommon_BRP->prach_Config.prach_ConfigInfo.highSpeedFlag;
    cfg->emtc_config.prach_catm_high_speed_flag.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CATM_HIGH_SPEED_FLAG;
    cfg->num_tlv++;

    struct PRACH_ConfigSIB_v1310 *ext4_prach = radioResourceConfigCommon_BRP->ext4->prach_ConfigCommon_v1310;

    PRACH_ParametersListCE_r13_t *prach_ParametersListCE_r13 = &ext4_prach->prach_ParametersListCE_r13;

    PRACH_ParametersCE_r13_t *p;
    cfg->emtc_config.prach_ce_level_0_enable.value = 0;
    cfg->emtc_config.prach_ce_level_0_enable.tl.tag=NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_ENABLE_TAG;
    cfg->num_tlv++;

    cfg->emtc_config.prach_ce_level_1_enable.value = 0;
    cfg->emtc_config.prach_ce_level_1_enable.tl.tag=NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_ENABLE_TAG;
    cfg->num_tlv++;

    cfg->emtc_config.prach_ce_level_2_enable.value = 0;
    cfg->emtc_config.prach_ce_level_2_enable.tl.tag=NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_ENABLE_TAG;
    cfg->num_tlv++;

    cfg->emtc_config.prach_ce_level_3_enable.value = 0;
    cfg->emtc_config.prach_ce_level_3_enable.tl.tag=NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_ENABLE_TAG;
    cfg->num_tlv++;

    switch (prach_ParametersListCE_r13->list.count) {
    case 4:
      p = prach_ParametersListCE_r13->list.array[3];
      cfg->emtc_config.prach_ce_level_3_enable.value = 1;
      cfg->emtc_config.prach_ce_level_3_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_ENABLE_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_3_configuration_index.value               = p->prach_ConfigIndex_r13;
      cfg->emtc_config.prach_ce_level_3_configuration_index.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_CONFIGURATION_INDEX_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_3_frequency_offset.value                  = p->prach_FreqOffset_r13;
      cfg->emtc_config.prach_ce_level_3_frequency_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_FREQUENCY_OFFSET_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt.value = p->numRepetitionPerPreambleAttempt_r13;
      cfg->emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
      cfg->num_tlv++;

      if (p->prach_StartingSubframe_r13) {
	cfg->emtc_config.prach_ce_level_3_starting_subframe_periodicity.value   = *p->prach_StartingSubframe_r13;
        cfg->emtc_config.prach_ce_level_3_starting_subframe_periodicity.tl.tag  = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_STARTING_SUBFRAME_PERIODICITY_TAG;
        cfg->num_tlv++;
      }

      cfg->emtc_config.prach_ce_level_3_hopping_enable.value                    = p->prach_HoppingConfig_r13;
      cfg->emtc_config.prach_ce_level_3_hopping_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_ENABLE_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_3_hopping_offset.value                    = cfg->rf_config.ul_channel_bandwidth.value - 6;
      cfg->emtc_config.prach_ce_level_3_hopping_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_OFFSET_TAG;
      cfg->num_tlv++;

    case 3:
      p = prach_ParametersListCE_r13->list.array[2];
      cfg->emtc_config.prach_ce_level_2_enable.value = 1;
      cfg->emtc_config.prach_ce_level_2_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_ENABLE_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_2_configuration_index.value               = p->prach_ConfigIndex_r13;
      cfg->emtc_config.prach_ce_level_2_configuration_index.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_CONFIGURATION_INDEX_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_2_frequency_offset.value                  = p->prach_FreqOffset_r13;
      cfg->emtc_config.prach_ce_level_2_frequency_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_FREQUENCY_OFFSET_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt.value = p->numRepetitionPerPreambleAttempt_r13;
      cfg->emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
      cfg->num_tlv++;

      if (p->prach_StartingSubframe_r13) {
	cfg->emtc_config.prach_ce_level_2_starting_subframe_periodicity.value   = *p->prach_StartingSubframe_r13;
        cfg->emtc_config.prach_ce_level_2_starting_subframe_periodicity.tl.tag  = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_STARTING_SUBFRAME_PERIODICITY_TAG;
        cfg->num_tlv++;
      }

      cfg->emtc_config.prach_ce_level_2_hopping_enable.value                    = p->prach_HoppingConfig_r13;
      cfg->emtc_config.prach_ce_level_2_hopping_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_ENABLE_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_2_hopping_offset.value                    = cfg->rf_config.ul_channel_bandwidth.value - 6;
      cfg->emtc_config.prach_ce_level_2_hopping_offset.tl.tag                   = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_OFFSET_TAG;
      cfg->num_tlv++;

    case 2:
      p = prach_ParametersListCE_r13->list.array[1];
      cfg->emtc_config.prach_ce_level_1_enable.value = 1;
      cfg->emtc_config.prach_ce_level_1_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_ENABLE_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_1_configuration_index.value               = p->prach_ConfigIndex_r13;
      cfg->emtc_config.prach_ce_level_1_configuration_index.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_CONFIGURATION_INDEX_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_1_frequency_offset.value                  = p->prach_FreqOffset_r13;
      cfg->emtc_config.prach_ce_level_1_frequency_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_FREQUENCY_OFFSET_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt.value = p->numRepetitionPerPreambleAttempt_r13;
      cfg->emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
      cfg->num_tlv++;

      if (p->prach_StartingSubframe_r13) {
	cfg->emtc_config.prach_ce_level_1_starting_subframe_periodicity.value   = *p->prach_StartingSubframe_r13;
        cfg->emtc_config.prach_ce_level_1_starting_subframe_periodicity.tl.tag  = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_STARTING_SUBFRAME_PERIODICITY_TAG;
        cfg->num_tlv++;
      }

      cfg->emtc_config.prach_ce_level_1_hopping_enable.value                    = p->prach_HoppingConfig_r13;
      cfg->emtc_config.prach_ce_level_1_hopping_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_ENABLE_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_1_hopping_offset.value                    = cfg->rf_config.ul_channel_bandwidth.value - 6;
      cfg->emtc_config.prach_ce_level_1_hopping_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_OFFSET_TAG;
      cfg->num_tlv++;

    case 1:
      p = prach_ParametersListCE_r13->list.array[0];
      cfg->emtc_config.prach_ce_level_0_enable.value                            = 1;
      cfg->emtc_config.prach_ce_level_0_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_ENABLE_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_0_configuration_index.value               = p->prach_ConfigIndex_r13;
      cfg->emtc_config.prach_ce_level_0_configuration_index.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_CONFIGURATION_INDEX_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_0_frequency_offset.value                  = p->prach_FreqOffset_r13;
      cfg->emtc_config.prach_ce_level_0_frequency_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_FREQUENCY_OFFSET_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt.value = p->numRepetitionPerPreambleAttempt_r13;
      cfg->emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
      cfg->num_tlv++;

      if (p->prach_StartingSubframe_r13) {
	cfg->emtc_config.prach_ce_level_0_starting_subframe_periodicity.value   = *p->prach_StartingSubframe_r13;
        cfg->emtc_config.prach_ce_level_0_starting_subframe_periodicity.tl.tag  = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_STARTING_SUBFRAME_PERIODICITY_TAG;
        cfg->num_tlv++;
      }

      cfg->emtc_config.prach_ce_level_0_hopping_enable.value                    = p->prach_HoppingConfig_r13;
      cfg->emtc_config.prach_ce_level_0_hopping_enable.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_ENABLE_TAG;
      cfg->num_tlv++;

      cfg->emtc_config.prach_ce_level_0_hopping_offset.value                    = cfg->rf_config.ul_channel_bandwidth.value - 6;
      cfg->emtc_config.prach_ce_level_0_hopping_offset.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_OFFSET_TAG;
      cfg->num_tlv++;
    }

    struct FreqHoppingParameters_r13 *ext4_freqHoppingParameters = radioResourceConfigCommonP->ext4->freqHoppingParameters_r13;
    if ((ext4_freqHoppingParameters) &&
        (ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeA_r13)){
      switch(ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeA_r13->present) {
      case      FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeA_r13_PR_NOTHING:  /* No components present */
        break;
      case FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeA_r13_PR_interval_FDD_r13:
        cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodea.value = ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeA_r13->choice.interval_FDD_r13;
        cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodea.tl.tag = NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEA_TAG;
        cfg->num_tlv++;
        break;
      case FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeA_r13_PR_interval_TDD_r13:
        cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodea.value = ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeA_r13->choice.interval_TDD_r13;
        cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodea.tl.tag = NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEA_TAG;
        cfg->num_tlv++;
        break;
      }
    }
    if ((ext4_freqHoppingParameters) &&
        (ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeB_r13)){
      switch(ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeB_r13->present) {
      case      FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeB_r13_PR_NOTHING:  /* No components present */
        break;
      case FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeB_r13_PR_interval_FDD_r13:
        cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodeb.value = ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeB_r13->choice.interval_FDD_r13;
        cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodeb.tl.tag = NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEB_TAG;
        cfg->num_tlv++;
        break;
      case FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeB_r13_PR_interval_TDD_r13:
        cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodeb.value = ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeB_r13->choice.interval_TDD_r13;
        cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodeb.tl.tag = NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEB_TAG;
        cfg->num_tlv++;
        break;
      }
    }
  }
#endif

}

void
config_dedicated(int Mod_idP,
		 int CC_idP,
		 uint16_t rnti,
		 struct PhysicalConfigDedicated *physicalConfigDedicated)
{

}

void
config_dedicated_scell(int Mod_idP,
		       uint16_t rnti,
		       SCellToAddMod_r10_t * sCellToAddMod_r10)
{

}


int
rrc_mac_config_req_eNB(module_id_t Mod_idP,
		       int CC_idP,
		       int physCellId,
		       int p_eNB,
		       int Ncp, int eutra_band, uint32_t dl_CarrierFreq,
#ifdef Rel14
		       int pbch_repetition,
#endif
		       rnti_t rntiP,
		       BCCH_BCH_Message_t * mib,
		       RadioResourceConfigCommonSIB_t *
		       radioResourceConfigCommon,
#ifdef Rel14
		       RadioResourceConfigCommonSIB_t *
		       radioResourceConfigCommon_BR,
#endif
		       struct PhysicalConfigDedicated
		       *physicalConfigDedicated,
#if defined(Rel10) || defined(Rel14)
		       SCellToAddMod_r10_t * sCellToAddMod_r10,
		       //struct PhysicalConfigDedicatedSCell_r10 *physicalConfigDedicatedSCell_r10,
#endif
		       MeasObjectToAddMod_t ** measObj,
		       MAC_MainConfig_t * mac_MainConfig,
		       long logicalChannelIdentity,
		       LogicalChannelConfig_t * logicalChannelConfig,
		       MeasGapConfig_t * measGapConfig,
		       TDD_Config_t * tdd_Config,
		       MobilityControlInfo_t * mobilityControlInfo,
		       SchedulingInfoList_t * schedulingInfoList,
		       uint32_t ul_CarrierFreq,
		       long *ul_Bandwidth,
		       AdditionalSpectrumEmission_t *
		       additionalSpectrumEmission,
		       struct MBSFN_SubframeConfigList
		       *mbsfn_SubframeConfigList
#if defined(Rel10) || defined(Rel14)
		       , uint8_t MBMS_Flag,
		       MBSFN_AreaInfoList_r9_t * mbsfn_AreaInfoList,
		       PMCH_InfoList_r9_t * pmch_InfoList
#endif
#ifdef Rel14
		       ,
		       SystemInformationBlockType1_v1310_IEs_t *
		       sib1_v13ext
#endif
                       ) {
			   
  int i;

  int UE_id = -1;
  eNB_MAC_INST *eNB = RC.mac[Mod_idP];
  UE_list_t *UE_list= &eNB->UE_list;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_IN);

  LOG_E(MAC, "RC.mac:%p mib:%p\n", RC.mac, mib);

  if (mib != NULL) {
    if (RC.mac == NULL)
      l2_init_eNB();

    mac_top_init_eNB();

    RC.mac[Mod_idP]->common_channels[CC_idP].mib = mib;
    RC.mac[Mod_idP]->common_channels[CC_idP].physCellId = physCellId;
    RC.mac[Mod_idP]->common_channels[CC_idP].p_eNB = p_eNB;
    RC.mac[Mod_idP]->common_channels[CC_idP].Ncp = Ncp;
    RC.mac[Mod_idP]->common_channels[CC_idP].eutra_band = eutra_band;
    RC.mac[Mod_idP]->common_channels[CC_idP].dl_CarrierFreq = dl_CarrierFreq;

    LOG_I(MAC,
	  "Configuring MIB for instance %d, CCid %d : (band %d,N_RB_DL %d,Nid_cell %d,p %d,DL freq %u,phich_config.resource %d, phich_config.duration %d)\n",
	  Mod_idP, 
	  CC_idP, 
	  eutra_band, 
	  to_prb((int)mib->message.dl_Bandwidth), 
	  physCellId, 
	  p_eNB,
	  dl_CarrierFreq,
	  (int)mib->message.phich_Config.phich_Resource,
	  (int)mib->message.phich_Config.phich_Duration);

    config_mib(Mod_idP,CC_idP,
	       eutra_band,
	       mib->message.dl_Bandwidth,
	       &mib->message.phich_Config,
	       physCellId,
	       Ncp,
	       p_eNB,
	       dl_CarrierFreq,
	       ul_CarrierFreq
#ifdef Rel14
	       , pbch_repetition
#endif
	       );

    mac_init_cell_params(Mod_idP,CC_idP);

    if (schedulingInfoList!=NULL)  {
      RC.mac[Mod_idP]->common_channels[CC_idP].tdd_Config         = tdd_Config;    
      RC.mac[Mod_idP]->common_channels[CC_idP].schedulingInfoList = schedulingInfoList;    
      config_sib1(Mod_idP,CC_idP,tdd_Config);
    }
#ifdef Rel14
    if (sib1_v13ext != NULL) {
      RC.mac[Mod_idP]->common_channels[CC_idP].sib1_v13ext = sib1_v13ext;
    }
#endif
    if (radioResourceConfigCommon != NULL) {
      LOG_I(MAC, "[CONFIG]SIB2/3 Contents (partial)\n");
      LOG_I(MAC, "[CONFIG]pusch_config_common.n_SB = %ld\n",
	    radioResourceConfigCommon->
	    pusch_ConfigCommon.pusch_ConfigBasic.n_SB);
      LOG_I(MAC, "[CONFIG]pusch_config_common.hoppingMode = %ld\n",
	    radioResourceConfigCommon->
	    pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode);
      LOG_I(MAC,
	    "[CONFIG]pusch_config_common.pusch_HoppingOffset = %ld\n",
	    radioResourceConfigCommon->
	    pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset);
      LOG_I(MAC, "[CONFIG]pusch_config_common.enable64QAM = %d\n",
	    radioResourceConfigCommon->
	    pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM);
      LOG_I(MAC,
	    "[CONFIG]pusch_config_common.groupHoppingEnabled = %d\n",
	    radioResourceConfigCommon->
	    pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.
	    groupHoppingEnabled);
      LOG_I(MAC,
	    "[CONFIG]pusch_config_common.groupAssignmentPUSCH = %ld\n",
	    radioResourceConfigCommon->
	    pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.
	    groupAssignmentPUSCH);
      LOG_I(MAC,
	    "[CONFIG]pusch_config_common.sequenceHoppingEnabled = %d\n",
	    radioResourceConfigCommon->
	    pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.
	    sequenceHoppingEnabled);
      LOG_I(MAC, "[CONFIG]pusch_config_common.cyclicShift  = %ld\n",
	    radioResourceConfigCommon->
	    pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift);

      AssertFatal(radioResourceConfigCommon->
		  rach_ConfigCommon.maxHARQ_Msg3Tx > 0,
		  "radioResourceconfigCommon %d == 0\n",
		  (int) radioResourceConfigCommon->
		  rach_ConfigCommon.maxHARQ_Msg3Tx);

      RC.mac[Mod_idP]->common_channels[CC_idP].
	radioResourceConfigCommon = radioResourceConfigCommon;
      if (ul_CarrierFreq > 0)
	RC.mac[Mod_idP]->common_channels[CC_idP].ul_CarrierFreq =
	  ul_CarrierFreq;
      if (ul_Bandwidth)
	RC.mac[Mod_idP]->common_channels[CC_idP].ul_Bandwidth =
	  *ul_Bandwidth;
      else
	RC.mac[Mod_idP]->common_channels[CC_idP].ul_Bandwidth =
	  RC.mac[Mod_idP]->common_channels[CC_idP].mib->message.
	  dl_Bandwidth;

      config_sib2(Mod_idP, CC_idP, radioResourceConfigCommon,
#ifdef Rel14
		  radioResourceConfigCommon_BR,
#endif
		  NULL, ul_Bandwidth, additionalSpectrumEmission,
		  mbsfn_SubframeConfigList);


    }
  } // mib != NULL

  // SRB2_lchan_config->choice.explicitValue.ul_SpecificParameters->logicalChannelGroup
  if (logicalChannelConfig != NULL) {	// check for eMTC specific things
    UE_id = find_UE_id(Mod_idP, rntiP);

    if (UE_id == -1) {
      LOG_E(MAC, "%s:%d:%s: ERROR, UE_id == -1\n", __FILE__,
	    __LINE__, __FUNCTION__);
    } else {
      if (logicalChannelConfig)
        UE_list->
          UE_template[CC_idP][UE_id].lcgidmap
          [logicalChannelIdentity] =
          *logicalChannelConfig->
	  ul_SpecificParameters->logicalChannelGroup;
      else
        UE_list->
          UE_template[CC_idP][UE_id].lcgidmap
          [logicalChannelIdentity] = 0;
    }
  }


  if (physicalConfigDedicated != NULL) {
    UE_id = find_UE_id(Mod_idP, rntiP);

    if (UE_id == -1)
      LOG_E(MAC, "%s:%d:%s: ERROR, UE_id == -1\n", __FILE__, __LINE__, __FUNCTION__);
    else
      UE_list->UE_template[CC_idP][UE_id].physicalConfigDedicated = physicalConfigDedicated;
  }


#if defined(Rel10) || defined(Rel14)

  if (sCellToAddMod_r10 != NULL) {
    UE_id = find_UE_id(Mod_idP, rntiP);
    if (UE_id == -1)
      LOG_E(MAC, "%s:%d:%s: ERROR, UE_id == -1\n", __FILE__,
	    __LINE__, __FUNCTION__);
    else
      config_dedicated_scell(Mod_idP, rntiP, sCellToAddMod_r10);
  }
#endif




  if (mbsfn_SubframeConfigList != NULL) {
    LOG_I(MAC,
	  "[eNB %d][CONFIG] Received %d subframe allocation pattern for MBSFN\n",
	  Mod_idP, mbsfn_SubframeConfigList->list.count);
    RC.mac[Mod_idP]->common_channels[0].num_sf_allocation_pattern =
      mbsfn_SubframeConfigList->list.count;

    for (i = 0; i < mbsfn_SubframeConfigList->list.count; i++) {
      RC.mac[Mod_idP]->common_channels[0].mbsfn_SubframeConfig[i] =
	mbsfn_SubframeConfigList->list.array[i];
      LOG_I(MAC,
	    "[eNB %d][CONFIG] MBSFN_SubframeConfig[%d] pattern is  %x\n",
	    Mod_idP, i,
	    RC.mac[Mod_idP]->
	    common_channels[0].mbsfn_SubframeConfig[i]->
	    subframeAllocation.choice.oneFrame.buf[0]);
    }

#ifdef Rel10
    RC.mac[Mod_idP]->common_channels[0].MBMS_flag = MBMS_Flag;
#endif
  }
#if defined(Rel10) || defined(Rel14)

  if (mbsfn_AreaInfoList != NULL) {
    // One eNB could be part of multiple mbsfn syc area, this could change over time so reset each time
    LOG_I(MAC,"[eNB %d][CONFIG] Received %d MBSFN Area Info\n", Mod_idP, mbsfn_AreaInfoList->list.count);
    RC.mac[Mod_idP]->common_channels[0].num_active_mbsfn_area = mbsfn_AreaInfoList->list.count;
    
    for (i =0; i< mbsfn_AreaInfoList->list.count; i++) {
      RC.mac[Mod_idP]->common_channels[0].mbsfn_AreaInfo[i] = mbsfn_AreaInfoList->list.array[i];
      LOG_I(MAC,"[eNB %d][CONFIG] MBSFN_AreaInfo[%d]: MCCH Repetition Period = %ld\n", Mod_idP,i,
	    RC.mac[Mod_idP]->common_channels[0].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_RepetitionPeriod_r9);
      //      config_sib13(Mod_idP,0,i,RC.mac[Mod_idP]->common_channels[0].mbsfn_AreaInfo[i]->mbsfn_AreaId_r9);
    }
  } 

  if (pmch_InfoList != NULL) {
    
    //    LOG_I(MAC,"DUY: lcid when entering rrc_mac config_req is %02d\n",(pmch_InfoList->list.array[0]->mbms_SessionInfoList_r9.list.array[0]->logicalChannelIdentity_r9));
    
    LOG_I(MAC, "[CONFIG] Number of PMCH in this MBSFN Area %d\n",
	  pmch_InfoList->list.count);
    
    for (i = 0; i < pmch_InfoList->list.count; i++) {
      RC.mac[Mod_idP]->common_channels[0].pmch_Config[i] =
	&pmch_InfoList->list.array[i]->pmch_Config_r9;
      
      LOG_I(MAC,
	    "[CONFIG] PMCH[%d]: This PMCH stop (sf_AllocEnd_r9) at subframe  %ldth\n",
	    i,
	    RC.mac[Mod_idP]->common_channels[0].
	    pmch_Config[i]->sf_AllocEnd_r9);
      LOG_I(MAC, "[CONFIG] PMCH[%d]: mch_Scheduling_Period = %ld\n",
	    i,
	    RC.mac[Mod_idP]->common_channels[0].
	    pmch_Config[i]->mch_SchedulingPeriod_r9);
      LOG_I(MAC, "[CONFIG] PMCH[%d]: dataMCS = %ld\n", i,
	    RC.mac[Mod_idP]->common_channels[0].
	    pmch_Config[i]->dataMCS_r9);
      
      // MBMS session info list in each MCH
      RC.mac[Mod_idP]->common_channels[0].mbms_SessionList[i] =
	&pmch_InfoList->list.array[i]->mbms_SessionInfoList_r9;
      LOG_I(MAC, "PMCH[%d] Number of session (MTCH) is: %d\n", i,
	    RC.mac[Mod_idP]->common_channels[0].
	    mbms_SessionList[i]->list.count);
    }
  }
  
#endif
    
    LOG_E(MAC, "%s() %s:%d RC.mac[Mod_idP]->if_inst->PHY_config_req:%p\n", __FUNCTION__, __FILE__, __LINE__, RC.mac[Mod_idP]->if_inst->PHY_config_req);

    // if in nFAPI mode 
    if (
        (nfapi_mode == 1 || nfapi_mode == 2) &&
        (RC.mac[Mod_idP]->if_inst->PHY_config_req == NULL)
       ) {
      while(RC.mac[Mod_idP]->if_inst->PHY_config_req == NULL) {
        // DJP AssertFatal(RC.mac[Mod_idP]->if_inst->PHY_config_req != NULL,"if_inst->phy_config_request is null\n");
        usleep(100 * 1000);
        printf("Waiting for PHY_config_req\n");
      }
    }

    if (radioResourceConfigCommon != NULL) {
      PHY_Config_t phycfg;
      phycfg.Mod_id = Mod_idP;
      phycfg.CC_id  = CC_idP;
      phycfg.cfg    = &RC.mac[Mod_idP]->config[CC_idP];
      
      if (RC.mac[Mod_idP]->if_inst->PHY_config_req) RC.mac[Mod_idP]->if_inst->PHY_config_req(&phycfg); 
      
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_OUT);
    }
    
    return(0);			   
}
