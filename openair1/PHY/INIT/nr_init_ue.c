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
#include "PHY/phy_extern_nr_ue.h"
//#include "SIMULATION/TOOLS/sim.h"
/*#include "RadioResourceConfigCommonSIB.h"
#include "RadioResourceConfigDedicated.h"
#include "TDD-Config.h"
#include "MBSFN-SubframeConfigList.h"*/
#include "openair1/PHY/defs_RU.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "assertions.h"
#include <math.h>
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
//#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_REFSIG/pss_nr.h"
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_REFSIG/nr_refsig.h"

//uint8_t dmrs1_tab_ue[8] = {0,2,3,4,6,8,9,10};

/*void phy_config_sib1_ue(uint8_t Mod_id,int CC_id,
                        uint8_t eNB_id,
                        TDD_Config_t *tdd_Config,
                        uint8_t SIwindowsize,
                        uint16_t SIperiod)
{

  NR_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][CC_id]->frame_parms;

  if (tdd_Config) {
    fp->tdd_config    = tdd_Config->subframeAssignment;
    fp->tdd_config_S  = tdd_Config->specialSubframePatterns;
  }

  fp->SIwindowsize  = SIwindowsize;
  fp->SIPeriod      = SIperiod;
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
  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;
  int i;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_CONFIG_SIB2, VCD_FUNCTION_IN);

  LOG_I(PHY,"[UE%d] Applying radioResourceConfigCommon from eNB%d\n",Mod_id,eNB_id);

  ue->prach_vars[eNB_id]->prach_pdu.root_seq_id                           =radioResourceConfigCommon->prach_Config.rootSequenceIndex;

  ue->prach_vars[eNB_id]->prach_Config_enabled=1;
  //ue->prach_vars[eNB_id]->prach_pdu.prach_ConfigIndex          =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_ConfigIndex;
  ue->prach_vars[eNB_id]->prach_pdu.restricted_set              =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.highSpeedFlag;
  ue->prach_vars[eNB_id]->prach_pdu.num_cs  =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.zeroCorrelationZoneConfig;
  //ue->prach_vars[eNB_id]->prach_pdu.prach_FreqOffset           =radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_FreqOffset;

  //compute_prach_seq(fp->prach_config_common.rootSequenceIndex,
  //      fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
  //      fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
  //      fp->prach_config_common.prach_ConfigInfo.highSpeedFlag,
  //      fp->frame_type,ue->X_u);



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
        LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %d\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
      } else if (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.present == MBSFN_SubframeConfig__subframeAllocation_PR_fourFrames) { // 24-bit subframe configuration
        fp->MBSFN_config[i].fourFrames_flag = 1;
        fp->MBSFN_config[i].mbsfn_SubframeConfig =
          mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[0]|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[1]<<8)|
          (mbsfn_SubframeConfigList->list.array[i]->subframeAllocation.choice.oneFrame.buf[2]<<16);

        LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %d\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
      }
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_CONFIG_SIB2, VCD_FUNCTION_OUT);

}

void phy_config_sib13_ue(uint8_t Mod_id,int CC_id,uint8_t eNB_id,int mbsfn_Area_idx,
                         long mbsfn_AreaId_r9)
{

  NR_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][CC_id]->frame_parms;


  LOG_I(PHY,"[UE%d] Applying MBSFN_Area_id %ld for index %d\n",Mod_id,mbsfn_AreaId_r9,mbsfn_Area_idx);

  if (mbsfn_Area_idx == 0) {
    fp->Nid_cell_mbsfn = (uint16_t)mbsfn_AreaId_r9;
    LOG_N(PHY,"Fix me: only called when mbsfn_Area_idx == 0)\n");
  }

  lte_gold_mbsfn(fp,PHY_vars_UE_g[Mod_id][CC_id]->lte_gold_mbsfn_table,fp->Nid_cell_mbsfn);

}*/


/*
 * Configures UE MAC and PHY with radioResourceCommon received in mobilityControlInfo IE during Handover
 */
