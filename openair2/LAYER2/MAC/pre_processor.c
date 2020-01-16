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

#define DEBUG_eNB_SCHEDULER 1
#define DEBUG_HEADER_PARSING 1


void
sort_ue_ul(module_id_t module_idP,
           int slice_idx,
           int sched_frameP,
           sub_frame_t sched_subframeP,
           rnti_t *rntiTable);

// This function stores the downlink buffer for all the logical channels
void
store_dlsch_buffer(module_id_t Mod_id,
                   int slice_idx,
                   frame_t frameP,
                   sub_frame_t subframeP) {
  int UE_id, lcid;
  rnti_t rnti;
  mac_rlc_status_resp_t rlc_status;
  UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
  UE_TEMPLATE *UE_template;

  for (UE_id = 0; UE_id < MAX_MOBILES_PER_ENB; UE_id++) {
    if (UE_list->active[UE_id] != TRUE)
      continue;

    if (!ue_dl_slice_membership(Mod_id, UE_id, slice_idx))
      continue;

    UE_template = &UE_list->UE_template[UE_PCCID(Mod_id, UE_id)][UE_id];
    // clear logical channel interface variables
    UE_template->dl_buffer_total = 0;
    UE_template->dl_pdus_total = 0;

    for (lcid = 0; lcid < MAX_NUM_LCID; ++lcid) {
      UE_template->dl_buffer_info[lcid] = 0;
      UE_template->dl_pdus_in_buffer[lcid] = 0;
      UE_template->dl_buffer_head_sdu_creation_time[lcid] = 0;
      UE_template->dl_buffer_head_sdu_remaining_size_to_send[lcid] = 0;
    }

    rnti = UE_RNTI(Mod_id, UE_id);

    for (lcid = 0; lcid < MAX_NUM_LCID; ++lcid) {    // loop over all the logical channels
      rlc_status = mac_rlc_status_ind(Mod_id, rnti, Mod_id, frameP, subframeP,
                                      ENB_FLAG_YES, MBMS_FLAG_NO, lcid, 0,0, 0
                                     );
      UE_template->dl_buffer_info[lcid] = rlc_status.bytes_in_buffer;    //storing the dlsch buffer for each logical channel
      UE_template->dl_pdus_in_buffer[lcid] = rlc_status.pdus_in_buffer;
      UE_template->dl_buffer_head_sdu_creation_time[lcid] = rlc_status.head_sdu_creation_time;
      UE_template->dl_buffer_head_sdu_creation_time_max =
        cmax(UE_template->dl_buffer_head_sdu_creation_time_max, rlc_status.head_sdu_creation_time);
      UE_template->dl_buffer_head_sdu_remaining_size_to_send[lcid] = rlc_status.head_sdu_remaining_size_to_send;
      UE_template->dl_buffer_head_sdu_is_segmented[lcid] = rlc_status.head_sdu_is_segmented;
      UE_template->dl_buffer_total += UE_template->dl_buffer_info[lcid];    //storing the total dlsch buffer
      UE_template->dl_pdus_total += UE_template->dl_pdus_in_buffer[lcid];
#ifdef DEBUG_eNB_SCHEDULER

      /* note for dl_buffer_head_sdu_remaining_size_to_send[lcid] :
       * 0 if head SDU has not been segmented (yet), else remaining size not already segmented and sent
       */
      if (UE_template->dl_buffer_info[lcid] > 0)
        LOG_D(MAC,
              "[eNB %d][SLICE %d] Frame %d Subframe %d : RLC status for UE %d in LCID%d: total of %d pdus and size %d, head sdu queuing time %d, remaining size %d, is segmeneted %d \n",
              Mod_id, RC.mac[Mod_id]->slice_info.dl[slice_idx].id, frameP,
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

int cqi2mcs(int cqi) {
  return cqi_to_mcs[cqi];
}

// This function returns the estimated number of RBs required by each UE for downlink scheduling
void
assign_rbs_required(module_id_t Mod_id,
                    int slice_idx,
                    frame_t frameP,
                    sub_frame_t subframe,
                    uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB]) {
  uint16_t TBS = 0;
  int UE_id, n, i, j, CC_id, pCCid, tmp;
  UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
  slice_info_t *sli = &RC.mac[Mod_id]->slice_info;
  eNB_UE_STATS *eNB_UE_stats, *eNB_UE_stats_i, *eNB_UE_stats_j;
  int N_RB_DL;

  // clear rb allocations across all CC_id
  for (UE_id = 0; UE_id < MAX_MOBILES_PER_ENB; UE_id++) {
    if (UE_list->active[UE_id] != TRUE) continue;

    if (!ue_dl_slice_membership(Mod_id, UE_id, slice_idx)) continue;

    pCCid = UE_PCCID(Mod_id, UE_id);

    //update CQI information across component carriers
    for (n = 0; n < UE_list->numactiveCCs[UE_id]; n++) {
      CC_id = UE_list->ordered_CCids[n][UE_id];
      eNB_UE_stats = &UE_list->eNB_UE_stats[CC_id][UE_id];
      //      eNB_UE_stats->dlsch_mcs1 = cmin(cqi_to_mcs[UE_list->UE_sched_ctrl[UE_id].dl_cqi[CC_id]], sli->dl[slice_idx].maxmcs);
      eNB_UE_stats->dlsch_mcs1 = cmin(cqi2mcs(UE_list->UE_sched_ctrl[UE_id].dl_cqi[CC_id]), sli->dl[slice_idx].maxmcs);
    }

    // provide the list of CCs sorted according to MCS
    for (i = 0; i < UE_list->numactiveCCs[UE_id]; ++i) {
      eNB_UE_stats_i = &UE_list->eNB_UE_stats[UE_list->ordered_CCids[i][UE_id]][UE_id];

      for (j = i + 1; j < UE_list->numactiveCCs[UE_id]; j++) {
        DevAssert(j < NFAPI_CC_MAX);
        eNB_UE_stats_j = &UE_list->eNB_UE_stats[UE_list->ordered_CCids[j][UE_id]][UE_id];

        if (eNB_UE_stats_j->dlsch_mcs1 > eNB_UE_stats_i->dlsch_mcs1) {
          tmp = UE_list->ordered_CCids[i][UE_id];
          UE_list->ordered_CCids[i][UE_id] = UE_list->ordered_CCids[j][UE_id];
          UE_list->ordered_CCids[j][UE_id] = tmp;
        }
      }
    }

    if (UE_list->UE_template[pCCid][UE_id].dl_buffer_total > 0) {
      LOG_D(MAC, "[preprocessor] assign RB for UE %d\n", UE_id);

      for (i = 0; i < UE_list->numactiveCCs[UE_id]; i++) {
        CC_id = UE_list->ordered_CCids[i][UE_id];
        eNB_UE_stats = &UE_list->eNB_UE_stats[CC_id][UE_id];
        const int min_rb_unit = get_min_rb_unit(Mod_id, CC_id);

        if (eNB_UE_stats->dlsch_mcs1 == 0) {
          nb_rbs_required[CC_id][UE_id] = 4;    // don't let the TBS get too small
        } else {
          nb_rbs_required[CC_id][UE_id] = min_rb_unit;
        }

        TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, nb_rbs_required[CC_id][UE_id]);
        LOG_D(MAC,
              "[preprocessor] start RB assignement for UE %d CC_id %d dl buffer %d (RB unit %d, MCS %d, TBS %d) \n",
              UE_id, CC_id,
              UE_list->UE_template[pCCid][UE_id].dl_buffer_total,
              nb_rbs_required[CC_id][UE_id],
              eNB_UE_stats->dlsch_mcs1, TBS);
        N_RB_DL = to_prb(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
        UE_list->UE_sched_ctrl[UE_id].max_rbs_allowed_slice[CC_id][slice_idx] =
          nb_rbs_allowed_slice(sli->dl[slice_idx].pct, N_RB_DL);

        /* calculating required number of RBs for each UE */
        while (TBS < UE_list->UE_template[pCCid][UE_id].dl_buffer_total) {
          nb_rbs_required[CC_id][UE_id] += min_rb_unit;

          if (nb_rbs_required[CC_id][UE_id] > UE_list->UE_sched_ctrl[UE_id].max_rbs_allowed_slice[CC_id][slice_idx]) {
            TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, UE_list->UE_sched_ctrl[UE_id].max_rbs_allowed_slice[CC_id][slice_idx]);
            nb_rbs_required[CC_id][UE_id] = UE_list->UE_sched_ctrl[UE_id].max_rbs_allowed_slice[CC_id][slice_idx];
            break;
          }

          TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, nb_rbs_required[CC_id][UE_id]);
        } // end of while

        LOG_D(MAC,
              "[eNB %d] Frame %d: UE %d on CC %d: RB unit %d,  nb_required RB %d (TBS %d, mcs %d)\n",
              Mod_id, frameP, UE_id, CC_id, min_rb_unit,
              nb_rbs_required[CC_id][UE_id], TBS,
              eNB_UE_stats->dlsch_mcs1);
        sli->pre_processor_results[slice_idx].mcs[CC_id][UE_id] = eNB_UE_stats->dlsch_mcs1;
      }
    }
  }
}


int
maxround_ul(module_id_t Mod_id, uint16_t rnti, int sched_frame,
            sub_frame_t sched_subframe) {
  uint8_t round, round_max = 0, UE_id;
  int CC_id, harq_pid;
  UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
  COMMON_channels_t *cc;

  for (CC_id = 0; CC_id < RC.nb_mac_CC[Mod_id]; CC_id++) {
    cc = &RC.mac[Mod_id]->common_channels[CC_id];
    UE_id = find_UE_id(Mod_id, rnti);

    if(UE_id == -1)
      continue;

    harq_pid = subframe2harqpid(cc, sched_frame, sched_subframe);
    round = UE_list->UE_sched_ctrl[UE_id].round_UL[CC_id][harq_pid];

    if (round > round_max) {
      round_max = round;
    }
  }

  return round_max;
}

void dlsch_scheduler_pre_processor_partitioning(module_id_t Mod_id,
    int slice_idx,
    const uint8_t rbs_retx[NFAPI_CC_MAX]) {
  int UE_id, CC_id, N_RB_DL, i;
  UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
  UE_sched_ctrl_t *ue_sched_ctl;
  uint16_t available_rbs;

  for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    if (UE_RNTI(Mod_id, UE_id) == NOT_A_RNTI) continue;

    if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1) continue;

    if (!ue_dl_slice_membership(Mod_id, UE_id, slice_idx)) continue;

    ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

    for (i = 0; i < UE_num_active_CC(UE_list, UE_id); ++i) {
      CC_id = UE_list->ordered_CCids[i][UE_id];
      N_RB_DL = to_prb(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
      available_rbs = nb_rbs_allowed_slice(RC.mac[Mod_id]->slice_info.dl[slice_idx].pct, N_RB_DL);

      if (rbs_retx[CC_id] < available_rbs)
        ue_sched_ctl->max_rbs_allowed_slice[CC_id][slice_idx] = available_rbs - rbs_retx[CC_id];
      else
        ue_sched_ctl->max_rbs_allowed_slice[CC_id][slice_idx] = 0;
    }
  }
}

