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

#include "PHY/defs_eNB.h"
#include "phy_init.h"
#include "SCHED/sched_eNB.h"
#include "PHY/phy_extern.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "SIMULATION/TOOLS/sim.h"
#include "LTE_RadioResourceConfigCommonSIB.h"
#include "LTE_RadioResourceConfigDedicated.h"
#include "LTE_TDD-Config.h"
#include "LTE_MBSFN-SubframeConfigList.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "assertions.h"
#include <math.h>
#include "nfapi/oai_integration/vendor_ext.h"
#include <openair1/PHY/LTE_ESTIMATION/lte_estimation.h>
extern uint32_t from_earfcn(int eutra_bandP,uint32_t dl_earfcn);
extern int32_t get_uldl_offset(int eutra_bandP);

extern uint16_t prach_root_sequence_map0_3[838];
extern uint16_t prach_root_sequence_map4[138];
uint8_t         dmrs1_tab[8] = { 0, 2, 3, 4, 6, 8, 9, 10 };


int             N_RB_DL_array[6] = { 6, 15, 25, 50, 75, 100 };

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

void pdsch_procedures(PHY_VARS_eNB *eNB,
                      L1_rxtx_proc_t *proc,
                      int harq_pid,
                      LTE_eNB_DLSCH_t *dlsch,
                      LTE_eNB_DLSCH_t *dlsch1);

void init_sss(void);

int
l1_north_init_eNB () {
  int i,j;

  if (RC.nb_L1_inst > 0 && RC.nb_L1_CC != NULL && RC.eNB != NULL) {
    AssertFatal(RC.nb_L1_inst>0,"nb_L1_inst=%d\n",RC.nb_L1_inst);
    AssertFatal(RC.nb_L1_CC!=NULL,"nb_L1_CC is null\n");
    AssertFatal(RC.eNB!=NULL,"RC.eNB is null\n");
    LOG_I(PHY,"%s() RC.nb_L1_inst:%d\n", __FUNCTION__, RC.nb_L1_inst);

    for (i=0; i<RC.nb_L1_inst; i++) {
      AssertFatal(RC.eNB[i]!=NULL,"RC.eNB[%d] is null\n",i);
      AssertFatal(RC.nb_L1_CC[i]>0,"RC.nb_L1_CC[%d]=%d\n",i,RC.nb_L1_CC[i]);
      LOG_I(PHY,"%s() RC.nb_L1_CC[%d]:%d\n", __FUNCTION__, i,  RC.nb_L1_CC[i]);

      for (j=0; j<RC.nb_L1_CC[i]; j++) {
        AssertFatal(RC.eNB[i][j]!=NULL,"RC.eNB[%d][%d] is null\n",i,j);

        if ((RC.eNB[i][j]->if_inst =  IF_Module_init(i))<0) return(-1);

        LOG_I(PHY,"%s() RC.eNB[%d][%d] installing callbacks\n", __FUNCTION__, i,  j);
        RC.eNB[i][j]->if_inst->PHY_config_req = phy_config_request;
        RC.eNB[i][j]->if_inst->schedule_response = schedule_response;
        RC.eNB[i][j]->if_inst->PHY_config_update_sib2_req = phy_config_update_sib2_request;
        RC.eNB[i][j]->if_inst->PHY_config_update_sib13_req = phy_config_update_sib13_request;
       
      }
    }
  } else {
    LOG_I(PHY,"%s() Not installing PHY callbacks - RC.nb_L1_inst:%d RC.nb_L1_CC:%p RC.eNB:%p\n", __FUNCTION__, RC.nb_L1_inst, RC.nb_L1_CC, RC.eNB);
  }

  return(0);
}


