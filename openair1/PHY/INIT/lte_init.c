/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
#ifdef EXMIMO
#include "openair0_lib.h"
#endif
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

#ifdef EXMIMO
extern openair0_rf_map rf_map[MAX_NUM_CCs];
#endif

extern uint16_t prach_root_sequence_map0_3[838];
extern uint16_t prach_root_sequence_map4[138];
uint8_t dmrs1_tab[8] = {0,2,3,4,6,8,9,10};

// FIXME not used anywhere
void phy_config_mib(LTE_DL_FRAME_PARMS *lte_frame_parms,
                    uint8_t N_RB_DL,
                    uint8_t Nid_cell,
                    uint8_t Ncp,
                    uint8_t frame_type,
                    uint8_t p_eNB,
                    PHICH_CONFIG_COMMON *phich_config)
{

  lte_frame_parms->N_RB_DL                            = N_RB_DL;
  lte_frame_parms->Nid_cell                           = Nid_cell;
  lte_frame_parms->nushift                            = Nid_cell%6;
  lte_frame_parms->Ncp                                = Ncp;
  lte_frame_parms->frame_type                         = frame_type;
  lte_frame_parms->nb_antennas_tx_eNB                 = p_eNB;
  lte_frame_parms->phich_config_common.phich_resource = phich_config->phich_resource;
  lte_frame_parms->phich_config_common.phich_duration = phich_config->phich_duration;
}

void phy_config_sib1_eNB(uint8_t Mod_id,
                         int CC_id,
                         TDD_Config_t *tdd_Config,
                         uint8_t SIwindowsize,
                         uint16_t SIPeriod)
{

  LTE_DL_FRAME_PARMS *lte_frame_parms = &PHY_vars_eNB_g[Mod_id][CC_id]->lte_frame_parms;

  if (tdd_Config) {
    lte_frame_parms->tdd_config    = tdd_Config->subframeAssignment;
    lte_frame_parms->tdd_config_S  = tdd_Config->specialSubframePatterns;
  }

  lte_frame_parms->SIwindowsize  = SIwindowsize;
  lte_frame_parms->SIPeriod      = SIPeriod;
}

void phy_config_sib1_ue(uint8_t Mod_id,int CC_id,
                        uint8_t CH_index,
                        TDD_Config_t *tdd_Config,
                        uint8_t SIwindowsize,
                        uint16_t SIperiod)
{

  LTE_DL_FRAME_PARMS *lte_frame_parms = &PHY_vars_UE_g[Mod_id][CC_id]->lte_frame_parms;

  if (tdd_Config) {
    lte_frame_parms->tdd_config    = tdd_Config->subframeAssignment;
    lte_frame_parms->tdd_config_S  = tdd_Config->specialSubframePatterns;
  }

  lte_frame_parms->SIwindowsize  = SIwindowsize;
  lte_frame_parms->SIPeriod      = SIperiod;
}

void phy_config_sib2_eNB(uint8_t Mod_id,
                         int CC_id,
                         RadioResourceConfigCommonSIB_t *radioResourceConfigCommon,
                         ARFCN_ValueEUTRA_t *ul_CArrierFreq,
                         long *ul_Bandwidth,
                         AdditionalSpectrumEmission_t *additionalSpectrumEmission,
                         struct MBSFN_SubframeConfigList  *mbsfn_SubframeConfigList)
{

  LTE_DL_FRAME_PARMS *lte_frame_parms = &PHY_vars_eNB_g[Mod_id][CC_id]->lte_frame_parms;
  //LTE_eNB_UE_stats *eNB_UE_stats      = PHY_vars_eNB_g[Mod_id][CC_id]->eNB_UE_stats;
  //int32_t rx_total_gain_eNB_dB        = PHY_vars_eNB_g[Mod_id][CC_id]->rx_total_gain_eNB_dB;
  int i;

  LOG_D(PHY,"[eNB%d] CCid %d Frame %d: Applying radioResourceConfigCommon\n",Mod_id,CC_id,PHY_vars_eNB_g[Mod_id][CC_id]->proc[8].frame_tx);

  lte_frame_parms->prach_config_common.rootSequenceIndex                           =radioResourceConfigCommon->prach_Config.rootSequenceIndex;
  LOG_D(PHY,"prach_config_common.rootSequenceIndex = %d\n",lte_frame_parms->prach_config_common.rootSequenceIndex );

  lte_frame_parms->prach_config_common.prach_Config_enabled=1;

  lte_frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex          =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_ConfigIndex;
  LOG_D(PHY,"prach_config_common.prach_ConfigInfo.prach_ConfigIndex = %d\n",lte_frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex);

  lte_frame_parms->prach_config_common.prach_ConfigInfo.highSpeedFlag              =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.highSpeedFlag;
  LOG_D(PHY,"prach_config_common.prach_ConfigInfo.highSpeedFlag = %d\n",lte_frame_parms->prach_config_common.prach_ConfigInfo.highSpeedFlag);
  lte_frame_parms->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig  =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig;
  LOG_D(PHY,"prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig = %d\n",lte_frame_parms->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig);
  lte_frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset           =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_FreqOffset;
  LOG_D(PHY,"prach_config_common.prach_ConfigInfo.prach_FreqOffset = %d\n",lte_frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset);
  compute_prach_seq(&lte_frame_parms->prach_config_common,lte_frame_parms->frame_type,
                    PHY_vars_eNB_g[Mod_id][CC_id]->X_u);

  lte_frame_parms->pucch_config_common.deltaPUCCH_Shift = 1+radioResourceConfigCommon->pucch_ConfigCommon.deltaPUCCH_Shift;
  lte_frame_parms->pucch_config_common.nRB_CQI          = radioResourceConfigCommon->pucch_ConfigCommon.nRB_CQI;
  lte_frame_parms->pucch_config_common.nCS_AN           = radioResourceConfigCommon->pucch_ConfigCommon.nCS_AN;
  lte_frame_parms->pucch_config_common.n1PUCCH_AN       = radioResourceConfigCommon->pucch_ConfigCommon.n1PUCCH_AN;



  lte_frame_parms->pdsch_config_common.referenceSignalPower = radioResourceConfigCommon->pdsch_ConfigCommon.referenceSignalPower;
  lte_frame_parms->pdsch_config_common.p_b                  = radioResourceConfigCommon->pdsch_ConfigCommon.p_b;


  lte_frame_parms->pusch_config_common.n_SB                                         = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.n_SB;
  LOG_D(PHY,"pusch_config_common.n_SB = %d\n",lte_frame_parms->pusch_config_common.n_SB );

  lte_frame_parms->pusch_config_common.hoppingMode                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode;
  LOG_D(PHY,"pusch_config_common.hoppingMode = %d\n",lte_frame_parms->pusch_config_common.hoppingMode);

  lte_frame_parms->pusch_config_common.pusch_HoppingOffset                          = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset;
  LOG_D(PHY,"pusch_config_common.pusch_HoppingOffset = %d\n",lte_frame_parms->pusch_config_common.pusch_HoppingOffset);

  lte_frame_parms->pusch_config_common.enable64QAM                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM;
  LOG_D(PHY,"pusch_config_common.enable64QAM = %d\n",lte_frame_parms->pusch_config_common.enable64QAM );

  lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled    = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled;
  LOG_D(PHY,"pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled = %d\n",lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled);

  lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH   = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH;
  LOG_D(PHY,"pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH = %d\n",lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH);

  lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled;
  LOG_D(PHY,"pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = %d\n",lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled);

  lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift            = dmrs1_tab[radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift];
  LOG_D(PHY,"pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift = %d\n",lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift);

  init_ul_hopping(lte_frame_parms);

  lte_frame_parms->soundingrs_ul_config_common.enabled_flag                        = 0;

  if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon.present==SoundingRS_UL_ConfigCommon_PR_setup) {
    lte_frame_parms->soundingrs_ul_config_common.enabled_flag                        = 1;
    lte_frame_parms->soundingrs_ul_config_common.srs_BandwidthConfig                 = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_BandwidthConfig;
    lte_frame_parms->soundingrs_ul_config_common.srs_SubframeConfig                  = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig;
    lte_frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission;

    if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts)
      lte_frame_parms->soundingrs_ul_config_common.srs_MaxUpPts                      = 1;
    else
      lte_frame_parms->soundingrs_ul_config_common.srs_MaxUpPts                      = 0;
  }



  lte_frame_parms->ul_power_control_config_common.p0_NominalPUSCH       = radioResourceConfigCommon->uplinkPowerControlCommon.p0_NominalPUSCH;
  lte_frame_parms->ul_power_control_config_common.alpha                 = radioResourceConfigCommon->uplinkPowerControlCommon.alpha;
  lte_frame_parms->ul_power_control_config_common.p0_NominalPUCCH       = radioResourceConfigCommon->uplinkPowerControlCommon.p0_NominalPUCCH;
  lte_frame_parms->ul_power_control_config_common.deltaPreambleMsg3     = radioResourceConfigCommon->uplinkPowerControlCommon.deltaPreambleMsg3;
  lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format1  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1;
  lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format1b  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b;
  lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format2  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2;
  lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format2a  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a;
  lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format2b  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b;

  lte_frame_parms->maxHARQ_Msg3Tx = radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx;


  // Now configure some of the Physical Channels

  // PUCCH

  init_ncs_cell(lte_frame_parms,PHY_vars_eNB_g[Mod_id][CC_id]->ncs_cell);

  init_ul_hopping(lte_frame_parms);


  // MBSFN
  if (mbsfn_SubframeConfigList != NULL) {
    lte_frame_parms->num_MBSFN_config = mbsfn_SubframeConfigList->list.count;

    for (i=0; i<mbsfn_SubframeConfigList->list.count; i++) {
      lte_frame_parms->MBSFN_config[i].radioframeAllocationPeriod = mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationPeriod;
      lte_frame_parms->MBSFN_config[i].radioframeAllocationOffset = mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationOffset;

      if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) {
        lte_frame_parms->MBSFN_config[i].fourFrames_flag = 0;
        lte_frame_parms->MBSFN_config[i].mbsfn_SubframeConfig = mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]; // 6-bit subframe configuration
        LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %ld\n", i,
              lte_frame_parms->MBSFN_config[i].mbsfn_SubframeConfig);
      } else if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames) { // 24-bit subframe configuration
        lte_frame_parms->MBSFN_config[i].fourFrames_flag = 1;
        lte_frame_parms->MBSFN_config[i].mbsfn_SubframeConfig =
          mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[1]<<8)|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[2]<<16);

        LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %ld\n", i,
              lte_frame_parms->MBSFN_config[i].mbsfn_SubframeConfig);
      }
    }

  } else
    lte_frame_parms->num_MBSFN_config = 0;
}


void phy_config_sib2_ue(uint8_t Mod_id,int CC_id,
                        uint8_t CH_index,
                        RadioResourceConfigCommonSIB_t *radioResourceConfigCommon,
                        ARFCN_ValueEUTRA_t *ul_CarrierFreq,
                        long *ul_Bandwidth,
                        AdditionalSpectrumEmission_t *additionalSpectrumEmission,
                        struct MBSFN_SubframeConfigList *mbsfn_SubframeConfigList)
{

