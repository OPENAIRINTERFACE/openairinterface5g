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
#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "rlc.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"

#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;

#define DEBUG_eNB_SCHEDULER
#define DEBUG_HEADER_PARSING 1

int next_ue_list_looped(UE_list_t* list, int UE_id) {
  if (UE_id < 0)
    return list->head;
  return list->next[UE_id] < 0 ? list->head : list->next[UE_id];
}

int g_start_ue_dl = -1;
int round_robin_dl(module_id_t Mod_id,
                   int CC_id,
                   int frame,
                   int subframe,
                   UE_list_t *UE_list,
                   int max_num_ue,
                   int n_rbg_sched,
                   uint8_t *rbgalloc_mask) {
  DevAssert(UE_list->head >= 0);
  DevAssert(n_rbg_sched > 0);
  const int RBGsize = get_min_rb_unit(Mod_id, CC_id);
  int num_ue_req = 0;
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;

  int rb_required[MAX_MOBILES_PER_ENB]; // how much UEs request
  memset(rb_required, 0, sizeof(rb_required));

  int rbg = 0;
  for (; !rbgalloc_mask[rbg]; rbg++)
    ; /* fast-forward to first allowed RBG */

  // Calculate the amount of RBs every UE wants to send.
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    // check whether there are HARQ retransmissions
    const COMMON_channels_t *cc = &RC.mac[Mod_id]->common_channels[CC_id];
    const uint8_t harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config, frame, subframe);
    UE_sched_ctrl_t *ue_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    const uint8_t round = ue_ctrl->round[CC_id][harq_pid];
    if (round != 8) { // retransmission: allocate
      int nb_rb = UE_info->UE_template[CC_id][UE_id].nb_rb[harq_pid];
      if (nb_rb % RBGsize != 0) {
        nb_rb += nb_rb % RBGsize; // should now divide evenly
      }
      int nb_rbg = nb_rb / RBGsize;
      if (nb_rbg > n_rbg_sched) // needs more RBGs than we can allocate
        continue;
      // retransmissions: directly allocate
      n_rbg_sched -= nb_rbg;
      ue_ctrl->pre_nb_available_rbs[CC_id] += nb_rb;
      for (; nb_rbg > 0; rbg++) {
        if (!rbgalloc_mask[rbg])
          continue;
        ue_ctrl->rballoc_sub_UE[CC_id][rbg] = 1;
        nb_rbg--;
      }
      LOG_D(MAC,
            "%4d.%d n_rbg_sched %d after retransmission reservation for UE %d "
            "round %d retx nb_rb %d pre_nb_available_rbs %d\n",
            frame, subframe, n_rbg_sched, UE_id, round,
            UE_info->UE_template[CC_id][UE_id].nb_rb[harq_pid],
            ue_ctrl->pre_nb_available_rbs[CC_id]);
      /* if there are no more RBG to give, return */
      if (n_rbg_sched <= 0)
        return 0;
      max_num_ue--;
      /* if there are no UEs that can be allocated anymore, return */
      if (max_num_ue == 0)
        return n_rbg_sched;
      for (; !rbgalloc_mask[rbg]; rbg++) /* fast-forward */ ;
    } else {
      const int dlsch_mcs1 = cqi_to_mcs[UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id]];
      UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1 = dlsch_mcs1;
      rb_required[UE_id] =
          find_nb_rb_DL(dlsch_mcs1,
                        UE_info->UE_template[CC_id][UE_id].dl_buffer_total,
                        n_rbg_sched * RBGsize,
                        RBGsize);
      if (rb_required[UE_id] > 0)
        num_ue_req++;
      LOG_D(MAC,
            "%d/%d UE_id %d rb_required %d n_rbg_sched %d\n",
            frame,
            subframe,
            UE_id,
            rb_required[UE_id],
            n_rbg_sched);
    }
  }

  if (num_ue_req == 0)
    return n_rbg_sched; // no UE has a transmission

  // after allocating retransmissions: build list of UE to allocate.
  // if start_UE does not exist anymore (detach etc), start at first UE
  if (g_start_ue_dl == -1)
    g_start_ue_dl = UE_list->head;
  int UE_id = g_start_ue_dl;
  UE_list_t UE_sched;
  int rb_required_total = 0;
  int num_ue_sched = 0;
  max_num_ue = min(min(max_num_ue, num_ue_req), n_rbg_sched);
  int *cur_UE = &UE_sched.head;
  while (num_ue_sched < max_num_ue) {
    while (rb_required[UE_id] == 0)
      UE_id = next_ue_list_looped(UE_list, UE_id);
    /* TODO: check that CCE allocation is feasible. If it is not, reduce
     * num_ue_req anyway because this would have been one opportunity */
    *cur_UE = UE_id;
    cur_UE = &UE_sched.next[UE_id];
    rb_required_total += rb_required[UE_id];
    num_ue_sched++;
    UE_id = next_ue_list_looped(UE_list, UE_id); // next candidate
  }
  *cur_UE = -1;

  /* for one UE after the next: allocate resources */
  for (int UE_id = UE_sched.head; UE_id >= 0;
       UE_id = next_ue_list_looped(&UE_sched, UE_id)) {
    if (rb_required[UE_id] <= 0)
      continue;
    UE_sched_ctrl_t *ue_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    ue_ctrl->rballoc_sub_UE[CC_id][rbg] = 1;
    ue_ctrl->pre_nb_available_rbs[CC_id] += RBGsize;
    rb_required[UE_id] -= RBGsize;
    rb_required_total -= RBGsize;
    if (rb_required_total <= 0)
      break;
    n_rbg_sched--;
    if (n_rbg_sched <= 0)
      break;
    for (rbg++; !rbgalloc_mask[rbg]; rbg++) /* fast-forward */ ;
  }

  /* if not all UEs could be allocated in this round */
  if (num_ue_req > max_num_ue) {
    /* go to the first one we missed */
    for (int i = 0; i < max_num_ue; ++i)
      g_start_ue_dl = next_ue_list_looped(UE_list, g_start_ue_dl);
  } else {
    /* else, just start with the next UE next time */
    g_start_ue_dl = next_ue_list_looped(UE_list, g_start_ue_dl);
  }

  return n_rbg_sched;
}