void phy_config_request(PHY_Config_t *phy_config) {
  uint8_t         Mod_id = phy_config->Mod_id;
  int             CC_id = phy_config->CC_id;
  nfapi_config_request_t *cfg = phy_config->cfg;
  LTE_DL_FRAME_PARMS *fp;
  PHICH_RESOURCE_t phich_resource_table[4]= {oneSixth,half,one,two};
  int                 eutra_band     = cfg->nfapi_config.rf_bands.rf_band[0];
  int                 dl_Bandwidth   = cfg->rf_config.dl_channel_bandwidth.value;
  int                 ul_Bandwidth   = cfg->rf_config.ul_channel_bandwidth.value;
  int                 Nid_cell       = cfg->sch_config.physical_cell_id.value;
  int                 Ncp            = cfg->subframe_config.dl_cyclic_prefix_type.value;
  int                 p_eNB          = cfg->rf_config.tx_antenna_ports.value;
  uint32_t            dl_CarrierFreq = cfg->nfapi_config.earfcn.value;
  LOG_A(PHY,"Configuring MIB for instance %d, CCid %d : (band %d,N_RB_DL %d, N_RB_UL %d, Nid_cell %d,eNB_tx_antenna_ports %d,Ncp %d,DL freq %u,phich_config.resource %d, phich_config.duration %d)\n",
        Mod_id, CC_id, eutra_band, dl_Bandwidth, ul_Bandwidth, Nid_cell, p_eNB,Ncp,dl_CarrierFreq,
        cfg->phich_config.phich_resource.value,
        cfg->phich_config.phich_duration.value);
  AssertFatal (RC.eNB != NULL, "PHY instance pointer doesn't exist\n");
  AssertFatal (RC.eNB[Mod_id] != NULL, "PHY instance %d doesn't exist\n", Mod_id);
  AssertFatal (RC.eNB[Mod_id][CC_id] != NULL, "PHY instance %d, CCid %d doesn't exist\n", Mod_id, CC_id);

  if (RC.eNB[Mod_id][CC_id]->configured == 1) {
    LOG_E(PHY,"Already eNB already configured, do nothing\n");
    return;
  }

  RC.eNB[Mod_id][CC_id]->mac_enabled = 1;
  fp = &RC.eNB[Mod_id][CC_id]->frame_parms;
  fp->N_RB_DL                            = dl_Bandwidth;
  fp->N_RB_UL                            = ul_Bandwidth;
  fp->Nid_cell                           = Nid_cell;
  fp->nushift                            = fp->Nid_cell%6;
  fp->eutra_band                         = eutra_band;
  fp->Ncp                                = Ncp;
  fp->Ncp_UL                             = Ncp;
  fp->nb_antenna_ports_eNB               = p_eNB;
  fp->threequarter_fs = 0;
  AssertFatal (cfg->phich_config.phich_resource.value < 4, "Illegal phich_Resource\n");
  fp->phich_config_common.phich_resource = phich_resource_table[cfg->phich_config.phich_resource.value];
  fp->phich_config_common.phich_duration = cfg->phich_config.phich_duration.value;
  // Note: "from_earfcn" has to be in a common library with MACRLC
  fp->dl_CarrierFreq = from_earfcn (eutra_band, dl_CarrierFreq);
  fp->ul_CarrierFreq = fp->dl_CarrierFreq - (get_uldl_offset (eutra_band) * 100000);
  fp->tdd_config = 0;
  fp->tdd_config_S = 0;

  if (fp->dl_CarrierFreq == fp->ul_CarrierFreq)
    fp->frame_type = TDD;
  else
    fp->frame_type = FDD;

  init_frame_parms (fp, 1);
  init_lte_top (fp);

  if (cfg->subframe_config.duplex_mode.value == 0) {
    fp->tdd_config = cfg->tdd_frame_structure_config.subframe_assignment.value;
    fp->tdd_config_S = cfg->tdd_frame_structure_config.special_subframe_patterns.value;
    fp->frame_type = TDD;
  } else {
    fp->frame_type = FDD;
  }

  fp->prach_config_common.rootSequenceIndex = cfg->prach_config.root_sequence_index.value;
  LOG_I (PHY, "prach_config_common.rootSequenceIndex = %d\n", cfg->prach_config.root_sequence_index.value);
  fp->prach_config_common.prach_Config_enabled = 1;
  fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex = cfg->prach_config.configuration_index.value;
  LOG_I (PHY, "prach_config_common.prach_ConfigInfo.prach_ConfigIndex = %d\n", cfg->prach_config.configuration_index.value);
  fp->prach_config_common.prach_ConfigInfo.highSpeedFlag = cfg->prach_config.high_speed_flag.value;
  LOG_I (PHY, "prach_config_common.prach_ConfigInfo.highSpeedFlag = %d\n", cfg->prach_config.high_speed_flag.value);
  fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig = cfg->prach_config.zero_correlation_zone_configuration.value;
  LOG_I (PHY, "prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig = %d\n", cfg->prach_config.zero_correlation_zone_configuration.value);
  fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset = cfg->prach_config.frequency_offset.value;
  LOG_I (PHY, "prach_config_common.prach_ConfigInfo.prach_FreqOffset = %d\n", cfg->prach_config.frequency_offset.value);
  init_prach_tables (839);
  compute_prach_seq (fp->prach_config_common.rootSequenceIndex,
                     fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
                     fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig, fp->prach_config_common.prach_ConfigInfo.highSpeedFlag, fp->frame_type, RC.eNB[Mod_id][CC_id]->X_u);

  if (cfg->emtc_config.prach_ce_level_0_enable.value == 1) {
    fp->prach_emtc_config_common.prach_Config_enabled = 1;
    fp->prach_emtc_config_common.rootSequenceIndex = cfg->emtc_config.prach_catm_root_sequence_index.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.highSpeedFlag = cfg->emtc_config.prach_catm_high_speed_flag.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig = cfg->emtc_config.prach_catm_zero_correlation_zone_configuration.value;
    // CE Level 3 parameters
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[3] = cfg->emtc_config.prach_ce_level_3_enable.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_starting_subframe_periodicity[3] = cfg->emtc_config.prach_ce_level_3_starting_subframe_periodicity.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[3] = cfg->emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt.value;
    AssertFatal (fp->prach_emtc_config_common.prach_ConfigInfo.prach_starting_subframe_periodicity[3] >= fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[3],
                 "prach_starting_subframe_periodicity[3] < prach_numPetitionPerPreambleAttempt[3]\n");
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[3] = cfg->emtc_config.prach_ce_level_3_configuration_index.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[3] = cfg->emtc_config.prach_ce_level_3_frequency_offset.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_hopping_enable[3] = cfg->emtc_config.prach_ce_level_3_hopping_enable.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_hopping_offset[3] = cfg->emtc_config.prach_ce_level_3_hopping_offset.value;

    if (fp->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[3] == 1)
      compute_prach_seq (fp->prach_emtc_config_common.rootSequenceIndex,
                         fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[3],
                         fp->prach_emtc_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
                         fp->prach_emtc_config_common.prach_ConfigInfo.highSpeedFlag, fp->frame_type, RC.eNB[Mod_id][CC_id]->X_u_br[3]);

    // CE Level 2 parameters
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[2] = cfg->emtc_config.prach_ce_level_2_enable.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_starting_subframe_periodicity[2] = cfg->emtc_config.prach_ce_level_2_starting_subframe_periodicity.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[2] = cfg->emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt.value;
    AssertFatal (fp->prach_emtc_config_common.prach_ConfigInfo.prach_starting_subframe_periodicity[2] >= fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[2],
                 "prach_starting_subframe_periodicity[2] < prach_numPetitionPerPreambleAttempt[2]\n");
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[2] = cfg->emtc_config.prach_ce_level_2_configuration_index.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[2] = cfg->emtc_config.prach_ce_level_2_frequency_offset.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_hopping_enable[2] = cfg->emtc_config.prach_ce_level_2_hopping_enable.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_hopping_offset[2] = cfg->emtc_config.prach_ce_level_2_hopping_offset.value;

    if (fp->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[2] == 1)
      compute_prach_seq (fp->prach_emtc_config_common.rootSequenceIndex,
                         fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[3],
                         fp->prach_emtc_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
                         fp->prach_emtc_config_common.prach_ConfigInfo.highSpeedFlag, fp->frame_type, RC.eNB[Mod_id][CC_id]->X_u_br[2]);

    // CE Level 1 parameters
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[1] = cfg->emtc_config.prach_ce_level_1_enable.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_starting_subframe_periodicity[1] = cfg->emtc_config.prach_ce_level_1_starting_subframe_periodicity.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[1] = cfg->emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt.value;
    AssertFatal (fp->prach_emtc_config_common.prach_ConfigInfo.prach_starting_subframe_periodicity[1] >= fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[1],
                 "prach_starting_subframe_periodicity[1] < prach_numPetitionPerPreambleAttempt[1]\n");
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[1] = cfg->emtc_config.prach_ce_level_1_configuration_index.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[1] = cfg->emtc_config.prach_ce_level_1_frequency_offset.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_hopping_enable[1] = cfg->emtc_config.prach_ce_level_1_hopping_enable.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_hopping_offset[1] = cfg->emtc_config.prach_ce_level_1_hopping_offset.value;

    if (fp->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[1] == 1)
      compute_prach_seq (fp->prach_emtc_config_common.rootSequenceIndex,
                         fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[3],
                         fp->prach_emtc_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
                         fp->prach_emtc_config_common.prach_ConfigInfo.highSpeedFlag, fp->frame_type, RC.eNB[Mod_id][CC_id]->X_u_br[1]);

    // CE Level 0 parameters
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[0] = cfg->emtc_config.prach_ce_level_0_enable.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_starting_subframe_periodicity[0] = cfg->emtc_config.prach_ce_level_0_starting_subframe_periodicity.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[0] = cfg->emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt.value;
    AssertFatal (fp->prach_emtc_config_common.prach_ConfigInfo.prach_starting_subframe_periodicity[0] >= fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[0],
                 "prach_starting_subframe_periodicity[0] %d < prach_numPetitionPerPreambleAttempt[0] %d\n",
                 fp->prach_emtc_config_common.prach_ConfigInfo.prach_starting_subframe_periodicity[0], fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[0]);
    AssertFatal (fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[0] > 0, "prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[0]==0\n");
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[0] = cfg->emtc_config.prach_ce_level_0_configuration_index.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[0] = cfg->emtc_config.prach_ce_level_0_frequency_offset.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_hopping_enable[0] = cfg->emtc_config.prach_ce_level_0_hopping_enable.value;
    fp->prach_emtc_config_common.prach_ConfigInfo.prach_hopping_offset[0] = cfg->emtc_config.prach_ce_level_0_hopping_offset.value;

    if (fp->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[0] == 1) {
      compute_prach_seq (fp->prach_emtc_config_common.rootSequenceIndex,
                         fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[3],
                         fp->prach_emtc_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
                         fp->prach_emtc_config_common.prach_ConfigInfo.highSpeedFlag, fp->frame_type, RC.eNB[Mod_id][CC_id]->X_u_br[0]);
      init_mpdcch(RC.eNB[Mod_id][CC_id]);
    }
  }

  fp->pucch_config_common.deltaPUCCH_Shift = 1 + cfg->pucch_config.delta_pucch_shift.value;
  fp->pucch_config_common.nRB_CQI = cfg->pucch_config.n_cqi_rb.value;
  fp->pucch_config_common.nCS_AN = cfg->pucch_config.n_an_cs.value;
  fp->pucch_config_common.n1PUCCH_AN = cfg->pucch_config.n1_pucch_an.value;
  fp->pdsch_config_common.referenceSignalPower = cfg->rf_config.reference_signal_power.value;
  fp->pdsch_config_common.p_b = cfg->subframe_config.pb.value;
  fp->pusch_config_common.n_SB = cfg->pusch_config.number_of_subbands.value;
  LOG_I (PHY, "pusch_config_common.n_SB = %d\n", fp->pusch_config_common.n_SB);
  fp->pusch_config_common.hoppingMode = cfg->pusch_config.hopping_mode.value;
  LOG_I (PHY, "pusch_config_common.hoppingMode = %d\n", fp->pusch_config_common.hoppingMode);
  fp->pusch_config_common.pusch_HoppingOffset = cfg->pusch_config.hopping_offset.value;
  LOG_I (PHY, "pusch_config_common.pusch_HoppingOffset = %d\n", fp->pusch_config_common.pusch_HoppingOffset);
  fp->pusch_config_common.enable64QAM                                  = 0;//radioResourceConfigCommon->pusch_ConfigCommon.pusch_ConfigBasic.enable64QAM;
  LOG_I(PHY,"pusch_config_common.enable64QAM = %d\n",fp->pusch_config_common.enable64QAM );
  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled     = 0;
  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled  = 0;

  if (cfg->uplink_reference_signal_config.uplink_rs_hopping.value == 1)
    fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled = 1;

  if (cfg->uplink_reference_signal_config.uplink_rs_hopping.value == 2)
    fp->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = 1;

  LOG_I(PHY,"pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled = %d\n",fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled);
  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH   =  cfg->uplink_reference_signal_config.group_assignment.value;
  LOG_I(PHY,"pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH = %d\n",fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH);
  LOG_I (PHY, "pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = %d\n", fp->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled);
  fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift = dmrs1_tab[cfg->uplink_reference_signal_config.cyclic_shift_1_for_drms.value];
  LOG_I (PHY, "pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift = %d\n", fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift);
  init_ul_hopping (fp);
  fp->soundingrs_ul_config_common.enabled_flag = 0;     // 1; Don't know how to turn this off in NFAPI
  fp->soundingrs_ul_config_common.srs_BandwidthConfig = cfg->srs_config.bandwidth_configuration.value;
  fp->soundingrs_ul_config_common.srs_SubframeConfig = cfg->srs_config.srs_subframe_configuration.value;
  fp->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission = cfg->srs_config.srs_acknack_srs_simultaneous_transmission.value;
  fp->soundingrs_ul_config_common.srs_MaxUpPts = cfg->srs_config.max_up_pts.value;
  fp->num_MBSFN_config = 0;
  fp->NonMBSFN_config_flag =cfg->fembms_config.non_mbsfn_config_flag.value;
  fp->FeMBMS_active = fp->NonMBSFN_config_flag;
  fp->NonMBSFN_config.non_mbsfn_SubframeConfig = cfg->fembms_config.non_mbsfn_subframeconfig.value;
  fp->NonMBSFN_config.radioframeAllocationPeriod = cfg->fembms_config.radioframe_allocation_period.value;
  fp->NonMBSFN_config.radioframeAllocationOffset = cfg->fembms_config.radioframe_allocation_offset.value;
  LOG_D(PHY,"eNB %d/%d frame NonMBSFN configured: %d (%x) %d/%d\n",Mod_id,CC_id,fp->NonMBSFN_config_flag,fp->NonMBSFN_config.non_mbsfn_SubframeConfig,fp->NonMBSFN_config.radioframeAllocationPeriod,
        fp->NonMBSFN_config.radioframeAllocationOffset);
  init_ncs_cell (fp, RC.eNB[Mod_id][CC_id]->ncs_cell);
  init_ul_hopping (fp);
  RC.eNB[Mod_id][CC_id]->configured = 1;
  LOG_I (PHY, "eNB %d/%d configured\n", Mod_id, CC_id);
}