/*void phy_config_afterHO_ue(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_id, MobilityControlInfo_t *mobilityControlInfo, uint8_t ho_failed)
{

  if(mobilityControlInfo!=NULL) {
    RadioResourceConfigCommon_t *radioResourceConfigCommon = &mobilityControlInfo->radioResourceConfigCommon;
    LOG_I(PHY,"radioResourceConfigCommon %p\n", radioResourceConfigCommon);
    memcpy((void *)&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms_before_ho,
           (void *)&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms,
           sizeof(NR_DL_FRAME_PARMS));
    PHY_vars_UE_g[Mod_id][CC_id]->ho_triggered = 1;
    //PHY_vars_UE_g[UE_id]->UE_mode[0] = PRACH;

    NR_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][CC_id]->frame_parms;
    //     int N_ZC;
    //     uint8_t prach_fmt;
    //     int u;

    LOG_I(PHY,"[UE%d] Handover triggered: Applying radioResourceConfigCommon from eNB %d\n",
          Mod_id,eNB_id);

    ue->prach_vars[eNB_id]->prach_pdu.root_seq_id                           =radioResourceConfigCommon->prach_Config.rootSequenceIndex;
    ue->prach_vars[eNB_id]->prach_Config_enabled=1;
    //ue->prach_vars[eNB_id]->prach_pdu.prach_ConfigIndex          =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->prach_ConfigIndex;
    ue->prach_vars[eNB_id]->prach_pdu.restricted_set              =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->highSpeedFlag;
    ue->prach_vars[eNB_id]->prach_pdu.num_cs  =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->zeroCorrelationZoneConfig;
    //ue->prach_vars[eNB_id]->prach_pdu.prach_FreqOffset           =radioResourceConfigCommon->prach_Config.prach_ConfigInfo->prach_FreqOffset;

    //     prach_fmt = get_prach_fmt(radioResourceConfigCommon->prach_Config.prach_ConfigInfo->prach_ConfigIndex,fp->frame_type);
    //     N_ZC = (prach_fmt <4)?839:139;
    //     u = (prach_fmt < 4) ? prach_root_sequence_map0_3[fp->prach_config_common.rootSequenceIndex] :
    //       prach_root_sequence_map4[fp->prach_config_common.rootSequenceIndex];

    //compute_prach_seq(u,N_ZC, PHY_vars_UE_g[Mod_id]->X_u);
    //compute_prach_seq(PHY_vars_UE_g[Mod_id][CC_id]->frame_parms.prach_config_common.rootSequenceIndex,
    //      PHY_vars_UE_g[Mod_id][CC_id]->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
    //      PHY_vars_UE_g[Mod_id][CC_id]->frame_parms.prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
    //      PHY_vars_UE_g[Mod_id][CC_id]->frame_parms.prach_config_common.prach_ConfigInfo.highSpeedFlag,
    //                  fp->frame_type,
    //                  PHY_vars_UE_g[Mod_id][CC_id]->X_u);


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


    PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[0][eNB_id]->crnti = mobilityControlInfo->newUE_Identity.buf[0]|(mobilityControlInfo->newUE_Identity.buf[1]<<8);
    PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[1][eNB_id]->crnti = mobilityControlInfo->newUE_Identity.buf[0]|(mobilityControlInfo->newUE_Identity.buf[1]<<8);

    LOG_I(PHY,"SET C-RNTI %x %x\n",PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[0][eNB_id]->crnti,
                                   PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[1][eNB_id]->crnti);
  }

  if(ho_failed) {
    LOG_D(PHY,"[UE%d] Handover failed, triggering RACH procedure\n",Mod_id);
    memcpy((void *)&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms,(void *)&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms_before_ho, sizeof(NR_DL_FRAME_PARMS));
    PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[eNB_id] = PRACH;
  }
}


void phy_config_meas_ue(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index,uint8_t n_adj_cells,unsigned int *adj_cell_id)
{

  PHY_NR_MEASUREMENTS *phy_meas = &PHY_vars_UE_g[Mod_id][CC_id]->measurements;
  int i;

  LOG_I(PHY,"Configuring inter-cell measurements for %d cells, ids: \n",n_adj_cells);

  for (i=0; i<n_adj_cells; i++) {
    LOG_I(PHY,"%d\n",adj_cell_id[i]);
    lte_gold(&PHY_vars_UE_g[Mod_id][CC_id]->frame_parms,PHY_vars_UE_g[Mod_id][CC_id]->lte_gold_table[i+1],adj_cell_id[i]);
  }

  phy_meas->n_adj_cells = n_adj_cells;
  memcpy((void*)phy_meas->adj_cell_id,(void *)adj_cell_id,n_adj_cells*sizeof(unsigned int));

}
*/


