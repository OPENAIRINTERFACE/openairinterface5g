/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

#include "defs.h"
#include "SCHED/defs.h"
#include "PHY/extern.h"
#include "SIMULATION/TOOLS/defs.h"
#include "RadioResourceConfigCommonSIB.h"
#include "RadioResourceConfigDedicated.h"
#include "TDD-Config.h"
#include "LAYER2/MAC/extern.h"
#include "MBSFN-SubframeConfigList.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
//#define DEBUG_PHY
#include "assertions.h"
#include <math.h>

extern uint16_t prach_root_sequence_map0_3[838];
extern uint16_t prach_root_sequence_map4[138];
uint8_t dmrs1_tab[8] = {0,2,3,4,6,8,9,10};

// FIXME not used anywhere
void phy_config_mib(LTE_DL_FRAME_PARMS *fp,
                    uint8_t N_RB_DL,
                    uint8_t Nid_cell,
                    uint8_t Ncp,
                    uint8_t frame_type,
                    uint8_t p_eNB,
                    PHICH_CONFIG_COMMON *phich_config)
{

  fp->N_RB_DL                            = N_RB_DL;
  fp->Nid_cell                           = Nid_cell;
  fp->nushift                            = Nid_cell%6;
  fp->Ncp                                = Ncp;
  fp->frame_type                         = frame_type;
  fp->nb_antennas_tx_eNB                 = p_eNB;
  fp->phich_config_common.phich_resource = phich_config->phich_resource;
  fp->phich_config_common.phich_duration = phich_config->phich_duration;
}

void phy_config_sib1_eNB(uint8_t Mod_id,
                         int CC_id,
                         TDD_Config_t *tdd_Config,
                         uint8_t SIwindowsize,
                         uint16_t SIPeriod)
{

  LTE_DL_FRAME_PARMS *fp = &PHY_vars_eNB_g[Mod_id][CC_id]->frame_parms;

  if (tdd_Config) {
    fp->tdd_config    = tdd_Config->subframeAssignment;
    fp->tdd_config_S  = tdd_Config->specialSubframePatterns;
  }

  fp->SIwindowsize  = SIwindowsize;
  fp->SIPeriod      = SIPeriod;
}

void phy_config_sib1_ue(uint8_t Mod_id,int CC_id,
                        uint8_t eNB_id,
                        TDD_Config_t *tdd_Config,
                        uint8_t SIwindowsize,
                        uint16_t SIperiod)
{

  LTE_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][CC_id]->frame_parms;

  if (tdd_Config) {
    fp->tdd_config    = tdd_Config->subframeAssignment;
    fp->tdd_config_S  = tdd_Config->specialSubframePatterns;
  }

  fp->SIwindowsize  = SIwindowsize;
  fp->SIPeriod      = SIperiod;
}

void phy_config_sib2_eNB(uint8_t Mod_id,
                         int CC_id,
                         RadioResourceConfigCommonSIB_t *radioResourceConfigCommon,
                         ARFCN_ValueEUTRA_t *ul_CArrierFreq,
                         long *ul_Bandwidth,
                         AdditionalSpectrumEmission_t *additionalSpectrumEmission,
                         struct MBSFN_SubframeConfigList  *mbsfn_SubframeConfigList)
{

  LTE_DL_FRAME_PARMS *fp = &PHY_vars_eNB_g[Mod_id][CC_id]->frame_parms;
  //LTE_eNB_UE_stats *eNB_UE_stats      = PHY_vars_eNB_g[Mod_id][CC_id]->eNB_UE_stats;
  //int32_t rx_total_gain_eNB_dB        = PHY_vars_eNB_g[Mod_id][CC_id]->rx_total_gain_eNB_dB;
  int i;

  LOG_D(PHY,"[eNB%d] CCid %d: Applying radioResourceConfigCommon\n",Mod_id,CC_id);

  fp->prach_config_common.rootSequenceIndex                           =radioResourceConfigCommon->prach_Config.rootSequenceIndex;
  LOG_D(PHY,"prach_config_common.rootSequenceIndex = %d\n",fp->prach_config_common.rootSequenceIndex );

  fp->prach_config_common.prach_Config_enabled=1;

  fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex          =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_ConfigIndex;
  LOG_D(PHY,"prach_config_common.prach_ConfigInfo.prach_ConfigIndex = %d\n",fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex);

  fp->prach_config_common.prach_ConfigInfo.highSpeedFlag              =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.highSpeedFlag;
  LOG_D(PHY,"prach_config_common.prach_ConfigInfo.highSpeedFlag = %d\n",fp->prach_config_common.prach_ConfigInfo.highSpeedFlag);
  fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig  =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig;
  LOG_D(PHY,"prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig = %d\n",fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig);
  fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset           =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_FreqOffset;
  LOG_D(PHY,"prach_config_common.prach_ConfigInfo.prach_FreqOffset = %d\n",fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset);
  compute_prach_seq(&fp->prach_config_common,fp->frame_type,
                    PHY_vars_eNB_g[Mod_id][CC_id]->X_u);

  fp->pucch_config_common.deltaPUCCH_Shift = 1+radioResourceConfigCommon->pucch_ConfigCommon.deltaPUCCH_Shift;
  fp->pucch_config_common.nRB_CQI          = radioResourceConfigCommon->pucch_ConfigCommon.nRB_CQI;
  fp->pucch_config_common.nCS_AN           = radioResourceConfigCommon->pucch_ConfigCommon.nCS_AN;
  fp->pucch_config_common.n1PUCCH_AN       = radioResourceConfigCommon->pucch_ConfigCommon.n1PUCCH_AN;



  fp->pdsch_config_common.referenceSignalPower = radioResourceConfigCommon->pdsch_ConfigCommon.referenceSignalPower;
  fp->pdsch_config_common.p_b                  = radioResourceConfigCommon->pdsch_ConfigCommon.p_b;


  fp->pusch_config_common.n_SB                                         = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.n_SB;
  LOG_D(PHY,"pusch_config_common.n_SB = %d\n",fp->pusch_config_common.n_SB );

  fp->pusch_config_common.hoppingMode                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode;
  LOG_D(PHY,"pusch_config_common.hoppingMode = %d\n",fp->pusch_config_common.hoppingMode);

  fp->pusch_config_common.pusch_HoppingOffset                          = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset;
  LOG_D(PHY,"pusch_config_common.pusch_HoppingOffset = %d\n",fp->pusch_config_common.pusch_HoppingOffset);

  fp->pusch_config_common.enable64QAM                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM;
  LOG_D(PHY,"pusch_config_common.enable64QAM = %d\n",fp->pusch_config_common.enable64QAM );

  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled    = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled;
  LOG_D(PHY,"pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled = %d\n",fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled);

  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH   = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH;
  LOG_D(PHY,"pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH = %d\n",fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH);

  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled;
  LOG_D(PHY,"pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = %d\n",fp->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled);

  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift            = dmrs1_tab[radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift];
  LOG_D(PHY,"pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift = %d\n",fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift);

  init_ul_hopping(fp);

  fp->soundingrs_ul_config_common.enabled_flag                        = 0;

  if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon.present==SoundingRS_UL_ConfigCommon_PR_setup) {
    fp->soundingrs_ul_config_common.enabled_flag                        = 1;
    fp->soundingrs_ul_config_common.srs_BandwidthConfig                 = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_BandwidthConfig;
    fp->soundingrs_ul_config_common.srs_SubframeConfig                  = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig;
    fp->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission;

    if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts)
      fp->soundingrs_ul_config_common.srs_MaxUpPts                      = 1;
    else
      fp->soundingrs_ul_config_common.srs_MaxUpPts                      = 0;
  }



  fp->ul_power_control_config_common.p0_NominalPUSCH       = radioResourceConfigCommon->uplinkPowerControlCommon.p0_NominalPUSCH;
  fp->ul_power_control_config_common.alpha                 = radioResourceConfigCommon->uplinkPowerControlCommon.alpha;
  fp->ul_power_control_config_common.p0_NominalPUCCH       = radioResourceConfigCommon->uplinkPowerControlCommon.p0_NominalPUCCH;
  fp->ul_power_control_config_common.deltaPreambleMsg3     = radioResourceConfigCommon->uplinkPowerControlCommon.deltaPreambleMsg3;
  fp->ul_power_control_config_common.deltaF_PUCCH_Format1  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1;
  fp->ul_power_control_config_common.deltaF_PUCCH_Format1b  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b;
  fp->ul_power_control_config_common.deltaF_PUCCH_Format2  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2;
  fp->ul_power_control_config_common.deltaF_PUCCH_Format2a  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a;
  fp->ul_power_control_config_common.deltaF_PUCCH_Format2b  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b;

  fp->maxHARQ_Msg3Tx = radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx;


  // Now configure some of the Physical Channels

  // PUCCH

  init_ncs_cell(fp,PHY_vars_eNB_g[Mod_id][CC_id]->ncs_cell);

  init_ul_hopping(fp);


  // MBSFN
  if (mbsfn_SubframeConfigList != NULL) {
    fp->num_MBSFN_config = mbsfn_SubframeConfigList->list.count;

    for (i=0; i<mbsfn_SubframeConfigList->list.count; i++) {
      fp->MBSFN_config[i].radioframeAllocationPeriod = mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationPeriod;
      fp->MBSFN_config[i].radioframeAllocationOffset = mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationOffset;

      if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) {
        fp->MBSFN_config[i].fourFrames_flag = 0;
        fp->MBSFN_config[i].mbsfn_SubframeConfig = mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]; // 6-bit subframe configuration
        LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %ld\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
      } else if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames) { // 24-bit subframe configuration
        fp->MBSFN_config[i].fourFrames_flag = 1;
        fp->MBSFN_config[i].mbsfn_SubframeConfig =
          mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[1]<<8)|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[2]<<16);

        LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %ld\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
      }
    }

  } else
    fp->num_MBSFN_config = 0;
}


void phy_config_sib2_ue(uint8_t Mod_id,int CC_id,
                        uint8_t eNB_id,
                        RadioResourceConfigCommonSIB_t *radioResourceConfigCommon,
                        ARFCN_ValueEUTRA_t *ul_CarrierFreq,
                        long *ul_Bandwidth,
                        AdditionalSpectrumEmission_t *additionalSpectrumEmission,
                        struct MBSFN_SubframeConfigList *mbsfn_SubframeConfigList)
{