  LTE_DL_FRAME_PARMS *lte_frame_parms = &PHY_vars_UE_g[Mod_id][CC_id]->lte_frame_parms;
  int i;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_CONFIG_SIB2, VCD_FUNCTION_IN);

  LOG_I(PHY,"[UE%d] Frame %d: Applying radioResourceConfigCommon from eNB%d\n",Mod_id,PHY_vars_UE_g[Mod_id][CC_id]->frame_rx,CH_index);

  lte_frame_parms->prach_config_common.rootSequenceIndex                           =radioResourceConfigCommon->prach_Config.rootSequenceIndex;

  lte_frame_parms->prach_config_common.prach_Config_enabled=1;
  lte_frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex          =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_ConfigIndex;
  lte_frame_parms->prach_config_common.prach_ConfigInfo.highSpeedFlag              =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.highSpeedFlag;
  lte_frame_parms->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig  =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig;
  lte_frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset           =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_FreqOffset;

  compute_prach_seq(&lte_frame_parms->prach_config_common,lte_frame_parms->frame_type,PHY_vars_UE_g[Mod_id][CC_id]->X_u);



  lte_frame_parms->pucch_config_common.deltaPUCCH_Shift = 1+radioResourceConfigCommon->pucch_ConfigCommon.deltaPUCCH_Shift;
  lte_frame_parms->pucch_config_common.nRB_CQI          = radioResourceConfigCommon->pucch_ConfigCommon.nRB_CQI;
  lte_frame_parms->pucch_config_common.nCS_AN           = radioResourceConfigCommon->pucch_ConfigCommon.nCS_AN;
  lte_frame_parms->pucch_config_common.n1PUCCH_AN       = radioResourceConfigCommon->pucch_ConfigCommon.n1PUCCH_AN;



  lte_frame_parms->pdsch_config_common.referenceSignalPower = radioResourceConfigCommon->pdsch_ConfigCommon.referenceSignalPower;
  lte_frame_parms->pdsch_config_common.p_b                  = radioResourceConfigCommon->pdsch_ConfigCommon.p_b;


  lte_frame_parms->pusch_config_common.n_SB                                         = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.n_SB;
  lte_frame_parms->pusch_config_common.hoppingMode                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode;
  lte_frame_parms->pusch_config_common.pusch_HoppingOffset                          = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset;
  lte_frame_parms->pusch_config_common.enable64QAM                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM;
  lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled    = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled;
  lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH   = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH;
  lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled;
  lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift            = dmrs1_tab[radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift];


  init_ul_hopping(lte_frame_parms);
  lte_frame_parms->soundingrs_ul_config_common.enabled_flag                        = 0;

  if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon.present==SoundingRS_UL_ConfigCommon_PR_setup) {
    lte_frame_parms->soundingrs_ul_config_common.enabled_flag                        = 1;
    lte_frame_parms->soundingrs_ul_config_common.srs_BandwidthConfig                 = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_BandwidthConfig;
    lte_frame_parms->soundingrs_ul_config_common.srs_SubframeConfig                  = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_SubframeConfig;
    lte_frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission = radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission;

    if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon.choice.setup.srs_MaxUpPts)
      lte_frame_parms->soundingrs_ul_config_common.srs_MaxUpPts                      = 1;
    else
      lte_frame_parms->soundingrs_ul_config_common.srs_MaxUpPts                      = 0;
  }



  lte_frame_parms->ul_power_control_config_common.p0_NominalPUSCH   = radioResourceConfigCommon->uplinkPowerControlCommon.p0_NominalPUSCH;
  lte_frame_parms->ul_power_control_config_common.alpha             = radioResourceConfigCommon->uplinkPowerControlCommon.alpha;
  lte_frame_parms->ul_power_control_config_common.p0_NominalPUCCH   = radioResourceConfigCommon->uplinkPowerControlCommon.p0_NominalPUCCH;
  lte_frame_parms->ul_power_control_config_common.deltaPreambleMsg3 = radioResourceConfigCommon->uplinkPowerControlCommon.deltaPreambleMsg3;
  lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format1  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1;
  lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format1b  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format1b;
  lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format2  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2;
  lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format2a  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2a;
  lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format2b  = radioResourceConfigCommon->uplinkPowerControlCommon.deltaFList_PUCCH.deltaF_PUCCH_Format2b;

  lte_frame_parms->maxHARQ_Msg3Tx = radioResourceConfigCommon->rach_ConfigCommon.maxHARQ_Msg3Tx;

  // Now configure some of the Physical Channels

  // PUCCH
  init_ncs_cell(lte_frame_parms,PHY_vars_UE_g[Mod_id][CC_id]->ncs_cell);

  init_ul_hopping(lte_frame_parms);


  // MBSFN

  if (mbsfn_SubframeConfigList != NULL) {
    lte_frame_parms->num_MBSFN_config = mbsfn_SubframeConfigList->list.count;

    for (i=0; i<mbsfn_SubframeConfigList->list.count; i++) {
      lte_frame_parms->MBSFN_config[i].radioframeAllocationPeriod = mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationPeriod;
      lte_frame_parms->MBSFN_config[i].radioframeAllocationOffset = mbsfn_SubframeConfigList->list.array[i]->radioframeAllocationOffset;

      if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) {
        lte_frame_parms->MBSFN_config[i].fourFrames_flag = 0;
        lte_frame_parms->MBSFN_config[i].mbsfn_SubframeConfig = mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]; // 6-bit subframe configuration
        LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %ld\n", i,
              lte_frame_parms->MBSFN_config[i].mbsfn_SubframeConfig);
      } else if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames) { // 24-bit subframe configuration
        lte_frame_parms->MBSFN_config[i].fourFrames_flag = 1;
        lte_frame_parms->MBSFN_config[i].mbsfn_SubframeConfig =
          mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[1]<<8)|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[2]<<16);

        LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %ld\n", i,
              lte_frame_parms->MBSFN_config[i].mbsfn_SubframeConfig);
      }
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_CONFIG_SIB2, VCD_FUNCTION_OUT);

}

void phy_config_sib13_ue(uint8_t Mod_id,int CC_id,uint8_t CH_index,int mbsfn_Area_idx,
                         long mbsfn_AreaId_r9)
{

  LTE_DL_FRAME_PARMS *lte_frame_parms = &PHY_vars_UE_g[Mod_id][CC_id]->lte_frame_parms;


  LOG_I(PHY,"[UE%d] Frame %d: Applying MBSFN_Area_id %d for index %d\n",Mod_id,PHY_vars_UE_g[Mod_id][CC_id]->frame_rx,mbsfn_AreaId_r9,mbsfn_Area_idx);

  if (mbsfn_Area_idx == 0) {
    lte_frame_parms->Nid_cell_mbsfn = (uint16_t)mbsfn_AreaId_r9;
    LOG_N(PHY,"Fix me: only called when mbsfn_Area_idx == 0)\n");
  }

  lte_gold_mbsfn(lte_frame_parms,PHY_vars_UE_g[Mod_id][CC_id]->lte_gold_mbsfn_table,lte_frame_parms->Nid_cell_mbsfn);

}


void phy_config_sib13_eNB(uint8_t Mod_id,int CC_id,int mbsfn_Area_idx,
                          long mbsfn_AreaId_r9)
{

  LTE_DL_FRAME_PARMS *lte_frame_parms = &PHY_vars_eNB_g[Mod_id][CC_id]->lte_frame_parms;


  LOG_I(PHY,"[eNB%d] Frame %d: Applying MBSFN_Area_id %d for index %d\n",Mod_id,PHY_vars_eNB_g[Mod_id][CC_id]->proc[8].frame_tx,mbsfn_AreaId_r9,mbsfn_Area_idx);

  if (mbsfn_Area_idx == 0) {
    lte_frame_parms->Nid_cell_mbsfn = (uint16_t)mbsfn_AreaId_r9;
    LOG_N(PHY,"Fix me: only called when mbsfn_Area_idx == 0)\n");
  }

  lte_gold_mbsfn(lte_frame_parms,PHY_vars_eNB_g[Mod_id][CC_id]->lte_gold_mbsfn_table,lte_frame_parms->Nid_cell_mbsfn);
}


