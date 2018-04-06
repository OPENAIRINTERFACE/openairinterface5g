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

/*! \file pre_processor.c
 * \brief eNB scheduler preprocessing fuction prior to scheduling
 * \author Navid Nikaein and Ankit Bhamri
 * \date 2013 - 2014
 * \email navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

#define _GNU_SOURCE
#include <stdlib.h>

#include "assertions.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "rlc.h"


#define DEBUG_eNB_SCHEDULER 1
#define DEBUG_HEADER_PARSING 1
//#define DEBUG_PACKET_TRACE 1

extern float slice_percentage[MAX_NUM_SLICES];
extern float slice_percentage_uplink[MAX_NUM_SLICES];
extern uint32_t sorting_policy[MAX_NUM_SLICES];

extern int slice_maxmcs[MAX_NUM_SLICES];
extern int slice_maxmcs_uplink[MAX_NUM_SLICES];


//#define ICIC 0

/* this function checks that get_eNB_UE_stats returns
 * a non-NULL pointer for all the active CCs of an UE
 */
/*
int phy_stats_exist(module_id_t Mod_id, int rnti)
{
  int CC_id;
  int i;
  int UE_id          = find_UE_id(Mod_id, rnti);
  UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
  if (UE_id == -1) {
    LOG_W(MAC, "[eNB %d] UE %x not found, should be there (in phy_stats_exist)\n",
	  Mod_id, rnti);
    return 0;
  }
  if (UE_list->numactiveCCs[UE_id] == 0) {
    LOG_W(MAC, "[eNB %d] UE %x has no active CC (in phy_stats_exist)\n",
	  Mod_id, rnti);
    return 0;
  }
  for (i = 0; i < UE_list->numactiveCCs[UE_id]; i++) {
    CC_id = UE_list->ordered_CCids[i][UE_id];
    if (mac_xface->get_eNB_UE_stats(Mod_id, CC_id, rnti) == NULL)
      return 0;
  }
  return 1;
}
*/

// This function stores the downlink buffer for all the logical channels
void
store_dlsch_buffer(module_id_t Mod_id, slice_id_t slice_id, frame_t frameP,
		   sub_frame_t subframeP)
{

    int UE_id, i;
    rnti_t rnti;
    mac_rlc_status_resp_t rlc_status;
    UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
    UE_TEMPLATE *UE_template;

    for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
	if (UE_list->active[UE_id] != TRUE)
	    continue;

	if (!ue_slice_membership(UE_id, slice_id))
	    continue;

	UE_template =
	    &UE_list->UE_template[UE_PCCID(Mod_id, UE_id)][UE_id];

	// clear logical channel interface variables
	UE_template->dl_buffer_total = 0;
	UE_template->dl_pdus_total = 0;

	for (i = 0; i < MAX_NUM_LCID; i++) {
	    UE_template->dl_buffer_info[i] = 0;
	    UE_template->dl_pdus_in_buffer[i] = 0;
	    UE_template->dl_buffer_head_sdu_creation_time[i] = 0;
	    UE_template->dl_buffer_head_sdu_remaining_size_to_send[i] = 0;
	}

	rnti = UE_RNTI(Mod_id, UE_id);

	for (i = 0; i < MAX_NUM_LCID; i++) {	// loop over all the logical channels

	    rlc_status =
		mac_rlc_status_ind(Mod_id, rnti, Mod_id, frameP, subframeP,
				   ENB_FLAG_YES, MBMS_FLAG_NO, i, 0);
	    UE_template->dl_buffer_info[i] = rlc_status.bytes_in_buffer;	//storing the dlsch buffer for each logical channel
	    UE_template->dl_pdus_in_buffer[i] = rlc_status.pdus_in_buffer;
	    UE_template->dl_buffer_head_sdu_creation_time[i] =
		rlc_status.head_sdu_creation_time;
	    UE_template->dl_buffer_head_sdu_creation_time_max =
		cmax(UE_template->dl_buffer_head_sdu_creation_time_max,
		     rlc_status.head_sdu_creation_time);
	    UE_template->dl_buffer_head_sdu_remaining_size_to_send[i] =
		rlc_status.head_sdu_remaining_size_to_send;
	    UE_template->dl_buffer_head_sdu_is_segmented[i] =
		rlc_status.head_sdu_is_segmented;
	    UE_template->dl_buffer_total += UE_template->dl_buffer_info[i];	//storing the total dlsch buffer
	    UE_template->dl_pdus_total +=
		UE_template->dl_pdus_in_buffer[i];

#ifdef DEBUG_eNB_SCHEDULER

	    /* note for dl_buffer_head_sdu_remaining_size_to_send[i] :
	     * 0 if head SDU has not been segmented (yet), else remaining size not already segmented and sent
	     */
	    if (UE_template->dl_buffer_info[i] > 0)
		LOG_D(MAC,
		      "[eNB %d][SLICE %d] Frame %d Subframe %d : RLC status for UE %d in LCID%d: total of %d pdus and size %d, head sdu queuing time %d, remaining size %d, is segmeneted %d \n",
		      Mod_id, slice_id, frameP, subframeP, UE_id,
		      i, UE_template->dl_pdus_in_buffer[i],
		      UE_template->dl_buffer_info[i],
		      UE_template->dl_buffer_head_sdu_creation_time[i],
		      UE_template->
		      dl_buffer_head_sdu_remaining_size_to_send[i],
		      UE_template->dl_buffer_head_sdu_is_segmented[i]);

#endif

	}

	//#ifdef DEBUG_eNB_SCHEDULER
	if (UE_template->dl_buffer_total > 0)
	    LOG_D(MAC,
		  "[eNB %d] Frame %d Subframe %d : RLC status for UE %d : total DL buffer size %d and total number of pdu %d \n",
		  Mod_id, frameP, subframeP, UE_id,
		  UE_template->dl_buffer_total,
		  UE_template->dl_pdus_total);

	//#endif
    }
}


// This function returns the estimated number of RBs required by each UE for downlink scheduling
void
assign_rbs_required(module_id_t Mod_id,
			slice_id_t slice_id,
		    frame_t frameP,
		    sub_frame_t subframe,
		    uint16_t
		    nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
		    int min_rb_unit[MAX_NUM_CCs])
{

    uint16_t TBS = 0;

    int UE_id, n, i, j, CC_id, pCCid, tmp;
    UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
    eNB_UE_STATS *eNB_UE_stats, *eNB_UE_stats_i, *eNB_UE_stats_j;
    int N_RB_DL;

    // clear rb allocations across all CC_id
    for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
	if (UE_list->active[UE_id] != TRUE)
	    continue;
	if (!ue_slice_membership(UE_id, slice_id))
	    continue;
	pCCid = UE_PCCID(Mod_id, UE_id);

	//update CQI information across component carriers
	for (n = 0; n < UE_list->numactiveCCs[UE_id]; n++) {

	    CC_id = UE_list->ordered_CCids[n][UE_id];
	    eNB_UE_stats = &UE_list->eNB_UE_stats[CC_id][UE_id];

	    eNB_UE_stats->dlsch_mcs1 =cmin(cqi_to_mcs[UE_list->UE_sched_ctrl[UE_id].dl_cqi[CC_id]],
									   slice_maxmcs[slice_id]);

	}

	// provide the list of CCs sorted according to MCS
	for (i = 0; i < UE_list->numactiveCCs[UE_id]; i++) {
	    eNB_UE_stats_i =
		&UE_list->eNB_UE_stats[UE_list->
				       ordered_CCids[i][UE_id]][UE_id];
	    for (j = i + 1; j < UE_list->numactiveCCs[UE_id]; j++) {
		DevAssert(j < MAX_NUM_CCs);
		eNB_UE_stats_j =
		    &UE_list->
		    eNB_UE_stats[UE_list->ordered_CCids[j][UE_id]][UE_id];
		if (eNB_UE_stats_j->dlsch_mcs1 >
		    eNB_UE_stats_i->dlsch_mcs1) {
		    tmp = UE_list->ordered_CCids[i][UE_id];
		    UE_list->ordered_CCids[i][UE_id] =
			UE_list->ordered_CCids[j][UE_id];
		    UE_list->ordered_CCids[j][UE_id] = tmp;
		}
	    }
	}

	if (UE_list->UE_template[pCCid][UE_id].dl_buffer_total > 0) {
	    LOG_D(MAC, "[preprocessor] assign RB for UE %d\n", UE_id);

	    for (i = 0; i < UE_list->numactiveCCs[UE_id]; i++) {
		CC_id = UE_list->ordered_CCids[i][UE_id];
		eNB_UE_stats = &UE_list->eNB_UE_stats[CC_id][UE_id];

		if (eNB_UE_stats->dlsch_mcs1 == 0) {
		    nb_rbs_required[CC_id][UE_id] = 4;	// don't let the TBS get too small
		} else {
		    nb_rbs_required[CC_id][UE_id] = min_rb_unit[CC_id];
		}

		TBS =
		    get_TBS_DL(eNB_UE_stats->dlsch_mcs1,
			       nb_rbs_required[CC_id][UE_id]);

		LOG_D(MAC,
		      "[preprocessor] start RB assignement for UE %d CC_id %d dl buffer %d (RB unit %d, MCS %d, TBS %d) \n",
		      UE_id, CC_id,
		      UE_list->UE_template[pCCid][UE_id].dl_buffer_total,
		      nb_rbs_required[CC_id][UE_id],
		      eNB_UE_stats->dlsch_mcs1, TBS);

		N_RB_DL =
		    to_prb(RC.mac[Mod_id]->common_channels[CC_id].
			   mib->message.dl_Bandwidth);

		UE_list->UE_sched_ctrl[UE_id].max_rbs_allowed_slice[CC_id][slice_id]= flexran_nb_rbs_allowed_slice(slice_percentage[slice_id],N_RB_DL);

		/* calculating required number of RBs for each UE */
		while (TBS <
		       UE_list->UE_template[pCCid][UE_id].
		       dl_buffer_total) {
		    nb_rbs_required[CC_id][UE_id] += min_rb_unit[CC_id];

		    if (nb_rbs_required[CC_id][UE_id] > UE_list->UE_sched_ctrl[UE_id].max_rbs_allowed_slice[CC_id][slice_id]) {
			TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, UE_list->UE_sched_ctrl[UE_id].max_rbs_allowed_slice[CC_id][slice_id]);
			nb_rbs_required[CC_id][UE_id] = UE_list->UE_sched_ctrl[UE_id].max_rbs_allowed_slice[CC_id][slice_id];
			break;
		    }

		    TBS =
			get_TBS_DL(eNB_UE_stats->dlsch_mcs1,
				   nb_rbs_required[CC_id][UE_id]);
		}		// end of while

		LOG_D(MAC,
		      "[eNB %d] Frame %d: UE %d on CC %d: RB unit %d,  nb_required RB %d (TBS %d, mcs %d)\n",
		      Mod_id, frameP, UE_id, CC_id, min_rb_unit[CC_id],
		      nb_rbs_required[CC_id][UE_id], TBS,
		      eNB_UE_stats->dlsch_mcs1);
	    }
	}
    }
}


