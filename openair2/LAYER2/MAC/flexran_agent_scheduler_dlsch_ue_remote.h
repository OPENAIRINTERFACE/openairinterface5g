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

/*! \file flexran_agent_scheduler_dlsch_ue_remote.h
 * \brief Local stub for remote scheduler used by the controller
 * \author Xenofon Foukas
 * \date 2016
 * \email: x.foukas@sms.ed.ac.uk
 * \version 0.1
 * @ingroup _mac

 */

#ifndef __LAYER2_MAC_FLEXRAN_AGENT_SCHEDULER_DLSCH_UE_REMOTE_H__
#define __LAYER2_MAC_FLEXRAN_AGENT_SCHEDULER_DLSCH_UE_REMOTE_H___

#include "flexran.pb-c.h"
#include "header.pb-c.h"

#include "ENB_APP/flexran_agent_defs.h"
#include "flexran_agent_mac.h"
#include "LAYER2/MAC/flexran_agent_mac_proto.h"

#include <sys/queue.h>

// Maximum value of schedule ahead of time
// Required to identify if a dl_command is for the future or not
#define SCHED_AHEAD_SUBFRAMES 20

typedef struct dl_mac_config_element_s {
    Protocol__FlexranMessage *dl_info;
     TAILQ_ENTRY(dl_mac_config_element_s) configs;
} dl_mac_config_element_t;

TAILQ_HEAD(DlMacConfigHead, dl_mac_config_element_s);

/*
 * Default scheduler used by the eNB agent
 */
void flexran_schedule_ue_spec_remote(mid_t mod_id, uint32_t frame,
				     uint32_t subframe, int *mbsfn_flag,
				     Protocol__FlexranMessage ** dl_info);


// Find the difference in subframes from the given subframe
// negative for older value
// 0 for equal
// positive for future value
// Based on  
int get_sf_difference(mid_t mod_id, uint32_t sfn_sf);

#endif
