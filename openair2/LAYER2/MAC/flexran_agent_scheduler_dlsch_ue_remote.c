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

/*! \file flexran_agent_scheduler_dlsch_ue_remote.c
 * \brief procedures related to remote scheduling in the DLSCH transport channel
 * \author Xenofon Foukas
 * \date 2016
 * \email: x.foukas@sms.ed.ac.uk
 * \version 0.1
 * @ingroup _mac

 */

#include "flexran_agent_common_internal.h"

#include "flexran_agent_scheduler_dlsch_ue_remote.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"

struct DlMacConfigHead queue_head;

int queue_initialized = 0;

//uint32_t skip_subframe = 1;
//uint32_t period = 10;
//uint32_t sched [] = {1, 2, 3};

void
flexran_schedule_ue_spec_remote(mid_t mod_id, uint32_t frame,
				uint32_t subframe, int *mbsfn_flag,
				Protocol__FlexranMessage ** dl_info)
{


    //if ((subframe == skip_subframe) && (frame % period == 0)) {
    //  LOG_I(MAC, "Will skip subframe %d %d\n", subframe, frame);
    //  for (int i = 0; i < 3; i++) {
    //    LOG_I(MAC, "%d\n", sched[i]);
    //  }
    //}

    /* if (frame == 500 && subframe == 1) { */
    /*   char policy[] = "rrc: \n - ul_scheduler: \n    behavior : tester_function\n    parameters:\n      period: !!int 3\nmac: \n - dl_scheduler: \n    parameters: \n      period : !!int 40\n      skip_subframe : !!int 3\n      sched : [!!int 4, !!int 5, !!int 6]"; */
    /*   apply_reconfiguration_policy(mod_id, policy, strlen(policy)); */
    /* } */

    eNB_MAC_INST *eNB;

    if (!queue_initialized) {
	TAILQ_INIT(&queue_head);
	queue_initialized = 1;
    }

    eNB = &eNB_mac_inst[mod_id];

    dl_mac_config_element_t *dl_config_elem;

    int diff;
    LOG_D(MAC, "[TEST] Current frame and subframe %d, %d\n", frame,
	  subframe);
    // First we check to see if we have a scheduling decision for this sfn_sf already in our queue
    while (queue_head.tqh_first != NULL) {
	dl_config_elem = queue_head.tqh_first;

	diff =
	    get_sf_difference(mod_id,
			      dl_config_elem->dl_info->
			      dl_mac_config_msg->sfn_sf);
	// Check if this decision is for now, for a later or a previous subframe
	if (diff == 0) {	// Now
	    LOG_D(MAC,
		  "Found a decision for this subframe in the queue. Let's use it!\n");
	    TAILQ_REMOVE(&queue_head, queue_head.tqh_first, configs);
	    *dl_info = dl_config_elem->dl_info;
	    free(dl_config_elem);
	    eNB->eNB_stats[mod_id].sched_decisions++;
	    return;
	} else if (diff < 0) {	//previous subframe , delete message and free memory
	    LOG_D(MAC,
		  "Found a decision for a previous subframe in the queue. Let's get rid of it\n");
	    TAILQ_REMOVE(&queue_head, queue_head.tqh_first, configs);
	    flexran_agent_mac_destroy_dl_config(dl_config_elem->dl_info);
	    free(dl_config_elem);
	    eNB->eNB_stats[mod_id].sched_decisions++;
	    eNB->eNB_stats[mod_id].missed_deadlines++;
	} else {		// next subframe, nothing to do now
	    LOG_D(MAC,
		  "Found a decision for a future subframe in the queue. Nothing to do now\n");
	    flexran_agent_mac_create_empty_dl_config(mod_id, dl_info);
	    return;
	}
    }

    //Done with the local cache. Now we need to check if something new arrived
    flexran_agent_get_pending_dl_mac_config(mod_id, dl_info);
    while (*dl_info != NULL) {

	diff =
	    get_sf_difference(mod_id,
			      (*dl_info)->dl_mac_config_msg->sfn_sf);
	if (diff == 0) {	// Got a command for this sfn_sf
	    LOG_D(MAC,
		  "Found a decision for this subframe pending. Let's use it\n");
	    eNB->eNB_stats[mod_id].sched_decisions++;
	    return;
	} else if (diff < 0) {
	    LOG_D(MAC,
		  "Found a decision for a previous subframe. Let's get rid of it\n");
	    flexran_agent_mac_destroy_dl_config(*dl_info);
	    *dl_info = NULL;
	    flexran_agent_get_pending_dl_mac_config(mod_id, dl_info);
	    eNB->eNB_stats[mod_id].sched_decisions++;
	    eNB->eNB_stats[mod_id].missed_deadlines++;
	} else {		// Intended for future subframe. Store it in local cache
	    LOG_D(MAC,
		  "Found a decision for a future subframe in the queue. Let's store it in the cache\n");
	    dl_mac_config_element_t *e =
		malloc(sizeof(dl_mac_config_element_t));
	    e->dl_info = *dl_info;
	    TAILQ_INSERT_TAIL(&queue_head, e, configs);
	    flexran_agent_mac_create_empty_dl_config(mod_id, dl_info);
	    // No need to look for another. Messages arrive ordered
	    return;
	}
    }

    // We found no pending command, so we will simply pass an empty one
    flexran_agent_mac_create_empty_dl_config(mod_id, dl_info);
}

int get_sf_difference(mid_t mod_id, uint32_t sfn_sf)
{
    int diff_in_subframes;

    uint16_t current_frame = flexran_get_current_system_frame_num(mod_id);
    uint16_t current_subframe = flexran_get_current_subframe(mod_id);
    uint32_t current_sfn_sf = flexran_get_sfn_sf(mod_id);

    if (sfn_sf == current_sfn_sf) {
	return 0;
    }

    uint16_t frame_mask = ((1 << 12) - 1);
    uint16_t frame = (sfn_sf & (frame_mask << 4)) >> 4;

    uint16_t sf_mask = ((1 << 4) - 1);
    uint16_t subframe = (sfn_sf & sf_mask);

    LOG_D(MAC, "[TEST] Target frame and subframe %d, %d\n", frame,
	  subframe);

    if (frame == current_frame) {
	return subframe - current_subframe;
    } else if (frame > current_frame) {
	diff_in_subframes =
	    ((frame * 10) + subframe) - ((current_frame * 10) +
					 current_subframe);

	//    diff_in_subframes = 9 - current_subframe;
	//diff_in_subframes += (subframe + 1);
	//diff_in_subframes += (frame-2) * 10;
	if (diff_in_subframes > SCHED_AHEAD_SUBFRAMES) {
	    return -1;
	} else {
	    return 1;
	}
    } else {			//frame < current_frame
	//diff_in_subframes = 9 - current_subframe;
	//diff_in_subframes += (subframe + 1);
	//if (frame > 0) {
	//  diff_in_subframes += (frame - 1) * 10;
	//}
	//diff_in_subframes += (1023 - current_frame) * 10;
	diff_in_subframes =
	    10240 - ((current_frame * 10) + current_subframe) +
	    ((frame * 10) + subframe);
	if (diff_in_subframes > SCHED_AHEAD_SUBFRAMES) {
	    return -1;
	} else {
	    return 1;
	}
    }
}