// This function scans all CC_ids for a particular UE to find the maximum round index of its HARQ processes

int
maxround(module_id_t Mod_id, uint16_t rnti, int frame,
	 sub_frame_t subframe, uint8_t ul_flag)
{

    uint8_t round, round_max = 0, UE_id;
    int CC_id, harq_pid;
    UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
    COMMON_channels_t *cc;

    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

	cc = &RC.mac[Mod_id]->common_channels[CC_id];

	UE_id = find_UE_id(Mod_id, rnti);
	if (cc->tdd_Config)
	    harq_pid = ((frame * 10) + subframe) % 10;
	else
	    harq_pid = ((frame * 10) + subframe) & 7;

	round = UE_list->UE_sched_ctrl[UE_id].round[CC_id][harq_pid];
	if (round > round_max) {
	    round_max = round;
	}
    }

    return round_max;
}

// This function scans all CC_ids for a particular UE to find the maximum DL CQI
// it returns -1 if the UE is not found in PHY layer (get_eNB_UE_stats gives NULL)
int maxcqi(module_id_t Mod_id, int32_t UE_id)
{
    UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
    int CC_id, n;
    int CQI = 0;

    for (n = 0; n < UE_list->numactiveCCs[UE_id]; n++) {
	CC_id = UE_list->ordered_CCids[n][UE_id];

	if (UE_list->UE_sched_ctrl[UE_id].dl_cqi[CC_id] > CQI) {
	    CQI = UE_list->UE_sched_ctrl[UE_id].dl_cqi[CC_id];
	}
    }

    return CQI;
}

struct sort_ue_dl_params {
    int Mod_idP;
    int frameP;
    int subframeP;
    int slice_id;
};

static int ue_dl_compare(const void *_a, const void *_b, void *_params)
{
    struct sort_ue_dl_params *params = _params;
    UE_list_t *UE_list = &RC.mac[params->Mod_idP]->UE_list;

	int i;
	int slice_id = params->slice_id;
    int UE_id1 = *(const int *) _a;
    int UE_id2 = *(const int *) _b;

    int rnti1 = UE_RNTI(params->Mod_idP, UE_id1);
    int pCC_id1 = UE_PCCID(params->Mod_idP, UE_id1);
    int round1 = maxround(params->Mod_idP, rnti1, params->frameP, params->subframeP, 1);

    int rnti2 = UE_RNTI(params->Mod_idP, UE_id2);
    int pCC_id2 = UE_PCCID(params->Mod_idP, UE_id2);
    int round2 = maxround(params->Mod_idP, rnti2, params->frameP, params->subframeP, 1);

    int cqi1 = maxcqi(params->Mod_idP, UE_id1);
    int cqi2 = maxcqi(params->Mod_idP, UE_id2);

  for (i = 0; i < CR_NUM; ++i) {
    switch (UE_list->sorting_criteria[slice_id][i]) {

      case CR_ROUND :
        if (round1 > round2)
          return -1;
        if (round1 < round2)
          return 1;
        break;

      case CR_SRB12 :
        if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[1] +
            UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[2] >
            UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[1] +
            UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[2])
          return -1;
        if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[1] +
            UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[2] <
            UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[1] +
            UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[2])
          return 1;
        break;

      case CR_HOL :
        if (UE_list-> UE_template[pCC_id1][UE_id1].dl_buffer_head_sdu_creation_time_max >
            UE_list-> UE_template[pCC_id2][UE_id2].dl_buffer_head_sdu_creation_time_max)
          return -1;
        if (UE_list-> UE_template[pCC_id1][UE_id1].dl_buffer_head_sdu_creation_time_max <
            UE_list-> UE_template[pCC_id2][UE_id2].dl_buffer_head_sdu_creation_time_max)
          return 1;
        break;

      case CR_LC :
        if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_total >
            UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_total)
          return -1;
        if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_total <
            UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_total)
          return 1;
        break;

      case CR_CQI :
        if (cqi1 > cqi2)
          return -1;
        if (cqi1 < cqi2)
          return 1;

      default :
        break;
    }
  }

    return 0;
#if 0
    /* The above order derives from the following.  */
    if (round2 > round1) {	// Check first if one of the UEs has an active HARQ process which needs service and swap order
	swap_UEs(UE_list, UE_id1, UE_id2, 0);
    } else if (round2 == round1) {
	// RK->NN : I guess this is for fairness in the scheduling. This doesn't make sense unless all UEs have the same configuration of logical channels.  This should be done on the sum of all information that has to be sent.  And still it wouldn't ensure fairness.  It should be based on throughput seen by each UE or maybe using the head_sdu_creation_time, i.e. swap UEs if one is waiting longer for service.
	//  for(j=0;j<MAX_NUM_LCID;j++){
	//    if (eNB_mac_inst[Mod_id][pCC_id1].UE_template[UE_id1].dl_buffer_info[j] <
	//      eNB_mac_inst[Mod_id][pCC_id2].UE_template[UE_id2].dl_buffer_info[j]){

	// first check the buffer status for SRB1 and SRB2

	if ((UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[1] +
	     UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_info[2]) <
	    (UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[1] +
	     UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_info[2])) {
	    swap_UEs(UE_list, UE_id1, UE_id2, 0);
	} else if (UE_list->UE_template[pCC_id1]
		   [UE_id1].dl_buffer_head_sdu_creation_time_max <
		   UE_list->UE_template[pCC_id2]
		   [UE_id2].dl_buffer_head_sdu_creation_time_max) {
	    swap_UEs(UE_list, UE_id1, UE_id2, 0);
	} else if (UE_list->UE_template[pCC_id1][UE_id1].dl_buffer_total <
		   UE_list->UE_template[pCC_id2][UE_id2].dl_buffer_total) {
	    swap_UEs(UE_list, UE_id1, UE_id2, 0);
	} else if (cqi1 < cqi2) {
	    swap_UEs(UE_list, UE_id1, UE_id2, 0);
	}
    }
#endif
}

void decode_sorting_policy(module_id_t Mod_idP, slice_id_t slice_id) {
	int i;

	UE_list_t *UE_list = &RC.mac[Mod_idP]->UE_list;
	uint32_t policy = sorting_policy[slice_id];
	uint32_t mask = 0x0000000F;
    uint16_t criterion;

	for(i = 0; i < CR_NUM; ++i) {
		criterion = (uint16_t)(policy >> 4*(CR_NUM - 1 - i) & mask);
        if (criterion >= CR_NUM) {
          LOG_W(MAC, "Invalid criterion in slice %d policy, revert to default policy \n", slice_id);
          sorting_policy[slice_id] = 0x1234;
          break;
        }
      UE_list->sorting_criteria[slice_id][i] = criterion;
	}
}


