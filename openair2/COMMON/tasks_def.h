/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

// This task is mandatory and must always be placed in first position
TASK_DEF(TASK_TIMER,    TASK_PRIORITY_MAX,          10)

// Other possible tasks in the process

// Common tasks:
/// Layer 2 and Layer 1 task supporting all the synchronous processing
TASK_DEF(TASK_L2L1,     TASK_PRIORITY_MAX_LEAST,    200)

///   Bearers Manager task
TASK_DEF(TASK_BM,       TASK_PRIORITY_MED,          200)

// eNodeB tasks and sub-tasks:
//// Layer 2 and Layer 1 sub-tasks
SUB_TASK_DEF(TASK_L2L1,     TASK_PHY_ENB,               200)
SUB_TASK_DEF(TASK_L2L1,     TASK_MAC_ENB,               200)
SUB_TASK_DEF(TASK_L2L1,     TASK_RLC_ENB,               200)
SUB_TASK_DEF(TASK_L2L1,     TASK_PDCP_ENB,              200)

///   Radio Resource Control task
TASK_DEF(TASK_RRC_ENB,  TASK_PRIORITY_MED,          200)
///   S1ap task
/// RAL task for ENB
TASK_DEF(TASK_RAL_ENB, TASK_PRIORITY_MED, 200)

// UDP TASK
TASK_DEF(TASK_UDP,      TASK_PRIORITY_MED,          1000)
// GTP_V1U task
TASK_DEF(TASK_GTPV1_U,  TASK_PRIORITY_MED,          1000)
TASK_DEF(TASK_S1AP,     TASK_PRIORITY_MED,          200)
///   X2ap task, acts as both source and target
TASK_DEF(TASK_X2AP,     TASK_PRIORITY_MED,          200)
///   Sctp task (Used by both S1AP and X2AP)
TASK_DEF(TASK_SCTP,     TASK_PRIORITY_MED,          200)
///   eNB APP task
TASK_DEF(TASK_ENB_APP,  TASK_PRIORITY_MED,          200)

// UE tasks and sub-tasks:
//// Layer 2 and Layer 1 sub-tasks
SUB_TASK_DEF(TASK_L2L1,     TASK_PHY_UE,                200)
SUB_TASK_DEF(TASK_L2L1,     TASK_MAC_UE,                200)
SUB_TASK_DEF(TASK_L2L1,     TASK_RLC_UE,                200)
SUB_TASK_DEF(TASK_L2L1,     TASK_PDCP_UE,               200)

///   Radio Resource Control task
TASK_DEF(TASK_RRC_UE,   TASK_PRIORITY_MED,          200)
///   Non Access Stratum task
TASK_DEF(TASK_NAS_UE,   TASK_PRIORITY_MED,          200)
TASK_DEF(TASK_RAL_UE,   TASK_PRIORITY_MED,          200)

//MESSAGE GENERATOR TASK
TASK_DEF(TASK_MSC,      TASK_PRIORITY_MED,          200)