void dlsch_scheduler_pre_processor_accounting(module_id_t Mod_id,
    int slice_idx,
    frame_t frameP,
    sub_frame_t subframeP,
    uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
    uint16_t nb_rbs_accounted[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB]) {
  int UE_id, CC_id;
  int i;
  rnti_t rnti;
  uint8_t harq_pid, round;
  uint16_t available_rbs[NFAPI_CC_MAX];
  uint8_t rbs_retx[NFAPI_CC_MAX];
  uint16_t average_rbs_per_user[NFAPI_CC_MAX];
  int total_ue_count[NFAPI_CC_MAX];
  int ue_count_newtx[NFAPI_CC_MAX];
  int ue_count_retx[NFAPI_CC_MAX];
  //uint8_t ue_retx_flag[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB];
  UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
  UE_sched_ctrl_t *ue_sched_ctl;
  COMMON_channels_t *cc;

  // Reset
  for (CC_id = 0; CC_id < RC.nb_mac_CC[Mod_id]; CC_id++) {
    total_ue_count[CC_id] = 0;
    ue_count_newtx[CC_id] = 0;
    ue_count_retx[CC_id] = 0;
    rbs_retx[CC_id] = 0;
    average_rbs_per_user[CC_id] = 0;
    available_rbs[CC_id] = 0;
    //for (UE_id = 0; UE_id < NFAPI_CC_MAX; ++UE_id) {
    //  ue_retx_flag[CC_id][UE_id] = 0;
    //}
  }

  // Find total UE count, and account the RBs required for retransmissions
  for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    rnti = UE_RNTI(Mod_id, UE_id);

    if (rnti == NOT_A_RNTI) continue;

    if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1) continue;

    if (!ue_dl_slice_membership(Mod_id, UE_id, slice_idx)) continue;

    for (i = 0; i < UE_num_active_CC(UE_list, UE_id); ++i) {
      CC_id = UE_list->ordered_CCids[i][UE_id];
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
      cc = &RC.mac[Mod_id]->common_channels[CC_id];
      harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config,frameP,subframeP);
      round = ue_sched_ctl->round[CC_id][harq_pid];

      if (nb_rbs_required[CC_id][UE_id] > 0) {
        total_ue_count[CC_id]++;
      }

      if (round != 8) {
        nb_rbs_required[CC_id][UE_id] = UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid];
        rbs_retx[CC_id] += nb_rbs_required[CC_id][UE_id];
        ue_count_retx[CC_id]++;
        //ue_retx_flag[CC_id][UE_id] = 1;
      } else {
        ue_count_newtx[CC_id]++;
      }
    }
  }

  // PARTITIONING
  // Reduces the available RBs according to slicing configuration
  dlsch_scheduler_pre_processor_partitioning(Mod_id, slice_idx, rbs_retx);

  for (CC_id = 0; CC_id < RC.nb_mac_CC[Mod_id]; ++CC_id) {
    if (UE_list->head < 0) continue; // no UEs in list

    // max_rbs_allowed_slice is saved in every UE, so take it from the first one
    ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_list->head];
    available_rbs[CC_id] = ue_sched_ctl->max_rbs_allowed_slice[CC_id][slice_idx];
  }

  switch (RC.mac[Mod_id]->slice_info.dl[slice_idx].accounting) {
    // If greedy scheduling, try to account all the required RBs
    case POL_GREEDY:
      for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
        rnti = UE_RNTI(Mod_id, UE_id);

        if (rnti == NOT_A_RNTI) continue;

        if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1) continue;

        if (!ue_dl_slice_membership(Mod_id, UE_id, slice_idx)) continue;

        for (i = 0; i < UE_num_active_CC(UE_list, UE_id); i++) {
          CC_id = UE_list->ordered_CCids[i][UE_id];

          if (available_rbs[CC_id] == 0) continue;

          nb_rbs_accounted[CC_id][UE_id] = cmin(nb_rbs_required[CC_id][UE_id], available_rbs[CC_id]);
          available_rbs[CC_id] -= nb_rbs_accounted[CC_id][UE_id];
        }
      }

      break;

    // Use the old, fair algorithm
    // Loop over all active UEs and account the avg number of RBs to each UE, based on all non-retx UEs.
    // case POL_FAIR:
    default:

      // FIXME: This is not ideal, why loop on UEs to find average_rbs_per_user[], that is per-CC?
      // TODO: Look how to loop on active CCs only without using the UE_num_active_CC() function.
      for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
        rnti = UE_RNTI(Mod_id, UE_id);

        if (rnti == NOT_A_RNTI) continue;

        if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1) continue;

        if (!ue_dl_slice_membership(Mod_id, UE_id, slice_idx)) continue;

        for (i = 0; i < UE_num_active_CC(UE_list, UE_id); ++i) {
          CC_id = UE_list->ordered_CCids[i][UE_id];
          ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
          available_rbs[CC_id] = ue_sched_ctl->max_rbs_allowed_slice[CC_id][slice_idx];
          const int min_rb_unit = get_min_rb_unit(Mod_id, CC_id);

          if (ue_count_newtx[CC_id] == 0) {
            average_rbs_per_user[CC_id] = 0;
          } else if (min_rb_unit*ue_count_newtx[CC_id] <= available_rbs[CC_id]) {
            average_rbs_per_user[CC_id] = (uint16_t)floor(available_rbs[CC_id]/ue_count_newtx[CC_id]);
          } else {
            // consider the total number of use that can be scheduled UE
            average_rbs_per_user[CC_id] = (uint16_t)min_rb_unit;
          }
        }
      }

      // note: nb_rbs_required is assigned according to total_buffer_dl
      // extend nb_rbs_required to capture per LCID RB required
      for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
        rnti = UE_RNTI(Mod_id, UE_id);

        if (rnti == NOT_A_RNTI) continue;

        if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1) continue;

        if (!ue_dl_slice_membership(Mod_id, UE_id, slice_idx)) continue;

        for (i = 0; i < UE_num_active_CC(UE_list, UE_id); i++) {
          CC_id = UE_list->ordered_CCids[i][UE_id];
          nb_rbs_accounted[CC_id][UE_id] = cmin(average_rbs_per_user[CC_id], nb_rbs_required[CC_id][UE_id]);
        }
      }

      break;
  }

  // Check retransmissions
  // TODO: Do this once at the beginning
  for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    rnti = UE_RNTI(Mod_id, UE_id);

    if (rnti == NOT_A_RNTI) continue;

    if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1) continue;

    if (!ue_dl_slice_membership(Mod_id, UE_id, slice_idx)) continue;

    for (i = 0; i < UE_num_active_CC(UE_list, UE_id); i++) {
      CC_id = UE_list->ordered_CCids[i][UE_id];
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
      cc = &RC.mac[Mod_id]->common_channels[CC_id];
      harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config,frameP,subframeP);
      round = ue_sched_ctl->round[CC_id][harq_pid];

      // control channel or retransmission
      /* TODO: do we have to check for retransmission? */
      if (mac_eNB_get_rrc_status(Mod_id, rnti) < RRC_RECONFIGURED || round != 8) {
        nb_rbs_accounted[CC_id][UE_id] = nb_rbs_required[CC_id][UE_id];
      }
    }
  }
}