// This fuction sorts the UE in order their dlsch buffer and CQI
void sort_UEs(module_id_t Mod_idP, slice_id_t slice_id, int frameP, sub_frame_t subframeP)
{
    int i;
    int list[NUMBER_OF_UE_MAX];
    int list_size = 0;
    int rnti;
    struct sort_ue_dl_params params = { Mod_idP, frameP, subframeP, slice_id };

    UE_list_t *UE_list = &RC.mac[Mod_idP]->UE_list;

	for (i = 0; i < NUMBER_OF_UE_MAX; i++) {

		if (UE_list->active[i] == FALSE)
			continue;
		if ((rnti = UE_RNTI(Mod_idP, i)) == NOT_A_RNTI)
			continue;
		if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
			continue;
		if (!ue_slice_membership(i, slice_id))
			continue;

		list[list_size] = i;
		list_size++;
	}

	decode_sorting_policy(Mod_idP, slice_id);

	qsort_r(list, list_size, sizeof(int), ue_dl_compare, &params);

	if (list_size) {
		for (i = 0; i < list_size - 1; i++)
			UE_list->next[list[i]] = list[i + 1];
		UE_list->next[list[list_size - 1]] = -1;
		UE_list->head = list[0];
	} else {
		UE_list->head = -1;
	}

#if 0


    int UE_id1, UE_id2;
    int pCC_id1, pCC_id2;
    int cqi1, cqi2, round1, round2;
    int i = 0, ii = 0;		//,j=0;
    rnti_t rnti1, rnti2;

    UE_list_t *UE_list = &RC.mac[Mod_idP]->UE_list;

    for (i = UE_list->head; i >= 0; i = UE_list->next[i]) {

	for (ii = UE_list->next[i]; ii >= 0; ii = UE_list->next[ii]) {

	    UE_id1 = i;
	    rnti1 = UE_RNTI(Mod_idP, UE_id1);
	    if (rnti1 == NOT_A_RNTI)
		continue;
	    if (UE_list->UE_sched_ctrl[UE_id1].ul_out_of_sync == 1)
		continue;
	    pCC_id1 = UE_PCCID(Mod_idP, UE_id1);
	    cqi1 = maxcqi(Mod_idP, UE_id1);	//
	    round1 = maxround(Mod_idP, rnti1, frameP, subframeP, 0);

	    UE_id2 = ii;
	    rnti2 = UE_RNTI(Mod_idP, UE_id2);
	    if (rnti2 == NOT_A_RNTI)
		continue;
	    if (UE_list->UE_sched_ctrl[UE_id2].ul_out_of_sync == 1)
		continue;
	    cqi2 = maxcqi(Mod_idP, UE_id2);
	    round2 = maxround(Mod_idP, rnti2, frameP, subframeP, 0);	//mac_xface->get_ue_active_harq_pid(Mod_id,rnti2,subframe,&harq_pid2,&round2,0);
	    pCC_id2 = UE_PCCID(Mod_idP, UE_id2);

	    if (round2 > round1) {	// Check first if one of the UEs has an active HARQ process which needs service and swap order
		swap_UEs(UE_list, UE_id1, UE_id2, 0);
	    } else if (round2 == round1) {
		// RK->NN : I guess this is for fairness in the scheduling. This doesn't make sense unless all UEs have the same configuration of logical channels.  This should be done on the sum of all information that has to be sent.  And still it wouldn't ensure fairness.  It should be based on throughput seen by each UE or maybe using the head_sdu_creation_time, i.e. swap UEs if one is waiting longer for service.
		//  for(j=0;j<MAX_NUM_LCID;j++){
		//    if (eNB_mac_inst[Mod_id][pCC_id1].UE_template[UE_id1].dl_buffer_info[j] <
		//      eNB_mac_inst[Mod_id][pCC_id2].UE_template[UE_id2].dl_buffer_info[j]){

		// first check the buffer status for SRB1 and SRB2

		if ((UE_list->UE_template[pCC_id1][UE_id1].
		     dl_buffer_info[1] +
		     UE_list->UE_template[pCC_id1][UE_id1].
		     dl_buffer_info[2]) <
		    (UE_list->UE_template[pCC_id2][UE_id2].
		     dl_buffer_info[1] +
		     UE_list->UE_template[pCC_id2][UE_id2].
		     dl_buffer_info[2])) {
		    swap_UEs(UE_list, UE_id1, UE_id2, 0);
		} else if (UE_list->UE_template[pCC_id1]
			   [UE_id1].dl_buffer_head_sdu_creation_time_max <
			   UE_list->UE_template[pCC_id2]
			   [UE_id2].dl_buffer_head_sdu_creation_time_max) {
		    swap_UEs(UE_list, UE_id1, UE_id2, 0);
		} else if (UE_list->UE_template[pCC_id1][UE_id1].
			   dl_buffer_total <
			   UE_list->UE_template[pCC_id2][UE_id2].
			   dl_buffer_total) {
		    swap_UEs(UE_list, UE_id1, UE_id2, 0);
		} else if (cqi1 < cqi2) {
		    swap_UEs(UE_list, UE_id1, UE_id2, 0);
		}
	    }
	}
    }
#endif
}