void
sort_ue_ul(module_id_t module_idP,
           int slice_idx,
           int sched_frameP,
           sub_frame_t sched_subframeP,
           rnti_t *rntiTable);

// This function stores the downlink buffer for all the logical channels
void
store_dlsch_buffer(module_id_t Mod_id,
                   int CC_id,
                   frame_t frameP,
                   sub_frame_t subframeP) {
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {

    UE_TEMPLATE *UE_template = &UE_info->UE_template[CC_id][UE_id];
    UE_sched_ctrl_t *UE_sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    UE_template->dl_buffer_total = 0;
    UE_template->dl_pdus_total = 0;

    /* loop over all activated logical channels */
    for (int i = 0; i < UE_sched_ctrl->dl_lc_num; ++i) {
      const int lcid = UE_sched_ctrl->dl_lc_ids[i];
      const mac_rlc_status_resp_t rlc_status = mac_rlc_status_ind(Mod_id,
                                                                  UE_template->rnti,
                                                                  Mod_id,
                                                                  frameP,
                                                                  subframeP,
                                                                  ENB_FLAG_YES,
                                                                  MBMS_FLAG_NO,
                                                                  lcid,
                                                                  0,
                                                                  0
                                                                 );
      UE_template->dl_buffer_info[lcid] = rlc_status.bytes_in_buffer;
      UE_template->dl_pdus_in_buffer[lcid] = rlc_status.pdus_in_buffer;
      UE_template->dl_buffer_head_sdu_creation_time[lcid] = rlc_status.head_sdu_creation_time;
      UE_template->dl_buffer_head_sdu_creation_time_max =
        cmax(UE_template->dl_buffer_head_sdu_creation_time_max, rlc_status.head_sdu_creation_time);
      UE_template->dl_buffer_head_sdu_remaining_size_to_send[lcid] = rlc_status.head_sdu_remaining_size_to_send;
      UE_template->dl_buffer_head_sdu_is_segmented[lcid] = rlc_status.head_sdu_is_segmented;
      UE_template->dl_buffer_total += UE_template->dl_buffer_info[lcid];
      UE_template->dl_pdus_total += UE_template->dl_pdus_in_buffer[lcid];

      /* update the number of bytes in the UE_sched_ctrl. The DLSCH will use
       * this to request the corresponding data from the RLC, and this might be
       * limited in the preprocessor */
      UE_sched_ctrl->dl_lc_bytes[i] = rlc_status.bytes_in_buffer;

#ifdef DEBUG_eNB_SCHEDULER
      /* note for dl_buffer_head_sdu_remaining_size_to_send[lcid] :
       * 0 if head SDU has not been segmented (yet), else remaining size not already segmented and sent
       */
      if (UE_template->dl_buffer_info[lcid] > 0)
        LOG_D(MAC,
              "[eNB %d] Frame %d Subframe %d : RLC status for UE %d in LCID%d: total of %d pdus and size %d, head sdu queuing time %d, remaining size %d, is segmeneted %d \n",
              Mod_id, frameP,
              subframeP, UE_id, lcid, UE_template->dl_pdus_in_buffer[lcid],
              UE_template->dl_buffer_info[lcid],
              UE_template->dl_buffer_head_sdu_creation_time[lcid],
              UE_template->dl_buffer_head_sdu_remaining_size_to_send[lcid],
              UE_template->dl_buffer_head_sdu_is_segmented[lcid]);
#endif
    }

    if (UE_template->dl_buffer_total > 0)
      LOG_D(MAC,
            "[eNB %d] Frame %d Subframe %d : RLC status for UE %d : total DL buffer size %d and total number of pdu %d \n",
            Mod_id, frameP, subframeP, UE_id,
            UE_template->dl_buffer_total,
            UE_template->dl_pdus_total);
  }
}

