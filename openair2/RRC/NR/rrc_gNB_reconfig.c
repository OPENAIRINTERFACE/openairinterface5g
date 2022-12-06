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

/*! \file rrc_gNB_reconfig.c
 * \brief rrc gNB RRCreconfiguration support routines
 * \author Raymond Knopp
 * \date 2019
 * \version 1.0
 * \company Eurecom
 * \email: raymond.knopp@eurecom.fr
 */
#ifndef RRC_GNB_NSA_C
#define RRC_GNB_NSA_C

#include "NR_ServingCellConfigCommon.h"
#include "NR_ServingCellConfig.h"
#include "NR_RRCReconfiguration.h"
#include "NR_RRCReconfiguration-IEs.h"
#include "NR_CellGroupConfig.h"
#include "NR_MAC-CellGroupConfig.h"
#include "NR_BSR-Config.h"
#include "NR_PDSCH-ServingCellConfig.h"
#include "NR_RLC-BearerConfig.h"
#include "BOOLEAN.h"
#include "assertions.h"
#include "common/utils/nr/nr_common.h"
#include "SIMULATION/TOOLS/sim.h"
#include "executables/softmodem-common.h"
#include "LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "nr_rrc_config.h"

void fix_servingcellconfigdedicated(NR_ServingCellConfig_t *scd) {

  int b = 0;
  while (b < scd->downlinkBWP_ToAddModList->list.count) {
    if (scd->downlinkBWP_ToAddModList->list.array[b]->bwp_Common->genericParameters.locationAndBandwidth == 0) {
      asn_sequence_del(&scd->downlinkBWP_ToAddModList->list, b, 1);
    } else {
      b++;
    }
  }

  if(scd->downlinkBWP_ToAddModList->list.count == 0) {
    free(scd->downlinkBWP_ToAddModList);
    scd->downlinkBWP_ToAddModList = NULL;
  }

  b = 0;
  while (b < scd->uplinkConfig->uplinkBWP_ToAddModList->list.count) {
    if (scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[b]->bwp_Common->genericParameters.locationAndBandwidth ==
        0) {
      asn_sequence_del(&scd->uplinkConfig->uplinkBWP_ToAddModList->list, b, 1);
    } else {
      b++;
    }
  }

  if(scd->uplinkConfig->uplinkBWP_ToAddModList->list.count == 0) {
    free(scd->uplinkConfig->uplinkBWP_ToAddModList);
    scd->uplinkConfig->uplinkBWP_ToAddModList = NULL;
  }

}

