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
#define RRC_GNB_NSA_C
#define RRC_GNB_NSA_C

#include "NR_RRCReconfiguration.h"

void fill_default_secondaryCellGroup(NR_ServingCellConfigCommon_t *servingcellconfigcommon,
				     RRCReconfiguration_IEs_t *reconfig,
				     NR_CellGroupConfig_t *secondaryCellGroup,
				     int scg_id,
				     int servCellIndex) {
  AssertFatal(servingcellconfigcommon!=NULL,"servingcellconfigcommon is null\n");
  AssertFatal(reconfig!=NULL,"reconfig is null\n");
  AssertFatal(secondaryCellGroup!=NULL,"secondaryCellGroup is null\n");

  memset(secondaryCellGroup,0,sizeof(NR_CellGroupConfig_t));
  secondaryCellGroup->cellGroupId = scg_id;

  RLC_BearerConfig_t *RLC_BearerConfig = calloc(1,sizeof(RLC_BearerConfig_t));

  RLC_BearerConfig->logicalChannelIdentity = 4;
  RLC_BearerConfig->servedRadioBearer = calloc(1,sizeof(struct NR_RLC_BearerConfig__servedRadioBearer));
  RLC_BearerConfig->servedRadioBearer->present = 	NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity;

  RLC_BearerConfig->servedRadioBearer.choice.drb_Identity=5;
  RLC_BearerConfig->reestablishRLC=calloc(1,sizeof(long));
  *RLC_BearerConfig->reestablishRLC=NR_RLC_BearerConfig__reestablishRLC_true;
  RLC_BearerConfig->rlc_Config=calloc(1,sizeof(struct NR_RLC_Config));
  RLC_BearerConfig->rlc_Config->present = NR_RLC_Config_PR_am;
  RLC_BearerConfig->rlc_Config->choice.am = calloc(1,sizeof(struct NR_RLC_Config__am));
  RLC_BearerConfig->rlc_Config->choice.am->ul_AM_RLC.sn_FiledLength = calloc(1,sizeof(NR_SN_FieldLengthAM_t));
  *RLC_BearerConfig->rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength   =	NR_SN_FieldLengthAM_size18;
  RLC_BearerConfig->rlc_Config->choice.am->ul_AM_RLC.t_PollRetransmit = NR_T_PollRetransmit_ms45;
  RLC_BearerConfig->rlc_Config->choice.am->ul_AM_RLC.pollPDU          = NR_PollPDU_p64;
  RLC_BearerConfig->rlc_Config->choice.am->ul_AM_RLC.pollByte         = NR_PollByte_kB500;
  RLC_BearerConfig->rlc_Config->choice.am->ul_AM_RLC.maxRetxThreshold = R_UL_AM_RLC__maxRetxThreshold_t32;

  RLC_BearerConfig->rlc_Config->choice.am->dl_AM_RLC.sn_FiledLength = calloc(1,sizeof(NR_SN_FieldLengthAM_t));
  *RLC_BearerConfig->rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength = NR_SN_FieldLengthAM_size18;
  RLC_BearerConfig->rlc_Config->choice.am->dl_AM_RLC.t_Reassembly   = NR_T_Reassembly_ms15;
  RLC_BearerConfig->rlc_Config->choice.am->dl_AM_RLC.t_StatusProhibit NR_T_StatusProhibit_ms15;

  RLC_BearerConfig->mac_LogicalChannelConfig = calloc(1,sizeof(struct NR_LogicalChannelConfig));
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.priority            = 1;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.prioritisedBitRate  = NR_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.bucketSizeDuration  = NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.allowedServingCells = NULL;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.allowedSCS_List     = NULL;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.maxPUSCH_Duration   = NULL;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.configuredGrantType1Allowed = NULL;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.logicalChannelGroup   = calloc(1,sizeof(long));
  *RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.logicalChannelGroup  = 1;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.schedulingRequestID   = NULL;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.logicalChannelSR_Mask = false;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.logicalChannelSR_DelayTimerApplied = false;
  RLC_BearerConfig->mac_LogicalChannelConfig->ul_SpecificParameters.bitRateQueryProhibitTimer   = NULL;

  secondaryCellGroup->rlc_BearerToAddModList = calloc(1,sizeof(struct NR_CellGroupConfig__rlc_BearerToAddModList));
  ASN_SEQUENCE_ADD(&secondaryCellGroup->rlc_BearerToAddModList->list, RLC_BearerConfig);

  secondaryCellGroup->mac_CellGroupConfig=calloc(1,sizeof(NR_MAC_CellGroupConfig));
  secondaryCellGroup->mac_CellGroupConfig->drx_Config = NULL;
  secondaryCellGroup->mac_CellGroupConfig->schedulingRequestConfig = NULL;
  secondaryCellGroup->mac_CellGroupConfig->bsr_Config=calloc(1,sizeof(NR_BSR_Config));
  secondaryCellGroup->mac_CellGroupConfig->bsr_Config->periodicBSR_Timer = NR_BSR_Config__periodicBSR_Timer_sf80;
  secondaryCellGroup->mac_CellGroupConfig->bsr_Config->retxBSR_Timer     = sf320;
  secondaryCellGroup->mac_CellGroupConfig->tag_Config=calloc(1,sizeof(NR_TAG_Config));
  secondaryCellGroup->mac_CellGroupConfig->tag_Config->tag_ToReleaseList = NULL;
  secondaryCellGroup->mac_CellGroupConfig->tag_Config->tag_ToAddModList  = calloc(1,sizeof(struct NR_TAG_Config__tag_ToAddModList));
  struct NR_TAG *tag=calloc(1,sizeof(struct NR_TAG));
  tag->tag_Id             = 0;
  tag->timeAlignmentTimer = NR_TimeAlignmentTimer_infinity;
  ASN_SEQUENCE_ADD(&secondaryCellGroup->mac_CellGroupConfig->tab_Config->tag_ToAddModList->list,tag);
  secondaryCellGroup->mac_CellGroupConfig->phr_Config  = calloc(1,sizeof(struct NR_SetupRelease_PHR_Config));
  secondaryCellGroup->mac_CellGroupConfig->phr_Config->present = NR_SetupRelease_PHR_Config_PR_setup;
  secondaryCellGroup->mac_CellGroupConfig->phr_Config->choice.setup  = calloc(1,sizeof(struct NR_PHR_Config));
  secondaryCellGroup->mac_CellGroupConfig->phr_Config->choice.setup->phr_PeriodicTimer = NR_PHR_Config__phr_PeriodicTimer_sf20;
  secondaryCellGroup->mac_CellGroupConfig->phr_Config->choice.setup->phr_ProhibitTimer = NR_PHR_Config__phr_ProhibitTimer_sf0;
  secondaryCellGroup->mac_CellGroupConfig->phr_Config->choice.setup->phr_Tx_PowerFactorChange = NR_PHR_Config__phr_Tx_PowerFactorChange_dB3;
  secondaryCellGroup->mac_CellGroupConfig->phr_Config->choice.setup->multiplePHR=false;
  secondaryCellGroup->mac_CellGroupConfig->phr_Config->choice.setup->dummy=false;
  secondaryCellGroup->mac_CellGroupConfig->phr_Config->choice.setup->phr_Type2OtherCell = false;
  secondaryCellGroup->mac_CellGroupConfig->phr_Config->choice.setup->phr_ModeOtherCG = NR_PHR_Config__phr_ModeOtherCG_real;

  secondaryCellGroup->mac_CellGroupConfig->skipUplinkTxDynamic=false;
  secondaryCellGroup->mac_CellGroupConfig->ext1 = NULL;

  secondaryCellGroup->physicalCellGroupConfig = calloc(1,sizeof(struct NR_PhysicalCellGroupConfig));

  secondaryCellGroup->physicalCellGroupConfig->harq_ACK_SpatialBundlingPUCCH=NULL;
  secondaryCellGroup->physicalCellGroupConfig->harq_ACK_SpatialBundlingPUSCH=NULL;
  secondaryCellGroup->physicalCellGroupConfig->p_NR_FR1=calloc(1,sizeof(NR_P_Max_t));
  *secondaryCellGroup->physicalCellGroupConfig->p_NR_FR1=20;
  secondaryCellGroup->physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook=NR_PhysicalCellGroupConfig__pdsch_HARQ_ACK_Codebook_dynamic;  
  secondaryCellGroup->physicalCellGroupConfig->tpc_SRS_RNTI=NULL;
  secondaryCellGroup->physicalCellGroupConfig->tpc_PUCCH_RNTI=NULL;
  secondaryCellGroup->physicalCellGroupConfig->tpc_PUSCH_RNTI=NULL;
  secondaryCellGroup->physicalCellGroupConfig->sp_CSI_RNTI=NULL;
  secondaryCellGroup->physicalCellGroupConfig->cs_RNTI=NULL;
  secondaryCellGroup->physicalCellGroupConfig->ext1=NULL;

  secondaryCellGroup->spCellConfig = calloc(1,sizeof(struct NR_SpCellConfig));
  secondaryCellGroup->spCellConfig->servCellIndex = calloc(1,sizeof(NR_ServCellIndex_t));
  *secondaryCellGroup->spCellConfig->servCellIndex = servCellIndex;
  secondaryCellGroup->spCellConfig->reconfigurationWithSync=calloc(1,sizeof(struct NR_ReconfigurationWithSync));
  secondaryCellGroup->spCellConfig->reconfigurationWithSync->spCellConfigCommon=servingcellconfigcommon;
  secondaryCellGroup->spCellConfig->reconfigurationWithSync->newUE_Identity=taus()&0xffff;
  secondaryCellGroup->spCellConfig->reconfigurationWithSync->t304=NR_ReconfigurationWithSync__t304_ms2000;
  secondaryCellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated = NULL;
  secondaryCellGroup->spCellConfig->reconfigurationWithSync->ext1                 = NULL;

  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants = calloc(1,sizeof(NR_SetupRelease_RLF_TimersAndConstants));
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->present = NR_SetupRelease_RLF_TimersAndConstants_PR_setup;
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup=calloc(1,sizeof(struct NR_RLF_TimersAndConstants));
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->t310 = NR_RLF_TimersAndConstants__t310_ms2000;
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->n310 = NR_RLF_TimersAndConstants__n310_n10;
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->n311 = NR_RLF_TimersAndConstants__n311_n1;
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->ext1 = calloc(1,sizeof(struct NR_RLF_TimersAndConstants__ext1));
  secondaryCellGroup->spCellConfig->rlf_TimersAndConstants->choice.setup->ext1->t311_v1530 = NR_RLF_TimersAndConstants__ext1__t311_v1530_ms30000;

  secondaryCellGroup->spCellConfig->rlmInSyncOutOfSyncThreshold                   = NULL;
 


  secondaryCellGroup->spCellConfig->spcellConfigDedicated = calloc(1,sizeof(struct NR_ServingCellConfig));
  secondaryCellGroup->spCellConfig->spcellConfigDedicated->tdd_UL_DL_ConfigurationDedicated = NULL;
  secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP = calloc(1,sizeof(struct NR_BWP_DownlinkDedicated));

  secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdcch_Config=NULL;
  secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config=calloc(1,sizeof(struct NR_SetupRelease_PDSCH_Config));
  secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->present = NR_SetupRelease_PDSCH_Config_PR_setup;
  secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup = calloc(1,sizeof(struct NR_PDSCH_Config));
  secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dataScramblingIdentityPDSCH = NULL;
  secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dmrs_DownlinkForPDSCH_MappingTypeA = calloc(1,sizeof(struct NR_SetupRelease_DMRS_DownlinkConfig));
  secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dmrs_DownlinkForPDSCH_MappingTypeA->present= NR_SetupRelease_DMRS_DownlinkConfig_PR_setup;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup = calloc(1,sizeof(struct NR_DMRS_DownlinkConfig));

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->maxLength=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->scramblingID0=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->scramblingID1=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS=NULL;

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = calloc(1,sizeof(long));
 *secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0;

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList=calloc(1,sizeof(NR_PDSCH_Config__tci_StatesToAddModList));

 NR_TCI_State *tci0=calloc(1,sizeof(NR_TCI_State));
 tci0->tci_StateId=0;
 tci0->qcl_Type1.cell=NULL;
 tci0->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci0->qcl_Type1.bwp_Id=1;
 tci0->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tci0->qcl_Type1.referenceSignal.choice.csi_rs = 2;
 tci0->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci0);

 NR_TCI_State *tci1=calloc(1,sizeof(NR_TCI_State));
 tci1->tci_StateId=1;
 tci1->qcl_Type1.cell=NULL;
 tci1->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci1->qcl_Type1.bwp_Id=1;
 tci1->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tci1->qcl_Type1.referenceSignal.choice.csi_rs = 6;
 tci1->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci1);

 NR_TCI_State *tci2=calloc(1,sizeof(NR_TCI_State));
 tci2->tci_StateId=2;
 tci2->qcl_Type1.cell=NULL;
 tci2->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci2->qcl_Type1.bwp_Id=1;
 tci2->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tci2->qcl_Type1.referenceSignal.choice.csi_rs = 10;
 tci2->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci2);

 NR_TCI_State *tci3=calloc(1,sizeof(NR_TCI_State));
 tci3->tci_StateId=3;
 tci3->qcl_Type1.cell=NULL;
 tci3->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci3->qcl_Type1.bwp_Id=1;
 tci3->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tci3->qcl_Type1.referenceSignal.choice.csi_rs = 14;
 tci3->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci3);

 NR_TCI_State *tci4=calloc(1,sizeof(NR_TCI_State));
 tci4->tci_StateId=4;
 tci4->qcl_Type1.cell=NULL;
 tci4->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci4->qcl_Type1.bwp_Id=1;
 tci4->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tci4->qcl_Type1.referenceSignal.choice.csi_rs = 18;
 tci4->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci4);

 NR_TCI_State *tci5=calloc(1,sizeof(NR_TCI_State));
 tci5->tci_StateId=5;
 tci5->qcl_Type1.cell=NULL;
 tci5->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci5->qcl_Type1.bwp_Id=1;
 tci5->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tci5->qcl_Type1.referenceSignal.choice.csi_rs = 22;
 tci5->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci5);

 NR_TCI_State *tci6=calloc(1,sizeof(NR_TCI_State));
 tci6->tci_StateId=6;
 tci6->qcl_Type1.cell=NULL;
 tci6->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci6->qcl_Type1.bwp_Id=1;
 tci6->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tci6->qcl_Type1.referenceSignal.choice.csi_rs = 26;
 tci6->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci6);

 NR_TCI_State *tci7=calloc(1,sizeof(NR_TCI_State));
 tci7->tci_StateId=7;
 tci7->qcl_Type1.cell=NULL;
 tci7->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci7->qcl_Type1.bwp_Id=1;
 tci7->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tci7->qcl_Type1.referenceSignal.choice.csi_rs = 30;
 tci7->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci7);

 NR_TCI_State *tci8=calloc(1,sizeof(NR_TCI_State));
 tci8->tci_StateId=8;
 tci8->qcl_Type1.cell=NULL;
 tci8->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci8->qcl_Type1.bwp_Id=1;
 tci8->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tci8->qcl_Type1.referenceSignal.choice.ssb = 0;
 tci8->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci8);

 NR_TCI_State *tci9=calloc(1,sizeof(NR_TCI_State));
 tci9->tci_StateId=9;
 tci9->qcl_Type1.cell=NULL;
 tci9->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci9->qcl_Type1.bwp_Id=1;
 tci9->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tci9->qcl_Type1.referenceSignal.choice.ssb = 1;
 tci9->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci9);

 NR_TCI_State *tci10=calloc(1,sizeof(NR_TCI_State));
 tci10->tci_StateId=10;
 tci10->qcl_Type1.cell=NULL;
 tci10->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci10->qcl_Type1.bwp_Id=1;
 tci10->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tci10->qcl_Type1.referenceSignal.choice.ssb = 2;
 tci10->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci10);

 NR_TCI_State *tci11=calloc(1,sizeof(NR_TCI_State));
 tci11->tci_StateId=11;
 tci11->qcl_Type1.cell=NULL;
 tci11->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci11->qcl_Type1.bwp_Id=1;
 tci11->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tci11->qcl_Type1.referenceSignal.choice.ssb = 3;
 tci11->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci11);

 NR_TCI_State *tci12=calloc(1,sizeof(NR_TCI_State));
 tci12->tci_StateId=12;
 tci12->qcl_Type1.cell=NULL;
 tci12->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci12->qcl_Type1.bwp_Id=1;
 tci12->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tci12->qcl_Type1.referenceSignal.choice.ssb = 4;
 tci12->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci12);

 NR_TCI_State *tci13=calloc(1,sizeof(NR_TCI_State));
 tci13->tci_StateId=13;
 tci13->qcl_Type1.cell=NULL;
 tci13->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci13->qcl_Type1.bwp_Id=1;
 tci13->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tci13->qcl_Type1.referenceSignal.choice.ssb = 5;
 tci13->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci13);

 NR_TCI_State *tci14=calloc(1,sizeof(NR_TCI_State));
 tci14->tci_StateId=14;
 tci14->qcl_Type1.cell=NULL;
 tci14->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci14->qcl_Type1.bwp_Id=1;
 tci14->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tci14->qcl_Type1.referenceSignal.choice.ssb = 6;
 tci14->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci14);

 NR_TCI_State *tci15=calloc(1,sizeof(NR_TCI_State));
 tci15->tci_StateId=15;
 tci15->qcl_Type1.cell=NULL;
 tci15->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tci15->qcl_Type1.bwp_Id=1;
 tci15->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tci15->qcl_Type1.referenceSignal.choice.ssb = 7;
 tci15->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToAddModList->list,tci15);

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->tci_StatesToReleaseList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->vrb_ToPRB_Interleaver=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->resourceAllocation=NR_PDSCH_Config__resourceAllocation_resourceAllocationType0;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->TimeDomainAllocationList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->pdsch_AggregationFactor=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->rateMatchPatternToAddModList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->rateMatchPatternToReleaseList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->rateMatchPatternGroup1=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->rateMatchPatternGroup2=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->rbg_Size=NR_PDSCH_Config__rbg_Size_config1;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->mcs_Table=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->maxNrofCodeWordsScheduledByDCI = calloc(1,sizeof(long));
 *secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->maxNrofCodeWordsScheduledByDCI = NR_PDSCH_Config__maxNrofCodeWordsScheduledByDCI_n1;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->prb_BundlingType.present = NR_PDSCH_Config__prb_BundlingType_PR_staticBundling;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->prb_BundlingType.staticBundling = calloc(1,sizeof(struct NR_PDSCH_Config__prb_BundlingType__staticBundling));
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->prb_BundlingType.staticBundling->bundleSize =
   calloc(1,sizeof(long));
 *secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->prb_BundlingType.staticBundling->bundleSize = NR_PDSCH_Config__prb_BundlingType__staticBundling__bundleSize_wideband;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->zp_CSI_RS_ResourceToAddModList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->zp_CSI_RS_ResourceToReleaseList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->aperiodic_ZP_CSI_RS_ResourceToAddModList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->aperiodic_ZP_CSI_RS_ResourceToReleaseList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->sp_ZP_CSI_RS_ResourceToAddModList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->sp_ZP_CSI_RS_ResourceToReleaseList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->pdsch_Config->setup->p_ZP_CSI_RS_ResourceSet=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->sps_Config = NULL;

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig = calloc(1,sizeof(struct NR_SetupRelease_RadioLinkMonitoringConfig));
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->present = NR_SetupRelease_RadioLinkMonitoringConfig_PR_setup;

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->setup = calloc(1,sizeof(struct NR_RadioLinkMonitoringConfig));
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->setup->failureDetectionResourcesToAddModList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->setup->failureDetectionResourcesToReleaseList=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->setup->beamFailureInstanceMaxCount = calloc(1,sizeof(long));
 *secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->setup->beamFailureInstanceMaxCount = NR_RadioLinkMonitoringConfig__beamFailureInstanceMaxCount_n3;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->setup->beamFailureDetectionTimer = calloc(1,sizeof(long));
 *secondaryCellGroup->spCellConfig->spcellConfigDedicated->initialDownlinkBWP->radioLinkMonitoringConfig->setup->beamFailureDetectionTimer = NR_RadioLinkMonitoringConfig__beamFailureDetectionTimer_pbfd2;

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->downlinkBWP_ToReleaseList= NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->downlinkBWP_ToAddModList = calloc(1,sizeof(struct NR_ServingCellConfig__downlinkBWP_ToAddModList));

 NR_BWP_Downlink_t *bwp=calloc(1,sizeof(NR_BWP_Downlink_t));
 bwp->bwp_Id=1;
 bwp->bwp_Common=calloc(1,sizeof(NR_BWP_DownlinkCommon_t));
 // copy common BWP size from initial BWP except for bandwdith
 memcpy((void*)&bwp->bwp_Common->genericParameters,servingcellconfigcommon->downlinkConfigCommon->genericParameters,
	sizeof(bwp->bwp_Common->genericParameters));
 bwp->bwp_Common->genericParameters.locationandbandwidth=PRBalloc_to_locationanbandwidth(scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,0);
 bwp->bwp_Common->pdcch_ConfigCommon=calloc(1,sizeof(NR_SetupRelease_PDCCH_ConfigCommon_t));
 bwp->bwp_Common->pdcch_ConfigCommon->present = NR_SetupRelease_PDCCH_ConfigCommon_PR_setup;
 bwp->bwp_Common->pdcch_ConfigCommon->choice.setup = calloc(1,sizeof(NR_PDCCH_ConfigCommon_t));
 bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->controlResourceSetZero=NULL;
 bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonControlResrouceSet=NULL;
 bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->searchSpaceZero=NULL;
 bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList=calloc(1,sizeof(struct NR_PDCCH_ConfigCommon__commonSearchSpaceList));

 NR_SearchSpace *ss=calloc(1,sizeof(NR_SearchSpace_t));
 ss->searchSpaceId = 1;
 ss->controlResourceSetId=calloc(1,sizeof(NR_ControlResourceSetId_t));
 *ss->controlResrouceSetId=0;
 ss->monitoringSlotPeriodicityAndOffset = calloc(1,sizeof(struct NR_SearchSpace__monitoringSlotPeriodicityAndOffset));
 ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
 ss->duration=NULL; 
 ss->monitoringSymbolsWithinSlot = calloc(1,sizeof(BIT_STRING_t));
 ss->monitoringSymbolsWithinSlot->buf = calloc(1,2);
 // should be '1100 0000 0000 00'B (LSB first!), first two symols in slot, adjust if needed
 ss->monitoringSymbolsWithinSlot->buf[0] = 0x3;
 ss->monitoringSymbolsWithinSlot->buf[1] = 0;
 ss->monitoringSymbolsWithinSlot->size = 2;
 ss->monitoringSymbolsWithinSlot->bits_unused = 2;
 ss->nrofCandidates = calloc(1,sizeof(struct NR_SearchSpace__nrofCandidates));
 ss->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
 ss->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n0;
 ss->nrofCandidates->aggregationLevel4 = NR_SearchSpace__nrofCandidates__aggregationLevel4_n1;
 ss->nrofCandidates->aggregationLevel8 = NR_SearchSpace__nrofCandidates__aggregationLevel8_n0;
 ss->nrofCandidates->aggregationLevel16 = NR_SearchSpace__nrofCandidates__aggregationLevel16_n0;
 ss->searchSpaceType = calloc(1,sizeof(struct NR_SearchSpace__searchSpaceType));
 ss->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_common;
 ss->searchSpaceType->choice.common=calloc(1,sizeof(struct NR_SearchSpace__searchSpaceType__common));
 ss->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0 = calloc(1,sizeof(struct NR_SearchSpace__searchSpaceType__common__dci_Format0_0_AndFormat1_0));

 ASN_SEQUENCE_ADD(&bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList->list,ss);
 bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->searchSpaceSIB1=NULL;
 bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->searchSpaceOtherSystemInformation=NULL;
 bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->pagingSearchSpace=NULL;
 bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->ra_SearchSpace=calloc(1,sizeof(NR_SearchSpaceId_t));
 *bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->ra_SearchSpace=1;

 bwp->bwp_Common->pdsch_ConfigCommon=calloc(1,sizeof(NR_SetupRelease_PDSCH_ConfigCommon_t));
 bwp->bwp_Common->pdsch_ConfigCommon->present = NR_SetupRelease_PDSCH_ConfigCommon_PR_setup;
 bwp->bwp_Common->pdsch_ConfigCommon->choice.setup = calloc(1,sizeof(NR_PDSCH_ConfigCommon_t));
 bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList = calloc(1,sizeof(NR_PDSCH_TimeDomainResourceAllocationList_t));

 // copy PDSCH TimeDomainResourceAllocation from InitialBWP
 NR_PDSCH_TimeDomainResourceAllocation_t *pdschi;
 for (int i=0;i<servingcellconfigcommon->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.size;i++) {
   pdschi= calloc(1,sizeof(*pdsch0));
   
   if (servingcellconfigcommon->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0) {
     pdschi->k0 = calloc(1,sizeof(*pdsch0->k0));
     *pdschi->k0 = *servingcellconfigcommon->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0;
   }  
   pdschi->mappingType = servingcellconfigcommon->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->mappingType;
   pdschi->startSymbolAndLength = servingcellconfigcommon->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
   ASN_SEQUENCE_ADD(&bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list,pdschi);
 }

 bwp->bwp_Dedicated=calloc(1,sizeof(NR_BWP_DownlinkDedicated_t));

 bwp->bwp_Dedicated->pdcch_Config=calloc(1,sizeof(NR_SetupRelease_PDCCH_Config_t));
 bwp->bwp_Dedicated->pdcch_Config->present = NR_SetupRelease_PDCCH_Config_PR_setup;
 bwp->bwp_Dedicated->pdcch_Config->choice.setup = calloc(1,sizeof(NR_PDCCH_Config_t));
 bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList = calloc(1,sizeof(struct NR_PDCCH_Config__controlResourceSetToAddModList));
 NR_ControlResourceSet *coreset0 = calloc(1,sizeof(*coreset0));
 coreset0->controlResourceSetId=1; 
 // frequencyDomainResources '11111111 11111111 00000000 00000000 00000000 00000'B,
 coreset0->frequencyDomainResources = calloc(1,sizeof(BIT_STRING_t));
 coreset0->frequencyDomainResources->buf = calloc(1,6);
 // should be '1100 0000 0000 00'B (LSB first!), first two symols in slot, adjust if needed
 coreset0->frequencyDomainResources->buf[0] = 0xff;
 coreset0->frequencyDomainResources->buf[1] = 0xff;
 coreset0->frequencyDomainResources->buf[2] = 0;
 coreset0->frequencyDomainResources->buf[3] = 0;
 coreset0->frequencyDomainResources->buf[4] = 0;
 coreset0->frequencyDomainResources->buf[5] = 0;
 coreset0->frequencyDomainResources->size = 6;
 coreset0->frequencyDomainResources->bits_unused = 3;
 coreset0->duration=1;
 coreset0->cce_REG_MappingType.present = NR_ControlResourceSet__cce_REG_MappingType_PR_nonInterleaved;
 coreset0->precoderGranularity = NR_ControlResourceSet__precoderGranularity_sameAsREG_bundle;

 coreset0->tci_StatesPDCCH_ToAddList=calloc(1,sizeof(struct NR_ControlResourceSet__tci_StatesPDCCH_ToAddList));
 NR_TCI_StateId_t *tci[8];
 for (int i=0;i<8;i++) {
   tci[i]=calloc(1,sizeof(NR_TCI_StateId_t));
   *tci[i] = i;
   ASN_SEQUENCE_ADD(&coreset0->tci_StatesPDCCH_ToAddList->list,tci[i]);
 }
 coreset0->tci_StatesPDCCH_ToReleaseList = NULL;
 coreset0->tci_PresentinDCI = NULL;
 coreset0->pdcch_DMRS_ScramblingID = NULL;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list,
		  coreset0);

 bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacestToAddModList = calloc(1,sizeof(struct NR_PDCCH_Config__searchSpacesToAddModList));
 NR_SearchSpace_t *ss3 = calloc(1,sizeof(NR_SearchSpace_t));
 NR_SearchSpace_t *ss2 = calloc(1,sizeof(NR_SearchSpace_t));
 sse3->searchSpaceId=3;
 sse3->controlResourceSetId=calloc(1,sizeof(NR_ControlResourceSetId_t));
 *sse3->controlResourceSetId=1;
 sse3->monitoringSlotPeriodicityAndOffset=calloc(1,sizeof(*sse3->monitoringSlotPeriodicityAndOffset));
 sse3->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
 sse3->monitoringSlotPeriodicityAndOffset->choice.sl1=NULL;
 sse3->duration=NULL;
 sse3->monitoringSymbolsWithinSlot = calloc(1,sizeof(BIT_STRING_t));
 sse3->monitoringSymbolsWithinSlot->buf = calloc(1,2);
 sse3->monitoringSymbolsWithinSlot->size = 2;
 sse3->monitoringSymbolsWithinSlot->bits_unused = 2;
 sse3->monitoringSymbolsWithinSlot->buf[0]=0x3;
 sse3->monitoringSymbolsWithinSlot->buf[0]=0x0;
 sse3->nrofCandidates=calloc(1,sizeof(*sse3->nrofCandidates));
 sse3->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
 sse3->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n0;
 sse3->nrofCandidates->aggregationLevel4 = NR_SearchSpace__nrofCandidates__aggregationLevel4_n1;
 sse3->nrofCandidates->aggregationLevel8 = NR_SearchSpace__nrofCandidates__aggregationLevel8_n0;
 sse3->nrofCandidates->aggregationLevel16 = NR_SearchSpace__nrofCandidates__aggregationLevel16_n0;
 sse3->searchSpaceType=calloc(1,sizeof(*sse3->searchSpaceType));
 sse3->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_common;
 sse3->searchSpaceType->common = calloc(1,sizeof(*sse3->searchSpaceType->common));
 sse3->searchSpaceType->common->dci_Format0_0_AndFormat1_0=calloc(1,sizeof(*sse3->searchSpaceType->common->dci_Format0_0_AndFormat1_0));
 sse3->searchSpaceType->common->dci_Format2_0=NULL;
 sse3->searchSpaceType->common->dci_Format2_2=NULL;
 sse3->searchSpaceType->common->dci_Format2_3=NULL;

 sse2->searchSpaceId=2;
 sse2->controlResourceSetId=calloc(1,sizeof(NR_ControlResourceSetId_t));
 *sse2->controlResourceSetId=1;
 sse2->monitoringSlotPeriodicityAndOffset=calloc(1,sizeof(*sse2->monitoringSlotPeriodicityAndOffset));
 sse2->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
 sse2->monitoringSlotPeriodicityAndOffset->choice.sl1=NULL;
 sse2->duration=NULL;
 sse2->monitoringSymbolsWithinSlot = calloc(1,sizeof(BIT_STRING_t));
 sse2->monitoringSymbolsWithinSlot->buf = calloc(1,2);
 sse2->monitoringSymbolsWithinSlot->size = 2;
 sse2->monitoringSymbolsWithinSlot->bits_unused = 2;
 sse2->monitoringSymbolsWithinSlot->buf[0]=0x3;
 sse2->monitoringSymbolsWithinSlot->buf[0]=0x0;
 sse2->nrofCandidates=calloc(1,sizeof(*sse2->nrofCandidates));
 sse2->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
 sse2->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n0;
 sse2->nrofCandidates->aggregationLevel4 = NR_SearchSpace__nrofCandidates__aggregationLevel4_n4;
 sse2->nrofCandidates->aggregationLevel8 = NR_SearchSpace__nrofCandidates__aggregationLevel8_n0;
 sse2->nrofCandidates->aggregationLevel16 = NR_SearchSpace__nrofCandidates__aggregationLevel16_n0;
 sse2->searchSpaceType=calloc(1,sizeof(*sse2->searchSpaceType));
 sse2->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
 sse2->searchSpaceType->ue_Specific = calloc(1,sizeof(*sse2->searchSpaceType->ue_Specific));
 sse2->searchSpaceType->ue_Specific->dci_Formats=NR_SearchSpace__searchSpaceType__ue_Specific__dci_Formats_formats0_0_And_1_0;

 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacestToAddModList->list,
		  sse3);
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacestToAddModList->list,
		  sse2);


 bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacestToReleaseList = NULL;



  bwp->bwp_Dedicated->pdsch_Config->present = NR_SetupRelease_PDSCH_Config_PR_setup;
  bwp->bwp_Dedicated->pdsch_Config->choice.setup = calloc(1,sizeof(struct NR_PDSCH_Config));
  bwp->bwp_Dedicated->pdsch_config->choice.setup->dataScramblingIdentityPDSCH = NULL;
  bwp->bwp_Dedicated->pdsch_config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA = calloc(1,sizeof(struct NR_SetupRelease_DMRS_DownlinkConfig));
  bwp->bwp_Dedicated->pdsch_config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->present= NR_SetupRelease_DMRS_DownlinkConfig_PR_setup;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup = calloc(1,sizeof(struct NR_DMRS_DownlinkConfig));

 bwp->bwp_Dedicated->pdsch_config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->maxLength=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->scramblingID0=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->scramblingID1=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS=NULL;

 bwp->bwp_Dedicated->pdsch_config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = calloc(1,sizeof(long));
 *bwp->bwp_Dedicated->pdsch_config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0;

 bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList=calloc(1,sizeof(NR_PDSCH_Config__tci_StatesToAddModList));

 NR_TCI_State *tcid0=calloc(1,sizeof(NR_TCI_State));
 tcid0->tci_StateId=0;
 tcid0->qcl_Type1.cell=NULL;
 tcid0->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid0->qcl_Type1.bwp_Id=1;
 tcid0->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tcid0->qcl_Type1.referenceSignal.choice.csi_rs = 2;
 tcid0->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid0);

 NR_TCI_State *tcid1=calloc(1,sizeof(NR_TCI_State));
 tcid1->tci_StateId=0;
 tcid1->qcl_Type1.cell=NULL;
 tcid1->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid1->qcl_Type1.bwp_Id=1;
 tcid1->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tcid1->qcl_Type1.referenceSignal.choice.csi_rs = 6;
 tcid1->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid1);

 NR_TCI_State *tcid2=calloc(1,sizeof(NR_TCI_State));
 tcid2->tci_StateId=2;
 tcid2->qcl_Type1.cell=NULL;
 tcid2->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid2->qcl_Type1.bwp_Id=1;
 tcid2->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tcid2->qcl_Type1.referenceSignal.choice.csi_rs = 10;
 tcid2->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid2);

 NR_TCI_State *tcid3=calloc(1,sizeof(NR_TCI_State));
 tcid3->tci_StateId=3;
 tcid3->qcl_Type1.cell=NULL;
 tcid3->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid3->qcl_Type1.bwp_Id=1;
 tcid3->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tcid3->qcl_Type1.referenceSignal.choice.csi_rs = 14;
 tcid3->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid3);

 NR_TCI_State *tcid4=calloc(1,sizeof(NR_TCI_State));
 tcid4->tci_StateId=4;
 tcid4->qcl_Type1.cell=NULL;
 tcid4->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid4->qcl_Type1.bwp_Id=1;
 tcid4->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tcid4->qcl_Type1.referenceSignal.choice.csi_rs = 18;
 tcid4->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid4);

 NR_TCI_State *tcid5=calloc(1,sizeof(NR_TCI_State));
 tcid5->tci_StateId=5;
 tcid5->qcl_Type1.cell=NULL;
 tcid5->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid5->qcl_Type1.bwp_Id=1;
 tcid5->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tcid5->qcl_Type1.referenceSignal.choice.csi_rs = 22;
 tcid5->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid5);

 NR_TCI_State *tcid6=calloc(1,sizeof(NR_TCI_State));
 tcid6->tci_StateId=6;
 tcid6->qcl_Type1.cell=NULL;
 tcid6->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid6->qcl_Type1.bwp_Id=1;
 tcid6->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tcid6->qcl_Type1.referenceSignal.choice.csi_rs = 26;
 tcid6->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid6);

 NR_TCI_State *tcid7=calloc(1,sizeof(NR_TCI_State));
 tcid7->tci_StateId=7;
 tcid7->qcl_Type1.cell=NULL;
 tcid7->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid7->qcl_Type1.bwp_Id=1;
 tcid7->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_csi_rs;
 tcid7->qcl_Type1.referenceSignal.choice.csi_rs = 30;
 tcid7->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeA;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid7);

 NR_TCI_State *tcid8=calloc(1,sizeof(NR_TCI_State));
 tcid8->tci_StateId=8;
 tcid8->qcl_Type1.cell=NULL;
 tcid8->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid8->qcl_Type1.bwp_Id=1;
 tcid8->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tcid8->qcl_Type1.referenceSignal.choice.ssb = 0;
 tcid8->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid8);

 NR_TCI_State *tcid9=calloc(1,sizeof(NR_TCI_State));
 tcid9->tci_StateId=9;
 tcid9->qcl_Type1.cell=NULL;
 tcid9->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid9->qcl_Type1.bwp_Id=1;
 tcid9->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tcid9->qcl_Type1.referenceSignal.choice.ssb = 1;
 tcid9->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid9);

 NR_TCI_State *tcid10=calloc(1,sizeof(NR_TCI_State));
 tcid10->tci_StateId=10;
 tcid10->qcl_Type1.cell=NULL;
 tcid10->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid10->qcl_Type1.bwp_Id=1;
 tcid10->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tcid10->qcl_Type1.referenceSignal.choice.ssb = 2;
 tcid10->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid10);

 NR_TCI_State *tcid11=calloc(1,sizeof(NR_TCI_State));
 tcid11->tci_StateId=11;
 tcid11->qcl_Type1.cell=NULL;
 tcid11->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid11->qcl_Type1.bwp_Id=1;
 tcid11->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tcid11->qcl_Type1.referenceSignal.choice.ssb = 3;
 tcid11->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid11);

 NR_TCI_State *tcid12=calloc(1,sizeof(NR_TCI_State));
 tcid12->tci_StateId=12;
 tcid12->qcl_Type1.cell=NULL;
 tcid12->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid12->qcl_Type1.bwp_Id=1;
 tcid12->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tcid12->qcl_Type1.referenceSignal.choice.ssb = 4;
 tcid12->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid12);

 NR_TCI_State *tcid13=calloc(1,sizeof(NR_TCI_State));
 tcid13->tci_StateId=13;
 tcid13->qcl_Type1.cell=NULL;
 tcid13->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid13->qcl_Type1.bwp_Id=1;
 tcid13->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tcid13->qcl_Type1.referenceSignal.choice.ssb = 5;
 tcid13->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid13);

 NR_TCI_State *tcid14=calloc(1,sizeof(NR_TCI_State));
 tcid14->tci_StateId=14;
 tcid14->qcl_Type1.cell=NULL;
 tcid14->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid14->qcl_Type1.bwp_Id=1;
 tcid14->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tcid14->qcl_Type1.referenceSignal.choice.ssb = 6;
 tcid14->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid14);

 NR_TCI_State *tcid15=calloc(1,sizeof(NR_TCI_State));
 tcid15->tci_StateId=15;
 tcid15->qcl_Type1.cell=NULL;
 tcid15->qcl_Type1.bwp_id=calloc(1,sizeof(NR_BWP_Id_t));
 *tcid15->qcl_Type1.bwp_Id=1;
 tcid15->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
 tcid15->qcl_Type1.referenceSignal.choice.ssb = 7;
 tcid15->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
 ASN_SEQUENCE_ADD(&&bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToAddModList->list,tcid15);

 bwp->bwp_Dedicated->pdsch_config->choice.setup->tci_StatesToReleaseList=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->vrb_ToPRB_Interleaver=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->resourceAllocation=NR_PDSCH_Config__resourceAllocation_resourceAllocationType0;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->TimeDomainAllocationList=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->pdsch_AggregationFactor=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->rateMatchPatternToAddModList=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->rateMatchPatternToReleaseList=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->rateMatchPatternGroup1=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->rateMatchPatternGroup2=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->rbg_Size=NR_PDSCH_Config__rbg_Size_config1;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->mcs_Table=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->maxNrofCodeWordsScheduledByDCI = calloc(1,sizeof(long));
 *bwp->bwp_Dedicated->pdsch_config->choice.setup->maxNrofCodeWordsScheduledByDCI = NR_PDSCH_Config__maxNrofCodeWordsScheduledByDCI_n1;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->prb_BundlingType.present = NR_PDSCH_Config__prb_BundlingType_PR_staticBundling;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->prb_BundlingType.staticBundling = calloc(1,sizeof(struct NR_PDSCH_Config__prb_BundlingType__staticBundling));
 bwp->bwp_Dedicated->pdsch_config->choice.setup->prb_BundlingType.staticBundling->bundleSize =
   calloc(1,sizeof(long));
 *bwp->bwp_Dedicated->pdsch_config->choice.setup->prb_BundlingType.staticBundling->bundleSize = NR_PDSCH_Config__prb_BundlingType__staticBundling__bundleSize_wideband;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->zp_CSI_RS_ResourceToAddModList=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->zp_CSI_RS_ResourceToReleaseList=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->aperiodic_ZP_CSI_RS_ResourceToAddModList=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->aperiodic_ZP_CSI_RS_ResourceToReleaseList=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->sp_ZP_CSI_RS_ResourceToAddModList=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->sp_ZP_CSI_RS_ResourceToReleaseList=NULL;
 bwp->bwp_Dedicated->pdsch_config->choice.setup->p_ZP_CSI_RS_ResourceSet=NULL;

 bwp->bwp_Dedicated->pdsch_Config->setup->tci_StatesToReleaseList=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->vrb_ToPRB_Interleaver=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->resourceAllocation=NR_PDSCH_Config__resourceAllocation_resourceAllocationType0;
 bwp->bwp_Dedicated->pdsch_Config->setup->TimeDomainAllocationList=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->pdsch_AggregationFactor=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->rateMatchPatternToAddModList=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->rateMatchPatternToReleaseList=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->rateMatchPatternGroup1=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->rateMatchPatternGroup2=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->rbg_Size=NR_PDSCH_Config__rbg_Size_config1;
 bwp->bwp_Dedicated->pdsch_Config->setup->mcs_Table=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->maxNrofCodeWordsScheduledByDCI = calloc(1,sizeof(long));
 *bwp->bwp_Dedicated->pdsch_Config->setup->maxNrofCodeWordsScheduledByDCI = NR_PDSCH_Config__maxNrofCodeWordsScheduledByDCI_n1;
 bwp->bwp_Dedicated->pdsch_Config->setup->prb_BundlingType.present = NR_PDSCH_Config__prb_BundlingType_PR_staticBundling;
 bwp->bwp_Dedicated->pdsch_Config->setup->prb_BundlingType.staticBundling = calloc(1,sizeof(struct NR_PDSCH_Config__prb_BundlingType__staticBundling));
 bwp->bwp_Dedicated->pdsch_Config->setup->prb_BundlingType.staticBundling->bundleSize =
   calloc(1,sizeof(long));
 *bwp->bwp_Dedicated->pdsch_Config->setup->prb_BundlingType.staticBundling->bundleSize = NR_PDSCH_Config__prb_BundlingType__staticBundling__bundleSize_wideband;
 bwp->bwp_Dedicated->pdsch_Config->setup->zp_CSI_RS_ResourceToAddModList=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->zp_CSI_RS_ResourceToReleaseList=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->aperiodic_ZP_CSI_RS_ResourceToAddModList=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->aperiodic_ZP_CSI_RS_ResourceToReleaseList=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->sp_ZP_CSI_RS_ResourceToAddModList=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->sp_ZP_CSI_RS_ResourceToReleaseList=NULL;
 bwp->bwp_Dedicated->pdsch_Config->setup->p_ZP_CSI_RS_ResourceSet=NULL;
 bwp->bwp_Dedicated->sps_Config = NULL;

 bwp->bwp_Dedicated->radioLinkMonitoringConfig = calloc(1,sizeof(struct NR_SetupRelease_RadioLinkMonitoringConfig));
 bwp->bwp_Dedicated->radioLinkMonitoringConfig->present = NR_SetupRelease_RadioLinkMonitoringConfig_PR_setup;

 bwp->bwp_Dedicated->radioLinkMonitoringConfig->setup = calloc(1,sizeof(struct NR_RadioLinkMonitoringConfig));
 bwp->bwp_Dedicated->radioLinkMonitoringConfig->setup->failureDetectionResourcesToAddModList=NULL;
 bwp->bwp_Dedicated->radioLinkMonitoringConfig->setup->failureDetectionResourcesToReleaseList=NULL;
 bwp->bwp_Dedicated->radioLinkMonitoringConfig->setup->beamFailureInstanceMaxCount = calloc(1,sizeof(long));
 *bwp->bwp_Dedicated->radioLinkMonitoringConfig->setup->beamFailureInstanceMaxCount = NR_RadioLinkMonitoringConfig__beamFailureInstanceMaxCount_n3;
 bwp->bwp_Dedicated->radioLinkMonitoringConfig->setup->beamFailureDetectionTimer = calloc(1,sizeof(long));
 *bwp->bwp_Dedicated->radioLinkMonitoringConfig->setup->beamFailureDetectionTimer = NR_RadioLinkMonitoringConfig__beamFailureDetectionTimer_pbfd2;
 
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->downlinkBWP_ToAddModList->list,bwp);

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->firstActiveDownlinkBWP_Id=calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spcellConfigDedicated->firstActiveDownlinkBWP_Id));
 
 *secondaryCellGroup->spCellConfig->spcellConfigDedicated->firstActiveDownlinkBWP_Id=1;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->bwp_InactivityTimer = NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->defaultDownlinkBWP_Id = NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig=calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig));

 NR_BWP_UplinkDedicated_t *initialUplinkBWP = calloc(1,sizeof(NR_BWP_UplinkDedicated_t));
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig->initialUplinkBWP = initialUplinkBWP;
 initialUplinkBWP->pucch_Config = NULL;
 initialUplinkBWP->pusch_Config = calloc(1,sizeof(*initalUplinkBWP->pusch_Config));
 initialUplinkBWP->pusch_Config->present = NR_SetupRelease_PUSCH_Config_PR_setup;
 NR_PUSCH_Config_t *pusch_Config = calloc(1,sizeof(NR_PUSCH_Config_t));
 initialUplinkBWP->pusch_Config->setup = pusch_Config;
 pusch_Config->txConfig=calloc(1,sizeof(*pusch_confi->txConfig));
 *pusch_Config->txConfig= NR_PUSCH_Config__txConfig_codebook;
 pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA = NULL;
 pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB = calloc(1,sizeof(*pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB));
 pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->present = NR_SetupRelease_DMRS_UplinkConfig_PR_setup;
 pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->setup = calloc(1,sizeof(*pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->setup));
 NR_DMRS_UplinkConfig_t *NR_DMRS_UplinkConfig = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->setup;
 NR_DMRS_UplinkConfig->dmrs_Type = NULL;
 NR_DMRS_UplinkConfig->dmrs_AdditionalPosition =NR_DMRS_UplinkConfig__dmrs_AdditionalPosition_pos0;
 NR_DMRS_UplinkConfig->phaseTrackingRS=NULL;
 NR_DMRS_UplinkConfig->maxLength=NULL;
 NR_DMRS_UplinkConfig->transformPrecodingDisabled = calloc(1,sizeof(*NR_DMRS_UplinkConfig->transformPrecodingDisabled));
 NR_DMRS_UplinkConfig->transformPrecodingDisabled->scramblingID0 = NULL;
 NR_DMRS_UplinkConfig->transformPrecodingDisabled->scramblingID1 = NULL;
 NR_DMRS_UplinkConfig->transformPrecodingEnabled = NULL;
 pusch_Config->pusch_PowerControl = calloc(1,sizeof(*pusch_Config->pusch_PowerControl));
 pusch_Config->pusch_PowerControl->tpc_Accumulation = NULL;
 pusch_Config->pusch_PowerControl->msg3_Alpha = NR_Alpha_alpha1;
 pusch_Config->pusch_PowerControl->p0_NominalWithoutGrant = NULL;
 pusch_Config->pusch_PowerControl->p0_AlphaSets = calloc(1,sizeof(pusch_Config->pusch_PowerControl->p0_AlphaSets));
 NR_P0_PUSCH_AlphaSet_t *aset = calloc(1,sizeof(NR_P0_PUSCH_AlphaSet_t));
 aset->p0_PUSCH_AlphaSetId=0;
 aset->p0=calloc(1,sizeof(*aset->p0));
 *aset->p0 = 0;
 aset->alpha=calloc(1,sizeof(*aset0->alpha));
 *aset->alpha=NR_Alpha_alpha1;
 ASN_SEQUENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->downlinkBWP_ToAddModList->list,aset);
 pusch_Config->pusch_PowerControl->pathlossReferenceRSToAddModList = calloc(1,sizeof(pusch_Config->pusch_PowerControl->pathlossReferenceRSToAddModList));
 NR_PUSCH_PathlossRefereneRS_t *pl = calloc(1,sizeof(NR_PUSCH_PathlossReferenceRS_t));
 pl->pusch_PathlossReferenceRS_Id=0;
 pl->referenceSignal.present = NR_PUSCH_PathlossReferenceRS__referenceSignal_PR_csi_RS_Index;
 pl->referenceSignal.choice.csi_RS_Index=0;
 ASN_SEQUENCE_ADD(&pusch_Config->pusch_PowerControl->pathlossReferenceRSToAddModList->list,pl);
 pusch_Config->pusch_PowerControl->pathlossReferenceRSToReleaseList = NULL;
 pusch_Config->pusch_PowerControl->twoPUSCH_PC_AdjustmentStates = NULL;
 pusch_Config->pusch_PowerControl->deltaMCS = NULL;
 pusch_Config->pusch_PowerControl->sri_PUSCH_MappingToAddModList = NULL;
 pusch_Config->pusch_PowerControl->sri_PUSCH_MappingToReleaseList = NULL;
 pusch_Config->frequencyHopping=NULL;
 pusch_Config->frequencyHoppingOffsetLists=NULL;
 pusch_Config->resourceAllocation = NR_PUSCH_Config__resourceAllocation_resourceAllocationType1;
 pusch_Config->pusch_TimeDomwinAllocationList = NULL;
 pusch_Config->pusch_AggregationFactor=NULL;
 pusch_Config->mcs_Table=NULL;
 pusch_Config->mcs_TableTransformPrecoder=NULL;
 pusch_Config->transformPrecoder=calloc(1,sizeof(*pusch_Config->transformPrecoder));
 *pusch_Config->transformPrecoder = NR_PUSCH_Config__transformPrecoder_disabled;
 pusch_Config->codebookSubset = NR_PUSCH_Config__codebookSubset_nonCoherent;
 pusch_Config->maxRank=calloc(1,sizeof(*pusch_Conig->maxRank));
 pusch_Config->maxRank= 1;
 pusch_Config->rbg_Size=NULL;
 pusch_Config->uci_OnPUSCH=NULL;
 pusch_Config->tp_pi2BPSK=NULL;

 initialUplinkBWP->srs_Config = calloc(1,sizeof(*initalUplinkBWP->srs_Config));
 initialUplinkBWP->srs_Config->present = NR_SetupRelease_SRS_Config_PR_setup;
 NR_SRS_Config_t *srs_Config = calloc(1,sizeof(NR_SRS_Config_t));
 initialUplinkBWP->srs_Config->setup=srs_Config;
 srs_Config->srs_ResourceSetToReleaseList=NULL;
 srs_Config->srs_ResourceSetToAddModList=calloc(1,sizeof(*srs_Config->srs_ResourceSetToAddModList));
 NR_SRS_ResourceSet_t *srs_resset0=calloc(1,sizeof(*srs_resset0));
 srs_resset0->srs_ResourceSetId = 0;
 srs_resset0->srs_ResourceIdList=calloc(1,sizeof(*srs_resset0->srs_ResourceIdList));
 NR_SRS_ResourceId_t *srs_resset0_id=calloc(1,sizeof(*srs_resset0_id));
 *srs_resset0_id=0;
 ASN_SEQUENCE_ADD(&srs_resset0->srs_ResourceIdList->list,srs_resset0_id);
 srs_Config->srs_ResourceToReleaseList=NULL;
 srs_resset0->resourceType.present =  NR_SRS_ResourceSet__resourceType_PR_aperiodic;
 srs_resset0->resourceType.choice.aperiodic = calloc(1,sizeof(*srs_resset0->resourceType.aperiodic));
 srs_resset0->resourceType.choice.aperiodic->aperiodicSRS-ResourceTrigger=1;
 srs_resset0->resourceType.choice.aperiodic->csi_RS=NULL;
 srs_resset0->resourceType.choice.aperiodic->slotOffset= calloc(1,*srs_resset0->resourceType.choice.aperiodic->slotOffset);
 *srs_resset0->resourceType.choice.aperiodic->slotOffset=2;
 srs_resset0->resourceType.choice.aperiodic->aperiodicSRS_ResourceTriggerList_v1530=NULL;
 srs_resset0->resourceType.choice.aperiodic->ext1=NULL;
 srs_resset0->usage=NR_SRS_ResourceSet__usage_codebook;
 srs_resset0->alpha = NR_Alpha_alpha1;
 srs_resset0->p0=calloc(1,sizeof(*srs_resset0->p0));
 *srs_resset0->p0=-80;
 srs_resset0->pathlossReferenceRS=NULL;
 srs_resset0->srs_PowerControlAdjustmentStates=NULL;
 ASN_SEQUENCE_ADD(&srs_Config->srs_ResourceSetToAddModList->list,srs_resset0);
 srs_Config->srs_ResourceToReleaseList=NULL;
 srs_Config->srs_ResourceToAddModList=calloc(1,sizeof(*srs_Config->srs_ResourceToAddModList));
 NR_SRS_Resource_t *srs_res0=calloc(1,sizeof(*srs_res0));
 srs_res0->srs_ResourceId=0;
 srs_res0->nrofSRS_Ports=NR_SRS_Resource__nrofSRS_Ports_port1;
 srs->res0->ptrs_PortIndex=NULL;
 srs_res0->transmissionComb.present=NR_SRS_Resource__transmissionComb_PR_n2; 
 srs_res0->transmissionComb.choice.n2=calloc(1,sizeof(*srs_res0->transmissionComb.choice.n2));
 srs_res0->transmissionComb.choice.n2->combOffset_n2=0;
 srs_res0->transmissionComb.choice.n2->cyclicShift_n2=0;
 srs_res0->resourceMapping.startPosition=2;
 srs_res0->nrofSymbols=NR_SRS_Resource__resourceMapping__nrofSymbols_n1;
 srs_res0->repetitionFactor=NR_SRS_Resource__resourceMapping__repetitionFactor_n1;
 srs_res0->freqDomainPosition=0;
 srs_res0->freqDomainShift=0;
 srs_res0->freqHopping.c_SRS = 61;
 srs_res0->b_SRS=0;
 srs_res0->b_hop=0;
 srs_res0->groupOrSequenceHopping=NR_SRS_Resource__groupOrSequenceHopping_neither;
 srs_res0->resourceType.present= NR_SRS_Resource__resourceType_PR_aperiodic;
 srs_res0->resourceType.choice.aperiodic=calloc(1,sizeof(*srs_res0->resourceType.choice.aperiodic));
 srs_res0->sequenceId=40;
 srs_res0->spatialRelationInfo=calloc(1,sizeof(*srs_res0->spatialRelationInfo));
 srs_res0->spatialRelationInfo->referenceSignal->servingCellId=NULL;
 srs_res0->spatialRelationInfo->referenceSignal->present=NR_SRS_SpatialRelationInfo__referenceSignal_PR_csi_RS_Index;
 srs_res0->spatialRelationInfo->referenceSignal->choice.csi_RS_Index=0;
 ASN_SEQUENCE_ADD(&srs_Config->srs_ResourceToAddModList->list,srs_res0);

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig->uplinkBWP_ToReleaseList = NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList = calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList));
 NR_BWP_Uplink_t *ubwp = calloc(1,sizeof(NR_BWP_Uplink_t));
 ubwp->bwp_Id=1;
 ubwp->bwp_Common = calloc(1,sizeof(*ubwp->bwp_Common));
 // copy bwp_Common from Initial UL BWP except for bandwidth
 memcpy((void*)&ubwp->bwp_Common->genericParameters,
	(void*)&servingcellconfigcommon->uplinkConfigCommon->genericParameters,
	sizeof(servingcellconfigcommon->uplinkConfigCommon->genericParameters));
 ubwp->bwp_Common->genericParameters.locationandbandwidth=PRBalloc_to_locationanbandwidth(scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,0);

 ubwp->bwp_Common->rach_ConfigCommon  = servingcellconfigcommon->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon;
 ubwp->bwp_Common->pusch_ConfigCommon = servingcellconfigcommon->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon;
 ubwp->bwp_Common->pucch_ConfigCommon = servingcellconfigcommon->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon;
 
 ubwp->bwp_Dedicated = calloc(1,sizeof(*ubwp->bwp_Dedicated));
 ubwp->bwp_Dedicated->pucch_Config = calloc(1,sizeof(*ubwp->bwp_Dedicated->pucch_Config));
 ubwp->bwp_Dedicated->pucch_Config->present = NR_SetupRelease_PUCCH_Config_PR_setup;
 NR_PUCCH_Config_t *pucch_Config = calloc(1,sizeof(NR_PUCCH_Config_t));
 ubwp->bwp_Dedicated->pucch_Config->setup=pucch_Config;
 pucch_Config->resourceSetToAddModList = calloc(1,sizeof(*pucch_Config->resourceSetToAddModList));
 pucch_Config->resourceSetToReleaseList = NULL;
 NR_PUCCH_ResourceSet_t *pucchresset0=calloc(1,sizeof(NR_PUCCH_ResourceSet_t));
 NR_PUCCH_ResourceSet_t *pucchresset1=calloc(1,sizeof(NR_PUCCH_ResourceSet_t));
 pucchreset0->pucch_ResourceSetId = 0;
 NR_PUCCH_ResourceId_t *pucchresset0id0=calloc(1,sizeof(NR_PUCCH_ResourceId_t));
 NR_PUCCH_ResourceId_t *pucchresset0id1=calloc(1,sizeof(NR_PUCCH_ResourceId_t));
 *pucchresset0id0=1;
 ASN_SEQ_ADD(&pucchresset0->resourseList->list,pucchresset0id0);
 *pucchresset0id1=2;
 ASN_SEQ_ADD(&pucchresset0->resourseList->list,pucchresset0id1);
 pucchresset0->maxPayloadMinus1=NULL;

 ASN_SEQ_ADD(&pucch_Config->resourceSetToAddModList,pucchresset0);

 pucchreset1->pucch_ResourceSetId = 1;
 NR_PUCCH_ResourceId_t *pucchresset1id0=calloc(1,sizeof(NR_PUCCH_ResourceId_t));
 NR_PUCCH_ResourceId_t *pucchresset1id1=calloc(1,sizeof(NR_PUCCH_ResourceId_t));
 *pucchresset1id0=3;
 ASN_SEQ_ADD(&pucchresset1->resourseList->list,pucchresset1id0);
 *pucchresset1id1=4;
 ASN_SEQ_ADD(&pucchresset1->resourseList->list,pucchresset1id1);
 pucchresset1->maxPayloadMinus1=NULL;
 ASN_SEQ_ADD(&pucch_Config->resourceSetToAddModList,pucchresset1);

 pucch_Config->resourceToAddModList = calloc(1,sizeof(*pucch_Config->resourceToAddModList));
 pucch_Config->resourceToReleaseList = NULL;
 NR_PUCCH_Resource *pucchres0=calloc(1,sizeof(*pucchres0));
 NR_PUCCH_Resource *pucchres1=calloc(1,sizeof(*pucchres1));
 NR_PUCCH_Resource *pucchres2=calloc(1,sizeof(*pucchres2));
 NR_PUCCH_Resource *pucchres3=calloc(1,sizeof(*pucchres3));
 pucchres0->pucch_ResourceId=1;
 pucchres0->startingPRB=48;
 pucchres0->intraSlotFrequencyHopping=NULL;
 pucchRes0->secondHopPRB=NULL;
 pucchres0->format.present= NR_PUCCH_Resource__format_PR_format0;
 pucchres0->format.choice.format0=calloc(1,sizeof(*pucchres0->format.choice.format0));
 pucchres0->format.choice.format0->initialCyclicShift=0;
 pucchres0->format.choice.format0->nrofSymbols=1;
 pucchres0->format.choice.format0->startingSymbolIndex=13;
 ASN_SEQ_ADD(&pucch_Config->resourceToAddModList->list,pucchres0);

 pucchres1->pucch_ResourceId=2;
 pucchres1->startingPRB=48;
 pucchres1->intraSlotFrequencyHopping=NULL;
 pucchRes1->secondHopPRB=NULL;
 pucchres1->format.present= NR_PUCCH_Resource__format_PR_format0;
 pucchres1->format.choice.format0=calloc(1,sizeof(*pucchres1->format.choice.format0));
 pucchres1->format.choice.format0->initialCyclicShift=0;
 pucchres1->format.choice.format0->nrofSymbols=1;
 pucchres1->format.choice.format0->startingSymbolIndex=12;
 ASN_SEQ_ADD(&pucch_Config->resourceToAddModList->list,pucchres1);

 pucchres2->pucch_ResourceId=3;
 pucchres2->startingPRB=40;
 pucchres2->intraSlotFrequencyHopping=NULL;
 pucchRes2->secondHopPRB=NULL;
 pucchres2->format.present= NR_PUCCH_Resource__format_PR_format2;
 pucchres2->format.choice.format2=calloc(1,sizeof(*pucchres2->format.choice.format2));
 pucchres2->format.choice.format2->=nrofPRBs=16;
 pucchres2->format.choice.format2->nrofSymbols=1;
 pucchres2->format.choice.format2->startingSymbolIndex=13;
 ASN_SEQ_ADD(&pucch_Config->resourceToAddModList->list,pucchres2);

 pucchres3->pucch_ResourceId=4;
 pucchres3->startingPRB=40;
 pucchres3->intraSlotFrequencyHopping=NULL;
 pucchRes3->secondHopPRB=NULL;
 pucchres3->format.present= NR_PUCCH_Resource__format_PR_format2;
 pucchres3->format.choice.format2=calloc(1,sizeof(*pucchres3->format.choice.format2));
 pucchres3->format.choice.format2->=nrofPRBs=16;
 pucchres3->format.choice.format2->nrofSymbols=1;
 pucchres3->format.choice.format2->startingSymbolIndex=12;
 ASN_SEQ_ADD(&pucch_Config->resourceToAddModList->list,pucchres3);

 pucch_Config->format2=calloc(1,sizeof(*pucch_Config->format2));
 pucch_Config->format2->present=NR_SetupRelease_PUCCH_FormatConfig_PR_setup;
 NR_PUCCH_FormatConfig_t *pucchfmt2 = calloc(1,sizeof(*pucch_Config->format2->setup));
 pucch_Config->format2->setup = pucchfmt2;
 pucchfmt2->interslotFrequencyHopping=NULL;
 pucchfmt2->additionalDMRS=NULL;
 pucchfmt2->maxCodeRate=calloc(1,sizeof(*pucch2fmt2->maxCodeRate));
 *pucchfmt2->maxCodeRate=NR_PUCCH_MaxCodeRate_zeroDot15;
 pucchfmt2->nrofSlots=NULL;
 pucchfmt2->pi2BPSK=NULL;
 pucchfmt2->simultaneousHARQ_ACK_CSI=NULL;
 pucch_Config->schedulingRequestResourceToAddModList=NULL;
 pucch_Config->schedulingRequestResourceToReleaseList=NULL;
 pucch_Config->multi_CSI_PUCCH_ResourceList=NULL;
 pucch_Config->dl_DataToUL_ACK = calloc(1,sizeof(*pucch_config->dl_DataToUL_ACK));
 long *delay[8];
 for (int i=0;i<8;i++) {
   delay[i] = calloc(1,sizeof(long));
   *delay[i] = (i<6) ? (i+2) : 0;
   ASN_SEQ_ADD(&pucch_Config->dl_DataToUL_ACK->list,delay[i]);
 }
 pucch_Config->spatialRelationInfoToAddModList = calloc(1,sizeof(*pucch_config->spatialRelationInfoToAddModList));
 NR_PUCCH_SpatialRelationInfoId_t *pucchspatial = calloc(1,sizeof(*pucchspatial));
 pucchspatial->pucch_SpatialRelationInfoId = 1;
 pucchspatial->servingCellId = NULL;
 pucchspatial->referencesignal.present = NR_PUCCH_SpatialRelationInfo__referenceSignal_PR_csi_RS_Index;
 pucchspatial->referencesignal.choice.csi_RS_Index = 0;
 pucchspatial->pucch_PathlossReferenceRS_Id = 0;
 pucchspatial->p0_PuCCH_Id = 1;
 pucchspatial->closedLoopIndex = NR_PUCCH_SpatialRelationInfo__closedLoopIndex_i0;
 ASN_SEQ_ADD(&pucch_Config->spatialRelationInfoToAddModList->list,pucchspatial);
 pucch_Config->spatialRelationInfoToReleaseList=NULL;
 pucch_Config->pucch_PowerControl=calloc(1,sizeof(*pucch_Config->pucch_PowerControl));
 pucch_Config->pucch_PowerControl->deltaF_PUCCH_f0 = calloc(1,sizeof(long));
 *pucch_Config->pucch_PowerControl->deltaF_PUCCH_f0 = 0;
 pucch_Config->pucch_PowerControl->deltaF_PUCCH_f1 = calloc(1,sizeof(long));
 *pucch_Config->pucch_PowerControl->deltaF_PUCCH_f1 = 0;
 pucch_Config->pucch_PowerControl->deltaF_PUCCH_f2 = calloc(1,sizeof(long));
 *pucch_Config->pucch_PowerControl->deltaF_PUCCH_f2 = 0;
 pucch_Config->pucch_PowerControl->deltaF_PUCCH_f3 = calloc(1,sizeof(long));
 *pucch_Config->pucch_PowerControl->deltaF_PUCCH_f3 = 0;
 pucch_Config->pucch_PowerControl->deltaF_PUCCH_f4 = calloc(1,sizeof(long));
 *pucch_Config->pucch_PowerControl->deltaF_PUCCH_f4 = 0;
 pucch_Config->pucch_PowerControl->p0_Set = calloc(1,sizeof(*pucch_Config->p0_Set));
 NR_P0_PUCCH *p00 = calloc(1,sizeof(*p00));
 p00->p0_PUCCH_id=1;
 p00->P0_PUCCH_Value = 0;
 ASN_SEQ_ADD(&pucch_Config->pucch_PowerControl->p0_Set->list,p00);
 pucch_Config->pucch_PowerControl->pathlossReferenceRSs = calloc(1,sizeof(*pucch_Config->pathlossReferenceRSs));
 NR_PUCCH_PathlossReferenceRS_t *pucchPLRef=calloc(1,sizeof(*pucchPLRef));
 pucchPLRef->pucch_PathlossReferenceRS_Id=0;
 pucchPLRef->referenceSignal.present = NR_PUCCH_PathlossReferenceRS__referenceSignal_PR_csi_RS_Index;
 pucchPLRef->referenceSignal.choice.csi_RS_Index=0;
 ASN_SEQ_ADD(&pucch_Config->pucch_PowerControl->pathlossReferenceRSs->list,pucchPLRef);

 // copy pusch_Config from dedicated initialBWP
 ubwp->bwp_Dedicated->pusch_Config = pusch_Config;
 ubwp->bwp_Dedicated->configuredGrantConfig = NULL;
 ubwp->bwp_Dedicated->srs_Config = srs_Config;
 ubwp->bwp_Dedicated->beamFailureRecoveryConfig = NULL;

 ASN_SEQENCE_ADD(&secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list,ubwp);

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_id = calloc(1,sizeof(NR_BWP_Id_t));
 *secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_id = 1;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig->pusch_ServingCellConfig = NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->uplinkConfig->carrierSwitching = NULL;

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->supplementaryUplink=NULL;

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->pdcch_ServingCellConfig=NULL;

 secondaryCellGroup->spCellConfig->spcellConfigDedicated->pdsch_ServingCellConfig=calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spcellConfigDedicated->pdsch_ServingCellConfig));
 NR_PDSCH_ServingCellConfig_t *pdsch_servingcellconfig = calloc(1,sizeof(NR_PDSCH_ServingCellConfing_t));
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->pdsch_ServingCellConfig->present = NR_SetupRelease_PDSCH_ServingCellConfig_PR_setup;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->pdsch_ServingCellConfig->setup = pdsch_servingcellconfig;
 pdsch_servingcellconfig->codeBlockGroupTransmission = NULL;
 pdsch_servingcellconfig->xOverhead = NULL;
 pdsch_servingcellconfig->nrofHARQ_ProcessesForPDSCH = NULL;
 pdsch_servingcellconfig->pucch_Cell= NULL;
 pdsch_servingcellconfig->maxMIMO_Layers = calloc(1,sizeof(*pdsch_servingcellconfig->maxMIMO_Layers));
 *pdsch_servingcellconfig->maxMIMO_Layers = 2;
 pdsch_servingcellconfig->processingType2Enabled = NULL;
 
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->csi_MeasConfig=calloc(1,sizeof(*secondaryCellGroup->spCellConfig->spcellConfigDedicated->csi_MeasConfig));
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->csi_MeasConfig->present = NR_SetupRelease_CSI_MeasConfig_PR_setup;
 NR_CSI_MeasConfig_t *csi_MeasConfig = calloc(1,sizeof(NR_CSI_MeasConfig_t));
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->csi_MeasConfig->setup = csi_MeasConfig;
 csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList = calloc(1,sizeof(*csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList));

 NR_NZP_CSI_RS_Resource *nzpres0 = calloc(1,sizeof(*nzpres0));
 nzpres0->nzp_CSI_RS_ResourceId=0;
 nzpres0->resourceMapping.frequencyDomainAllocation.present = NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_other;
 nzpres0->resourceMapping.frequencyDomainAllocation.choice.other.buf = calloc(1,1);
 nzpres0->resourceMapping.frequencyDomainAllocation.choice.other.size = 1;
 nzpres0->resourceMapping.frequencyDomainAllocation.choice.other.bits_unused = 2;
 nzpres0->resourceMapping.frequencyDomainAllocation.choice.other.buf[0]=1;
 nzpres0->nrofPorts = NR_CSI_RS_ResourceMapping__nrofPorts_p2;
 nzpres0->firstOFDMSymbolInTimeDomain=7;
 nzpres0->firstOFDMSymbolInTimeDomain2=NULL;
 nzpres0->cdm_Type = NR_CSI_RS_ResourceMapping__cdm_Type_fd_CDM2;
 nzpres0->density.present=NR_CSI_RS_ResourceMapping__density_PR_one;
 nzpres0->density.choice.one=NULL;
 nzpres0->freqBand.startingRB=0;
 nzpres0->freqBand.nrofRBs= scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
 nzpres0->powerControlOffset=13;
 nzpres0->powerControlOffsetSS=NULL;
 nzpres0->scramblingID=40;
 nzpres0->periodicityAndOffset = calloc(1,sizeof(*nzpres0->periodicityAndOffset));
 nzpres0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots320;
 nzpres0->periodicityAndOffset->choice.slots320 = 2;
 nzpres0->qcl_InfoPeriodicCSI_RS=NULL;
 ASN_SEQUENCE_ADD(&csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list,nzpres0);

 NR_NZP_CSI_RS_Resource *nzpres2 = calloc(1,sizeof(*nzpres2));
 nzpres2->nzp_CSI_RS_ResourceId=2;
 nzpres2->resourceMapping.frequencyDomainAllocation.present = NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row1;
 nzpres2->resourceMapping.frequencyDomainAllocation.choice.row1.buf = calloc(1,1);
 nzpres2->resourceMapping.frequencyDomainAllocation.choice.row1.size = 1;
 nzpres2->resourceMapping.frequencyDomainAllocation.choice.row1.bits_unused = 4;
 nzpres2->resourceMapping.frequencyDomainAllocation.choice.row1.buf[0]=1;
 nzpres2->nrofPorts = NR_CSI_RS_ResourceMapping__nrofPorts_p1;
 nzpres2->firstOFDMSymbolInTimeDomain=4;
 nzpres2->firstOFDMSymbolInTimeDomain2=NULL;
 nzpres2->cdm_Type = NR_CSI_RS_ResourceMapping__cdm_Type_noCDM;
 nzpres2->density.present=NR_CSI_RS_ResourceMapping__density_PR_three;
 nzpres2->density.choice.three=NULL;
 nzpres2->freqBand.startingRB=0;
 nzpres2->freqBand.nrofRBs= scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
 nzpres2->powerControlOffset=0;
 nzpres2->powerControlOffsetSS=calloc(1,sizeof(*nzpres2->powerControlOffsetSS));
 *nzpres2->powerControlOffsetSS=NR_NZP_CSI_RS_Resource__powerControlOffsetSS_db0;
 nzpres2->scramblingID=40;
 nzpres2->periodicityAndOffset = calloc(1,sizeof(*nzpres2->periodicityAndOffset));
 nzpres2->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots160;
 nzpres2->periodicityAndOffset->choice.slots160 = 25;
 nzpres2->qcl_InfoPeriodicCSI_RS=calloc(1,sizeof(*nzpres2->qcl_InfoPeriodicCSI_RS));
 *nzpres2->qcl_InfoPeriodicCSI_RS=8;
 ASN_SEQUENCE_ADD(&csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list,nzpres2);


 NR_NZP_CSI_RS_Resource *nzpres3 = calloc(1,sizeof(*nzpres3));
 nzpres3->nzp_CSI_RS_ResourceId=3;
 nzpres3->resourceMapping.frequencyDomainAllocation.present = NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row1;
 nzpres3->resourceMapping.frequencyDomainAllocation.choice.row1.buf = calloc(1,1);
 nzpres3->resourceMapping.frequencyDomainAllocation.choice.row1.size = 1;
 nzpres3->resourceMapping.frequencyDomainAllocation.choice.row1.bits_unused = 4;
 nzpres3->resourceMapping.frequencyDomainAllocation.choice.row1.buf[0]=1;
 nzpres3->nrofPorts = NR_CSI_RS_ResourceMapping__nrofPorts_p1;
 nzpres3->firstOFDMSymbolInTimeDomain=8;
 nzpres3->firstOFDMSymbolInTimeDomain2=NULL;
 nzpres3->cdm_Type = NR_CSI_RS_ResourceMapping__cdm_Type_noCDM;
 nzpres3->density.present=NR_CSI_RS_ResourceMapping__density_PR_three;
 nzpres3->density.choice.three=NULL;
 nzpres3->freqBand.startingRB=0;
 nzpres3->freqBand.nrofRBs= scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
 nzpres3->powerControlOffset=0;
 nzpres3->powerControlOffsetSS=calloc(1,sizeof(*nzpres3->powerControlOffsetSS));
 *nzpres3->powerControlOffsetSS=NR_NZP_CSI_RS_Resource__powerControlOffsetSS_db0;
 nzpres3->scramblingID=40;
 nzpres3->periodicityAndOffset = calloc(1,sizeof(*nzpres3->periodicityAndOffset));
 nzpres3->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots160;
 nzpres3->periodicityAndOffset->choice.slots160 = 25;
 nzpres3->qcl_InfoPeriodicCSI_RS=calloc(1,sizeof(*nzpres3->qcl_InfoPeriodicCSI_RS));
 *nzpres3->qcl_InfoPeriodicCSI_RS=8;
 ASN_SEQUENCE_ADD(&csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list,nzpres3);

 NR_NZP_CSI_RS_Resource *nzpres4 = calloc(1,sizeof(*nzpres4));
 nzpres4->nzp_CSI_RS_ResourceId=3;
 nzpres4->resourceMapping.frequencyDomainAllocation.present = NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row1;
 nzpres4->resourceMapping.frequencyDomainAllocation.choice.row1.buf = calloc(1,1);
 nzpres4->resourceMapping.frequencyDomainAllocation.choice.row1.size = 1;
 nzpres4->resourceMapping.frequencyDomainAllocation.choice.row1.bits_unused = 4;
 nzpres4->resourceMapping.frequencyDomainAllocation.choice.row1.buf[0]=1;
 nzpres4->nrofPorts = NR_CSI_RS_ResourceMapping__nrofPorts_p1;
 nzpres4->firstOFDMSymbolInTimeDomain=4;
 nzpres4->firstOFDMSymbolInTimeDomain2=NULL;
 nzpres4->cdm_Type = NR_CSI_RS_ResourceMapping__cdm_Type_noCDM;
 nzpres4->density.present=NR_CSI_RS_ResourceMapping__density_PR_three;
 nzpres4->density.choice.three=NULL;
 nzpres4->freqBand.startingRB=0;
 nzpres4->freqBand.nrofRBs= scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
 nzpres4->powerControlOffset=0;
 nzpres4->powerControlOffsetSS=calloc(1,sizeof(*nzpres4->powerControlOffsetSS));
 *nzpres4->powerControlOffsetSS=NR_NZP_CSI_RS_Resource__powerControlOffsetSS_db0;
 nzpres4->scramblingID=40;
 nzpres4->periodicityAndOffset = calloc(1,sizeof(*nzpres4->periodicityAndOffset));
 nzpres4->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots160;
 nzpres4->periodicityAndOffset->choice.slots160 = 26;
 nzpres4->qcl_InfoPeriodicCSI_RS=calloc(1,sizeof(*nzpres4->qcl_InfoPeriodicCSI_RS));
 *nzpres4->qcl_InfoPeriodicCSI_RS=8;
 ASN_SEQUENCE_ADD(&csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list,nzpres4);

 NR_NZP_CSI_RS_Resource *nzpres5 = calloc(1,sizeof(*nzpres5));
 nzpres5->nzp_CSI_RS_ResourceId=3;
 nzpres5->resourceMapping.frequencyDomainAllocation.present = NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row1;
 nzpres5->resourceMapping.frequencyDomainAllocation.choice.row1.buf = calloc(1,1);
 nzpres5->resourceMapping.frequencyDomainAllocation.choice.row1.size = 1;
 nzpres5->resourceMapping.frequencyDomainAllocation.choice.row1.bits_unused = 4;
 nzpres5->resourceMapping.frequencyDomainAllocation.choice.row1.buf[0]=1;
 nzpres5->nrofPorts = NR_CSI_RS_ResourceMapping__nrofPorts_p1;
 nzpres5->firstOFDMSymbolInTimeDomain=8;
 nzpres5->firstOFDMSymbolInTimeDomain2=NULL;
 nzpres5->cdm_Type = NR_CSI_RS_ResourceMapping__cdm_Type_noCDM;
 nzpres5->density.present=NR_CSI_RS_ResourceMapping__density_PR_three;
 nzpres5->density.choice.three=NULL;
 nzpres5->freqBand.startingRB=0;
 nzpres5->freqBand.nrofRBs= scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
 nzpres5->powerControlOffset=0;
 nzpres5->powerControlOffsetSS=calloc(1,sizeof(*nzpres5->powerControlOffsetSS));
 *nzpres5->powerControlOffsetSS=NR_NZP_CSI_RS_Resource__powerControlOffsetSS_db0;
 nzpres5->scramblingID=40;
 nzpres5->periodicityAndOffset = calloc(1,sizeof(*nzpres5->periodicityAndOffset));
 nzpres5->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots160;
 nzpres5->periodicityAndOffset->choice.slots160 = 26;
 nzpres5->qcl_InfoPeriodicCSI_RS=calloc(1,sizeof(*nzpres5->qcl_InfoPeriodicCSI_RS));
 *nzpres5->qcl_InfoPeriodicCSI_RS=8;
 ASN_SEQUENCE_ADD(&csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list,nzpres5);