int
maxround_ul(module_id_t Mod_id, uint16_t rnti, int sched_frame,
            sub_frame_t sched_subframe) {
  uint8_t round, round_max = 0, UE_id;
  int CC_id, harq_pid;
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;
  COMMON_channels_t *cc;

  for (CC_id = 0; CC_id < RC.nb_mac_CC[Mod_id]; CC_id++) {
    cc = &RC.mac[Mod_id]->common_channels[CC_id];
    UE_id = find_UE_id(Mod_id, rnti);

    if(UE_id == -1)
      continue;

    harq_pid = subframe2harqpid(cc, sched_frame, sched_subframe);
    round = UE_info->UE_sched_ctrl[UE_id].round_UL[CC_id][harq_pid];

    if (round > round_max) {
      round_max = round;
    }
  }

  return round_max;
}

// This function assigns pre-available RBS to each UE in specified sub-bands before scheduling is done
void
dlsch_scheduler_pre_processor(module_id_t Mod_id,
                              int CC_id,
                              frame_t frameP,
                              sub_frame_t subframeP) {
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;
  const int N_RBG = to_rbg(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
  const int RBGsize = get_min_rb_unit(Mod_id, CC_id);

  store_dlsch_buffer(Mod_id, CC_id, frameP, subframeP);

  UE_list_t UE_to_sched;
  UE_to_sched.head = -1;
  for (int i = 0; i < MAX_MOBILES_PER_ENB; ++i)
    UE_to_sched.next[i] = -1;

  int first = 1;
  int last_UE_id = -1;
  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    UE_sched_ctrl_t *ue_sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

    /* initialize per-UE scheduling information */
    ue_sched_ctrl->pre_nb_available_rbs[CC_id] = 0;
    ue_sched_ctrl->dl_pow_off[CC_id] = 2;
    memset(ue_sched_ctrl->rballoc_sub_UE[CC_id], 0, sizeof(ue_sched_ctrl->rballoc_sub_UE[CC_id]));

    const rnti_t rnti = UE_RNTI(Mod_id, UE_id);
    if (rnti == NOT_A_RNTI) {
      LOG_E(MAC, "UE %d has RNTI NOT_A_RNTI!\n", UE_id);
      continue;
    }
    if (UE_info->active[UE_id] != TRUE) {
      LOG_E(MAC, "UE %d RNTI %x is NOT active!\n", UE_id, rnti);
      continue;
    }

    /* define UEs to schedule */
    if (first) {
      first = 0;
      UE_to_sched.head = UE_id;
    } else {
      UE_to_sched.next[last_UE_id] = UE_id;
    }
    UE_to_sched.next[UE_id] = -1;
    last_UE_id = UE_id;
  }

  if (UE_to_sched.head < 0)
    return;

  uint8_t *vrb_map = RC.mac[Mod_id]->common_channels[CC_id].vrb_map;
  uint8_t rbgalloc_mask[N_RBG_MAX];
  int n_rbg_sched = 0;
  for (int i = 0; i < N_RBG; i++) {
    // calculate mask: init to one + "AND" with vrb_map:
    // if any RB in vrb_map is blocked (1), the current RBG will be 0
    rbgalloc_mask[i] = 1;
    for (int j = 0; j < RBGsize; j++)
      rbgalloc_mask[i] &= !vrb_map[RBGsize * i + j];
    n_rbg_sched += rbgalloc_mask[i];
  }

  round_robin_dl(Mod_id,
                 CC_id,
                 frameP,
                 subframeP,
                 &UE_to_sched,
                 3, // max_num_ue
                 n_rbg_sched,
                 rbgalloc_mask);

  // the following block is meant for validation of the pre-processor to check
  // whether all UE allocations are non-overlapping and is not necessary for
  // scheduling functionality
#ifdef DEBUG_eNB_SCHEDULER
  char t[26] = "_________________________";
  t[N_RBG] = 0;
  for (int i = 0; i < N_RBG; i++)
    for (int j = 0; j < RBGsize; j++)
      if (vrb_map[RBGsize*i+j] != 0)
        t[i] = 'x';
  int print = 0;
  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    const UE_sched_ctrl_t *ue_sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

    if (ue_sched_ctrl->pre_nb_available_rbs[CC_id] == 0)
      continue;

    LOG_D(MAC,
          "%4d.%d UE%d %d RBs allocated, pre MCS %d\n",
          frameP,
          subframeP,
          UE_id,
          ue_sched_ctrl->pre_nb_available_rbs[CC_id],
          UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1);

    print = 1;

    for (int i = 0; i < N_RBG; i++) {
      if (!ue_sched_ctrl->rballoc_sub_UE[CC_id][i])
        continue;
      for (int j = 0; j < RBGsize; j++) {
        if (vrb_map[RBGsize*i+j] != 0) {
          LOG_I(MAC, "%4d.%d DL scheduler allocation list: %s\n", frameP, subframeP, t);
          LOG_E(MAC, "%4d.%d: UE %d allocated at locked RB %d/RBG %d\n", frameP,
                subframeP, UE_id, RBGsize * i + j, i);
        }
        vrb_map[RBGsize*i+j] = 1;
      }
      t[i] = '0' + UE_id;
    }
  }
  if (print)
    LOG_I(MAC, "%4d.%d DL scheduler allocation list: %s\n", frameP, subframeP, t);
