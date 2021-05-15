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

#include "phy_init.h"
#include "SCHED_UE/sched_UE.h"
#include "PHY/phy_extern_ue.h"
#include "SIMULATION/TOOLS/sim.h"
#include "LTE_RadioResourceConfigCommonSIB.h"
#include "LTE_RadioResourceConfigDedicated.h"
#include "LTE_TDD-Config.h"
#include "LTE_MBSFN-SubframeConfigList.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "assertions.h"
#include <math.h>
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "nfapi/oai_integration/vendor_ext.h"
void init_7_5KHz(void);

uint8_t dmrs1_tab_ue[8] = {0,2,3,4,6,8,9,10};

void init_sss(void);

void phy_config_sib1_ue(module_id_t Mod_id,int CC_id,
                        uint8_t eNB_id,
                        LTE_TDD_Config_t *tdd_Config,
                        uint8_t SIwindowsize,
                        uint16_t SIperiod) {
  LTE_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][CC_id]->frame_parms;

  if (tdd_Config) {
    fp->tdd_config    = tdd_Config->subframeAssignment;
    fp->tdd_config_S  = tdd_Config->specialSubframePatterns;
  }

  fp->SIwindowsize  = SIwindowsize;
  fp->SIPeriod      = SIperiod;
}

void phy_config_sib2_ue(module_id_t Mod_id,int CC_id,
                        uint8_t eNB_id,
                        LTE_RadioResourceConfigCommonSIB_t *radioResourceConfigCommon,
                        LTE_ARFCN_ValueEUTRA_t *ul_CarrierFreq,
                        long *ul_Bandwidth,
                        LTE_AdditionalSpectrumEmission_t *additionalSpectrumEmission,
                        struct LTE_MBSFN_SubframeConfigList *mbsfn_SubframeConfigList) {
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
  compute_prach_seq(fp->prach_config_common.rootSequenceIndex,
                    fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
                    fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
                    fp->prach_config_common.prach_ConfigInfo.highSpeedFlag,
                    fp->frame_type,ue->X_u);
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
  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift            = dmrs1_tab_ue[radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift];
  init_ul_hopping(fp);
  fp->soundingrs_ul_config_common.enabled_flag                        = 0;

  if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon.present == LTE_SoundingRS_UL_ConfigCommon_PR_setup) {
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

      if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) {
        fp->MBSFN_config[i].fourFrames_flag = 0;
        fp->MBSFN_config[i].mbsfn_SubframeConfig = mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]; // 6-bit subframe configuration
        LOG_I(PHY, "[CONFIG] LTE_MBSFN_SubframeConfig[%d] oneFrame pattern is  %d\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
      } else if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames) { // 24-bit subframe configuration
        fp->MBSFN_config[i].fourFrames_flag = 1;
        fp->MBSFN_config[i].mbsfn_SubframeConfig =
          mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[2]|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[1]<<8)|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]<<16);
        LOG_I(PHY, "[CONFIG]  LTE_MBSFN_SubframeConfig[%d] fourFrame pattern is  %x\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
      }
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_CONFIG_SIB2, VCD_FUNCTION_OUT);
}

void phy_config_mbsfn_list_ue(module_id_t                          Mod_id,
                        int                                  CC_id,
                        struct LTE_MBSFN_SubframeConfigList *mbsfn_SubframeConfigList){
  // MBSFN

    PHY_VARS_UE *ue        = PHY_vars_UE_g[Mod_id][CC_id];
    LTE_DL_FRAME_PARMS *fp = &ue->frame_parms;
    int i;

  if (mbsfn_SubframeConfigList != NULL) {

    fp->num_MBSFN_config = mbsfn_SubframeConfigList->list.count;

    for (i=0; i<mbsfn_SubframeConfigList->list.count; i++) {
      fp->MBSFN_config[i].radioframeAllocationPeriod = mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationPeriod;
      fp->MBSFN_config[i].radioframeAllocationOffset = mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationOffset;

      if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) {
        fp->MBSFN_config[i].fourFrames_flag = 0;
        fp->MBSFN_config[i].mbsfn_SubframeConfig = mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]; // 6-bit subframe configuration
        LOG_I(PHY, "[CONFIG] LTE_MBSFN_SubframeConfig[%d] pattern is  %d\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
      } else if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames) { // 24-bit subframe configuration
        fp->MBSFN_config[i].fourFrames_flag = 1;
        fp->MBSFN_config[i].mbsfn_SubframeConfig =
          mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[2]|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[1]<<8)|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]<<16);
        LOG_I(PHY, "[CONFIG]  LTE_MBSFN_SubframeConfig[%d] pattern is  %x\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
      }
    }
  }
}