void phy_config_update_sib2_request(PHY_Config_t *phy_config) {

  uint8_t         Mod_id = phy_config->Mod_id;
  int             CC_id = phy_config->CC_id;
  int i;
  nfapi_config_request_t *cfg = phy_config->cfg;
  LOG_I(PHY,"Configure sib2 Mod_id(%d), CC_id(%d) cfg %p\n",Mod_id,CC_id,cfg);

  LTE_DL_FRAME_PARMS *fp = &RC.eNB[Mod_id][CC_id]->frame_parms;

  fp->num_MBSFN_config =  cfg->embms_mbsfn_config.num_mbsfn_config;
  for( i=0; i < cfg->embms_mbsfn_config.num_mbsfn_config; i++){
      fp->MBSFN_config[i].radioframeAllocationPeriod =  cfg->embms_mbsfn_config.radioframe_allocation_period[i];
      fp->MBSFN_config[i].radioframeAllocationOffset =  cfg->embms_mbsfn_config.radioframe_allocation_offset[i];
      fp->MBSFN_config[i].fourFrames_flag =            cfg->embms_mbsfn_config.fourframes_flag[i];
      fp->MBSFN_config[i].mbsfn_SubframeConfig =       cfg->embms_mbsfn_config.mbsfn_subframeconfig[i];  // 6-bit subframe configuration
      LOG_I(PHY, "[CONFIG] MBSFN_SubframeConfig[%d] pattern is  %x\n", i,
              fp->MBSFN_config[i].mbsfn_SubframeConfig);
  }



}
void phy_config_update_sib13_request(PHY_Config_t *phy_config) {

  uint8_t         Mod_id = phy_config->Mod_id;
  int             CC_id = phy_config->CC_id;
  nfapi_config_request_t *cfg = phy_config->cfg;

  LOG_I(PHY,"configure sib3 Mod_id(%d), CC_id(%d) cfg %p\n",Mod_id,CC_id,cfg);
  LTE_DL_FRAME_PARMS *fp = &RC.eNB[Mod_id][CC_id]->frame_parms;
  LOG_I (PHY, "[eNB%d] Applying MBSFN_Area_id %d for index %d\n", Mod_id, (uint16_t)cfg->embms_sib13_config.mbsfn_area_id_r9.value, (uint8_t)cfg->embms_sib13_config.mbsfn_area_idx.value);

  //cfg->embms_sib13_config.mbsfn_area_idx;
  //cfg->embms_sib13_config.mbsfn_area_id_r9;

  AssertFatal((uint8_t)cfg->embms_sib13_config.mbsfn_area_idx.value == 0, "Fix me: only called when mbsfn_Area_idx == 0\n");
  if (cfg->embms_sib13_config.mbsfn_area_idx.value == 0) {
    fp->Nid_cell_mbsfn = (uint16_t)cfg->embms_sib13_config.mbsfn_area_id_r9.value;
    LOG_I(PHY,"Fix me: only called when mbsfn_Area_idx == 0)\n");
  }
  lte_gold_mbsfn (fp, RC.eNB[Mod_id][CC_id]->lte_gold_mbsfn_table, fp->Nid_cell_mbsfn);

  lte_gold_mbsfn_khz_1dot25 (fp, RC.eNB[Mod_id][CC_id]->lte_gold_mbsfn_khz_1dot25_table, fp->Nid_cell_mbsfn);
}