void dlsch_scheduler_pre_processor_positioning(module_id_t Mod_id,
    int slice_idx,
    uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
    uint16_t nb_rbs_accounted[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
    uint16_t nb_rbs_remaining[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
    uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX]) {
  int UE_id, CC_id;
  int i;
  int N_RBG[NFAPI_CC_MAX];
  UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;

  for (CC_id = 0; CC_id < RC.nb_mac_CC[Mod_id]; CC_id++) {
    COMMON_channels_t *cc = &RC.mac[Mod_id]->common_channels[CC_id];
    N_RBG[CC_id] = to_rbg(cc->mib->message.dl_Bandwidth);
  }

  // Try to allocate accounted RBs
  for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    if (UE_RNTI(Mod_id, UE_id) == NOT_A_RNTI) continue;

    if (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 1) continue;

    if (!ue_dl_slice_membership(Mod_id, UE_id, slice_idx)) continue;

    for (i = 0; i < UE_num_active_CC(UE_list, UE_id); i++) {
      CC_id = UE_list->ordered_CCids[i][UE_id];
      nb_rbs_remaining[CC_id][UE_id] = nb_rbs_accounted[CC_id][UE_id];

      if (nb_rbs_required[CC_id][UE_id] > 0)
        LOG_D(MAC,
              "Step 1: nb_rbs_remaining[%d][%d]= %d (accounted %d, required %d,  pre_nb_available_rbs %d, N_RBG %d)\n",
              CC_id,
              UE_id,
              nb_rbs_remaining[CC_id][UE_id],
              nb_rbs_accounted[CC_id][UE_id],
              nb_rbs_required[CC_id][UE_id],
              UE_list->UE_sched_ctrl[UE_id].pre_nb_available_rbs[CC_id],
              N_RBG[CC_id]);

      LOG_T(MAC, "calling dlsch_scheduler_pre_processor_allocate .. \n ");
      dlsch_scheduler_pre_processor_allocate(Mod_id,
                                             UE_id,
                                             CC_id,
                                             N_RBG[CC_id],
                                             nb_rbs_required,
                                             nb_rbs_remaining,
                                             rballoc_sub);
    }
  }
}