void fill_default_secondaryCellGroup(NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                                     NR_ServingCellConfig_t *servingcellconfigdedicated,
                                     NR_CellGroupConfig_t *secondaryCellGroup,
                                     NR_UE_NR_Capability_t *uecap,
                                     int scg_id,
                                     int servCellIndex,
                                     const gNB_RrcConfigurationReq *configuration,
                                     int uid) {

  const rrc_pdsch_AntennaPorts_t* pdschap = &configuration->pdsch_AntennaPorts;
  const int dl_antenna_ports = pdschap->N1 * pdschap->N2 * pdschap->XP;
  const int do_csirs = configuration->do_CSIRS;

  AssertFatal(servingcellconfigcommon!=NULL,"servingcellconfigcommon is null\n");
  AssertFatal(secondaryCellGroup!=NULL,"secondaryCellGroup is null\n");

  if(uecap == NULL)
    LOG_E(RRC,"No UE Capabilities available when programming default CellGroup in NSA\n");

  // This assert will never happen in the current implementation because NUMBER_OF_UE_MAX = 4.
  // However, if in the future NUMBER_OF_UE_MAX is increased, it will be necessary to improve the allocation of SRS resources,
  // where the startPosition = 2 or 3 and sl160 = 17, 17, 27 ... 157 only give us 30 different allocations.
  AssertFatal(uid>=0 && uid<30, "gNB cannot allocate the SRS resources\n");

  uint64_t bitmap = get_ssb_bitmap(servingcellconfigcommon);
  fix_servingcellconfigdedicated(servingcellconfigdedicated);

  memset(secondaryCellGroup,0,sizeof(NR_CellGroupConfig_t));
  secondaryCellGroup->cellGroupId = scg_id;
  NR_RLC_BearerConfig_t *RLC_BearerConfig = calloc(1,sizeof(*RLC_BearerConfig));
  nr_rlc_bearer_init(RLC_BearerConfig, NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity);
  nr_drb_config(RLC_BearerConfig->rlc_Config, NR_RLC_Config_PR_um_Bi_Directional);
  //nr_drb_config(RLC_BearerConfig->rlc_Config, NR_RLC_Config_PR_am);
  nr_rlc_bearer_init_ul_spec(RLC_BearerConfig->mac_LogicalChannelConfig);

  secondaryCellGroup->rlc_BearerToAddModList = calloc(1,sizeof(*secondaryCellGroup->rlc_BearerToAddModList));
  ASN_SEQUENCE_ADD(&secondaryCellGroup->rlc_BearerToAddModList->list, RLC_BearerConfig);

  NR_MAC_CellGroupConfig_t *mac_CellGroupConfig = secondaryCellGroup->mac_CellGroupConfig;

  mac_CellGroupConfig=calloc(1,sizeof(*mac_CellGroupConfig));
  mac_CellGroupConfig->drx_Config = NULL;

  mac_CellGroupConfig->bsr_Config=calloc(1,sizeof(*mac_CellGroupConfig->bsr_Config));
  mac_CellGroupConfig->bsr_Config->periodicBSR_Timer = NR_BSR_Config__periodicBSR_Timer_sf10;
  mac_CellGroupConfig->bsr_Config->retxBSR_Timer     = NR_BSR_Config__retxBSR_Timer_sf160;
  mac_CellGroupConfig->tag_Config=calloc(1,sizeof(*mac_CellGroupConfig->tag_Config));
  mac_CellGroupConfig->tag_Config->tag_ToReleaseList = NULL;
  mac_CellGroupConfig->tag_Config->tag_ToAddModList  = calloc(1,sizeof(*mac_CellGroupConfig->tag_Config->tag_ToAddModList));
  struct NR_TAG *tag=calloc(1,sizeof(*tag));
  tag->tag_Id             = 0;
  tag->timeAlignmentTimer = NR_TimeAlignmentTimer_infinity;
  ASN_SEQUENCE_ADD(&mac_CellGroupConfig->tag_Config->tag_ToAddModList->list,tag);
  set_phr_config(mac_CellGroupConfig);
  mac_CellGroupConfig->skipUplinkTxDynamic=false;
  mac_CellGroupConfig->ext1 = NULL;
  secondaryCellGroup->physicalCellGroupConfig = calloc(1,sizeof(*secondaryCellGroup->physicalCellGroupConfig));
  secondaryCellGroup->physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH=NULL;
  secondaryCellGroup->physicalCellGroupConfig->harq_ACK_SpatialBundlingPUSCH=NULL;
  secondaryCellGroup->physicalCellGroupConfig->p_NR_FR1=calloc(1,sizeof(*secondaryCellGroup->physicalCellGroupConfig->p_NR_FR1));
  *secondaryCellGroup->physicalCellGroupConfig->p_NR_FR1=20;
  secondaryCellGroup->physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook=NR_PhysicalCellGroupConfig__pdsch_HARQ_ACK_Codebook_dynamic;
  secondaryCellGroup->physicalCellGroupConfig->tpc_SRS_RNTI=NULL;
  secondaryCellGroup->physicalCellGroupConfig->tpc_PUCCH_RNTI=NULL;
  secondaryCellGroup->physicalCellGroupConfig->tpc_PUSCH_RNTI=NULL;
  secondaryCellGroup->physicalCellGroupConfig->sp_CSI_RNTI=NULL;
  secondaryCellGroup->physicalCellGroupConfig->cs_RNTI=NULL;
  secondaryCellGroup->physicalCellGroupConfig->ext1=NULL;
  secondaryCellGroup->spCellConfig = calloc(1,sizeof(*secondaryCellGroup->spCellConfig));
  secondaryCellGroup->spCellConfig->servCellIndex = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->servCellIndex));
  *secondaryCellGroup->spCellConfig->servCellIndex = servCellIndex;
  secondaryCellGroup->spCellConfig->reconfigurationWithSync=calloc(1,sizeof(*secondaryCellGroup->spCellConfig->reconfigurationWithSync));
  secondaryCellGroup->spCellConfig->reconfigurationWithSync->spCellConfigCommon=servingcellconfigcommon;
  secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity=(get_softmodem_params()->phy_test==1) ? 0x1234 : (taus()&0xffff);
  secondaryCellGroup->spCellConfig->reconfigurationWithSync->t304=NR_ReconfigurationWithSync__t304_ms2000;
  secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated = NULL;
  secondaryCellGroup->spCellConfig->reconfigurationWithSync->ext1                 = NULL;

  // For 2-step contention-free random access procedure
  if(get_softmodem_params()->sa == 0) {
    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated = calloc(1,sizeof(struct NR_ReconfigurationWithSync__rach_ConfigDedicated));
    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->present= NR_ReconfigurationWithSync__rach_ConfigDedicated_PR_uplink;
    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink= calloc(1,sizeof(struct NR_RACH_ConfigDedicated));
    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra= calloc(1,sizeof(struct NR_CFRA));
    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->ra_Prioritization= NULL;
    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra->occasions= calloc(1,sizeof(struct NR_CFRA__occasions));
    memcpy(&secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra->occasions->rach_ConfigGeneric,
           &servingcellconfigcommon->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric, sizeof(NR_RACH_ConfigGeneric_t));
    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra->occasions->ssb_perRACH_Occasion= calloc(1,sizeof(long));
    *secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra->occasions->ssb_perRACH_Occasion = NR_CFRA__occasions__ssb_perRACH_Occasion_one;
    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra->resources.present = NR_CFRA__resources_PR_ssb;
    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra->resources.choice.ssb = calloc(1,sizeof(struct NR_CFRA__resources__ssb));
    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra->resources.choice.ssb->ra_ssb_OccasionMaskIndex = 0;

    int n_ssb = 0;
    struct NR_CFRA_SSB_Resource *ssbElem[64];
    for (int i=0;i<64;i++) {
      if ((bitmap>>(63-i))&0x01){
        ssbElem[n_ssb] = calloc(1,sizeof(struct NR_CFRA_SSB_Resource));
        ssbElem[n_ssb]->ssb = i;
        ssbElem[n_ssb]->ra_PreambleIndex = 63 - (uid % 64);
        ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra->resources.choice.ssb->ssb_ResourceList.list,ssbElem[n_ssb]);
        n_ssb++;
      }
    }

    secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra->ext1 = NULL;
  }

  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->rlf_TimersAndConstants));
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->present = NR_SetupRelease_RLF_TimersAndConstants_PR_setup;
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup=calloc(1,sizeof(*secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup));
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->t310 = NR_RLF_TimersAndConstants__t310_ms4000;
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->n310 = NR_RLF_TimersAndConstants__n310_n20;
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->n311 = NR_RLF_TimersAndConstants__n311_n1;
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->ext1 = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->ext1));
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->ext1->t311 = NR_RLF_TimersAndConstants__ext1__t311_ms30000;
  secondaryCellGroup->spCellConfig->rlmInSyncOutOfSyncThreshold                   = NULL;

  secondaryCellGroup->spCellConfig->spCellConfigDedicated = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated));
  secondaryCellGroup->spCellConfig->spCellConfigDedicated->tdd_UL_DL_ConfigurationDedicated = NULL;

  /// initialDownlinkBWP

  secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP));
  secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdcch_Config=NULL;
  secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdsch_Config = config_pdsch(bitmap, 0, dl_antenna_ports);
  secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->sps_Config = NULL; // calloc(1,sizeof(struct NR_SetupRelease_SPS_Config));

  secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig = NULL;