  PHY_VARS_UE *ue        = PHY_vars_UE_g[Mod_id][CC_id];
  LTE_DL_FRAME_PARMS *fp = &ue->frame_parms;
  int i;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_CONFIG_SIB2, VCD_FUNCTION_IN);

  LOG_I(PHY,"[UE%d] Applying radioResourceConfigCommon from eNB%d\n",Mod_id,eNB_id);

  fp->prach_config_common.rootSequenceIndex                           =radioResourceConfigCommon->prach_Config.rootSequenceIndex;

  fp->prach_config_common.prach_Config_enabled=1;
  fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex          =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_ConfigIndex;
  fp->prach_config_common.prach_ConfigInfo.highSpeedFlag              =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.highSpeedFlag;
  fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig  =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig;
  fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset           =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_FreqOffset;

  compute_prach_seq(&fp->prach_config_common,fp->frame_type,ue->X_u);



  fp->pucch_config_common.deltaPUCCH_Shift = 1+radioResourceConfigCommon->pucch_ConfigCommon.deltaPUCCH_Shift;
  fp->pucch_config_common.nRB_CQI          = radioResourceConfigCommon->pucch_ConfigCommon.nRB_CQI;
  fp->pucch_config_common.nCS_AN           = radioResourceConfigCommon->pucch_ConfigCommon.nCS_AN;
  fp->pucch_config_common.n1PUCCH_AN       = radioResourceConfigCommon->pucch_ConfigCommon.n1PUCCH_AN;



  fp->pdsch_config_common.referenceSignalPower = radioResourceConfigCommon->pdsch_ConfigCommon.referenceSignalPower;
  fp->pdsch_config_common.p_b                  = radioResourceConfigCommon->pdsch_ConfigCommon.p_b;


  fp->pusch_config_common.n_SB                                         = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.n_SB;
  fp->pusch_config_common.hoppingMode                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode;
  fp->pusch_config_common.pusch_HoppingOffset                          = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset;
  fp->pusch_config_common.enable64QAM                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM;
  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled    = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled;
  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH   = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH;
  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled;
  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift            = dmrs1_tab[radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift];


  init_ul_hopping(fp);
  fp->soundingrs_ul_config_common.enabled_flag                        = 0;

  if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon.present==SoundingRS_UL_ConfigCommon_PR_setup) {
    fp->soundingrs_ul_config_common.enabled_flag                        = 1;
    fp->soundingrs_ul_config_common.srs_BandwidthConfig                 = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_BandwidthConfig;
    fp->soundingrs_ul_config_common.srs_SubframeConfig                  = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig;
    fp->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission;

    if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts)
      fp->soundingrs_ul_config_common.srs_MaxUpPts                      = 1;
    else
      fp->soundingrs_ul_config_common.srs_MaxUpPts                      = 0;
  }



  fp->ul_power_control_config_common.p0_NominalPUSCH   = radioResourceConfigCommon->uplinkPowerControlCommon.p0_NominalPUSCH;
  fp->ul_power_control_config_common.alpha             = radioResourceConfigCommon->uplinkPowerControlCommon.alpha;
  fp->ul_power_control_config_common.p0_NominalPUCCH   = radioResourceConfigCommon->uplinkPowerControlCommon.p0_NominalPUCCH;
  fp->ul_power_control_config_common.deltaPreambleMsg3 = radioResourceConfigCommon->uplinkPowerControlCommon.deltaPreambleMsg3;
  fp->ul_power_control_config_common.deltaF_PUCCH_Format1  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1;
  fp->ul_power_control_config_common.deltaF_PUCCH_Format1b  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b;
  fp->ul_power_control_config_common.deltaF_PUCCH_Format2  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2;
  fp->ul_power_control_config_common.deltaF_PUCCH_Format2a  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a;
  fp->ul_power_control_config_common.deltaF_PUCCH_Format2b  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b;

  fp->maxHARQ_Msg3Tx = radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx;

  // Now configure some of the Physical Channels

  // PUCCH
  init_ncs_cell(fp,ue->ncs_cell);

  init_ul_hopping(fp);

  // PCH
  init_ue_paging_info(ue,radioResourceConfigCommon->pcch_Config.defaultPagingCycle,radioResourceConfigCommon->pcch_Config.nB);

  // MBSFN

  if (mbsfn_SubframeConfigList != NULL) {
    fp->num_MBSFN_config = mbsfn_SubframeConfigList->list.count;

    for (i=0; i<mbsfn_SubframeConfigList->list.count; i++) {
      fp->MBSFN_config[i].radioframeAllocationPeriod = mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationPeriod;
      fp->MBSFN_config[i].radioframeAllocationOffset = mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationOffset;

      if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) {
        fp->MBSFN_config[i].fourFrames_flag = 0;
        fp->MBSFN_config[i].mbsfn_SubframeConfig = mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]; // 6-bit subframe configuration
        LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %ld\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
      } else if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames) { // 24-bit subframe configuration
        fp->MBSFN_config[i].fourFrames_flag = 1;
        fp->MBSFN_config[i].mbsfn_SubframeConfig =
          mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[1]<<8)|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[2]<<16);

        LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %ld\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
      }
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_CONFIG_SIB2, VCD_FUNCTION_OUT);

}

void phy_config_sib13_ue(uint8_t Mod_id,int CC_id,uint8_t eNB_id,int mbsfn_Area_idx,
                         long mbsfn_AreaId_r9)
{

  LTE_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][CC_id]->frame_parms;


  LOG_I(PHY,"[UE%d] Applying MBSFN_Area_id %d for index %d\n",Mod_id,mbsfn_AreaId_r9,mbsfn_Area_idx);

  if (mbsfn_Area_idx == 0) {
    fp->Nid_cell_mbsfn = (uint16_t)mbsfn_AreaId_r9;
    LOG_N(PHY,"Fix me: only called when mbsfn_Area_idx == 0)\n");
  }

  lte_gold_mbsfn(fp,PHY_vars_UE_g[Mod_id][CC_id]->lte_gold_mbsfn_table,fp->Nid_cell_mbsfn);

}


void phy_config_sib13_eNB(uint8_t Mod_id,int CC_id,int mbsfn_Area_idx,
                          long mbsfn_AreaId_r9)
{

  LTE_DL_FRAME_PARMS *fp = &PHY_vars_eNB_g[Mod_id][CC_id]->frame_parms;


  LOG_I(PHY,"[eNB%d] Applying MBSFN_Area_id %d for index %d\n",Mod_id,mbsfn_AreaId_r9,mbsfn_Area_idx);

  if (mbsfn_Area_idx == 0) {
    fp->Nid_cell_mbsfn = (uint16_t)mbsfn_AreaId_r9;
    LOG_N(PHY,"Fix me: only called when mbsfn_Area_idx == 0)\n");
  }

  lte_gold_mbsfn(fp,PHY_vars_eNB_g[Mod_id][CC_id]->lte_gold_mbsfn_table,fp->Nid_cell_mbsfn);
}