void phy_config_dedicated_eNB_step2(PHY_VARS_eNB *phy_vars_eNB)
{

  uint8_t UE_id;
  struct PhysicalConfigDedicated *physicalConfigDedicated;

  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    physicalConfigDedicated = phy_vars_eNB->physicalConfigDedicated[UE_id];

    if (physicalConfigDedicated != NULL) {
      LOG_I(PHY,"[eNB %d] Frame %d: Sent physicalConfigDedicated=%p for UE %d\n",phy_vars_eNB->Mod_id, phy_vars_eNB->proc[8].frame_tx,physicalConfigDedicated,UE_id);
      LOG_D(PHY,"------------------------------------------------------------------------\n");

      if (physicalConfigDedicated->pdsch_ConfigDedicated) {
        phy_vars_eNB->pdsch_config_dedicated[UE_id].p_a=physicalConfigDedicated->pdsch_ConfigDedicated->p_a;
        LOG_D(PHY,"pdsch_config_dedicated.p_a %d\n",phy_vars_eNB->pdsch_config_dedicated[UE_id].p_a);
        LOG_D(PHY,"\n");
      }

      if (physicalConfigDedicated->pucch_ConfigDedicated) {
        if (physicalConfigDedicated->pucch_ConfigDedicated->ackNackRepetition.present==PUCCH_ConfigDedicated__ackNackRepetition_PR_release)
          phy_vars_eNB->pucch_config_dedicated[UE_id].ackNackRepetition=0;
        else {
          phy_vars_eNB->pucch_config_dedicated[UE_id].ackNackRepetition=1;
        }

        if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) {
          phy_vars_eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode = multiplexing;
        } else {
          if (physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode)
            phy_vars_eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode = *physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode;
          else
            phy_vars_eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode = bundling;
        }

        if ( phy_vars_eNB->pucch_config_dedicated[UE_id].tdd_AckNackFeedbackMode == multiplexing)
          LOG_D(PHY,"pucch_config_dedicated.tdd_AckNackFeedbackMode = multiplexing\n");
        else
          LOG_D(PHY,"pucch_config_dedicated.tdd_AckNackFeedbackMode = bundling\n");

      }

      if (physicalConfigDedicated->pusch_ConfigDedicated) {
        phy_vars_eNB->pusch_config_dedicated[UE_id].betaOffset_ACK_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_ACK_Index;
        phy_vars_eNB->pusch_config_dedicated[UE_id].betaOffset_RI_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_RI_Index;
        phy_vars_eNB->pusch_config_dedicated[UE_id].betaOffset_CQI_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_CQI_Index;

        LOG_D(PHY,"pusch_config_dedicated.betaOffset_ACK_Index %d\n",phy_vars_eNB->pusch_config_dedicated[UE_id].betaOffset_ACK_Index);
        LOG_D(PHY,"pusch_config_dedicated.betaOffset_RI_Index %d\n",phy_vars_eNB->pusch_config_dedicated[UE_id].betaOffset_RI_Index);
        LOG_D(PHY,"pusch_config_dedicated.betaOffset_CQI_Index %d\n",phy_vars_eNB->pusch_config_dedicated[UE_id].betaOffset_CQI_Index);
        LOG_D(PHY,"\n");


      }

      if (physicalConfigDedicated->uplinkPowerControlDedicated) {

        phy_vars_eNB->ul_power_control_dedicated[UE_id].p0_UE_PUSCH = physicalConfigDedicated->uplinkPowerControlDedicated->p0_UE_PUSCH;
        phy_vars_eNB->ul_power_control_dedicated[UE_id].deltaMCS_Enabled= physicalConfigDedicated->uplinkPowerControlDedicated->deltaMCS_Enabled;
        phy_vars_eNB->ul_power_control_dedicated[UE_id].accumulationEnabled= physicalConfigDedicated->uplinkPowerControlDedicated->accumulationEnabled;
        phy_vars_eNB->ul_power_control_dedicated[UE_id].p0_UE_PUCCH= physicalConfigDedicated->uplinkPowerControlDedicated->p0_UE_PUCCH;
        phy_vars_eNB->ul_power_control_dedicated[UE_id].pSRS_Offset= physicalConfigDedicated->uplinkPowerControlDedicated->pSRS_Offset;
        phy_vars_eNB->ul_power_control_dedicated[UE_id].filterCoefficient= *physicalConfigDedicated->uplinkPowerControlDedicated->filterCoefficient;
        LOG_D(PHY,"ul_power_control_dedicated.p0_UE_PUSCH %d\n",phy_vars_eNB->ul_power_control_dedicated[UE_id].p0_UE_PUSCH);
        LOG_D(PHY,"ul_power_control_dedicated.deltaMCS_Enabled %d\n",phy_vars_eNB->ul_power_control_dedicated[UE_id].deltaMCS_Enabled);
        LOG_D(PHY,"ul_power_control_dedicated.accumulationEnabled %d\n",phy_vars_eNB->ul_power_control_dedicated[UE_id].accumulationEnabled);
        LOG_D(PHY,"ul_power_control_dedicated.p0_UE_PUCCH %d\n",phy_vars_eNB->ul_power_control_dedicated[UE_id].p0_UE_PUCCH);
        LOG_D(PHY,"ul_power_control_dedicated.pSRS_Offset %d\n",phy_vars_eNB->ul_power_control_dedicated[UE_id].pSRS_Offset);
        LOG_D(PHY,"ul_power_control_dedicated.filterCoefficient %d\n",phy_vars_eNB->ul_power_control_dedicated[UE_id].filterCoefficient);
        LOG_D(PHY,"\n");
      }

      if (physicalConfigDedicated->antennaInfo) {
        phy_vars_eNB->transmission_mode[UE_id] = 1+(physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode);
        LOG_I(PHY,"Transmission Mode (phy_config_dedicated_eNB_step2) %d\n",phy_vars_eNB->transmission_mode[UE_id]);
        LOG_D(PHY,"\n");
      }

      if (physicalConfigDedicated->schedulingRequestConfig) {
        if (physicalConfigDedicated->schedulingRequestConfig->present == SchedulingRequestConfig_PR_setup) {
          phy_vars_eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex = physicalConfigDedicated->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex;
          phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex=physicalConfigDedicated->schedulingRequestConfig->choice.setup.sr_ConfigIndex;
          phy_vars_eNB->scheduling_request_config[UE_id].dsr_TransMax=physicalConfigDedicated->schedulingRequestConfig->choice.setup.dsr_TransMax;

          LOG_D(PHY,"scheduling_request_config.sr_PUCCH_ResourceIndex %d\n",phy_vars_eNB->scheduling_request_config[UE_id].sr_PUCCH_ResourceIndex);
          LOG_D(PHY,"scheduling_request_config.sr_ConfigIndex %d\n",phy_vars_eNB->scheduling_request_config[UE_id].sr_ConfigIndex);
          LOG_D(PHY,"scheduling_request_config.dsr_TransMax %d\n",phy_vars_eNB->scheduling_request_config[UE_id].dsr_TransMax);
        }

        LOG_D(PHY,"------------------------------------------------------------\n");

      }

      phy_vars_eNB->physicalConfigDedicated[UE_id] = NULL;
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
    memcpy((void *)&PHY_vars_UE_g[Mod_id][CC_id]->lte_frame_parms_before_ho,
           (void *)&PHY_vars_UE_g[Mod_id][CC_id]->lte_frame_parms,
           sizeof(LTE_DL_FRAME_PARMS));
    PHY_vars_UE_g[Mod_id][CC_id]->ho_triggered = 1;
    //PHY_vars_UE_g[UE_id]->UE_mode[0] = PRACH;

    LTE_DL_FRAME_PARMS *lte_frame_parms = &PHY_vars_UE_g[Mod_id][CC_id]->lte_frame_parms;
    //     int N_ZC;
    //     uint8_t prach_fmt;
    //     int u;

    LOG_I(PHY,"[UE%d] Frame %d: Handover triggered: Applying radioResourceConfigCommon from eNB %d\n",
          Mod_id,PHY_vars_UE_g[Mod_id][CC_id]->frame_rx,eNB_id);

    lte_frame_parms->prach_config_common.rootSequenceIndex                           =radioResourceConfigCommon->prach_Config.rootSequenceIndex;
    lte_frame_parms->prach_config_common.prach_Config_enabled=1;
    lte_frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex          =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->prach_ConfigIndex;
    lte_frame_parms->prach_config_common.prach_ConfigInfo.highSpeedFlag              =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->highSpeedFlag;
    lte_frame_parms->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig  =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->zeroCorrelationZoneConfig;
    lte_frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset           =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->prach_FreqOffset;

    //     prach_fmt = get_prach_fmt(radioResourceConfigCommon->prach_Config.prach_ConfigInfo->prach_ConfigIndex,lte_frame_parms->frame_type);
    //     N_ZC = (prach_fmt <4)?839:139;
    //     u = (prach_fmt < 4) ? prach_root_sequence_map0_3[lte_frame_parms->prach_config_common.rootSequenceIndex] :
    //       prach_root_sequence_map4[lte_frame_parms->prach_config_common.rootSequenceIndex];

    //compute_prach_seq(u,N_ZC, PHY_vars_UE_g[Mod_id]->X_u);
    compute_prach_seq(&PHY_vars_UE_g[Mod_id][CC_id]->lte_frame_parms.prach_config_common,
                      lte_frame_parms->frame_type,
                      PHY_vars_UE_g[Mod_id][CC_id]->X_u);


    lte_frame_parms->pucch_config_common.deltaPUCCH_Shift = 1+radioResourceConfigCommon->pucch_ConfigCommon->deltaPUCCH_Shift;
    lte_frame_parms->pucch_config_common.nRB_CQI          = radioResourceConfigCommon->pucch_ConfigCommon->nRB_CQI;
    lte_frame_parms->pucch_config_common.nCS_AN           = radioResourceConfigCommon->pucch_ConfigCommon->nCS_AN;
    lte_frame_parms->pucch_config_common.n1PUCCH_AN       = radioResourceConfigCommon->pucch_ConfigCommon->n1PUCCH_AN;
    lte_frame_parms->pdsch_config_common.referenceSignalPower = radioResourceConfigCommon->pdsch_ConfigCommon->referenceSignalPower;
    lte_frame_parms->pdsch_config_common.p_b                  = radioResourceConfigCommon->pdsch_ConfigCommon->p_b;


    lte_frame_parms->pusch_config_common.n_SB                                         = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.n_SB;
    lte_frame_parms->pusch_config_common.hoppingMode                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode;
    lte_frame_parms->pusch_config_common.pusch_HoppingOffset                          = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset;
    lte_frame_parms->pusch_config_common.enable64QAM                                  = radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM;
    lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled    = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled;
    lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH   = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH;
    lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled;
    lte_frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift            = radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift;

    init_ul_hopping(lte_frame_parms);
    lte_frame_parms->soundingrs_ul_config_common.enabled_flag                        = 0;

    if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon->present==SoundingRS_UL_ConfigCommon_PR_setup) {
      lte_frame_parms->soundingrs_ul_config_common.enabled_flag                        = 1;
      lte_frame_parms->soundingrs_ul_config_common.srs_BandwidthConfig                 = radioResourceConfigCommon->soundingRS_UL_ConfigCommon->choice.setup.srs_BandwidthConfig;
      lte_frame_parms->soundingrs_ul_config_common.srs_SubframeConfig                  = radioResourceConfigCommon->soundingRS_UL_ConfigCommon->choice.setup.srs_SubframeConfig;
      lte_frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission = radioResourceConfigCommon->soundingRS_UL_ConfigCommon->choice.setup.ackNackSRS_SimultaneousTransmission;

      if (radioResourceConfigCommon->soundingRS_UL_ConfigCommon->choice.setup.srs_MaxUpPts)
        lte_frame_parms->soundingrs_ul_config_common.srs_MaxUpPts                      = 1;
      else
        lte_frame_parms->soundingrs_ul_config_common.srs_MaxUpPts                      = 0;
    }

    lte_frame_parms->ul_power_control_config_common.p0_NominalPUSCH   = radioResourceConfigCommon->uplinkPowerControlCommon->p0_NominalPUSCH;
    lte_frame_parms->ul_power_control_config_common.alpha             = radioResourceConfigCommon->uplinkPowerControlCommon->alpha;
    lte_frame_parms->ul_power_control_config_common.p0_NominalPUCCH   = radioResourceConfigCommon->uplinkPowerControlCommon->p0_NominalPUCCH;
    lte_frame_parms->ul_power_control_config_common.deltaPreambleMsg3 = radioResourceConfigCommon->uplinkPowerControlCommon->deltaPreambleMsg3;
    lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format1  = radioResourceConfigCommon->uplinkPowerControlCommon->deltaFList_PUCCH.deltaF_PUCCH_Format1;
    lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format1b  = radioResourceConfigCommon->uplinkPowerControlCommon->deltaFList_PUCCH.deltaF_PUCCH_Format1b;
    lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format2  = radioResourceConfigCommon->uplinkPowerControlCommon->deltaFList_PUCCH.deltaF_PUCCH_Format2;
    lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format2a  = radioResourceConfigCommon->uplinkPowerControlCommon->deltaFList_PUCCH.deltaF_PUCCH_Format2a;
    lte_frame_parms->ul_power_control_config_common.deltaF_PUCCH_Format2b  = radioResourceConfigCommon->uplinkPowerControlCommon->deltaFList_PUCCH.deltaF_PUCCH_Format2b;

    lte_frame_parms->maxHARQ_Msg3Tx = radioResourceConfigCommon->rach_ConfigCommon->maxHARQ_Msg3Tx;

    // Now configure some of the Physical Channels
    if (radioResourceConfigCommon->antennaInfoCommon)
      lte_frame_parms->nb_antennas_tx                     = (1<<radioResourceConfigCommon->antennaInfoCommon->antennaPortsCount);
    else
      lte_frame_parms->nb_antennas_tx                     = 1;

    //PHICH
    if (radioResourceConfigCommon->antennaInfoCommon) {
      lte_frame_parms->phich_config_common.phich_resource = radioResourceConfigCommon->phich_Config->phich_Resource;
      lte_frame_parms->phich_config_common.phich_duration = radioResourceConfigCommon->phich_Config->phich_Duration;
    }

    //Target CellId
    lte_frame_parms->Nid_cell = mobilityControlInfo->targetPhysCellId;
    lte_frame_parms->nushift  = lte_frame_parms->Nid_cell%6;

    // PUCCH
    init_ncs_cell(lte_frame_parms,PHY_vars_UE_g[Mod_id][CC_id]->ncs_cell);

    init_ul_hopping(lte_frame_parms);

    // RNTI

    PHY_vars_UE_g[Mod_id][CC_id]->lte_ue_pdcch_vars[eNB_id]->crnti = mobilityControlInfo->newUE_Identity.buf[0]|(mobilityControlInfo->newUE_Identity.buf[1]<<8);

  }

  if(ho_failed) {
    LOG_D(PHY,"[UE%d] Handover failed, triggering RACH procedure\n",Mod_id);
    memcpy((void *)&PHY_vars_UE_g[Mod_id][CC_id]->lte_frame_parms,(void *)&PHY_vars_UE_g[Mod_id][CC_id]->lte_frame_parms_before_ho, sizeof(LTE_DL_FRAME_PARMS));
    PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[eNB_id] = PRACH;
  }
}