void dlsch_scheduler_pre_processor_accounting(module_id_t Mod_id,
                                              slice_id_t slice_id,
                                              frame_t frameP,
                                              sub_frame_t subframeP,
                                              int N_RBG[MAX_NUM_CCs],
                                              int min_rb_unit[MAX_NUM_CCs],
                                              uint8_t rballoc_sub[MAX_NUM_CCs][N_RBG_MAX],
                                              uint8_t MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX],
                                              uint16_t nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX]) {


  int UE_id, CC_id;
  int ii, r1;

  rnti_t rnti;
  uint8_t harq_pid, round, transmission_mode;
  uint8_t total_rbs_used[MAX_NUM_CCs];
  uint8_t total_ue_count[MAX_NUM_CCs];
  uint16_t average_rbs_per_user[MAX_NUM_CCs];
  uint16_t nb_rbs_required_remaining[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
  uint16_t nb_rbs_required_remaining_1[MAX_NUM_CCs][NUMBER_OF_UE_MAX];

  int N_RB_DL;
  UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
  UE_sched_ctrl *ue_sched_ctl;
  COMMON_channels_t *cc;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    total_ue_count[CC_id] = 0;
    total_rbs_used[CC_id] = 0;
    average_rbs_per_user[CC_id] = 0;
    for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; ++UE_id) {
      nb_rbs_required_remaining[CC_id][UE_id] = 0;
    }
  }

  // loop over all active UEs
  for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    rnti = UE_RNTI(Mod_id, UE_id);

    if (rnti == NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
      continue;
    if (!ue_slice_membership(UE_id, slice_id))
      continue;

    for (ii = 0; ii < UE_num_active_CC(UE_list, UE_id); ii++) {
      CC_id = UE_list->ordered_CCids[ii][UE_id];
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
      cc = &RC.mac[Mod_id]->common_channels[CC_id];
      // TODO Can we use subframe2harqpid() here?
      if (cc->tdd_Config)
        harq_pid = ((frameP * 10) + subframeP) % 10;
      else
        harq_pid = ((frameP * 10) + subframeP) & 7;
      round = ue_sched_ctl->round[CC_id][harq_pid];

      average_rbs_per_user[CC_id] = 0;

      if (round != 8) {
        nb_rbs_required[CC_id][UE_id] = UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid];
        total_rbs_used[CC_id] += nb_rbs_required[CC_id][UE_id];
      }

      //nb_rbs_required_remaining[UE_id] = nb_rbs_required[UE_id];
      if (nb_rbs_required[CC_id][UE_id] > 0) {
        total_ue_count[CC_id] = total_ue_count[CC_id] + 1;
      }
    }
  }

  // loop over all active UEs and calculate avg rb per user based on total active UEs
  for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    rnti = UE_RNTI(Mod_id, UE_id);

    if (rnti == NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
      continue;
    if (!ue_slice_membership(UE_id, slice_id))
      continue;

    for (ii = 0; ii < UE_num_active_CC(UE_list, UE_id); ii++) {
      CC_id = UE_list->ordered_CCids[ii][UE_id];

      // hypothetical assignment
      /*
       * If schedule is enabled and if the priority of the UEs is modified
       * The average rbs per logical channel per user will depend on the level of
       * priority. Concerning the hypothetical assignement, we should assign more
       * rbs to prioritized users. Maybe, we can do a mapping between the
       * average rbs per user and the level of priority or multiply the average rbs
       * per user by a coefficient which represents the degree of priority.
       */

      N_RB_DL = to_prb(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth) - total_rbs_used[CC_id];

      // recalculate based on the what is left after retransmission
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
      ue_sched_ctl->max_rbs_allowed_slice[CC_id][slice_id] =
              flexran_nb_rbs_allowed_slice(slice_percentage[slice_id], N_RB_DL);

      if (total_ue_count[CC_id] == 0) {
        average_rbs_per_user[CC_id] = 0;
      } else if ((min_rb_unit[CC_id] * total_ue_count[CC_id]) <=
                 (ue_sched_ctl->max_rbs_allowed_slice[CC_id][slice_id])) {
        average_rbs_per_user[CC_id] =
                (uint16_t) floor(ue_sched_ctl->max_rbs_allowed_slice[CC_id][slice_id] / total_ue_count[CC_id]);
      } else {
        // consider the total number of use that can be scheduled UE
        average_rbs_per_user[CC_id] = min_rb_unit[CC_id];
      }
    }
  }

  // note: nb_rbs_required is assigned according to total_buffer_dl
  // extend nb_rbs_required to capture per LCID RB required
  for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    rnti = UE_RNTI(Mod_id, UE_id);

    if (rnti == NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
      continue;
    if (!ue_slice_membership(UE_id, slice_id))
      continue;

    for (ii = 0; ii < UE_num_active_CC(UE_list, UE_id); ii++) {
      CC_id = UE_list->ordered_CCids[ii][UE_id];
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
      cc = &RC.mac[Mod_id]->common_channels[CC_id];
      // TODO Can we use subframe2harqpid() here?
      if (cc->tdd_Config)
        harq_pid = ((frameP * 10) + subframeP) % 10;
      else
        harq_pid = ((frameP * 10) + subframeP) & 7;
      round = ue_sched_ctl->round[CC_id][harq_pid];

      // control channel or retransmission
      /* TODO: do we have to check for retransmission? */
      if (mac_eNB_get_rrc_status(Mod_id, rnti) < RRC_RECONFIGURED || round != 8) {
        nb_rbs_required_remaining_1[CC_id][UE_id] =
                nb_rbs_required[CC_id][UE_id];
      } else {
        nb_rbs_required_remaining_1[CC_id][UE_id] =
                cmin(average_rbs_per_user[CC_id], nb_rbs_required[CC_id][UE_id]);
      }
    }
  }

  //Allocation to UEs is done in 2 rounds,
  // 1st stage: average number of RBs allocated to each UE
  // 2nd stage: remaining RBs are allocated to high priority UEs
  for (r1 = 0; r1 < 2; r1++) {

    for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
      for (ii = 0; ii < UE_num_active_CC(UE_list, UE_id); ii++) {
        CC_id = UE_list->ordered_CCids[ii][UE_id];

        if (r1 == 0) {
          nb_rbs_required_remaining[CC_id][UE_id] =
                  nb_rbs_required_remaining_1[CC_id][UE_id];
        } else {    // rb required based only on the buffer - rb allocated in the 1st round + extra reaming rb form the 1st round
          nb_rbs_required_remaining[CC_id][UE_id] =
                  nb_rbs_required[CC_id][UE_id] -
                  nb_rbs_required_remaining_1[CC_id][UE_id] +
                  nb_rbs_required_remaining[CC_id][UE_id];
          if (nb_rbs_required_remaining[CC_id][UE_id] < 0)
            abort();
        }

        if (nb_rbs_required[CC_id][UE_id] > 0)
          LOG_D(MAC,
                "round %d : nb_rbs_required_remaining[%d][%d]= %d (remaining_1 %d, required %d,  pre_nb_available_rbs %d, N_RBG %d, rb_unit %d)\n",
                r1, CC_id, UE_id,
                nb_rbs_required_remaining[CC_id][UE_id],
                nb_rbs_required_remaining_1[CC_id][UE_id],
                nb_rbs_required[CC_id][UE_id],
                UE_list->UE_sched_ctrl[UE_id].pre_nb_available_rbs[CC_id],
                N_RBG[CC_id],
                min_rb_unit[CC_id]);

      }
    }

    for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {

      for (ii = 0; ii < UE_num_active_CC(UE_list, UE_id); ii++) {

        CC_id = UE_list->ordered_CCids[ii][UE_id];
        // if there are UEs with traffic
        if (total_ue_count[CC_id] > 0) {
          // ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
          // round = ue_sched_ctl->round[CC_id][harq_pid];

          rnti = UE_RNTI(Mod_id, UE_id);

          // LOG_D(MAC,"UE %d rnti 0x\n", UE_id, rnti );
          if (rnti == NOT_A_RNTI)
            continue;
          if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
            continue;
          if (!ue_slice_membership(UE_id, slice_id))
            continue;

          transmission_mode = get_tmode(Mod_id, CC_id, UE_id);
          // mac_xface->get_ue_active_harq_pid(Mod_id,CC_id,rnti,frameP,subframeP,&harq_pid,&round,0);
          // rrc_status = mac_eNB_get_rrc_status(Mod_id,rnti);
          /* 1st allocate for the retx */

          // retransmission in data channels
          // control channel in the 1st transmission
          // data channel for all TM
          LOG_T(MAC,
                "calling dlsch_scheduler_pre_processor_allocate .. \n ");
          dlsch_scheduler_pre_processor_allocate(Mod_id, UE_id,
                                                 CC_id,
                                                 N_RBG[CC_id],
                                                 transmission_mode,
                                                 min_rb_unit
                                                 [CC_id],
                                                 to_prb(RC.mac
                                                        [Mod_id]->common_channels
                                                        [CC_id].mib->message.dl_Bandwidth),
                                                 nb_rbs_required,
                                                 nb_rbs_required_remaining,
                                                 rballoc_sub,
                                                 MIMO_mode_indicator);

#ifdef TM5
          // data chanel TM5: to be revisited
          if ((round == 0) &&
          (transmission_mode == 5) &&
          (ue_sched_ctl->dl_pow_off[CC_id] != 1)) {

          for (j = 0; j < N_RBG[CC_id]; j += 2) {

              if ((((j == (N_RBG[CC_id] - 1))
                && (rballoc_sub[CC_id][j] == 0)
                && (ue_sched_ctl->
                    rballoc_sub_UE[CC_id][j] == 0))
               || ((j < (N_RBG[CC_id] - 1))
                   && (rballoc_sub[CC_id][j + 1] == 0)
                   &&
                   (ue_sched_ctl->rballoc_sub_UE
                    [CC_id][j + 1] == 0)))
              && (nb_rbs_required_remaining[CC_id][UE_id]
                  > 0)) {

              for (ii = UE_list->next[UE_id + 1]; ii >= 0;
                   ii = UE_list->next[ii]) {

                  UE_id2 = ii;
                  rnti2 = UE_RNTI(Mod_id, UE_id2);
                  ue_sched_ctl2 =
                  &UE_list->UE_sched_ctrl[UE_id2];
                  round2 = ue_sched_ctl2->round[CC_id];
                  if (rnti2 == NOT_A_RNTI)
                  continue;
                  if (UE_list->
                  UE_sched_ctrl
                  [UE_id2].ul_out_of_sync == 1)
                  continue;

                  eNB_UE_stats2 =
                  UE_list->
                  eNB_UE_stats[CC_id][UE_id2];
                  //mac_xface->get_ue_active_harq_pid(Mod_id,CC_id,rnti2,frameP,subframeP,&harq_pid2,&round2,0);

                  if ((mac_eNB_get_rrc_status
                   (Mod_id,
                    rnti2) >= RRC_RECONFIGURED)
                  && (round2 == 0)
                  &&
                  (get_tmode(Mod_id, CC_id, UE_id2)
                   == 5)
                  && (ue_sched_ctl->
                      dl_pow_off[CC_id] != 1)) {

                  if ((((j == (N_RBG[CC_id] - 1))
                        &&
                        (ue_sched_ctl->rballoc_sub_UE
                         [CC_id][j] == 0))
                       || ((j < (N_RBG[CC_id] - 1))
                       &&
                       (ue_sched_ctl->
                        rballoc_sub_UE[CC_id][j +
                                  1]
                        == 0)))
                      &&
                      (nb_rbs_required_remaining
                       [CC_id]
                       [UE_id2] > 0)) {

                      if ((((eNB_UE_stats2->
                         DL_pmi_single ^
                         eNB_UE_stats1->
                         DL_pmi_single)
                        << (14 - j)) & 0xc000) == 0x4000) {	//MU-MIMO only for 25 RBs configuration

                      rballoc_sub[CC_id][j] = 1;
                      ue_sched_ctl->
                          rballoc_sub_UE[CC_id]
                          [j] = 1;
                      ue_sched_ctl2->
                          rballoc_sub_UE[CC_id]
                          [j] = 1;
                      MIMO_mode_indicator[CC_id]
                          [j] = 0;

                      if (j < N_RBG[CC_id] - 1) {
                          rballoc_sub[CC_id][j +
                                     1] =
                          1;
                          ue_sched_ctl->
                          rballoc_sub_UE
                          [CC_id][j + 1] = 1;
                          ue_sched_ctl2->rballoc_sub_UE
                          [CC_id][j + 1] = 1;
                          MIMO_mode_indicator
                          [CC_id][j + 1]
                          = 0;
                      }

                      ue_sched_ctl->
                          dl_pow_off[CC_id]
                          = 0;
                      ue_sched_ctl2->
                          dl_pow_off[CC_id]
                          = 0;


                      if ((j == N_RBG[CC_id] - 1)
                          && ((N_RB_DL == 25)
                          || (N_RB_DL ==
                              50))) {

                          nb_rbs_required_remaining
                          [CC_id][UE_id] =
                          nb_rbs_required_remaining
                          [CC_id][UE_id] -
                          min_rb_unit[CC_id]
                          + 1;
                          ue_sched_ctl->pre_nb_available_rbs
                          [CC_id] =
                          ue_sched_ctl->pre_nb_available_rbs
                          [CC_id] +
                          min_rb_unit[CC_id]
                          - 1;
                          nb_rbs_required_remaining
                          [CC_id][UE_id2] =
                          nb_rbs_required_remaining
                          [CC_id][UE_id2] -
                          min_rb_unit[CC_id]
                          + 1;
                          ue_sched_ctl2->pre_nb_available_rbs
                          [CC_id] =
                          ue_sched_ctl2->pre_nb_available_rbs
                          [CC_id] +
                          min_rb_unit[CC_id]
                          - 1;
                      } else {

                          nb_rbs_required_remaining
                          [CC_id][UE_id] =
                          nb_rbs_required_remaining
                          [CC_id][UE_id] - 4;
                          ue_sched_ctl->pre_nb_available_rbs
                          [CC_id] =
                          ue_sched_ctl->pre_nb_available_rbs
                          [CC_id] + 4;
                          nb_rbs_required_remaining
                          [CC_id][UE_id2] =
                          nb_rbs_required_remaining
                          [CC_id][UE_id2] -
                          4;
                          ue_sched_ctl2->pre_nb_available_rbs
                          [CC_id] =
                          ue_sched_ctl2->pre_nb_available_rbs
                          [CC_id] + 4;
                      }

                      break;
                      }
                  }
                  }
              }
              }
          }
          }
#endif
        } // total_ue_count
      } // CC
    } // UE
  } // end of for for r1 and r2
}

