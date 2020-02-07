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
  uint16_t nb_allocated_rbs[MAX_MOBILES_PER_ENB];
  uint16_t total_allocated_rbs = 0;
  uint16_t average_rbs_per_user = 0;
  int16_t total_remaining_rbs = 0;
  uint16_t total_ue_count = 0;
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_info_t *UE_info = &eNB->UE_info;
  const int N_RB_UL = to_prb(eNB->common_channels[CC_id].ul_Bandwidth);
  uint16_t available_rbs = N_RB_UL - 2 * first_rb[CC_id]; // top and bottom // - UE_info->first_rb_offset[CC_id];

  // maximize MCS and then allocate required RB according to the buffer occupancy with the limit of max available UL RB
  LOG_D(MAC, "In ulsch_preprocessor: assign max mcs min rb\n");
  assign_max_mcs_min_rb(module_idP, CC_id, frameP, subframeP, first_rb);

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    if (UE_info->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul > 0) {
      total_ue_count++;
    }
  }

  if (total_ue_count == 0)
    average_rbs_per_user = 0;
  else if (total_ue_count == 1)
    average_rbs_per_user = available_rbs + 1;
  else if (total_ue_count <= available_rbs)
    average_rbs_per_user = (uint16_t) floor(available_rbs / total_ue_count);
  else
    average_rbs_per_user = 1;

  if (total_ue_count > 0)
    LOG_D(MAC, "[eNB %d] Frame %d subframe %d: total ue to be scheduled %d\n",
          module_idP,
          frameP,
          subframeP,
          total_ue_count);

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    uint8_t harq_pid = subframe2harqpid(&RC.mac[module_idP]->common_channels[CC_id],
                                        sched_frameP, sched_subframeP);

    if (UE_info->UE_sched_ctrl[UE_id].round_UL[CC_id][harq_pid] > 0)
      nb_allocated_rbs[UE_id] = UE_info->UE_template[CC_id][UE_id].nb_rb_ul[harq_pid];
    else
      nb_allocated_rbs[UE_id] = cmin(UE_info->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul, average_rbs_per_user);

    total_allocated_rbs += nb_allocated_rbs[UE_id];
    LOG_D(MAC, "In ulsch_preprocessor: assigning %d RBs for UE %d CCid %d, harq_pid %d\n",
          nb_allocated_rbs[UE_id],
          UE_id,
          CC_id,
          harq_pid);
  }

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    UE_TEMPLATE *UE_template = &UE_info->UE_template[CC_id][UE_id];
    total_remaining_rbs = available_rbs - total_allocated_rbs;

    /* TODO this has already been accounted for - do we need it again? */
    //if (total_ue_count == 1)
    //  total_remaining_rbs++;

    while (UE_template->pre_allocated_nb_rb_ul > 0 &&
           nb_allocated_rbs[UE_id] < UE_template->pre_allocated_nb_rb_ul &&
           total_remaining_rbs > 0) {
      nb_allocated_rbs[UE_id] = cmin(nb_allocated_rbs[UE_id] + 1, UE_template->pre_allocated_nb_rb_ul);
      total_remaining_rbs--;
      total_allocated_rbs++;
    }

    UE_template->pre_allocated_nb_rb_ul = nb_allocated_rbs[UE_id];
    LOG_D(MAC, "******************UL Scheduling Information for UE%d CC_id %d ************************\n",
          UE_id,
          CC_id);
    LOG_D(MAC, "[eNB %d] total RB allocated for UE%d CC_id %d  = %d\n",
          module_idP,
          UE_id,
          CC_id,
          UE_template->pre_allocated_nb_rb_ul);
  }
}