#if 0
void phy_config_harq_ue(module_id_t Mod_id,
                        int CC_id,
                        uint8_t eNB_id,
                        uint16_t max_harq_tx) {
  int num_of_threads,num_of_code_words;
  PHY_VARS_NR_UE *phy_vars_ue = PHY_vars_UE_g[Mod_id][CC_id];

  for (num_of_threads=0; num_of_threads<RX_NB_TH_MAX; num_of_threads++)
    for (num_of_code_words=0; num_of_code_words<NR_MAX_NB_CODEWORDS; num_of_code_words++)
      phy_vars_ue->ulsch[num_of_threads][eNB_id][num_of_code_words]->Mlimit = max_harq_tx;
}
#endif

extern uint16_t beta_cqi[16];

/*! \brief Helper function to allocate memory for DLSCH data structures.
 * \param[out] pdsch Pointer to the LTE_UE_PDSCH structure to initialize.
 * \param[in] frame_parms LTE_DL_FRAME_PARMS structure.
 * \note This function is optimistic in that it expects malloc() to succeed.
 */
void phy_init_nr_ue__PDSCH(NR_UE_PDSCH *const pdsch,
                           const NR_DL_FRAME_PARMS *const fp) {
  AssertFatal( pdsch, "pdsch==0" );
  pdsch->pmi_ext = (uint8_t *)malloc16_clear( fp->N_RB_DL );
  pdsch->llr[0] = (int16_t *)malloc16_clear( (8*(3*8*6144))*sizeof(int16_t) );
  pdsch->layer_llr[0] = (int16_t *)malloc16_clear( (8*(3*8*6144))*sizeof(int16_t) );
  pdsch->llr128 = (int16_t **)malloc16_clear( sizeof(int16_t *) );
  // FIXME! no further allocation for (int16_t*)pdsch->llr128 !!! expect SIGSEGV
  // FK, 11-3-2015: this is only as a temporary pointer, no memory is stored there
  pdsch->rxdataF_ext            = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->rxdataF_uespec_pilots  = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->rxdataF_comp0          = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->rho                    = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_ch_estimates        = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->dl_ch_estimates_ext    = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->dl_bf_ch_estimates     = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->dl_bf_ch_estimates_ext = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  //pdsch->dl_ch_rho_ext          = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  //pdsch->dl_ch_rho2_ext         = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_mag0             = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->dl_ch_magb0            = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->dl_ch_magr0            = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->ptrs_phase_per_slot    = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->ptrs_re_per_slot       = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
  pdsch->dl_ch_ptrs_estimates_ext = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
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
      pdsch->dl_ch_estimates[idx]         = (int32_t *)malloc16_clear( sizeof(int32_t) * fp->ofdm_symbol_size*7*2);
      pdsch->dl_ch_estimates_ext[idx]     = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_bf_ch_estimates[idx]      = (int32_t *)malloc16_clear( sizeof(int32_t) * fp->ofdm_symbol_size*7*2);
      pdsch->dl_bf_ch_estimates_ext[idx]  = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      //pdsch->dl_ch_rho_ext[idx]           = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      //pdsch->dl_ch_rho2_ext[idx]          = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_mag0[idx]              = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_magb0[idx]             = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_magr0[idx]             = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->ptrs_re_per_slot[idx]        = (int32_t *)malloc16_clear(sizeof(int32_t) * 14);
      pdsch->ptrs_phase_per_slot[idx]     = (int32_t *)malloc16_clear( sizeof(int32_t) * 14 );
      pdsch->dl_ch_ptrs_estimates_ext[idx]= (int32_t *)malloc16_clear( sizeof(int32_t) * num);
    }
  }
}

void phy_init_nr_ue_PUSCH(NR_UE_PUSCH *const pusch,
                          const NR_DL_FRAME_PARMS *const fp) {
  AssertFatal( pusch, "pusch==0" );

  for (int i=0; i<NR_MAX_NB_LAYERS; i++) {
    pusch->txdataF_layers[i] = (int32_t *)malloc16_clear((NR_MAX_PUSCH_ENCODED_LENGTH)*sizeof(int32_t *));
  }
}