// This function assigns pre-available RBS to each UE in specified sub-bands before scheduling is done
void
dlsch_scheduler_pre_processor(module_id_t Mod_id,
                              slice_id_t slice_id,
                              frame_t frameP,
                              sub_frame_t subframeP,
                              int N_RBG[MAX_NUM_CCs],
                              int *mbsfn_flag) {

  int UE_id;
  uint8_t CC_id;
  uint16_t i, j;

  uint8_t rballoc_sub[MAX_NUM_CCs][N_RBG_MAX];
  uint8_t MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX]; // If TM5 is revisited, we can move this inside accounting

  int min_rb_unit[MAX_NUM_CCs];
  uint16_t nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX];

  UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
  UE_sched_ctrl *ue_sched_ctl;
//  int rrc_status = RRC_IDLE;

#ifdef TM5
  int harq_pid1 = 0;
  int round1 = 0, round2 = 0;
  int UE_id2;
  uint16_t i1, i2, i3;
  rnti_t rnti1, rnti2;
  LTE_eNB_UE_stats *eNB_UE_stats1 = NULL;
  LTE_eNB_UE_stats *eNB_UE_stats2 = NULL;
  UE_sched_ctrl *ue_sched_ctl1, *ue_sched_ctl2;
#endif

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

    if (mbsfn_flag[CC_id] > 0)    // If this CC is allocated for MBSFN skip it here
      continue;

    min_rb_unit[CC_id] = get_min_rb_unit(Mod_id, CC_id);

    for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; ++UE_id) {
      if (UE_list->active[UE_id] != TRUE)
        continue;

      if (!ue_slice_membership(UE_id, slice_id))
        continue;

      // Initialize scheduling information for all active UEs
      dlsch_scheduler_pre_processor_reset(Mod_id,
                                          UE_id,
                                          CC_id,
                                          frameP,
                                          subframeP,
                                          N_RBG[CC_id],
                                          nb_rbs_required,
                                          rballoc_sub,
                                          MIMO_mode_indicator);

    }
  }

  // Store the DLSCH buffer for each logical channel
  store_dlsch_buffer(Mod_id, slice_id, frameP, subframeP);

  // Calculate the number of RBs required by each UE on the basis of logical channel's buffer
  assign_rbs_required(Mod_id, slice_id, frameP, subframeP, nb_rbs_required, min_rb_unit);

  // Sorts the user on the basis of dlsch logical channel buffer and CQI
  sort_UEs(Mod_id, slice_id, frameP, subframeP);

  // This function does the main allocation of the number of RBs
  dlsch_scheduler_pre_processor_accounting(Mod_id,
                                           slice_id,
                                           frameP,
                                           subframeP,
                                           N_RBG,
                                           min_rb_unit,
                                           rballoc_sub,
                                           MIMO_mode_indicator,
                                           nb_rbs_required);


#ifdef TM5
  // This has to be revisited!!!!
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
  i1 = 0;
  i2 = 0;
  i3 = 0;

  for (j = 0; j < N_RBG[CC_id]; j++) {
      if (MIMO_mode_indicator[CC_id][j] == 2) {
      i1 = i1 + 1;
      } else if (MIMO_mode_indicator[CC_id][j] == 1) {
      i2 = i2 + 1;
      } else if (MIMO_mode_indicator[CC_id][j] == 0) {
      i3 = i3 + 1;
      }
  }

  if ((i1 < N_RBG[CC_id]) && (i2 > 0) && (i3 == 0)) {
      PHY_vars_eNB_g[Mod_id][CC_id]->check_for_SUMIMO_transmissions =
      PHY_vars_eNB_g[Mod_id][CC_id]->
      check_for_SUMIMO_transmissions + 1;
  }

  if (i3 == N_RBG[CC_id] && i1 == 0 && i2 == 0) {
      PHY_vars_eNB_g[Mod_id][CC_id]->FULL_MUMIMO_transmissions =
      PHY_vars_eNB_g[Mod_id][CC_id]->FULL_MUMIMO_transmissions +
      1;
  }

  if ((i1 < N_RBG[CC_id]) && (i3 > 0)) {
      PHY_vars_eNB_g[Mod_id][CC_id]->check_for_MUMIMO_transmissions =
      PHY_vars_eNB_g[Mod_id][CC_id]->
      check_for_MUMIMO_transmissions + 1;
  }

  PHY_vars_eNB_g[Mod_id][CC_id]->check_for_total_transmissions =
      PHY_vars_eNB_g[Mod_id][CC_id]->check_for_total_transmissions +
      1;

  }
#endif

  for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {

    ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
    for (i = 0; i < UE_num_active_CC(UE_list, UE_id); i++) {
      CC_id = UE_list->ordered_CCids[i][UE_id];
      //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].dl_pow_off = dl_pow_off[UE_id];

      if (ue_sched_ctl->pre_nb_available_rbs[CC_id] > 0) {
        LOG_D(MAC, "******************DL Scheduling Information for UE%d ************************\n", UE_id);
        LOG_D(MAC, "dl power offset UE%d = %d \n", UE_id, ue_sched_ctl->dl_pow_off[CC_id]);
        LOG_D(MAC, "***********RB Alloc for every subband for UE%d ***********\n", UE_id);

        for (j = 0; j < N_RBG[CC_id]; j++) {
          //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].rballoc_sub[UE_id] = rballoc_sub_UE[CC_id][UE_id][UE_id];
          LOG_D(MAC, "RB Alloc for UE%d and Subband%d = %d\n", UE_id, j, ue_sched_ctl->rballoc_sub_UE[CC_id][j]);
        }

        //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].pre_nb_available_rbs = pre_nb_available_rbs[CC_id][UE_id];
        LOG_D(MAC, "[eNB %d][SLICE %d]Total RBs allocated for UE%d = %d\n",
              Mod_id, slice_id, UE_id, ue_sched_ctl->pre_nb_available_rbs[CC_id]);
      }
    }
  }
}

#define SF0_LIMIT 1

void
dlsch_scheduler_pre_processor_reset(int module_idP,
                                    int UE_id,
                                    uint8_t CC_id,
                                    int frameP,
                                    int subframeP,
                                    int N_RBG,
                                    uint16_t nb_rbs_required[MAX_NUM_CCs]
                                    [NUMBER_OF_UE_MAX],
                                    unsigned char
                                    rballoc_sub[MAX_NUM_CCs]
                                    [N_RBG_MAX],
                                    unsigned char
                                    MIMO_mode_indicator[MAX_NUM_CCs]
                                    [N_RBG_MAX])
{
  int i, j;
  UE_list_t *UE_list = &RC.mac[module_idP]->UE_list;
  UE_sched_ctrl *ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
  rnti_t rnti = UE_RNTI(module_idP, UE_id);

  uint8_t *vrb_map = RC.mac[module_idP]->common_channels[CC_id].vrb_map;
  int N_RB_DL =
          to_prb(RC.mac[module_idP]->common_channels[CC_id].mib->message.dl_Bandwidth);
  int RBGsize = N_RB_DL / N_RBG, RBGsize_last;
#ifdef SF0_LIMIT
  int sf0_upper = -1, sf0_lower = -1;
#endif


  LOG_D(MAC, "Running preprocessor for UE %d (%x)\n", UE_id, rnti);
  // initialize harq_pid and round

  if (ue_sched_ctl->ta_timer)
    ue_sched_ctl->ta_timer--;

    /*
       eNB_UE_stats *eNB_UE_stats;

       if (eNB_UE_stats == NULL)
       return;


       mac_xface->get_ue_active_harq_pid(module_idP,CC_id,rnti,
       frameP,subframeP,
       &ue_sched_ctl->harq_pid[CC_id],
       &ue_sched_ctl->round[CC_id],
       openair_harq_DL);


       if (ue_sched_ctl->ta_timer == 0) {

       // WE SHOULD PROTECT the eNB_UE_stats with a mutex here ...

       ue_sched_ctl->ta_timer = 20;  // wait 20 subframes before taking TA measurement from PHY
       switch (N_RB_DL) {
       case 6:
       ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update;
       break;

       case 15:
       ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/2;
       break;

       case 25:
       ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/4;
       break;

       case 50:
       ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/8;
       break;

       case 75:
       ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/12;
       break;

       case 100:
       ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/16;
       break;
       }
       // clear the update in case PHY does not have a new measurement after timer expiry
       eNB_UE_stats->timing_advance_update =  0;
       }
       else {
       ue_sched_ctl->ta_timer--;
       ue_sched_ctl->ta_update =0; // don't trigger a timing advance command
       }


       if (UE_id==0) {
       VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_TIMING_ADVANCE,ue_sched_ctl->ta_update);
       }
     */

  nb_rbs_required[CC_id][UE_id] = 0;
  ue_sched_ctl->pre_nb_available_rbs[CC_id] = 0;
  ue_sched_ctl->dl_pow_off[CC_id] = 2;

  switch (N_RB_DL) {
    case 6:
      RBGsize = 1;
      RBGsize_last = 1;
      break;
    case 15:
      RBGsize = 2;
      RBGsize_last = 1;
      break;
    case 25:
      RBGsize = 2;
      RBGsize_last = 1;
      break;
    case 50:
      RBGsize = 3;
      RBGsize_last = 2;
      break;
    case 75:
      RBGsize = 4;
      RBGsize_last = 3;
      break;
    case 100:
      RBGsize = 4;
      RBGsize_last = 4;
      break;
    default:
      AssertFatal(1 == 0, "unsupported RBs (%d)\n", N_RB_DL);
  }

#ifdef SF0_LIMIT
  switch (N_RBG) {
    case 6:
      sf0_lower = 0;
      sf0_upper = 5;
      break;
    case 8:
      sf0_lower = 2;
      sf0_upper = 5;
      break;
    case 13:
      sf0_lower = 4;
      sf0_upper = 7;
      break;
    case 17:
      sf0_lower = 7;
      sf0_upper = 9;
      break;
    case 25:
      sf0_lower = 11;
      sf0_upper = 13;
      break;
    default:
      AssertFatal(1 == 0, "unsupported RBs (%d)\n", N_RB_DL);
  }
#endif

  // Initialize Subbands according to VRB map
  for (i = 0; i < N_RBG; i++) {
    int rb_size = i == N_RBG - 1 ? RBGsize_last : RBGsize;

    ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 0;
    rballoc_sub[CC_id][i] = 0;

#ifdef SF0_LIMIT
    // for avoiding 6+ PRBs around DC in subframe 0 (avoid excessive errors)
    /* TODO: make it proper - allocate those RBs, do not "protect" them, but
     * compute number of available REs and limit MCS according to the
     * TBS table 36.213 7.1.7.2.1-1 (can be done after pre-processor)
     */
    if (subframeP == 0 && i >= sf0_lower && i <= sf0_upper)
      rballoc_sub[CC_id][i] = 1;
#endif

    // for SI-RNTI,RA-RNTI and P-RNTI allocations
    for (j = 0; j < rb_size; j++) {
      if (vrb_map[j + (i * RBGsize)] != 0) {
        rballoc_sub[CC_id][i] = 1;
        LOG_D(MAC, "Frame %d, subframe %d : vrb %d allocated\n",
              frameP, subframeP, j + (i * RBGsize));
        break;
      }
    }
    LOG_D(MAC, "Frame %d Subframe %d CC_id %d RBG %i : rb_alloc %d\n",
          frameP, subframeP, CC_id, i, rballoc_sub[CC_id][i]);
    MIMO_mode_indicator[CC_id][i] = 2;
  }
}