#endif
}

/// ULSCH PRE_PROCESSOR

void ulsch_scheduler_pre_processor(module_id_t module_idP,
                                   int CC_id,
                                   int frameP,
                                   sub_frame_t subframeP,
                                   int sched_frameP,
                                   unsigned char sched_subframeP,
                                   uint16_t *first_rb) {
  int UE_id;
  uint16_t n;
  uint8_t harq_pid;
  uint16_t nb_allocated_rbs[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB];
  uint16_t total_allocated_rbs[NFAPI_CC_MAX];
  uint16_t average_rbs_per_user[NFAPI_CC_MAX];
  int16_t total_remaining_rbs[NFAPI_CC_MAX];
  uint16_t total_ue_count[NFAPI_CC_MAX];
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_info_t *UE_info = &eNB->UE_info;
  slice_info_t *sli = &eNB->slice_info;
  const int slice_idx = 0;
  UE_TEMPLATE *UE_template = 0;
  UE_sched_ctrl_t *ue_sched_ctl;
  int N_RB_UL = 0;
  uint16_t available_rbs, first_rb_offset;
  rnti_t rntiTable[MAX_MOBILES_PER_ENB];
  // sort ues
  LOG_D(MAC, "In ulsch_preprocessor: sort ue \n");
  sort_ue_ul(module_idP, slice_idx, sched_frameP, sched_subframeP, rntiTable);
  // maximize MCS and then allocate required RB according to the buffer occupancy with the limit of max available UL RB
  LOG_D(MAC, "In ulsch_preprocessor: assign max mcs min rb\n");
  assign_max_mcs_min_rb(module_idP, slice_idx, frameP, subframeP, first_rb);
  // we need to distribute RBs among UEs
  // step1:  reset the vars
  uint8_t CC_nb = (uint8_t) RC.nb_mac_CC[module_idP];

  for (CC_id = 0; CC_id < CC_nb; CC_id++) {
    total_allocated_rbs[CC_id] = 0;
    total_remaining_rbs[CC_id] = 0;
    average_rbs_per_user[CC_id] = 0;
    total_ue_count[CC_id] = 0;
  }

  // Step 1.5: Calculate total_ue_count
  for (UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    // This is not the actual CC_id in the list
    for (n = 0; n < UE_info->numactiveULCCs[UE_id]; n++) {
      CC_id = UE_info->ordered_ULCCids[n][UE_id];
      UE_template = &UE_info->UE_template[CC_id][UE_id];

      if (UE_template->pre_allocated_nb_rb_ul[slice_idx] > 0) {
        total_ue_count[CC_id]++;
      }
    }
  }

  // step 2: calculate the average rb per UE
  LOG_D(MAC, "In ulsch_preprocessor: step2 \n");

  for (UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    if (UE_info->UE_template[CC_id][UE_id].rach_resource_type > 0) continue;

    LOG_D(MAC, "In ulsch_preprocessor: handling UE %d/%x\n",
          UE_id,
          rntiTable[UE_id]);

    for (n = 0; n < UE_info->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_info->ordered_ULCCids[n][UE_id];
      LOG_D(MAC, "In ulsch_preprocessor: handling UE %d/%x CCid %d\n",
            UE_id,
            rntiTable[UE_id],
            CC_id);
      /*
          if((mac_xface->get_nCCE_max(module_idP,CC_id,3,subframeP) - nCCE_to_be_used[CC_id])  > (1<<aggregation)) {
          nCCE_to_be_used[CC_id] = nCCE_to_be_used[CC_id] + (1<<aggregation);
          max_num_ue_to_be_scheduled+=1;
          } */
      N_RB_UL = to_prb(eNB->common_channels[CC_id].ul_Bandwidth);
      ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
      ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_idx] =
        nb_rbs_allowed_slice(sli->ul[slice_idx].pct, N_RB_UL);
      first_rb_offset = UE_info->first_rb_offset[CC_id][slice_idx];
      available_rbs = N_RB_UL - 2 * first_rb[CC_id]; // factor 2: top&bottom

      if (available_rbs < 0)
        available_rbs = 0;

      if (total_ue_count[CC_id] == 0) {
        average_rbs_per_user[CC_id] = 0;
      } else if (total_ue_count[CC_id] == 1) {    // increase the available RBs, special case,
        average_rbs_per_user[CC_id] = (uint16_t) (available_rbs + 1);
      } else if (total_ue_count[CC_id] <= available_rbs) {
        average_rbs_per_user[CC_id] = (uint16_t) floor(available_rbs / total_ue_count[CC_id]);
      } else {
        average_rbs_per_user[CC_id] = 1;
        LOG_W(MAC, "[eNB %d] frame %d subframe %d: UE %d CC %d: can't get average rb per user (should not be here)\n",
              module_idP,
              frameP,
              subframeP,
              UE_id,
              CC_id);
      }

      if (total_ue_count[CC_id] > 0) {
        LOG_D(MAC, "[eNB %d] Frame %d subframe %d: total ue to be scheduled %d\n",
              module_idP,
              frameP,
              subframeP,
              total_ue_count[CC_id]);
      }
    }
  }

  // step 3: assigne RBS
  for (UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    // if (continueTable[UE_id]) continue;
    for (n = 0; n < UE_info->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_info->ordered_ULCCids[n][UE_id];
      UE_template = &UE_info->UE_template[CC_id][UE_id];
      harq_pid = subframe2harqpid(&RC.mac[module_idP]->common_channels[CC_id],
                                  sched_frameP, sched_subframeP);

      //      mac_xface->get_ue_active_harq_pid(module_idP,CC_id,rnti,frameP,subframeP,&harq_pid,&round,openair_harq_UL);

      if (UE_info->UE_sched_ctrl[UE_id].round_UL[CC_id][harq_pid] > 0) {
        nb_allocated_rbs[CC_id][UE_id] = UE_info->UE_template[CC_id][UE_id].nb_rb_ul[harq_pid];
      } else {
        nb_allocated_rbs[CC_id][UE_id] =
          cmin(UE_info->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul[slice_idx], average_rbs_per_user[CC_id]);
      }

      total_allocated_rbs[CC_id] += nb_allocated_rbs[CC_id][UE_id];
      LOG_D(MAC, "In ulsch_preprocessor: assigning %d RBs for UE %d/%x CCid %d, harq_pid %d\n",
            nb_allocated_rbs[CC_id][UE_id],
            UE_id,
            rntiTable[UE_id],
            CC_id,
            harq_pid);
    }
  }

  // step 4: assigne the remaining RBs and set the pre_allocated rbs accordingly
  for (UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    // if (continueTable[UE_id]) continue;
    ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];

    for (n = 0; n < UE_info->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_info->ordered_ULCCids[n][UE_id];
      UE_template = &UE_info->UE_template[CC_id][UE_id];
      N_RB_UL = to_prb(eNB->common_channels[CC_id].ul_Bandwidth);
      first_rb_offset = UE_info->first_rb_offset[CC_id][slice_idx];
      available_rbs = cmin(ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_idx], N_RB_UL - first_rb[CC_id] - first_rb_offset);
      total_remaining_rbs[CC_id] = available_rbs - total_allocated_rbs[CC_id];

      if (total_ue_count[CC_id] == 1) {
        total_remaining_rbs[CC_id]++;
      }

      while (UE_template->pre_allocated_nb_rb_ul[slice_idx] > 0 &&
             nb_allocated_rbs[CC_id][UE_id] < UE_template->pre_allocated_nb_rb_ul[slice_idx] &&
             total_remaining_rbs[CC_id] > 0) {
        nb_allocated_rbs[CC_id][UE_id] = cmin(nb_allocated_rbs[CC_id][UE_id] + 1, UE_template->pre_allocated_nb_rb_ul[slice_idx]);
        total_remaining_rbs[CC_id]--;
        total_allocated_rbs[CC_id]++;
      }

      UE_template->pre_allocated_nb_rb_ul[slice_idx] = nb_allocated_rbs[CC_id][UE_id];
      LOG_D(MAC, "******************UL Scheduling Information for UE%d CC_id %d ************************\n",
            UE_id,
            CC_id);
      LOG_D(MAC, "[eNB %d] total RB allocated for UE%d CC_id %d  = %d\n",
            module_idP,
            UE_id,
            CC_id,
            UE_template->pre_allocated_nb_rb_ul[slice_idx]);
    }
  }

  return;
}

