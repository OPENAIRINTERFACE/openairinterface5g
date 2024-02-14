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

/* \file rrc_defs.h
 * \brief RRC structures/types definition
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef __OPENAIR_NR_RRC_DEFS_H__
#define __OPENAIR_NR_RRC_DEFS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/platform_types.h"
#include "commonDef.h"
#include "common/platform_constants.h"

#include "NR_asn_constant.h"
#include "NR_MeasConfig.h"
#include "NR_CellGroupConfig.h"
#include "NR_RadioBearerConfig.h"
#include "NR_RLC-BearerConfig.h"
#include "NR_TAG.h"
#include "NR_asn_constant.h"
#include "NR_MIB.h"
#include "NR_SIB1.h"
#include "NR_BCCH-BCH-Message.h"
#include "NR_DL-DCCH-Message.h"
#include "NR_SystemInformation.h"
#include "NR_UE-NR-Capability.h"
#include "NR_SL-PreconfigurationNR-r16.h"
#include "NR_MasterInformationBlockSidelink.h"

#include "RRC/NR/nr_rrc_common.h"
#include "as_message.h"
#include "common/utils/nr/nr_common.h"

#define NB_CNX_UE 2//MAX_MANAGED_RG_PER_MOBILE
#define MAX_MEAS_OBJ 7
#define MAX_MEAS_CONFIG 7
#define MAX_MEAS_ID 7

typedef uint32_t channel_t;

typedef enum {
  nr_SecondaryCellGroupConfig_r15=0,
  nr_RadioBearerConfigX_r15=1
} nsa_message_t;

#define MAX_UE_NR_CAPABILITY_SIZE 2048
typedef struct OAI_NR_UECapability_s {
  uint8_t sdu[MAX_UE_NR_CAPABILITY_SIZE];
  uint16_t sdu_size;
  NR_UE_NR_Capability_t *UE_NR_Capability;
} OAI_NR_UECapability_t;

typedef enum Rrc_State_NR_e {
  RRC_STATE_IDLE_NR = 0,
  RRC_STATE_INACTIVE_NR,
  RRC_STATE_CONNECTED_NR,
  RRC_STATE_DETACH_NR,
  RRC_STATE_FIRST_NR = RRC_STATE_IDLE_NR,
  RRC_STATE_LAST_NR = RRC_STATE_CONNECTED_NR,
} Rrc_State_NR_t;

typedef enum requested_SI_List_e {
  SIB2  = 1,
  SIB3  = 2,
  SIB4  = 4,
  SIB5  = 8,
  SIB6  = 16,
  SIB7  = 32,
  SIB8  = 64,
  SIB9  = 128
} requested_SI_List_t;

// 3GPP TS 38.300 Section 9.2.6
typedef enum RA_trigger_e {
  RA_NOT_RUNNING,
  INITIAL_ACCESS_FROM_RRC_IDLE,
  RRC_CONNECTION_REESTABLISHMENT,
  DURING_HANDOVER,
  NON_SYNCHRONISED,
  TRANSITION_FROM_RRC_INACTIVE,
  TO_ESTABLISH_TA,
  REQUEST_FOR_OTHER_SI,
  BEAM_FAILURE_RECOVERY,
} RA_trigger_t;

typedef struct UE_RRC_SI_INFO_NR_s {
  uint32_t default_otherSI_map;
  NR_SIB1_t *sib1;
  NR_timer_t sib1_timer;
  NR_SIB2_t *sib2;
  NR_timer_t sib2_timer;
  NR_SIB3_t *sib3;
  NR_timer_t sib3_timer;
  NR_SIB4_t *sib4;
  NR_timer_t sib4_timer;
  NR_SIB5_t *sib5;
  NR_timer_t sib5_timer;
  NR_SIB6_t *sib6;
  NR_timer_t sib6_timer;
  NR_SIB7_t *sib7;
  NR_timer_t sib7_timer;
  NR_SIB8_t *sib8;
  NR_timer_t sib8_timer;
  NR_SIB9_t *sib9;
  NR_timer_t sib9_timer;
  NR_SIB10_r16_t *sib10;
  NR_timer_t sib10_timer;
  NR_SIB11_r16_t *sib11;
  NR_timer_t sib11_timer;
  NR_SIB12_r16_t *sib12;
  NR_timer_t sib12_timer;
  NR_SIB13_r16_t *sib13;
  NR_timer_t sib13_timer;
  NR_SIB14_r16_t *sib14;
  NR_timer_t sib14_timer;
} NR_UE_RRC_SI_INFO;

typedef struct NR_UE_Timers_Constants_s {
  // timers
  NR_timer_t T300;
  NR_timer_t T301;
  NR_timer_t T302;
  NR_timer_t T304;
  NR_timer_t T310;
  NR_timer_t T311;
  NR_timer_t T319;
  NR_timer_t T320;
  NR_timer_t T325;
  NR_timer_t T390;
  // counters
  uint32_t N310_cnt;
  uint32_t N311_cnt;
  // constants (limits configured by the network)
  uint32_t N310_k;
  uint32_t N311_k;
} NR_UE_Timers_Constants_t;

typedef enum {
  OUT_OF_SYNC = 0,
  IN_SYNC = 1
} nr_sync_msg_t;

typedef enum { RB_NOT_PRESENT, RB_ESTABLISHED, RB_SUSPENDED } NR_RB_status_t;

typedef struct rrcPerNB {
  NR_MeasObjectToAddMod_t *MeasObj[MAX_MEAS_OBJ];
  NR_ReportConfigToAddMod_t *ReportConfig[MAX_MEAS_CONFIG];
  NR_QuantityConfig_t *QuantityConfig;
  NR_MeasIdToAddMod_t *MeasId[MAX_MEAS_ID];
  NR_MeasGapConfig_t *measGapConfig;
  NR_RB_status_t Srb[NR_NUM_SRB];
  NR_RB_status_t status_DRBs[MAX_DRBS_PER_UE];
  bool active_RLC_entity[NR_MAX_NUM_LCID];
  NR_UE_RRC_SI_INFO SInfo;
  NR_RSRP_Range_t s_measure;
} rrcPerNB_t;

typedef struct NR_UE_RRC_INST_s {
  instance_t ue_id;
  rrcPerNB_t perNB[NB_CNX_UE];

  char                           *uecap_file;
  rnti_t                         rnti;

  OAI_NR_UECapability_t UECap;
  NR_UE_Timers_Constants_t timers_and_constants;
  RA_trigger_t ra_trigger;
  plmn_t plmnID;

  NR_BWP_Id_t dl_bwp_id;
  NR_BWP_Id_t ul_bwp_id;

  /* KgNB as computed from parameters within USIM card */
  uint8_t kgnb[32];
  /* Used integrity/ciphering algorithms */
  //RRC_LIST_TYPE(NR_SecurityAlgorithmConfig_t, NR_SecurityAlgorithmConfig) SecurityAlgorithmConfig_list;
  NR_CipheringAlgorithm_t  cipheringAlgorithm;
  e_NR_IntegrityProtAlgorithm  integrityProtAlgorithm;
  long keyToUse;
  bool as_security_activated;

  long               selected_plmn_identity;
  Rrc_State_NR_t     nrRrcState;
  as_nas_info_t      initialNasMsg;

  //Sidelink params
  NR_SL_PreconfigurationNR_r16_t *sl_preconfig;

} NR_UE_RRC_INST_t;

#endif