#if 0
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig));
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->present = NR_SetupRelease_RadioLinkMonitoringConfig_PR_setup;

 secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->choice.setup = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->choice.setup));
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->choice.setup->failureDetectionResourcesToAddModList=NULL;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->choice.setup->failureDetectionResourcesToReleaseList=NULL;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->choice.setup->beamFailureInstanceMaxCount = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->choice.setup->beamFailureInstanceMaxCount));
 *secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->choice.setup->beamFailureInstanceMaxCount = NR_RadioLinkMonitoringConfig__beamFailureInstanceMaxCount_n3;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->choice.setup->beamFailureDetectionTimer = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->choice.setup->beamFailureDetectionTimer));
 *secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->choice.setup->beamFailureDetectionTimer = NR_RadioLinkMonitoringConfig__beamFailureDetectionTimer_pbfd2;
#endif

  /// initialUplinkBWP

  if (!secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig) {
    secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig=calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig));
  }

  NR_BWP_UplinkDedicated_t *initialUplinkBWP = calloc(1,sizeof(*initialUplinkBWP));
  secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP = initialUplinkBWP;
  initialUplinkBWP->pucch_Config = NULL;

  NR_PUSCH_Config_t *pusch_Config = NULL;
  if (servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList) {
    pusch_Config = servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup;
  }
  initialUplinkBWP->pusch_Config = config_pusch(pusch_Config, servingcellconfigcommon);

  long maxMIMO_Layers = servingcellconfigdedicated->uplinkConfig &&
                                servingcellconfigdedicated->uplinkConfig->pusch_ServingCellConfig &&
                                servingcellconfigdedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->ext1 &&
                                servingcellconfigdedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->ext1->maxMIMO_Layers ?
                            *servingcellconfigdedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->ext1->maxMIMO_Layers : 1;

  int curr_bwp = NRRIV2BW(servingcellconfigcommon->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);
  initialUplinkBWP->srs_Config = calloc(1,sizeof(*initialUplinkBWP->srs_Config));
  config_srs(initialUplinkBWP->srs_Config, NULL, curr_bwp, uid, 0, maxMIMO_Layers, configuration->do_SRS);

  // Downlink BWPs
  int n_dl_bwp = 1;
  if (servingcellconfigdedicated && servingcellconfigdedicated->downlinkBWP_ToAddModList) {
    n_dl_bwp = servingcellconfigdedicated->downlinkBWP_ToAddModList->list.count;
  }
  if(n_dl_bwp>0) {
    secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList));
    for (int bwp_loop = 0; bwp_loop < n_dl_bwp; bwp_loop++) {
      NR_BWP_Downlink_t *bwp = calloc(1,sizeof(*bwp));
      config_downlinkBWP(bwp, servingcellconfigcommon,
                         servingcellconfigdedicated,
                         uecap,
                         dl_antenna_ports,
                         configuration->force_256qam_off,
                         bwp_loop, false);
      ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list,bwp);
    }
    secondaryCellGroup->spCellConfig->spCellConfigDedicated->firstActiveDownlinkBWP_Id = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->firstActiveDownlinkBWP_Id));
    *secondaryCellGroup->spCellConfig->spCellConfigDedicated->firstActiveDownlinkBWP_Id = servingcellconfigdedicated->firstActiveDownlinkBWP_Id ? *servingcellconfigdedicated->firstActiveDownlinkBWP_Id : 1;
    secondaryCellGroup->spCellConfig->spCellConfigDedicated->defaultDownlinkBWP_Id = calloc(1, sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->defaultDownlinkBWP_Id));
    *secondaryCellGroup->spCellConfig->spCellConfigDedicated->defaultDownlinkBWP_Id = servingcellconfigdedicated->defaultDownlinkBWP_Id ? *servingcellconfigdedicated->defaultDownlinkBWP_Id : 1;
  }

  // Uplink BWPs
  int n_ul_bwp = 1;
  if (servingcellconfigdedicated && servingcellconfigdedicated->uplinkConfig && servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList) {
    n_ul_bwp = servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count;
  }
  if(n_ul_bwp>0) {
    secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList));
    for (int bwp_loop = 0; bwp_loop < n_ul_bwp; bwp_loop++) {
      NR_BWP_Uplink_t *ubwp = calloc(1,sizeof(*ubwp));
      config_uplinkBWP(ubwp, bwp_loop, false, uid,
                       configuration,
                       servingcellconfigdedicated,
                       servingcellconfigcommon,
                       uecap);
      ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list,ubwp);
    }
    secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id));
    *secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id = servingcellconfigdedicated->uplinkConfig->firstActiveUplinkBWP_Id ? *servingcellconfigdedicated->uplinkConfig->firstActiveUplinkBWP_Id : 1;
  }

  secondaryCellGroup->spCellConfig->spCellConfigDedicated->bwp_InactivityTimer = NULL;
  secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToReleaseList= NULL;
  secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToReleaseList = NULL;

  mac_CellGroupConfig->schedulingRequestConfig = calloc(1, sizeof(*mac_CellGroupConfig->schedulingRequestConfig));
  mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList = CALLOC(1,sizeof(*mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList));
  struct NR_SchedulingRequestToAddMod *schedulingrequestlist = CALLOC(1,sizeof(*schedulingrequestlist));
  schedulingrequestlist->schedulingRequestId  = 0;
  schedulingrequestlist->sr_ProhibitTimer = NULL;
  schedulingrequestlist->sr_TransMax      = NR_SchedulingRequestToAddMod__sr_TransMax_n64;
  ASN_SEQUENCE_ADD(&(mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList->list),schedulingrequestlist);

 secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig));
 NR_PUSCH_ServingCellConfig_t *pusch_scc = calloc(1,sizeof(*pusch_scc));
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->present = NR_SetupRelease_PUSCH_ServingCellConfig_PR_setup;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup = pusch_scc;
 pusch_scc->codeBlockGroupTransmission = NULL;
 pusch_scc->rateMatching = NULL;
 pusch_scc->xOverhead = NULL;
 pusch_scc->ext1=calloc(1,sizeof(*pusch_scc->ext1));
 pusch_scc->ext1->maxMIMO_Layers = calloc(1,sizeof(*pusch_scc->ext1->maxMIMO_Layers));
 *pusch_scc->ext1->maxMIMO_Layers = 1;
 pusch_scc->ext1->processingType2Enabled = NULL;

 secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->carrierSwitching = NULL;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->supplementaryUplink=NULL;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->pdcch_ServingCellConfig=NULL;

 secondaryCellGroup->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig=calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig));
 NR_PDSCH_ServingCellConfig_t *pdsch_servingcellconfig = calloc(1,sizeof(*pdsch_servingcellconfig));
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->present = NR_SetupRelease_PDSCH_ServingCellConfig_PR_setup;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup = pdsch_servingcellconfig;
 pdsch_servingcellconfig->codeBlockGroupTransmission = NULL;
 pdsch_servingcellconfig->xOverhead = NULL;
 pdsch_servingcellconfig->nrofHARQ_ProcessesForPDSCH = calloc(1, sizeof(*pdsch_servingcellconfig->nrofHARQ_ProcessesForPDSCH));
 *pdsch_servingcellconfig->nrofHARQ_ProcessesForPDSCH = NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH_n16;
 pdsch_servingcellconfig->pucch_Cell= NULL;
 pdsch_servingcellconfig->ext1=calloc(1,sizeof(*pdsch_servingcellconfig->ext1));
 pdsch_servingcellconfig->ext1->maxMIMO_Layers = calloc(1,sizeof(*pdsch_servingcellconfig->ext1->maxMIMO_Layers));
 *pdsch_servingcellconfig->ext1->maxMIMO_Layers = dl_antenna_ports;
 pdsch_servingcellconfig->ext1->processingType2Enabled = NULL;
 
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig=NULL;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig=calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig));
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig->present = NR_SetupRelease_CSI_MeasConfig_PR_setup;

 NR_CSI_MeasConfig_t *csi_MeasConfig = calloc(1,sizeof(*csi_MeasConfig));
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup = csi_MeasConfig;

 csi_MeasConfig->csi_ResourceConfigToAddModList = calloc(1,sizeof(*csi_MeasConfig->csi_ResourceConfigToAddModList));
 csi_MeasConfig->csi_ResourceConfigToReleaseList = NULL;

 csi_MeasConfig->csi_SSB_ResourceSetToAddModList = calloc(1,sizeof(*csi_MeasConfig->csi_SSB_ResourceSetToAddModList));
 csi_MeasConfig->csi_SSB_ResourceSetToReleaseList = NULL;

 csi_MeasConfig->csi_ReportConfigToAddModList = calloc(1,sizeof(*csi_MeasConfig->csi_ReportConfigToAddModList));
 csi_MeasConfig->csi_ReportConfigToReleaseList = NULL;

 NR_CSI_SSB_ResourceSet_t *ssbresset0 = calloc(1,sizeof(*ssbresset0));
 ssbresset0->csi_SSB_ResourceSetId = 0;

  for (int i=0;i<64;i++) {
    if ((bitmap >> (63 - i)) & 0x01) {
      NR_SSB_Index_t *ssbres = NULL;
      asn1cCallocOne(ssbres, i);
      ASN_SEQUENCE_ADD(&ssbresset0->csi_SSB_ResourceList.list, ssbres);
    }
  }
 ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list,ssbresset0);

 for (int bwp_loop = 0; bwp_loop < n_dl_bwp; bwp_loop++) {

  NR_BWP_Downlink_t *bwp = secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_loop];
  int curr_bwp = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);

   config_csirs(servingcellconfigcommon, csi_MeasConfig, uid, dl_antenna_ports, curr_bwp, do_csirs, bwp_loop);
   config_csiim(do_csirs, dl_antenna_ports, curr_bwp, csi_MeasConfig, bwp_loop);

   if (do_csirs) {
     NR_CSI_ResourceConfig_t *csires = calloc(1,sizeof(*csires));
     csires->csi_ResourceConfigId = bwp->bwp_Id;
     csires->csi_RS_ResourceSetList.present = NR_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_nzp_CSI_RS_SSB;
     csires->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB = calloc(1,sizeof(*csires->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB));
     csires->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList = calloc(1,sizeof(*csires->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList));
     NR_NZP_CSI_RS_ResourceSetId_t *nzp0 = calloc(1,sizeof(*nzp0));
     *nzp0 = bwp_loop;
     ASN_SEQUENCE_ADD(&csires->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList->list,nzp0);
     csires->bwp_Id = bwp->bwp_Id;
     csires->resourceType = NR_CSI_ResourceConfig__resourceType_periodic;
     ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ResourceConfigToAddModList->list,csires);
   }
   if (do_csirs && dl_antenna_ports > 1) {
     NR_CSI_ResourceConfig_t *csiresim = calloc(1,sizeof(*csiresim));
     csiresim->csi_ResourceConfigId = bwp->bwp_Id+10;
     csiresim->csi_RS_ResourceSetList.present = NR_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_csi_IM_ResourceSetList;
     csiresim->csi_RS_ResourceSetList.choice.csi_IM_ResourceSetList = calloc(1,sizeof(*csiresim->csi_RS_ResourceSetList.choice.csi_IM_ResourceSetList));
     NR_CSI_IM_ResourceSetId_t *csiim00 = calloc(1,sizeof(*csiim00));
     *csiim00 = bwp_loop;
     ASN_SEQUENCE_ADD(&csiresim->csi_RS_ResourceSetList.choice.csi_IM_ResourceSetList->list,csiim00);
     csiresim->bwp_Id = bwp->bwp_Id;
     csiresim->resourceType = NR_CSI_ResourceConfig__resourceType_periodic;
     ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ResourceConfigToAddModList->list,csiresim);
   }
   NR_CSI_ResourceConfig_t *ssbres = calloc(1,sizeof(*ssbres));
   ssbres->csi_ResourceConfigId = bwp->bwp_Id+20;
   ssbres->csi_RS_ResourceSetList.present = NR_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_nzp_CSI_RS_SSB;
   ssbres->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB = calloc(1,sizeof(*ssbres->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB));
   ssbres->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList = calloc(1,sizeof(*ssbres->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList));
   NR_CSI_SSB_ResourceSetId_t *ssbres00 = calloc(1,sizeof(*ssbres00));
   *ssbres00 = 0;
   ASN_SEQUENCE_ADD(&ssbres->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList->list,ssbres00);
   ssbres->bwp_Id = bwp->bwp_Id;
   ssbres->resourceType = NR_CSI_ResourceConfig__resourceType_periodic;
   ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ResourceConfigToAddModList->list,ssbres);

   NR_PUCCH_CSI_Resource_t *pucchcsires1 = calloc(1,sizeof(*pucchcsires1));
   pucchcsires1->uplinkBandwidthPartId = bwp->bwp_Id;
   pucchcsires1->pucch_Resource=2;

   if (do_csirs && dl_antenna_ports > 1) {
     NR_CSI_ReportConfig_t *csirep1 = calloc(1,sizeof(*csirep1));
     csirep1->reportConfigId=bwp->bwp_Id;
     csirep1->carrier=NULL;
     csirep1->resourcesForChannelMeasurement=bwp->bwp_Id;
     csirep1->csi_IM_ResourcesForInterference=calloc(1,sizeof(*csirep1->csi_IM_ResourcesForInterference));
     *csirep1->csi_IM_ResourcesForInterference=bwp->bwp_Id+10;
     csirep1->nzp_CSI_RS_ResourcesForInterference=NULL;
     csirep1->reportConfigType.present = NR_CSI_ReportConfig__reportConfigType_PR_periodic;
     csirep1->reportConfigType.choice.periodic = calloc(1,sizeof(*csirep1->reportConfigType.choice.periodic));
     csirep1->reportConfigType.choice.periodic->reportSlotConfig.present=NR_CSI_ReportPeriodicityAndOffset_PR_slots320;
     csirep1->reportConfigType.choice.periodic->reportSlotConfig.choice.slots320 = 9 + (20 * uid) % 320;
     ASN_SEQUENCE_ADD(&csirep1->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list,pucchcsires1);
     csirep1->reportQuantity.present = NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_PMI_CQI;
     csirep1->reportQuantity.choice.cri_RI_PMI_CQI=(NULL_t)0;
     csirep1->reportFreqConfiguration = calloc(1,sizeof(*csirep1->reportFreqConfiguration));
     csirep1->reportFreqConfiguration->cqi_FormatIndicator = calloc(1,sizeof(*csirep1->reportFreqConfiguration->cqi_FormatIndicator));
     *csirep1->reportFreqConfiguration->cqi_FormatIndicator=NR_CSI_ReportConfig__reportFreqConfiguration__cqi_FormatIndicator_widebandCQI;
     csirep1->reportFreqConfiguration->pmi_FormatIndicator = calloc(1,sizeof(*csirep1->reportFreqConfiguration->pmi_FormatIndicator));
     *csirep1->reportFreqConfiguration->pmi_FormatIndicator=NR_CSI_ReportConfig__reportFreqConfiguration__pmi_FormatIndicator_widebandPMI;
     csirep1->reportFreqConfiguration->csi_ReportingBand = calloc(1,sizeof(*csirep1->reportFreqConfiguration->csi_ReportingBand));
     csirep1->reportFreqConfiguration->csi_ReportingBand->present = NR_CSI_ReportConfig__reportFreqConfiguration__csi_ReportingBand_PR_subbands7;
     csirep1->reportFreqConfiguration->csi_ReportingBand->choice.subbands7.size=1;
     csirep1->reportFreqConfiguration->csi_ReportingBand->choice.subbands7.bits_unused=1;
     csirep1->reportFreqConfiguration->csi_ReportingBand->choice.subbands7.buf=malloc(1);
     csirep1->reportFreqConfiguration->csi_ReportingBand->choice.subbands7.buf[0]=254;
     csirep1->timeRestrictionForChannelMeasurements= NR_CSI_ReportConfig__timeRestrictionForChannelMeasurements_configured;
     csirep1->timeRestrictionForInterferenceMeasurements=NR_CSI_ReportConfig__timeRestrictionForInterferenceMeasurements_configured;
     csirep1->codebookConfig=calloc(1,sizeof(*csirep1->codebookConfig));
     csirep1->codebookConfig->codebookType.present = NR_CodebookConfig__codebookType_PR_type1;
     csirep1->codebookConfig->codebookType.choice.type1 = calloc(1,sizeof(*csirep1->codebookConfig->codebookType.choice.type1));
     csirep1->codebookConfig->codebookType.choice.type1->subType.present=NR_CodebookConfig__codebookType__type1__subType_PR_typeI_SinglePanel;
     csirep1->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel=calloc(1,sizeof(*csirep1->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel));
     struct NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel *singlePanelConfig = csirep1->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel;
     if (dl_antenna_ports == 2) {
       singlePanelConfig->nrOfAntennaPorts.present = NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts_PR_two;
       singlePanelConfig->nrOfAntennaPorts.choice.two = calloc(1,sizeof(*singlePanelConfig->nrOfAntennaPorts.choice.two));
       singlePanelConfig->nrOfAntennaPorts.choice.two->twoTX_CodebookSubsetRestriction.size=1;
       singlePanelConfig->nrOfAntennaPorts.choice.two->twoTX_CodebookSubsetRestriction.bits_unused=2;
       singlePanelConfig->nrOfAntennaPorts.choice.two->twoTX_CodebookSubsetRestriction.buf=malloc(1);
       singlePanelConfig->nrOfAntennaPorts.choice.two->twoTX_CodebookSubsetRestriction.buf[0]=0xfc;
       singlePanelConfig->typeI_SinglePanel_ri_Restriction.size=1;
       singlePanelConfig->typeI_SinglePanel_ri_Restriction.bits_unused=0;
       singlePanelConfig->typeI_SinglePanel_ri_Restriction.buf=malloc(1);
       singlePanelConfig->typeI_SinglePanel_ri_Restriction.buf[0]=0x03;
       csirep1->codebookConfig->codebookType.choice.type1->codebookMode=1;
     } else if (dl_antenna_ports < 16) {
       singlePanelConfig->nrOfAntennaPorts.present = NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts_PR_moreThanTwo;
       singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo = calloc(1,sizeof(*singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo));
       if(dl_antenna_ports == 4)
       singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->n1_n2.present=
         NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts__moreThanTwo__n1_n2_PR_two_one_TypeI_SinglePanel_Restriction;
       else if(dl_antenna_ports == 8)
         singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->n1_n2.present=
           NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts__moreThanTwo__n1_n2_PR_four_one_TypeI_SinglePanel_Restriction;
       else if(dl_antenna_ports == 12)
         singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->n1_n2.present=
           NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts__moreThanTwo__n1_n2_PR_six_one_TypeI_SinglePanel_Restriction;
       else//default
         singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->n1_n2.present=
           NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts__moreThanTwo__n1_n2_PR_two_one_TypeI_SinglePanel_Restriction;

     /*singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->n1_n2.choice.two_one_TypeI_SinglePanel_Restriction.size=1;
     singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->n1_n2.choice.two_one_TypeI_SinglePanel_Restriction.bits_unused=1;
     singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->n1_n2.choice.two_one_TypeI_SinglePanel_Restriction.buf=malloc(1);
     singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->n1_n2.choice.two_one_TypeI_SinglePanel_Restriction.buf[0]=0xc0; //'00000011'B
     singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->typeI_SinglePanel_codebookSubsetRestriction_i2->size = 1;
     singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->typeI_SinglePanel_codebookSubsetRestriction_i2->bits_unused=1;
     singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->typeI_SinglePanel_codebookSubsetRestriction_i2->buf=malloc(1);
     singlePanelConfig->nrOfAntennaPorts.choice.moreThanTwo->typeI_SinglePanel_codebookSubsetRestriction_i2->buf[0]=0xc0;*/
       csirep1->codebookConfig->codebookType.choice.type1->codebookMode=1;
     } else {//32 antennas are Not implemented yet
       csirep1->codebookConfig->codebookType.choice.type1->codebookMode=2;
     }
     csirep1->dummy = NULL;
     csirep1->groupBasedBeamReporting.present = NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled;
     csirep1->groupBasedBeamReporting.choice.disabled=calloc(1,sizeof(*csirep1->groupBasedBeamReporting.choice.disabled));
     //csirep1->groupBasedBeamReporting.choice.disabled->nrofReportedRS = calloc(1,sizeof(*csirep1->groupBasedBeamReporting.choice.disabled->nrofReportedRS));
     //*csirep1->groupBasedBeamReporting.choice.disabled->nrofReportedRS=NR_CSI_ReportConfig__groupBasedBeamReporting__disabled__nrofReportedRS_n1;
     csirep1->cqi_Table = calloc(1,sizeof(*csirep1->cqi_Table));
     if(bwp->bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table!=NULL)
        *csirep1->cqi_Table = NR_CSI_ReportConfig__cqi_Table_table2;
     else
        *csirep1->cqi_Table = NR_CSI_ReportConfig__cqi_Table_table1;
     csirep1->subbandSize = NR_CSI_ReportConfig__subbandSize_value2;
     csirep1->non_PMI_PortIndication = NULL;
     csirep1->ext1 = NULL;
     ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ReportConfigToAddModList->list,csirep1);
   }

   if (do_csirs) {
     NR_CSI_ReportConfig_t *csirep2 = calloc(1,sizeof(*csirep2));
     csirep2->reportConfigId=bwp->bwp_Id+10;
     csirep2->carrier=NULL;
     csirep2->resourcesForChannelMeasurement=bwp->bwp_Id;
     csirep2->csi_IM_ResourcesForInterference=NULL;
     csirep2->nzp_CSI_RS_ResourcesForInterference=NULL;
     csirep2->reportConfigType.present = NR_CSI_ReportConfig__reportConfigType_PR_periodic;
     csirep2->reportConfigType.choice.periodic = calloc(1,sizeof(*csirep2->reportConfigType.choice.periodic));
     csirep2->reportConfigType.choice.periodic->reportSlotConfig.present=NR_CSI_ReportPeriodicityAndOffset_PR_slots320;
     csirep2->reportConfigType.choice.periodic->reportSlotConfig.choice.slots320 = 39 + (20 * uid) % 320;
     ASN_SEQUENCE_ADD(&csirep2->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list,pucchcsires1);
     csirep2->reportQuantity.present = NR_CSI_ReportConfig__reportQuantity_PR_cri_RSRP;
     csirep2->reportQuantity.choice.cri_RSRP=(NULL_t)0;
     csirep2->reportFreqConfiguration = calloc(1,sizeof(*csirep2->reportFreqConfiguration));
     csirep2->reportFreqConfiguration->cqi_FormatIndicator = NULL;
     csirep2->reportFreqConfiguration->pmi_FormatIndicator=NULL;
     csirep2->reportFreqConfiguration->csi_ReportingBand=NULL;
     csirep2->timeRestrictionForChannelMeasurements= NR_CSI_ReportConfig__timeRestrictionForChannelMeasurements_configured;
     csirep2->timeRestrictionForInterferenceMeasurements=NR_CSI_ReportConfig__timeRestrictionForInterferenceMeasurements_configured;
     csirep2->codebookConfig=NULL;
     csirep2->dummy = NULL;
     csirep2->groupBasedBeamReporting.present = NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled;
     csirep2->groupBasedBeamReporting.choice.disabled=calloc(1,sizeof(*csirep2->groupBasedBeamReporting.choice.disabled));
     csirep2->groupBasedBeamReporting.choice.disabled->nrofReportedRS = calloc(1,sizeof(*csirep2->groupBasedBeamReporting.choice.disabled->nrofReportedRS));
     *csirep2->groupBasedBeamReporting.choice.disabled->nrofReportedRS=NR_CSI_ReportConfig__groupBasedBeamReporting__disabled__nrofReportedRS_n1;
     csirep2->cqi_Table = NULL;
     csirep2->subbandSize = NR_CSI_ReportConfig__subbandSize_value1;
     csirep2->non_PMI_PortIndication = NULL;
     csirep2->ext1 = NULL;
     ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ReportConfigToAddModList->list,csirep2);
   }
   else{
     NR_CSI_ReportConfig_t *csirep2 = calloc(1,sizeof(*csirep2));
     csirep2->reportConfigId=bwp->bwp_Id+10;
     csirep2->carrier=NULL;
     csirep2->resourcesForChannelMeasurement=bwp->bwp_Id+20;
     csirep2->csi_IM_ResourcesForInterference=NULL;
     csirep2->nzp_CSI_RS_ResourcesForInterference=NULL;
     csirep2->reportConfigType.present = NR_CSI_ReportConfig__reportConfigType_PR_periodic;
     csirep2->reportConfigType.choice.periodic = calloc(1,sizeof(*csirep2->reportConfigType.choice.periodic));
     csirep2->reportConfigType.choice.periodic->reportSlotConfig.present=NR_CSI_ReportPeriodicityAndOffset_PR_slots320;
     csirep2->reportConfigType.choice.periodic->reportSlotConfig.choice.slots320 = 39 + (20 * uid) % 320;
     ASN_SEQUENCE_ADD(&csirep2->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list,pucchcsires1);
     csirep2->reportQuantity.present = NR_CSI_ReportConfig__reportQuantity_PR_ssb_Index_RSRP;
     csirep2->reportQuantity.choice.ssb_Index_RSRP=(NULL_t)0;
     csirep2->reportFreqConfiguration = NULL;
     csirep2->timeRestrictionForChannelMeasurements= NR_CSI_ReportConfig__timeRestrictionForChannelMeasurements_configured;
     csirep2->timeRestrictionForInterferenceMeasurements=NR_CSI_ReportConfig__timeRestrictionForInterferenceMeasurements_configured;
     csirep2->codebookConfig= NULL;
     csirep2->dummy = NULL;
     csirep2->groupBasedBeamReporting.present = NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled;
     csirep2->groupBasedBeamReporting.choice.disabled=calloc(1,sizeof(*csirep2->groupBasedBeamReporting.choice.disabled));
     csirep2->groupBasedBeamReporting.choice.disabled->nrofReportedRS = calloc(1,sizeof(*csirep2->groupBasedBeamReporting.choice.disabled->nrofReportedRS));
     *csirep2->groupBasedBeamReporting.choice.disabled->nrofReportedRS=NR_CSI_ReportConfig__groupBasedBeamReporting__disabled__nrofReportedRS_n1;
     csirep2->cqi_Table = NULL;
     csirep2->subbandSize = NR_CSI_ReportConfig__subbandSize_value1;
     csirep2->non_PMI_PortIndication = NULL;
     csirep2->ext1 = NULL;
     ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_ReportConfigToAddModList->list,csirep2);
   }
 }
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->sCellDeactivationTimer=NULL;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->crossCarrierSchedulingConfig=NULL;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->tag_Id=0;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->dummy1=NULL;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->pathlossReferenceLinking=NULL;
 secondaryCellGroup->spCellConfig->spCellConfigDedicated->servingCellMO=NULL;

  *servingcellconfigdedicated = *secondaryCellGroup->spCellConfig->spCellConfigDedicated;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_SpCellConfig, (void *)secondaryCellGroup->spCellConfig);
  }
}