void phy_config_meas_ue(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index,uint8_t n_adj_cells,unsigned int *adj_cell_id)
{

  PHY_MEASUREMENTS *phy_meas = &PHY_vars_UE_g[Mod_id][CC_id]->PHY_measurements;
  int i;

  LOG_I(PHY,"Configuring inter-cell measurements for %d cells, ids: \n",n_adj_cells);

  for (i=0; i<n_adj_cells; i++) {
    LOG_I(PHY,"%d\n",adj_cell_id[i]);
    lte_gold(&PHY_vars_UE_g[Mod_id][CC_id]->lte_frame_parms,PHY_vars_UE_g[Mod_id][CC_id]->lte_gold_table[i+1],adj_cell_id[i]);
  }

  phy_meas->n_adj_cells = n_adj_cells;
  memcpy((void*)phy_meas->adj_cell_id,(void *)adj_cell_id,n_adj_cells*sizeof(unsigned int));

}

void phy_config_dedicated_eNB(uint8_t Mod_id,
                              int CC_id,
                              uint16_t rnti,
                              struct PhysicalConfigDedicated *physicalConfigDedicated)
{

  PHY_VARS_eNB *phy_vars_eNB = PHY_vars_eNB_g[Mod_id][CC_id];
  int8_t UE_id = find_ue(rnti,phy_vars_eNB);

  if (UE_id == -1) {
    LOG_E( PHY, "[eNB %"PRIu8"] Frame %d: find_ue() returns -1\n", Mod_id, phy_vars_eNB->proc[8].frame_tx );
    return;
  }


  if (physicalConfigDedicated) {
    phy_vars_eNB->physicalConfigDedicated[UE_id] = physicalConfigDedicated;
    LOG_D(PHY,"phy_config_dedicated_eNB: physicalConfigDedicated=%p\n",physicalConfigDedicated);

    if (physicalConfigDedicated->antennaInfo) {
      switch(physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode) {
      case AntennaInfoDedicated__transmissionMode_tm1:
	phy_vars_eNB->transmission_mode[UE_id] = 1;
	break;
      case AntennaInfoDedicated__transmissionMode_tm2:
	phy_vars_eNB->transmission_mode[UE_id] = 2;
	break;
      case AntennaInfoDedicated__transmissionMode_tm3:
	phy_vars_eNB->transmission_mode[UE_id] = 3;
	break;
      case AntennaInfoDedicated__transmissionMode_tm4:
	phy_vars_eNB->transmission_mode[UE_id] = 4;
	break;
      case AntennaInfoDedicated__transmissionMode_tm5:
	phy_vars_eNB->transmission_mode[UE_id] = 5;
	break;
      case AntennaInfoDedicated__transmissionMode_tm6:
	phy_vars_eNB->transmission_mode[UE_id] = 6;
	break;
      case AntennaInfoDedicated__transmissionMode_tm7:
	phy_vars_eNB->transmission_mode[UE_id] = 7;
	break;
      default:
	LOG_E(PHY,"Unknown transmission mode!\n");
	break;
      }
      LOG_I(PHY,"Transmission Mode (phy_config_dedicated_eNB) %d\n",phy_vars_eNB->transmission_mode[UE_id]);
 
    } else {
      LOG_D(PHY,"[eNB %d] Frame %d: Received NULL radioResourceConfigDedicated->antennaInfo from eNB %d\n",Mod_id, phy_vars_eNB->proc[8].frame_tx,UE_id);
    }

  } else {
    LOG_E(PHY,"[eNB %d] Frame %d: Received NULL radioResourceConfigDedicated from eNB %d\n",Mod_id, phy_vars_eNB->proc[8].frame_tx,UE_id);
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

  //PHY_VARS_eNB *phy_vars_eNB = PHY_vars_eNB_g[Mod_id][CC_id];
  uint8_t UE_id = find_ue(rnti,PHY_vars_eNB_g[Mod_id][0]);
  struct PhysicalConfigDedicatedSCell_r10 *physicalConfigDedicatedSCell_r10 = sCellToAddMod_r10->radioResourceConfigDedicatedSCell_r10->physicalConfigDedicatedSCell_r10;
  //struct RadioResourceConfigCommonSCell_r10 *physicalConfigCommonSCell_r10 = sCellToAddMod_r10->radioResourceConfigCommonSCell_r10;
  //PhysCellId_t physCellId_r10 = sCellToAddMod_r10->cellIdentification_r10->physCellId_r10;
  ARFCN_ValueEUTRA_t dl_CarrierFreq_r10 = sCellToAddMod_r10->cellIdentification_r10->dl_CarrierFreq_r10;
  uint32_t carrier_freq_local;

#ifdef EXMIMO
#ifdef DRIVER2013
  exmimo_config_t *p_exmimo_config = openair0_exmimo_pci[rf_map[CC_id].card].exmimo_config_ptr;
#endif
#endif

  if ((dl_CarrierFreq_r10>=36000) && (dl_CarrierFreq_r10<=36199)) {
    carrier_freq_local = 1900000000 + (dl_CarrierFreq_r10-36000)*100000; //band 33 from 3GPP 36.101 v 10.9 Table 5.7.3-1
    LOG_I(PHY,"[eNB %d] Frame %d: Configured SCell %d to frequency %d (ARFCN %d) for UE %d\n",Mod_id,/*phy_vars_eNB->frame*/0,CC_id,carrier_freq_local,dl_CarrierFreq_r10,UE_id);
#ifdef EXMIMO
#ifdef DRIVER2013
    //carrier_freq[CC_id] = carrier_freq_local;
    //openair_daq_vars.freq_offset = -6540;
    p_exmimo_config->rf.rf_freq_rx[rf_map[CC_id].chain] = carrier_freq_local;//+openair_daq_vars.freq_offset2;
    p_exmimo_config->rf.rf_freq_tx[rf_map[CC_id].chain] = carrier_freq_local;//+openair_daq_vars.freq_offset2;
    p_exmimo_config->rf.tx_gain[rf_map[CC_id].chain][0] = 25;
    p_exmimo_config->rf.rf_vcocal[rf_map[CC_id].chain] = 910;
    p_exmimo_config->rf.rf_local[rf_map[CC_id].chain] = 8255063; //this should be taken form calibration file
    p_exmimo_config->rf.rffe_band_mode[rf_map[CC_id].chain] = B19G_TDD;
#endif
#endif
  } else if ((dl_CarrierFreq_r10>=6150) && (dl_CarrierFreq_r10<=6449)) {
    carrier_freq_local = 832000000 + (dl_CarrierFreq_r10-6150)*100000; //band 20 from 3GPP 36.101 v 10.9 Table 5.7.3-1
    // this is actually for the UL only, but we use it for DL too, since there is no TDD mode for this band
    LOG_I(PHY,"[eNB %d] Frame %d: Configured SCell %d to frequency %d (ARFCN %d) for UE %d\n",Mod_id,/*phy_vars_eNB->frame*/0,CC_id,carrier_freq_local,dl_CarrierFreq_r10,UE_id);
#ifdef EXMIMO
#ifdef DRIVER2013
    //carrier_freq[CC_id] = carrier_freq_local;
    //openair_daq_vars.freq_offset = -2000;
    p_exmimo_config->rf.rf_freq_rx[rf_map[CC_id].chain] = carrier_freq_local;//+openair_daq_vars.freq_offset2;
    p_exmimo_config->rf.rf_freq_tx[rf_map[CC_id].chain] = carrier_freq_local;//+openair_daq_vars.freq_offset2;
    p_exmimo_config->rf.tx_gain[rf_map[CC_id].chain][0] = 10;
    p_exmimo_config->rf.rf_vcocal[rf_map[CC_id].chain] = 2015;
    p_exmimo_config->rf.rf_local[rf_map[CC_id].chain] =  8254992; //this should be taken form calibration file
    p_exmimo_config->rf.rffe_band_mode[rf_map[CC_id].chain] = DD_TDD;
#endif
#endif
  } else {
    LOG_E(PHY,"[eNB %d] Frame %d: ARFCN %d of SCell %d for UE %d not supported\n",Mod_id,/*phy_vars_eNB->frame*/0,dl_CarrierFreq_r10,CC_id,UE_id);
  }

  if (physicalConfigDedicatedSCell_r10) {
//#warning " phy_vars_eNB->physicalConfigDedicatedSCell_r10 does not exist in phy_vars_eNB"
    //  phy_vars_eNB->physicalConfigDedicatedSCell_r10[UE_id] = physicalConfigDedicatedSCell_r10;
    LOG_I(PHY,"[eNB %d] Frame %d: Configured phyConfigDedicatedSCell with CC_id %d for UE %d\n",Mod_id,/*phy_vars_eNB->frame*/0,CC_id,UE_id);
  } else {
    LOG_E(PHY,"[eNB %d] Frame %d: Received NULL radioResourceConfigDedicated (CC_id %d, UE %d)\n",Mod_id, /*phy_vars_eNB->frame*/0,CC_id,UE_id);
    return;
  }

}
#endif

void phy_config_dedicated_ue(uint8_t Mod_id,int CC_id,uint8_t CH_index,
                             struct PhysicalConfigDedicated *physicalConfigDedicated )
{

  PHY_VARS_UE *phy_vars_ue = PHY_vars_UE_g[Mod_id][CC_id];

  phy_vars_ue->total_TBS[CH_index]=0;
  phy_vars_ue->total_TBS_last[CH_index]=0;
  phy_vars_ue->bitrate[CH_index]=0;
  phy_vars_ue->total_received_bits[CH_index]=0;
  phy_vars_ue->dlsch_errors[CH_index]=0;
  phy_vars_ue->dlsch_errors_last[CH_index]=0;
  phy_vars_ue->dlsch_received[CH_index]=0;
  phy_vars_ue->dlsch_received_last[CH_index]=0;
  phy_vars_ue->dlsch_fer[CH_index]=0;



