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

/*! \file LAYER2/MAC/eNB_scheduler_dlsch.h
* \brief DLSCH Scheduler policy variables used during different phases of scheduling
* \author Navid Nikaein and Niccolo' Iardella
* \date 2018
* \version 0.2
* \email navid.nikaein@eurecom.fr

*/
/** @defgroup _oai2  openair2 Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

/*@}*/

#ifndef __LAYER2_MAC_ENB_SCHEDULER_DLSCH_H__
#define __LAYER2_MAC_ENB_SCHEDULER_DLSCH_H__

// number of active slices for  past and current time
int       n_active_slices = 1;
int       n_active_slices_current = 1;
int       slice_counter = 0;

int       intraslice_share_active = 1;
int       intraslice_share_active_current = 1;
int       interslice_share_active = 1;
int       interslice_share_active_current = 1;

// RB share for each slice for past and current time
float     slice_percentage[MAX_NUM_SLICES]         = {1.0, 0.0, 0.0, 0.0};
float     slice_percentage_current[MAX_NUM_SLICES] = {1.0, 0.0, 0.0, 0.0};
float     slice_percentage_total = 0;
float     slice_percentage_total_current = 0;
float     slice_percentage_avg = 0.25;

int       slice_isolation[MAX_NUM_SLICES] = {0, 0, 0, 0};
int       slice_isolation_current[MAX_NUM_SLICES] = {0, 0, 0, 0};
int       slice_priority[MAX_NUM_SLICES] = {10, 5, 2, 0};
int       slice_priority_current[MAX_NUM_SLICES] = {10, 5, 2, 0};

// Frequency ranges for slice positioning
int       slice_position[MAX_NUM_SLICES*2]         = {0, N_RBG_MAX, 0, N_RBG_MAX, 0, N_RBG_MAX, 0, N_RBG_MAX};
int       slice_position_current[MAX_NUM_SLICES*2] = {0, N_RBG_MAX, 0, N_RBG_MAX, 0, N_RBG_MAX, 0, N_RBG_MAX};

// MAX MCS for each slice for past and current time
int       slice_maxmcs[MAX_NUM_SLICES]         = { 28, 28, 28, 28 };
int       slice_maxmcs_current[MAX_NUM_SLICES] = { 28, 28, 28, 28 };

// The lists of criteria that enforce the sorting policies of the slices
uint32_t  slice_sorting[MAX_NUM_SLICES]         = {0x012345, 0x012345, 0x012345, 0x012345};
uint32_t  slice_sorting_current[MAX_NUM_SLICES] = {0x012345, 0x012345, 0x012345, 0x012345};

// Accounting policy (just greedy(1) or fair(0) setting for now)
int       slice_accounting[MAX_NUM_SLICES]         = {0, 0, 0, 0};
int       slice_accounting_current[MAX_NUM_SLICES] = {0, 0, 0, 0};

int       update_dl_scheduler[MAX_NUM_SLICES]         = { 1, 1, 1, 1 };
int       update_dl_scheduler_current[MAX_NUM_SLICES] = { 1, 1, 1, 1 };

// name of available scheduler
char *dl_scheduler_type[MAX_NUM_SLICES] =
        { "schedule_ue_spec",
          "schedule_ue_spec",
          "schedule_ue_spec",
          "schedule_ue_spec"
        };

// pointer to the slice specific scheduler
slice_scheduler_dl slice_sched_dl[MAX_NUM_SLICES] = {0};

pre_processor_results_t pre_processor_results[MAX_NUM_SLICES];

#endif //__LAYER2_MAC_ENB_SCHEDULER_DLSCH_H__