void fill_default_reconfig(NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                           NR_ServingCellConfig_t *servingcellconfigdedicated,
                           NR_RRCReconfiguration_IEs_t *reconfig,
                           NR_CellGroupConfig_t *secondaryCellGroup,
                           NR_UE_NR_Capability_t *uecap,
                           const gNB_RrcConfigurationReq *configuration,
                           int uid) {
  AssertFatal(servingcellconfigcommon!=NULL,"servingcellconfigcommon is null\n");
  AssertFatal(reconfig!=NULL,"reconfig is null\n");
  AssertFatal(secondaryCellGroup!=NULL,"secondaryCellGroup is null\n");
  // radioBearerConfig
  reconfig->radioBearerConfig=NULL;
  // secondaryCellGroup
  fill_default_secondaryCellGroup(servingcellconfigcommon,
                                  servingcellconfigdedicated,
                                  secondaryCellGroup,
                                  uecap,
                                  1,
                                  1,
                                  configuration,
                                  uid);

  xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void*)secondaryCellGroup);

  char scg_buffer[1024];
  asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig, NULL, (void *)secondaryCellGroup, scg_buffer, 1024);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n", enc_rval.failed_type->name, enc_rval.encoded);
  reconfig->secondaryCellGroup = calloc(1,sizeof(*reconfig->secondaryCellGroup));
  OCTET_STRING_fromBuf(reconfig->secondaryCellGroup,
                       (const char *)scg_buffer,
                       (enc_rval.encoded+7)>>3);
  // measConfig
  reconfig->measConfig=NULL;
  // lateNonCriticalExtension
  reconfig->lateNonCriticalExtension = NULL;
  // nonCriticalExtension
  reconfig->nonCriticalExtension = NULL;
}