int init_nr_ue_signal(PHY_VARS_NR_UE *ue,
                      int nb_connected_eNB,
                      uint8_t abstraction_flag) {
  // create shortcuts
  NR_DL_FRAME_PARMS *const fp            = &ue->frame_parms;
  NR_UE_COMMON *const common_vars        = &ue->common_vars;
  NR_UE_PBCH  **const pbch_vars          = ue->pbch_vars;
  NR_UE_PRACH **const prach_vars         = ue->prach_vars;
  int i,j,k,l,slot,symb,q;
  int eNB_id;
  int th_id;
  uint32_t ****pusch_dmrs;
  uint16_t N_n_scid[2] = {0,1}; // [HOTFIX] This is a temporary implementation of scramblingID0 and scramblingID1 which are given by DMRS-UplinkConfig
  int n_scid;
  abstraction_flag = 0;
  fp->nb_antennas_tx = 1;
  fp->nb_antennas_rx=1;
  // dmrs_UplinkConfig_t *dmrs_Uplink_Config = &ue->pusch_config.dmrs_UplinkConfig;
  // ptrs_UplinkConfig_t *ptrs_Uplink_Config = &ue->pusch_config.dmrs_UplinkConfig.ptrs_UplinkConfig;
  LOG_I(PHY, "Initializing UE vars (abstraction %u) for gNB TXant %u, UE RXant %u\n", abstraction_flag, fp->nb_antennas_tx, fp->nb_antennas_rx);
  //LOG_D(PHY,"[MSC_NEW][FRAME 00000][PHY_UE][MOD %02u][]\n", ue->Mod_id+NB_eNB_INST);
  phy_init_nr_top(ue);
  // many memory allocation sizes are hard coded
  AssertFatal( fp->nb_antennas_rx <= 2, "hard coded allocation for ue_common_vars->dl_ch_estimates[eNB_id]" );
  AssertFatal( nb_connected_eNB <= NUMBER_OF_CONNECTED_eNB_MAX, "n_connected_eNB is too large" );
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
  // init NR modulation lookup tables
  nr_generate_modulation_table();

  /////////////////////////PUSCH init/////////////////////////
  ///////////
  for (th_id = 0; th_id < RX_NB_TH_MAX; th_id++) {
    for (eNB_id = 0; eNB_id < ue->n_connected_eNB; eNB_id++) {
      ue->pusch_vars[th_id][eNB_id] = (NR_UE_PUSCH *)malloc16(sizeof(NR_UE_PUSCH));
      phy_init_nr_ue_PUSCH( ue->pusch_vars[th_id][eNB_id], fp );
    }
  }

  ///////////
  ////////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////PUSCH DMRS init/////////////////////////
  ///////////

  // default values until overwritten by RRCConnectionReconfiguration

  for (i=0; i<MAX_NR_OF_UL_ALLOCATIONS; i++) {
    ue->pusch_config.pusch_TimeDomainResourceAllocation[i] = (PUSCH_TimeDomainResourceAllocation_t *)malloc16(sizeof(PUSCH_TimeDomainResourceAllocation_t));
    ue->pusch_config.pusch_TimeDomainResourceAllocation[i]->mappingType = typeB;
  }

  for (i=0;i<MAX_NR_OF_DL_ALLOCATIONS;i++){
    ue->PDSCH_Config.pdsch_TimeDomainResourceAllocation[i] = (NR_PDSCH_TimeDomainResourceAllocation_t *)malloc16(sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
    ue->PDSCH_Config.pdsch_TimeDomainResourceAllocation[i]->mappingType = typeA;
  }

  //------------- config DMRS parameters--------------//
  // dmrs_Uplink_Config->pusch_dmrs_type = pusch_dmrs_type1;
  // dmrs_Uplink_Config->pusch_dmrs_AdditionalPosition = pusch_dmrs_pos0;
  // dmrs_Uplink_Config->pusch_maxLength = pusch_len1;
  //-------------------------------------------------//
  ue->dmrs_DownlinkConfig.pdsch_dmrs_type = pdsch_dmrs_type1;
  ue->dmrs_DownlinkConfig.pdsch_dmrs_AdditionalPosition = pdsch_dmrs_pos0;
  ue->dmrs_DownlinkConfig.pdsch_maxLength = pdsch_len1;
  //-------------------------------------------------//

  ue->nr_gold_pusch_dmrs = (uint32_t ****)malloc16(fp->slots_per_frame*sizeof(uint32_t ***));
  pusch_dmrs             = ue->nr_gold_pusch_dmrs;
  n_scid = 0; // This quantity is indicated by higher layer parameter dmrs-SeqInitialization

  for (slot=0; slot<fp->slots_per_frame; slot++) {
    pusch_dmrs[slot] = (uint32_t ***)malloc16(fp->symbols_per_slot*sizeof(uint32_t **));
    AssertFatal(pusch_dmrs[slot]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d - malloc failed\n", slot);

    for (symb=0; symb<fp->symbols_per_slot; symb++) {
      pusch_dmrs[slot][symb] = (uint32_t **)malloc16(NR_MAX_NB_CODEWORDS*sizeof(uint32_t *));
      AssertFatal(pusch_dmrs[slot][symb]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);

      for (q=0; q<NR_MAX_NB_CODEWORDS; q++) {
        pusch_dmrs[slot][symb][q] = (uint32_t *)malloc16(NR_MAX_PDSCH_DMRS_INIT_LENGTH_DWORD*sizeof(uint32_t));
        AssertFatal(pusch_dmrs[slot][symb][q]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d symbol %d codeword %d - malloc failed\n", slot, symb, q);
      }
    }
  }

  nr_init_pusch_dmrs(ue, N_n_scid, n_scid);
  ///////////
  ////////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////PUSCH PTRS init/////////////////////////
  ///////////

  //------------- config PTRS parameters--------------//
  // ptrs_Uplink_Config->timeDensity.ptrs_mcs1 = 2; // setting MCS values to 0 indicate abscence of time_density field in the configuration
  // ptrs_Uplink_Config->timeDensity.ptrs_mcs2 = 4;
  // ptrs_Uplink_Config->timeDensity.ptrs_mcs3 = 10;
  // ptrs_Uplink_Config->frequencyDensity.n_rb0 = 25;     // setting N_RB values to 0 indicate abscence of frequency_density field in the configuration
  // ptrs_Uplink_Config->frequencyDensity.n_rb1 = 75;
  // ptrs_Uplink_Config->resourceElementOffset = 0;
  //-------------------------------------------------//

  ///////////
  ////////////////////////////////////////////////////////////////////////////////////////////

  for (i=0; i<10; i++)
    ue->tx_power_dBm[i]=-127;

  if (abstraction_flag == 0) {
    // init TX buffers
    common_vars->txdata  = (int32_t **)malloc16( fp->nb_antennas_tx*sizeof(int32_t *) );
    common_vars->txdataF = (int32_t **)malloc16( fp->nb_antennas_tx*sizeof(int32_t *) );

    for (i=0; i<fp->nb_antennas_tx; i++) {
      common_vars->txdata[i]  = (int32_t *)malloc16_clear( fp->samples_per_subframe*10*sizeof(int32_t) );
      common_vars->txdataF[i] = (int32_t *)malloc16_clear( fp->samples_per_slot_wCP*sizeof(int32_t) );
    }

    // init RX buffers
    common_vars->rxdata   = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );

    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      common_vars->common_vars_rx_data_per_thread[th_id].rxdataF  = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
    }

    for (i=0; i<fp->nb_antennas_rx; i++) {
      common_vars->rxdata[i] = (int32_t *) malloc16_clear( (2*(fp->samples_per_frame)+2048)*sizeof(int32_t) );

      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        common_vars->common_vars_rx_data_per_thread[th_id].rxdataF[i] = (int32_t *)malloc16_clear( sizeof(int32_t)*(fp->samples_per_slot_wCP) );
      }
    }
  }

  // DLSCH
  for (eNB_id=0; eNB_id<ue->n_connected_eNB; eNB_id++) {
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      ue->pdsch_vars[th_id][eNB_id]     = (NR_UE_PDSCH *)malloc16_clear(sizeof(NR_UE_PDSCH));
    }

    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      ue->pdcch_vars[th_id][eNB_id] = (NR_UE_PDCCH *)malloc16_clear(sizeof(NR_UE_PDCCH));
    }

    prach_vars[eNB_id]     = (NR_UE_PRACH *)malloc16_clear(sizeof(NR_UE_PRACH));
    pbch_vars[eNB_id]      = (NR_UE_PBCH *)malloc16_clear(sizeof(NR_UE_PBCH));

    if (abstraction_flag == 0) {
      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        phy_init_nr_ue__PDSCH( ue->pdsch_vars[th_id][eNB_id], fp );
      }

      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        ue->pdsch_vars[th_id][eNB_id]->llr_shifts      = (uint8_t *)malloc16_clear(7*2*fp->N_RB_DL*12);
        ue->pdsch_vars[th_id][eNB_id]->llr_shifts_p        = ue->pdsch_vars[0][eNB_id]->llr_shifts;
        ue->pdsch_vars[th_id][eNB_id]->llr[1]              = (int16_t *)malloc16_clear( (8*(3*8*8448))*sizeof(int16_t) );
        ue->pdsch_vars[th_id][eNB_id]->layer_llr[1]        = (int16_t *)malloc16_clear( (8*(3*8*8448))*sizeof(int16_t) );
        ue->pdsch_vars[th_id][eNB_id]->llr128_2ndstream    = (int16_t **)malloc16_clear( sizeof(int16_t *) );
        ue->pdsch_vars[th_id][eNB_id]->rho                 = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );
      }

      for (int i=0; i<fp->nb_antennas_rx; i++) {
        for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
          ue->pdsch_vars[th_id][eNB_id]->rho[i]     = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
        }
      }

      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        ue->pdsch_vars[th_id][eNB_id]->dl_ch_rho2_ext      = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      }

      for (i=0; i<fp->nb_antennas_rx; i++)
        for (j=0; j<4; j++) {
          const int idx = (j<<1)+i;
          const size_t num = 7*2*fp->N_RB_DL*12+4;

          for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
            ue->pdsch_vars[th_id][eNB_id]->dl_ch_rho2_ext[idx] = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
          }
        }

      //const size_t num = 7*2*fp->N_RB_DL*12+4;
      for (k=0; k<8; k++) { //harq_pid
        for (l=0; l<8; l++) { //round
          for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
            ue->pdsch_vars[th_id][eNB_id]->rxdataF_comp1[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
            ue->pdsch_vars[th_id][eNB_id]->dl_ch_rho_ext[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
            ue->pdsch_vars[th_id][eNB_id]->dl_ch_mag1[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
            ue->pdsch_vars[th_id][eNB_id]->dl_ch_magb1[k][l] = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
          }

          for (int i=0; i<fp->nb_antennas_rx; i++)
            for (int j=0; j<4; j++) { //frame_parms->nb_antennas_tx; j++)
              const int idx = (j<<1)+i;

              for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
                ue->pdsch_vars[th_id][eNB_id]->dl_ch_rho_ext[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
                ue->pdsch_vars[th_id][eNB_id]->rxdataF_comp1[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
                ue->pdsch_vars[th_id][eNB_id]->dl_ch_mag1[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
                ue->pdsch_vars[th_id][eNB_id]->dl_ch_magb1[k][l][idx] = (int32_t *)malloc16_clear( 7*2*sizeof(int32_t)*(fp->N_RB_DL*12) );
              }
            }
        }
      }

      // 100 PRBs * 12 REs/PRB * 4 PDCCH SYMBOLS * 2 LLRs/RE
      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        ue->pdcch_vars[th_id][eNB_id]->llr   = (int16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
        ue->pdcch_vars[th_id][eNB_id]->llr16 = (int16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
        ue->pdcch_vars[th_id][eNB_id]->wbar  = (int16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
        ue->pdcch_vars[th_id][eNB_id]->e_rx  = (int16_t *)malloc16_clear( 4*2*100*12 );
        ue->pdcch_vars[th_id][eNB_id]->rxdataF_comp        = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
        ue->pdcch_vars[th_id][eNB_id]->dl_ch_rho_ext       = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
        ue->pdcch_vars[th_id][eNB_id]->rho                 = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
        ue->pdcch_vars[th_id][eNB_id]->rxdataF_ext         = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
        ue->pdcch_vars[th_id][eNB_id]->dl_ch_estimates_ext = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
        // Channel estimates
        ue->pdcch_vars[th_id][eNB_id]->dl_ch_estimates      = (int32_t **)malloc16_clear(8*sizeof(int32_t *));
        ue->pdcch_vars[th_id][eNB_id]->dl_ch_estimates_time = (int32_t **)malloc16_clear(8*sizeof(int32_t *));

        for (i=0; i<fp->nb_antennas_rx; i++) {
          ue->pdcch_vars[th_id][eNB_id]->rho[i] = (int32_t *)malloc16_clear( sizeof(int32_t)*(100*12*4) );

          for (j=0; j<4; j++) {
            int idx = (j<<1) + i;
            ue->pdcch_vars[th_id][eNB_id]->dl_ch_estimates[idx] = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->symbols_per_slot*(fp->ofdm_symbol_size+LTE_CE_FILTER_LENGTH) );
            ue->pdcch_vars[th_id][eNB_id]->dl_ch_estimates_time[idx] = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->ofdm_symbol_size*2 );
            //  size_t num = 7*2*fp->N_RB_DL*12;
            size_t num = 4*273*12;  // 4 symbols, 100 PRBs, 12 REs per PRB
            ue->pdcch_vars[th_id][eNB_id]->rxdataF_comp[idx]        = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
            ue->pdcch_vars[th_id][eNB_id]->dl_ch_rho_ext[idx]       = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
            ue->pdcch_vars[th_id][eNB_id]->rxdataF_ext[idx]         = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
            ue->pdcch_vars[th_id][eNB_id]->dl_ch_estimates_ext[idx] = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
          }
        }
      }

      // PBCH
      pbch_vars[eNB_id]->rxdataF_ext         = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
      pbch_vars[eNB_id]->rxdataF_comp        = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      pbch_vars[eNB_id]->dl_ch_estimates     = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      pbch_vars[eNB_id]->dl_ch_estimates_ext = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      pbch_vars[eNB_id]->dl_ch_estimates_time = (int32_t **)malloc16_clear( 8*sizeof(int32_t *) );
      pbch_vars[eNB_id]->llr                 = (int16_t *)malloc16_clear( 1920 ); //
      prach_vars[eNB_id]->prachF             = (int16_t *)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(fp->ofdm_symbol_size*12)) );
      prach_vars[eNB_id]->prach              = (int16_t *)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(fp->ofdm_symbol_size*12)) );

      for (i=0; i<fp->nb_antennas_rx; i++) {
        pbch_vars[eNB_id]->rxdataF_ext[i]    = (int32_t *)malloc16_clear( sizeof(int32_t)*20*12*4 );

        for (j=0; j<4; j++) {//fp->nb_antennas_tx;j++) {
          int idx = (j<<1)+i;
          pbch_vars[eNB_id]->rxdataF_comp[idx]        = (int32_t *)malloc16_clear( sizeof(int32_t)*20*12*4 );
          pbch_vars[eNB_id]->dl_ch_estimates[idx]     = (int32_t *)malloc16_clear( sizeof(int32_t)*7*2*sizeof(int)*(fp->ofdm_symbol_size) );
          pbch_vars[eNB_id]->dl_ch_estimates_time[idx]= (int32_t *)malloc16_clear( sizeof(int32_t)*7*2*sizeof(int)*(fp->ofdm_symbol_size) );
          pbch_vars[eNB_id]->dl_ch_estimates_ext[idx] = (int32_t *)malloc16_clear( sizeof(int32_t)*20*12*4 );
        }
      }
    }

    pbch_vars[eNB_id]->decoded_output = (uint8_t *)malloc16_clear( 64 );
  }

  // initialization for the last instance of pdsch_vars (used for MU-MIMO)
  for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
    ue->pdsch_vars[th_id][eNB_id]     = (NR_UE_PDSCH *)malloc16_clear( sizeof(NR_UE_PDSCH) );
  }

  if (abstraction_flag == 0) {
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      //phy_init_lte_ue__PDSCH( ue->pdsch_vars[th_id][eNB_id], fp );
      ue->pdsch_vars[th_id][eNB_id]->llr[1] = (int16_t *)malloc16_clear( (8*(3*8*8448))*sizeof(int16_t) );
      ue->pdsch_vars[th_id][eNB_id]->layer_llr[1] = (int16_t *)malloc16_clear( (8*(3*8*8448))*sizeof(int16_t) );
    }
  } else { //abstraction == 1
    ue->sinr_dB = (double *) malloc16_clear( fp->N_RB_DL*12*sizeof(double) );
  }

  ue->sinr_CQI_dB = (double *) malloc16_clear( fp->N_RB_DL*12*sizeof(double) );
  ue->init_averaging = 1;

  // default value until overwritten by RRCConnectionReconfiguration
  if (fp->nb_antenna_ports_gNB==2)
    ue->pdsch_config_dedicated->p_a = dBm3;
  else
    ue->pdsch_config_dedicated->p_a = dB0;

  // set channel estimation to do linear interpolation in time
  ue->high_speed_flag = 1;
  ue->ch_est_alpha    = 24576;
  // enable MIB/SIB decoding by default
  ue->decode_MIB = 1;
  ue->decode_SIB = 1;

  init_nr_prach_tables(839);

  return 0;
}

