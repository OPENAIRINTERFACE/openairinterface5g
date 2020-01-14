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

// This task is mandatory and must always be placed in first position
TASK_DEF(TASK_TIMER,    TASK_PRIORITY_MAX,          10)

// Other possible tasks in the process

// Common tasks:

///   Bearers Manager task
TASK_DEF(TASK_BM,       TASK_PRIORITY_MED,          200)

// eNodeB tasks and sub-tasks:

///   Radio Resource Control task
TASK_DEF(TASK_RRC_ENB,  TASK_PRIORITY_MED,          200)

// Define here for now
TASK_DEF(TASK_RRC_ENB_NB_IoT,  TASK_PRIORITY_MED,          200)

///   S1ap task
/// RAL task for ENB
TASK_DEF(TASK_RAL_ENB, TASK_PRIORITY_MED, 200)

// UDP TASK
TASK_DEF(TASK_UDP,      TASK_PRIORITY_MED,          1000)
// GTP_V1U task
TASK_DEF(TASK_GTPV1_U,  TASK_PRIORITY_MED,          1000)
TASK_DEF(TASK_S1AP,     TASK_PRIORITY_MED,          200)
TASK_DEF(TASK_CU_F1,     TASK_PRIORITY_MED,          200)
TASK_DEF(TASK_DU_F1,     TASK_PRIORITY_MED,          200)
///   M3ap task, acts as both source and target
TASK_DEF(TASK_M3AP,     TASK_PRIORITY_MED,          200)
///   M3ap task, acts as both source and target
TASK_DEF(TASK_M3AP_MME,     TASK_PRIORITY_MED,          200)
///   M3ap task, acts as both source and target
TASK_DEF(TASK_M3AP_MCE,     TASK_PRIORITY_MED,          200)
///   M2ap task, acts as both source and target
TASK_DEF(TASK_M2AP_MCE,     TASK_PRIORITY_MED,          200)
TASK_DEF(TASK_M2AP_ENB,     TASK_PRIORITY_MED,          200)
///   X2ap task, acts as both source and target
TASK_DEF(TASK_X2AP,     TASK_PRIORITY_MED,          200)
///   Sctp task (Used by both S1AP and X2AP)
TASK_DEF(TASK_SCTP,     TASK_PRIORITY_MED,          200)
///   eNB APP task
TASK_DEF(TASK_ENB_APP,  TASK_PRIORITY_MED,          200)
///   eNB Agent task
TASK_DEF(TASK_FLEXRAN_AGENT,  TASK_PRIORITY_MED,          200)
TASK_DEF(TASK_PROTO_AGENT,  TASK_PRIORITY_MED,          200)
// UE tasks and sub-tasks:

///   Radio Resource Control task
TASK_DEF(TASK_RRC_UE,   TASK_PRIORITY_MED,          200)
///   Non Access Stratum task
TASK_DEF(TASK_NAS_UE,   TASK_PRIORITY_MED,          200)
TASK_DEF(TASK_RAL_UE,   TASK_PRIORITY_MED,          200)

//MESSAGE GENERATOR TASK
TASK_DEF(TASK_MSC,      TASK_PRIORITY_MED,          200)