  if (physicalConfigDedicated) {
    LOG_D(PHY,"[UE %d] Frame %d: Received physicalConfigDedicated from eNB %d\n",Mod_id, phy_vars_ue->frame_rx,CH_index);
    LOG_D(PHY,"------------------------------------------------------------------------\n");

    if (physicalConfigDedicated->pdsch_ConfigDedicated) {
      phy_vars_ue->pdsch_config_dedicated[CH_index].p_a=physicalConfigDedicated->pdsch_ConfigDedicated->p_a;
      LOG_D(PHY,"pdsch_config_dedicated.p_a %d\n",phy_vars_ue->pdsch_config_dedicated[CH_index].p_a);
      LOG_D(PHY,"\n");
    }

    if (physicalConfigDedicated->pucch_ConfigDedicated) {
      if (physicalConfigDedicated->pucch_ConfigDedicated->ackNackRepetition.present==PUCCH_ConfigDedicated__ackNackRepetition_PR_release)
        phy_vars_ue->pucch_config_dedicated[CH_index].ackNackRepetition=0;
      else {
        phy_vars_ue->pucch_config_dedicated[CH_index].ackNackRepetition=1;
      }

      if (physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode)
        phy_vars_ue->pucch_config_dedicated[CH_index].tdd_AckNackFeedbackMode = *physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode;
      else
        phy_vars_ue->pucch_config_dedicated[CH_index].tdd_AckNackFeedbackMode = bundling;

      if ( phy_vars_ue->pucch_config_dedicated[CH_index].tdd_AckNackFeedbackMode == multiplexing)
        LOG_D(PHY,"pucch_config_dedicated.tdd_AckNackFeedbackMode = multiplexing\n");
      else
        LOG_D(PHY,"pucch_config_dedicated.tdd_AckNackFeedbackMode = bundling\n");
    }

    if (physicalConfigDedicated->pusch_ConfigDedicated) {
      phy_vars_ue->pusch_config_dedicated[CH_index].betaOffset_ACK_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_ACK_Index;
      phy_vars_ue->pusch_config_dedicated[CH_index].betaOffset_RI_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_RI_Index;
      phy_vars_ue->pusch_config_dedicated[CH_index].betaOffset_CQI_Index = physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_CQI_Index;


      LOG_D(PHY,"pusch_config_dedicated.betaOffset_ACK_Index %d\n",phy_vars_ue->pusch_config_dedicated[CH_index].betaOffset_ACK_Index);
      LOG_D(PHY,"pusch_config_dedicated.betaOffset_RI_Index %d\n",phy_vars_ue->pusch_config_dedicated[CH_index].betaOffset_RI_Index);
      LOG_D(PHY,"pusch_config_dedicated.betaOffset_CQI_Index %d\n",phy_vars_ue->pusch_config_dedicated[CH_index].betaOffset_CQI_Index);
      LOG_D(PHY,"\n");


    }

    if (physicalConfigDedicated->uplinkPowerControlDedicated) {

      phy_vars_ue->ul_power_control_dedicated[CH_index].p0_UE_PUSCH = physicalConfigDedicated->uplinkPowerControlDedicated->p0_UE_PUSCH;
      phy_vars_ue->ul_power_control_dedicated[CH_index].deltaMCS_Enabled= physicalConfigDedicated->uplinkPowerControlDedicated->deltaMCS_Enabled;
      phy_vars_ue->ul_power_control_dedicated[CH_index].accumulationEnabled= physicalConfigDedicated->uplinkPowerControlDedicated->accumulationEnabled;
      phy_vars_ue->ul_power_control_dedicated[CH_index].p0_UE_PUCCH= physicalConfigDedicated->uplinkPowerControlDedicated->p0_UE_PUCCH;
      phy_vars_ue->ul_power_control_dedicated[CH_index].pSRS_Offset= physicalConfigDedicated->uplinkPowerControlDedicated->pSRS_Offset;
      phy_vars_ue->ul_power_control_dedicated[CH_index].filterCoefficient= *physicalConfigDedicated->uplinkPowerControlDedicated->filterCoefficient;
      LOG_D(PHY,"ul_power_control_dedicated.p0_UE_PUSCH %d\n",phy_vars_ue->ul_power_control_dedicated[CH_index].p0_UE_PUSCH);
      LOG_D(PHY,"ul_power_control_dedicated.deltaMCS_Enabled %d\n",phy_vars_ue->ul_power_control_dedicated[CH_index].deltaMCS_Enabled);
      LOG_D(PHY,"ul_power_control_dedicated.accumulationEnabled %d\n",phy_vars_ue->ul_power_control_dedicated[CH_index].accumulationEnabled);
      LOG_D(PHY,"ul_power_control_dedicated.p0_UE_PUCCH %d\n",phy_vars_ue->ul_power_control_dedicated[CH_index].p0_UE_PUCCH);
      LOG_D(PHY,"ul_power_control_dedicated.pSRS_Offset %d\n",phy_vars_ue->ul_power_control_dedicated[CH_index].pSRS_Offset);
      LOG_D(PHY,"ul_power_control_dedicated.filterCoefficient %d\n",phy_vars_ue->ul_power_control_dedicated[CH_index].filterCoefficient);
      LOG_D(PHY,"\n");
    }

    if (physicalConfigDedicated->antennaInfo) {
      phy_vars_ue->transmission_mode[CH_index] = 1+(physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode);
      LOG_D(PHY,"Transmission Mode %d\n",phy_vars_ue->transmission_mode[CH_index]);
      LOG_D(PHY,"\n");
      switch(physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode) {
      case AntennaInfoDedicated__transmissionMode_tm1:
	phy_vars_ue->transmission_mode[CH_index] = 1;
	break;
      case AntennaInfoDedicated__transmissionMode_tm2:
	phy_vars_ue->transmission_mode[CH_index] = 2;
	break;
      case AntennaInfoDedicated__transmissionMode_tm3:
	phy_vars_ue->transmission_mode[CH_index] = 3;
	break;
      case AntennaInfoDedicated__transmissionMode_tm4:
	phy_vars_ue->transmission_mode[CH_index] = 4;
	break;
      case AntennaInfoDedicated__transmissionMode_tm5:
	phy_vars_ue->transmission_mode[CH_index] = 5;
	break;
      case AntennaInfoDedicated__transmissionMode_tm6:
	phy_vars_ue->transmission_mode[CH_index] = 6;
	break;
      case AntennaInfoDedicated__transmissionMode_tm7:
	phy_vars_ue->transmission_mode[CH_index] = 7;
	break;
      default:
	LOG_E(PHY,"Unknown transmission mode!\n");
	break;
      } 
    } else {
      LOG_D(PHY,"[UE %d] Frame %d: Received NULL physicalConfigDedicated->antennaInfo from eNB %d\n",Mod_id, phy_vars_ue->frame_rx,CH_index);
    }

    if (physicalConfigDedicated->schedulingRequestConfig) {
      if (physicalConfigDedicated->schedulingRequestConfig->present == SchedulingRequestConfig_PR_setup) {
        phy_vars_ue->scheduling_request_config[CH_index].sr_PUCCH_ResourceIndex = physicalConfigDedicated->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex;
        phy_vars_ue->scheduling_request_config[CH_index].sr_ConfigIndex=physicalConfigDedicated->schedulingRequestConfig->choice.setup.sr_ConfigIndex;
        phy_vars_ue->scheduling_request_config[CH_index].dsr_TransMax=physicalConfigDedicated->schedulingRequestConfig->choice.setup.dsr_TransMax;

        LOG_D(PHY,"scheduling_request_config.sr_PUCCH_ResourceIndex %d\n",phy_vars_ue->scheduling_request_config[CH_index].sr_PUCCH_ResourceIndex);
        LOG_D(PHY,"scheduling_request_config.sr_ConfigIndex %d\n",phy_vars_ue->scheduling_request_config[CH_index].sr_ConfigIndex);
        LOG_D(PHY,"scheduling_request_config.dsr_TransMax %d\n",phy_vars_ue->scheduling_request_config[CH_index].dsr_TransMax);
      }

      LOG_D(PHY,"------------------------------------------------------------\n");

    }

#ifdef CBA

    if (physicalConfigDedicated->pusch_CBAConfigDedicated_vlola) {
      phy_vars_ue->pusch_ca_config_dedicated[CH_index].betaOffset_CA_Index = (uint16_t) *physicalConfigDedicated->pusch_CBAConfigDedicated_vlola->betaOffset_CBA_Index;
      phy_vars_ue->pusch_ca_config_dedicated[CH_index].cShift = (uint16_t) *physicalConfigDedicated->pusch_CBAConfigDedicated_vlola->cShift_CBA;
      LOG_D(PHY,"[UE %d ] physicalConfigDedicated pusch CBA config dedicated: beta offset %d cshift %d \n",Mod_id,
            phy_vars_ue->pusch_ca_config_dedicated[CH_index].betaOffset_CA_Index,
            phy_vars_ue->pusch_ca_config_dedicated[CH_index].cShift);
    }

#endif
  } else {
    LOG_D(PHY,"[PHY][UE %d] Frame %d: Received NULL radioResourceConfigDedicated from eNB %d\n",Mod_id, phy_vars_ue->frame_rx,CH_index);
    return;
  }

}

void  phy_config_cba_rnti (module_id_t Mod_id,int CC_id,eNB_flag_t eNB_flag, uint8_t index, rnti_t cba_rnti, uint8_t cba_group_id, uint8_t num_active_cba_groups)
{
  //   uint8_t i;

  if (eNB_flag == 0 ) {
    //LOG_D(PHY,"[UE %d] configure cba group %d with rnti %x, num active cba grp %d\n", index, index, cba_rnti, num_active_cba_groups);
    PHY_vars_UE_g[Mod_id][CC_id]->ulsch_ue[index]->num_active_cba_groups=num_active_cba_groups;
    PHY_vars_UE_g[Mod_id][CC_id]->ulsch_ue[index]->cba_rnti[cba_group_id]=cba_rnti;
  } else {
    //for (i=index; i < NUMBER_OF_UE_MAX; i+=num_active_cba_groups){
    //  LOG_D(PHY,"[eNB %d] configure cba group %d with rnti %x for UE %d, num active cba grp %d\n",Mod_id, i%num_active_cba_groups, cba_rnti, i, num_active_cba_groups);
    PHY_vars_eNB_g[Mod_id][CC_id]->ulsch_eNB[index]->num_active_cba_groups=num_active_cba_groups;
    PHY_vars_eNB_g[Mod_id][CC_id]->ulsch_eNB[index]->cba_rnti[cba_group_id] = cba_rnti;
    //}
  }
}

void phy_init_lte_top(LTE_DL_FRAME_PARMS *lte_frame_parms)
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

  lte_sync_time_init(lte_frame_parms);

  generate_ul_ref_sigs();
  generate_ul_ref_sigs_rx();

  generate_64qam_table();
  generate_16qam_table();
  generate_RIV_tables();


  //set_taus_seed(1328);

}

/*! \brief Helper function to allocate memory for DLSCH data structures.
 * \param[out] pdsch Pointer to the LTE_UE_PDSCH structure to initialize.
 * \param[in] frame_parms LTE_DL_FRAME_PARMS structure.
 * \note This function is optimistic in that it expects malloc() to succeed.
 */