void
assign_max_mcs_min_rb(module_id_t module_idP,
                      int slice_idx,
                      int frameP,
                      sub_frame_t subframeP,
                      uint16_t *first_rb) {
  int i;
  uint16_t n, UE_id;
  uint8_t CC_id;
  int mcs;
  int rb_table_index = 0, tbs, tx_power;
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_info_t *UE_info = &eNB->UE_info;
  slice_info_t *sli = &eNB->slice_info;
  UE_TEMPLATE *UE_template;
  UE_sched_ctrl_t *ue_sched_ctl;
  int Ncp;
  int N_RB_UL;
  int first_rb_offset, available_rbs;

  for (i = UE_info->list.head; i >= 0; i = UE_info->list.next[i]) {
    if (UE_info->UE_sched_ctrl[i].phr_received == 1) {
      /* if we've received the power headroom information the UE, we can go to
       * maximum mcs */
      mcs = cmin(20, sli->ul[slice_idx].maxmcs);
    } else {
      /* otherwise, limit to QPSK PUSCH */
      mcs = cmin(10, sli->ul[slice_idx].maxmcs);
    }

    UE_id = i;

    for (n = 0; n < UE_info->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_info->ordered_ULCCids[n][UE_id];
      AssertFatal(CC_id < RC.nb_mac_CC[module_idP], "CC_id %u should be < %u, loop n=%u < numactiveULCCs[%u]=%u",
                  CC_id,
                  NFAPI_CC_MAX,
                  n,
                  UE_id,
                  UE_info->numactiveULCCs[UE_id]);
      UE_template = &UE_info->UE_template[CC_id][UE_id];
      UE_template->pre_assigned_mcs_ul = mcs;
      ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
      Ncp = eNB->common_channels[CC_id].Ncp;
      N_RB_UL = to_prb(eNB->common_channels[CC_id].ul_Bandwidth);
      ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_idx] = nb_rbs_allowed_slice(sli->ul[slice_idx].pct, N_RB_UL);
      int bytes_to_schedule = UE_template->estimated_ul_buffer - UE_template->scheduled_ul_bytes;

      if (bytes_to_schedule < 0) bytes_to_schedule = 0;

      int bits_to_schedule = bytes_to_schedule * 8;

      // if this UE has UL traffic
      if (bits_to_schedule > 0) {
        tbs = get_TBS_UL(UE_template->pre_assigned_mcs_ul, 3) << 3; // 1 or 2 PRB with cqi enabled does not work well!
        rb_table_index = 2;
        // fixme: set use_srs flag
        tx_power = estimate_ue_tx_power(tbs, rb_table[rb_table_index], 0, Ncp, 0);

        while ((UE_template->phr_info - tx_power < 0 || tbs > bits_to_schedule) && UE_template->pre_assigned_mcs_ul > 3) {
          // LOG_I(MAC,"UE_template->phr_info %d tx_power %d mcs %d\n", UE_template->phr_info,tx_power, mcs);
          UE_template->pre_assigned_mcs_ul--;
          tbs = get_TBS_UL(UE_template->pre_assigned_mcs_ul, rb_table[rb_table_index]) << 3;
          tx_power = estimate_ue_tx_power(tbs, rb_table[rb_table_index], 0, Ncp, 0);   // fixme: set use_srs
        }

        first_rb_offset = UE_info->first_rb_offset[CC_id][slice_idx];
        available_rbs = N_RB_UL - 2 * first_rb[CC_id];

        while (tbs < bits_to_schedule &&
               rb_table[rb_table_index] < available_rbs &&
               UE_template->phr_info - tx_power > 0 &&
               rb_table_index < 32) {
          rb_table_index++;
          tbs = get_TBS_UL(UE_template->pre_assigned_mcs_ul, rb_table[rb_table_index]) << 3;
          tx_power = estimate_ue_tx_power(tbs, rb_table[rb_table_index], 0, Ncp, 0);
        }

        if (rb_table[rb_table_index] > (available_rbs - 1)) {
          rb_table_index--;
        }

        // 1 or 2 PRB with cqi enabled does not work well
        if (rb_table[rb_table_index] < 3) {
          rb_table_index = 2; //3PRB
        }

        UE_template->pre_allocated_rb_table_index_ul = rb_table_index;
        UE_template->pre_allocated_nb_rb_ul[slice_idx] = rb_table[rb_table_index];
        LOG_D(MAC, "[eNB %d] frame %d subframe %d: for UE %d CC %d: pre-assigned mcs %d, pre-allocated rb_table[%d]=%d RBs (phr %d, tx power %d)\n",
              module_idP,
              frameP,
              subframeP,
              UE_id,
              CC_id,
              UE_template->pre_assigned_mcs_ul,
              UE_template->pre_allocated_rb_table_index_ul,
              UE_template->pre_allocated_nb_rb_ul[slice_idx],
              UE_template->phr_info, tx_power);
      } else {
        /* if UE has pending scheduling request then pre-allocate 3 RBs */
        //if (UE_template->ul_active == 1 && UE_template->ul_SR == 1) {
        if (UE_is_to_be_scheduled(module_idP, CC_id, i)) {
          /* use QPSK mcs */
          UE_template->pre_assigned_mcs_ul = 10;
          UE_template->pre_allocated_rb_table_index_ul = 2;
          UE_template->pre_allocated_nb_rb_ul[slice_idx] = 3;
        } else {
          UE_template->pre_assigned_mcs_ul = 0;
          UE_template->pre_allocated_rb_table_index_ul = -1;
          UE_template->pre_allocated_nb_rb_ul[slice_idx] = 0;
        }
      }
    }
  }
}