// This function assigns pre-available RBS to each UE in specified sub-bands before scheduling is done
void
dlsch_scheduler_pre_processor(module_id_t Mod_id,
                              int CC_id,
                              frame_t frameP,
                              sub_frame_t subframeP) {
  int UE_id;
  uint16_t i, j;
  int slice_idx = 0;
  // TODO: remove NFAPI_CC_MAX, here for compatibility for the moment
  uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX];
  memset(rballoc_sub, 0, sizeof(rballoc_sub));

  eNB_MAC_INST *eNB = RC.mac[Mod_id];
  slice_info_t *sli = &eNB->slice_info;
  uint16_t (*nb_rbs_required)[MAX_MOBILES_PER_ENB]  = sli->pre_processor_results[slice_idx].nb_rbs_required;
  uint16_t (*nb_rbs_accounted)[MAX_MOBILES_PER_ENB] = sli->pre_processor_results[slice_idx].nb_rbs_accounted;
  uint16_t (*nb_rbs_remaining)[MAX_MOBILES_PER_ENB] = sli->pre_processor_results[slice_idx].nb_rbs_remaining;

  UE_list_t *UE_list = &eNB->UE_list;
  UE_sched_ctrl_t *ue_sched_ctl;

  // Initialize scheduling information for all active UEs
  memset(&sli->pre_processor_results[slice_idx], 0, sizeof(sli->pre_processor_results[slice_idx]));
  dlsch_scheduler_pre_processor_reset(Mod_id,
                                      CC_id,
                                      rballoc_sub[CC_id]);
  // STATUS
  // Store the DLSCH buffer for each logical channel
  store_dlsch_buffer(Mod_id,
                     slice_idx,
                     frameP,
                     subframeP);
  // Calculate the number of RBs required by each UE on the basis of logical channel's buffer
  assign_rbs_required(Mod_id,
                      slice_idx,
                      frameP,
                      subframeP,
                      nb_rbs_required);

  // ACCOUNTING
  // This procedure decides the number of RBs to allocate
  dlsch_scheduler_pre_processor_accounting(Mod_id,
                                           slice_idx,
                                           frameP,
                                           subframeP,
                                           nb_rbs_required,
                                           nb_rbs_accounted);
  // POSITIONING
  // This procedure does the main allocation of the RBs
  dlsch_scheduler_pre_processor_positioning(Mod_id,
                                            slice_idx,
                                            nb_rbs_required,
                                            nb_rbs_accounted,
                                            nb_rbs_remaining,
                                            rballoc_sub);


  for (UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

    for (i = 0; i < UE_num_active_CC(UE_list, UE_id); i++) {
      CC_id = UE_list->ordered_CCids[i][UE_id];
      //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].dl_pow_off = dl_pow_off[UE_id];
      COMMON_channels_t *cc = &eNB->common_channels[CC_id];
      int N_RBG = to_rbg(cc->mib->message.dl_Bandwidth);

      if (ue_sched_ctl->pre_nb_available_rbs[CC_id] > 0) {
        LOG_D(MAC, "******************DL Scheduling Information for UE%d ************************\n",
              UE_id);
        LOG_D(MAC, "dl power offset UE%d = %d \n",
              UE_id,
              ue_sched_ctl->dl_pow_off[CC_id]);
        LOG_D(MAC, "***********RB Alloc for every subband for UE%d ***********\n",
              UE_id);

        for (j = 0; j < N_RBG; j++) {
          //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].rballoc_sub[UE_id] = rballoc_sub_UE[CC_id][UE_id][UE_id];
          LOG_D(MAC, "RB Alloc for UE%d and Subband%d = %d\n",
                UE_id, j,
                ue_sched_ctl->rballoc_sub_UE[CC_id][j]);
        }

        //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].pre_nb_available_rbs = pre_nb_available_rbs[CC_id][UE_id];
        LOG_D(MAC, "[eNB %d][SLICE %d]Total RBs allocated for UE%d = %d\n",
              Mod_id,
              eNB->slice_info.dl[slice_idx].id,
              UE_id,
              ue_sched_ctl->pre_nb_available_rbs[CC_id]);
      }
    }
  }
}