void phy_config_dedicated_eNB_step2(PHY_VARS_eNB *eNB)
{

  uint8_t UE_id;
  struct PhysicalConfigDedicated *physicalConfigDedicated;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;

  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    physicalConfigDedicated = eNB->physicalConfigDedicated[UE_id];

    if (physicalConfigDedicated != NULL) {
      LOG_I(PHY,"[eNB %d] Frame %d: Sent physicalConfigDedicated=%p for UE %d\n",eNB->Mod_id,physicalConfigDedicated,UE_id);
      LOG_D(PHY,"------------------------------------------------------------------------\n");

      if (physicalConfigDedicated->pdsch_ConfigDedicated) {
        eNB->pdsch_config_dedicated[UE_id].p_a=physicalConfigDedicated->pdsch_ConfigDedicated->p_a;
        LOG_D(PHY,"pdsch_config_dedicated.p_a %d\n",eNB->pdsch_config_dedicated[UE_id].p_a);
        LOG_D(PHY,"\n");
      }

      if (physicalConfigDedicated->pucch_ConfigDedicated) {
        if (physicalConfigDedicated->pucch_ConfigDedicated->ackNackRepetition.present==PUCCH_ConfigDedicated__ackNackRepetition_PR_release)
          eNB->pucch_config_dedicated[UE_id].ackNackRepetition=0;
        else {
          eNB->pucch_config_dedicated[UE_id].ackNackRepetition=1;
        }

        if (fp->frame_type == FDD) {
          eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode = multiplexing;
        } else {
          if (physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode)
            eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode = *physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode;
          else
            eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode = bundling;
        }

        if ( eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode == multiplexing)
          LOG_D(PHY,"pucch_config_dedicated.tdd_AckNackFeedbackMode = multiplexing\n");
        else
          LOG_D(PHY,"pucch_config_dedicated.tdd_AckNackFeedbackMode = bundling\n");

      }

      if (physicalConfigDedicated->pusch_ConfigDedicated) {
        eNB->pusch_config_dedicated[UE_id].betaOffset_ACK_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_ACK_Index;
        eNB->pusch_config_dedicated[UE_id].betaOffset_RI_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_RI_Index;
        eNB->pusch_config_dedicated[UE_id].betaOffset_CQI_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_CQI_Index;

        LOG_D(PHY,"pusch_config_dedicated.betaOffset_ACK_Index %d\n",eNB->pusch_config_dedicated[UE_id].betaOffset_ACK_Index);
        LOG_D(PHY,"pusch_config_dedicated.betaOffset_RI_Index %d\n",eNB->pusch_config_dedicated[UE_id].betaOffset_RI_Index);
        LOG_D(PHY,"pusch_config_dedicated.betaOffset_CQI_Index %d\n",eNB->pusch_config_dedicated[UE_id].betaOffset_CQI_Index);
        LOG_D(PHY,"\n");


      }

      if (physicalConfigDedicated->uplinkPowerControlDedicated) {

        eNB->ul_power_control_dedicated[UE_id].p0_UE_PUSCH = physicalConfigDedicated->uplinkPowerControlDedicated->p0_UE_PUSCH;
        eNB->ul_power_control_dedicated[UE_id].deltaMCS_Enabled= physicalConfigDedicated->uplinkPowerControlDedicated->deltaMCS_Enabled;
        eNB->ul_power_control_dedicated[UE_id].accumulationEnabled= physicalConfigDedicated->uplinkPowerControlDedicated->accumulationEnabled;
        eNB->ul_power_control_dedicated[UE_id].p0_UE_PUCCH= physicalConfigDedicated->uplinkPowerControlDedicated->p0_UE_PUCCH;
        eNB->ul_power_control_dedicated[UE_id].pSRS_Offset= physicalConfigDedicated->uplinkPowerControlDedicated->pSRS_Offset;
        eNB->ul_power_control_dedicated[UE_id].filterCoefficient= *physicalConfigDedicated->uplinkPowerControlDedicated->filterCoefficient;
        LOG_D(PHY,"ul_power_control_dedicated.p0_UE_PUSCH %d\n",eNB->ul_power_control_dedicated[UE_id].p0_UE_PUSCH);
        LOG_D(PHY,"ul_power_control_dedicated.deltaMCS_Enabled %d\n",eNB->ul_power_control_dedicated[UE_id].deltaMCS_Enabled);
        LOG_D(PHY,"ul_power_control_dedicated.accumulationEnabled %d\n",eNB->ul_power_control_dedicated[UE_id].accumulationEnabled);
        LOG_D(PHY,"ul_power_control_dedicated.p0_UE_PUCCH %d\n",eNB->ul_power_control_dedicated[UE_id].p0_UE_PUCCH);
        LOG_D(PHY,"ul_power_control_dedicated.pSRS_Offset %d\n",eNB->ul_power_control_dedicated[UE_id].pSRS_Offset);
        LOG_D(PHY,"ul_power_control_dedicated.filterCoefficient %d\n",eNB->ul_power_control_dedicated[UE_id].filterCoefficient);
        LOG_D(PHY,"\n");
      }

      if (physicalConfigDedicated->antennaInfo) {
        eNB->transmission_mode[UE_id] = 1+(physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode);
        LOG_D(PHY,"Transmission Mode (phy_config_dedicated_eNB_step2) %d\n",eNB->transmission_mode[UE_id]);
        LOG_D(PHY,"\n");
      }

      if (physicalConfigDedicated->schedulingRequestConfig) {
        if (physicalConfigDedicated->schedulingRequestConfig->present == SchedulingRequestConfig_PR_setup) {
          eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex = physicalConfigDedicated->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex;
          eNB->scheduling_request_config[UE_id].sr_ConfigIndex=physicalConfigDedicated->schedulingRequestConfig->choice.setup.sr_ConfigIndex;
          eNB->scheduling_request_config[UE_id].dsr_TransMax=physicalConfigDedicated->schedulingRequestConfig->choice.setup.dsr_TransMax;

          LOG_D(PHY,"scheduling_request_config.sr_PUCCH_ResourceIndex %d\n",eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex);
          LOG_D(PHY,"scheduling_request_config.sr_ConfigIndex %d\n",eNB->scheduling_request_config[UE_id].sr_ConfigIndex);
          LOG_D(PHY,"scheduling_request_config.dsr_TransMax %d\n",eNB->scheduling_request_config[UE_id].dsr_TransMax);
        }

        LOG_D(PHY,"------------------------------------------------------------\n");

      }

      if (physicalConfigDedicated->soundingRS_UL_ConfigDedicated) {
        if (physicalConfigDedicated->soundingRS_UL_ConfigDedicated->present == SoundingRS_UL_ConfigDedicated_PR_setup) {

          eNB->soundingrs_ul_config_dedicated[UE_id].duration             = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.duration;
          eNB->soundingrs_ul_config_dedicated[UE_id].cyclicShift          = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.cyclicShift;
          eNB->soundingrs_ul_config_dedicated[UE_id].freqDomainPosition   = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.freqDomainPosition;
          eNB->soundingrs_ul_config_dedicated[UE_id].srs_Bandwidth        = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.srs_Bandwidth;
          eNB->soundingrs_ul_config_dedicated[UE_id].srs_ConfigIndex      = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex;
          eNB->soundingrs_ul_config_dedicated[UE_id].srs_HoppingBandwidth = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.srs_HoppingBandwidth;
          eNB->soundingrs_ul_config_dedicated[UE_id].transmissionComb     = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb;


          LOG_D(PHY,"soundingrs_ul_config_dedicated.srs_ConfigIndex %d\n",eNB->soundingrs_ul_config_dedicated[UE_id].srs_ConfigIndex);

        }

        LOG_D(PHY,"------------------------------------------------------------\n");

      }

      eNB->physicalConfigDedicated[UE_id] = NULL;
    }
  }
}

/*
 * Configures UE MAC and PHY with radioResourceCommon received in mobilityControlInfo IE during Handover
 */
void phy_config_afterHO_ue(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_id, MobilityControlInfo_t *mobilityControlInfo, uint8_t ho_failed)
{

  if(mobilityControlInfo!=NULL) {
    RadioResourceConfigCommon_t *radioResourceConfigCommon = &mobilityControlInfo->radioResourceConfigCommon;
    LOG_I(PHY,"radioResourceConfigCommon %p\n", radioResourceConfigCommon);
    memcpy((void *)&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms_before_ho,
           (void *)&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms,
           sizeof(LTE_DL_FRAME_PARMS));
    PHY_vars_UE_g[Mod_id][CC_id]->ho_triggered = 1;
    //PHY_vars_UE_g[UE_id]->UE_mode[0] = PRACH;

    LTE_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][CC_id]->frame_parms;
    //     int N_ZC;
    //     uint8_t prach_fmt;
    //     int u;

    LOG_I(PHY,"[UE%d] Handover triggered: Applying radioResourceConfigCommon from eNB %d\n",
          Mod_id,eNB_id);

    fp->prach_config_common.rootSequenceIndex                           =radioResourceConfigCommon->prach_Config.rootSequenceIndex;
    fp->prach_config_common.prach_Config_enabled=1;
    fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex          =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->prach_ConfigIndex;
    fp->prach_config_common.prach_ConfigInfo.highSpeedFlag              =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->highSpeedFlag;
    fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig  =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->zeroCorrelationZoneConfig;
    fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset           =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->prach_FreqOffset;

    //     prach_fmt = get_prach_fmt(radioResourceConfigCommon->prach_Config.prach_ConfigInfo->prach_ConfigIndex,fp->frame_type);
    //     N_ZC = (prach_fmt <4)?839:139;
    //     u = (prach_fmt < 4) ? prach_root_sequence_map0_3[fp->prach_config_common.rootSequenceIndex] :
    //       prach_root_sequence_map4[fp->prach_config_common.rootSequenceIndex];

    //compute_prach_seq(u,N_ZC, PHY_vars_UE_g[Mod_id]->X_u);
    compute_prach_seq(&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms.prach_config_common,
                      fp->frame_type,
                      PHY_vars_UE_g[Mod_id][CC_id]->X_u);


    fp->pucch_config_common.deltaPUCCH_Shift = 1+radioResourceConfigCommon->pucch_ConfigCommon->deltaPUCCH_Shift;
    fp->pucch_config_common.nRB_CQI          = radioResourceConfigCommon->pucch_ConfigCommon->nRB_CQI;
    fp->pucch_config_common.nCS_AN           = radioResourceConfigCommon->pucch_ConfigCommon->nCS_AN;
    fp->pucch_config_common.n1PUCCH_AN       = radioResourceConfigCommon->pucch_ConfigCommon->n1PUCCH_AN;
    fp->pdsch_config_common.referenceSignalPower = radioResourceConfigCommon->pdsch_ConfigCommon->referenceSignalPower;
    fp->pdsch_config_common.p_b                  = radioResourceConfigCommon->pdsch_ConfigCommon->p_b;


    fp->pusch_config_common.n_SB                                         = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.n_SB;
    fp->pusch_config_common.hoppingMode                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode;
    fp->pusch_config_common.pusch_HoppingOffset                          = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset;
    fp->pusch_config_common.enable64QAM                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM;
    fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled    = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled;
    fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH   = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH;
    fp->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled;
    fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift            = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift;

    init_ul_hopping(fp);
    fp->soundingrs_ul_config_common.enabled_flag                        = 0;

    if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon->present==SoundingRS_UL_ConfigCommon_PR_setup) {
      fp->soundingrs_ul_config_common.enabled_flag                        = 1;
      fp->soundingrs_ul_config_common.srs_BandwidthConfig                 = radioResourceConfigCommon->soundingRS_UL_ConfigCommon->choice.setup.srs_BandwidthConfig;
      fp->soundingrs_ul_config_common.srs_SubframeConfig                  = radioResourceConfigCommon->soundingRS_UL_ConfigCommon->choice.setup.srs_SubframeConfig;
      fp->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission = radioResourceConfigCommon->soundingRS_UL_ConfigCommon->choice.setup.ackNackSRS_SimultaneousTransmission;

      if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon->choice.setup.srs_MaxUpPts)
        fp->soundingrs_ul_config_common.srs_MaxUpPts                      = 1;
      else
        fp->soundingrs_ul_config_common.srs_MaxUpPts                      = 0;
    }

    fp->ul_power_control_config_common.p0_NominalPUSCH   = radioResourceConfigCommon->uplinkPowerControlCommon->p0_NominalPUSCH;
    fp->ul_power_control_config_common.alpha             = radioResourceConfigCommon->uplinkPowerControlCommon->alpha;
    fp->ul_power_control_config_common.p0_NominalPUCCH   = radioResourceConfigCommon->uplinkPowerControlCommon->p0_NominalPUCCH;
    fp->ul_power_control_config_common.deltaPreambleMsg3 = radioResourceConfigCommon->uplinkPowerControlCommon->deltaPreambleMsg3;
    fp->ul_power_control_config_common.deltaF_PUCCH_Format1  = radioResourceConfigCommon->uplinkPowerControlCommon->deltaFList_PUCCH.deltaF_PUCCH_Format1;
    fp->ul_power_control_config_common.deltaF_PUCCH_Format1b  = radioResourceConfigCommon->uplinkPowerControlCommon->deltaFList_PUCCH.deltaF_PUCCH_Format1b;
    fp->ul_power_control_config_common.deltaF_PUCCH_Format2  = radioResourceConfigCommon->uplinkPowerControlCommon->deltaFList_PUCCH.deltaF_PUCCH_Format2;
    fp->ul_power_control_config_common.deltaF_PUCCH_Format2a  = radioResourceConfigCommon->uplinkPowerControlCommon->deltaFList_PUCCH.deltaF_PUCCH_Format2a;
    fp->ul_power_control_config_common.deltaF_PUCCH_Format2b  = radioResourceConfigCommon->uplinkPowerControlCommon->deltaFList_PUCCH.deltaF_PUCCH_Format2b;

    fp->maxHARQ_Msg3Tx = radioResourceConfigCommon->rach_ConfigCommon->maxHARQ_Msg3Tx;

    // Now configure some of the Physical Channels
    if (radioResourceConfigCommon->antennaInfoCommon)
      fp->nb_antennas_tx                     = (1<<radioResourceConfigCommon->antennaInfoCommon->antennaPortsCount);
    else
      fp->nb_antennas_tx                     = 1;

    //PHICH
    if (radioResourceConfigCommon->antennaInfoCommon) {
      fp->phich_config_common.phich_resource = radioResourceConfigCommon->phich_Config->phich_Resource;
      fp->phich_config_common.phich_duration = radioResourceConfigCommon->phich_Config->phich_Duration;
    }

    //Target CellId
    fp->Nid_cell = mobilityControlInfo->targetPhysCellId;
    fp->nushift  = fp->Nid_cell%6;

    // PUCCH
    init_ncs_cell(fp,PHY_vars_UE_g[Mod_id][CC_id]->ncs_cell);

    init_ul_hopping(fp);

    // RNTI

    PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[eNB_id]->crnti = mobilityControlInfo->newUE_Identity.buf[0]|(mobilityControlInfo->newUE_Identity.buf[1]<<8);

  }

  if(ho_failed) {
    LOG_D(PHY,"[UE%d] Handover failed, triggering RACH procedure\n",Mod_id);
    memcpy((void *)&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms,(void *)&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms_before_ho, sizeof(LTE_DL_FRAME_PARMS));
    PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[eNB_id] = PRACH;
  }
}