nzp-CSI-RS-ResourceSetToAddModList
{
{
nzp-CSI-ResourceSetId 0,
nzp-CSI-RS-Resources
{
0
}
},
{
nzp-CSI-ResourceSetId 3,
nzp-CSI-RS-Resources
{
2,
3,
4,
5
},
repetition off,
trs-Info true


 secondaryCellGroup->spCellConfig->spcellConfigDedicated->sCellDeactivationTimer=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->crossCarrierSchedulingConfig=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->tag_Id=0;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->ue_BeamLockFunction=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->pathlossReferenceLinking=NULL;
 secondaryCellGroup->spCellConfig->spcellConfigDedicated->servingCellMO=NULL;

}
void fill_default_reconfig(NR_ServingCellConfigCommon_t *servingcellconfigcommon,
			   RRCReconfiguration_IEs_t *reconfig,

			   NR_CellGroupConfig_t *secondaryCellGroup) {

  AssertFatal(servingcellconfigcommon!=NULL,"servingcellconfigcommon is null\n");
  AssertFatal(reconfig!=NULL,"reconfig is null\n");
  AssertFatal(secondaryCellGroup!=NULL,"secondaryCellGroup is null\n");

  // radioBearerConfig
  reconfig->radioBearerConfig=NULL;
  // secondaryCellGroup
  fill_default_secondaryCellGroup(servingcellconfigcommon,reconfig,secondaryCellGroup,0);
  // measConfig
  reconfig->measConfig=NULL;
  // lateNonCriticalExtension
  reconfig->lateNonCriticalExtension = NULL;
  // nonCriticalExtension
  reconfig->nonCriticalExtension = NULL;
}


#endif