void init_nr_ue_transport(PHY_VARS_NR_UE *ue,
                          int abstraction_flag) {
  for (int i=0; i<NUMBER_OF_CONNECTED_eNB_MAX; i++) {
    for (int j=0; j<2; j++) {
      for (int k=0; k<RX_NB_TH_MAX; k++) {
        AssertFatal((ue->dlsch[k][i][j]  = new_nr_ue_dlsch(1,NR_MAX_DLSCH_HARQ_PROCESSES,NSOFT,MAX_LDPC_ITERATIONS,ue->frame_parms.N_RB_DL, abstraction_flag))!=NULL,"Can't get ue dlsch structures\n");
        LOG_D(PHY,"dlsch[%d][%d][%d] => %p\n",k,i,j,ue->dlsch[k][i][j]);
        AssertFatal((ue->ulsch[k][i][j]  = new_nr_ue_ulsch(ue->frame_parms.N_RB_UL, NR_MAX_ULSCH_HARQ_PROCESSES, abstraction_flag))!=NULL,"Can't get ue ulsch structures\n");
        LOG_D(PHY,"ulsch[%d][%d][%d] => %p\n",k,i,j,ue->ulsch[k][i][j]);
      }
    }

    ue->dlsch_SI[i]  = new_nr_ue_dlsch(1,1,NSOFT,MAX_LDPC_ITERATIONS,ue->frame_parms.N_RB_DL, abstraction_flag);
    ue->dlsch_ra[i]  = new_nr_ue_dlsch(1,1,NSOFT,MAX_LDPC_ITERATIONS,ue->frame_parms.N_RB_DL, abstraction_flag);
    ue->transmission_mode[i] = ue->frame_parms.nb_antenna_ports_gNB==1 ? 1 : 2;
  }

  //ue->frame_parms.pucch_config_common.deltaPUCCH_Shift = 1;
  ue->dlsch_MCH[0]  = new_nr_ue_dlsch(1,NR_MAX_DLSCH_HARQ_PROCESSES,NSOFT,MAX_LDPC_ITERATIONS_MBSFN,ue->frame_parms.N_RB_DL,0);

  for(int i=0; i<5; i++)
    ue->dl_stats[i] = 0;
}