struct sort_ue_ul_params {
  int module_idP;
  int sched_frameP;
  int sched_subframeP;
};

static int ue_ul_compare(const void *_a, const void *_b, void *_params) {
  struct sort_ue_ul_params *params = _params;
  UE_info_t *UE_info = &RC.mac[params->module_idP]->UE_info;
  int UE_id1 = *(const int *) _a;
  int UE_id2 = *(const int *) _b;
  int rnti1 = UE_RNTI(params->module_idP, UE_id1);
  int pCCid1 = UE_PCCID(params->module_idP, UE_id1);
  int round1 = maxround_ul(params->module_idP, rnti1, params->sched_frameP,
                           params->sched_subframeP);
  int rnti2 = UE_RNTI(params->module_idP, UE_id2);
  int pCCid2 = UE_PCCID(params->module_idP, UE_id2);
  int round2 = maxround_ul(params->module_idP, rnti2, params->sched_frameP,
                           params->sched_subframeP);

  if (round1 > round2)
    return -1;

  if (round1 < round2)
    return 1;

  if (UE_info->UE_template[pCCid1][UE_id1].ul_buffer_info[LCGID0] >
      UE_info->UE_template[pCCid2][UE_id2].ul_buffer_info[LCGID0])
    return -1;

  if (UE_info->UE_template[pCCid1][UE_id1].ul_buffer_info[LCGID0] <
      UE_info->UE_template[pCCid2][UE_id2].ul_buffer_info[LCGID0])
    return 1;

  int bytes_to_schedule1 = UE_info->UE_template[pCCid1][UE_id1].estimated_ul_buffer - UE_info->UE_template[pCCid1][UE_id1].scheduled_ul_bytes;

  if (bytes_to_schedule1 < 0) bytes_to_schedule1 = 0;

  int bytes_to_schedule2 = UE_info->UE_template[pCCid2][UE_id2].estimated_ul_buffer - UE_info->UE_template[pCCid2][UE_id2].scheduled_ul_bytes;

  if (bytes_to_schedule2 < 0) bytes_to_schedule2 = 0;

  if (bytes_to_schedule1 > bytes_to_schedule2)
    return -1;

  if (bytes_to_schedule1 < bytes_to_schedule2)
    return 1;

  if (UE_info->UE_template[pCCid1][UE_id1].pre_assigned_mcs_ul >
      UE_info->UE_template[pCCid2][UE_id2].pre_assigned_mcs_ul)
    return -1;

  if (UE_info->UE_template[pCCid1][UE_id1].pre_assigned_mcs_ul <
      UE_info->UE_template[pCCid2][UE_id2].pre_assigned_mcs_ul)
    return 1;

  if (UE_info->UE_sched_ctrl[UE_id1].cqi_req_timer >
      UE_info->UE_sched_ctrl[UE_id2].cqi_req_timer)
    return -1;

  if (UE_info->UE_sched_ctrl[UE_id1].cqi_req_timer <
      UE_info->UE_sched_ctrl[UE_id2].cqi_req_timer)
    return 1;

  return 0;
}

