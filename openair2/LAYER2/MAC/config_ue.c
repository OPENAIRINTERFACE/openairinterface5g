
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


/*! \file config_ue.c
 * \brief UE configuration performed by RRC or as a consequence of RRC procedures / This includes FeMBMS UE procedures
 * \author  Navid Nikaein, Raymond Knopp and Javier Morgade
 * \date 2010 - 2014 / 2019
 * \version 0.1
 * \email: navid.nikaein@eurecom.fr, javier.morgade@ieee.org
 * @ingroup _mac

 */


#include "COMMON/platform_types.h"
#include "COMMON/platform_constants.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "SCHED_UE/sched_UE.h"
#include "LTE_SystemInformationBlockType2.h"
#include "LTE_RadioResourceConfigDedicated.h"
#if (LTE_RRC_VERSION >= MAKE_VERSION(13, 0, 0))
  #include "LTE_PRACH-ConfigSIB-v1310.h"
#endif
#include "LTE_MeasGapConfig.h"
#include "LTE_MeasObjectToAddModList.h"
#include "LTE_TDD-Config.h"
#include "LTE_MAC-MainConfig.h"
#include "mac.h"
#include "mac_proto.h"
#include "mac_extern.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/INIT/phy_init.h"

#include "common/ran_context.h"
#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
  #include "LTE_MBSFN-AreaInfoList-r9.h"
  #include "LTE_MBSFN-AreaInfo-r9.h"
  #include "LTE_MBSFN-SubframeConfigList.h"
  #include "LTE_PMCH-InfoList-r9.h"
#endif

extern void mac_init_cell_params(int Mod_idP,int CC_idP);
extern void phy_reset_ue(module_id_t Mod_id,uint8_t CC_id,uint8_t eNB_index);



/* sec 5.9, 36.321: MAC Reset Procedure */
void ue_mac_reset(module_id_t module_idP, uint8_t eNB_index) {
  //Resetting Bj
  UE_mac_inst[module_idP].scheduling_info.Bj[0] = 0;
  UE_mac_inst[module_idP].scheduling_info.Bj[1] = 0;
  UE_mac_inst[module_idP].scheduling_info.Bj[2] = 0;
  //Stopping all timers
  //timeAlignmentTimer expires
  // PHY changes for UE MAC reset
  phy_reset_ue(module_idP, 0, eNB_index);
  // notify RRC to relase PUCCH/SRS
  // cancel all pending SRs
  UE_mac_inst[module_idP].scheduling_info.SR_pending = 0;
  UE_mac_inst[module_idP].scheduling_info.SR_COUNTER = 0;
  //Set BSR Trigger Bmp and remove timer flags
  UE_mac_inst[module_idP].BSR_reporting_active = BSR_TRIGGER_NONE;
  // stop ongoing RACH procedure
  // discard explicitly signaled ra_PreambleIndex and ra_RACH_MaskIndex, if any
  UE_mac_inst[module_idP].RA_prach_resources.ra_PreambleIndex = 0;  // check!
  UE_mac_inst[module_idP].RA_prach_resources.ra_RACH_MaskIndex = 0;
  ue_init_mac(module_idP);  //This will hopefully do the rest of the MAC reset procedure
}

int32_t **rxdata;
int32_t **txdata;