void phy_config_meas_ue(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index,uint8_t n_adj_cells,unsigned int *adj_cell_id)
{

  PHY_MEASUREMENTS *phy_meas = &PHY_vars_UE_g[Mod_id][CC_id]->measurements;
  int i;

  LOG_I(PHY,"Configuring inter-cell measurements for %d cells, ids: \n",n_adj_cells);

  for (i=0; i<n_adj_cells; i++) {
    LOG_I(PHY,"%d\n",adj_cell_id[i]);
    lte_gold(&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms,PHY_vars_UE_g[Mod_id][CC_id]->lte_gold_table[i+1],adj_cell_id[i]);
  }

  phy_meas->n_adj_cells = n_adj_cells;
  memcpy((void*)phy_meas->adj_cell_id,(void *)adj_cell_id,n_adj_cells*sizeof(unsigned int));

}

void phy_config_dedicated_eNB(uint8_t Mod_id,
                              int CC_id,
                              uint16_t rnti,
                              struct PhysicalConfigDedicated *physicalConfigDedicated)
{

  PHY_VARS_eNB *eNB = PHY_vars_eNB_g[Mod_id][CC_id];
  int8_t UE_id = find_ue(rnti,eNB);

  if (UE_id == -1) {
    LOG_E( PHY, "[eNB %"PRIu8"] find_ue() returns -1\n");
    return;
  }


  if (physicalConfigDedicated) {
    eNB->physicalConfigDedicated[UE_id] = physicalConfigDedicated;
    LOG_I(PHY,"phy_config_dedicated_eNB: physicalConfigDedicated=%p\n",physicalConfigDedicated);

    if (physicalConfigDedicated->antennaInfo) {
      switch(physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode) {
      case AntennaInfoDedicated__transmissionMode_tm1:
	eNB->transmission_mode[UE_id] = 1;
	break;
      case AntennaInfoDedicated__transmissionMode_tm2:
	eNB->transmission_mode[UE_id] = 2;
	break;
      case AntennaInfoDedicated__transmissionMode_tm3:
	eNB->transmission_mode[UE_id] = 3;
	break;
      case AntennaInfoDedicated__transmissionMode_tm4:
	eNB->transmission_mode[UE_id] = 4;
	break;
      case AntennaInfoDedicated__transmissionMode_tm5:
	eNB->transmission_mode[UE_id] = 5;
	break;
      case AntennaInfoDedicated__transmissionMode_tm6:
	eNB->transmission_mode[UE_id] = 6;
	break;
      case AntennaInfoDedicated__transmissionMode_tm7:
	eNB->transmission_mode[UE_id] = 7;
	break;
      default:
	LOG_E(PHY,"Unknown transmission mode!\n");
	break;
      }
      LOG_I(PHY,"Transmission Mode (phy_config_dedicated_eNB) %d\n",eNB->transmission_mode[UE_id]);
 
    } else {
      LOG_D(PHY,"[eNB %d] : Received NULL radioResourceConfigDedicated->antennaInfo from eNB %d\n",Mod_id,UE_id);
    }
  } else {
    LOG_E(PHY,"[eNB %d] Received NULL radioResourceConfigDedicated from eNB %d\n",Mod_id, UE_id);
    return;
  }

}
#ifdef Rel10
void phy_config_dedicated_scell_ue(uint8_t Mod_id,
                                   uint8_t eNB_index,
                                   SCellToAddMod_r10_t *sCellToAddMod_r10,
                                   int CC_id)
{

}
void phy_config_dedicated_scell_eNB(uint8_t Mod_id,
                                    uint16_t rnti,
                                    SCellToAddMod_r10_t *sCellToAddMod_r10,
                                    int CC_id)
{


  uint8_t UE_id = find_ue(rnti,PHY_vars_eNB_g[Mod_id][0]);
  struct PhysicalConfigDedicatedSCell_r10 *physicalConfigDedicatedSCell_r10 = sCellToAddMod_r10->radioResourceConfigDedicatedSCell_r10->physicalConfigDedicatedSCell_r10;
  //struct RadioResourceConfigCommonSCell_r10 *physicalConfigCommonSCell_r10 = sCellToAddMod_r10->radioResourceConfigCommonSCell_r10;
  //PhysCellId_t physCellId_r10 = sCellToAddMod_r10->cellIdentification_r10->physCellId_r10;
  ARFCN_ValueEUTRA_t dl_CarrierFreq_r10 = sCellToAddMod_r10->cellIdentification_r10->dl_CarrierFreq_r10;
  uint32_t carrier_freq_local;

  if ((dl_CarrierFreq_r10>=36000) && (dl_CarrierFreq_r10<=36199)) {
    carrier_freq_local = 1900000000 + (dl_CarrierFreq_r10-36000)*100000; //band 33 from 3GPP 36.101 v 10.9 Table 5.7.3-1
    LOG_I(PHY,"[eNB %d] Frame %d: Configured SCell %d to frequency %d (ARFCN %d) for UE %d\n",Mod_id,/*eNB->frame*/0,CC_id,carrier_freq_local,dl_CarrierFreq_r10,UE_id);
  } else if ((dl_CarrierFreq_r10>=6150) && (dl_CarrierFreq_r10<=6449)) {
    carrier_freq_local = 832000000 + (dl_CarrierFreq_r10-6150)*100000; //band 20 from 3GPP 36.101 v 10.9 Table 5.7.3-1
    // this is actually for the UL only, but we use it for DL too, since there is no TDD mode for this band
    LOG_I(PHY,"[eNB %d] Frame %d: Configured SCell %d to frequency %d (ARFCN %d) for UE %d\n",Mod_id,/*eNB->frame*/0,CC_id,carrier_freq_local,dl_CarrierFreq_r10,UE_id);
  } else {
    LOG_E(PHY,"[eNB %d] Frame %d: ARFCN %d of SCell %d for UE %d not supported\n",Mod_id,/*eNB->frame*/0,dl_CarrierFreq_r10,CC_id,UE_id);
  }

  if (physicalConfigDedicatedSCell_r10) {
//#warning " eNB->physicalConfigDedicatedSCell_r10 does not exist in eNB"
    //  eNB->physicalConfigDedicatedSCell_r10[UE_id] = physicalConfigDedicatedSCell_r10;
    LOG_I(PHY,"[eNB %d] Frame %d: Configured phyConfigDedicatedSCell with CC_id %d for UE %d\n",Mod_id,/*eNB->frame*/0,CC_id,UE_id);
  } else {
    LOG_E(PHY,"[eNB %d] Frame %d: Received NULL radioResourceConfigDedicated (CC_id %d, UE %d)\n",Mod_id, /*eNB->frame*/0,CC_id,UE_id);
    return;
  }

}
#endif

void phy_config_harq_ue(uint8_t Mod_id,int CC_id,uint8_t eNB_id,
                        uint16_t max_harq_tx )
{

  PHY_VARS_UE *phy_vars_ue = PHY_vars_UE_g[Mod_id][CC_id];
  phy_vars_ue->ulsch[eNB_id]->Mlimit = max_harq_tx;
}

void phy_config_dedicated_ue(uint8_t Mod_id,int CC_id,uint8_t eNB_id,
                             struct PhysicalConfigDedicated *physicalConfigDedicated )
{

  PHY_VARS_UE *phy_vars_ue = PHY_vars_UE_g[Mod_id][CC_id];

  phy_vars_ue->total_TBS[eNB_id]=0;
  phy_vars_ue->total_TBS_last[eNB_id]=0;
  phy_vars_ue->bitrate[eNB_id]=0;
  phy_vars_ue->total_received_bits[eNB_id]=0;
  phy_vars_ue->dlsch_errors[eNB_id]=0;
  phy_vars_ue->dlsch_errors_last[eNB_id]=0;
  phy_vars_ue->dlsch_received[eNB_id]=0;
  phy_vars_ue->dlsch_received_last[eNB_id]=0;
  phy_vars_ue->dlsch_fer[eNB_id]=0;

  phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex = -1;
  phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex = -1;

