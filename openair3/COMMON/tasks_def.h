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
TASK_DEF(TASK_TIMER,    TASK_PRIORITY_MED, 10)

// Other possible tasks in the process

/// GTPV1-U task
TASK_DEF(TASK_GTPV1_U,  TASK_PRIORITY_MED, 200)
/// FW_IP task
TASK_DEF(TASK_FW_IP,    TASK_PRIORITY_MED, 200)
/// MME Applicative task
TASK_DEF(TASK_MME_APP,  TASK_PRIORITY_MED, 200)
/// NAS task
TASK_DEF(TASK_NAS_MME,  TASK_PRIORITY_MED, 200)
/// S11 task
TASK_DEF(TASK_S11,      TASK_PRIORITY_MED, 200)
/// S1AP task
TASK_DEF(TASK_S1AP,     TASK_PRIORITY_MED, 200)
/// S6a task
TASK_DEF(TASK_S6A,      TASK_PRIORITY_MED, 200)
/// SCTP task
TASK_DEF(TASK_SCTP,     TASK_PRIORITY_MED, 200)
/// Serving and Proxy Gateway Application task
TASK_DEF(TASK_SPGW_APP, TASK_PRIORITY_MED, 200)
/// UDP task
TASK_DEF(TASK_UDP,      TASK_PRIORITY_MED, 200)
//MESSAGE GENERATOR TASK
TASK_DEF(TASK_MSC,      TASK_PRIORITY_MED,          200)