//-----------------------------------------------------------------------------
/*
 * This function sorts the UEs in order, depending on their ulsch buffer and CQI
 */
void sort_ue_ul(module_id_t module_idP,
                int slice_idx,
                int sched_frameP,
                sub_frame_t sched_subframeP,
                rnti_t *rntiTable)
//-----------------------------------------------------------------------------
{
  int list[MAX_MOBILES_PER_ENB];
  int list_size = 0;
  struct sort_ue_ul_params params = { module_idP, sched_frameP, sched_subframeP };
  UE_info_t *UE_info = &RC.mac[module_idP]->UE_info;
  UE_sched_ctrl_t *UE_scheduling_control = NULL;

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    UE_scheduling_control = &(UE_info->UE_sched_ctrl[i]);

    /* Check CDRX configuration and if UE is in active time for this subframe */
    if (UE_scheduling_control->cdrx_configured == TRUE) {
      if (UE_scheduling_control->in_active_time == FALSE) {
        continue;
      }
    }

    rntiTable[i] = UE_RNTI(module_idP, i);

    // Valid element and is not the actual CC_id in the list
    if (UE_info->active[i] == TRUE &&
        rntiTable[i] != NOT_A_RNTI &&
        UE_info->UE_sched_ctrl[i].ul_out_of_sync != 1 &&
        ue_ul_slice_membership(module_idP, i, slice_idx)) {
      list[list_size++] = i; // Add to list
    }
  }

  qsort_r(list, list_size, sizeof(int), ue_ul_compare, &params);

  if (list_size) { // At mimimum one list element
    for (int i = 0; i < list_size - 1; i++) {
      UE_info->list.next[list[i]] = list[i + 1];
    }

    UE_info->list.next[list[list_size - 1]] = -1;
    UE_info->list.head = list[0];
  } else { // No element
    UE_info->list.head = -1;
  }
}