  if (physicalConfigDedicated) {
    LOG_D(PHY,"[UE %d] Received physicalConfigDedicated from eNB %d\n",Mod_id, eNB_id);
    LOG_D(PHY,"------------------------------------------------------------------------\n");

    if (physicalConfigDedicated->pdsch_ConfigDedicated) {
      phy_vars_ue->pdsch_config_dedicated[eNB_id].p_a=physicalConfigDedicated->pdsch_ConfigDedicated->p_a;
      LOG_D(PHY,"pdsch_config_dedicated.p_a %d\n",phy_vars_ue->pdsch_config_dedicated[eNB_id].p_a);
      LOG_D(PHY,"\n");
    }

    if (physicalConfigDedicated->pucch_ConfigDedicated) {
      if (physicalConfigDedicated->pucch_ConfigDedicated->ackNackRepetition.present==PUCCH_ConfigDedicated__ackNackRepetition_PR_release)
        phy_vars_ue->pucch_config_dedicated[eNB_id].ackNackRepetition=0;
      else {
        phy_vars_ue->pucch_config_dedicated[eNB_id].ackNackRepetition=1;
      }

      if (physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode)
        phy_vars_ue->pucch_config_dedicated[eNB_id].tdd_AckNackFeedbackMode = *physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode;
      else
        phy_vars_ue->pucch_config_dedicated[eNB_id].tdd_AckNackFeedbackMode = bundling;

      if ( phy_vars_ue->pucch_config_dedicated[eNB_id].tdd_AckNackFeedbackMode == multiplexing)
        LOG_D(PHY,"pucch_config_dedicated.tdd_AckNackFeedbackMode = multiplexing\n");
      else
        LOG_D(PHY,"pucch_config_dedicated.tdd_AckNackFeedbackMode = bundling\n");
    }

    if (physicalConfigDedicated->pusch_ConfigDedicated) {
      phy_vars_ue->pusch_config_dedicated[eNB_id].betaOffset_ACK_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_ACK_Index;
      phy_vars_ue->pusch_config_dedicated[eNB_id].betaOffset_RI_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_RI_Index;
      phy_vars_ue->pusch_config_dedicated[eNB_id].betaOffset_CQI_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_CQI_Index;


      LOG_D(PHY,"pusch_config_dedicated.betaOffset_ACK_Index %d\n",phy_vars_ue->pusch_config_dedicated[eNB_id].betaOffset_ACK_Index);
      LOG_D(PHY,"pusch_config_dedicated.betaOffset_RI_Index %d\n",phy_vars_ue->pusch_config_dedicated[eNB_id].betaOffset_RI_Index);
      LOG_D(PHY,"pusch_config_dedicated.betaOffset_CQI_Index %d\n",phy_vars_ue->pusch_config_dedicated[eNB_id].betaOffset_CQI_Index);
      LOG_D(PHY,"\n");


    }

    if (physicalConfigDedicated->uplinkPowerControlDedicated) {

      phy_vars_ue->ul_power_control_dedicated[eNB_id].p0_UE_PUSCH = physicalConfigDedicated->uplinkPowerControlDedicated->p0_UE_PUSCH;
      phy_vars_ue->ul_power_control_dedicated[eNB_id].deltaMCS_Enabled= physicalConfigDedicated->uplinkPowerControlDedicated->deltaMCS_Enabled;
      phy_vars_ue->ul_power_control_dedicated[eNB_id].accumulationEnabled= physicalConfigDedicated->uplinkPowerControlDedicated->accumulationEnabled;
      phy_vars_ue->ul_power_control_dedicated[eNB_id].p0_UE_PUCCH= physicalConfigDedicated->uplinkPowerControlDedicated->p0_UE_PUCCH;
      phy_vars_ue->ul_power_control_dedicated[eNB_id].pSRS_Offset= physicalConfigDedicated->uplinkPowerControlDedicated->pSRS_Offset;
      phy_vars_ue->ul_power_control_dedicated[eNB_id].filterCoefficient= *physicalConfigDedicated->uplinkPowerControlDedicated->filterCoefficient;
      LOG_D(PHY,"ul_power_control_dedicated.p0_UE_PUSCH %d\n",phy_vars_ue->ul_power_control_dedicated[eNB_id].p0_UE_PUSCH);
      LOG_D(PHY,"ul_power_control_dedicated.deltaMCS_Enabled %d\n",phy_vars_ue->ul_power_control_dedicated[eNB_id].deltaMCS_Enabled);
      LOG_D(PHY,"ul_power_control_dedicated.accumulationEnabled %d\n",phy_vars_ue->ul_power_control_dedicated[eNB_id].accumulationEnabled);
      LOG_D(PHY,"ul_power_control_dedicated.p0_UE_PUCCH %d\n",phy_vars_ue->ul_power_control_dedicated[eNB_id].p0_UE_PUCCH);
      LOG_D(PHY,"ul_power_control_dedicated.pSRS_Offset %d\n",phy_vars_ue->ul_power_control_dedicated[eNB_id].pSRS_Offset);
      LOG_D(PHY,"ul_power_control_dedicated.filterCoefficient %d\n",phy_vars_ue->ul_power_control_dedicated[eNB_id].filterCoefficient);
      LOG_D(PHY,"\n");
    }

    if (physicalConfigDedicated->antennaInfo) {
      phy_vars_ue->transmission_mode[eNB_id] = 1+(physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode);
      LOG_D(PHY,"Transmission Mode %d\n",phy_vars_ue->transmission_mode[eNB_id]);
      LOG_D(PHY,"\n");
      switch(physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode) {
      case AntennaInfoDedicated__transmissionMode_tm1:
	phy_vars_ue->transmission_mode[eNB_id] = 1;
	break;
      case AntennaInfoDedicated__transmissionMode_tm2:
	phy_vars_ue->transmission_mode[eNB_id] = 2;
	break;
      case AntennaInfoDedicated__transmissionMode_tm3:
	phy_vars_ue->transmission_mode[eNB_id] = 3;
	break;
      case AntennaInfoDedicated__transmissionMode_tm4:
	phy_vars_ue->transmission_mode[eNB_id] = 4;
	break;
      case AntennaInfoDedicated__transmissionMode_tm5:
	phy_vars_ue->transmission_mode[eNB_id] = 5;
	break;
      case AntennaInfoDedicated__transmissionMode_tm6:
	phy_vars_ue->transmission_mode[eNB_id] = 6;
	break;
      case AntennaInfoDedicated__transmissionMode_tm7:
	phy_vars_ue->transmission_mode[eNB_id] = 7;
	break;
      default:
	LOG_E(PHY,"Unknown transmission mode!\n");
	break;
      } 
    } else {
      LOG_D(PHY,"[UE %d] Received NULL physicalConfigDedicated->antennaInfo from eNB %d\n",Mod_id, eNB_id);
    }

    if (physicalConfigDedicated->schedulingRequestConfig) {
      if (physicalConfigDedicated->schedulingRequestConfig->present == SchedulingRequestConfig_PR_setup) {
        phy_vars_ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex = physicalConfigDedicated->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex;
        phy_vars_ue->scheduling_request_config[eNB_id].sr_ConfigIndex=physicalConfigDedicated->schedulingRequestConfig->choice.setup.sr_ConfigIndex;
        phy_vars_ue->scheduling_request_config[eNB_id].dsr_TransMax=physicalConfigDedicated->schedulingRequestConfig->choice.setup.dsr_TransMax;

        LOG_D(PHY,"scheduling_request_config.sr_PUCCH_ResourceIndex %d\n",phy_vars_ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
        LOG_D(PHY,"scheduling_request_config.sr_ConfigIndex %d\n",phy_vars_ue->scheduling_request_config[eNB_id].sr_ConfigIndex);
        LOG_D(PHY,"scheduling_request_config.dsr_TransMax %d\n",phy_vars_ue->scheduling_request_config[eNB_id].dsr_TransMax);
      }

      LOG_D(PHY,"------------------------------------------------------------\n");

    }

    if (physicalConfigDedicated->soundingRS_UL_ConfigDedicated) {

      phy_vars_ue->soundingrs_ul_config_dedicated[eNB_id].srsConfigDedicatedSetup = 0;
      if (physicalConfigDedicated->soundingRS_UL_ConfigDedicated->present == SoundingRS_UL_ConfigDedicated_PR_setup) {
        phy_vars_ue->soundingrs_ul_config_dedicated[eNB_id].srsConfigDedicatedSetup = 1;
        phy_vars_ue->soundingrs_ul_config_dedicated[eNB_id].duration             = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.duration;
        phy_vars_ue->soundingrs_ul_config_dedicated[eNB_id].cyclicShift          = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.cyclicShift;
        phy_vars_ue->soundingrs_ul_config_dedicated[eNB_id].freqDomainPosition   = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.freqDomainPosition;
        phy_vars_ue->soundingrs_ul_config_dedicated[eNB_id].srs_Bandwidth        = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.srs_Bandwidth;
        phy_vars_ue->soundingrs_ul_config_dedicated[eNB_id].srs_ConfigIndex      = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex;
        phy_vars_ue->soundingrs_ul_config_dedicated[eNB_id].srs_HoppingBandwidth = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.srs_HoppingBandwidth;
        phy_vars_ue->soundingrs_ul_config_dedicated[eNB_id].transmissionComb     = physicalConfigDedicated->soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb;


        LOG_D(PHY,"soundingrs_ul_config_dedicated.srs_ConfigIndex %d\n",phy_vars_ue->soundingrs_ul_config_dedicated[eNB_id].srs_ConfigIndex);
      }

      LOG_D(PHY,"------------------------------------------------------------\n");

    }


    if (physicalConfigDedicated->cqi_ReportConfig) {
      if (physicalConfigDedicated->cqi_ReportConfig->cqi_ReportModeAperiodic) {
	// configure PUSCH CQI reporting
	phy_vars_ue->cqi_report_config[eNB_id].cqi_ReportModeAperiodic = *physicalConfigDedicated->cqi_ReportConfig->cqi_ReportModeAperiodic;
	if ((phy_vars_ue->cqi_report_config[eNB_id].cqi_ReportModeAperiodic != rm12) && 
	    (phy_vars_ue->cqi_report_config[eNB_id].cqi_ReportModeAperiodic != rm30) &&
	    (phy_vars_ue->cqi_report_config[eNB_id].cqi_ReportModeAperiodic != rm31))
	  LOG_E(PHY,"Unsupported Aperiodic CQI Feedback Mode : %d\n",phy_vars_ue->cqi_report_config[eNB_id].cqi_ReportModeAperiodic);
      }
      if (physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic) {
	if (physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->present == CQI_ReportPeriodic_PR_setup) {
	// configure PUCCH CQI reporting
	  phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PUCCH_ResourceIndex = physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_PUCCH_ResourceIndex;
	  phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex     = physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_pmi_ConfigIndex;
	  if (physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.ri_ConfigIndex)
	    phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex = *physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.ri_ConfigIndex;	
	}
	else if (physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->present == CQI_ReportPeriodic_PR_release) {
	  // handle release
	  phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex = -1;
	  phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex = -1;
	}
      }
    }
    
#ifdef CBA

    if (physicalConfigDedicated->pusch_CBAConfigDedicated_vlola) {
      phy_vars_ue->pusch_ca_config_dedicated[eNB_id].betaOffset_CA_Index = (uint16_t) *physicalConfigDedicated->pusch_CBAConfigDedicated_vlola->betaOffset_CBA_Index;
      phy_vars_ue->pusch_ca_config_dedicated[eNB_id].cShift = (uint16_t) *physicalConfigDedicated->pusch_CBAConfigDedicated_vlola->cShift_CBA;
      LOG_D(PHY,"[UE %d ] physicalConfigDedicated pusch CBA config dedicated: beta offset %d cshift %d \n",Mod_id,
            phy_vars_ue->pusch_ca_config_dedicated[eNB_id].betaOffset_CA_Index,
            phy_vars_ue->pusch_ca_config_dedicated[eNB_id].cShift);
    }

#endif
  } else {
    LOG_D(PHY,"[PHY][UE %d] Received NULL radioResourceConfigDedicated from eNB %d\n",Mod_id,eNB_id);
    return;
  }

  // fill cqi parameters for periodic CQI reporting
  get_cqipmiri_params(phy_vars_ue,eNB_id);

}

void  phy_config_cba_rnti (module_id_t Mod_id,int CC_id,eNB_flag_t eNB_flag, uint8_t index, rnti_t cba_rnti, uint8_t cba_group_id, uint8_t num_active_cba_groups)
{
  //   uint8_t i;

  if (eNB_flag == 0 ) {
    //LOG_D(PHY,"[UE %d] configure cba group %d with rnti %x, num active cba grp %d\n", index, index, cba_rnti, num_active_cba_groups);
    PHY_vars_UE_g[Mod_id][CC_id]->ulsch[index]->num_active_cba_groups=num_active_cba_groups;
    PHY_vars_UE_g[Mod_id][CC_id]->ulsch[index]->cba_rnti[cba_group_id]=cba_rnti;
  } else {
    //for (i=index; i < NUMBER_OF_UE_MAX; i+=num_active_cba_groups){
    //  LOG_D(PHY,"[eNB %d] configure cba group %d with rnti %x for UE %d, num active cba grp %d\n",Mod_id, i%num_active_cba_groups, cba_rnti, i, num_active_cba_groups);
    PHY_vars_eNB_g[Mod_id][CC_id]->ulsch[index]->num_active_cba_groups=num_active_cba_groups;
    PHY_vars_eNB_g[Mod_id][CC_id]->ulsch[index]->cba_rnti[cba_group_id] = cba_rnti;
    //}
  }
}

void phy_init_lte_top(LTE_DL_FRAME_PARMS *frame_parms)
{

  crcTableInit();

  ccodedot11_init();
  ccodedot11_init_inv();

  ccodelte_init();
  ccodelte_init_inv();

  treillis_table_init();

  phy_generate_viterbi_tables();
  phy_generate_viterbi_tables_lte();

  init_td8();
  init_td16();
#ifdef __AVX2__
  init_td16avx2();
#endif

  lte_sync_time_init(frame_parms);

  generate_ul_ref_sigs();
  generate_ul_ref_sigs_rx();

  generate_64qam_table();
  generate_16qam_table();
  generate_RIV_tables();

  init_unscrambling_lut();
  init_scrambling_lut();
  //set_taus_seed(1328);

}

/*! \brief Helper function to allocate memory for DLSCH data structures.
 * \param[out] pdsch Pointer to the LTE_UE_PDSCH structure to initialize.
 * \param[in] frame_parms LTE_DL_FRAME_PARMS structure.
 * \note This function is optimistic in that it expects malloc() to succeed.
 */
void phy_init_lte_ue__PDSCH( LTE_UE_PDSCH* const pdsch, const LTE_DL_FRAME_PARMS* const fp )
{
  AssertFatal( pdsch, "pdsch==0" );

  pdsch->pmi_ext = (uint8_t*)malloc16_clear( fp->N_RB_DL );
  pdsch->llr[0] = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
  pdsch->llr128 = (int16_t**)malloc16_clear( sizeof(int16_t*) );
  pdsch->llr128_2ndstream = (int16_t**)malloc16_clear( sizeof(int16_t*) );
  // FIXME! no further allocation for (int16_t*)pdsch->llr128 !!! expect SIGSEGV

  pdsch->rxdataF_ext         = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->rxdataF_comp0       = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->rho                 = (int32_t**)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t*) );
  pdsch->dl_ch_estimates_ext = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_rho_ext       = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_rho2_ext       = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_mag0          = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_magb0         = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );

  // the allocated memory size is fixed:
  AssertFatal( fp->nb_antennas_rx <= 2, "nb_antennas_rx > 2" );

  for (int i=0; i<fp->nb_antennas_rx; i++) {
    pdsch->rho[i]     = (int32_t*)malloc16_clear( sizeof(int32_t)*(fp->N_RB_DL*12*7*2) );

    for (int j=0; j<4; j++) { //fp->nb_antennas_tx; j++)
      const int idx = (j<<1)+i;
      const size_t num = 7*2*fp->N_RB_DL*12;
      pdsch->rxdataF_ext[idx]         = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch->rxdataF_comp0[idx]       = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_estimates_ext[idx] = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_rho_ext[idx]       = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_rho2_ext[idx]       = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_mag0[idx]          = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_magb0[idx]         = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
    }
  }
}