void phy_init_lte_ue__PDSCH( LTE_UE_PDSCH* const pdsch, const LTE_DL_FRAME_PARMS* const frame_parms )
{
  AssertFatal( pdsch, "pdsch==0" );

  pdsch->pmi_ext = (uint8_t*)malloc16_clear( frame_parms->N_RB_DL );
  pdsch->llr[0] = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
  pdsch->llr128 = (int16_t**)malloc16_clear( sizeof(int16_t*) );
  pdsch->llr128_2ndstream = (int16_t**)malloc16_clear( sizeof(int16_t*) );
  // FIXME! no further allocation for (int16_t*)pdsch->llr128 !!! expect SIGSEGV

  pdsch->rxdataF_ext         = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->rxdataF_comp0       = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->rho                 = (int32_t**)malloc16_clear( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
  pdsch->dl_ch_estimates_ext = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_rho_ext       = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_rho2_ext       = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_mag0          = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_magb0         = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );

  // the allocated memory size is fixed:
  AssertFatal( frame_parms->nb_antennas_rx <= 2, "nb_antennas_rx > 2" );

  for (int i=0; i<frame_parms->nb_antennas_rx; i++) {
    pdsch->rho[i]     = (int32_t*)malloc16_clear( sizeof(int32_t)*(frame_parms->N_RB_DL*12*7*2) );

    for (int j=0; j<4; j++) { //frame_parms->nb_antennas_tx; j++)
      const int idx = (j<<1)+i;
      const size_t num = 7*2*frame_parms->N_RB_DL*12;
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

/*! \brief Helper function to allocate memory for DLSCH_FLP data structures.
 * \param[out] pdsch_flp Pointer to the LTE_UE_PDSCH_FLP structure to initialize.
 * \param[in] frame_parms LTE_DL_FRAME_PARMS structure.
 * \note This function is optimistic in that it expects malloc() to succeed.
 */
void phy_init_lte_ue__PDSCH_FLP( LTE_UE_PDSCH_FLP* const pdsch_flp, const LTE_DL_FRAME_PARMS* const frame_parms )
{
  AssertFatal( pdsch_flp, "pdsch==0" );

  pdsch_flp->llr[0] = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
  pdsch_flp->llr[1] = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
  pdsch_flp->llr128 = (int16_t**)malloc16_clear( sizeof(int16_t*) );
  // FIXME! no further allocation for (int16_t*)pdsch_flp->llr128 !!! expect SIGSEGV

  pdsch_flp->pmi_ext             = (uint8_t*)malloc16_clear( frame_parms->N_RB_DL );
  pdsch_flp->rxdataF_ext         = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch_flp->dl_ch_estimates_ext = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch_flp->rxdataF_comp        = (double**)malloc16_clear( 8*sizeof(double*) );
  pdsch_flp->dl_ch_rho_ext       = (double**)malloc16_clear( 8*sizeof(double*) );
  pdsch_flp->dl_ch_mag           = (double**)malloc16_clear( 8*sizeof(double*) );
  pdsch_flp->dl_ch_magb          = (double**)malloc16_clear( 8*sizeof(double*) );
  pdsch_flp->rho                 = (double**)malloc16_clear( frame_parms->nb_antennas_rx*sizeof(double*) );

  // the allocated memory size is fixed:
  AssertFatal( frame_parms->nb_antennas_rx <= 2, "nb_antennas_rx > 2" );

  for (int i=0; i<frame_parms->nb_antennas_rx; i++) {
    pdsch_flp->rho[i] = (double*)malloc16_clear( sizeof(double)*(frame_parms->N_RB_DL*12*7*2) );

    for (int j=0; j<4; j++) { //frame_parms->nb_antennas_tx; j++)
      const int idx = (j<<1)+i;
      const size_t num = 7*2*frame_parms->N_RB_DL*12;
      pdsch_flp->rxdataF_ext[idx]         = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch_flp->dl_ch_estimates_ext[idx] = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch_flp->rxdataF_comp[idx]        = (double*)malloc16_clear( sizeof(double) * num );
      pdsch_flp->dl_ch_rho_ext[idx]       = (double*)malloc16_clear( sizeof(double) * num );
      pdsch_flp->dl_ch_mag[idx]           = (double*)malloc16_clear( sizeof(double) * num );
      pdsch_flp->dl_ch_magb[idx]          = (double*)malloc16_clear( sizeof(double) * num );
    }
  }
}

int phy_init_lte_ue(PHY_VARS_UE *phy_vars_ue,
                    int nb_connected_eNB,
                    uint8_t abstraction_flag)
{

  // create shortcuts
  LTE_DL_FRAME_PARMS* const frame_parms      = &phy_vars_ue->lte_frame_parms;
  LTE_UE_COMMON* const ue_common_vars        = &phy_vars_ue->lte_ue_common_vars;
  LTE_UE_PDSCH** const ue_pdsch_vars         = phy_vars_ue->lte_ue_pdsch_vars;
  LTE_UE_PDSCH_FLP** const ue_pdsch_vars_flp = phy_vars_ue->lte_ue_pdsch_vars_flp;
  LTE_UE_PDSCH** const ue_pdsch_vars_SI      = phy_vars_ue->lte_ue_pdsch_vars_SI;
  LTE_UE_PDSCH** const ue_pdsch_vars_ra      = phy_vars_ue->lte_ue_pdsch_vars_ra;
  LTE_UE_PDSCH** const ue_pdsch_vars_mch     = phy_vars_ue->lte_ue_pdsch_vars_MCH;
  LTE_UE_PBCH** const ue_pbch_vars           = phy_vars_ue->lte_ue_pbch_vars;
  LTE_UE_PDCCH** const ue_pdcch_vars         = phy_vars_ue->lte_ue_pdcch_vars;
  LTE_UE_PRACH** const ue_prach_vars         = phy_vars_ue->lte_ue_prach_vars;

  int i,j,k;
  int eNB_id;

  msg("Initializing UE vars (abstraction %"PRIu8") for eNB TXant %"PRIu8", UE RXant %"PRIu8"\n",abstraction_flag,frame_parms->nb_antennas_tx,frame_parms->nb_antennas_rx);
  LOG_D(PHY,"[MSC_NEW][FRAME 00000][PHY_UE][MOD %02u][]\n", phy_vars_ue->Mod_id+NB_eNB_INST);

  // many memory allocation sizes are hard coded
  AssertFatal( frame_parms->nb_antennas_rx <= 2, "hard coded allocation for ue_common_vars->dl_ch_estimates[eNB_id]" );
  AssertFatal( phy_vars_ue->n_connected_eNB <= NUMBER_OF_CONNECTED_eNB_MAX, "n_connected_eNB is too large" );
#ifndef USER_MODE
  AssertFatal( frame_parms->nb_antennas_tx <= NB_ANTENNAS_TX, "nb_antennas_tx too large" );
  AssertFatal( frame_parms->nb_antennas_rx <= NB_ANTENNAS_RX, "nb_antennas_rx too large" );
#endif
  // init phy_vars_ue

  for (i=0; i<4; i++) {
    phy_vars_ue->rx_gain_max[i] = 135;
    phy_vars_ue->rx_gain_med[i] = 128;
    phy_vars_ue->rx_gain_byp[i] = 120;
  }

  phy_vars_ue->n_connected_eNB = nb_connected_eNB;

  for(eNB_id = 0; eNB_id < phy_vars_ue->n_connected_eNB; eNB_id++) {
    phy_vars_ue->total_TBS[eNB_id] = 0;
    phy_vars_ue->total_TBS_last[eNB_id] = 0;
    phy_vars_ue->bitrate[eNB_id] = 0;
    phy_vars_ue->total_received_bits[eNB_id] = 0;
  }

  phy_vars_ue->tx_power_dBm=-127;

  if (abstraction_flag == 0) {

    // init TX buffers

    ue_common_vars->txdata  = (int32_t**)malloc16( frame_parms->nb_antennas_tx*sizeof(int32_t*) );
    ue_common_vars->txdataF = (int32_t **)malloc16( frame_parms->nb_antennas_tx*sizeof(int32_t*) );

    for (i=0; i<frame_parms->nb_antennas_tx; i++) {
#ifdef USER_MODE
      ue_common_vars->txdata[i]  = (int32_t*)malloc16_clear( FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(int32_t) );
#else //USER_MODE
      ue_common_vars->txdata[i]  = TX_DMA_BUFFER[0][i];
#endif //USER_MODE
      ue_common_vars->txdataF[i] = (int32_t *)malloc16_clear( FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(int32_t) );
    }

    // init RX buffers

    ue_common_vars->rxdata   = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
    ue_common_vars->rxdataF  = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
    ue_common_vars->rxdataF2 = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );

    for (i=0; i<frame_parms->nb_antennas_rx; i++) {
#ifndef USER_MODE
      ue_common_vars->rxdata[i] = (int32_t*) RX_DMA_BUFFER[0][i];
#else //USER_MODE
      ue_common_vars->rxdata[i] = (int32_t*) malloc16_clear( (FRAME_LENGTH_COMPLEX_SAMPLES+2048)*sizeof(int32_t) );
#endif //USER_MODE
      // RK 2 times because of output format of FFT!
      // FIXME We should get rid of this
      ue_common_vars->rxdataF[i] = (int32_t*)malloc16_clear( 2*sizeof(int32_t)*(frame_parms->ofdm_symbol_size*14) );
      // RK 2 times because of output format of FFT!  We should get rid of this
      // FIXME We should get rid of this
      ue_common_vars->rxdataF2[i] = (int32_t*)malloc16_clear( 2*sizeof(int32_t)*(frame_parms->ofdm_symbol_size*frame_parms->symbols_per_tti*10) );
    }
  }

  // Channel estimates
  for (eNB_id=0; eNB_id<7; eNB_id++) {
    ue_common_vars->dl_ch_estimates[eNB_id]      = (int32_t**)malloc16_clear(8*sizeof(int32_t*));
    ue_common_vars->dl_ch_estimates_time[eNB_id] = (int32_t**)malloc16_clear(8*sizeof(int32_t*));

    for (i=0; i<frame_parms->nb_antennas_rx; i++)
      for (j=0; j<4; j++) {
        int idx = (j<<1) + i;
        ue_common_vars->dl_ch_estimates[eNB_id][idx] = (int32_t*)malloc16_clear( sizeof(int32_t)*frame_parms->symbols_per_tti*(frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH) );
        ue_common_vars->dl_ch_estimates_time[eNB_id][idx] = (int32_t*)malloc16_clear( sizeof(int32_t)*frame_parms->ofdm_symbol_size*2 );
      }
  }

  // DLSCH
  for (eNB_id=0; eNB_id<phy_vars_ue->n_connected_eNB; eNB_id++) {
    ue_pdsch_vars[eNB_id]     = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
#ifdef ENABLE_FULL_FLP
    ue_pdsch_vars_flp[eNB_id] = (LTE_UE_PDSCH_FLP *)malloc16_clear(sizeof(LTE_UE_PDSCH_FLP));
#else
    ue_pdsch_vars_flp[eNB_id] = 0;
#endif
    ue_pdsch_vars_SI[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    ue_pdsch_vars_ra[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    ue_pdsch_vars_mch[eNB_id] = (LTE_UE_PDSCH *)malloc16_clear(sizeof(LTE_UE_PDSCH));
    ue_pdcch_vars[eNB_id]     = (LTE_UE_PDCCH *)malloc16_clear(sizeof(LTE_UE_PDCCH));
    ue_prach_vars[eNB_id]     = (LTE_UE_PRACH *)malloc16_clear(sizeof(LTE_UE_PRACH));
    ue_pbch_vars[eNB_id]      = (LTE_UE_PBCH *)malloc16_clear(sizeof(LTE_UE_PBCH));

    if (abstraction_flag == 0) {
      phy_init_lte_ue__PDSCH( ue_pdsch_vars[eNB_id], frame_parms );

      ue_pdsch_vars[eNB_id]->llr_shifts   = (uint8_t*)malloc16_clear(7*2*frame_parms->N_RB_DL*12);
      ue_pdsch_vars[eNB_id]->llr_shifts_p = ue_pdsch_vars[eNB_id]->llr_shifts;
      ue_pdsch_vars[eNB_id]->dl_ch_mag1   = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      ue_pdsch_vars[eNB_id]->dl_ch_magb1  = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      ue_pdsch_vars[eNB_id]->llr[1]       = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );

      for (k=0; k<8; k++)
        ue_pdsch_vars[eNB_id]->rxdataF_comp1[k] = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );

      for (i=0; i<frame_parms->nb_antennas_rx; i++)
        for (j=0; j<4; j++) {
          int idx = (j<<1)+i;
          ue_pdsch_vars[eNB_id]->dl_ch_mag1[idx]  = (int32_t*)malloc16_clear( 7*2*sizeof(int32_t)*(frame_parms->N_RB_DL*12) );
          ue_pdsch_vars[eNB_id]->dl_ch_magb1[idx] = (int32_t*)malloc16_clear( 7*2*sizeof(int32_t)*(frame_parms->N_RB_DL*12) );

          for (k=0; k<8; k++)
            ue_pdsch_vars[eNB_id]->rxdataF_comp1[idx][k] = (int32_t*)malloc16_clear( sizeof(int32_t)*(frame_parms->N_RB_DL*12*14) );
        }

#ifdef ENABLE_FULL_FLP
      phy_init_lte_ue__PDSCH_FLP( ue_pdsch_vars_flp[eNB_id], frame_parms );
#endif

      phy_init_lte_ue__PDSCH( ue_pdsch_vars_SI[eNB_id], frame_parms );
      phy_init_lte_ue__PDSCH( ue_pdsch_vars_ra[eNB_id], frame_parms );
      phy_init_lte_ue__PDSCH( ue_pdsch_vars_mch[eNB_id], frame_parms );
      // 100 PRBs * 12 REs/PRB * 4 PDCCH SYMBOLS * 2 LLRs/RE
      ue_pdcch_vars[eNB_id]->llr   = (uint16_t*)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      ue_pdcch_vars[eNB_id]->llr16 = (uint16_t*)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      ue_pdcch_vars[eNB_id]->wbar  = (uint16_t*)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
      ue_pdcch_vars[eNB_id]->e_rx  = (int8_t*)malloc16_clear( 4*2*100*12 );

      ue_pdcch_vars[eNB_id]->rxdataF_comp        = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      ue_pdcch_vars[eNB_id]->dl_ch_rho_ext       = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      ue_pdcch_vars[eNB_id]->rho                 = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
      ue_pdcch_vars[eNB_id]->rxdataF_ext         = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      ue_pdcch_vars[eNB_id]->dl_ch_estimates_ext = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );

      for (i=0; i<frame_parms->nb_antennas_rx; i++) {
        //ue_pdcch_vars[eNB_id]->rho[i] = (int32_t*)malloc16_clear( sizeof(int32_t)*(frame_parms->N_RB_DL*12*7*2) );
        ue_pdcch_vars[eNB_id]->rho[i] = (int32_t*)malloc16_clear( sizeof(int32_t)*(100*12*4) );

        for (j=0; j<4; j++) { //frame_parms->nb_antennas_tx; j++)
          int idx = (j<<1)+i;
          //  size_t num = 7*2*frame_parms->N_RB_DL*12;
          size_t num = 4*100*12;  // 4 symbols, 100 PRBs, 12 REs per PRB
          ue_pdcch_vars[eNB_id]->rxdataF_comp[idx]        = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
          ue_pdcch_vars[eNB_id]->dl_ch_rho_ext[idx]       = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
          ue_pdcch_vars[eNB_id]->rxdataF_ext[idx]         = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
          ue_pdcch_vars[eNB_id]->dl_ch_estimates_ext[idx] = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
        }
      }

      // PBCH
      ue_pbch_vars[eNB_id]->rxdataF_ext         = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
      ue_pbch_vars[eNB_id]->rxdataF_comp        = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      ue_pbch_vars[eNB_id]->dl_ch_estimates_ext = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
      ue_pbch_vars[eNB_id]->llr                 = (int8_t*)malloc16_clear( 1920 );
      ue_prach_vars[eNB_id]->prachF             = (int16_t*)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(frame_parms->ofdm_symbol_size*12)) );
      ue_prach_vars[eNB_id]->prach              = (int16_t*)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(frame_parms->ofdm_symbol_size*12)) );

      for (i=0; i<frame_parms->nb_antennas_rx; i++) {
        ue_pbch_vars[eNB_id]->rxdataF_ext[i]    = (int32_t*)malloc16_clear( sizeof(int32_t)*6*12*4 );

        for (j=0; j<4; j++) {//frame_parms->nb_antennas_tx;j++) {
          int idx = (j<<1)+i;
          ue_pbch_vars[eNB_id]->rxdataF_comp[idx]        = (int32_t*)malloc16_clear( sizeof(int32_t)*6*12*4 );
          ue_pbch_vars[eNB_id]->dl_ch_estimates_ext[idx] = (int32_t*)malloc16_clear( sizeof(int32_t)*6*12*4 );
        }
      }
    }

    ue_pbch_vars[eNB_id]->decoded_output = (uint8_t*)malloc16_clear( 64 );
  }

  // initialization for the last instance of ue_pdsch_vars (used for MU-MIMO)

  ue_pdsch_vars[eNB_id]     = (LTE_UE_PDSCH *)malloc16_clear( sizeof(LTE_UE_PDSCH) );
  ue_pdsch_vars_SI[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear( sizeof(LTE_UE_PDSCH) );
  ue_pdsch_vars_ra[eNB_id]  = (LTE_UE_PDSCH *)malloc16_clear( sizeof(LTE_UE_PDSCH) );
  ue_pdsch_vars_flp[eNB_id] = (LTE_UE_PDSCH_FLP *)malloc16_clear( sizeof(LTE_UE_PDSCH_FLP) );

  if (abstraction_flag == 0) {
    phy_init_lte_ue__PDSCH( ue_pdsch_vars[eNB_id], frame_parms );
    ue_pdsch_vars[eNB_id]->llr[1] = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );

    phy_init_lte_ue__PDSCH_FLP( ue_pdsch_vars_flp[eNB_id], frame_parms );
  } else { //abstraction == 1
    phy_vars_ue->sinr_dB = (double*) malloc16_clear( frame_parms->N_RB_DL*12*sizeof(double) );
  }

  phy_vars_ue->sinr_CQI_dB = (double*) malloc16_clear( frame_parms->N_RB_DL*12*sizeof(double) );

  phy_vars_ue->init_averaging = 1;
  phy_vars_ue->pdsch_config_dedicated->p_a = dB0; // default value until overwritten by RRCConnectionReconfiguration

  // set channel estimation to do linear interpolation in time
  phy_vars_ue->high_speed_flag = 1;
  phy_vars_ue->ch_est_alpha    = 24576;

  init_prach_tables(839);

  return 0;
}