void phy_config_sib13_eNB(module_id_t Mod_id,int CC_id,int mbsfn_Area_idx,
                          long mbsfn_AreaId_r9) {
  LTE_DL_FRAME_PARMS *fp = &RC.eNB[Mod_id][CC_id]->frame_parms;
  LOG_I (PHY, "[eNB%d] Applying MBSFN_Area_id %ld for index %d\n", Mod_id, mbsfn_AreaId_r9, mbsfn_Area_idx);

  if (mbsfn_Area_idx == 0) {
    fp->Nid_cell_mbsfn = (uint16_t)mbsfn_AreaId_r9;
    LOG_I(PHY,"Fix me: only called when mbsfn_Area_idx == 0)\n");
  }

  lte_gold_mbsfn (fp, RC.eNB[Mod_id][CC_id]->lte_gold_mbsfn_table, fp->Nid_cell_mbsfn);
  lte_gold_mbsfn_khz_1dot25 (fp, RC.eNB[Mod_id][CC_id]->lte_gold_mbsfn_khz_1dot25_table, fp->Nid_cell_mbsfn);
}

void fill_subframe_mask(PHY_VARS_eNB *eNB) {

  AssertFatal(eNB->subframe_mask!=NULL,"Subframe mask is null\n");
  int32_t **dummy_tx=eNB->common_vars.txdataF;
  eNB->common_vars.txdataF = eNB->subframe_mask;
  
  for (int sf=0;sf<10;sf++) {
    if (subframe_select(&eNB->frame_parms,sf)==SF_DL) {
      LTE_DL_eNB_HARQ_t *dlsch_harq=eNB->dlsch[0][0]->harq_processes[0];
      L1_rxtx_proc_t proc;
      proc.frame_tx=0;
      proc.subframe_tx=sf;
      eNB->dlsch[0][0]->harq_ids[0][sf]=0;
      eNB->dlsch[0][0]->rnti=0x1234;
      dlsch_harq->nb_rb=48;
      dlsch_harq->rb_alloc[0]=(uint32_t)0xffffffff;
      dlsch_harq->Qm=2;
      dlsch_harq->Nl=1;
      dlsch_harq->pdsch_start=1;
      dlsch_harq->mimo_mode=(eNB->frame_parms.nb_antenna_ports_eNB>1) ? ALAMOUTI : SISO;
      dlsch_harq->dl_power_off = 1;
      computeRhoB_eNB(4,eNB->frame_parms.pdsch_config_common.p_b,eNB->frame_parms.nb_antenna_ports_eNB,eNB->dlsch[0][0],dlsch_harq->dl_power_off);
      pdsch_procedures(eNB,&proc,0,eNB->dlsch[0][0],NULL);
    }
  }
  eNB->common_vars.txdataF = dummy_tx;
}