int phy_init_lte_ue(PHY_VARS_UE *ue,
                    int nb_connected_eNB,
                    uint8_t abstraction_flag)
{

  // create shortcuts
  LTE_DL_FRAME_PARMS* const fp            = &ue->frame_parms;
  LTE_UE_COMMON* const common_vars        = &ue->common_vars;
  LTE_UE_PDSCH** const pdsch_vars         = ue->pdsch_vars;
  LTE_UE_PDSCH** const pdsch_vars_SI      = ue->pdsch_vars_SI;
  LTE_UE_PDSCH** const pdsch_vars_ra      = ue->pdsch_vars_ra;
  LTE_UE_PDSCH** const pdsch_vars_mch     = ue->pdsch_vars_MCH;
  LTE_UE_PBCH** const pbch_vars           = ue->pbch_vars;
  LTE_UE_PDCCH** const pdcch_vars         = ue->pdcch_vars;
  LTE_UE_PRACH** const prach_vars         = ue->prach_vars;

  int i,j,k;
  int eNB_id;

  printf("Initializing UE vars (abstraction %"PRIu8") for eNB TXant %"PRIu8", UE RXant %"PRIu8"\n",abstraction_flag,fp->nb_antennas_tx,fp->nb_antennas_rx);
  LOG_D(PHY,"[MSC_NEW][FRAME 00000][PHY_UE][MOD %02u][]\n", ue->Mod_id+NB_eNB_INST);

  // many memory allocation sizes are hard coded
  AssertFatal( fp->nb_antennas_rx <= 2, "hard coded allocation for ue_common_vars->dl_ch_estimates[eNB_id]" );
  AssertFatal( ue->n_connected_eNB <= NUMBER_OF_CONNECTED_eNB_MAX, "n_connected_eNB is too large" );
  // init phy_vars_ue

  for (i=0; i<4; i++) {
    ue->rx_gain_max[i] = 135;
    ue->rx_gain_med[i] = 128;
    ue->rx_gain_byp[i] = 120;
  }

  ue->n_connected_eNB = nb_connected_eNB;

  for(eNB_id = 0; eNB_id < ue->n_connected_eNB; eNB_id++) {
    ue->total_TBS[eNB_id] = 0;
    ue->total_TBS_last[eNB_id] = 0;
    ue->bitrate[eNB_id] = 0;
    ue->total_received_bits[eNB_id] = 0;
  }

  for (i=0;i<10;i++)
    ue->tx_power_dBm[i]=-127;

  if (abstraction_flag == 0) {

    // init TX buffers

    common_vars->txdata  = (int32_t**)malloc16( fp->nb_antennas_tx*sizeof(int32_t*) );
    common_vars->txdataF = (int32_t **)malloc16( fp->nb_antennas_tx*sizeof(int32_t*) );

    for (i=0; i<fp->nb_antennas_tx; i++) {

      common_vars->txdata[i]  = (int32_t*)malloc16_clear( fp->samples_per_tti*10*sizeof(int32_t) );
      common_vars->txdataF[i] = (int32_t *)malloc16_clear( fp->ofdm_symbol_size*fp->symbols_per_tti*10*sizeof(int32_t) );
    }

    // init RX buffers

    common_vars->rxdata   = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
    common_vars->rxdataF  = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
    common_vars->rxdataF2 = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );

    for (i=0; i<fp->nb_antennas_rx; i++) {
      common_vars->rxdata[i] = (int32_t*) malloc16_clear( (fp->samples_per_tti*10+2048)*sizeof(int32_t) );
      common_vars->rxdataF[i] = (int32_t*)malloc16_clear( sizeof(int32_t)*(fp->ofdm_symbol_size*14) );
      common_vars->rxdataF2[i] = (int32_t*)malloc16_clear( sizeof(int32_t)*(fp->ofdm_symbol_size*fp->symbols_per_tti*10) );
    }
  }

  // Channel estimates
  for (eNB_id=0; eNB_id<7; eNB_id++) {
    common_vars->dl_ch_estimates[eNB_id]      = (int32_t**)malloc16_clear(8*sizeof(int32_t*));
    common_vars->dl_ch_estimates_time[eNB_id] = (int32_t**)malloc16_clear(8*sizeof(int32_t*));

    for (i=0; i<fp->nb_antennas_rx; i++)
      for (j=0; j<4; j++) {
        int idx = (j<<1) + i;
        common_vars->dl_ch_estimates[eNB_id][idx] = (int32_t*)malloc16_clear( sizeof(int32_t)*fp->symbols_per_tti*(fp->ofdm_symbol_size+LTE_CE_FILTER_LENGTH) );
        common_vars->dl_ch_estimates_time[eNB_id][idx] = (int32_t*)malloc16_clear( sizeof(int32_t)*fp->ofdm_symbol_size*2 );
      }
  }

  // DLSCH
  for (eNB_id=0; eNB_id<ue->n_connected_eNB; eNB_id++) {
    pdsch_vars[eNB_id]     = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    pdsch_vars_SI[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    pdsch_vars_ra[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    pdsch_vars_mch[eNB_id] = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    pdcch_vars[eNB_id]     = (LTE_UE_PDCCH *)malloc16_clear(sizeof(LTE_UE_PDCCH));
    prach_vars[eNB_id]     = (LTE_UE_PRACH *)malloc16_clear(sizeof(LTE_UE_PRACH));
    pbch_vars[eNB_id]      = (LTE_UE_PBCH *)malloc16_clear(sizeof(LTE_UE_PBCH));

    if (abstraction_flag == 0) {
      phy_init_lte_ue__PDSCH( pdsch_vars[eNB_id], fp );

      pdsch_vars[eNB_id]->llr_shifts   = (uint8_t*)malloc16_clear(7*2*fp->N_RB_DL*12);
      pdsch_vars[eNB_id]->llr_shifts_p = pdsch_vars[eNB_id]->llr_shifts;
      pdsch_vars[eNB_id]->dl_ch_mag1   = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      pdsch_vars[eNB_id]->dl_ch_magb1  = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      pdsch_vars[eNB_id]->llr[1]       = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );

      for (k=0; k<8; k++)
        pdsch_vars[eNB_id]->rxdataF_comp1[k] = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );

      for (i=0; i<fp->nb_antennas_rx; i++)
        for (j=0; j<4; j++) {
          int idx = (j<<1)+i;
          pdsch_vars[eNB_id]->dl_ch_mag1[idx]  = (int32_t*)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
          pdsch_vars[eNB_id]->dl_ch_magb1[idx] = (int32_t*)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );

          for (k=0; k<8; k++)
            pdsch_vars[eNB_id]->rxdataF_comp1[idx][k] = (int32_t*)malloc16_clear( sizeof(int32_t)*(fp->N_RB_DL*12*14) );
        }

      phy_init_lte_ue__PDSCH( pdsch_vars_SI[eNB_id], fp );
      phy_init_lte_ue__PDSCH( pdsch_vars_ra[eNB_id], fp );
      phy_init_lte_ue__PDSCH( pdsch_vars_mch[eNB_id], fp );
      // 100 PRBs * 12 REs/PRB * 4 PDCCH SYMBOLS * 2 LLRs/RE
      pdcch_vars[eNB_id]->llr   = (uint16_t*)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      pdcch_vars[eNB_id]->llr16 = (uint16_t*)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      pdcch_vars[eNB_id]->wbar  = (uint16_t*)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      pdcch_vars[eNB_id]->e_rx  = (int8_t*)malloc16_clear( 4*2*100*12 );

      pdcch_vars[eNB_id]->rxdataF_comp        = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      pdcch_vars[eNB_id]->dl_ch_rho_ext       = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      pdcch_vars[eNB_id]->rho                 = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
      pdcch_vars[eNB_id]->rxdataF_ext         = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      pdcch_vars[eNB_id]->dl_ch_estimates_ext = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );

      for (i=0; i<fp->nb_antennas_rx; i++) {
        //ue_pdcch_vars[eNB_id]->rho[i] = (int32_t*)malloc16_clear( sizeof(int32_t)*(fp->N_RB_DL*12*7*2) );
        pdcch_vars[eNB_id]->rho[i] = (int32_t*)malloc16_clear( sizeof(int32_t)*(100*12*4) );

        for (j=0; j<4; j++) { //fp->nb_antennas_tx; j++)
          int idx = (j<<1)+i;
          //  size_t num = 7*2*fp->N_RB_DL*12;
          size_t num = 4*100*12;  // 4 symbols, 100 PRBs, 12 REs per PRB
          pdcch_vars[eNB_id]->rxdataF_comp[idx]        = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
          pdcch_vars[eNB_id]->dl_ch_rho_ext[idx]       = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
          pdcch_vars[eNB_id]->rxdataF_ext[idx]         = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
          pdcch_vars[eNB_id]->dl_ch_estimates_ext[idx] = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
        }
      }

      // PBCH
      pbch_vars[eNB_id]->rxdataF_ext         = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
      pbch_vars[eNB_id]->rxdataF_comp        = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      pbch_vars[eNB_id]->dl_ch_estimates_ext = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      pbch_vars[eNB_id]->llr                 = (int8_t*)malloc16_clear( 1920 );
      prach_vars[eNB_id]->prachF             = (int16_t*)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(fp->ofdm_symbol_size*12)) );
      prach_vars[eNB_id]->prach              = (int16_t*)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(fp->ofdm_symbol_size*12)) );

      for (i=0; i<fp->nb_antennas_rx; i++) {
        pbch_vars[eNB_id]->rxdataF_ext[i]    = (int32_t*)malloc16_clear( sizeof(int32_t)*6*12*4 );

        for (j=0; j<4; j++) {//fp->nb_antennas_tx;j++) {
          int idx = (j<<1)+i;
          pbch_vars[eNB_id]->rxdataF_comp[idx]        = (int32_t*)malloc16_clear( sizeof(int32_t)*6*12*4 );
          pbch_vars[eNB_id]->dl_ch_estimates_ext[idx] = (int32_t*)malloc16_clear( sizeof(int32_t)*6*12*4 );
        }
      }
    }

    pbch_vars[eNB_id]->decoded_output = (uint8_t*)malloc16_clear( 64 );
  }

  // initialization for the last instance of pdsch_vars (used for MU-MIMO)

  pdsch_vars[eNB_id]     = (LTE_UE_PDSCH *)malloc16_clear( sizeof(LTE_UE_PDSCH) );
  pdsch_vars_SI[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear( sizeof(LTE_UE_PDSCH) );
  pdsch_vars_ra[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear( sizeof(LTE_UE_PDSCH) );

  if (abstraction_flag == 0) {
    phy_init_lte_ue__PDSCH( pdsch_vars[eNB_id], fp );
    pdsch_vars[eNB_id]->llr[1] = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );

  } else { //abstraction == 1
    ue->sinr_dB = (double*) malloc16_clear( fp->N_RB_DL*12*sizeof(double) );
  }

  ue->sinr_CQI_dB = (double*) malloc16_clear( fp->N_RB_DL*12*sizeof(double) );

  ue->init_averaging = 1;
  ue->pdsch_config_dedicated->p_a = dB0; // default value until overwritten by RRCConnectionReconfiguration

  // set channel estimation to do linear interpolation in time
  ue->high_speed_flag = 1;
  ue->ch_est_alpha    = 24576;

  init_prach_tables(839);


  return 0;
}