void fill_default_rbconfig(NR_RadioBearerConfig_t *rbconfig,
                          int eps_bearer_id, int rb_id,
                           e_NR_CipheringAlgorithm ciphering_algorithm,
                           e_NR_SecurityConfig__keyToUse key_to_use) {

  rbconfig->srb_ToAddModList = NULL;
  rbconfig->srb3_ToRelease = NULL;
  rbconfig->drb_ToAddModList = calloc(1,sizeof(*rbconfig->drb_ToAddModList));
  NR_DRB_ToAddMod_t *drb_ToAddMod = calloc(1,sizeof(*drb_ToAddMod));
  drb_ToAddMod->cnAssociation = calloc(1,sizeof(*drb_ToAddMod->cnAssociation));
  drb_ToAddMod->cnAssociation->present = NR_DRB_ToAddMod__cnAssociation_PR_eps_BearerIdentity;
  drb_ToAddMod->cnAssociation->choice.eps_BearerIdentity= eps_bearer_id;
  drb_ToAddMod->drb_Identity = rb_id;
  drb_ToAddMod->reestablishPDCP = NULL;
  drb_ToAddMod->recoverPDCP = NULL;
  drb_ToAddMod->pdcp_Config = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config));
  drb_ToAddMod->pdcp_Config->drb = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config->drb));
  drb_ToAddMod->pdcp_Config->drb->discardTimer = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config->drb->discardTimer));
  *drb_ToAddMod->pdcp_Config->drb->discardTimer=NR_PDCP_Config__drb__discardTimer_infinity;
  drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeUL = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeUL));
  *drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeUL = NR_PDCP_Config__drb__pdcp_SN_SizeUL_len18bits;
  drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeDL = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeDL));
  *drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeDL = NR_PDCP_Config__drb__pdcp_SN_SizeDL_len18bits;
  drb_ToAddMod->pdcp_Config->drb->headerCompression.present = NR_PDCP_Config__drb__headerCompression_PR_notUsed;
  drb_ToAddMod->pdcp_Config->drb->headerCompression.choice.notUsed = 0;

  drb_ToAddMod->pdcp_Config->drb->integrityProtection=NULL;
  drb_ToAddMod->pdcp_Config->drb->statusReportRequired=NULL;
  drb_ToAddMod->pdcp_Config->drb->outOfOrderDelivery=NULL;
  drb_ToAddMod->pdcp_Config->moreThanOneRLC = NULL;

  drb_ToAddMod->pdcp_Config->t_Reordering = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config->t_Reordering));
  *drb_ToAddMod->pdcp_Config->t_Reordering = NR_PDCP_Config__t_Reordering_ms100;
  drb_ToAddMod->pdcp_Config->ext1 = NULL;

  ASN_SEQUENCE_ADD(&rbconfig->drb_ToAddModList->list,drb_ToAddMod);

  rbconfig->drb_ToReleaseList = NULL;

  rbconfig->securityConfig = calloc(1,sizeof(*rbconfig->securityConfig));
  rbconfig->securityConfig->securityAlgorithmConfig = calloc(1,sizeof(*rbconfig->securityConfig->securityAlgorithmConfig));
  rbconfig->securityConfig->securityAlgorithmConfig->cipheringAlgorithm = ciphering_algorithm;
  rbconfig->securityConfig->securityAlgorithmConfig->integrityProtAlgorithm=NULL;
  rbconfig->securityConfig->keyToUse = calloc(1,sizeof(*rbconfig->securityConfig->keyToUse));
  *rbconfig->securityConfig->keyToUse = key_to_use;

