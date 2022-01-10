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
#include "LTE_SystemInformationBlockType2.h"
//#include "RadioResourceConfigCommonSIB.h"
#include "LTE_RadioResourceConfigDedicated.h"
#include "LTE_PRACH-ConfigSIB-v1310.h"
#include "LTE_MeasGapConfig.h"
#include "LTE_MeasObjectToAddModList.h"
#include "LTE_TDD-Config.h"
#include "LTE_MAC-MainConfig.h"
#include "mac.h"
#include "mac_proto.h"
#include "mac_extern.h"
#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "common/ran_context.h"
#include "LTE_MBSFN-AreaInfoList-r9.h"
#include "LTE_MBSFN-AreaInfo-r9.h"
#include "LTE_MBSFN-SubframeConfigList.h"
#include "LTE_MBSFN-SubframeConfig.h"
#include "LTE_PMCH-InfoList-r9.h"
#include <openair2/LAYER2/MAC/mac_proto.h>

extern RAN_CONTEXT_t RC;
extern int l2_init_eNB(void);
extern void mac_top_init_eNB(void);
extern void mac_init_cell_params(int Mod_idP,int CC_idP);


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
  {1,  19200, 19800, 21100, 21700, 0     },
  {2,  18500, 19100, 19300, 19900, 6000  },
  {3,  17100, 17850, 18050, 18800, 12000 },
  {4,  17100, 17550, 21100, 21550, 19500 },
  {5,  8240,  8490,  8690,  8940,  24000 },
  {6,  8300,  8400,  8750,  8850,  26500 },
  {7,  25000, 25700, 26200, 26900, 27500 },
  {8,  8800,  9150,  9250,  9600,  34500 },
  {9,  17499, 17849, 18449, 18799, 38000 },
  {10, 17100, 17700, 21100, 21700, 41500 },
  {11, 14279, 14529, 14759, 15009, 47500 },
  {12, 6980,  7160,  7280,  7460,  50100 },
  {13, 7770,  7870,  7460,  7560,  51800 },
  {14, 7880,  7980,  7580,  7680,  52800 },
  {17, 7040,  7160,  7340,  7460,  57300 },
  {18, 8150,  9650,  8600,  10100, 58500 },
  {19, 8300,  8450,  8750,  8900,  60000 },
  {20, 8320,  8620,  7910,  8210,  61500 },
  {21, 14479, 14629, 14959, 15109, 64500 },
  {22, 34100, 34900, 35100, 35900, 66000 },
  {23, 20000, 20200, 21800, 22000, 75000 },
  {24, 16126, 16605, 15250, 15590, 77000 },
  {25, 18500, 19150, 19300, 19950, 80400 },
  {26, 8140,  8490,  8590,  8940,  86900 },
  {27, 8070,  8240,  8520,  8690,  90400 },
  {28, 7030,  7580,  7580,  8130,  92100 },
  {29, 0,     0,     7170,  7280,  96600 },
  {30, 23050, 23250, 23500, 23600, 97700 },
  {31, 45250, 34900, 46250, 35900, 98700 },
  {32, 0,     0,     14520, 14960, 99200 },
  {33, 19000, 19200, 19000, 19200, 360000},
  {34, 20100, 20250, 20100, 20250, 362000},
  {35, 18500, 19100, 18500, 19100, 363500},
  {36, 19300, 19900, 19300, 19900, 369500},
  {37, 19100, 19300, 19100, 19300, 375500},
  {38, 25700, 26200, 25700, 26300, 377500},
  {39, 18800, 19200, 18800, 19200, 382500},
  {40, 23000, 24000, 23000, 24000, 386500},
  {41, 24960, 26900, 24960, 26900, 396500},
  {42, 34000, 36000, 34000, 36000, 415900},
  {43, 36000, 38000, 36000, 38000, 435900},
  {44, 7030,  8030,  7030,  8030,  455900},
  {45, 14470, 14670, 14470, 14670, 465900},
  {46, 51500, 59250, 51500, 59250, 467900},
  {65, 19200, 20100, 21100, 22000, 655360},
  {66, 17100, 18000, 21100, 22000, 664360},
  {67, 0,     0,     7380,  7580,  67336 },
  {68, 6980,  7280,  7530,  7830,  67536 }
};


#define BANDTABLE_SIZE (sizeof(eutra_bandtable)/sizeof(eutra_bandentry_t))