int phy_init_lte_eNB(PHY_VARS_eNB *phy_vars_eNB,
                     unsigned char is_secondary_eNB,
                     uint8_t cooperation_flag,
                     unsigned char abstraction_flag)
{

  // shortcuts
  LTE_DL_FRAME_PARMS* const frame_parms = &phy_vars_eNB->lte_frame_parms;
  LTE_eNB_COMMON* const eNB_common_vars = &phy_vars_eNB->lte_eNB_common_vars;
  LTE_eNB_PUSCH** const eNB_pusch_vars  = phy_vars_eNB->lte_eNB_pusch_vars;
  LTE_eNB_SRS* const eNB_srs_vars       = phy_vars_eNB->lte_eNB_srs_vars;
  LTE_eNB_PRACH* const eNB_prach_vars   = &phy_vars_eNB->lte_eNB_prach_vars;
  int i, j, eNB_id, UE_id; 

  phy_vars_eNB->total_dlsch_bitrate = 0;
  phy_vars_eNB->total_transmitted_bits = 0;
  phy_vars_eNB->total_system_throughput = 0;
  phy_vars_eNB->check_for_MUMIMO_transmissions=0;

  LOG_I(PHY,"[eNB %"PRIu8"] Initializing DL_FRAME_PARMS : N_RB_DL %"PRIu8", PHICH Resource %d, PHICH Duration %d\n",
        phy_vars_eNB->Mod_id,
        frame_parms->N_RB_DL,frame_parms->phich_config_common.phich_resource,
        frame_parms->phich_config_common.phich_duration);
  LOG_D(PHY,"[MSC_NEW][FRAME 00000][PHY_eNB][MOD %02"PRIu8"][]\n", phy_vars_eNB->Mod_id);

  lte_gold(frame_parms,phy_vars_eNB->lte_gold_table,frame_parms->Nid_cell);
  generate_pcfich_reg_mapping(frame_parms);
  generate_phich_reg_mapping(frame_parms);

  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    phy_vars_eNB->first_run_timing_advance[UE_id] =
      1; ///This flag used to be static. With multiple eNBs this does no longer work, hence we put it in the structure. However it has to be initialized with 1, which is performed here.

    // clear whole structure
    bzero( &phy_vars_eNB->eNB_UE_stats[UE_id], sizeof(LTE_eNB_UE_stats) );

    phy_vars_eNB->physicalConfigDedicated[UE_id] = NULL;
  }

  phy_vars_eNB->first_run_I0_measurements =
    1; ///This flag used to be static. With multiple eNBs this does no longer work, hence we put it in the structure. However it has to be initialized with 1, which is performed here.

#ifndef USER_MODE
  AssertFatal( frame_parms->nb_antennas_tx <= NB_ANTENNAS_TX, "nb_antennas_tx too large" );
  AssertFatal( frame_parms->nb_antennas_rx <= NB_ANTENNAS_RX, "nb_antennas_rx too large" );
#endif

  for (eNB_id=0; eNB_id<3; eNB_id++) {

    if (abstraction_flag==0) {

      // TX vars
      eNB_common_vars->txdata[eNB_id]  = (int32_t**)malloc16( frame_parms->nb_antennas_tx*sizeof(int32_t*) );
      eNB_common_vars->txdataF[eNB_id] = (int32_t **)malloc16( frame_parms->nb_antennas_tx*sizeof(int32_t*) );

      for (i=0; i<frame_parms->nb_antennas_tx; i++) {
#ifdef USER_MODE
        eNB_common_vars->txdata[eNB_id][i]  = (int32_t*)malloc16_clear( FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(int32_t) );
        eNB_common_vars->txdataF[eNB_id][i] = (int32_t*)malloc16_clear( FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(int32_t) );
#else // USER_MODE
        eNB_common_vars->txdata[eNB_id][i]  = TX_DMA_BUFFER[eNB_id][i];
        eNB_common_vars->txdataF[eNB_id][i] = (int32_t *)malloc16_clear( FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(int32_t) );
#endif //USER_MODE
#ifdef DEBUG_PHY
        msg("[openair][LTE_PHY][INIT] lte_eNB_common_vars->txdata[%d][%d] = %p\n",eNB_id,i,eNB_common_vars->txdata[eNB_id][i]);
        msg("[openair][LTE_PHY][INIT] lte_eNB_common_vars->txdataF[%d][%d] = %p (%d bytes)\n",
            eNB_id,i,eNB_common_vars->txdataF[eNB_id][i],
            FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(int32_t));
#endif
      }

      // RX vars
      eNB_common_vars->rxdata[eNB_id]        = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
      eNB_common_vars->rxdata_7_5kHz[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
      eNB_common_vars->rxdataF[eNB_id]       = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );

      for (i=0; i<frame_parms->nb_antennas_rx; i++) {
#ifndef USER_MODE
        eNB_common_vars->rxdata[eNB_id][i] = (int32_t*)RX_DMA_BUFFER[eNB_id][i];
#else //USER_MODE
        eNB_common_vars->rxdata[eNB_id][i] = (int32_t*)malloc16_clear( FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(int32_t) );
#endif //USER_MODE
        eNB_common_vars->rxdata_7_5kHz[eNB_id][i] = (int32_t*)malloc16_clear( frame_parms->samples_per_tti*sizeof(int32_t) );
        // RK 2 times because of output format of FFT!
        // FIXME We should get rid of this
        eNB_common_vars->rxdataF[eNB_id][i] = (int32_t*)malloc16_clear( 2*sizeof(int32_t)*(frame_parms->ofdm_symbol_size*frame_parms->symbols_per_tti) );
#ifdef DEBUG_PHY
        msg("[openair][LTE_PHY][INIT] lte_eNB_common_vars->rxdata[%d][%d] = %p\n",eNB_id,i,eNB_common_vars->rxdata[eNB_id][i]);
        msg("[openair][LTE_PHY][INIT] lte_eNB_common_vars->rxdata_7_5kHz[%d][%d] = %p\n",eNB_id,i,eNB_common_vars->rxdata_7_5kHz[eNB_id][i]);
#endif
      }

      // Channel estimates for SRS
      for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {

        eNB_srs_vars[UE_id].srs_ch_estimates[eNB_id]      = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
        eNB_srs_vars[UE_id].srs_ch_estimates_time[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );

        for (i=0; i<frame_parms->nb_antennas_rx; i++) {
          eNB_srs_vars[UE_id].srs_ch_estimates[eNB_id][i]      = (int32_t*)malloc16_clear( sizeof(int32_t)*frame_parms->ofdm_symbol_size );
          eNB_srs_vars[UE_id].srs_ch_estimates_time[eNB_id][i] = (int32_t*)malloc16_clear( sizeof(int32_t)*frame_parms->ofdm_symbol_size*2 );
        }
      } //UE_id

      eNB_common_vars->sync_corr[eNB_id] = (uint32_t*)malloc16_clear( LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*sizeof(uint32_t)*frame_parms->samples_per_tti );
    } else { //UPLINK abstraction = 1
      phy_vars_eNB->sinr_dB = (double*) malloc16_clear( frame_parms->N_RB_DL*12*sizeof(double) );
    }
  } //eNB_id

