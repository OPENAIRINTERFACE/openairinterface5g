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

/*! \file LAYER2/MAC/eNB_scheduler_ulsch.h
* \brief ULSCH Scheduler policy variables used during different phases of scheduling
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

#ifndef __LAYER2_MAC_ENB_SCHEDULER_ULSCH_H__
#define __LAYER2_MAC_ENB_SCHEDULER_ULSCH_H__

/* number of active slices for  past and current time*/
int n_active_slices_uplink = 1;
int n_active_slices_current_uplink = 1;

/* RB share for each slice for past and current time*/
float avg_slice_percentage_uplink=0.25;
float slice_percentage_uplink[MAX_NUM_SLICES] = {1.0, 0.0, 0.0, 0.0};
float slice_percentage_current_uplink[MAX_NUM_SLICES] = {1.0, 0.0, 0.0, 0.0};
float total_slice_percentage_uplink = 0;
float total_slice_percentage_current_uplink = 0;

// MAX MCS for each slice for past and current time
int slice_maxmcs_uplink[MAX_NUM_SLICES] = {20, 20, 20, 20};
int slice_maxmcs_current_uplink[MAX_NUM_SLICES] = {20,20,20,20};

/*resource blocks allowed*/
uint16_t         nb_rbs_allowed_slice_uplink[NFAPI_CC_MAX][MAX_NUM_SLICES];
/*Slice Update */
int update_ul_scheduler[MAX_NUM_SLICES] = {1, 1, 1, 1};
int update_ul_scheduler_current[MAX_NUM_SLICES] = {1, 1, 1, 1};

/* name of available scheduler*/
char *ul_scheduler_type[MAX_NUM_SLICES] = {"schedule_ulsch_rnti",
                                           "schedule_ulsch_rnti",
                                           "schedule_ulsch_rnti",
                                           "schedule_ulsch_rnti"
};

/* Slice Function Pointer */
slice_scheduler_ul slice_sched_ul[MAX_NUM_SLICES] = {0};

#endif //__LAYER2_MAC_ENB_SCHEDULER_ULSCH_H__