void phy_config_sib13_ue(module_id_t Mod_id,int CC_id,uint8_t eNB_id,int mbsfn_Area_idx,
                         long mbsfn_AreaId_r9) {
  LTE_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][CC_id]->frame_parms;
  LOG_I(PHY,"[UE%d] Applying MBSFN_Area_id %ld for index %d\n",Mod_id,mbsfn_AreaId_r9,mbsfn_Area_idx);

  if (mbsfn_Area_idx == 0) {
    fp->Nid_cell_mbsfn = (uint16_t)mbsfn_AreaId_r9;
    LOG_I(PHY,"Fix me: only called when mbsfn_Area_idx == 0)\n");
  }

  lte_gold_mbsfn(fp,PHY_vars_UE_g[Mod_id][CC_id]->lte_gold_mbsfn_table,fp->Nid_cell_mbsfn);
  lte_gold_mbsfn_khz_1dot25(fp,PHY_vars_UE_g[Mod_id][CC_id]->lte_gold_mbsfn_khz_1dot25_table,fp->Nid_cell_mbsfn);
}


void phy_config_sib1_fembms_ue(module_id_t Mod_id,int CC_id,
                               uint8_t eNB_id,
                               struct LTE_NonMBSFN_SubframeConfig_r14 *nonMBSFN_SubframeConfig) {
  PHY_VARS_UE *ue        = PHY_vars_UE_g[Mod_id][CC_id];
  LTE_DL_FRAME_PARMS *fp = &ue->frame_parms;

  if (nonMBSFN_SubframeConfig != NULL) {
    fp->NonMBSFN_config_flag = 1;
    fp->NonMBSFN_config.radioframeAllocationPeriod=nonMBSFN_SubframeConfig->radioFrameAllocationPeriod_r14;
    fp->NonMBSFN_config.radioframeAllocationOffset=nonMBSFN_SubframeConfig->radioFrameAllocationOffset_r14;
    fp->NonMBSFN_config.non_mbsfn_SubframeConfig=(nonMBSFN_SubframeConfig->subframeAllocation_r14.buf[0]<<1 | nonMBSFN_SubframeConfig->subframeAllocation_r14.buf[0]>>7);
  }
}



/*
 * Configures UE MAC and PHY with radioResourceCommon received in mobilityControlInfo IE during Handover
 */