int
rrc_mac_config_req_ue(module_id_t Mod_idP,
                      int CC_idP,
                      uint8_t eNB_index,
                      LTE_RadioResourceConfigCommonSIB_t *
                      radioResourceConfigCommon,
                      struct LTE_PhysicalConfigDedicated
                      *physicalConfigDedicated,
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  LTE_SCellToAddMod_r10_t *sCellToAddMod_r10,
  //struct PhysicalConfigDedicatedSCell_r10 *physicalConfigDedicatedSCell_r10,
#endif
                      LTE_MeasObjectToAddMod_t **measObj,
                      LTE_MAC_MainConfig_t *mac_MainConfig,
                      long logicalChannelIdentity,
                      LTE_LogicalChannelConfig_t *logicalChannelConfig,
                      LTE_MeasGapConfig_t *measGapConfig,
                      LTE_TDD_Config_t *tdd_Config,
                      LTE_MobilityControlInfo_t *mobilityControlInfo,
                      uint8_t *SIwindowsize,
                      uint16_t *SIperiod,
                      LTE_ARFCN_ValueEUTRA_t *ul_CarrierFreq,
                      long *ul_Bandwidth,
                      LTE_AdditionalSpectrumEmission_t *
                      additionalSpectrumEmission,
                      struct LTE_MBSFN_SubframeConfigList
                      *mbsfn_SubframeConfigList
#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
  , uint8_t MBMS_Flag,
  LTE_MBSFN_AreaInfoList_r9_t *mbsfn_AreaInfoList,
  LTE_PMCH_InfoList_r9_t *pmch_InfoList
#endif
#ifdef CBA
  , uint8_t num_active_cba_groups, uint16_t cba_rnti
#endif
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  ,config_action_t  config_action
  ,const uint32_t *const sourceL2Id
  ,const uint32_t *const destinationL2Id
#endif
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                           ,
                           uint8_t FeMBMS_Flag,
                           struct LTE_NonMBSFN_SubframeConfig_r14 * nonMBSFN_SubframeConfig,
                           LTE_MBSFN_AreaInfoList_r9_t * mbsfn_AreaInfoList_fembms
#endif
		      )
{

  int i;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_IN);
  LOG_I(MAC, "[CONFIG][UE %d] Configuring MAC/PHY from eNB %d\n",
        Mod_idP, eNB_index);

  if (tdd_Config != NULL) {
    UE_mac_inst[Mod_idP].tdd_Config = tdd_Config;
  }

  if (tdd_Config && SIwindowsize && SIperiod) {
    phy_config_sib1_ue(Mod_idP, 0, eNB_index, tdd_Config,
                       *SIwindowsize, *SIperiod);
  }

  if (radioResourceConfigCommon != NULL) {
    UE_mac_inst[Mod_idP].radioResourceConfigCommon =
      radioResourceConfigCommon;
    phy_config_sib2_ue(Mod_idP, 0, eNB_index,
                       radioResourceConfigCommon, ul_CarrierFreq,
                       ul_Bandwidth, additionalSpectrumEmission,
                       NULL/*mbsfn_SubframeConfigList*/);
  }

  // SRB2_lchan_config->choice.explicitValue.ul_SpecificParameters->logicalChannelGroup
  if (logicalChannelConfig != NULL) {
    LOG_I(MAC,
          "[CONFIG][UE %d] Applying RRC logicalChannelConfig from eNB%d\n",
          Mod_idP, eNB_index);
    UE_mac_inst[Mod_idP].logicalChannelConfig[logicalChannelIdentity] =
      logicalChannelConfig;
    UE_mac_inst[Mod_idP].scheduling_info.Bj[logicalChannelIdentity] = 0;  // initilize the bucket for this lcid
    AssertFatal(logicalChannelConfig->ul_SpecificParameters != NULL,
                "[UE %d] LCID %ld NULL ul_SpecificParameters\n",
                Mod_idP, logicalChannelIdentity);
    UE_mac_inst[Mod_idP].scheduling_info.bucket_size[logicalChannelIdentity] = logicalChannelConfig->ul_SpecificParameters->prioritisedBitRate *
        logicalChannelConfig->ul_SpecificParameters->bucketSizeDuration; // set the max bucket size

    if (logicalChannelConfig->ul_SpecificParameters->
        logicalChannelGroup != NULL) {
      UE_mac_inst[Mod_idP].scheduling_info.
      LCGID[logicalChannelIdentity] =
        *logicalChannelConfig->ul_SpecificParameters->
        logicalChannelGroup;
      LOG_D(MAC,
            "[CONFIG][UE %d] LCID %ld is attached to the LCGID %ld\n",
            Mod_idP, logicalChannelIdentity,
            *logicalChannelConfig->
            ul_SpecificParameters->logicalChannelGroup);
    } else {
      UE_mac_inst[Mod_idP].scheduling_info.
      LCGID[logicalChannelIdentity] = MAX_NUM_LCGID;
    }

    UE_mac_inst[Mod_idP].
    scheduling_info.LCID_buffer_remain[logicalChannelIdentity] = 0;
  }

  if (mac_MainConfig != NULL) {
    LOG_I(MAC,
          "[CONFIG][UE%d] Applying RRC macMainConfig from eNB%d\n",
          Mod_idP, eNB_index);
    UE_mac_inst[Mod_idP].macConfig = mac_MainConfig;
    UE_mac_inst[Mod_idP].measGapConfig = measGapConfig;

    if (mac_MainConfig->ul_SCH_Config) {
      if (mac_MainConfig->ul_SCH_Config->periodicBSR_Timer) {
        UE_mac_inst[Mod_idP].scheduling_info.periodicBSR_Timer =
          (uint16_t) *
          mac_MainConfig->ul_SCH_Config->periodicBSR_Timer;
      } else {
        UE_mac_inst[Mod_idP].scheduling_info.periodicBSR_Timer =
#if (LTE_RRC_VERSION < MAKE_VERSION(12, 0, 0))
          (uint16_t)
          LTE_MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_infinity
#else
          (uint16_t) LTE_PeriodicBSR_Timer_r12_infinity;
#endif
          ;
      }

      if (mac_MainConfig->ul_SCH_Config->maxHARQ_Tx) {
        UE_mac_inst[Mod_idP].scheduling_info.maxHARQ_Tx =
          (uint16_t) * mac_MainConfig->ul_SCH_Config->maxHARQ_Tx;
      } else {
        UE_mac_inst[Mod_idP].scheduling_info.maxHARQ_Tx =
          (uint16_t)
          LTE_MAC_MainConfig__ul_SCH_Config__maxHARQ_Tx_n5;
      }

      if(NFAPI_MODE !=  NFAPI_UE_STUB_PNF)
        phy_config_harq_ue(Mod_idP, 0, eNB_index,
                           UE_mac_inst[Mod_idP].
                           scheduling_info.maxHARQ_Tx);

      if (mac_MainConfig->ul_SCH_Config->retxBSR_Timer) {
        UE_mac_inst[Mod_idP].scheduling_info.retxBSR_Timer =
          (uint16_t) mac_MainConfig->ul_SCH_Config->
          retxBSR_Timer;
      } else {
#if (LTE_RRC_VERSION < MAKE_VERSION(12, 0, 0))
        UE_mac_inst[Mod_idP].scheduling_info.retxBSR_Timer =
          (uint16_t)
          LTE_MAC_MainConfig__ul_SCH_Config__retxBSR_Timer_sf2560;
#else
        UE_mac_inst[Mod_idP].scheduling_info.retxBSR_Timer =
          (uint16_t) LTE_RetxBSR_Timer_r12_sf2560;
#endif
      }
    }

#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))

    if (mac_MainConfig->ext1
        && mac_MainConfig->ext1->sr_ProhibitTimer_r9) {
      UE_mac_inst[Mod_idP].scheduling_info.sr_ProhibitTimer =
        (uint16_t) * mac_MainConfig->ext1->sr_ProhibitTimer_r9;
    } else {
      UE_mac_inst[Mod_idP].scheduling_info.sr_ProhibitTimer = 0;
    }

    if (mac_MainConfig->ext2
        && mac_MainConfig->ext2->mac_MainConfig_v1020) {
      if (mac_MainConfig->ext2->
          mac_MainConfig_v1020->extendedBSR_Sizes_r10) {
        UE_mac_inst[Mod_idP].scheduling_info.
        extendedBSR_Sizes_r10 =
          (uint16_t) *
          mac_MainConfig->ext2->
          mac_MainConfig_v1020->extendedBSR_Sizes_r10;
      } else {
        UE_mac_inst[Mod_idP].scheduling_info.
        extendedBSR_Sizes_r10 = (uint16_t) 0;
      }

      if (mac_MainConfig->ext2->mac_MainConfig_v1020->
          extendedPHR_r10) {
        UE_mac_inst[Mod_idP].scheduling_info.extendedPHR_r10 =
          (uint16_t) *
          mac_MainConfig->ext2->mac_MainConfig_v1020->
          extendedPHR_r10;
      } else {
        UE_mac_inst[Mod_idP].scheduling_info.extendedPHR_r10 =
          (uint16_t) 0;
      }
    } else {
      UE_mac_inst[Mod_idP].scheduling_info.extendedBSR_Sizes_r10 =
        (uint16_t) 0;
      UE_mac_inst[Mod_idP].scheduling_info.extendedPHR_r10 =
        (uint16_t) 0;
    }

#endif
    UE_mac_inst[Mod_idP].scheduling_info.periodicBSR_SF =
      MAC_UE_BSR_TIMER_NOT_RUNNING;
    UE_mac_inst[Mod_idP].scheduling_info.retxBSR_SF =
      MAC_UE_BSR_TIMER_NOT_RUNNING;
    UE_mac_inst[Mod_idP].BSR_reporting_active = BSR_TRIGGER_NONE;
    LOG_D(MAC, "[UE %d]: periodic BSR %d (SF), retx BSR %d (SF)\n",
          Mod_idP,
          UE_mac_inst[Mod_idP].scheduling_info.periodicBSR_SF,
          UE_mac_inst[Mod_idP].scheduling_info.retxBSR_SF);
    UE_mac_inst[Mod_idP].scheduling_info.drx_config =
      mac_MainConfig->drx_Config;
    UE_mac_inst[Mod_idP].scheduling_info.phr_config =
      mac_MainConfig->phr_Config;

    if (mac_MainConfig->phr_Config) {
      UE_mac_inst[Mod_idP].PHR_state =
        mac_MainConfig->phr_Config->present;
      UE_mac_inst[Mod_idP].PHR_reconfigured = 1;
      UE_mac_inst[Mod_idP].scheduling_info.periodicPHR_Timer =
        mac_MainConfig->phr_Config->choice.setup.periodicPHR_Timer;
      UE_mac_inst[Mod_idP].scheduling_info.prohibitPHR_Timer =
        mac_MainConfig->phr_Config->choice.setup.prohibitPHR_Timer;
      UE_mac_inst[Mod_idP].scheduling_info.PathlossChange =
        mac_MainConfig->phr_Config->choice.setup.dl_PathlossChange;
    } else {
      UE_mac_inst[Mod_idP].PHR_reconfigured = 0;
      UE_mac_inst[Mod_idP].PHR_state =
        LTE_MAC_MainConfig__phr_Config_PR_setup;
      UE_mac_inst[Mod_idP].scheduling_info.periodicPHR_Timer =
        LTE_MAC_MainConfig__phr_Config__setup__periodicPHR_Timer_sf20;
      UE_mac_inst[Mod_idP].scheduling_info.prohibitPHR_Timer =
        LTE_MAC_MainConfig__phr_Config__setup__prohibitPHR_Timer_sf20;
      UE_mac_inst[Mod_idP].scheduling_info.PathlossChange =
        LTE_MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB1;
    }

    UE_mac_inst[Mod_idP].scheduling_info.periodicPHR_SF =
      get_sf_perioidicPHR_Timer(UE_mac_inst[Mod_idP].
                                scheduling_info.periodicPHR_Timer);
    UE_mac_inst[Mod_idP].scheduling_info.prohibitPHR_SF =
      get_sf_prohibitPHR_Timer(UE_mac_inst[Mod_idP].
                               scheduling_info.prohibitPHR_Timer);
    UE_mac_inst[Mod_idP].scheduling_info.PathlossChange_db =
      get_db_dl_PathlossChange(UE_mac_inst[Mod_idP].
                               scheduling_info.PathlossChange);
    UE_mac_inst[Mod_idP].PHR_reporting_active = 0;
    LOG_D(MAC,
          "[UE %d] config PHR (%d): periodic %d (SF) prohibit %d (SF)  pathlosschange %d (db) \n",
          Mod_idP,
          (mac_MainConfig->phr_Config) ? mac_MainConfig->
          phr_Config->present : -1,
          UE_mac_inst[Mod_idP].scheduling_info.periodicPHR_SF,
          UE_mac_inst[Mod_idP].scheduling_info.prohibitPHR_SF,
          UE_mac_inst[Mod_idP].scheduling_info.PathlossChange_db);
  }

  if (physicalConfigDedicated != NULL) {
    if(NFAPI_MODE !=  NFAPI_UE_STUB_PNF)
      phy_config_dedicated_ue(Mod_idP, 0, eNB_index,
                              physicalConfigDedicated);

    UE_mac_inst[Mod_idP].physicalConfigDedicated = physicalConfigDedicated; // for SR proc
  }