uint32_t to_earfcn(int eutra_bandP, uint32_t dl_CarrierFreq, uint32_t bw) {
  uint32_t dl_CarrierFreq_by_100k = dl_CarrierFreq / 100000;
  int bw_by_100 = bw / 100;
  int i;
  AssertFatal(eutra_bandP < 69, "eutra_band %d > 68\n", eutra_bandP);

  for (i = 0; i < BANDTABLE_SIZE && eutra_bandtable[i].band != eutra_bandP; i++);

  AssertFatal(i < BANDTABLE_SIZE, "i %d >= BANDTABLE_SIZE %ld\n", i, BANDTABLE_SIZE);
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

uint32_t to_earfcn_DL(int eutra_bandP, long long int dl_CarrierFreq, uint32_t bw) {
  uint32_t dl_CarrierFreq_by_100k = dl_CarrierFreq / 100000;
  int bw_by_100 = bw / 100;
  int i;
  AssertFatal(eutra_bandP < 69, "eutra_band %d > 68\n", eutra_bandP);

  for (i = 0; i < BANDTABLE_SIZE && eutra_bandtable[i].band != eutra_bandP; i++);

  AssertFatal(i < BANDTABLE_SIZE, "i = %d , it will trigger out-of-bounds read.\n",i);
  AssertFatal(dl_CarrierFreq_by_100k >= eutra_bandtable[i].dl_min,
              "Band %d, bw %u : DL carrier frequency %lld Hz < %u\n",
              eutra_bandP, bw, dl_CarrierFreq,
              eutra_bandtable[i].dl_min);
  AssertFatal(dl_CarrierFreq_by_100k <=
              (eutra_bandtable[i].dl_max - bw_by_100),
              "Band %d, bw %u : DL carrier frequency %lld Hz > %d\n",
              eutra_bandP, bw, dl_CarrierFreq,
              eutra_bandtable[i].dl_max - bw_by_100);
  return (dl_CarrierFreq_by_100k - eutra_bandtable[i].dl_min +
          (eutra_bandtable[i].N_OFFs_DL / 10));
}

uint32_t to_earfcn_UL(int eutra_bandP, long long int ul_CarrierFreq, uint32_t bw) {
  uint32_t ul_CarrierFreq_by_100k = ul_CarrierFreq / 100000;
  int bw_by_100 = bw / 100;
  int i;
  AssertFatal(eutra_bandP < 69, "eutra_band %d > 68\n", eutra_bandP);

  for (i = 0; i < BANDTABLE_SIZE && eutra_bandtable[i].band != eutra_bandP; i++);

  AssertFatal(i < BANDTABLE_SIZE, "i = %d , it will trigger out-of-bounds read.\n",i);
  AssertFatal(ul_CarrierFreq_by_100k >= eutra_bandtable[i].ul_min,
              "Band %d, bw %u : UL carrier frequency %lld Hz < %u\n",
              eutra_bandP, bw, ul_CarrierFreq,
              eutra_bandtable[i].ul_min);
  AssertFatal(ul_CarrierFreq_by_100k <=
              (eutra_bandtable[i].ul_max - bw_by_100),
              "Band %d, bw %u : UL carrier frequency %lld Hz > %d\n",
              eutra_bandP, bw, ul_CarrierFreq,
              eutra_bandtable[i].ul_max - bw_by_100);
  return (ul_CarrierFreq_by_100k - eutra_bandtable[i].ul_min +
          ((eutra_bandtable[i].N_OFFs_DL + 180000) / 10));
}

uint32_t from_earfcn(int eutra_bandP, uint32_t dl_earfcn) {
  int i;
  AssertFatal(eutra_bandP < 69, "eutra_band %d > 68\n", eutra_bandP);

  for (i = 0; i < BANDTABLE_SIZE && eutra_bandtable[i].band != eutra_bandP; i++);

  AssertFatal(i < BANDTABLE_SIZE, "i %d >= BANDTABLE_SIZE %ld\n", i, BANDTABLE_SIZE);
  return (eutra_bandtable[i].dl_min +
          (dl_earfcn - (eutra_bandtable[i].N_OFFs_DL / 10))) * 100000;
}


int32_t get_uldl_offset(int eutra_bandP) {
  int i;

  for (i = 0; i < BANDTABLE_SIZE && eutra_bandtable[i].band != eutra_bandP; i++);

  AssertFatal(i < BANDTABLE_SIZE, "i %d >= BANDTABLE_SIZE %ld\n", i, BANDTABLE_SIZE);
  return (eutra_bandtable[i].dl_min - eutra_bandtable[i].ul_min);
}

uint32_t bw_table[6] = {6*180,15*180,25*180,50*180,75*180,100*180};

void config_mib(int                 Mod_idP,
                int                 CC_idP,
                int                 eutra_bandP,
                int                 dl_BandwidthP,
                LTE_PHICH_Config_t  *phich_configP,
                int                 Nid_cellP,
                int                 NcpP,
                int                 p_eNBP,
                uint32_t            dl_CarrierFreqP,
                uint32_t            ul_CarrierFreqP,
                uint32_t            pbch_repetitionP
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
  LOG_D(PHY,"%s() dl_BandwidthP:%d\n", __FUNCTION__, dl_BandwidthP);
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
  cfg->emtc_config.pbch_repetitions_enable_r13.value                 = pbch_repetitionP;
  cfg->emtc_config.pbch_repetitions_enable_r13.tl.tag = NFAPI_EMTC_CONFIG_PBCH_REPETITIONS_ENABLE_R13_TAG;
  cfg->num_tlv++;
  LOG_I(MAC,
        "%s() NFAPI_CONFIG_REQUEST(num_tlv:%u) DL_BW:%u UL_BW:%u Ncp %d,p_eNB %d,earfcn %d,band %d,phich_resource %u phich_duration %u phich_power_offset %u PSS %d SSS %d PCI %d"
        " PBCH repetition %d"
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
        ,cfg->emtc_config.pbch_repetitions_enable_r13.value
       );
}

void config_sib1(int Mod_idP, int CC_idP, LTE_TDD_Config_t *tdd_ConfigP) {
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
  } else { // FDD
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
            LTE_RadioResourceConfigCommonSIB_t *radioResourceConfigCommonP,
            LTE_RadioResourceConfigCommonSIB_t *radioResourceConfigCommon_BRP,
            LTE_ARFCN_ValueEUTRA_t *ul_CArrierFreqP,
            long *ul_BandwidthP,
            LTE_AdditionalSpectrumEmission_t *additionalSpectrumEmissionP,
            struct LTE_MBSFN_SubframeConfigList  *mbsfn_SubframeConfigListP,
            int dl_Bandwidth) {
  nfapi_config_request_t *cfg = &RC.mac[Mod_idP]->config[CC_idP];
  cfg->subframe_config.pb.value               = radioResourceConfigCommonP->pdsch_ConfigCommon.p_b;
  cfg->subframe_config.pb.tl.tag = NFAPI_SUBFRAME_CONFIG_PB_TAG;
  cfg->num_tlv++;
  cfg->rf_config.reference_signal_power.value = radioResourceConfigCommonP->pdsch_ConfigCommon.referenceSignalPower;
  cfg->rf_config.reference_signal_power.tl.tag = NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG;
  cfg->num_tlv++;
  cfg->nfapi_config.max_transmit_power.value  = cfg->rf_config.reference_signal_power.value + power_off_dB[dl_Bandwidth];
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
  } else if (radioResourceConfigCommonP->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled == true) {
    cfg->uplink_reference_signal_config.uplink_rs_hopping.value     = 2;
  } else {
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
  if (radioResourceConfigCommonP->soundingRS_UL_ConfigCommon.present== LTE_SoundingRS_UL_ConfigCommon_PR_setup) {
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
    } else {
      cfg->srs_config.max_up_pts.value                                 = 0;
    }

    cfg->srs_config.max_up_pts.tl.tag = NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG;
    cfg->num_tlv++;
  }

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
    struct LTE_PRACH_ConfigSIB_v1310 *ext4_prach = radioResourceConfigCommon_BRP->ext4->prach_ConfigCommon_v1310;
    LTE_PRACH_ParametersListCE_r13_t *prach_ParametersListCE_r13 = &ext4_prach->prach_ParametersListCE_r13;
    LTE_PRACH_ParametersCE_r13_t *p;
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
        cfg->emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt.value = 1<<p->numRepetitionPerPreambleAttempt_r13;
        cfg->emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
        cfg->num_tlv++;

        if (p->prach_StartingSubframe_r13) {
          cfg->emtc_config.prach_ce_level_3_starting_subframe_periodicity.value   = 2<<*p->prach_StartingSubframe_r13;
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
        cfg->emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt.value = 1<<p->numRepetitionPerPreambleAttempt_r13;
        cfg->emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
        cfg->num_tlv++;

        if (p->prach_StartingSubframe_r13) {
          cfg->emtc_config.prach_ce_level_2_starting_subframe_periodicity.value   = 2<<*p->prach_StartingSubframe_r13;
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
        cfg->emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt.value = 1<<p->numRepetitionPerPreambleAttempt_r13;
        cfg->emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
        cfg->num_tlv++;

        if (p->prach_StartingSubframe_r13) {
          cfg->emtc_config.prach_ce_level_1_starting_subframe_periodicity.value   = 2<<*p->prach_StartingSubframe_r13;
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
        cfg->emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt.value = 1<<p->numRepetitionPerPreambleAttempt_r13;
        cfg->emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt.tl.tag = NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG;
        cfg->num_tlv++;

        if (p->prach_StartingSubframe_r13) {
          cfg->emtc_config.prach_ce_level_0_starting_subframe_periodicity.value   = 2<<*p->prach_StartingSubframe_r13;
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

    AssertFatal(cfg->emtc_config.prach_ce_level_0_enable.value>0,"CE_level0 is not enabled\n");
    struct LTE_FreqHoppingParameters_r13 *ext4_freqHoppingParameters = radioResourceConfigCommonP->ext4->freqHoppingParameters_r13;

    if ((ext4_freqHoppingParameters) &&
        (ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeA_r13)) {
      switch(ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeA_r13->present) {
        case LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeA_r13_PR_NOTHING:  /* No components present */
          break;

        case LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeA_r13_PR_interval_FDD_r13:
          cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodea.value = ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeA_r13->choice.interval_FDD_r13;
          cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodea.tl.tag = NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEA_TAG;
          cfg->num_tlv++;
          break;

        case LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeA_r13_PR_interval_TDD_r13:
          cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodea.value = ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeA_r13->choice.interval_TDD_r13;
          cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodea.tl.tag = NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEA_TAG;
          cfg->num_tlv++;
          break;
      }
    }

    if ((ext4_freqHoppingParameters) &&
        (ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeB_r13)) {
      switch(ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeB_r13->present) {
        case LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeB_r13_PR_NOTHING:  /* No components present */
          break;

        case LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeB_r13_PR_interval_FDD_r13:
          cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodeb.value = ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeB_r13->choice.interval_FDD_r13;
          cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodeb.tl.tag = NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEB_TAG;
          cfg->num_tlv++;
          break;

        case LTE_FreqHoppingParameters_r13__interval_ULHoppingConfigCommonModeB_r13_PR_interval_TDD_r13:
          cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodeb.value = ext4_freqHoppingParameters->interval_ULHoppingConfigCommonModeB_r13->choice.interval_TDD_r13;
          cfg->emtc_config.pucch_interval_ulhoppingconfigcommonmodeb.tl.tag = NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEB_TAG;
          cfg->num_tlv++;
          break;
      }
    }
  }
}

void
config_sib2_mbsfn_part( int Mod_idP,
              int CC_idP,
            struct LTE_MBSFN_SubframeConfigList  *mbsfn_SubframeConfigListP) {

  //LTE_DL_FRAME_PARMS *fp = &RC.eNB[Mod_idP][CC_idP]->frame_parms;
  //int i;
  //if(mbsfn_SubframeConfigListP != NULL) {
  //  fp->num_MBSFN_config = mbsfn_SubframeConfigListP->list.count;

  //  for(i = 0; i < mbsfn_SubframeConfigListP->list.count; i++) {
  //    fp->MBSFN_config[i].radioframeAllocationPeriod = mbsfn_SubframeConfigListP->list.array[i]->radioframeAllocationPeriod;
  //    fp->MBSFN_config[i].radioframeAllocationOffset = mbsfn_SubframeConfigListP->list.array[i]->radioframeAllocationOffset;

  //    if (mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) {
  //      fp->MBSFN_config[i].fourFrames_flag = 0;
  //      fp->MBSFN_config[i].mbsfn_SubframeConfig = mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.choice.oneFrame.buf[0];  // 6-bit subframe configuration
  //      LOG_I (PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %d\n", i, fp->MBSFN_config[i].mbsfn_SubframeConfig);
  //    } else if (mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames) {       // 24-bit subframe configuration
  //      fp->MBSFN_config[i].fourFrames_flag = 1;
  //      fp->MBSFN_config[i].mbsfn_SubframeConfig =
  //        mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.choice.oneFrame.buf[2]|
  //        (mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.choice.oneFrame.buf[1]<<8)|
  //        (mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]<<16);

  //      LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %x\n", i,
  //            fp->MBSFN_config[i].mbsfn_SubframeConfig);
  //    }
  //  }

  //} else
  //  fp->num_MBSFN_config = 0;

   PHY_Config_t phycfg;
   phycfg.Mod_id = Mod_idP;
   phycfg.CC_id  = CC_idP;
   phycfg.cfg    = &RC.mac[Mod_idP]->config[CC_idP];
  int i;

  if(mbsfn_SubframeConfigListP != NULL) {
    phycfg.cfg->embms_mbsfn_config.num_mbsfn_config = mbsfn_SubframeConfigListP->list.count;

    for(i = 0; i < mbsfn_SubframeConfigListP->list.count; i++) {
       phycfg.cfg->embms_mbsfn_config.radioframe_allocation_period[i] = mbsfn_SubframeConfigListP->list.array[i]->radioframeAllocationPeriod;
       phycfg.cfg->embms_mbsfn_config.radioframe_allocation_offset[i] = mbsfn_SubframeConfigListP->list.array[i]->radioframeAllocationOffset;

      if (mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) {
        phycfg.cfg->embms_mbsfn_config.fourframes_flag[i] = 0;
        phycfg.cfg->embms_mbsfn_config.mbsfn_subframeconfig[i] = mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.choice.oneFrame.buf[0];  // 6-bit subframe configuration
        LOG_I (MAC, "[CONFIG] MBSFN_SubframeConfig[%d] oneFrame pattern is  %d\n", i, phycfg.cfg->embms_mbsfn_config.mbsfn_subframeconfig[i]);
      } else if (mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames) {       // 24-bit subframe configuration
        phycfg.cfg->embms_mbsfn_config.fourframes_flag[i]  = 1;
        phycfg.cfg->embms_mbsfn_config.mbsfn_subframeconfig[i] =
          mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.choice.oneFrame.buf[2]|
          (mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.choice.oneFrame.buf[1]<<8)|
          (mbsfn_SubframeConfigListP->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]<<16);

        LOG_I(MAC, "[CONFIG] MBSFN_SubframeConfig[%d] fourFrame pattern is  %x\n", i,
              phycfg.cfg->embms_mbsfn_config.mbsfn_subframeconfig[i]);
      }
    }
    phycfg.cfg->num_tlv++;

  } else{
    phycfg.cfg->embms_mbsfn_config.num_mbsfn_config = 0;
    phycfg.cfg->num_tlv++;
  }

  phycfg.cfg->embms_mbsfn_config.tl.tag = NFAPI_EMBMS_MBSFN_CONFIG_TAG;

   if (RC.mac[Mod_idP]->if_inst->PHY_config_update_sib2_req) RC.mac[Mod_idP]->if_inst->PHY_config_update_sib2_req(&phycfg);
}

void
config_sib13( int Mod_id,
              int CC_id,
              int mbsfn_Area_idx,
             long mbsfn_AreaId_r9){

  //nfapi_config_request_t *cfg = &RC.mac[Mod_id]->config[CC_id];

  //work around until PHY_config_re "update" mechanisms get defined
//  LTE_DL_FRAME_PARMS *fp = &RC.eNB[Mod_id][CC_id]->frame_parms;
//  LOG_I (MAC, "[eNB%d] Applying MBSFN_Area_id %ld for index %d\n", Mod_id, mbsfn_AreaId_r9, mbsfn_Area_idx);
//
//  AssertFatal(mbsfn_Area_idx == 0, "Fix me: only called when mbsfn_Area_idx == 0\n");
//  if (mbsfn_Area_idx == 0) {
//    fp->Nid_cell_mbsfn = (uint16_t)mbsfn_AreaId_r9;
//    LOG_I(MAC,"Fix me: only called when mbsfn_Area_idx == 0)\n");
//  }
//  lte_gold_mbsfn (fp, RC.eNB[Mod_id][CC_id]->lte_gold_mbsfn_table, fp->Nid_cell_mbsfn);
//
//  lte_gold_mbsfn_khz_1dot25 (fp, RC.eNB[Mod_id][CC_id]->lte_gold_mbsfn_khz_1dot25_table, fp->Nid_cell_mbsfn);
//
   PHY_Config_t phycfg;
   phycfg.Mod_id = Mod_id;
   phycfg.CC_id  = CC_id;
   phycfg.cfg    = &RC.mac[Mod_id]->config[CC_id];

   phycfg.cfg->embms_sib13_config.mbsfn_area_idx.value =  (uint8_t)mbsfn_Area_idx;
   phycfg.cfg->embms_sib13_config.mbsfn_area_idx.tl.tag =  NFAPI_EMBMS_MBSFN_CONFIG_AREA_IDX_TAG;
   phycfg.cfg->num_tlv++;
   phycfg.cfg->embms_sib13_config.mbsfn_area_id_r9.value = (uint32_t)mbsfn_AreaId_r9;
   phycfg.cfg->embms_sib13_config.mbsfn_area_id_r9.tl.tag = NFAPI_EMBMS_MBSFN_CONFIG_AREA_IDR9_TAG;
   phycfg.cfg->num_tlv++;

   if (RC.mac[Mod_id]->if_inst->PHY_config_update_sib13_req) RC.mac[Mod_id]->if_inst->PHY_config_update_sib13_req(&phycfg);

//    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_OUT);

}




void
config_dedicated(int Mod_idP,
                 int CC_idP,
                 uint16_t rnti,
                 struct LTE_PhysicalConfigDedicated *physicalConfigDedicated) {
}

void
config_dedicated_scell(int Mod_idP,
                       uint16_t rnti,
                       LTE_SCellToAddMod_r10_t *sCellToAddMod_r10) {
}


int rrc_mac_config_req_eNB(module_id_t Mod_idP,
                           int CC_idP,
                           int physCellId,
                           int p_eNB,
                           int Ncp, int eutra_band, uint32_t dl_CarrierFreq,
                           int pbch_repetition,
                           rnti_t rntiP,
                           LTE_BCCH_BCH_Message_t *mib,
                           LTE_RadioResourceConfigCommonSIB_t *
                           radioResourceConfigCommon,
                           LTE_RadioResourceConfigCommonSIB_t *radioResourceConfigCommon_BR,
                           struct LTE_PhysicalConfigDedicated
                           *physicalConfigDedicated,
                           LTE_SCellToAddMod_r10_t *sCellToAddMod_r10,
                           LTE_MeasObjectToAddMod_t **measObj,
                           LTE_MAC_MainConfig_t *mac_MainConfig,
                           long logicalChannelIdentity,
                           LTE_LogicalChannelConfig_t *logicalChannelConfig,
                           LTE_MeasGapConfig_t *measGapConfig,
                           LTE_TDD_Config_t *tdd_Config,
                           LTE_MobilityControlInfo_t *mobilityControlInfo,
                           LTE_SchedulingInfoList_t *schedulingInfoList,
                           uint32_t ul_CarrierFreq,
                           long *ul_Bandwidth,
                           LTE_AdditionalSpectrumEmission_t *
                           additionalSpectrumEmission,
                           struct LTE_MBSFN_SubframeConfigList
                           *mbsfn_SubframeConfigList,
                           uint8_t MBMS_Flag,
                           LTE_MBSFN_AreaInfoList_r9_t *mbsfn_AreaInfoList,
                           LTE_PMCH_InfoList_r9_t *pmch_InfoList,
                           LTE_SystemInformationBlockType1_v1310_IEs_t *sib1_v13ext,
                           uint8_t FeMBMS_Flag,
                           LTE_BCCH_DL_SCH_Message_MBMS_t *mib_fembms,
                           LTE_SchedulingInfo_MBMS_r14_t *schedulingInfo_fembms,
                           struct LTE_NonMBSFN_SubframeConfig_r14 *nonMBSFN_SubframeConfig,
                           LTE_SystemInformationBlockType1_MBMS_r14_t   *sib1_mbms_r14_fembms,
                           LTE_MBSFN_AreaInfoList_r9_t *mbsfn_AreaInfoList_fembms,
   		           LTE_MBSFNAreaConfiguration_r9_t*mbms_AreaConfig
                          ) {
  int i;
  int UE_id = -1;
  eNB_MAC_INST *eNB = RC.mac[Mod_idP];
  UE_info_t *UE_info= &eNB->UE_info;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_IN);
  LOG_D(MAC, "RC.mac:%p mib:%p\n", RC.mac, mib);

  if (mib != NULL) {
    if (RC.mac == NULL)
      l2_init_eNB();

    //mac_top_init_eNB();
    RC.mac[Mod_idP]->common_channels[CC_idP].mib = mib;
    RC.mac[Mod_idP]->common_channels[CC_idP].physCellId = physCellId;
    RC.mac[Mod_idP]->common_channels[CC_idP].p_eNB = p_eNB;
    RC.mac[Mod_idP]->common_channels[CC_idP].Ncp = Ncp;
    RC.mac[Mod_idP]->common_channels[CC_idP].eutra_band = eutra_band;
    RC.mac[Mod_idP]->common_channels[CC_idP].dl_CarrierFreq = dl_CarrierFreq;
    LOG_A(MAC,
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
               ul_CarrierFreq,
               pbch_repetition
              );
    mac_init_cell_params(Mod_idP,CC_idP);

    if (schedulingInfoList!=NULL)  {
      RC.mac[Mod_idP]->common_channels[CC_idP].tdd_Config         = tdd_Config;
      RC.mac[Mod_idP]->common_channels[CC_idP].schedulingInfoList = schedulingInfoList;
      config_sib1(Mod_idP,CC_idP,tdd_Config);
    }

    //TODO MBMS this must be passed through function
    /*if (schedulingInfoList_MBMS!=NULL)  {
      RC.mac[Mod_idP]->common_channels[CC_idP].schedulingInfoList_MBMS = schedulingInfoList_MBMS;
      config_sib1_mbms(Mod_idP,CC_idP,tdd_Config);
    }*/

    if (sib1_v13ext != NULL) {
      RC.mac[Mod_idP]->common_channels[CC_idP].sib1_v13ext = sib1_v13ext;
    }

    AssertFatal(radioResourceConfigCommon != NULL, "radioResourceConfigCommon is null\n");
    LOG_I(MAC, "[CONFIG]SIB2/3 Contents (partial)\n");
    LOG_I(MAC, "[CONFIG]pusch_config_common.n_SB = %ld\n",
          radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.n_SB);
    LOG_I(MAC, "[CONFIG]pusch_config_common.hoppingMode = %ld\n",
          radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode);
    LOG_I(MAC, "[CONFIG]pusch_config_common.pusch_HoppingOffset = %ld\n",
          radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset);
    LOG_I(MAC, "[CONFIG]pusch_config_common.enable64QAM = %d\n",
          radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM);
    LOG_I(MAC, "[CONFIG]pusch_config_common.groupHoppingEnabled = %d\n",
          radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled);
    LOG_I(MAC, "[CONFIG]pusch_config_common.groupAssignmentPUSCH = %ld\n",
          radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH);
    LOG_I(MAC, "[CONFIG]pusch_config_common.sequenceHoppingEnabled = %d\n",
          radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled);
    LOG_I(MAC, "[CONFIG]pusch_config_common.cyclicShift  = %ld\n",
          radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift);
    AssertFatal(radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx > 0,
                "radioResourceconfigCommon %d == 0\n",
                (int) radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx);
    RC.mac[Mod_idP]->common_channels[CC_idP].radioResourceConfigCommon = radioResourceConfigCommon;
    RC.mac[Mod_idP]->common_channels[CC_idP].radioResourceConfigCommon_BR = radioResourceConfigCommon_BR;

    if (ul_CarrierFreq > 0) RC.mac[Mod_idP]->common_channels[CC_idP].ul_CarrierFreq = ul_CarrierFreq;

    if (ul_Bandwidth) RC.mac[Mod_idP]->common_channels[CC_idP].ul_Bandwidth = *ul_Bandwidth;
    else RC.mac[Mod_idP]->common_channels[CC_idP].ul_Bandwidth = RC.mac[Mod_idP]->common_channels[CC_idP].mib->message.dl_Bandwidth;

    config_sib2(Mod_idP, CC_idP, radioResourceConfigCommon,
                radioResourceConfigCommon_BR,
                NULL, ul_Bandwidth, additionalSpectrumEmission,
                mbsfn_SubframeConfigList,
                mib->message.dl_Bandwidth);
  } // mib != NULL

  if (mobilityControlInfo !=NULL) {
    if ((UE_id = add_new_ue(Mod_idP, CC_idP,
                            rntiP, -1,
                            0
                           )) == -1) {
      LOG_E(MAC, "%s:%d: fatal\n", __FILE__, __LINE__);
      abort();
    }
  }

  if (logicalChannelIdentity > 0) { // is SRB1,2 or DRB
    if ((UE_id = find_UE_id(Mod_idP, rntiP)) < 0) {
      LOG_E(MAC,"Configuration received for unknown UE (%x), shouldn't happen\n",rntiP);
      return(-1);
    }
    int idx = -1;
    UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    for (int i = 0; i < sched_ctrl->dl_lc_num; ++i) {
      if (sched_ctrl->dl_lc_ids[i] == logicalChannelIdentity) {
        /* TODO this might also mean we have to remove it, not clear */
        idx = i;
        break;
      }
    }
    if (idx < 0) {
      sched_ctrl->dl_lc_num++;
      sched_ctrl->dl_lc_ids[sched_ctrl->dl_lc_num-1] = logicalChannelIdentity;
      sched_ctrl->dl_lc_bytes[sched_ctrl->dl_lc_num-1] = 0;
      LOG_I(MAC, "UE %d RNTI %x adding LC %ld idx %d to scheduling control (total %d)\n", UE_id, rntiP, logicalChannelIdentity, sched_ctrl->dl_lc_num-1, sched_ctrl->dl_lc_num);
      if (logicalChannelIdentity == 1) { // if it is SRB1, add SRB2 directly because RRC does not indicate this separately
        sched_ctrl->dl_lc_num++;
        sched_ctrl->dl_lc_ids[sched_ctrl->dl_lc_num-1] = 2;
        sched_ctrl->dl_lc_bytes[sched_ctrl->dl_lc_num-1] = 0;
        LOG_I(MAC, "UE %d RNTI %x adding LC 2 idx %d to scheduling control (total %d)\n", UE_id, rntiP, sched_ctrl->dl_lc_num-1, sched_ctrl->dl_lc_num);
      }
    }
  }

  // SRB2_lchan_config->choice.explicitValue.ul_SpecificParameters->logicalChannelGroup
  if (logicalChannelConfig != NULL) { // check for eMTC specific things
    UE_id = find_UE_id(Mod_idP, rntiP);

    if (UE_id<0) {
      LOG_E(MAC,"Configuration received for unknown UE (%x), shouldn't happen\n",rntiP);
      return(-1);
    }

    if (logicalChannelConfig) {
      UE_info->UE_template[CC_idP][UE_id].lcgidmap[logicalChannelIdentity]      = *logicalChannelConfig->ul_SpecificParameters->logicalChannelGroup;
      UE_info->UE_template[CC_idP][UE_id].lcgidpriority[logicalChannelIdentity] =  logicalChannelConfig->ul_SpecificParameters->priority;
    } else UE_info->UE_template[CC_idP][UE_id].lcgidmap[logicalChannelIdentity]   =  0;
  }

  if (physicalConfigDedicated != NULL) {
    UE_id = find_UE_id(Mod_idP, rntiP);

    if (UE_id<0) {
      LOG_E(MAC,"Configuration received for unknown UE (%x), shouldn't happen\n",rntiP);
      return(-1);
    }

    UE_info->UE_template[CC_idP][UE_id].physicalConfigDedicated = physicalConfigDedicated;
    LOG_I(MAC,"Added physicalConfigDedicated %p for %d.%d\n",physicalConfigDedicated,CC_idP,UE_id);
  }

  if (sCellToAddMod_r10 != NULL) {
    if (UE_id<0) {
      LOG_E(MAC,"Configuration received for unknown UE (%x), shouldn't happen\n",rntiP);
      return(-1);
    }

    AssertFatal(UE_id>=0,"Configuration received for unknown UE (%x), shouldn't happen\n",rntiP);
    config_dedicated_scell(Mod_idP, rntiP, sCellToAddMod_r10);
  }

  if (mbsfn_SubframeConfigList != NULL) {
    LOG_I(MAC,
          "[eNB %d][CONFIG] Received %d subframe allocation pattern for MBSFN\n",
          Mod_idP, mbsfn_SubframeConfigList->list.count);
    RC.mac[Mod_idP]->common_channels[0].num_sf_allocation_pattern = mbsfn_SubframeConfigList->list.count;

    for (i = 0; i < mbsfn_SubframeConfigList->list.count; i++) {
      RC.mac[Mod_idP]->common_channels[0].mbsfn_SubframeConfig[i] = mbsfn_SubframeConfigList->list.array[i];
      LOG_I(MAC,
            "[eNB %d][CONFIG] MBSFN_SubframeConfig[%d] pattern is  %x\n",
            Mod_idP, i,
            RC.mac[Mod_idP]->
            common_channels[0].mbsfn_SubframeConfig[i]->
            subframeAllocation.choice.oneFrame.buf[0]);
    }

    RC.mac[Mod_idP]->common_channels[0].MBMS_flag = MBMS_Flag;
    config_sib2_mbsfn_part(Mod_idP,0,mbsfn_SubframeConfigList);
  }

  if (nonMBSFN_SubframeConfig != NULL) {
    LOG_D(MAC,
          "[eNB %d][CONFIG] Received a non MBSFN subframe allocation pattern (%x,%x):%x for FeMBMS-CAS\n",
          Mod_idP, nonMBSFN_SubframeConfig->subframeAllocation_r14.buf[0],nonMBSFN_SubframeConfig->subframeAllocation_r14.buf[1],
          nonMBSFN_SubframeConfig->subframeAllocation_r14.buf[0]<<1 | nonMBSFN_SubframeConfig->subframeAllocation_r14.buf[1]>>7 );
    //RC.mac[Mod_idP]->common_channels[0].non_mbsfn_SubframeConfig = (int)(nonMBSFN_SubframeConfig->subframeAllocation_r14.buf[0]<<1) | (int)(nonMBSFN_SubframeConfig->subframeAllocation_r14.buf[1]>>7);
    RC.mac[Mod_idP]->common_channels[0].non_mbsfn_SubframeConfig = nonMBSFN_SubframeConfig;
    nfapi_config_request_t *cfg = &RC.mac[Mod_idP]->config[CC_idP];
    cfg->fembms_config.non_mbsfn_config_flag.value   = 1;
    cfg->fembms_config.non_mbsfn_config_flag.tl.tag = NFAPI_FEMBMS_CONFIG_NON_MBSFN_FLAG_TAG;
    cfg->num_tlv++;
    cfg->fembms_config.non_mbsfn_subframeconfig.value = (nonMBSFN_SubframeConfig->subframeAllocation_r14.buf[0]<<1 | nonMBSFN_SubframeConfig->subframeAllocation_r14.buf[1]>>7);
    cfg->fembms_config.non_mbsfn_subframeconfig.tl.tag = NFAPI_FEMBMS_CONFIG_NON_MBSFN_SUBFRAMECONFIG_TAG;
    cfg->num_tlv++;
    cfg->fembms_config.radioframe_allocation_period.value   = nonMBSFN_SubframeConfig->radioFrameAllocationPeriod_r14;
    cfg->fembms_config.radioframe_allocation_period.tl.tag = NFAPI_FEMBMS_CONFIG_RADIOFRAME_ALLOCATION_PERIOD_TAG;
    cfg->num_tlv++;
    cfg->fembms_config.radioframe_allocation_offset.value   = nonMBSFN_SubframeConfig->radioFrameAllocationOffset_r14;
    cfg->fembms_config.radioframe_allocation_offset.tl.tag = NFAPI_FEMBMS_CONFIG_RADIOFRAME_ALLOCATION_OFFSET_TAG;
    cfg->num_tlv++;
    //We need to reuse current MCH scheduler
    //TOCHECK whether we can simply reuse current mbsfn_SubframeConfig stuff
    RC.mac[Mod_idP]->common_channels[0].FeMBMS_flag = FeMBMS_Flag;
  }

  if (mbsfn_AreaInfoList != NULL) {
    // One eNB could be part of multiple mbsfn syc area, this could change over time so reset each time
    LOG_I(MAC,"[eNB %d][CONFIG] Received %d MBSFN Area Info\n", Mod_idP, mbsfn_AreaInfoList->list.count);
    RC.mac[Mod_idP]->common_channels[0].num_active_mbsfn_area = mbsfn_AreaInfoList->list.count;

    for (i =0; i< mbsfn_AreaInfoList->list.count; i++) {
      RC.mac[Mod_idP]->common_channels[0].mbsfn_AreaInfo[i] = mbsfn_AreaInfoList->list.array[i];
      LOG_I(MAC,"[eNB %d][CONFIG] MBSFN_AreaInfo[%d]: MCCH Repetition Period = %ld\n", Mod_idP,i,
            RC.mac[Mod_idP]->common_channels[0].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_RepetitionPeriod_r9);
      //      config_sib13(Mod_idP,0,i,RC.mac[Mod_idP]->common_channels[0].mbsfn_AreaInfo[i]->mbsfn_AreaId_r9);
	config_sib13(Mod_idP,0,i,RC.mac[Mod_idP]->common_channels[0].mbsfn_AreaInfo[i]->mbsfn_AreaId_r9);
    }
  }

  if(mbms_AreaConfig != NULL) {
    RC.mac[Mod_idP]->common_channels[0].commonSF_AllocPeriod_r9 = mbms_AreaConfig->commonSF_AllocPeriod_r9; 
    LOG_I(MAC, "[eNB %d][CONFIG]  LTE_MBSFNAreaConfiguration_r9_t(%p) commonSF_AllocPeriod_r9(%d)\n",Mod_idP,mbms_AreaConfig, RC.mac[Mod_idP]->common_channels[0].commonSF_AllocPeriod_r9);
    for(i=0; i < mbms_AreaConfig->commonSF_Alloc_r9.list.count; i++){
      RC.mac[Mod_idP]->common_channels[0].commonSF_Alloc_r9_mbsfn_SubframeConfig[i] = mbms_AreaConfig->commonSF_Alloc_r9.list.array[i];
     LOG_I(RRC,"[eNB %d][CONFIG] MBSFNArea[%d] commonSF_Alloc_r9: radioframeAllocationPeriod(%ldn),radioframeAllocationOffset(%ld), subframeAllocation(%x,%x,%x)\n"
      ,Mod_idP,i
      ,RC.mac[Mod_idP]->common_channels[0].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->radioframeAllocationPeriod
      ,RC.mac[Mod_idP]->common_channels[0].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->radioframeAllocationOffset
      ,RC.mac[Mod_idP]->common_channels[0].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[0]
      ,RC.mac[Mod_idP]->common_channels[0].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[1]
      ,RC.mac[Mod_idP]->common_channels[0].commonSF_Alloc_r9_mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[2]);
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
       for(int ii=0; ii < RC.mac[Mod_idP]->common_channels[0].mbms_SessionList[i]->list.count;ii++){
            LOG_I(MAC, "PMCH[%d] MBMS Session[%d] is: %lu\n", i,ii,
               RC.mac[Mod_idP]->common_channels[0].mbms_SessionList[i]->list.array[ii]->logicalChannelIdentity_r9);
       }
    }
  }

  LOG_D(MAC, "%s() %s:%d RC.mac[Mod_idP]->if_inst->PHY_config_req:%p\n", __FUNCTION__, __FILE__, __LINE__, RC.mac[Mod_idP]->if_inst->PHY_config_req);

  // if in nFAPI mode
  if (
    (NFAPI_MODE == NFAPI_MODE_PNF ||NFAPI_MODE == NFAPI_MODE_VNF) &&
    (RC.mac[Mod_idP]->if_inst->PHY_config_req == NULL)
  ) {
    while(RC.mac[Mod_idP]->if_inst->PHY_config_req == NULL) {
      // DJP AssertFatal(RC.mac[Mod_idP]->if_inst->PHY_config_req != NULL,"if_inst->phy_config_request is null\n");
      usleep(100 * 1000);
      LOG_I(MAC, "Waiting for PHY_config_req\n");
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

  RC.mac[Mod_idP]->scheduler_mode = global_scheduler_mode;
  return(0);
}


//-----------------------------------------------------------------------------
/*
* Configure local CDRX timers and thresholds following the drx_configuration input
*/
void eNB_Config_Local_DRX(instance_t Mod_id,
                          rrc_mac_drx_config_req_t *rrc_mac_drx_config_req)
//-----------------------------------------------------------------------------
{
  UE_info_t *UE_info_mac = &RC.mac[Mod_id]->UE_info;
  UE_sched_ctrl_t *UE_scheduling_control = NULL;
  LTE_DRX_Config_t *const drx_Configuration = rrc_mac_drx_config_req->drx_Configuration;
  rnti_t rnti = rrc_mac_drx_config_req->rnti;
  int UE_id = find_UE_id(Mod_id, rnti);

  /* Check UE_id */
  if (UE_id == -1) {
    LOG_E(MAC, "[eNB_Config_Local_DRX] UE_id == -1\n");
    return;
  }

  /* Get struct to modify */
  UE_scheduling_control = &(UE_info_mac->UE_sched_ctrl[UE_id]);
  UE_scheduling_control->cdrx_configured = FALSE; // will be set to true when no error

  /* Check drx_Configuration */
  if (drx_Configuration == NULL) {
    LOG_W(MAC, "[eNB_Config_Local_DRX] drx_Configuration parameter is NULL, cannot configure local UE parameters for CDRX\n");
    return;
  }

  /* Check if drx config present */
  if (drx_Configuration->present != LTE_DRX_Config_PR_setup) {
    LOG_I(MAC, "[eNB_Config_Local_DRX] No drx_Configuration present, don't configure local UE parameters for CDRX\n");
    return;
  }

  /* Modify scheduling control structure according to DRX configuration: doesn't support every configurations! */  
  UE_scheduling_control->cdrx_configured = FALSE; // will be set to true when receiving RRC Reconfiguration Complete
  UE_scheduling_control->cdrx_waiting_ack = TRUE; // waiting for RRC Reconfiguration Complete message
  UE_scheduling_control->in_active_time = FALSE;
  UE_scheduling_control->dci0_ongoing_timer = 0;
  UE_scheduling_control->on_duration_timer = 0;
  struct LTE_DRX_Config__setup *choiceSetup = &drx_Configuration->choice.setup;

  switch (choiceSetup->onDurationTimer) {
    case 0:
      UE_scheduling_control->on_duration_timer_thres = 1;
      break;

    case 1:
      UE_scheduling_control->on_duration_timer_thres = 2;
      break;

    case 2:
      UE_scheduling_control->on_duration_timer_thres = 3;
      break;

    case 3:
      UE_scheduling_control->on_duration_timer_thres = 4;
      break;

    case 4:
      UE_scheduling_control->on_duration_timer_thres = 5;
      break;

    case 5:
      UE_scheduling_control->on_duration_timer_thres = 6;
      break;

    case 6:
      UE_scheduling_control->on_duration_timer_thres = 8;
      break;

    case 7:
      UE_scheduling_control->on_duration_timer_thres = 10;
      break;

    case 8:
      UE_scheduling_control->on_duration_timer_thres = 20;
      break;

    case 9:
      UE_scheduling_control->on_duration_timer_thres = 30;
      break;

    case 10:
      UE_scheduling_control->on_duration_timer_thres = 40;
      break;

    case 11:
      UE_scheduling_control->on_duration_timer_thres = 50;
      break;

    case 12:
      UE_scheduling_control->on_duration_timer_thres = 60;
      break;

    case 13:
      UE_scheduling_control->on_duration_timer_thres = 80;
      break;

    case 14:
      UE_scheduling_control->on_duration_timer_thres = 100;
      break;

    case 15:
      UE_scheduling_control->on_duration_timer_thres = 200;
      break;

    default:
      LOG_E(MAC, "[eNB_Config_Local_DRX] Error in local DRX configuration, the on duration timer value specified is unknown\n");
      break;
  }

  UE_scheduling_control->drx_inactivity_timer = 0;

  switch (choiceSetup->drx_InactivityTimer) {
    case 0:
      UE_scheduling_control->drx_inactivity_timer_thres = 1;
      break;

    case 1:
      UE_scheduling_control->drx_inactivity_timer_thres = 2;
      break;

    case 2:
      UE_scheduling_control->drx_inactivity_timer_thres = 3;
      break;

    case 3:
      UE_scheduling_control->drx_inactivity_timer_thres = 4;
      break;

    case 4:
      UE_scheduling_control->drx_inactivity_timer_thres = 5;
      break;

    case 5:
      UE_scheduling_control->drx_inactivity_timer_thres = 6;
      break;

    case 6:
      UE_scheduling_control->drx_inactivity_timer_thres = 8;
      break;

    case 7:
      UE_scheduling_control->drx_inactivity_timer_thres = 10;
      break;

    case 8:
      UE_scheduling_control->drx_inactivity_timer_thres = 20;
      break;

    case 9:
      UE_scheduling_control->drx_inactivity_timer_thres = 30;
      break;

    case 10:
      UE_scheduling_control->drx_inactivity_timer_thres = 40;
      break;

    case 11:
      UE_scheduling_control->drx_inactivity_timer_thres = 50;
      break;

    case 12:
      UE_scheduling_control->drx_inactivity_timer_thres = 60;
      break;

    case 13:
      UE_scheduling_control->drx_inactivity_timer_thres = 80;
      break;

    case 14:
      UE_scheduling_control->drx_inactivity_timer_thres = 100;
      break;

    case 15:
      UE_scheduling_control->drx_inactivity_timer_thres = 200;
      break;

    case 16:
      UE_scheduling_control->drx_inactivity_timer_thres = 300;
      break;

    case 17:
      UE_scheduling_control->drx_inactivity_timer_thres = 500;
      break;

    case 18:
      UE_scheduling_control->drx_inactivity_timer_thres = 750;
      break;

    case 19:
      UE_scheduling_control->drx_inactivity_timer_thres = 1280;
      break;

    case 20:
      UE_scheduling_control->drx_inactivity_timer_thres = 1920;
      break;

    case 21:
      UE_scheduling_control->drx_inactivity_timer_thres = 2560;
      break;

    case 22:
      UE_scheduling_control->drx_inactivity_timer_thres = 0;
      break;

    default:
      LOG_E(MAC, "[eNB_Config_Local_DRX] Error in local DRX configuration, the drx inactivity timer value specified is unknown\n");
      break;
  }

  if (choiceSetup->shortDRX == NULL) {
    UE_scheduling_control->in_short_drx_cycle = FALSE;
    UE_scheduling_control->drx_shortCycle_timer_value = 0;
    UE_scheduling_control->short_drx_cycle_duration = 0;
    UE_scheduling_control->drx_shortCycle_timer = 0;
    UE_scheduling_control->drx_shortCycle_timer_thres = -1;
  } else {
    UE_scheduling_control->in_short_drx_cycle = FALSE;
    UE_scheduling_control->drx_shortCycle_timer_value = (uint8_t) choiceSetup->shortDRX->drxShortCycleTimer;

    switch (choiceSetup->shortDRX->shortDRX_Cycle) {
      case 0:
        UE_scheduling_control->short_drx_cycle_duration = 2;
        break;

      case 1:
        UE_scheduling_control->short_drx_cycle_duration = 5;
        break;

      case 2:
        UE_scheduling_control->short_drx_cycle_duration = 8;
        break;

      case 3:
        UE_scheduling_control->short_drx_cycle_duration = 10;
        break;

      case 4:
        UE_scheduling_control->short_drx_cycle_duration = 16;
        break;

      case 5:
        UE_scheduling_control->short_drx_cycle_duration = 20;
        break;

      case 6:
        UE_scheduling_control->short_drx_cycle_duration = 32;
        break;

      case 7:
        UE_scheduling_control->short_drx_cycle_duration = 40;
        break;

      case 8:
        UE_scheduling_control->short_drx_cycle_duration = 64;
        break;

      case 9:
        UE_scheduling_control->short_drx_cycle_duration = 80;
        break;

      case 10:
        UE_scheduling_control->short_drx_cycle_duration = 128;
        break;

      case 11:
        UE_scheduling_control->short_drx_cycle_duration = 160;
        break;

      case 12:
        UE_scheduling_control->short_drx_cycle_duration = 256;
        break;

      case 13:
        UE_scheduling_control->short_drx_cycle_duration = 320;
        break;

      case 14:
        UE_scheduling_control->short_drx_cycle_duration = 512;
        break;

      case 15:
        UE_scheduling_control->short_drx_cycle_duration = 640;
        break;

      default:
        LOG_E(MAC, "[eNB_Config_Local_DRX] Error in local DRX configuration, the short drx timer value specified is unknown\n");
        break;
    }

    UE_scheduling_control->drx_shortCycle_timer = 0;
    UE_scheduling_control->drx_shortCycle_timer_thres = UE_scheduling_control->drx_shortCycle_timer_value * UE_scheduling_control->short_drx_cycle_duration;
  }

  UE_scheduling_control->in_long_drx_cycle = FALSE;
  UE_scheduling_control->drx_longCycle_timer = 0;

  switch (choiceSetup->longDRX_CycleStartOffset.present) {
    case 	LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf10:
      UE_scheduling_control->drx_longCycle_timer_thres = 10;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf10;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf20:
      UE_scheduling_control->drx_longCycle_timer_thres = 20;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf20;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf32:
      UE_scheduling_control->drx_longCycle_timer_thres = 32;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf32;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf40:
      UE_scheduling_control->drx_longCycle_timer_thres = 40;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf40;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf64:
      UE_scheduling_control->drx_longCycle_timer_thres = 64;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf64;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf80:
      UE_scheduling_control->drx_longCycle_timer_thres = 80;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf80;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf128:
      UE_scheduling_control->drx_longCycle_timer_thres = 128;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf128;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf160:
      UE_scheduling_control->drx_longCycle_timer_thres = 160;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf160;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf256:
      UE_scheduling_control->drx_longCycle_timer_thres = 256;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf256;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf320:
      UE_scheduling_control->drx_longCycle_timer_thres = 320;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf320;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf512:
      UE_scheduling_control->drx_longCycle_timer_thres = 512;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf512;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf640:
      UE_scheduling_control->drx_longCycle_timer_thres = 640;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf640;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf1024:
      UE_scheduling_control->drx_longCycle_timer_thres = 1024;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf1024;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf1280:
      UE_scheduling_control->drx_longCycle_timer_thres = 1280;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf1280;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf2048:
      UE_scheduling_control->drx_longCycle_timer_thres = 2048;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf2048;
      break;

    case  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf2560:
      UE_scheduling_control->drx_longCycle_timer_thres = 2560;
      UE_scheduling_control->drx_start_offset = (uint16_t) choiceSetup->longDRX_CycleStartOffset.choice.sf2560;
      break;

    default:
      LOG_E(MAC, "[eNB_Config_Local_DRX] Invalid long_DRX value in DRX local configuration\n");
      break;
  }

  memset(UE_scheduling_control->drx_retransmission_timer, 0, sizeof(UE_scheduling_control->drx_retransmission_timer));

  switch (choiceSetup->drx_RetransmissionTimer) {
    case 0:
      memset(UE_scheduling_control->drx_retransmission_timer_thres, 1, sizeof(UE_scheduling_control->drx_retransmission_timer_thres));
      break;

    case 1:
      memset(UE_scheduling_control->drx_retransmission_timer_thres, 2, sizeof(UE_scheduling_control->drx_retransmission_timer_thres));
      break;

    case 2:
      memset(UE_scheduling_control->drx_retransmission_timer_thres, 4, sizeof(UE_scheduling_control->drx_retransmission_timer_thres));
      break;

    case 3:
      memset(UE_scheduling_control->drx_retransmission_timer_thres, 6, sizeof(UE_scheduling_control->drx_retransmission_timer_thres));
      break;

    case 4:
      memset(UE_scheduling_control->drx_retransmission_timer_thres, 8, sizeof(UE_scheduling_control->drx_retransmission_timer_thres));
      break;

    case 5:
      memset(UE_scheduling_control->drx_retransmission_timer_thres, 16, sizeof(UE_scheduling_control->drx_retransmission_timer_thres));
      break;

    case 6:
      memset(UE_scheduling_control->drx_retransmission_timer_thres, 24, sizeof(UE_scheduling_control->drx_retransmission_timer_thres));
      break;

    case 7:
      memset(UE_scheduling_control->drx_retransmission_timer_thres, 33, sizeof(UE_scheduling_control->drx_retransmission_timer_thres));
      break;

    default:
      LOG_E(MAC, "[eNB_Config_Local_DRX] Error in local DRX configuration, the drx retransmission timer value specified is unknown\n");
      break;
  }
}
