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

#include "nr_rrc_defs.h"

typedef struct rlc_bearer_config_s{
  long        LogicalChannelIdentity;
  long        servedRadioBearer_present;
  long        srb_Identity;
  long        drb_Identity;
  long        reestablishRLC;
  long        rlc_Config_present;
  long        ul_AM_sn_FieldLength;
  long        t_PollRetransmit;
  long        pollPDU;
  long        pollByte;
  long        maxRetxThreshold;
  long        dl_AM_sn_FieldLength;
  long        dl_AM_t_Reassembly;
  long        t_StatusProhibit;
  long        ul_UM_sn_FieldLength;
  long        dl_UM_sn_FieldLength;
  long        dl_UM_t_Reassembly;
  long        priority;
  long        prioritisedBitRate;
  long        bucketSizeDuration;
  long        allowedServingCells;
  long        subcarrierspacing;
  long        maxPUSCH_Duration;
  long        configuredGrantType1Allowed;
  long        logicalChannelGroup;
  long        schedulingRequestID; /* OPTIONAL */
  BOOLEAN_t   logicalChannelSR_Mask;
  BOOLEAN_t   logicalChannelSR_DelayTimerApplied;
}rlc_bearer_config_t;

typedef struct mac_cellgroup_s{
  long        DRX_Config_PR;
  long        drx_onDurationTimer_PR;
  long        subMilliSeconds;
  long        milliSeconds;
  long        drx_InactivityTimer;
  long        drx_HARQ_RTT_TimerDL;
  long        drx_HARQ_RTT_TimerUL;
  long        drx_RetransmissionTimerDL;
  long        drx_RetransmissionTimerUL;
  long        drx_LongCycleStartOffset_PR;
  long        drx_LongCycleStartOffset;
  long        drx_ShortCycle;
  long        drx_ShortCycleTimer;
  long        drx_SlotOffset;
  long        schedulingRequestId;
  long        sr_ProhibitTimer;
  long        sr_TransMax;
  long        periodicBSR_Timer;
  long        retxBSR_Timer;
  long        logicalChannelSR_DelayTimer;
  long        tag_Id;
  long        timeAlignmentTimer;
  long        PHR_Config_PR;
  long        phr_PeriodicTimer;
  long        phr_ProhibitTimer;
  long        phr_Tx_PowerFactorChange;
  BOOLEAN_t   multiplePHR;
  BOOLEAN_t   phr_Type2SpCell;
  BOOLEAN_t   phr_Type2OtherCell;
  long        phr_ModeOtherCG;
  BOOLEAN_t   skipUplinkTxDynamic;
}mac_cellgroup_t;

typedef struct physicalcellgroup_s{
  long        harq_ACK_SpatialBundlingPUCCH;
  long        harq_ACK_SpatialBundlingPUSCH;
  long        p_NR;
  long        pdsch_HARQ_ACK_Codebook;
  long        tpc_SRS_RNTI;
  long        tpc_PUCCH_RNTI;
  long        tpc_PUSCH_RNTI;
  long        sp_CSI_RNTI;
  long        RNTI_Value;
}physicalcellgroup_t;

void rrc_config_servingcellconfigcommon(uint8_t Mod_id,
                                        int CC_id
                                        #if defined(ENABLE_ITTI)
                                        ,gNB_RrcConfigurationReq *common_configuration
                                        #endif
                                       );

void rrc_config_rlc_bearer(uint8_t Mod_id,
                           int CC_id,
                           rlc_bearer_config_t *rlc_config
                          );

void rrc_config_mac_cellgroup(uint8_t Mod_id,
                              int CC_id,
                              mac_cellgroup_t *mac_cellgroup_config
                             );

void rrc_config_physicalcellgroup(uint8_t Mod_id,
                                  int CC_id,
                                  physicalcellgroup_t *physicalcellgroup_config
                                 );