#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))

  if (sCellToAddMod_r10 != NULL) {
    phy_config_dedicated_scell_ue(Mod_idP, eNB_index,
                                  sCellToAddMod_r10, 1);
    UE_mac_inst[Mod_idP].physicalConfigDedicatedSCell_r10 = sCellToAddMod_r10->radioResourceConfigDedicatedSCell_r10->physicalConfigDedicatedSCell_r10; // using SCell index 0
  }

#endif

  if (measObj != NULL) {
    if (measObj[0] != NULL &&
        measObj[0]->measObject.present == LTE_MeasObjectToAddMod__measObject_PR_measObjectEUTRA &&
        measObj[0]->measObject.choice.measObjectEUTRA.cellsToAddModList != NULL) {
      UE_mac_inst[Mod_idP].n_adj_cells =
        measObj[0]->measObject.choice.
        measObjectEUTRA.cellsToAddModList->list.count;
      LOG_I(MAC, "Number of adjacent cells %d\n",
            UE_mac_inst[Mod_idP].n_adj_cells);

      for (i = 0; i < UE_mac_inst[Mod_idP].n_adj_cells; i++) {
        UE_mac_inst[Mod_idP].adj_cell_id[i] =
          measObj[0]->measObject.choice.
          measObjectEUTRA.cellsToAddModList->list.array[i]->
          physCellId;
        LOG_I(MAC, "Cell %d : Nid_cell %d\n", i,
              UE_mac_inst[Mod_idP].adj_cell_id[i]);
      }

      phy_config_meas_ue(Mod_idP, 0, eNB_index,
                         UE_mac_inst[Mod_idP].n_adj_cells,
                         UE_mac_inst[Mod_idP].adj_cell_id);
    }
  }

  if (mobilityControlInfo != NULL) {
    LOG_D(MAC, "[UE%d] MAC Reset procedure triggered by RRC eNB %d \n",
          Mod_idP, eNB_index);
    ue_mac_reset(Mod_idP, eNB_index);

    if (mobilityControlInfo->radioResourceConfigCommon.
        rach_ConfigCommon) {
      memcpy((void *)
             &UE_mac_inst[Mod_idP].radioResourceConfigCommon->
             rach_ConfigCommon,
             (void *) mobilityControlInfo->
             radioResourceConfigCommon.rach_ConfigCommon,
             sizeof(LTE_RACH_ConfigCommon_t));
    }

    memcpy((void *) &UE_mac_inst[Mod_idP].
           radioResourceConfigCommon->prach_Config.prach_ConfigInfo,
           (void *) mobilityControlInfo->
           radioResourceConfigCommon.prach_Config.prach_ConfigInfo,
           sizeof(LTE_PRACH_ConfigInfo_t));
    UE_mac_inst[Mod_idP].radioResourceConfigCommon->
    prach_Config.rootSequenceIndex =
      mobilityControlInfo->radioResourceConfigCommon.
      prach_Config.rootSequenceIndex;

    if (mobilityControlInfo->radioResourceConfigCommon.
        pdsch_ConfigCommon) {
      memcpy((void *)
             &UE_mac_inst[Mod_idP].radioResourceConfigCommon->
             pdsch_ConfigCommon,
             (void *) mobilityControlInfo->
             radioResourceConfigCommon.pdsch_ConfigCommon,
             sizeof(LTE_PDSCH_ConfigCommon_t));
    }

    // not a pointer: mobilityControlInfo->radioResourceConfigCommon.pusch_ConfigCommon
    memcpy((void *) &UE_mac_inst[Mod_idP].
           radioResourceConfigCommon->pusch_ConfigCommon,
           (void *) &mobilityControlInfo->
           radioResourceConfigCommon.pusch_ConfigCommon,
           sizeof(LTE_PUSCH_ConfigCommon_t));

    if (mobilityControlInfo->radioResourceConfigCommon.phich_Config) {
      /* memcpy((void *)&UE_mac_inst[Mod_idP].radioResourceConfigCommon->phich_Config,
      (void *)mobilityControlInfo->radioResourceConfigCommon.phich_Config,
      sizeof(LTE_PHICH_Config_t)); */
    }

    if (mobilityControlInfo->radioResourceConfigCommon.
        pucch_ConfigCommon) {
      memcpy((void *)
             &UE_mac_inst[Mod_idP].radioResourceConfigCommon->
             pucch_ConfigCommon,
             (void *) mobilityControlInfo->
             radioResourceConfigCommon.pucch_ConfigCommon,
             sizeof(LTE_PUCCH_ConfigCommon_t));
    }

    if (mobilityControlInfo->
        radioResourceConfigCommon.soundingRS_UL_ConfigCommon) {
      memcpy((void *)
             &UE_mac_inst[Mod_idP].radioResourceConfigCommon->
             soundingRS_UL_ConfigCommon,
             (void *) mobilityControlInfo->
             radioResourceConfigCommon.soundingRS_UL_ConfigCommon,
             sizeof(LTE_SoundingRS_UL_ConfigCommon_t));
    }

    if (mobilityControlInfo->
        radioResourceConfigCommon.uplinkPowerControlCommon) {
      memcpy((void *)
             &UE_mac_inst[Mod_idP].radioResourceConfigCommon->
             uplinkPowerControlCommon,
             (void *) mobilityControlInfo->
             radioResourceConfigCommon.uplinkPowerControlCommon,
             sizeof(LTE_UplinkPowerControlCommon_t));
    }

    //configure antennaInfoCommon somewhere here..
    if (mobilityControlInfo->radioResourceConfigCommon.p_Max) {
      //to be configured
    }

    if (mobilityControlInfo->radioResourceConfigCommon.tdd_Config) {
      UE_mac_inst[Mod_idP].tdd_Config =
        mobilityControlInfo->radioResourceConfigCommon.tdd_Config;
    }

    if (mobilityControlInfo->
        radioResourceConfigCommon.ul_CyclicPrefixLength) {
      memcpy((void *)
             &UE_mac_inst[Mod_idP].radioResourceConfigCommon->
             ul_CyclicPrefixLength,
             (void *) mobilityControlInfo->
             radioResourceConfigCommon.ul_CyclicPrefixLength,
             sizeof(LTE_UL_CyclicPrefixLength_t));
    }

    // store the previous rnti in case of failure, and set thenew rnti
    UE_mac_inst[Mod_idP].crnti_before_ho = UE_mac_inst[Mod_idP].crnti;
    UE_mac_inst[Mod_idP].crnti =
      ((mobilityControlInfo->
        newUE_Identity.buf[0]) | (mobilityControlInfo->
                                  newUE_Identity.buf[1] << 8));
    LOG_I(MAC, "[UE %d] Received new identity %x from %d\n", Mod_idP,
          UE_mac_inst[Mod_idP].crnti, eNB_index);
    UE_mac_inst[Mod_idP].rach_ConfigDedicated =
      malloc(sizeof(*mobilityControlInfo->rach_ConfigDedicated));

    if (mobilityControlInfo->rach_ConfigDedicated) {
      memcpy((void *) UE_mac_inst[Mod_idP].rach_ConfigDedicated,
             (void *) mobilityControlInfo->rach_ConfigDedicated,
             sizeof(*mobilityControlInfo->rach_ConfigDedicated));
    }

    phy_config_afterHO_ue(Mod_idP, 0, eNB_index, mobilityControlInfo,
                          0);
  }

  if (mbsfn_SubframeConfigList != NULL) {
    phy_config_mbsfn_list_ue(Mod_idP, 0,
                       mbsfn_SubframeConfigList);

    LOG_I(MAC,
          "[UE %d][CONFIG] Received %d subframe allocation pattern for MBSFN\n",
          Mod_idP, mbsfn_SubframeConfigList->list.count);
    UE_mac_inst[Mod_idP].num_sf_allocation_pattern =
      mbsfn_SubframeConfigList->list.count;

    for (i = 0; i < mbsfn_SubframeConfigList->list.count; i++) {
      LOG_I(MAC,
            "[UE %d] Configuring MBSFN_SubframeConfig %d from received SIB2 \n",
            Mod_idP, i);
      UE_mac_inst[Mod_idP].mbsfn_SubframeConfig[i] =
        mbsfn_SubframeConfigList->list.array[i];
      //  LOG_I("[UE %d] MBSFN_SubframeConfig[%d] pattern is  %ld\n", Mod_idP,
      //    UE_mac_inst[Mod_idP].mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[0]);
    }
  }