int phy_init_lte_eNB(PHY_VARS_eNB *eNB,
                     unsigned char is_secondary_eNB,
                     unsigned char abstraction_flag) {
  // shortcuts
  LTE_DL_FRAME_PARMS *const fp       = &eNB->frame_parms;
  LTE_eNB_COMMON *const common_vars  = &eNB->common_vars;
  LTE_eNB_PUSCH **const pusch_vars   = eNB->pusch_vars;
  LTE_eNB_SRS *const srs_vars        = eNB->srs_vars;
  LTE_eNB_PRACH *const prach_vars    = &eNB->prach_vars;
  LTE_eNB_PRACH *const prach_vars_br = &eNB->prach_vars_br;
  int             i, UE_id;
  LOG_I(PHY,"[eNB %d] %s() About to wait for eNB to be configured", eNB->Mod_id, __FUNCTION__);
  eNB->total_dlsch_bitrate = 0;
  eNB->total_transmitted_bits = 0;
  eNB->total_system_throughput = 0;
  eNB->check_for_MUMIMO_transmissions = 0;
  LOG_I(PHY,
        "[eNB %"PRIu8"] Initializing DL_FRAME_PARMS : N_RB_DL %"PRIu8", PHICH Resource %d, PHICH Duration %d nb_antennas_tx:%u nb_antennas_rx:%u nb_antenna_ports_eNB:%u PRACH[rootSequenceIndex:%u prach_Config_enabled:%u configIndex:%u highSpeed:%u zeroCorrelationZoneConfig:%u freqOffset:%u]\n",
        eNB->Mod_id,
        fp->N_RB_DL,fp->phich_config_common.phich_resource,
        fp->phich_config_common.phich_duration,
        fp->nb_antennas_tx, fp->nb_antennas_rx, fp->nb_antenna_ports_eNB,
        fp->prach_config_common.rootSequenceIndex,
        fp->prach_config_common.prach_Config_enabled,
        fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
        fp->prach_config_common.prach_ConfigInfo.highSpeedFlag,
        fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
        fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset
       );
  LOG_D(PHY,"[MSC_NEW][FRAME 00000][PHY_eNB][MOD %02"PRIu8"][]\n", eNB->Mod_id);
  LOG_I (PHY, "[eNB %" PRIu8 "] Initializing DL_FRAME_PARMS : N_RB_DL %" PRIu8 ", PHICH Resource %d, PHICH Duration %d\n",
         eNB->Mod_id, fp->N_RB_DL, fp->phich_config_common.phich_resource, fp->phich_config_common.phich_duration);
  LOG_D (PHY, "[MSC_NEW][FRAME 00000][PHY_eNB][MOD %02" PRIu8 "][]\n", eNB->Mod_id);
  crcTableInit();
  lte_gold (fp, eNB->lte_gold_table, fp->Nid_cell);
  generate_pcfich_reg_mapping (fp);
  generate_phich_reg_mapping (fp);
  init_sss();

  init_fde();
  for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
    eNB->first_run_timing_advance[UE_id] =
      1;   ///This flag used to be static. With multiple eNBs this does no longer work, hence we put it in the structure. However it has to be initialized with 1, which is performed here.
    // clear whole structure
    bzero (&eNB->UE_stats[UE_id], sizeof (LTE_eNB_UE_stats));
    eNB->physicalConfigDedicated[UE_id] = NULL;
  }

  eNB->first_run_I0_measurements =
    1; ///This flag used to be static. With multiple eNBs this does no longer work, hence we put it in the structure. However it has to be initialized with 1, which is performed here.

  eNB->use_DTX=1;
  if (NFAPI_MODE!=NFAPI_MODE_VNF) {
    common_vars->rxdata  = (int32_t **)NULL;
    common_vars->txdataF = (int32_t **)malloc16(NB_ANTENNA_PORTS_ENB*sizeof(int32_t *));
    if (eNB->use_DTX==0) {
      eNB->subframe_mask = (int32_t **)malloc16(NB_ANTENNA_PORTS_ENB*sizeof(int32_t *));
    }
    common_vars->rxdataF = (int32_t **)malloc16(64*sizeof(int32_t *));
    LOG_I(PHY,"[INIT] NB_ANTENNA_PORTS_ENB:%d fp->nb_antenna_ports_eNB:%d\n", NB_ANTENNA_PORTS_ENB, fp->nb_antenna_ports_eNB);

    for (i=0; i<NB_ANTENNA_PORTS_ENB; i++) {
      if (i<fp->nb_antenna_ports_eNB || i==5) {
        common_vars->txdataF[i] = (int32_t *)malloc16_clear(fp->ofdm_symbol_size*fp->symbols_per_tti*10*sizeof(int32_t) );
        LOG_I(PHY,"[INIT] common_vars->txdataF[%d] = %p (%lu bytes)\n",
              i,common_vars->txdataF[i],
              fp->ofdm_symbol_size*fp->symbols_per_tti*10*sizeof(int32_t));
	if (eNB->use_DTX==0 && i<fp->nb_antenna_ports_eNB) eNB->subframe_mask[i] = malloc16_clear(fp->ofdm_symbol_size*fp->symbols_per_tti*10*sizeof(int32_t) );
      }
    }

    LOG_I(PHY,"[INIT]SRS allocation\n");
    // Channel estimates for SRS
    for (int srs_id = 0; srs_id < NUMBER_OF_SRS_MAX; srs_id++) {
      srs_vars[srs_id].srs_ch_estimates = (int32_t **) malloc16 (64 * sizeof (int32_t *));
      srs_vars[srs_id].srs_ch_estimates_time = (int32_t **) malloc16 (64 * sizeof (int32_t *));

      for (i = 0; i < 64; i++) {
        srs_vars[srs_id].srs_ch_estimates[i] = (int32_t *) malloc16_clear (sizeof (int32_t) * fp->ofdm_symbol_size);
        srs_vars[srs_id].srs_ch_estimates_time[i] = (int32_t *) malloc16_clear (sizeof (int32_t) * fp->ofdm_symbol_size * 2);
      }
    }                             //UE_id

    generate_ul_ref_sigs_rx ();
    init_ulsch_power_LUT ();

    // SRS
    for (int srs_id = 0; srs_id < NUMBER_OF_SRS_MAX; srs_id++) {
      srs_vars[srs_id].srs = (int32_t *) malloc16_clear (2 * fp->ofdm_symbol_size * sizeof (int32_t));
    }

    LOG_I(PHY,"PRACH allocation\n");
    // PRACH
    prach_vars->prachF = (int16_t *) malloc16_clear (1024 * 2 * sizeof (int16_t));
    // assume maximum of 64 RX antennas for PRACH receiver
    prach_vars->prach_ifft[0] = (int32_t **) malloc16_clear (64 * sizeof (int32_t *));

    for (i = 0; i < 64; i++)
      prach_vars->prach_ifft[0][i] = (int32_t *) malloc16_clear (1024 * 2 * sizeof (int32_t));

    prach_vars->rxsigF[0] = (int16_t **) malloc16_clear (64 * sizeof (int16_t *));
    // PRACH BR
    prach_vars_br->prachF = (int16_t *)malloc16_clear( 1024*2*sizeof(int32_t) );

    // assume maximum of 64 RX antennas for PRACH receiver
    for (int ce_level = 0; ce_level < 4; ce_level++) {
      prach_vars_br->prach_ifft[ce_level] = (int32_t **) malloc16_clear (64 * sizeof (int32_t *));

      for (i = 0; i < 64; i++)
        prach_vars_br->prach_ifft[ce_level][i] = (int32_t *) malloc16_clear (1024 * 2 * sizeof (int32_t));

      prach_vars->rxsigF[ce_level] = (int16_t **) malloc16_clear (64 * sizeof (int16_t *));
    }

    /* number of elements of an array X is computed as sizeof(X) / sizeof(X[0])
    AssertFatal(fp->nb_antennas_rx <= sizeof(prach_vars->rxsigF) / sizeof(prach_vars->rxsigF[0]),
                "nb_antennas_rx too large");
    for (i=0; i<fp->nb_antennas_rx; i++) {
      prach_vars->rxsigF[i] = (int16_t*)malloc16_clear( fp->ofdm_symbol_size*12*2*sizeof(int16_t) );
      LOG_D(PHY,"[INIT] prach_vars->rxsigF[%d] = %p\n",i,prach_vars->rxsigF[i]);
      }*/

    printf("NUMBER_OF_ULSCH_MAX %d\n",NUMBER_OF_ULSCH_MAX);
    for (int ULSCH_id=0; ULSCH_id<NUMBER_OF_ULSCH_MAX; ULSCH_id++) {
      //FIXME
      pusch_vars[ULSCH_id] = (LTE_eNB_PUSCH *) malloc16_clear (NUMBER_OF_ULSCH_MAX * sizeof (LTE_eNB_PUSCH));
      pusch_vars[ULSCH_id]->rxdataF_ext = (int32_t **) malloc16 (4 * sizeof (int32_t *));
      pusch_vars[ULSCH_id]->rxdataF_ext2 = (int32_t **) malloc16 (4 * sizeof (int32_t *));
      pusch_vars[ULSCH_id]->drs_ch_estimates = (int32_t **) malloc16 (4 * sizeof (int32_t *));
      pusch_vars[ULSCH_id]->drs_ch_estimates_time = (int32_t **) malloc16 (4 * sizeof (int32_t *));
      pusch_vars[ULSCH_id]->rxdataF_comp = (int32_t **) malloc16 (4 * sizeof (int32_t *));
      pusch_vars[ULSCH_id]->ul_ch_mag = (int32_t **) malloc16 (4 * sizeof (int32_t *));
      pusch_vars[ULSCH_id]->ul_ch_magb = (int32_t **) malloc16 (4 * sizeof (int32_t *));
      AssertFatal (fp->ofdm_symbol_size > 127, "fp->ofdm_symbol_size %d<128\n", fp->ofdm_symbol_size);
      AssertFatal (fp->symbols_per_tti > 11, "fp->symbols_per_tti %d < 12\n", fp->symbols_per_tti);
      AssertFatal (fp->N_RB_UL > 5, "fp->N_RB_UL %d < 6\n", fp->N_RB_UL);
      AssertFatal (fp->nb_antennas_rx > 0,"fp->nb_antennas_rx = %d\n",fp->nb_antennas_rx);
      for (i = 0; i < fp->nb_antennas_rx; i++) {
        // FIXME We should get rid of this
        pusch_vars[ULSCH_id]->rxdataF_ext[i]      = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->N_RB_UL*12*fp->symbols_per_tti );
        pusch_vars[ULSCH_id]->drs_ch_estimates[i] = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->N_RB_UL*12*fp->symbols_per_tti );
        pusch_vars[ULSCH_id]->drs_ch_estimates_time[i] = (int32_t *)malloc16_clear( 4*sizeof(int32_t)*fp->ofdm_symbol_size );
        pusch_vars[ULSCH_id]->rxdataF_comp[i]     = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->N_RB_UL*12*fp->symbols_per_tti );
        pusch_vars[ULSCH_id]->ul_ch_mag[i]  = (int32_t *)malloc16_clear( fp->symbols_per_tti*sizeof(int32_t)*fp->N_RB_UL*12 );
        pusch_vars[ULSCH_id]->ul_ch_magb[i] = (int32_t *)malloc16_clear( fp->symbols_per_tti*sizeof(int32_t)*fp->N_RB_UL*12 );
      }

      pusch_vars[ULSCH_id]->llr = (int16_t *)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
    } //ULSCH_id

    for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++)
      eNB->UE_stats_ptr[UE_id] = &eNB->UE_stats[UE_id];
  }

  eNB->pdsch_config_dedicated->p_a = dB0;       //defaul value until overwritten by RRCConnectionReconfiguration
  return (0);
}

