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
#include "SystemInformationBlockType2.h"
//#include "RadioResourceConfigCommonSIB.h"
#include "RadioResourceConfigDedicated.h"
#include "MeasGapConfig.h"
#include "MeasObjectToAddModList.h"
#include "TDD-Config.h"
#include "defs.h"
#include "proto.h"
#include "extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#if defined(Rel10) || defined(Rel14)
#include "MBSFN-AreaInfoList-r9.h"
#include "MBSFN-AreaInfo-r9.h"
#include "MBSFN-SubframeConfigList.h"
#include "PMCH-InfoList-r9.h"
#endif

/* sec 5.9, 36.321: MAC Reset Procedure */
void ue_mac_reset(module_id_t module_idP,uint8_t eNB_index)
{

  //Resetting Bj
  UE_mac_inst[module_idP].scheduling_info.Bj[0] = 0;
  UE_mac_inst[module_idP].scheduling_info.Bj[1] = 0;
  UE_mac_inst[module_idP].scheduling_info.Bj[2] = 0;

  //Stopping all timers

  //timeAlignmentTimer expires

  // PHY changes for UE MAC reset
  mac_xface->phy_reset_ue(module_idP,0,eNB_index);

  // notify RRC to relase PUCCH/SRS
  // cancel all pending SRs
  UE_mac_inst[module_idP].scheduling_info.SR_pending=0;
  UE_mac_inst[module_idP].scheduling_info.SR_COUNTER=0;

//Set BSR Trigger Bmp and remove timer flags
  UE_mac_inst[module_idP].BSR_reporting_active = BSR_TRIGGER_NONE;

  // stop ongoing RACH procedure

  // discard explicitly signaled ra_PreambleIndex and ra_RACH_MaskIndex, if any
  UE_mac_inst[module_idP].RA_prach_resources.ra_PreambleIndex  = 0; // check!
  UE_mac_inst[module_idP].RA_prach_resources.ra_RACH_MaskIndex = 0;


  ue_init_mac(module_idP); //This will hopefully do the rest of the MAC reset procedure

}