void init_N_TA_offset(PHY_VARS_NR_UE *ue){

  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;

  if (fp->frame_type == FDD) {
    ue->N_TA_offset = 0;
  } else {
    int N_RB = fp->N_RB_DL;
    int N_TA_offset = fp->ul_CarrierFreq < 6e9 ? 400 : 431; // reference samples  for 25600Tc @ 30.72 Ms/s for FR1, same @ 61.44 Ms/s for FR2
    double factor = 1;
    switch (fp->numerology_index) {
      case 0: //15 kHz scs
        AssertFatal(N_TA_offset == 400, "scs_common 15kHz only for FR1\n");
        if (N_RB <= 25) factor = .25;      // 7.68 Ms/s
        else if (N_RB <=50) factor = .5;   // 15.36 Ms/s
        else if (N_RB <=75) factor = 1.0;  // 30.72 Ms/s
        else if (N_RB <=100) factor = 1.0; // 30.72 Ms/s
        else AssertFatal(1==0, "Too many PRBS for mu=0\n");
        break;
      case 1: //30 kHz sc
        AssertFatal(N_TA_offset == 400, "scs_common 30kHz only for FR1\n");
        if (N_RB <= 106) factor = 2.0; // 61.44 Ms/s
        else if (N_RB <= 275) factor = 4.0; // 122.88 Ms/s
        break;
      case 2: //60 kHz scs
        AssertFatal(1==0, "scs_common should not be 60 kHz\n");
        break;
      case 3: //120 kHz scs
        AssertFatal(N_TA_offset == 431, "scs_common 120kHz only for FR2\n");
        break;
      case 4: //240 kHz scs
        AssertFatal(1==0, "scs_common should not be 60 kHz\n");
        if (N_RB <= 32) factor = 1.0; // 61.44 Ms/s
        else if (N_RB <= 66) factor = 2.0; // 122.88 Ms/s
        else AssertFatal(1==0, "N_RB %d is too big for curretn FR2 implementation\n", N_RB);
        break;

      if (N_RB == 100)
        ue->N_TA_offset = 624;
      else if (N_RB == 50)
        ue->N_TA_offset = 624/2;
      else if (N_RB == 25)
        ue->N_TA_offset = 624/4;
    }

    if (fp->threequarter_fs == 1)
      factor = factor*.75;

    ue->N_TA_offset = (int)(N_TA_offset * factor);

    LOG_I(PHY,"UE %d Setting N_TA_offset to %d samples (factor %f, UL Freq %lu, N_RB %d)\n", ue->Mod_id, ue->N_TA_offset, factor, fp->ul_CarrierFreq, N_RB);
  }
}

void phy_init_nr_top(PHY_VARS_NR_UE *ue) {
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  crcTableInit();
  load_dftslib();
  init_context_synchro_nr(frame_parms);
  generate_ul_reference_signal_sequences(SHRT_MAX);
  // Polar encoder init for PBCH
  //lte_sync_time_init(frame_parms);
  //generate_ul_ref_sigs();
  //generate_ul_ref_sigs_rx();
  //generate_64qam_table();
  //generate_16qam_table();
  //generate_RIV_tables();
  //init_unscrambling_lut();
  //init_scrambling_lut();
  //set_taus_seed(1328);
}