#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))

  if (mbsfn_AreaInfoList != NULL) {
    LOG_I(MAC, "[UE %d][CONFIG] Received %d MBSFN Area Info\n",
          Mod_idP, mbsfn_AreaInfoList->list.count);
    UE_mac_inst[Mod_idP].num_active_mbsfn_area =
      mbsfn_AreaInfoList->list.count;

    for (i = 0; i < mbsfn_AreaInfoList->list.count; i++) {
      UE_mac_inst[Mod_idP].mbsfn_AreaInfo[i] =
        mbsfn_AreaInfoList->list.array[i];
      LOG_I(MAC,
            "[UE %d] MBSFN_AreaInfo[%d]: MCCH Repetition Period = %ld\n",
            Mod_idP, i,
            UE_mac_inst[Mod_idP].mbsfn_AreaInfo[i]->
            mcch_Config_r9.mcch_RepetitionPeriod_r9);
      phy_config_sib13_ue(Mod_idP, 0, eNB_index, i,
                          UE_mac_inst[Mod_idP].
                          mbsfn_AreaInfo[i]->mbsfn_AreaId_r9);
    }
  }

  if (pmch_InfoList != NULL) {
    //    LOG_I(MAC,"DUY: lcid when entering rrc_mac config_req is %02d\n",(pmch_InfoList->list.array[0]->mbms_SessionInfoList_r9.list.array[0]->logicalChannelIdentity_r9));
    LOG_I(MAC, "[UE %d] Configuring PMCH_config from MCCH MESSAGE \n",
          Mod_idP);

    for (i = 0; i < pmch_InfoList->list.count; i++) {
      UE_mac_inst[Mod_idP].pmch_Config[i] =
        &pmch_InfoList->list.array[i]->pmch_Config_r9;
      LOG_I(MAC, "[UE %d] PMCH[%d]: MCH_Scheduling_Period = %ld\n",
            Mod_idP, i,
            UE_mac_inst[Mod_idP].
            pmch_Config[i]->mch_SchedulingPeriod_r9);
    }

    UE_mac_inst[Mod_idP].mcch_status = 1;
  }