#ifndef NO_UL_REF

  if (abstraction_flag==0) {
    generate_ul_ref_sigs_rx();

    // SRS
    for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
      eNB_srs_vars[UE_id].srs = (int32_t*)malloc16_clear(2*frame_parms->ofdm_symbol_size*sizeof(int32_t));
    }
  }

#endif

  // ULSCH VARS

  eNB_prach_vars->prachF = (int16_t*)malloc16_clear( 2*1024 /*FIXME what is the correct number?*/ *sizeof(int16_t) );

  /* number of elements of an array X is computed as sizeof(X) / sizeof(X[0]) */
  AssertFatal(frame_parms->nb_antennas_rx <= sizeof(eNB_prach_vars->rxsigF) / sizeof(eNB_prach_vars->rxsigF[0]),
              "nb_antennas_rx too large");
  for (i=0; i<frame_parms->nb_antennas_rx; i++) {
    eNB_prach_vars->rxsigF[i] = (int16_t*)malloc16_clear( frame_parms->ofdm_symbol_size*12*2*sizeof(int16_t) );
#ifdef DEBUG_PHY
    msg("[openair][LTE_PHY][INIT] prach_vars->rxsigF[%d] = %p\n",i,eNB_prach_vars->rxsigF[i]);
#endif
  }

  AssertFatal(frame_parms->nb_antennas_rx <= sizeof(eNB_prach_vars->prach_ifft) / sizeof(eNB_prach_vars->prach_ifft[0]),
              "nb_antennas_rx too large");
  for (i=0; i<frame_parms->nb_antennas_rx; i++) {
    eNB_prach_vars->prach_ifft[i] = (int16_t*)malloc16_clear(1024*2*sizeof(int16_t));
#ifdef DEBUG_PHY
    msg("[openair][LTE_PHY][INIT] prach_vars->prach_ifft[%d] = %p\n",i,eNB_prach_vars->prach_ifft[i]);
#endif
  }

  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {

    //FIXME
    eNB_pusch_vars[UE_id] = (LTE_eNB_PUSCH*)malloc16_clear( NUMBER_OF_UE_MAX*sizeof(LTE_eNB_PUSCH) );

    if (abstraction_flag==0) {
      for (eNB_id=0; eNB_id<3; eNB_id++) {

        eNB_pusch_vars[UE_id]->rxdataF_ext[eNB_id]      = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
        eNB_pusch_vars[UE_id]->rxdataF_ext2[eNB_id]     = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
        eNB_pusch_vars[UE_id]->drs_ch_estimates[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
        eNB_pusch_vars[UE_id]->drs_ch_estimates_time[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
        eNB_pusch_vars[UE_id]->rxdataF_comp[eNB_id]     = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
        eNB_pusch_vars[UE_id]->ul_ch_mag[eNB_id]  = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
        eNB_pusch_vars[UE_id]->ul_ch_magb[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );

        for (i=0; i<frame_parms->nb_antennas_rx; i++) {
          // RK 2 times because of output format of FFT!
          // FIXME We should get rid of this
          eNB_pusch_vars[UE_id]->rxdataF_ext[eNB_id][i]      = (int32_t*)malloc16_clear( 2*sizeof(int32_t)*frame_parms->N_RB_UL*12*frame_parms->symbols_per_tti );
          eNB_pusch_vars[UE_id]->rxdataF_ext2[eNB_id][i]     = (int32_t*)malloc16_clear( sizeof(int32_t)*frame_parms->N_RB_UL*12*frame_parms->symbols_per_tti );
          eNB_pusch_vars[UE_id]->drs_ch_estimates[eNB_id][i] = (int32_t*)malloc16_clear( sizeof(int32_t)*frame_parms->N_RB_UL*12*frame_parms->symbols_per_tti );
          eNB_pusch_vars[UE_id]->drs_ch_estimates_time[eNB_id][i] = (int32_t*)malloc16_clear( 2*2*sizeof(int32_t)*frame_parms->ofdm_symbol_size );
          eNB_pusch_vars[UE_id]->rxdataF_comp[eNB_id][i]     = (int32_t*)malloc16_clear( sizeof(int32_t)*frame_parms->N_RB_UL*12*frame_parms->symbols_per_tti );
          eNB_pusch_vars[UE_id]->ul_ch_mag[eNB_id][i]  = (int32_t*)malloc16_clear( frame_parms->symbols_per_tti*sizeof(int32_t)*frame_parms->N_RB_UL*12 );
          eNB_pusch_vars[UE_id]->ul_ch_magb[eNB_id][i] = (int32_t*)malloc16_clear( frame_parms->symbols_per_tti*sizeof(int32_t)*frame_parms->N_RB_UL*12 );
        }

        // In case of Distributed Alamouti Collabrative scheme separate channel estimates are required for both the UEs
        if (cooperation_flag == 2) {
          eNB_pusch_vars[UE_id]->drs_ch_estimates_0[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) ); // UE 0 DRS estimates
          eNB_pusch_vars[UE_id]->drs_ch_estimates_1[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) ); // UE 1 DRS estimates

          for (i=0; i<frame_parms->nb_antennas_rx; i++) {
            eNB_pusch_vars[UE_id]->drs_ch_estimates_0[eNB_id][i] = (int32_t*)malloc16_clear( frame_parms->symbols_per_tti*sizeof(int32_t)*frame_parms->N_RB_UL*12 );
            eNB_pusch_vars[UE_id]->drs_ch_estimates_1[eNB_id][i] = (int32_t*)malloc16_clear( frame_parms->symbols_per_tti*sizeof(int32_t)*frame_parms->N_RB_UL*12 );
          }

          // Compensated data for the case of Distributed Alamouti Scheme
          eNB_pusch_vars[UE_id]->rxdataF_comp_0[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) ); // it will contain(y)*(h0*)
          eNB_pusch_vars[UE_id]->rxdataF_comp_1[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) ); // it will contain(y*)*(h1)

          for (i=0; i<frame_parms->nb_antennas_rx; i++) {
            eNB_pusch_vars[UE_id]->rxdataF_comp_0[eNB_id][i] = (int32_t*)malloc16_clear( frame_parms->symbols_per_tti*sizeof(int32_t)*frame_parms->N_RB_UL*12 );
            eNB_pusch_vars[UE_id]->rxdataF_comp_1[eNB_id][i] = (int32_t*)malloc16_clear( frame_parms->symbols_per_tti*sizeof(int32_t)*frame_parms->N_RB_UL*12 );
          }

          // UE 0
          eNB_pusch_vars[UE_id]->ul_ch_mag_0[eNB_id]  = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
          eNB_pusch_vars[UE_id]->ul_ch_magb_0[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );

          for (i=0; i<frame_parms->nb_antennas_rx; i++) {
            eNB_pusch_vars[UE_id]->ul_ch_mag_0[eNB_id][i]  = (int32_t*)malloc16_clear( frame_parms->symbols_per_tti*sizeof(int32_t)*frame_parms->N_RB_UL*12 );
            eNB_pusch_vars[UE_id]->ul_ch_magb_0[eNB_id][i] = (int32_t*)malloc16_clear( frame_parms->symbols_per_tti*sizeof(int32_t)*frame_parms->N_RB_UL*12 );
          }

          // UE 1
          eNB_pusch_vars[UE_id]->ul_ch_mag_1[eNB_id]  = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
          eNB_pusch_vars[UE_id]->ul_ch_magb_1[eNB_id] = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );

          for (i=0; i<frame_parms->nb_antennas_rx; i++) {
            eNB_pusch_vars[UE_id]->ul_ch_mag_1[eNB_id][i]  = (int32_t*)malloc16( frame_parms->symbols_per_tti*sizeof(int32_t)*frame_parms->N_RB_UL*12 );
            eNB_pusch_vars[UE_id]->ul_ch_magb_1[eNB_id][i] = (int32_t*)malloc16( frame_parms->symbols_per_tti*sizeof(int32_t)*frame_parms->N_RB_UL*12 );
          }
        }//cooperation_flag
      } //eNB_id

      eNB_pusch_vars[UE_id]->llr = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
    } // abstraction_flag
  } //UE_id

  if (abstraction_flag==0) {
    if (is_secondary_eNB) {
      for (eNB_id=0; eNB_id<3; eNB_id++) {
        phy_vars_eNB->dl_precoder_SeNB[eNB_id] = (int **)malloc16(4*sizeof(int*));

        if (phy_vars_eNB->dl_precoder_SeNB[eNB_id]) {
#ifdef DEBUG_PHY
          msg("[openair][SECSYS_PHY][INIT] phy_vars_eNB->dl_precoder_SeNB[%d] allocated at %p\n",eNB_id,
              phy_vars_eNB->dl_precoder_SeNB[eNB_id]);
#endif
        } else {
          msg("[openair][SECSYS_PHY][INIT] phy_vars_eNB->dl_precoder_SeNB[%d] not allocated\n",eNB_id);
          return(-1);
        }

        for (j=0; j<phy_vars_eNB->lte_frame_parms.nb_antennas_tx; j++) {
          phy_vars_eNB->dl_precoder_SeNB[eNB_id][j] = (int *)malloc16(2*sizeof(int)*(phy_vars_eNB->lte_frame_parms.ofdm_symbol_size)); // repeated format (hence the '2*')

          if (phy_vars_eNB->dl_precoder_SeNB[eNB_id][j]) {
#ifdef DEBUG_PHY
            msg("[openair][LTE_PHY][INIT] phy_vars_eNB->dl_precoder_SeNB[%d][%d] allocated at %p\n",eNB_id,j,
                phy_vars_eNB->dl_precoder_SeNB[eNB_id][j]);
#endif
            memset(phy_vars_eNB->dl_precoder_SeNB[eNB_id][j],0,2*sizeof(int)*(phy_vars_eNB->lte_frame_parms.ofdm_symbol_size));
          } else {
            msg("[openair][LTE_PHY][INIT] phy_vars_eNB->dl_precoder_SeNB[%d][%d] not allocated\n",eNB_id,j);
            return(-1);
          }
        } //for(j=...nb_antennas_tx

      } //for(eNB_id...
    }
  }

  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++)
    phy_vars_eNB->eNB_UE_stats_ptr[UE_id] = &phy_vars_eNB->eNB_UE_stats[UE_id];

  phy_vars_eNB->pdsch_config_dedicated->p_a = dB0; //defaul value until overwritten by RRCConnectionReconfiguration

  init_prach_tables(839);

  return (0);
}

