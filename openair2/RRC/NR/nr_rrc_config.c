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

/*! \file nr_rrc_config.c
 * \brief rrc config for gNB
 * \author Raymond Knopp, WEI-TAI CHEN
 * \date 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: raymond.knopp@eurecom.fr, kroempa@gmail.com
 */

#include "nr_rrc_config.h"

void rrc_config_rlc_bearer(uint8_t Mod_id,
                           int CC_id,
                           rlc_bearer_config_t *rlc_config
                          ){
  rlc_config->LogicalChannelIdentity[CC_id]                 = 0;
  rlc_config->servedRadioBearer_present[CC_id]              = 0;
  rlc_config->srb_Identity[CC_id]                           = 0;
  rlc_config->drb_Identity[CC_id]                           = 0;
  rlc_config->reestablishRLC[CC_id]                         = 0;
  rlc_config->rlc_Config_present[CC_id]                     = 0;
  rlc_config->ul_AM_sn_FieldLength[CC_id]                   = 0;
  rlc_config->t_PollRetransmit[CC_id]                       = 0;
  rlc_config->pollPDU[CC_id]                                = 0;
  rlc_config->pollByte[CC_id]                               = 0;
  rlc_config->maxRetxThreshold[CC_id]                       = 0;
  rlc_config->dl_AM_sn_FieldLength[CC_id]                   = 0;
  rlc_config->dl_AM_t_Reassembly[CC_id]                     = 0;
  rlc_config->t_StatusProhibit[CC_id]                       = 0;
  rlc_config->ul_UM_sn_FieldLength[CC_id]                   = 0;
  rlc_config->dl_UM_sn_FieldLength[CC_id]                   = 0;
  rlc_config->dl_UM_t_Reassembly[CC_id]                     = 0;
  rlc_config->priority[CC_id]                               = 0;
  rlc_config->prioritisedBitRate[CC_id]                     = 0;
  rlc_config->bucketSizeDuration[CC_id]                     = 0;
  rlc_config->allowedServingCells[CC_id]                    = 0;
  rlc_config->subcarrierspacing[CC_id]                      = 0;
  rlc_config->maxPUSCH_Duration[CC_id]                      = 0;
  rlc_config->configuredGrantType1Allowed[CC_id]            = 0;
  rlc_config->logicalChannelGroup[CC_id]                    = 0;
  rlc_config->schedulingRequestID[CC_id]                    = 0;
  rlc_config->logicalChannelSR_Mask[CC_id]                  = 0;
  rlc_config->logicalChannelSR_DelayTimerApplied[CC_id]     = 0;
}

void rrc_config_mac_cellgroup(uint8_t Mod_id,
                              int CC_id,
                              mac_cellgroup_t *mac_cellgroup_config
                             ){
  mac_cellgroup_config->DRX_Config_PR[CC_id]                = 0;
  mac_cellgroup_config->drx_onDurationTimer_PR[CC_id]       = 0;
  mac_cellgroup_config->subMilliSeconds[CC_id]              = 0;
  mac_cellgroup_config->milliSeconds[CC_id]                 = 0;
  mac_cellgroup_config->drx_InactivityTimer[CC_id]          = 0;
  mac_cellgroup_config->drx_HARQ_RTT_TimerDL[CC_id]         = 0;
  mac_cellgroup_config->drx_HARQ_RTT_TimerUL[CC_id]         = 0;
  mac_cellgroup_config->drx_RetransmissionTimerDL[CC_id]    = 0;
  mac_cellgroup_config->drx_RetransmissionTimerUL[CC_id]    = 0;
  mac_cellgroup_config->drx_LongCycleStartOffset_PR[CC_id]  = 0;
  mac_cellgroup_config->drx_LongCycleStartOffset[CC_id]     = 0;
  mac_cellgroup_config->drx_ShortCycle[CC_id]               = 0;
  mac_cellgroup_config->drx_ShortCycleTimer[CC_id]          = 0;
  mac_cellgroup_config->drx_SlotOffset[CC_id]               = 0;
  mac_cellgroup_config->schedulingRequestId[CC_id]          = 0;
  mac_cellgroup_config->sr_ProhibitTimer[CC_id]             = 0;
  mac_cellgroup_config->sr_TransMax[CC_id]                  = 0;
  mac_cellgroup_config->periodicBSR_Timer[CC_id]            = 0;
  mac_cellgroup_config->retxBSR_Timer[CC_id]                = 0;
  mac_cellgroup_config->logicalChannelSR_DelayTimer[CC_id]  = 0;
  mac_cellgroup_config->tag_Id[CC_id]                       = 0;
  mac_cellgroup_config->timeAlignmentTimer[CC_id]           = 0;
  mac_cellgroup_config->PHR_Config_PR[CC_id]                = 0;
  mac_cellgroup_config->phr_PeriodicTimer[CC_id]            = 0;
  mac_cellgroup_config->phr_ProhibitTimer[CC_id]            = 0;
  mac_cellgroup_config->phr_Tx_PowerFactorChange[CC_id]     = 0;
  mac_cellgroup_config->multiplePHR[CC_id]                  = 0;
  mac_cellgroup_config->phr_Type2SpCell[CC_id]              = 0;
  mac_cellgroup_config->phr_Type2OtherCell[CC_id]           = 0;
  mac_cellgroup_config->phr_ModeOtherCG[CC_id]              = 0;
  mac_cellgroup_config->skipUplinkTxDynamic[CC_id]          = 0;
}

void rrc_config_physicalcellgroup(uint8_t Mod_id,
                                  int CC_id,
                                  physicalcellgroup_t *physicalcellgroup_config
                                 ){
  physicalcellgroup_config->harq_ACK_SpatialBundlingPUCCH[CC_id]    = 0;
  physicalcellgroup_config->harq_ACK_SpatialBundlingPUSCH[CC_id]    = 0;
  physicalcellgroup_config->p_NR[CC_id]                             = 0;
  physicalcellgroup_config->pdsch_HARQ_ACK_Codebook[CC_id]          = 0;
  physicalcellgroup_config->tpc_SRS_RNTI[CC_id]                     = 0;
  physicalcellgroup_config->tpc_PUCCH_RNTI[CC_id]                   = 0;
  physicalcellgroup_config->tpc_PUSCH_RNTI[CC_id]                   = 0;
  physicalcellgroup_config->sp_CSI_RNTI[CC_id]                      = 0;
  physicalcellgroup_config->RNTI_Value[CC_id]                       = 0;
}

void rrc_config_rachdedicated(uint8_t Mod_id,
                              int CC_id,
                              physicalcellgroup_t *physicalcellgroup_config
                              ){
  
}