void phy_free_lte_eNB(PHY_VARS_eNB *eNB) {
  LTE_DL_FRAME_PARMS *const fp       = &eNB->frame_parms;
  LTE_eNB_COMMON *const common_vars  = &eNB->common_vars;
  LTE_eNB_PUSCH **const pusch_vars   = eNB->pusch_vars;
  LTE_eNB_SRS *const srs_vars        = eNB->srs_vars;
  LTE_eNB_PRACH *const prach_vars    = &eNB->prach_vars;
  LTE_eNB_PRACH *const prach_vars_br = &eNB->prach_vars_br;
  int i, UE_id;

  if (common_vars->txdataF) {
    for (i = 0; i < NB_ANTENNA_PORTS_ENB; i++) {
      if (i < fp->nb_antenna_ports_eNB || i == 5) {
        free_and_zero(common_vars->txdataF[i]);
      }
    }
  }
  /* rxdataF[i] is not allocated -> don't free */

  free_and_zero(common_vars->txdataF);
  free_and_zero(common_vars->rxdataF);

  // Channel estimates for SRS
  for (int srs_id = 0; srs_id < NUMBER_OF_SRS_MAX; srs_id++) {
    for (i=0; i<64; i++) {
      free_and_zero(srs_vars[srs_id].srs_ch_estimates[i]);
      free_and_zero(srs_vars[srs_id].srs_ch_estimates_time[i]);
    }

    free_and_zero(srs_vars[srs_id].srs_ch_estimates);
    free_and_zero(srs_vars[srs_id].srs_ch_estimates_time);
  } //UE_id

  free_ul_ref_sigs();

  for (UE_id=0; UE_id<NUMBER_OF_SRS_MAX; UE_id++) free_and_zero(srs_vars[UE_id].srs);

  free_and_zero(prach_vars->prachF);

  if (prach_vars->prach_ifft[0]) {
    for (i = 0; i < 64; i++) {
      free_and_zero(prach_vars->prach_ifft[0][i]);
    }
    free_and_zero(prach_vars->prach_ifft[0]);
  }

  for (int ce_level = 0; ce_level < 4; ce_level++) {
    if (prach_vars_br->prach_ifft[ce_level]) {
      for (i = 0; i < 64; i++) {
        free_and_zero(prach_vars_br->prach_ifft[ce_level][i]);
      }
      free_and_zero(prach_vars_br->prach_ifft[ce_level]);
    }
    free_and_zero(prach_vars->rxsigF[ce_level]);
  }

  free_and_zero(prach_vars_br->prachF);
  free_and_zero(prach_vars->rxsigF[0]);

  for (int ULSCH_id=0; ULSCH_id<NUMBER_OF_ULSCH_MAX; ULSCH_id++) {
    for (i = 0; i < fp->nb_antennas_rx; i++) {
      free_and_zero(pusch_vars[ULSCH_id]->rxdataF_ext[i]);
      free_and_zero(pusch_vars[ULSCH_id]->rxdataF_ext2[i]);
      free_and_zero(pusch_vars[ULSCH_id]->drs_ch_estimates[i]);
      free_and_zero(pusch_vars[ULSCH_id]->drs_ch_estimates_time[i]);
      free_and_zero(pusch_vars[ULSCH_id]->rxdataF_comp[i]);
      free_and_zero(pusch_vars[ULSCH_id]->ul_ch_mag[i]);
      free_and_zero(pusch_vars[ULSCH_id]->ul_ch_magb[i]);
    }

    free_and_zero(pusch_vars[ULSCH_id]->rxdataF_ext);
    free_and_zero(pusch_vars[ULSCH_id]->rxdataF_ext2);
    free_and_zero(pusch_vars[ULSCH_id]->drs_ch_estimates);
    free_and_zero(pusch_vars[ULSCH_id]->drs_ch_estimates_time);
    free_and_zero(pusch_vars[ULSCH_id]->rxdataF_comp);
    free_and_zero(pusch_vars[ULSCH_id]->ul_ch_mag);
    free_and_zero(pusch_vars[ULSCH_id]->ul_ch_magb);
    free_and_zero(pusch_vars[ULSCH_id]->llr);
    free_and_zero(pusch_vars[ULSCH_id]);
  } //UE_id

  for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) eNB->UE_stats_ptr[UE_id] = NULL;
}

void install_schedule_handlers(IF_Module_t *if_inst) {
  if_inst->PHY_config_req = phy_config_request;
  if_inst->schedule_response = schedule_response;
}