void
dlsch_scheduler_pre_processor_reset(module_id_t module_idP,
                                    int CC_id,
                                    uint8_t rballoc_sub[N_RBG_MAX]) {
  UE_list_t *UE_list = &RC.mac[module_idP]->UE_list;

  for (int UE_id = 0; UE_id < MAX_MOBILES_PER_ENB; ++UE_id) {
    UE_sched_ctrl_t *ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
    const rnti_t rnti = UE_RNTI(module_idP, UE_id);

    if (rnti == NOT_A_RNTI)
      continue;

    if (UE_list->active[UE_id] != TRUE)
      continue;

    // initialize harq_pid and round
    if (ue_sched_ctl->ta_timer)
      ue_sched_ctl->ta_timer--;

    ue_sched_ctl->pre_nb_available_rbs[CC_id] = 0;
    ue_sched_ctl->dl_pow_off[CC_id] = 2;

    for (int i = 0; i < sizeof(ue_sched_ctl->rballoc_sub_UE[CC_id])/sizeof(unsigned char); i++) {
      ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 0;
    }
  }

  const int N_RB_DL = to_prb(RC.mac[module_idP]->common_channels[CC_id].mib->message.dl_Bandwidth);
  const int RBGsize = get_min_rb_unit(module_idP, CC_id);
  const uint8_t *vrb_map = RC.mac[module_idP]->common_channels[CC_id].vrb_map;

  // Check for SI-RNTI, RA-RNTI or P-RNTI allocations in VRB map and
  // initialize subbands accordly
  for (int i = 0; i < N_RB_DL; i++) {
    if (vrb_map[i] != 0)
      rballoc_sub[i/RBGsize] = 1;
  }
}