void phy_config_afterHO_ue(module_id_t Mod_id,uint8_t CC_id,uint8_t eNB_id, LTE_MobilityControlInfo_t *mobilityControlInfo, uint8_t ho_failed) {
  if(mobilityControlInfo!=NULL) {
    LTE_RadioResourceConfigCommon_t *radioResourceConfigCommon = &mobilityControlInfo->radioResourceConfigCommon;
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
    compute_prach_seq(PHY_vars_UE_g[Mod_id][CC_id]->frame_parms.prach_config_common.rootSequenceIndex,
                      PHY_vars_UE_g[Mod_id][CC_id]->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
                      PHY_vars_UE_g[Mod_id][CC_id]->frame_parms.prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
                      PHY_vars_UE_g[Mod_id][CC_id]->frame_parms.prach_config_common.prach_ConfigInfo.highSpeedFlag,
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

    if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon->present == LTE_SoundingRS_UL_ConfigCommon_PR_setup) {
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
    PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[0][eNB_id]->crnti = mobilityControlInfo->newUE_Identity.buf[0]|(mobilityControlInfo->newUE_Identity.buf[1]<<8);
    PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[1][eNB_id]->crnti = mobilityControlInfo->newUE_Identity.buf[0]|(mobilityControlInfo->newUE_Identity.buf[1]<<8);
    LOG_I(PHY,"SET C-RNTI %x %x\n",PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[0][eNB_id]->crnti,
          PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[1][eNB_id]->crnti);
  }

  if(ho_failed) {
    LOG_D(PHY,"[UE%d] Handover failed, triggering RACH procedure\n",Mod_id);
    memcpy((void *)&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms,(void *)&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms_before_ho, sizeof(LTE_DL_FRAME_PARMS));
    PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[eNB_id] = PRACH;
  }
}

void phy_config_meas_ue(module_id_t Mod_id,uint8_t CC_id,uint8_t eNB_index,uint8_t n_adj_cells,unsigned int *adj_cell_id) {
  PHY_MEASUREMENTS *phy_meas = &PHY_vars_UE_g[Mod_id][CC_id]->measurements;
  int i;
  LOG_I(PHY,"Configuring inter-cell measurements for %d cells, ids: \n",n_adj_cells);

  for (i=0; i<n_adj_cells; i++) {
    LOG_I(PHY,"%d\n",adj_cell_id[i]);
    lte_gold(&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms,PHY_vars_UE_g[Mod_id][CC_id]->lte_gold_table[i+1],adj_cell_id[i]);
  }

  phy_meas->n_adj_cells = n_adj_cells;
  memcpy((void *)phy_meas->adj_cell_id,(void *)adj_cell_id,n_adj_cells*sizeof(unsigned int));
}


void phy_config_dedicated_scell_ue(uint8_t Mod_id,
                                   uint8_t eNB_index,
                                   LTE_SCellToAddMod_r10_t *sCellToAddMod_r10,
                                   int CC_id) {
}



void phy_config_harq_ue(module_id_t Mod_id,
                        int CC_id,
                        uint8_t eNB_id,
                        uint16_t max_harq_tx) {
  PHY_VARS_UE *phy_vars_ue = PHY_vars_UE_g[Mod_id][CC_id];
  phy_vars_ue->ulsch[eNB_id]->Mlimit = max_harq_tx;
}

extern uint16_t beta_cqi[16];

void phy_config_dedicated_ue(module_id_t Mod_id,int CC_id,uint8_t eNB_id,
                             struct LTE_PhysicalConfigDedicated *physicalConfigDedicated ) {
  static uint8_t first_dedicated_configuration = 0;
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
      if (physicalConfigDedicated->pucch_ConfigDedicated->ackNackRepetition.present == LTE_PUCCH_ConfigDedicated__ackNackRepetition_PR_release)
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
      LOG_D(PHY,"pusch_config_dedicated.betaOffset_CQI_Index %d => %d)\n",phy_vars_ue->pusch_config_dedicated[eNB_id].betaOffset_CQI_Index,
            beta_cqi[phy_vars_ue->pusch_config_dedicated[eNB_id].betaOffset_CQI_Index]);
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
      LOG_I(PHY,"Transmission Mode %d\n",phy_vars_ue->transmission_mode[eNB_id]);

      switch(physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode) {
        case LTE_AntennaInfoDedicated__transmissionMode_tm1:
          phy_vars_ue->transmission_mode[eNB_id] = 1;
          break;

        case LTE_AntennaInfoDedicated__transmissionMode_tm2:
          phy_vars_ue->transmission_mode[eNB_id] = 2;
          break;

        case LTE_AntennaInfoDedicated__transmissionMode_tm3:
          phy_vars_ue->transmission_mode[eNB_id] = 3;
          break;

        case LTE_AntennaInfoDedicated__transmissionMode_tm4:
          phy_vars_ue->transmission_mode[eNB_id] = 4;
          break;

        case LTE_AntennaInfoDedicated__transmissionMode_tm5:
          phy_vars_ue->transmission_mode[eNB_id] = 5;
          break;

        case LTE_AntennaInfoDedicated__transmissionMode_tm6:
          phy_vars_ue->transmission_mode[eNB_id] = 6;
          break;

        case LTE_AntennaInfoDedicated__transmissionMode_tm7:
          lte_gold_ue_spec_port5(phy_vars_ue->lte_gold_uespec_port5_table, phy_vars_ue->frame_parms.Nid_cell, phy_vars_ue->pdcch_vars[0][eNB_id]->crnti);
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
      if (physicalConfigDedicated->schedulingRequestConfig->present == LTE_SchedulingRequestConfig_PR_setup) {
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

      if (physicalConfigDedicated->soundingRS_UL_ConfigDedicated->present == LTE_SoundingRS_UL_ConfigDedicated_PR_setup) {
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
        if (physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->present == LTE_CQI_ReportPeriodic_PR_setup) {
          // configure PUCCH CQI reporting
          phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PUCCH_ResourceIndex = physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_PUCCH_ResourceIndex;
          phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex     = physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.cqi_pmi_ConfigIndex;

          if (physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.ri_ConfigIndex)
            phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex = *physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.ri_ConfigIndex;
        } else if (physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->present == LTE_CQI_ReportPeriodic_PR_release) {
          // handle release
          phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex = -1;
          phy_vars_ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex = -1;
        }
      }
    }
  } else {
    LOG_D(PHY,"[PHY][UE %d] Received NULL radioResourceConfigDedicated from eNB %d\n",Mod_id,eNB_id);
    return;
  }

  // fill cqi parameters for periodic CQI reporting
  get_cqipmiri_params(phy_vars_ue,eNB_id);
  // disable MIB SIB decoding once we are on connected mode
  first_dedicated_configuration ++;

  if(first_dedicated_configuration > 1) {
    LOG_I(PHY,"Disable SIB MIB decoding \n");
    phy_vars_ue->decode_SIB = 0;
    phy_vars_ue->decode_MIB = 0;
  }

  if(NFAPI_MODE!=NFAPI_UE_STUB_PNF) {
    //phy_vars_ue->pdcch_vars[1][eNB_id]->crnti = phy_vars_ue->pdcch_vars[0][eNB_id]->crnti;
    if(phy_vars_ue->pdcch_vars[0][eNB_id]->crnti == 0x1234)
      phy_vars_ue->pdcch_vars[0][eNB_id]->crnti = phy_vars_ue->pdcch_vars[1][eNB_id]->crnti;
    else
      phy_vars_ue->pdcch_vars[1][eNB_id]->crnti = phy_vars_ue->pdcch_vars[0][eNB_id]->crnti;

    LOG_I(PHY,"C-RNTI %x %x \n", phy_vars_ue->pdcch_vars[0][eNB_id]->crnti,
          phy_vars_ue->pdcch_vars[1][eNB_id]->crnti);
  }
}

/*! \brief Helper function to allocate memory for DLSCH data structures.
 * \param[out] pdsch Pointer to the LTE_UE_PDSCH structure to initialize.
 * \param[in] frame_parms LTE_DL_FRAME_PARMS structure.
 * \note This function is optimistic in that it expects malloc() to succeed.
 */
void phy_init_lte_ue__PDSCH( LTE_UE_PDSCH *const pdsch, const LTE_DL_FRAME_PARMS *const fp ) {
  AssertFatal( pdsch, "pdsch==0" );
  pdsch->pmi_ext = (uint8_t *)malloc16_clear( fp->N_RB_DL );
  pdsch->llr[0] = (int16_t *)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
  pdsch->llr128 = (int16_t **)malloc16_clear( sizeof(int16_t *) );
  // FIXME! no further allocation for (int16_t*)pdsch->llr128 !!! expect SIGSEGV
  // FK, 11-3-2015: this is only as a temporary pointer, no memory is stored there
  pdsch->rxdataF_ext            = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->rxdataF_uespec_pilots  = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->rxdataF_comp0          = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->rho                    = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_ch_estimates_ext    = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->dl_bf_ch_estimates     = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->dl_bf_ch_estimates_ext = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  //pdsch->dl_ch_rho_ext          = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  //pdsch->dl_ch_rho2_ext         = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_mag0             = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->dl_ch_magb0            = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  // the allocated memory size is fixed:
  AssertFatal( fp->nb_antennas_rx <= 2, "nb_antennas_rx > 2" );

  for (int i=0; i<fp->nb_antennas_rx; i++) {
    pdsch->rho[i]     = (int32_t *)malloc16_clear( sizeof(int32_t)*(fp->N_RB_DL*12*7*2) );

    for (int j=0; j<4; j++) { //fp->nb_antennas_tx; j++)
      const int idx = (j<<1)+i;
      const size_t num = 7*2*fp->N_RB_DL*12;
      pdsch->rxdataF_ext[idx]             = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->rxdataF_uespec_pilots[idx]   = (int32_t *)malloc16_clear( sizeof(int32_t) * fp->N_RB_DL*12);
      pdsch->rxdataF_comp0[idx]           = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_estimates_ext[idx]     = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_bf_ch_estimates[idx]      = (int32_t *)malloc16_clear( sizeof(int32_t) * fp->ofdm_symbol_size*7*2);
      pdsch->dl_bf_ch_estimates_ext[idx]  = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      //pdsch->dl_ch_rho_ext[idx]           = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      //pdsch->dl_ch_rho2_ext[idx]          = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_mag0[idx]              = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_magb0[idx]             = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
    }
  }
}

int init_lte_ue_signal(PHY_VARS_UE *ue,
                       int nb_connected_eNB,
                       uint8_t abstraction_flag) {
  // create shortcuts
  LTE_DL_FRAME_PARMS *const fp            = &ue->frame_parms;
  LTE_UE_COMMON *const common_vars        = &ue->common_vars;
  LTE_UE_PDSCH **const pdsch_vars_SI      = ue->pdsch_vars_SI;
  LTE_UE_PDSCH **const pdsch_vars_ra      = ue->pdsch_vars_ra;
  LTE_UE_PDSCH **const pdsch_vars_p       = ue->pdsch_vars_p;
  //LTE_UE_PDSCH **const pdsch_vars_mch     = ue->pdsch_vars_MCH;
  LTE_UE_PDSCH* (*pdsch_vars_MCH_th)[][NUMBER_OF_CONNECTED_eNB_MAX] = &ue->pdsch_vars_MCH;
  LTE_UE_PDSCH* (*pdsch_vars_th)[][NUMBER_OF_CONNECTED_eNB_MAX+1] = &ue->pdsch_vars;
  LTE_UE_PDCCH* (*pdcch_vars_th)[][NUMBER_OF_CONNECTED_eNB_MAX]   = &ue->pdcch_vars;
  LTE_UE_PBCH **const pbch_vars           = ue->pbch_vars;
  LTE_UE_PRACH **const prach_vars         = ue->prach_vars;
  int i,j,k,l;
  int eNB_id;
  int th_id;
  LOG_D(PHY,"Initializing UE vars (abstraction %"PRIu8") for eNB TXant %"PRIu8", UE RXant %"PRIu8"\n",abstraction_flag,fp->nb_antennas_tx,fp->nb_antennas_rx);
  crcTableInit();
  load_dftslib();
  init_frame_parms(&ue->frame_parms,1);
  lte_sync_time_init(&ue->frame_parms);
  init_lte_top(&ue->frame_parms);
  init_sss();
  init_7_5KHz();
  init_ul_hopping(&ue->frame_parms);
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

  for (i=0; i<10; i++)
    ue->tx_power_dBm[i]=-127;

  // init TX buffers
  common_vars->txdata  = (int32_t **)malloc16( fp->nb_antennas_tx*sizeof(int32_t *) );
  common_vars->txdataF = (int32_t **)malloc16( fp->nb_antennas_tx*sizeof(int32_t *) );

  for (i=0; i<fp->nb_antennas_tx; i++) {
    common_vars->txdata[i]  = (int32_t *)malloc16_clear( fp->samples_per_tti*10*sizeof(int32_t) );
    common_vars->txdataF[i] = (int32_t *)malloc16_clear( fp->ofdm_symbol_size*fp->symbols_per_tti*10*sizeof(int32_t) );
  }

  // init RX buffers
  common_vars->rxdata   = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
  common_vars->common_vars_rx_data_per_thread[0].rxdataF  = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
  common_vars->common_vars_rx_data_per_thread[1].rxdataF  = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );

  for (i=0; i<fp->nb_antennas_rx; i++) {
    common_vars->rxdata[i] = (int32_t *) malloc16_clear( (fp->samples_per_tti*10+2048)*sizeof(int32_t) );
    LOG_I(PHY,"common_vars->rxdata[%d] %p\n",i,common_vars->rxdata[i]);
    common_vars->common_vars_rx_data_per_thread[0].rxdataF[i] = (int32_t *)malloc16_clear( sizeof(int32_t)*(fp->ofdm_symbol_size*14) );
    common_vars->common_vars_rx_data_per_thread[1].rxdataF[i] = (int32_t *)malloc16_clear( sizeof(int32_t)*(fp->ofdm_symbol_size*14) );
  }

  // Channel estimates
  for (eNB_id=0; eNB_id<7; eNB_id++) {
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      common_vars->common_vars_rx_data_per_thread[th_id].dl_ch_estimates[eNB_id]      = (int32_t **)malloc16_clear(8*sizeof(int32_t *));
      common_vars->common_vars_rx_data_per_thread[th_id].dl_ch_estimates_time[eNB_id] = (int32_t **)malloc16_clear(8*sizeof(int32_t *));
    }

    for (i=0; i<fp->nb_antennas_rx; i++)
      for (j=0; j<4; j++) {
        int idx = (j<<1) + i;

        for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
          common_vars->common_vars_rx_data_per_thread[th_id].dl_ch_estimates[eNB_id][idx] = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->symbols_per_tti*(fp->ofdm_symbol_size+LTE_CE_FILTER_LENGTH) );
          common_vars->common_vars_rx_data_per_thread[th_id].dl_ch_estimates_time[eNB_id][idx] = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->ofdm_symbol_size*2 );
        }
      }
  }

  // DLSCH
  for (eNB_id=0; eNB_id<ue->n_connected_eNB; eNB_id++) {
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      (*pdsch_vars_th)[th_id][eNB_id]     = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
      (*pdsch_vars_MCH_th)[th_id][eNB_id]     = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    }

    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      (*pdcch_vars_th)[th_id][eNB_id] = (LTE_UE_PDCCH *)malloc16_clear(sizeof(LTE_UE_PDCCH));
    }

    pdsch_vars_SI[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    pdsch_vars_ra[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    pdsch_vars_p[eNB_id]   = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    //pdsch_vars_mch[eNB_id] = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    prach_vars[eNB_id]     = (LTE_UE_PRACH *)malloc16_clear(sizeof(LTE_UE_PRACH));
    pbch_vars[eNB_id]      = (LTE_UE_PBCH *)malloc16_clear(sizeof(LTE_UE_PBCH));

    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      phy_init_lte_ue__PDSCH( (*pdsch_vars_th)[th_id][eNB_id], fp );
      phy_init_lte_ue__PDSCH( (*pdsch_vars_MCH_th)[th_id][eNB_id], fp );
    }

    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      (*pdsch_vars_th)[th_id][eNB_id]->llr_shifts      = (uint8_t *)malloc16_clear(7*2*fp->N_RB_DL*12);
      (*pdsch_vars_th)[th_id][eNB_id]->llr_shifts_p        = (*pdsch_vars_th)[0][eNB_id]->llr_shifts;
      (*pdsch_vars_th)[th_id][eNB_id]->llr[1]              = (int16_t *)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
      (*pdsch_vars_th)[th_id][eNB_id]->llr128_2ndstream    = (int16_t **)malloc16_clear( sizeof(int16_t *) );
      (*pdsch_vars_th)[th_id][eNB_id]->rho                 = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );

      (*pdsch_vars_MCH_th)[th_id][eNB_id]->llr_shifts      = (uint8_t *)malloc16_clear(7*2*fp->N_RB_DL*12);
      (*pdsch_vars_MCH_th)[th_id][eNB_id]->llr_shifts_p        = (*pdsch_vars_th)[0][eNB_id]->llr_shifts;
      (*pdsch_vars_MCH_th)[th_id][eNB_id]->llr[1]              = (int16_t *)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
      (*pdsch_vars_MCH_th)[th_id][eNB_id]->llr128_2ndstream    = (int16_t **)malloc16_clear( sizeof(int16_t *) );
      (*pdsch_vars_MCH_th)[th_id][eNB_id]->rho                 = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );

    }

    for (int i=0; i<fp->nb_antennas_rx; i++) {
      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        (*pdsch_vars_th)[th_id][eNB_id]->rho[i]     = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
	(*pdsch_vars_MCH_th)[th_id][eNB_id]->rho[i]     = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
      }
    }

    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      (*pdsch_vars_th)[th_id][eNB_id]->dl_ch_rho2_ext      = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      (*pdsch_vars_MCH_th)[th_id][eNB_id]->dl_ch_rho2_ext      = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
    }

    for (i=0; i<fp->nb_antennas_rx; i++)
      for (j=0; j<4; j++) {
        const int idx = (j<<1)+i;
        const size_t num = 7*2*fp->N_RB_DL*12+4;

        for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
          (*pdsch_vars_th)[th_id][eNB_id]->dl_ch_rho2_ext[idx] = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
	  (*pdsch_vars_MCH_th)[th_id][eNB_id]->dl_ch_rho2_ext[idx] = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
        }
      }

    //const size_t num = 7*2*fp->N_RB_DL*12+4;
    for (k=0; k<8; k++) { //harq_pid
      for (l=0; l<8; l++) { //round
        for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
          (*pdsch_vars_th)[th_id][eNB_id]->rxdataF_comp1[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
          (*pdsch_vars_th)[th_id][eNB_id]->dl_ch_rho_ext[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
          (*pdsch_vars_th)[th_id][eNB_id]->dl_ch_mag1[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
          (*pdsch_vars_th)[th_id][eNB_id]->dl_ch_magb1[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );

          (*pdsch_vars_MCH_th)[th_id][eNB_id]->rxdataF_comp1[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
          (*pdsch_vars_MCH_th)[th_id][eNB_id]->dl_ch_rho_ext[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
          (*pdsch_vars_MCH_th)[th_id][eNB_id]->dl_ch_mag1[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
          (*pdsch_vars_MCH_th)[th_id][eNB_id]->dl_ch_magb1[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
        }

        for (int i=0; i<fp->nb_antennas_rx; i++)
          for (int j=0; j<4; j++) { //frame_parms->nb_antennas_tx; j++)
            const int idx = (j<<1)+i;

            for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
              (*pdsch_vars_th)[th_id][eNB_id]->dl_ch_rho_ext[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
              (*pdsch_vars_th)[th_id][eNB_id]->rxdataF_comp1[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
              (*pdsch_vars_th)[th_id][eNB_id]->dl_ch_mag1[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
              (*pdsch_vars_th)[th_id][eNB_id]->dl_ch_magb1[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );

              (*pdsch_vars_MCH_th)[th_id][eNB_id]->dl_ch_rho_ext[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
              (*pdsch_vars_MCH_th)[th_id][eNB_id]->rxdataF_comp1[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
              (*pdsch_vars_MCH_th)[th_id][eNB_id]->dl_ch_mag1[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
              (*pdsch_vars_MCH_th)[th_id][eNB_id]->dl_ch_magb1[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
            }
          }
      }
    }

    phy_init_lte_ue__PDSCH( pdsch_vars_SI[eNB_id], fp );
    phy_init_lte_ue__PDSCH( pdsch_vars_ra[eNB_id], fp );
    phy_init_lte_ue__PDSCH( pdsch_vars_p[eNB_id], fp );
    //phy_init_lte_ue__PDSCH( pdsch_vars_mch[eNB_id], fp );

    // 100 PRBs * 12 REs/PRB * 4 PDCCH SYMBOLS * 2 LLRs/RE
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      (*pdcch_vars_th)[th_id][eNB_id]->llr   = (uint16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      (*pdcch_vars_th)[th_id][eNB_id]->llr16 = (uint16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      (*pdcch_vars_th)[th_id][eNB_id]->wbar  = (uint16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      (*pdcch_vars_th)[th_id][eNB_id]->e_rx  = (int8_t *)malloc16_clear( 4*2*100*12 );
      (*pdcch_vars_th)[th_id][eNB_id]->rxdataF_comp        = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      (*pdcch_vars_th)[th_id][eNB_id]->dl_ch_rho_ext       = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      (*pdcch_vars_th)[th_id][eNB_id]->rho                 = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
      (*pdcch_vars_th)[th_id][eNB_id]->rxdataF_ext         = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      (*pdcch_vars_th)[th_id][eNB_id]->dl_ch_estimates_ext = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
    }

    for (i=0; i<fp->nb_antennas_rx; i++) {
      //ue_pdcch_vars[eNB_id]->rho[i] = (int32_t*)malloc16_clear( sizeof(int32_t)*(fp->N_RB_DL*12*7*2) );
      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        (*pdcch_vars_th)[th_id][eNB_id]->rho[i] = (int32_t *)malloc16_clear( sizeof(int32_t)*(100*12*4) );
      }

      for (j=0; j<4; j++) { //fp->nb_antennas_tx; j++)
        int idx = (j<<1)+i;
        //  size_t num = 7*2*fp->N_RB_DL*12;
        size_t num = 4*100*12;  // 4 symbols, 100 PRBs, 12 REs per PRB

        for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
          (*pdcch_vars_th)[th_id][eNB_id]->rxdataF_comp[idx]        = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
          (*pdcch_vars_th)[th_id][eNB_id]->dl_ch_rho_ext[idx]       = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
          (*pdcch_vars_th)[th_id][eNB_id]->rxdataF_ext[idx]         = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
          (*pdcch_vars_th)[th_id][eNB_id]->dl_ch_estimates_ext[idx] = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
        }
      }
    }

    phy_init_lte_ue__PDSCH( pdsch_vars_SI[eNB_id], fp );
    phy_init_lte_ue__PDSCH( pdsch_vars_ra[eNB_id], fp );
    phy_init_lte_ue__PDSCH( pdsch_vars_p[eNB_id], fp );
    //phy_init_lte_ue__PDSCH( pdsch_vars_mch[eNB_id], fp );

    // 100 PRBs * 12 REs/PRB * 4 PDCCH SYMBOLS * 2 LLRs/RE
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      (*pdcch_vars_th)[th_id][eNB_id]->llr   = (uint16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      (*pdcch_vars_th)[th_id][eNB_id]->llr16 = (uint16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      (*pdcch_vars_th)[th_id][eNB_id]->wbar  = (uint16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      (*pdcch_vars_th)[th_id][eNB_id]->e_rx  = (int8_t *)malloc16_clear( 4*2*100*12 );
      (*pdcch_vars_th)[th_id][eNB_id]->rxdataF_comp        = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      (*pdcch_vars_th)[th_id][eNB_id]->dl_ch_rho_ext       = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      (*pdcch_vars_th)[th_id][eNB_id]->rho                 = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
      (*pdcch_vars_th)[th_id][eNB_id]->rxdataF_ext         = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      (*pdcch_vars_th)[th_id][eNB_id]->dl_ch_estimates_ext = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      (*pdcch_vars_th)[th_id][eNB_id]->dciFormat = 0;
      (*pdcch_vars_th)[th_id][eNB_id]->agregationLevel = 0xFF;
    }

    for (i=0; i<fp->nb_antennas_rx; i++) {
      //ue_pdcch_vars[eNB_id]->rho[i] = (int32_t*)malloc16_clear( sizeof(int32_t)*(fp->N_RB_DL*12*7*2) );
      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        (*pdcch_vars_th)[th_id][eNB_id]->rho[i] = (int32_t *)malloc16_clear( sizeof(int32_t)*(100*12*4) );
      }

      for (j=0; j<4; j++) { //fp->nb_antennas_tx; j++)
        int idx = (j<<1)+i;
        //  size_t num = 7*2*fp->N_RB_DL*12;
        size_t num = 4*100*12;  // 4 symbols, 100 PRBs, 12 REs per PRB

        for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
          (*pdcch_vars_th)[th_id][eNB_id]->rxdataF_comp[idx]        = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
          (*pdcch_vars_th)[th_id][eNB_id]->dl_ch_rho_ext[idx]       = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
          (*pdcch_vars_th)[th_id][eNB_id]->rxdataF_ext[idx]         = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
          (*pdcch_vars_th)[th_id][eNB_id]->dl_ch_estimates_ext[idx] = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
        }
      }
    }

    // PBCH
    pbch_vars[eNB_id]->rxdataF_ext         = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
    pbch_vars[eNB_id]->rxdataF_comp        = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
    pbch_vars[eNB_id]->dl_ch_estimates_ext = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
    pbch_vars[eNB_id]->llr                 = (int8_t *)malloc16_clear( 1920 );
    prach_vars[eNB_id]->prachF             = (int16_t *)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(fp->ofdm_symbol_size*12)) );
    prach_vars[eNB_id]->prach              = (int16_t *)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(fp->ofdm_symbol_size*12)) );

    for (i=0; i<fp->nb_antennas_rx; i++) {
      pbch_vars[eNB_id]->rxdataF_ext[i]    = (int32_t *)malloc16_clear( sizeof(int32_t)*6*12*4 );

      for (j=0; j<4; j++) {//fp->nb_antennas_tx;j++) {
        int idx = (j<<1)+i;
        pbch_vars[eNB_id]->rxdataF_comp[idx]        = (int32_t *)malloc16_clear( sizeof(int32_t)*6*12*4 );
        pbch_vars[eNB_id]->dl_ch_estimates_ext[idx] = (int32_t *)malloc16_clear( sizeof(int32_t)*6*12*4 );
      }
    }

    pbch_vars[eNB_id]->decoded_output = (uint8_t *)malloc16_clear( 64 );
  }

  // initialization for the last instance of pdsch_vars (used for MU-MIMO)
  for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
    (*pdsch_vars_th)[th_id][eNB_id]     = (LTE_UE_PDSCH *)malloc16_clear( sizeof(LTE_UE_PDSCH) );
  }

  pdsch_vars_SI[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear( sizeof(LTE_UE_PDSCH) );
  pdsch_vars_ra[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear( sizeof(LTE_UE_PDSCH) );
  pdsch_vars_p[eNB_id]   = (LTE_UE_PDSCH *)malloc16_clear( sizeof(LTE_UE_PDSCH) );

  for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
    phy_init_lte_ue__PDSCH( (*pdsch_vars_th)[th_id][eNB_id], fp );
    (*pdsch_vars_th)[th_id][eNB_id]->llr[1] = (int16_t *)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
  }

  ue->sinr_CQI_dB = (double *) malloc16_clear( fp->N_RB_DL*12*sizeof(double) );
  ue->init_averaging = 1;

  // default value until overwritten by RRCConnectionReconfiguration
  if (fp->nb_antenna_ports_eNB==2)
    ue->pdsch_config_dedicated->p_a = dBm3;
  else
    ue->pdsch_config_dedicated->p_a = dB0;

  // set channel estimation to do linear interpolation in time
  ue->high_speed_flag = 1;
  ue->ch_est_alpha    = 24576;
  // enable MIB/SIB decoding by default
  ue->decode_MIB = 1;
  ue->decode_SIB = 1;
  init_prach_tables(839);
  return 0;
}

void init_lte_ue_transport(PHY_VARS_UE *ue,int abstraction_flag) {
  int i,j,k;

  for (i=0; i<NUMBER_OF_CONNECTED_eNB_MAX; i++) {
    for (j=0; j<2; j++) {
      for (k=0; k<2; k++) {
        AssertFatal((ue->dlsch[k][i][j]  = new_ue_dlsch(1,NUMBER_OF_HARQ_PID_MAX,NSOFT,MAX_TURBO_ITERATIONS,ue->frame_parms.N_RB_DL, abstraction_flag))!=NULL,"Can't get ue dlsch structures\n");
        LOG_D(PHY,"dlsch[%d][%d][%d] => %p\n",k,i,j,ue->dlsch[i][j]);
      }
    }

    AssertFatal((ue->ulsch[i]  = new_ue_ulsch(ue->frame_parms.N_RB_UL, abstraction_flag))!=NULL,"Can't get ue ulsch structures\n");
    ue->dlsch_SI[i]  = new_ue_dlsch(1,1,NSOFT,MAX_TURBO_ITERATIONS,ue->frame_parms.N_RB_DL, abstraction_flag);
    ue->dlsch_ra[i]  = new_ue_dlsch(1,1,NSOFT,MAX_TURBO_ITERATIONS,ue->frame_parms.N_RB_DL, abstraction_flag);
    ue->transmission_mode[i] = ue->frame_parms.nb_antenna_ports_eNB==1 ? 1 : 2;
  }

  ue->frame_parms.pucch_config_common.deltaPUCCH_Shift = 1;
  ue->dlsch_MCH[0]  = new_ue_dlsch(1,NUMBER_OF_HARQ_PID_MAX,NSOFT,MAX_TURBO_ITERATIONS_MBSFN,ue->frame_parms.N_RB_DL,0);
}