#endif

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  if(nonMBSFN_SubframeConfig!=NULL) {
    	LOG_I(MAC, "[UE %d] Configuring LTE_NonMBSFN \n",
	  Mod_idP);
	phy_config_sib1_fembms_ue(Mod_idP, CC_idP, 0, nonMBSFN_SubframeConfig);
  }
#endif

#ifdef CBA

  if (cba_rnti) {
    UE_mac_inst[Mod_idP].cba_rnti[num_active_cba_groups - 1] =
      cba_rnti;
    LOG_D(MAC,
          "[UE %d] configure CBA group %d RNTI %x for eNB %d (total active cba group %d)\n",
          Mod_idP, Mod_idP % num_active_cba_groups, cba_rnti,
          eNB_index, num_active_cba_groups);
    phy_config_cba_rnti(Mod_idP, CC_idP, eNB_flagP, eNB_index,
                        cba_rnti, num_active_cba_groups - 1,
                        num_active_cba_groups);
  }

#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_OUT);
  //for D2D
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))

  switch (config_action) {
    case CONFIG_ACTION_ADD:
      if (sourceL2Id) {
        UE_mac_inst[Mod_idP].sourceL2Id = *sourceL2Id;
        LOG_I(MAC,"[UE %d] Configure source L2Id 0x%08x \n", Mod_idP, *sourceL2Id );
      }

      if (destinationL2Id) {
        LOG_I(MAC,"[UE %d] Configure destination L2Id 0x%08x\n", Mod_idP, *destinationL2Id );
        int j = 0;
        int i = 0;

        for (i=0; i< MAX_NUM_DEST; i++) {
          if ((UE_mac_inst[Mod_idP].destinationList[i] == 0) && (j == 0)) j = i+1;

          if (UE_mac_inst[Mod_idP].destinationList[i] == *destinationL2Id) break; //destination already exists!
        }

        if ((i == MAX_NUM_DEST) && (j > 0))  UE_mac_inst[Mod_idP].destinationList[j-1] = *destinationL2Id;

        UE_mac_inst[Mod_idP].numCommFlows++;
      }

      break;

    case CONFIG_ACTION_REMOVE:
      //TODO
      break;

    default:
      break;
  }

#endif
  return (0);
}