void
dlsch_scheduler_pre_processor_allocate(module_id_t Mod_id,
				       int UE_id,
				       uint8_t CC_id,
				       int N_RBG,
				       int transmission_mode,
				       int min_rb_unit,
				       uint8_t N_RB_DL,
				       uint16_t
				       nb_rbs_required[MAX_NUM_CCs]
				       [NUMBER_OF_UE_MAX],
				       uint16_t
				       nb_rbs_required_remaining
				       [MAX_NUM_CCs]
				       [NUMBER_OF_UE_MAX], unsigned char
				       rballoc_sub[MAX_NUM_CCs]
				       [N_RBG_MAX], unsigned char
				       MIMO_mode_indicator
				       [MAX_NUM_CCs][N_RBG_MAX])
{

    int i;
    UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
    UE_sched_ctrl *ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

    for (i = 0; i < N_RBG; i++) {

	if ((rballoc_sub[CC_id][i] == 0) &&
	    (ue_sched_ctl->rballoc_sub_UE[CC_id][i] == 0) &&
	    (nb_rbs_required_remaining[CC_id][UE_id] > 0) &&
	    (ue_sched_ctl->pre_nb_available_rbs[CC_id] <
	     nb_rbs_required[CC_id][UE_id])) {

	    // if this UE is not scheduled for TM5
	    if (ue_sched_ctl->dl_pow_off[CC_id] != 0) {

		if ((i == N_RBG - 1)
		    && ((N_RB_DL == 25) || (N_RB_DL == 50))) {
		    if (nb_rbs_required_remaining[CC_id][UE_id] >=
			min_rb_unit - 1) {
			rballoc_sub[CC_id][i] = 1;
			ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 1;
			MIMO_mode_indicator[CC_id][i] = 1;
			if (transmission_mode == 5) {
			    ue_sched_ctl->dl_pow_off[CC_id] = 1;
			}
			nb_rbs_required_remaining[CC_id][UE_id] =
			    nb_rbs_required_remaining[CC_id][UE_id] -
			    min_rb_unit + 1;
			ue_sched_ctl->pre_nb_available_rbs[CC_id] =
			    ue_sched_ctl->pre_nb_available_rbs[CC_id] +
			    min_rb_unit - 1;
		    }
		} else {
		    if (nb_rbs_required_remaining[CC_id][UE_id] >=
			min_rb_unit) {
			rballoc_sub[CC_id][i] = 1;
			ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 1;
			MIMO_mode_indicator[CC_id][i] = 1;
			if (transmission_mode == 5) {
			    ue_sched_ctl->dl_pow_off[CC_id] = 1;
			}
			nb_rbs_required_remaining[CC_id][UE_id] =
			    nb_rbs_required_remaining[CC_id][UE_id] -
			    min_rb_unit;
			ue_sched_ctl->pre_nb_available_rbs[CC_id] =
			    ue_sched_ctl->pre_nb_available_rbs[CC_id] +
			    min_rb_unit;
		    }
		}
	    }			// dl_pow_off[CC_id][UE_id] ! = 0
	}
    }
}


/// ULSCH PRE_PROCESSOR