int
rrc_mac_config_req(
  module_id_t                      Mod_idP,
  int                              CC_idP,
  eNB_flag_t                       eNB_flagP,
  rnti_t                           rntiP,
  uint8_t                          eNB_index,
                       RadioResourceConfigCommonSIB_t  *radioResourceConfigCommon,
                       struct PhysicalConfigDedicated  *physicalConfigDedicated,
#if defined(Rel10) || defined(Rel14)
                       SCellToAddMod_r10_t *sCellToAddMod_r10,
                       //struct PhysicalConfigDedicatedSCell_r10 *physicalConfigDedicatedSCell_r10,
#endif
                       MeasObjectToAddMod_t           **measObj,
                       MAC_MainConfig_t                *mac_MainConfig,
                       long                             logicalChannelIdentity,
                       LogicalChannelConfig_t          *logicalChannelConfig,
                       MeasGapConfig_t                 *measGapConfig,
                       TDD_Config_t                    *tdd_Config,
                       MobilityControlInfo_t           *mobilityControlInfo,
                       uint8_t                              *SIwindowsize,
                       uint16_t                             *SIperiod,
                       ARFCN_ValueEUTRA_t              *ul_CarrierFreq,
                       long                            *ul_Bandwidth,
                       AdditionalSpectrumEmission_t    *additionalSpectrumEmission,
                       struct MBSFN_SubframeConfigList *mbsfn_SubframeConfigList
#if defined(Rel10) || defined(Rel14)
                       ,uint8_t                              MBMS_Flag,
                       MBSFN_AreaInfoList_r9_t         *mbsfn_AreaInfoList,
                       PMCH_InfoList_r9_t              *pmch_InfoList
#endif
#ifdef CBA
                       ,uint8_t                              num_active_cba_groups,
                       uint16_t                              cba_rnti
#endif
                      )
{

  int i;

  int UE_id = -1;
  eNB_MAC_INST *eNB = &eNB_mac_inst[Mod_idP];
  UE_list_t *UE_list= &eNB->UE_list;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_IN);

  if (eNB_flagP==0) {
    LOG_I(MAC,"[CONFIG][UE %d] Configuring MAC/PHY from eNB %d\n",Mod_idP,eNB_index);

    if (tdd_Config != NULL) {
      UE_mac_inst[Mod_idP].tdd_Config = tdd_Config;
    }
  } else {
    UE_id = find_UE_id(Mod_idP, rntiP);
    if (physicalConfigDedicated == NULL) {
      LOG_I(MAC,"[CONFIG][eNB %d/%d] Configuring MAC/PHY\n", Mod_idP, CC_idP);
    } else {
      LOG_I(MAC,"[CONFIG][eNB %d/%d] Configuring MAC/PHY for UE %d (%x)\n", Mod_idP, CC_idP, UE_id, UE_RNTI(Mod_idP, UE_id));
    }
  }

  if (tdd_Config && SIwindowsize && SIperiod) {
    if (eNB_flagP == ENB_FLAG_YES) {
      mac_xface->phy_config_sib1_eNB(Mod_idP, CC_idP, tdd_Config, *SIwindowsize, *SIperiod);
    } else {
      mac_xface->phy_config_sib1_ue(Mod_idP,0,eNB_index,tdd_Config,*SIwindowsize,*SIperiod);
    }
  }

  if (radioResourceConfigCommon!=NULL) {
    if (eNB_flagP==1) {
      LOG_I(MAC,"[CONFIG]SIB2/3 Contents (partial)\n");
      LOG_I(MAC,"[CONFIG]pusch_config_common.n_SB = %ld\n",radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.n_SB);
      LOG_I(MAC,"[CONFIG]pusch_config_common.hoppingMode = %ld\n",radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.hoppingMode);
      LOG_I(MAC,"[CONFIG]pusch_config_common.pusch_HoppingOffset = %ld\n",  radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.pusch_HoppingOffset);
      LOG_I(MAC,"[CONFIG]pusch_config_common.enable64QAM = %d\n",radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM);
      LOG_I(MAC,"[CONFIG]pusch_config_common.groupHoppingEnabled = %d\n",radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupHoppingEnabled);
      LOG_I(MAC,"[CONFIG]pusch_config_common.groupAssignmentPUSCH = %ld\n",radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH);
      LOG_I(MAC,"[CONFIG]pusch_config_common.sequenceHoppingEnabled = %d\n",radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled);
      LOG_I(MAC,"[CONFIG]pusch_config_common.cyclicShift  = %ld\n",radioResourceConfigCommon->pusch_ConfigCommon.ul_ReferenceSignalsPUSCH.cyclicShift);
      mac_xface->phy_config_sib2_eNB(Mod_idP, CC_idP, radioResourceConfigCommon, ul_CarrierFreq, ul_Bandwidth, additionalSpectrumEmission, mbsfn_SubframeConfigList);
    } else {
      UE_mac_inst[Mod_idP].radioResourceConfigCommon = radioResourceConfigCommon;
      mac_xface->phy_config_sib2_ue(Mod_idP,0,eNB_index,radioResourceConfigCommon,ul_CarrierFreq,ul_Bandwidth,additionalSpectrumEmission,mbsfn_SubframeConfigList);
    }
  }

  // SRB2_lchan_config->choice.explicitValue.ul_SpecificParameters->logicalChannelGroup
  if (logicalChannelConfig!= NULL) {
    if (eNB_flagP==0) {
      LOG_I(MAC,"[CONFIG][UE %d] Applying RRC logicalChannelConfig from eNB%d\n",Mod_idP,eNB_index);
      UE_mac_inst[Mod_idP].logicalChannelConfig[logicalChannelIdentity]=logicalChannelConfig;
      UE_mac_inst[Mod_idP].scheduling_info.Bj[logicalChannelIdentity]=0; // initilize the bucket for this lcid

      if (logicalChannelConfig->ul_SpecificParameters) {
        UE_mac_inst[Mod_idP].scheduling_info.bucket_size[logicalChannelIdentity]=logicalChannelConfig->ul_SpecificParameters->prioritisedBitRate *
            logicalChannelConfig->ul_SpecificParameters->bucketSizeDuration; // set the max bucket size
        if (logicalChannelConfig->ul_SpecificParameters->logicalChannelGroup != NULL) {
            UE_mac_inst[Mod_idP].scheduling_info.LCGID[logicalChannelIdentity]=*logicalChannelConfig->ul_SpecificParameters->logicalChannelGroup;
            LOG_D(MAC,"[CONFIG][UE %d] LCID %ld is attached to the LCGID %ld\n",Mod_idP,logicalChannelIdentity,*logicalChannelConfig->ul_SpecificParameters->logicalChannelGroup);
        }
        else {
        	UE_mac_inst[Mod_idP].scheduling_info.LCGID[logicalChannelIdentity] = MAX_NUM_LCGID;
        }
        UE_mac_inst[Mod_idP].scheduling_info.LCID_buffer_remain[logicalChannelIdentity] = 0;
      } else {
        LOG_E(MAC,"[CONFIG][UE %d] LCID %ld NULL ul_SpecificParameters\n",Mod_idP,logicalChannelIdentity);
        mac_xface->macphy_exit("NULL ul_SpecificParameters");
      }
    }
    else {
      if (UE_id == -1) {
        LOG_E(MAC,"%s:%d:%s: ERROR, UE_id == -1\n", __FILE__, __LINE__, __FUNCTION__);
      } else {
        if (logicalChannelConfig)
	  UE_list->UE_template[CC_idP][UE_id].lcgidmap[logicalChannelIdentity] = *logicalChannelConfig->ul_SpecificParameters->logicalChannelGroup;
        else
	  UE_list->UE_template[CC_idP][UE_id].lcgidmap[logicalChannelIdentity] = 0;
      }
    }
  }

  if (mac_MainConfig != NULL) {
    if (eNB_flagP==0) {
      LOG_I(MAC,"[CONFIG][UE%d] Applying RRC macMainConfig from eNB%d\n",Mod_idP,eNB_index);
      UE_mac_inst[Mod_idP].macConfig=mac_MainConfig;
      UE_mac_inst[Mod_idP].measGapConfig=measGapConfig;

      if (mac_MainConfig->ul_SCH_Config) {

        if (mac_MainConfig->ul_SCH_Config->periodicBSR_Timer) {
          UE_mac_inst[Mod_idP].scheduling_info.periodicBSR_Timer = (uint16_t) *mac_MainConfig->ul_SCH_Config->periodicBSR_Timer;
        } else {
          UE_mac_inst[Mod_idP].scheduling_info.periodicBSR_Timer = (uint16_t) PeriodicBSR_Timer_r12_infinity;
        }

        if (mac_MainConfig->ul_SCH_Config->maxHARQ_Tx) {
          UE_mac_inst[Mod_idP].scheduling_info.maxHARQ_Tx     = (uint16_t) *mac_MainConfig->ul_SCH_Config->maxHARQ_Tx;
        } else {
          UE_mac_inst[Mod_idP].scheduling_info.maxHARQ_Tx     = (uint16_t) MAC_MainConfig__ul_SCH_Config__maxHARQ_Tx_n5;
        }
        mac_xface->phy_config_harq_ue(Mod_idP,0,eNB_index,UE_mac_inst[Mod_idP].scheduling_info.maxHARQ_Tx);

        if (mac_MainConfig->ul_SCH_Config->retxBSR_Timer) {
          UE_mac_inst[Mod_idP].scheduling_info.retxBSR_Timer     = (uint16_t) mac_MainConfig->ul_SCH_Config->retxBSR_Timer;
        } else {
          UE_mac_inst[Mod_idP].scheduling_info.retxBSR_Timer     = (uint16_t)RetxBSR_Timer_r12_sf2560;
      }
      }

#if defined(Rel10) || defined(Rel14)

      if (mac_MainConfig->ext1 && mac_MainConfig->ext1->sr_ProhibitTimer_r9) {
        UE_mac_inst[Mod_idP].scheduling_info.sr_ProhibitTimer  = (uint16_t) *mac_MainConfig->ext1->sr_ProhibitTimer_r9;
      } else {
        UE_mac_inst[Mod_idP].scheduling_info.sr_ProhibitTimer  = 0;
      }

      if (mac_MainConfig->ext2 && mac_MainConfig->ext2->mac_MainConfig_v1020) {
        if (mac_MainConfig->ext2->mac_MainConfig_v1020->extendedBSR_Sizes_r10) {
          UE_mac_inst[Mod_idP].scheduling_info.extendedBSR_Sizes_r10 = (uint16_t) *mac_MainConfig->ext2->mac_MainConfig_v1020->extendedBSR_Sizes_r10;
        } else {
          UE_mac_inst[Mod_idP].scheduling_info.extendedBSR_Sizes_r10 = (uint16_t)0;
        }
        if (mac_MainConfig->ext2->mac_MainConfig_v1020->extendedPHR_r10) {
          UE_mac_inst[Mod_idP].scheduling_info.extendedPHR_r10 = (uint16_t) *mac_MainConfig->ext2->mac_MainConfig_v1020->extendedPHR_r10;
        } else {
          UE_mac_inst[Mod_idP].scheduling_info.extendedPHR_r10 = (uint16_t)0;
        }
      } else {
        UE_mac_inst[Mod_idP].scheduling_info.extendedBSR_Sizes_r10 = (uint16_t)0;
        UE_mac_inst[Mod_idP].scheduling_info.extendedPHR_r10 = (uint16_t)0;
      }
#endif
      UE_mac_inst[Mod_idP].scheduling_info.periodicBSR_SF  = MAC_UE_BSR_TIMER_NOT_RUNNING;
      UE_mac_inst[Mod_idP].scheduling_info.retxBSR_SF     = MAC_UE_BSR_TIMER_NOT_RUNNING;

       UE_mac_inst[Mod_idP].BSR_reporting_active = BSR_TRIGGER_NONE;

      LOG_D(MAC,"[UE %d]: periodic BSR %d (SF), retx BSR %d (SF)\n",
            Mod_idP,
            UE_mac_inst[Mod_idP].scheduling_info.periodicBSR_SF,
            UE_mac_inst[Mod_idP].scheduling_info.retxBSR_SF);

      UE_mac_inst[Mod_idP].scheduling_info.drx_config     = mac_MainConfig->drx_Config;
      UE_mac_inst[Mod_idP].scheduling_info.phr_config     = mac_MainConfig->phr_Config;

      if (mac_MainConfig->phr_Config) {
        UE_mac_inst[Mod_idP].PHR_state = mac_MainConfig->phr_Config->present;
        UE_mac_inst[Mod_idP].PHR_reconfigured = 1;
        UE_mac_inst[Mod_idP].scheduling_info.periodicPHR_Timer = mac_MainConfig->phr_Config->choice.setup.periodicPHR_Timer;
        UE_mac_inst[Mod_idP].scheduling_info.prohibitPHR_Timer = mac_MainConfig->phr_Config->choice.setup.prohibitPHR_Timer;
        UE_mac_inst[Mod_idP].scheduling_info.PathlossChange = mac_MainConfig->phr_Config->choice.setup.dl_PathlossChange;
      } else {
        UE_mac_inst[Mod_idP].PHR_reconfigured = 0;
        UE_mac_inst[Mod_idP].PHR_state = MAC_MainConfig__phr_Config_PR_setup;
        UE_mac_inst[Mod_idP].scheduling_info.periodicPHR_Timer = MAC_MainConfig__phr_Config__setup__periodicPHR_Timer_sf20;
        UE_mac_inst[Mod_idP].scheduling_info.prohibitPHR_Timer = MAC_MainConfig__phr_Config__setup__prohibitPHR_Timer_sf20;
        UE_mac_inst[Mod_idP].scheduling_info.PathlossChange = MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB1;
      }

      UE_mac_inst[Mod_idP].scheduling_info.periodicPHR_SF =  get_sf_perioidicPHR_Timer(UE_mac_inst[Mod_idP].scheduling_info.periodicPHR_Timer);
      UE_mac_inst[Mod_idP].scheduling_info.prohibitPHR_SF =  get_sf_prohibitPHR_Timer(UE_mac_inst[Mod_idP].scheduling_info.prohibitPHR_Timer);
      UE_mac_inst[Mod_idP].scheduling_info.PathlossChange_db =  get_db_dl_PathlossChange(UE_mac_inst[Mod_idP].scheduling_info.PathlossChange);
      UE_mac_inst[Mod_idP].PHR_reporting_active = 0;
      LOG_D(MAC,"[UE %d] config PHR (%d): periodic %d (SF) prohibit %d (SF)  pathlosschange %d (db) \n",
            Mod_idP,
            (mac_MainConfig->phr_Config)?mac_MainConfig->phr_Config->present:-1,
            UE_mac_inst[Mod_idP].scheduling_info.periodicPHR_SF,
            UE_mac_inst[Mod_idP].scheduling_info.prohibitPHR_SF,
            UE_mac_inst[Mod_idP].scheduling_info.PathlossChange_db);
    }
  }

  if (physicalConfigDedicated != NULL) {
    if (eNB_flagP==1) {
      if (UE_id == -1)
        LOG_E(MAC,"%s:%d:%s: ERROR, UE_id == -1\n", __FILE__, __LINE__, __FUNCTION__);
      else
        mac_xface->phy_config_dedicated_eNB(Mod_idP, CC_idP, UE_RNTI(Mod_idP, UE_id), physicalConfigDedicated);
    } else {
      mac_xface->phy_config_dedicated_ue(Mod_idP,0,eNB_index,physicalConfigDedicated);
      UE_mac_inst[Mod_idP].physicalConfigDedicated=physicalConfigDedicated; // for SR proc
    }
  }