//  xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void*)rbconfig);
}
/* Function to set or overwrite PTRS DL RRC parameters */
void rrc_config_dl_ptrs_params(NR_BWP_Downlink_t *bwp, int *ptrsNrb, int *ptrsMcs, int *epre_Ratio, int * reOffset)
{
  int i=0;
  /* check for memory allocation  */
  if(bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS == NULL) {
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS=calloc(1,sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS));
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->present = NR_SetupRelease_PTRS_DownlinkConfig_PR_setup;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup= calloc(1, sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup));
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->frequencyDensity = calloc(1,sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->frequencyDensity));
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->timeDensity = calloc(1, sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->timeDensity));
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->epre_Ratio = calloc(1,sizeof(long));
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->resourceElementOffset = calloc(1,sizeof(long));
    /* Fill the given values */
    for(i = 0; i < 2; i++) {
      ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->frequencyDensity->list,&ptrsNrb[i]);
    }
    for(i = 0; i < 3; i++) {
      ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->timeDensity->list,&ptrsMcs[i]);
    }
  }// if memory exist then over write the old values
  else {
    for(i = 0; i < 2; i++) {
      *bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->frequencyDensity->list.array[i] = ptrsNrb[i];
    }
    for(i = 0; i < 3; i++) {
      *bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->timeDensity->list.array[i] = ptrsMcs[i];
    }
  }

  *bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->epre_Ratio = *epre_Ratio;
  *bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->resourceElementOffset = *reOffset;
}
#endif