void
ulsch_scheduler_pre_processor(module_id_t module_idP,
			      slice_id_t slice_id, int frameP,
			      sub_frame_t subframeP, uint16_t * first_rb)
{

    int16_t i;
    uint16_t UE_id, n, r;
    uint8_t CC_id, harq_pid;
    uint16_t nb_allocated_rbs[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
	total_allocated_rbs[MAX_NUM_CCs],
	average_rbs_per_user[MAX_NUM_CCs];
    int16_t total_remaining_rbs[MAX_NUM_CCs];
    uint16_t total_ue_count[MAX_NUM_CCs];
    rnti_t rnti = -1;
    UE_list_t *UE_list = &RC.mac[module_idP]->UE_list;
    UE_TEMPLATE *UE_template = 0;
    UE_sched_ctrl *ue_sched_ctl;
    int N_RB_UL = 0;

    LOG_D(MAC, "In ulsch_preprocessor: assign max mcs min rb\n");
    // maximize MCS and then allocate required RB according to the buffer occupancy with the limit of max available UL RB
    assign_max_mcs_min_rb(module_idP, slice_id, frameP, subframeP, first_rb);

    LOG_D(MAC, "In ulsch_preprocessor: sort ue \n");
    // sort ues
    sort_ue_ul(module_idP, frameP, subframeP);


    // we need to distribute RBs among UEs
    // step1:  reset the vars
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
      total_allocated_rbs[CC_id] = 0;
      total_remaining_rbs[CC_id] = 0;
      average_rbs_per_user[CC_id] = 0;
      total_ue_count[CC_id] = 0;
    }

    // Step 1.5: Calculate total_ue_count
    for (i = UE_list->head_ul; i >= 0; i = UE_list->next_ul[i]) {
      for (n = 0; n < UE_list->numactiveULCCs[i]; n++) {
        // This is the actual CC_id in the list
        CC_id = UE_list->ordered_ULCCids[n][i];
        UE_template = &UE_list->UE_template[CC_id][i];
        if (!ue_slice_membership(i, slice_id))
          continue;
        if (UE_template->pre_allocated_nb_rb_ul[slice_id] > 0) {
          total_ue_count[CC_id] += 1;
        }
      }
    }

    LOG_D(MAC, "In ulsch_preprocessor: step2 \n");
    // step 2: calculate the average rb per UE
    for (i = UE_list->head_ul; i >= 0; i = UE_list->next_ul[i]) {

      rnti = UE_RNTI(module_idP, i);
      UE_id = i;

      if (rnti == NOT_A_RNTI)
        continue;

      if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
        continue;

      if (!ue_slice_membership(UE_id, slice_id))
          continue;

      LOG_D(MAC, "In ulsch_preprocessor: handling UE %d/%x\n", UE_id,
            rnti);
      for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
        // This is the actual CC_id in the list
        CC_id = UE_list->ordered_ULCCids[n][UE_id];
        LOG_D(MAC,
              "In ulsch_preprocessor: handling UE %d/%x CCid %d\n",
              UE_id, rnti, CC_id);

        average_rbs_per_user[CC_id] = 0;

        /*
           if((mac_xface->get_nCCE_max(module_idP,CC_id,3,subframeP) - nCCE_to_be_used[CC_id])  > (1<<aggregation)) {
           nCCE_to_be_used[CC_id] = nCCE_to_be_used[CC_id] + (1<<aggregation);
           max_num_ue_to_be_scheduled+=1;
           } */



        N_RB_UL = to_prb(RC.mac[module_idP]->common_channels[CC_id].ul_Bandwidth);
        ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
        ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_id] = flexran_nb_rbs_allowed_slice(slice_percentage_uplink[slice_id],N_RB_UL);

        if (total_ue_count[CC_id] == 0) {
          average_rbs_per_user[CC_id] = 0;
        } else if (total_ue_count[CC_id] == 1) {    // increase the available RBs, special case,
          average_rbs_per_user[CC_id] = ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_id] - first_rb[CC_id] + 1;
        } else if (total_ue_count[CC_id] <= (ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_id] - first_rb[CC_id])) {
          average_rbs_per_user[CC_id] = (uint16_t) floor((ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_id] - first_rb[CC_id]) / total_ue_count[CC_id]);
        } else {
          average_rbs_per_user[CC_id] = 1;
          LOG_W(MAC,
                "[eNB %d] frame %d subframe %d: UE %d CC %d: can't get average rb per user (should not be here)\n",
                module_idP, frameP, subframeP, UE_id, CC_id);
        }
        if (total_ue_count[CC_id] > 0)
          LOG_D(MAC, "[eNB %d] Frame %d subframe %d: total ue to be scheduled %d\n",
                module_idP, frameP, subframeP, total_ue_count[CC_id]);
      }
    }

    // step 3: assigne RBS
    for (i = UE_list->head_ul; i >= 0; i = UE_list->next_ul[i]) {
      rnti = UE_RNTI(module_idP, i);

      if (rnti == NOT_A_RNTI)
        continue;
      if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
        continue;
      if (!ue_slice_membership(i, slice_id))
        continue;


      UE_id = i;

      for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
        // This is the actual CC_id in the list
        CC_id = UE_list->ordered_ULCCids[n][UE_id];
        UE_template = &UE_list->UE_template[CC_id][UE_id];
        harq_pid = subframe2harqpid(&RC.mac[module_idP]->common_channels[CC_id],
                                    frameP, subframeP);

        //      mac_xface->get_ue_active_harq_pid(module_idP,CC_id,rnti,frameP,subframeP,&harq_pid,&round,openair_harq_UL);

        if (UE_list->UE_sched_ctrl[UE_id].round_UL[CC_id] > 0) {
          nb_allocated_rbs[CC_id][UE_id] = UE_list->UE_template[CC_id][UE_id].nb_rb_ul[harq_pid];
        } else {
          nb_allocated_rbs[CC_id][UE_id] =
                  cmin(UE_list->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul[slice_id],
                       average_rbs_per_user[CC_id]);
        }

        total_allocated_rbs[CC_id] += nb_allocated_rbs[CC_id][UE_id];
        LOG_D(MAC,
              "In ulsch_preprocessor: assigning %d RBs for UE %d/%x CCid %d, harq_pid %d\n",
              nb_allocated_rbs[CC_id][UE_id], UE_id, rnti, CC_id,
              harq_pid);
      }
    }

    // step 4: assigne the remaining RBs and set the pre_allocated rbs accordingly
  for (r = 0; r < 2; r++) {

    for (i = UE_list->head_ul; i >= 0; i = UE_list->next_ul[i]) {
      rnti = UE_RNTI(module_idP, i);

      if (rnti == NOT_A_RNTI)
        continue;
      if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
        continue;
      if (!ue_slice_membership(i, slice_id))
        continue;

      UE_id = i;
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

      for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
        // This is the actual CC_id in the list
        CC_id = UE_list->ordered_ULCCids[n][UE_id];
        UE_template = &UE_list->UE_template[CC_id][UE_id];
        total_remaining_rbs[CC_id] =
                ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_id] - first_rb[CC_id] - total_allocated_rbs[CC_id];

        if (total_ue_count[CC_id] == 1) {
          total_remaining_rbs[CC_id] += 1;
        }

        if (r == 0) {
          while ((UE_template->pre_allocated_nb_rb_ul[slice_id] > 0)
                 && (nb_allocated_rbs[CC_id][UE_id] < UE_template->pre_allocated_nb_rb_ul[slice_id])
                 && (total_remaining_rbs[CC_id] > 0)) {
            nb_allocated_rbs[CC_id][UE_id] =
                    cmin(nb_allocated_rbs[CC_id][UE_id] + 1,
                         UE_template->pre_allocated_nb_rb_ul[slice_id]);
            total_remaining_rbs[CC_id]--;
            total_allocated_rbs[CC_id]++;
          }
        } else {
          UE_template->pre_allocated_nb_rb_ul[slice_id] =
                  nb_allocated_rbs[CC_id][UE_id];
          LOG_D(MAC,
                "******************UL Scheduling Information for UE%d CC_id %d ************************\n",
                UE_id, CC_id);
          LOG_D(MAC,
                "[eNB %d] total RB allocated for UE%d CC_id %d  = %d\n",
                module_idP, UE_id, CC_id,
                UE_template->pre_allocated_nb_rb_ul[slice_id]);
        }
      }
    }
  }

#if 0
    /* this logging is wrong, ue_sched_ctl may not be valid here
     * TODO: fix
     */
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

	if (total_allocated_rbs[CC_id] > 0) {
	    LOG_D(MAC, "[eNB %d] total RB allocated for all UEs = %d/%d\n",
		  module_idP, total_allocated_rbs[CC_id],
              ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_id] - first_rb[CC_id]);
	}
    }
#endif
}


void
assign_max_mcs_min_rb(module_id_t module_idP, int slice_id, int frameP,
		      sub_frame_t subframeP, uint16_t * first_rb)
{

  int i;
  uint16_t n, UE_id;
  uint8_t CC_id;
  rnti_t rnti = -1;
  int mcs;
  int rb_table_index = 0, tbs, tx_power;
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_list_t *UE_list = &eNB->UE_list;

  UE_TEMPLATE *UE_template;
  UE_sched_ctrl *ue_sched_ctl;
  int Ncp;
  int N_RB_UL;

  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    if (UE_list->active[i] != TRUE)
      continue;

    rnti = UE_RNTI(module_idP, i);

    if (rnti == NOT_A_RNTI)
      continue;
    if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
      continue;
    if (!ue_slice_membership(i, slice_id))
      continue;

    if (UE_list->UE_sched_ctrl[i].phr_received == 1) {
      /* if we've received the power headroom information the UE, we can go to
       * maximum mcs */
      mcs = cmin(20, slice_maxmcs_uplink[slice_id]);
    } else {
      /* otherwise, limit to QPSK PUSCH */
      mcs = cmin(10, slice_maxmcs_uplink[slice_id]);
    }

    UE_id = i;

    for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];

      if (CC_id >= MAX_NUM_CCs) {
        LOG_E(MAC, "CC_id %u should be < %u, loop n=%u < numactiveULCCs[%u]=%u",
              CC_id, MAX_NUM_CCs, n, UE_id, UE_list->numactiveULCCs[UE_id]);
      }

      AssertFatal(CC_id < MAX_NUM_CCs,
                  "CC_id %u should be < %u, loop n=%u < numactiveULCCs[%u]=%u",
                  CC_id, MAX_NUM_CCs, n, UE_id,
                  UE_list->numactiveULCCs[UE_id]);

      UE_template = &UE_list->UE_template[CC_id][UE_id];
      UE_template->pre_assigned_mcs_ul = mcs;
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

      Ncp = RC.mac[module_idP]->common_channels[CC_id].Ncp;
      N_RB_UL = to_prb(RC.mac[module_idP]->common_channels[CC_id].ul_Bandwidth);
      ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_id] = flexran_nb_rbs_allowed_slice(slice_percentage_uplink[slice_id],N_RB_UL);

      int bytes_to_schedule = UE_template->estimated_ul_buffer - UE_template->scheduled_ul_bytes;
      if (bytes_to_schedule < 0) bytes_to_schedule = 0;
      int bits_to_schedule = bytes_to_schedule * 8;

      // if this UE has UL traffic
      if (bits_to_schedule > 0) {
        tbs = get_TBS_UL(UE_template->pre_assigned_mcs_ul, 3) << 3; // 1 or 2 PRB with cqi enabled does not work well!
        rb_table_index = 2;

        // fixme: set use_srs flag
        tx_power = estimate_ue_tx_power(tbs, rb_table[rb_table_index], 0, Ncp, 0);

        while ((((UE_template->phr_info - tx_power) < 0)
              || (tbs > bits_to_schedule))
              && (UE_template->pre_assigned_mcs_ul > 3)) {
          // LOG_I(MAC,"UE_template->phr_info %d tx_power %d mcs %d\n", UE_template->phr_info,tx_power, mcs);
          UE_template->pre_assigned_mcs_ul--;
          tbs = get_TBS_UL(UE_template->pre_assigned_mcs_ul, rb_table[rb_table_index]) << 3;
          tx_power = estimate_ue_tx_power(tbs, rb_table[rb_table_index], 0, Ncp, 0);   // fixme: set use_srs
        }

        while ((tbs < bits_to_schedule)
                && (rb_table[rb_table_index] < (ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_id] - first_rb[CC_id]))
                && ((UE_template->phr_info - tx_power) > 0)
                && (rb_table_index < 32)) {
          rb_table_index++;
          tbs = get_TBS_UL(UE_template->pre_assigned_mcs_ul, rb_table[rb_table_index]) << 3;
          tx_power = estimate_ue_tx_power(tbs, rb_table[rb_table_index], 0, Ncp, 0);
        }

        UE_template->ue_tx_power = tx_power;

        if (rb_table[rb_table_index] > (ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_id] - first_rb[CC_id] - 1)) {
          rb_table_index--;
        }
        // 1 or 2 PRB with cqi enabled does not work well
        if (rb_table[rb_table_index] < 3) {
            rb_table_index = 2;	//3PRB
        }

        UE_template->pre_allocated_rb_table_index_ul = rb_table_index;
        UE_template->pre_allocated_nb_rb_ul[slice_id] = rb_table[rb_table_index];
        LOG_D(MAC,
              "[eNB %d] frame %d subframe %d: for UE %d CC %d: pre-assigned mcs %d, pre-allocated rb_table[%d]=%d RBs (phr %d, tx power %d)\n",
              module_idP, frameP, subframeP, UE_id, CC_id,
              UE_template->pre_assigned_mcs_ul,
              UE_template->pre_allocated_rb_table_index_ul,
              UE_template->pre_allocated_nb_rb_ul[slice_id],
              UE_template->phr_info, tx_power);
      } else {
        /* if UE has pending scheduling request then pre-allocate 3 RBs */
        //if (UE_template->ul_active == 1 && UE_template->ul_SR == 1) {
        if (UE_is_to_be_scheduled(module_idP, CC_id, i)) {
          /* use QPSK mcs */
          UE_template->pre_assigned_mcs_ul = 10;
          UE_template->pre_allocated_rb_table_index_ul = 2;
          UE_template->pre_allocated_nb_rb_ul[slice_id] = 3;
        } else {
          UE_template->pre_assigned_mcs_ul = 0;
          UE_template->pre_allocated_rb_table_index_ul = -1;
          UE_template->pre_allocated_nb_rb_ul[slice_id] = 0;
        }
      }
    }
  }
}