#if defined(Rel10) || defined(Rel14)

  if (sCellToAddMod_r10 != NULL) {

    if (eNB_flagP==1) {
      if (UE_id == -1)
        LOG_E(MAC,"%s:%d:%s: ERROR, UE_id == -1\n", __FILE__, __LINE__, __FUNCTION__);
      else
        mac_xface->phy_config_dedicated_scell_eNB(Mod_idP,UE_RNTI(Mod_idP,UE_id),sCellToAddMod_r10,1);
    } else {

	//#warning "phy_config_dedicated_scell_ue is empty"
      mac_xface->phy_config_dedicated_scell_ue(Mod_idP,eNB_index,sCellToAddMod_r10,1);
      UE_mac_inst[Mod_idP].physicalConfigDedicatedSCell_r10 = sCellToAddMod_r10->radioResourceConfigDedicatedSCell_r10->physicalConfigDedicatedSCell_r10; // using SCell index 0
    }
  }

#endif

  if (eNB_flagP == 0) {
    if (measObj!= NULL) {
      if (measObj[0]!= NULL) {
        if (measObj[0]->measObject.choice.measObjectEUTRA.cellsToAddModList != NULL) {
          UE_mac_inst[Mod_idP].n_adj_cells = measObj[0]->measObject.choice.measObjectEUTRA.cellsToAddModList->list.count;
          LOG_D(MAC,"Number of adjacent cells %d\n",UE_mac_inst[Mod_idP].n_adj_cells);

          for (i=0; i<UE_mac_inst[Mod_idP].n_adj_cells; i++) {
            UE_mac_inst[Mod_idP].adj_cell_id[i] = measObj[0]->measObject.choice.measObjectEUTRA.cellsToAddModList->list.array[i]->physCellId;
            LOG_D(MAC,"Cell %d : Nid_cell %d\n",i,UE_mac_inst[Mod_idP].adj_cell_id[i]);
          }

          mac_xface->phy_config_meas_ue(Mod_idP,0,eNB_index,UE_mac_inst[Mod_idP].n_adj_cells,UE_mac_inst[Mod_idP].adj_cell_id);
          }
      }

      /*
      if (quantityConfig != NULL) {
      if (quantityConfig[0] != NULL) {
      UE_mac_inst[Mod_idP].quantityConfig = quantityConfig[0];
      LOG_I(MAC,"UE %d configured filterCoeff.",UE_mac_inst[Mod_idP].crnti);
      mac_xface->phy_config_meas_ue
      }
      }
       */
    }
  }

  if (eNB_flagP==0) {
    if(mobilityControlInfo != NULL) {

      LOG_D(MAC,"[UE%d] MAC Reset procedure triggered by RRC eNB %d \n",Mod_idP,eNB_index);
      ue_mac_reset(Mod_idP,eNB_index);

      if(mobilityControlInfo->radioResourceConfigCommon.rach_ConfigCommon) {
        memcpy((void *)&UE_mac_inst[Mod_idP].radioResourceConfigCommon->rach_ConfigCommon,
               (void *)mobilityControlInfo->radioResourceConfigCommon.rach_ConfigCommon,
               sizeof(RACH_ConfigCommon_t));
      }

      memcpy((void *)&UE_mac_inst[Mod_idP].radioResourceConfigCommon->prach_Config.prach_ConfigInfo,
             (void *)mobilityControlInfo->radioResourceConfigCommon.prach_Config.prach_ConfigInfo,
             sizeof(PRACH_ConfigInfo_t));
      UE_mac_inst[Mod_idP].radioResourceConfigCommon->prach_Config.rootSequenceIndex = mobilityControlInfo->radioResourceConfigCommon.prach_Config.rootSequenceIndex;

      if(mobilityControlInfo->radioResourceConfigCommon.pdsch_ConfigCommon) {
        memcpy((void *)&UE_mac_inst[Mod_idP].radioResourceConfigCommon->pdsch_ConfigCommon,
               (void *)mobilityControlInfo->radioResourceConfigCommon.pdsch_ConfigCommon,
               sizeof(PDSCH_ConfigCommon_t));
      }

      // not a pointer: mobilityControlInfo->radioResourceConfigCommon.pusch_ConfigCommon
      memcpy((void *)&UE_mac_inst[Mod_idP].radioResourceConfigCommon->pusch_ConfigCommon,
             (void *)&mobilityControlInfo->radioResourceConfigCommon.pusch_ConfigCommon,
             sizeof(PUSCH_ConfigCommon_t));

      if(mobilityControlInfo->radioResourceConfigCommon.phich_Config) {
        /* memcpy((void *)&UE_mac_inst[Mod_idP].radioResourceConfigCommon->phich_Config,
        (void *)mobilityControlInfo->radioResourceConfigCommon.phich_Config,
        sizeof(PHICH_Config_t)); */
      }

      if(mobilityControlInfo->radioResourceConfigCommon.pucch_ConfigCommon) {
        memcpy((void *)&UE_mac_inst[Mod_idP].radioResourceConfigCommon->pucch_ConfigCommon,
               (void *)mobilityControlInfo->radioResourceConfigCommon.pucch_ConfigCommon,
               sizeof(PUCCH_ConfigCommon_t));
      }

      if(mobilityControlInfo->radioResourceConfigCommon.soundingRS_UL_ConfigCommon) {
        memcpy((void *)&UE_mac_inst[Mod_idP].radioResourceConfigCommon->soundingRS_UL_ConfigCommon,
               (void *)mobilityControlInfo->radioResourceConfigCommon.soundingRS_UL_ConfigCommon,
               sizeof(SoundingRS_UL_ConfigCommon_t));
      }

      if(mobilityControlInfo->radioResourceConfigCommon.uplinkPowerControlCommon) {
        memcpy((void *)&UE_mac_inst[Mod_idP].radioResourceConfigCommon->uplinkPowerControlCommon,
               (void *)mobilityControlInfo->radioResourceConfigCommon.uplinkPowerControlCommon,
               sizeof(UplinkPowerControlCommon_t));
      }

      //configure antennaInfoCommon somewhere here..
      if(mobilityControlInfo->radioResourceConfigCommon.p_Max) {
        //to be configured
      }

      if(mobilityControlInfo->radioResourceConfigCommon.tdd_Config) {
        UE_mac_inst[Mod_idP].tdd_Config = mobilityControlInfo->radioResourceConfigCommon.tdd_Config;
      }

      if(mobilityControlInfo->radioResourceConfigCommon.ul_CyclicPrefixLength) {
        memcpy((void *)&UE_mac_inst[Mod_idP].radioResourceConfigCommon->ul_CyclicPrefixLength,
               (void *)mobilityControlInfo->radioResourceConfigCommon.ul_CyclicPrefixLength,
               sizeof(UL_CyclicPrefixLength_t));
      }

      // store the previous rnti in case of failure, and set thenew rnti
      UE_mac_inst[Mod_idP].crnti_before_ho = UE_mac_inst[Mod_idP].crnti;
      UE_mac_inst[Mod_idP].crnti = ((mobilityControlInfo->newUE_Identity.buf[0])|(mobilityControlInfo->newUE_Identity.buf[1]<<8));
      LOG_I(MAC,"[UE %d] Received new identity %x from %d\n", Mod_idP, UE_mac_inst[Mod_idP].crnti, eNB_index);
      UE_mac_inst[Mod_idP].rach_ConfigDedicated = malloc(sizeof(*mobilityControlInfo->rach_ConfigDedicated));

      if (mobilityControlInfo->rach_ConfigDedicated) {
        memcpy((void*)UE_mac_inst[Mod_idP].rach_ConfigDedicated,
               (void*)mobilityControlInfo->rach_ConfigDedicated,
               sizeof(*mobilityControlInfo->rach_ConfigDedicated));
      }

      mac_xface->phy_config_afterHO_ue(Mod_idP,0,eNB_index,mobilityControlInfo,0);
    }
  }

  if (mbsfn_SubframeConfigList != NULL) {
    if (eNB_flagP == 1) {
      LOG_I(MAC,"[eNB %d][CONFIG] Received %d subframe allocation pattern for MBSFN\n", Mod_idP, mbsfn_SubframeConfigList->list.count);
      eNB_mac_inst[Mod_idP].common_channels[0].num_sf_allocation_pattern= mbsfn_SubframeConfigList->list.count;

      for (i=0; i<mbsfn_SubframeConfigList->list.count; i++) {
        eNB_mac_inst[Mod_idP].common_channels[0].mbsfn_SubframeConfig[i] = mbsfn_SubframeConfigList->list.array[i];
        LOG_I(MAC, "[eNB %d][CONFIG] MBSFN_SubframeConfig[%d] pattern is  %x\n", Mod_idP, i,
              eNB_mac_inst[Mod_idP].common_channels[0].mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[0]);
      }

#if defined(Rel10) || defined(Rel14)
      eNB_mac_inst[Mod_idP].common_channels[0].MBMS_flag = MBMS_Flag;
#endif
    } else { // UE
      LOG_I(MAC,"[UE %d][CONFIG] Received %d subframe allocation pattern for MBSFN\n", Mod_idP, mbsfn_SubframeConfigList->list.count);
      UE_mac_inst[Mod_idP].num_sf_allocation_pattern= mbsfn_SubframeConfigList->list.count;

      for (i=0; i<mbsfn_SubframeConfigList->list.count; i++) {
        LOG_I(MAC, "[UE %d] Configuring MBSFN_SubframeConfig %d from received SIB2 \n", Mod_idP, i);
        UE_mac_inst[Mod_idP].mbsfn_SubframeConfig[i] = mbsfn_SubframeConfigList->list.array[i];
        //  LOG_I("[UE %d] MBSFN_SubframeConfig[%d] pattern is  %ld\n", Mod_idP,
        //    UE_mac_inst[Mod_idP].mbsfn_SubframeConfig[i]->subframeAllocation.choice.oneFrame.buf[0]);
      }
    }
  }

