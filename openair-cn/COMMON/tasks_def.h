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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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