int phy_init_lte_eNB(PHY_VARS_eNB *eNB,
                     unsigned char is_secondary_eNB,
                     unsigned char abstraction_flag)
{

  // shortcuts
  LTE_DL_FRAME_PARMS* const fp      = &eNB->frame_parms;
  LTE_eNB_COMMON* const common_vars = &eNB->common_vars;
  LTE_eNB_PUSCH** const pusch_vars  = eNB->pusch_vars;
  LTE_eNB_SRS* const srs_vars       = eNB->srs_vars;
  LTE_eNB_PRACH* const prach_vars   = &eNB->prach_vars;
  int i, j, eNB_id, UE_id; 

  eNB->total_dlsch_bitrate = 0;
  eNB->total_transmitted_bits = 0;
  eNB->total_system_throughput = 0;
  eNB->check_for_MUMIMO_transmissions=0;
  LOG_I(PHY,"[eNB %"PRIu8"] Initializing DL_FRAME_PARMS : N_RB_DL %"PRIu8", PHICH Resource %d, PHICH Duration %d\n",
        eNB->Mod_id,
        fp->N_RB_DL,fp->phich_config_common.phich_resource,
        fp->phich_config_common.phich_duration);
  LOG_D(PHY,"[MSC_NEW][FRAME 00000][PHY_eNB][MOD %02"PRIu8"][]\n", eNB->Mod_id);

  if (eNB->node_function != NGFI_RRU_IF4p5) {
    lte_gold(fp,eNB->lte_gold_table,fp->Nid_cell);
    generate_pcfich_reg_mapping(fp);
    generate_phich_reg_mapping(fp);
    
    for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
      eNB->first_run_timing_advance[UE_id] =
	1; ///This flag used to be static. With multiple eNBs this does no longer work, hence we put it in the structure. However it has to be initialized with 1, which is performed here.

      // clear whole structure
      bzero( &eNB->UE_stats[UE_id], sizeof(LTE_eNB_UE_stats) );
      
      eNB->physicalConfigDedicated[UE_id] = NULL;
    }
    
    eNB->first_run_I0_measurements =
      1; ///This flag used to be static. With multiple eNBs this does no longer work, hence we put it in the structure. However it has to be initialized with 1, which is performed here.
  }
  //  for (eNB_id=0; eNB_id<3; eNB_id++) {
  {
    eNB_id=0;
    if (abstraction_flag==0) {
      
      // TX vars
      if (eNB->node_function != NGFI_RCC_IF4p5)
	common_vars->txdata[eNB_id]  = (int32_t**)malloc16( fp->nb_antennas_tx*sizeof(int32_t*) );
      common_vars->txdataF[eNB_id] = (int32_t **)malloc16( fp->nb_antennas_tx*sizeof(int32_t*) );
      
      for (i=0; i<fp->nb_antennas_tx; i++) {
	if (eNB->node_function != NGFI_RCC_IF4p5)
	  common_vars->txdata[eNB_id][i]  = (int32_t*)malloc16_clear( fp->samples_per_tti*10*sizeof(int32_t) );
	common_vars->txdataF[eNB_id][i] = (int32_t*)malloc16_clear( fp->ofdm_symbol_size*fp->symbols_per_tti*10*sizeof(int32_t) );
#ifdef DEBUG_PHY
	printf("[openair][LTE_PHY][INIT] common_vars->txdata[%d][%d] = %p\n",eNB_id,i,common_vars->txdata[eNB_id][i]);
	printf("[openair][LTE_PHY][INIT] common_vars->txdataF[%d][%d] = %p (%d bytes)\n",
	    eNB_id,i,common_vars->txdataF[eNB_id][i],
	    fp->ofdm_symbol_size*fp->symbols_per_tti*10*sizeof(int32_t));
#endif
      }
      
      // RX vars
      if (eNB->node_function != NGFI_RCC_IF4p5) {
	common_vars->rxdata[eNB_id]        = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
	common_vars->rxdata_7_5kHz[eNB_id] = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
      }
      common_vars->rxdataF[eNB_id]       = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
      
      for (i=0; i<fp->nb_antennas_rx; i++) {
	if (eNB->node_function != NGFI_RCC_IF4p5) {
	  common_vars->rxdata[eNB_id][i] = (int32_t*)malloc16_clear( fp->samples_per_tti*10*sizeof(int32_t) );
	  common_vars->rxdata_7_5kHz[eNB_id][i] = (int32_t*)malloc16_clear( fp->samples_per_tti*sizeof(int32_t) );
	}
	
	common_vars->rxdataF[eNB_id][i] = (int32_t*)malloc16_clear(sizeof(int32_t)*(fp->ofdm_symbol_size*fp->symbols_per_tti) );
#ifdef DEBUG_PHY
	printf("[openair][LTE_PHY][INIT] common_vars->rxdata[%d][%d] = %p\n",eNB_id,i,common_vars->rxdata[eNB_id][i]);
	printf("[openair][LTE_PHY][INIT] common_vars->rxdata_7_5kHz[%d][%d] = %p\n",eNB_id,i,common_vars->rxdata_7_5kHz[eNB_id][i]);
#endif
      }
      
      if (eNB->node_function != NGFI_RRU_IF4p5) {
	// Channel estimates for SRS
	for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
	  
	  srs_vars[UE_id].srs_ch_estimates[eNB_id]      = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
	  srs_vars[UE_id].srs_ch_estimates_time[eNB_id] = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
	  
	  for (i=0; i<fp->nb_antennas_rx; i++) {
	    srs_vars[UE_id].srs_ch_estimates[eNB_id][i]      = (int32_t*)malloc16_clear( sizeof(int32_t)*fp->ofdm_symbol_size );
	    srs_vars[UE_id].srs_ch_estimates_time[eNB_id][i] = (int32_t*)malloc16_clear( sizeof(int32_t)*fp->ofdm_symbol_size*2 );
	  }
	} //UE_id
	
	common_vars->sync_corr[eNB_id] = (uint32_t*)malloc16_clear( LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*sizeof(uint32_t)*fp->samples_per_tti );
      }
    } else { //UPLINK abstraction = 1
      eNB->sinr_dB = (double*) malloc16_clear( fp->N_RB_DL*12*sizeof(double) );
    }
  } //eNB_id
  
  
  
  if (abstraction_flag==0) {
    if (eNB->node_function != NGFI_RRU_IF4p5) {
      generate_ul_ref_sigs_rx();
      
      // SRS
      for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
	srs_vars[UE_id].srs = (int32_t*)malloc16_clear(2*fp->ofdm_symbol_size*sizeof(int32_t));
      }
    }
  }
  
  
  
  // ULSCH VARS, skip if NFGI_RRU_IF4
  
  if (eNB->node_function!=NGFI_RRU_IF4p5)
    prach_vars->prachF = (int16_t*)malloc16_clear( 1024*2*sizeof(int16_t) );
  
  /* number of elements of an array X is computed as sizeof(X) / sizeof(X[0]) */
  AssertFatal(fp->nb_antennas_rx <= sizeof(prach_vars->rxsigF) / sizeof(prach_vars->rxsigF[0]),
              "nb_antennas_rx too large");
  for (i=0; i<fp->nb_antennas_rx; i++) {
    prach_vars->rxsigF[i] = (int16_t*)malloc16_clear( fp->ofdm_symbol_size*12*2*sizeof(int16_t) );
#ifdef DEBUG_PHY
    printf("[openair][LTE_PHY][INIT] prach_vars->rxsigF[%d] = %p\n",i,prach_vars->rxsigF[i]);
#endif
  }
  
  if (eNB->node_function != NGFI_RRU_IF4p5) {
    AssertFatal(fp->nb_antennas_rx <= sizeof(prach_vars->prach_ifft) / sizeof(prach_vars->prach_ifft[0]),
		"nb_antennas_rx too large");
    for (i=0; i<fp->nb_antennas_rx; i++) {
      prach_vars->prach_ifft[i] = (int16_t*)malloc16_clear(1024*2*sizeof(int16_t));
#ifdef DEBUG_PHY
      printf("[openair][LTE_PHY][INIT] prach_vars->prach_ifft[%d] = %p\n",i,prach_vars->prach_ifft[i]);
#endif
    }

    for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
      
      //FIXME
      pusch_vars[UE_id] = (LTE_eNB_PUSCH*)malloc16_clear( NUMBER_OF_UE_MAX*sizeof(LTE_eNB_PUSCH) );
      
      if (abstraction_flag==0) {
	for (eNB_id=0; eNB_id<3; eNB_id++) {
	  
	  pusch_vars[UE_id]->rxdataF_ext[eNB_id]      = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
	  pusch_vars[UE_id]->rxdataF_ext2[eNB_id]     = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
	  pusch_vars[UE_id]->drs_ch_estimates[eNB_id] = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
	  pusch_vars[UE_id]->drs_ch_estimates_time[eNB_id] = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
	  pusch_vars[UE_id]->rxdataF_comp[eNB_id]     = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
	  pusch_vars[UE_id]->ul_ch_mag[eNB_id]  = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
	  pusch_vars[UE_id]->ul_ch_magb[eNB_id] = (int32_t**)malloc16( fp->nb_antennas_rx*sizeof(int32_t*) );
	  
	  for (i=0; i<fp->nb_antennas_rx; i++) {
	    // RK 2 times because of output format of FFT!
	    // FIXME We should get rid of this
	    pusch_vars[UE_id]->rxdataF_ext[eNB_id][i]      = (int32_t*)malloc16_clear( 2*sizeof(int32_t)*fp->N_RB_UL*12*fp->symbols_per_tti );
	    pusch_vars[UE_id]->rxdataF_ext2[eNB_id][i]     = (int32_t*)malloc16_clear( sizeof(int32_t)*fp->N_RB_UL*12*fp->symbols_per_tti );
	    pusch_vars[UE_id]->drs_ch_estimates[eNB_id][i] = (int32_t*)malloc16_clear( sizeof(int32_t)*fp->N_RB_UL*12*fp->symbols_per_tti );
	    pusch_vars[UE_id]->drs_ch_estimates_time[eNB_id][i] = (int32_t*)malloc16_clear( 2*2*sizeof(int32_t)*fp->ofdm_symbol_size );
	    pusch_vars[UE_id]->rxdataF_comp[eNB_id][i]     = (int32_t*)malloc16_clear( sizeof(int32_t)*fp->N_RB_UL*12*fp->symbols_per_tti );
	    pusch_vars[UE_id]->ul_ch_mag[eNB_id][i]  = (int32_t*)malloc16_clear( fp->symbols_per_tti*sizeof(int32_t)*fp->N_RB_UL*12 );
	    pusch_vars[UE_id]->ul_ch_magb[eNB_id][i] = (int32_t*)malloc16_clear( fp->symbols_per_tti*sizeof(int32_t)*fp->N_RB_UL*12 );
	  }
	} //eNB_id
	
	pusch_vars[UE_id]->llr = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
      } // abstraction_flag
    } //UE_id

    
    if (abstraction_flag==0) {
      if (is_secondary_eNB) {
	for (eNB_id=0; eNB_id<3; eNB_id++) {
	  eNB->dl_precoder_SeNB[eNB_id] = (int **)malloc16(4*sizeof(int*));
	  
	  if (eNB->dl_precoder_SeNB[eNB_id]) {
#ifdef DEBUG_PHY
	    printf("[openair][SECSYS_PHY][INIT] eNB->dl_precoder_SeNB[%d] allocated at %p\n",eNB_id,
		eNB->dl_precoder_SeNB[eNB_id]);
#endif
	  } else {
	    printf("[openair][SECSYS_PHY][INIT] eNB->dl_precoder_SeNB[%d] not allocated\n",eNB_id);
	    return(-1);
	  }
	  
	  for (j=0; j<fp->nb_antennas_tx; j++) {
	    eNB->dl_precoder_SeNB[eNB_id][j] = (int *)malloc16(2*sizeof(int)*(fp->ofdm_symbol_size)); // repeated format (hence the '2*')
	    
	    if (eNB->dl_precoder_SeNB[eNB_id][j]) {
#ifdef DEBUG_PHY
	      printf("[openair][LTE_PHY][INIT] eNB->dl_precoder_SeNB[%d][%d] allocated at %p\n",eNB_id,j,
		  eNB->dl_precoder_SeNB[eNB_id][j]);
#endif
	      memset(eNB->dl_precoder_SeNB[eNB_id][j],0,2*sizeof(int)*(fp->ofdm_symbol_size));
	    } else {
	      printf("[openair][LTE_PHY][INIT] eNB->dl_precoder_SeNB[%d][%d] not allocated\n",eNB_id,j);
	      return(-1);
	    }
	  } //for(j=...nb_antennas_tx
	  
	} //for(eNB_id...
      }
    }
    
    for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++)
      eNB->UE_stats_ptr[UE_id] = &eNB->UE_stats[UE_id];
    
    eNB->pdsch_config_dedicated->p_a = dB0; //defaul value until overwritten by RRCConnectionReconfiguration

    init_prach_tables(839);
  } // node_function != NGFI_RRU_IF4p5

  return (0);
}