struct sort_ue_ul_params {
    int module_idP;
    int frameP;
    int subframeP;
};

static int ue_ul_compare(const void *_a, const void *_b, void *_params)
{
    struct sort_ue_ul_params *params = _params;
    UE_list_t *UE_list = &RC.mac[params->module_idP]->UE_list;

    int UE_id1 = *(const int *) _a;
    int UE_id2 = *(const int *) _b;

    int rnti1 = UE_RNTI(params->module_idP, UE_id1);
    int pCCid1 = UE_PCCID(params->module_idP, UE_id1);
    int round1 = maxround(params->module_idP, rnti1, params->frameP,
			  params->subframeP, 1);

    int rnti2 = UE_RNTI(params->module_idP, UE_id2);
    int pCCid2 = UE_PCCID(params->module_idP, UE_id2);
    int round2 = maxround(params->module_idP, rnti2, params->frameP,
			  params->subframeP, 1);

    if (round1 > round2)
	return -1;
    if (round1 < round2)
	return 1;

    if (UE_list->UE_template[pCCid1][UE_id1].ul_buffer_info[LCGID0] >
	UE_list->UE_template[pCCid2][UE_id2].ul_buffer_info[LCGID0])
	return -1;
    if (UE_list->UE_template[pCCid1][UE_id1].ul_buffer_info[LCGID0] <
	UE_list->UE_template[pCCid2][UE_id2].ul_buffer_info[LCGID0])
	return 1;

    int bytes_to_schedule1 = UE_list->UE_template[pCCid1][UE_id1].estimated_ul_buffer - UE_list->UE_template[pCCid1][UE_id1].scheduled_ul_bytes;
    if (bytes_to_schedule1 < 0) bytes_to_schedule1 = 0;
    int bytes_to_schedule2 = UE_list->UE_template[pCCid2][UE_id2].estimated_ul_buffer - UE_list->UE_template[pCCid2][UE_id2].scheduled_ul_bytes;
    if (bytes_to_schedule2 < 0) bytes_to_schedule2 = 0;

    if (bytes_to_schedule1 > bytes_to_schedule2)
	return -1;
    if (bytes_to_schedule1 < bytes_to_schedule2)
	return 1;

    if (UE_list->UE_template[pCCid1][UE_id1].pre_assigned_mcs_ul >
	UE_list->UE_template[pCCid2][UE_id2].pre_assigned_mcs_ul)
	return -1;
    if (UE_list->UE_template[pCCid1][UE_id1].pre_assigned_mcs_ul <
	UE_list->UE_template[pCCid2][UE_id2].pre_assigned_mcs_ul)
	return 1;

    return 0;

#if 0
    /* The above order derives from the following.
     * The last case is not handled: "if (UE_list->UE_template[pCCid2][UE_id2].ul_total_buffer > 0 )"
     * I don't think it makes a big difference.
     */
    if (round2 > round1) {
	swap_UEs(UE_list, UE_id1, UE_id2, 1);
    } else if (round2 == round1) {
	if (UE_list->UE_template[pCCid1][UE_id1].ul_buffer_info[LCGID0] <
	    UE_list->UE_template[pCCid2][UE_id2].ul_buffer_info[LCGID0]) {
	    swap_UEs(UE_list, UE_id1, UE_id2, 1);
	} else if (UE_list->UE_template[pCCid1][UE_id1].ul_total_buffer <
		   UE_list->UE_template[pCCid2][UE_id2].ul_total_buffer) {
	    swap_UEs(UE_list, UE_id1, UE_id2, 1);
	} else if (UE_list->UE_template[pCCid1][UE_id1].
		   pre_assigned_mcs_ul <
		   UE_list->UE_template[pCCid2][UE_id2].
		   pre_assigned_mcs_ul) {
	    if (UE_list->UE_template[pCCid2][UE_id2].ul_total_buffer > 0) {
		swap_UEs(UE_list, UE_id1, UE_id2, 1);
	    }
	}
    }
#endif
}

void sort_ue_ul(module_id_t module_idP, int frameP, sub_frame_t subframeP)
{
    int i;
    int list[NUMBER_OF_UE_MAX];
    int list_size = 0;
    int rnti;
    struct sort_ue_ul_params params = { module_idP, frameP, subframeP };

    UE_list_t *UE_list = &RC.mac[module_idP]->UE_list;

    for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
	if (UE_list->active[i] == FALSE)
	    continue;
	if ((rnti = UE_RNTI(module_idP, i)) == NOT_A_RNTI)
	    continue;
	if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
	    continue;

	list[list_size] = i;
	list_size++;
    }

    qsort_r(list, list_size, sizeof(int), ue_ul_compare, &params);

    if (list_size) {
	for (i = 0; i < list_size - 1; i++)
	    UE_list->next_ul[list[i]] = list[i + 1];
	UE_list->next_ul[list[list_size - 1]] = -1;
	UE_list->head_ul = list[0];
    } else {
	UE_list->head_ul = -1;
    }

#if 0
    int UE_id1, UE_id2;
    int pCCid1, pCCid2;
    int round1, round2;
    int i = 0, ii = 0;
    rnti_t rnti1, rnti2;

    UE_list_t *UE_list = &RC.mac[module_idP]->UE_list;

    for (i = UE_list->head_ul; i >= 0; i = UE_list->next_ul[i]) {

	//LOG_I(MAC,"sort ue ul i %d\n",i);
	for (ii = UE_list->next_ul[i]; ii >= 0; ii = UE_list->next_ul[ii]) {
	    //LOG_I(MAC,"sort ul ue 2 ii %d\n",ii);

	    UE_id1 = i;
	    rnti1 = UE_RNTI(module_idP, UE_id1);

	    if (rnti1 == NOT_A_RNTI)
		continue;
	    if (UE_list->UE_sched_ctrl[i].ul_out_of_sync == 1)
		continue;


	    pCCid1 = UE_PCCID(module_idP, UE_id1);
	    round1 = maxround(module_idP, rnti1, frameP, subframeP, 1);

	    UE_id2 = ii;
	    rnti2 = UE_RNTI(module_idP, UE_id2);

	    if (rnti2 == NOT_A_RNTI)
		continue;
	    if (UE_list->UE_sched_ctrl[UE_id2].ul_out_of_sync == 1)
		continue;

	    pCCid2 = UE_PCCID(module_idP, UE_id2);
	    round2 = maxround(module_idP, rnti2, frameP, subframeP, 1);

	    if (round2 > round1) {
		swap_UEs(UE_list, UE_id1, UE_id2, 1);
	    } else if (round2 == round1) {
		if (UE_list->
		    UE_template[pCCid1][UE_id1].ul_buffer_info[LCGID0] <
		    UE_list->UE_template[pCCid2][UE_id2].
		    ul_buffer_info[LCGID0]) {
		    swap_UEs(UE_list, UE_id1, UE_id2, 1);
		} else if (UE_list->UE_template[pCCid1][UE_id1].
			   ul_total_buffer <
			   UE_list->UE_template[pCCid2][UE_id2].
			   ul_total_buffer) {
		    swap_UEs(UE_list, UE_id1, UE_id2, 1);
		} else if (UE_list->
			   UE_template[pCCid1][UE_id1].pre_assigned_mcs_ul
			   <
			   UE_list->
			   UE_template[pCCid2][UE_id2].pre_assigned_mcs_ul)
		{
		    if (UE_list->UE_template[pCCid2][UE_id2].
			ul_total_buffer > 0) {
			swap_UEs(UE_list, UE_id1, UE_id2, 1);
		    }
		}
	    }
	}
    }
#endif
}