void
dlsch_scheduler_pre_processor_allocate(module_id_t Mod_id,
                                       int UE_id,
                                       uint8_t CC_id,
                                       int N_RBG,
                                       uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                       uint16_t nb_rbs_remaining[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
                                       uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX]) {
  int i;
  UE_list_t *UE_list = &RC.mac[Mod_id]->UE_list;
  UE_sched_ctrl_t *ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
  int N_RB_DL = to_prb(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
  const int min_rb_unit = get_min_rb_unit(Mod_id, CC_id);

  for (i = 0; i < N_RBG; i++) {
    if (rballoc_sub[CC_id][i] != 0) continue;

    if (ue_sched_ctl->rballoc_sub_UE[CC_id][i] != 0) continue;

    if (nb_rbs_remaining[CC_id][UE_id] <= 0) continue;

    if (ue_sched_ctl->pre_nb_available_rbs[CC_id] >= nb_rbs_required[CC_id][UE_id]) continue;

    if (ue_sched_ctl->dl_pow_off[CC_id] == 0) continue;

    if ((i == N_RBG - 1) && ((N_RB_DL == 25) || (N_RB_DL == 50))) {
      // Allocating last, smaller RBG
      if (nb_rbs_remaining[CC_id][UE_id] >= min_rb_unit - 1) {
        rballoc_sub[CC_id][i] = 1;
        ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 1;

        nb_rbs_remaining[CC_id][UE_id] = nb_rbs_remaining[CC_id][UE_id] - min_rb_unit + 1;
        ue_sched_ctl->pre_nb_available_rbs[CC_id] = ue_sched_ctl->pre_nb_available_rbs[CC_id] + min_rb_unit - 1;
      }
    } else {
      // Allocating a standard-sized RBG
      if (nb_rbs_remaining[CC_id][UE_id] >= min_rb_unit) {
        rballoc_sub[CC_id][i] = 1;
        ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 1;

        nb_rbs_remaining[CC_id][UE_id] = nb_rbs_remaining[CC_id][UE_id] - min_rb_unit;
        ue_sched_ctl->pre_nb_available_rbs[CC_id] = ue_sched_ctl->pre_nb_available_rbs[CC_id] + min_rb_unit;
      }
    }
  }
}


/// ULSCH PRE_PROCESSOR

void ulsch_scheduler_pre_processor(module_id_t module_idP,
                                   int slice_idx,
                                   int frameP,
                                   sub_frame_t subframeP,
                                   int sched_frameP,
                                   unsigned char sched_subframeP,
                                   uint16_t *first_rb) {
  int UE_id;
  uint16_t n;
  uint8_t CC_id, harq_pid;
  uint16_t nb_allocated_rbs[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB];
  uint16_t total_allocated_rbs[NFAPI_CC_MAX];
  uint16_t average_rbs_per_user[NFAPI_CC_MAX];
  int16_t total_remaining_rbs[NFAPI_CC_MAX];
  uint16_t total_ue_count[NFAPI_CC_MAX];
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_list_t *UE_list = &eNB->UE_list;
  slice_info_t *sli = &eNB->slice_info;
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
  for (UE_id = UE_list->head_ul; UE_id >= 0; UE_id = UE_list->next_ul[UE_id]) {
    // This is not the actual CC_id in the list
    for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      UE_template = &UE_list->UE_template[CC_id][UE_id];

      if (UE_template->pre_allocated_nb_rb_ul[slice_idx] > 0) {
        total_ue_count[CC_id]++;
      }
    }
  }

  // step 2: calculate the average rb per UE
  LOG_D(MAC, "In ulsch_preprocessor: step2 \n");

  for (UE_id = UE_list->head_ul; UE_id >= 0; UE_id = UE_list->next_ul[UE_id]) {
    if (UE_list->UE_template[CC_id][UE_id].rach_resource_type > 0) continue;

    LOG_D(MAC, "In ulsch_preprocessor: handling UE %d/%x\n",
          UE_id,
          rntiTable[UE_id]);

    for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
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
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
      ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_idx] =
        nb_rbs_allowed_slice(sli->ul[slice_idx].pct, N_RB_UL);
      first_rb_offset = UE_list->first_rb_offset[CC_id][slice_idx];
      available_rbs =
        cmin(ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_idx], N_RB_UL - first_rb[CC_id] - first_rb_offset);

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
  for (UE_id = UE_list->head_ul; UE_id >= 0; UE_id = UE_list->next_ul[UE_id]) {
    // if (continueTable[UE_id]) continue;
    for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      UE_template = &UE_list->UE_template[CC_id][UE_id];
      harq_pid = subframe2harqpid(&RC.mac[module_idP]->common_channels[CC_id],
                                  sched_frameP, sched_subframeP);

      //      mac_xface->get_ue_active_harq_pid(module_idP,CC_id,rnti,frameP,subframeP,&harq_pid,&round,openair_harq_UL);

      if (UE_list->UE_sched_ctrl[UE_id].round_UL[CC_id][harq_pid] > 0) {
        nb_allocated_rbs[CC_id][UE_id] = UE_list->UE_template[CC_id][UE_id].nb_rb_ul[harq_pid];
      } else {
        nb_allocated_rbs[CC_id][UE_id] =
          cmin(UE_list->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul[slice_idx], average_rbs_per_user[CC_id]);
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
  for (UE_id = UE_list->head_ul; UE_id >= 0; UE_id = UE_list->next_ul[UE_id]) {
    // if (continueTable[UE_id]) continue;
    ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];

    for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      UE_template = &UE_list->UE_template[CC_id][UE_id];
      N_RB_UL = to_prb(eNB->common_channels[CC_id].ul_Bandwidth);
      first_rb_offset = UE_list->first_rb_offset[CC_id][slice_idx];
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
  UE_list_t *UE_list = &eNB->UE_list;
  slice_info_t *sli = &eNB->slice_info;
  UE_TEMPLATE *UE_template;
  UE_sched_ctrl_t *ue_sched_ctl;
  int Ncp;
  int N_RB_UL;
  int first_rb_offset, available_rbs;

  for (i = UE_list->head_ul; i >= 0; i = UE_list->next_ul[i]) {
    if (UE_list->UE_sched_ctrl[i].phr_received == 1) {
      /* if we've received the power headroom information the UE, we can go to
       * maximum mcs */
      mcs = cmin(20, sli->ul[slice_idx].maxmcs);
    } else {
      /* otherwise, limit to QPSK PUSCH */
      mcs = cmin(10, sli->ul[slice_idx].maxmcs);
    }

    UE_id = i;

    for (n = 0; n < UE_list->numactiveULCCs[UE_id]; n++) {
      // This is the actual CC_id in the list
      CC_id = UE_list->ordered_ULCCids[n][UE_id];
      AssertFatal(CC_id < RC.nb_mac_CC[module_idP], "CC_id %u should be < %u, loop n=%u < numactiveULCCs[%u]=%u",
                  CC_id,
                  NFAPI_CC_MAX,
                  n,
                  UE_id,
                  UE_list->numactiveULCCs[UE_id]);
      UE_template = &UE_list->UE_template[CC_id][UE_id];
      UE_template->pre_assigned_mcs_ul = mcs;
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
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

        first_rb_offset = UE_list->first_rb_offset[CC_id][slice_idx];
        available_rbs =
          cmin(ue_sched_ctl->max_rbs_allowed_slice_uplink[CC_id][slice_idx], N_RB_UL - first_rb[CC_id] - first_rb_offset);

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
  UE_list_t *UE_list = &RC.mac[params->module_idP]->UE_list;
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

  if (UE_list->UE_sched_ctrl[UE_id1].cqi_req_timer >
      UE_list->UE_sched_ctrl[UE_id2].cqi_req_timer)
    return -1;

  if (UE_list->UE_sched_ctrl[UE_id1].cqi_req_timer <
      UE_list->UE_sched_ctrl[UE_id2].cqi_req_timer)
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
  UE_list_t *UE_list = &RC.mac[module_idP]->UE_list;
  UE_sched_ctrl_t *UE_scheduling_control = NULL;

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    UE_scheduling_control = &(UE_list->UE_sched_ctrl[i]);

    /* Check CDRX configuration and if UE is in active time for this subframe */
    if (UE_scheduling_control->cdrx_configured == TRUE) {
      if (UE_scheduling_control->in_active_time == FALSE) {
        continue;
      }
    }

    rntiTable[i] = UE_RNTI(module_idP, i);

    // Valid element and is not the actual CC_id in the list
    if (UE_list->active[i] == TRUE &&
        rntiTable[i] != NOT_A_RNTI &&
        UE_list->UE_sched_ctrl[i].ul_out_of_sync != 1 &&
        ue_ul_slice_membership(module_idP, i, slice_idx)) {
      list[list_size++] = i; // Add to list
    }
  }

  qsort_r(list, list_size, sizeof(int), ue_ul_compare, &params);

  if (list_size) { // At mimimum one list element
    for (int i = 0; i < list_size - 1; i++) {
      UE_list->next_ul[list[i]] = list[i + 1];
    }

    UE_list->next_ul[list[list_size - 1]] = -1;
    UE_list->head_ul = list[0];
  } else { // No element
    UE_list->head_ul = -1;
  }
}