void calculate_max_mcs_min_rb(module_id_t mod_id,
                              int CC_id,
                              int bytes,
                              int phr,
                              int max_mcs,
                              int *mcs,
                              int max_rbs,
                              int *rb_index,
                              int *tx_power) {
  const int Ncp = RC.mac[mod_id]->common_channels[CC_id].Ncp;
  /* TODO shouldn't we consider the SRS or other quality indicators? */
  *mcs = max_mcs;
  *rb_index = 2;
  int tbs = get_TBS_UL(*mcs, rb_table[*rb_index]);

  // fixme: set use_srs flag
  *tx_power = estimate_ue_tx_power(tbs * 8, rb_table[*rb_index], 0, Ncp, 0);

  /* find maximum MCS */
  while ((phr - *tx_power < 0 || tbs > bytes) && *mcs > 3) {
    mcs--;
    tbs = get_TBS_UL(*mcs, rb_table[*rb_index]);
    *tx_power = estimate_ue_tx_power(tbs * 8, rb_table[*rb_index], 0, Ncp, 0);
  }

  /* find minimum necessary RBs */
  while (tbs < bytes
         && *rb_index < 32
         && rb_table[*rb_index] < max_rbs
         && phr - *tx_power > 0) {
    (*rb_index)++;
    tbs = get_TBS_UL(*mcs, rb_table[*rb_index]);
    *tx_power = estimate_ue_tx_power(tbs * 8, rb_table[*rb_index], 0, Ncp, 0);
  }

  /* Decrease if we went to far in last iteration */
  if (rb_table[*rb_index] > max_rbs)
    (*rb_index)--;

  // 1 or 2 PRB with cqi enabled does not work well
  if (rb_table[*rb_index] < 3) {
    *rb_index = 2; //3PRB
  }
}

void
assign_max_mcs_min_rb(module_id_t module_idP,
                      int CC_id,
                      int frameP,
                      sub_frame_t subframeP,
                      uint16_t *first_rb) {
  const int N_RB_UL = to_prb(RC.mac[module_idP]->common_channels[CC_id].ul_Bandwidth);
  const int available_rbs = N_RB_UL - 2 * first_rb[CC_id]; // top and bottom - UE_info->first_rb_offset[CC_id];
  const int Ncp = RC.mac[module_idP]->common_channels[CC_id].Ncp;
  UE_info_t *UE_info = &RC.mac[module_idP]->UE_info;

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    UE_TEMPLATE *UE_template = &UE_info->UE_template[CC_id][UE_id];

    const int B = cmax(UE_template->estimated_ul_buffer - UE_template->scheduled_ul_bytes, 0);

    const int UE_to_be_scheduled = UE_is_to_be_scheduled(module_idP, CC_id, UE_id);
    if (B == 0 && !UE_to_be_scheduled)
      continue;

    /* if UE has pending scheduling request then pre-allocate 3 RBs */
    if (B == 0 && UE_to_be_scheduled) {
      UE_template->pre_assigned_mcs_ul = 10; /* use QPSK mcs only */
      UE_template->pre_allocated_rb_table_index_ul = 2;
      UE_template->pre_allocated_nb_rb_ul = 3;
      continue;
    }

    int mcs;
    int rb_table_index;
    int tx_power;
    calculate_max_mcs_min_rb(
        module_idP,
        CC_id,
        B,
        UE_template->phr_info,
        UE_info->UE_sched_ctrl[UE_id].phr_received == 1 ? 20 : 10,
        &mcs,
        available_rbs,
        &rb_table_index,
        &tx_power);

    UE_template->pre_assigned_mcs_ul = mcs;
    UE_template->pre_allocated_rb_table_index_ul = rb_table_index;
    UE_template->pre_allocated_nb_rb_ul = rb_table[rb_table_index];
    LOG_D(MAC, "[eNB %d] frame %d subframe %d: for UE %d CC %d: pre-assigned mcs %d, pre-allocated rb_table[%d]=%d RBs (phr %d, tx power %d)\n",
          module_idP,
          frameP,
          subframeP,
          UE_id,
          CC_id,
          UE_template->pre_assigned_mcs_ul,
          UE_template->pre_allocated_rb_table_index_ul,
          UE_template->pre_allocated_nb_rb_ul,
          UE_template->phr_info, tx_power);
  }
}