#if defined(Rel10) || defined(Rel14)

  if (mbsfn_AreaInfoList != NULL) {
    if (eNB_flagP == 1) {
      // One eNB could be part of multiple mbsfn syc area, this could change over time so reset each time
      LOG_I(MAC,"[eNB %d][CONFIG] Received %d MBSFN Area Info\n", Mod_idP, mbsfn_AreaInfoList->list.count);
      eNB_mac_inst[Mod_idP].common_channels[0].num_active_mbsfn_area = mbsfn_AreaInfoList->list.count;

      for (i =0; i< mbsfn_AreaInfoList->list.count; i++) {
        eNB_mac_inst[Mod_idP].common_channels[0].mbsfn_AreaInfo[i] = mbsfn_AreaInfoList->list.array[i];
        LOG_I(MAC,"[eNB %d][CONFIG] MBSFN_AreaInfo[%d]: MCCH Repetition Period = %ld\n", Mod_idP,i,
              eNB_mac_inst[Mod_idP].common_channels[0].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_RepetitionPeriod_r9);
        mac_xface->phy_config_sib13_eNB(Mod_idP,0,i,eNB_mac_inst[Mod_idP].common_channels[0].mbsfn_AreaInfo[i]->mbsfn_AreaId_r9);
      }
    } else { // UE
      LOG_I(MAC,"[UE %d][CONFIG] Received %d MBSFN Area Info\n", Mod_idP, mbsfn_AreaInfoList->list.count);
      UE_mac_inst[Mod_idP].num_active_mbsfn_area = mbsfn_AreaInfoList->list.count;

      for (i =0; i< mbsfn_AreaInfoList->list.count; i++) {
        UE_mac_inst[Mod_idP].mbsfn_AreaInfo[i] = mbsfn_AreaInfoList->list.array[i];
        LOG_I(MAC,"[UE %d] MBSFN_AreaInfo[%d]: MCCH Repetition Period = %ld\n",Mod_idP, i,
              UE_mac_inst[Mod_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_RepetitionPeriod_r9);
        mac_xface->phy_config_sib13_ue(Mod_idP,0,eNB_index,i,UE_mac_inst[Mod_idP].mbsfn_AreaInfo[i]->mbsfn_AreaId_r9);
      }
    }
  }


  if (pmch_InfoList != NULL) {

    //    LOG_I(MAC,"DUY: lcid when entering rrc_mac config_req is %02d\n",(pmch_InfoList->list.array[0]->mbms_SessionInfoList_r9.list.array[0]->logicalChannelIdentity_r9));

    if (eNB_flagP == 1) {

      LOG_I(MAC, "[CONFIG] Number of PMCH in this MBSFN Area %d\n", pmch_InfoList->list.count);

      for (i =0; i< pmch_InfoList->list.count; i++) {
        eNB_mac_inst[Mod_idP].common_channels[0].pmch_Config[i] = &pmch_InfoList->list.array[i]->pmch_Config_r9;

        LOG_I(MAC, "[CONFIG] PMCH[%d]: This PMCH stop (sf_AllocEnd_r9) at subframe  %ldth\n", i,
              eNB_mac_inst[Mod_idP].common_channels[0].pmch_Config[i]->sf_AllocEnd_r9);
        LOG_I(MAC, "[CONFIG] PMCH[%d]: mch_Scheduling_Period = %ld\n", i,
              eNB_mac_inst[Mod_idP].common_channels[0].pmch_Config[i]->mch_SchedulingPeriod_r9);
        LOG_I(MAC, "[CONFIG] PMCH[%d]: dataMCS = %ld\n", i,
              eNB_mac_inst[Mod_idP].common_channels[0].pmch_Config[i]->dataMCS_r9);

        // MBMS session info list in each MCH
        eNB_mac_inst[Mod_idP].common_channels[0].mbms_SessionList[i] = &pmch_InfoList->list.array[i]->mbms_SessionInfoList_r9;
        LOG_I(MAC, "PMCH[%d] Number of session (MTCH) is: %d\n",i, eNB_mac_inst[Mod_idP].common_channels[0].mbms_SessionList[i]->list.count);
      }
    } else { // UE
      LOG_I(MAC, "[UE %d] Configuring PMCH_config from MCCH MESSAGE \n",Mod_idP);

      for (i =0; i< pmch_InfoList->list.count; i++) {
        UE_mac_inst[Mod_idP].pmch_Config[i] = &pmch_InfoList->list.array[i]->pmch_Config_r9;
        LOG_I(MAC, "[UE %d] PMCH[%d]: MCH_Scheduling_Period = %ld\n", Mod_idP, i,
              UE_mac_inst[Mod_idP].pmch_Config[i]->mch_SchedulingPeriod_r9);
      }

      UE_mac_inst[Mod_idP].mcch_status = 1;
    }
  }

#endif
#ifdef CBA

  if (eNB_flagP == 0) {
    if (cba_rnti) {
      UE_mac_inst[Mod_idP].cba_rnti[num_active_cba_groups-1] = cba_rnti;
      LOG_D(MAC,"[UE %d] configure CBA group %d RNTI %x for eNB %d (total active cba group %d)\n",
            Mod_idP,Mod_idP%num_active_cba_groups, cba_rnti,eNB_index,num_active_cba_groups);
      mac_xface->phy_config_cba_rnti(Mod_idP,CC_idP,eNB_flagP,eNB_index,cba_rnti,num_active_cba_groups-1, num_active_cba_groups);
    }
  } else {
    if (cba_rnti) {
      LOG_D(MAC,"[eNB %d] configure CBA RNTI for UE  %d (total active cba groups %d)\n",
            Mod_idP, UE_id, num_active_cba_groups);
      eNB_mac_inst[Mod_idP].common_channels[CC_idP].num_active_cba_groups=num_active_cba_groups;

      for (i=0; i < num_active_cba_groups; i ++) {
        if (eNB_mac_inst[Mod_idP].common_channels[CC_idP].cba_rnti[i] != cba_rnti + i) {
          eNB_mac_inst[Mod_idP].common_channels[CC_idP].cba_rnti[i] = cba_rnti + i;
        }

        //only configure UE ids up to num_active_cba_groups
        //we use them as candidates for the transmission of dci format0)
        if (UE_id%num_active_cba_groups == i) {
          mac_xface->phy_config_cba_rnti(Mod_idP,CC_idP,eNB_flagP,UE_id,cba_rnti + i,i,num_active_cba_groups );
          LOG_D(MAC,"[eNB %d] configure CBA groups %d with RNTI %x for UE  %d (total active cba groups %d)\n",
                Mod_idP, i, eNB_mac_inst[Mod_idP].common_channels[CC_idP].cba_rnti[i],UE_id, num_active_cba_groups);
        }
      }
    }
  }

#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_OUT);

  return(0);
}
#ifdef LOCALIZATION
//------------------------------------------------------------------------------
double
rrc_get_estimated_ue_distance(
  const protocol_ctxt_t* const ctxt_pP,
  const int         CC_idP,
  const uint8_t     loc_typeP
)
//------------------------------------------------------------------------------
{
  // localization types:
  // 0: power based
  // 1: time based
  LTE_eNB_UE_stats     *eNB_UE_stats     = NULL;
  UE_list_t*            UE_list = &eNB_mac_inst[ctxt_pP->module_id].UE_list;
  int                   pCCid;
  int                   UE_id;

  if(ctxt_pP->rnti == NOT_A_RNTI) {
    return -1;
  }

  UE_id = find_UE_id(ctxt_pP->module_id,ctxt_pP->rnti);
  pCCid = UE_PCCID(ctxt_pP->module_id,UE_id);
  eNB_UE_stats = mac_xface->get_eNB_UE_stats(ctxt_pP->module_id,pCCid,ctxt_pP->rnti);

  switch (loc_typeP) {
  case 0:
    return eNB_UE_stats->distance.power_based;
    break;

  case 1:
    return eNB_UE_stats->distance.time_based;
    break;

  default:
    return  eNB_UE_stats->distance.power_based;
  }

  //    LOG_D(LOCALIZE, "DEBUG ME, dist = %d\n", &eNB_mac_inst[ctxt_pP->module_id].UE_list.UE_template[CC_idP][UE_id].distance.power_based);

}

#endif
