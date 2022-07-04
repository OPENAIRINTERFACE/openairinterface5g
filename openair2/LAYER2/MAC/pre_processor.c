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
#include <openair2/LAYER2/NR_MAC_COMMON/nr_mac_extern.h>
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

int get_rbg_size_last(module_id_t Mod_id, int CC_id) {
  const int RBGsize = get_min_rb_unit(Mod_id, CC_id);
  const int N_RB_DL = to_prb(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
  if (N_RB_DL == 15 || N_RB_DL == 25 || N_RB_DL == 50 || N_RB_DL == 75)
    return RBGsize - 1;
  else
    return RBGsize;
}

bool try_allocate_harq_retransmission(module_id_t Mod_id,
                                      int CC_id,
                                      int frame,
                                      int subframe,
                                      int UE_id,
                                      int start_rbg,
                                      int *n_rbg_sched,
                                      uint8_t *rbgalloc_mask) {
  const int N_RBG = to_rbg(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
  const int RBGsize = get_min_rb_unit(Mod_id, CC_id);
  const int RBGlastsize = get_rbg_size_last(Mod_id, CC_id);
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;
  // check whether there are HARQ retransmissions
  const COMMON_channels_t *cc = &RC.mac[Mod_id]->common_channels[CC_id];
  const uint8_t harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config, frame, subframe);
  UE_sched_ctrl_t *ue_ctrl = &UE_info->UE_sched_ctrl[UE_id];
  // retransmission: allocate
  const int nb_rb = UE_info->UE_template[CC_id][UE_id].nb_rb[harq_pid];
  if (nb_rb == 0) {
    return false;
  }
  int nb_rbg = (nb_rb + (nb_rb % RBGsize)) / RBGsize;
  // needs more RBGs than we can allocate
  if (nb_rbg > *n_rbg_sched) {
    LOG_D(MAC,
          "retransmission of UE %d needs more RBGs (%d) than we have (%d)\n",
          UE_id, nb_rbg, *n_rbg_sched);
    return false;
  }
  // ensure that the number of RBs can be contained by the RBGs (!), i.e.
  // if we allocate the last RBG this one should have the full RBGsize
  if ((nb_rb % RBGsize) == 0 && nb_rbg == *n_rbg_sched
      && rbgalloc_mask[N_RBG - 1] && RBGlastsize != RBGsize) {
    LOG_D(MAC,
          "retransmission of UE %d needs %d RBs, but the last RBG %d is too small (%d, normal %d)\n",
          UE_id, nb_rb, N_RBG - 1, RBGlastsize, RBGsize);
    return false;
  }
  const uint8_t cqi = ue_ctrl->dl_cqi[CC_id];
  const int idx = CCE_try_allocate_dlsch(Mod_id, CC_id, subframe, UE_id, cqi);
  if (idx < 0) { // cannot allocate CCE
    LOG_D(MAC, "cannot allocate UE %d: no CCE can be allocated\n", UE_id);
    return false;
  }
  /* if nb_rb is not multiple of RBGsize, then last RBG must be free
   * (it will be allocated just below)
   */
  if (nb_rb % RBGsize && !rbgalloc_mask[N_RBG-1]) {
    LOG_E(MAC, "retransmission: last RBG already allocated (this should not happen)\n");
    return false;
  }
  ue_ctrl->pre_dci_dl_pdu_idx = idx;
  // retransmissions: directly allocate
  *n_rbg_sched -= nb_rbg;
  ue_ctrl->pre_nb_available_rbs[CC_id] += nb_rb;
  if (nb_rb % RBGsize) {
    /* special case: if nb_rb is not multiple of RBGsize, then allocate last RBG.
     * If we instead allocated another RBG then we will retransmit with more
     * RBs and the UE will not accept it.
     * (This has been seen in a test with cots UEs, if not true, then change
     * code as needed.)
     * At this point rbgalloc_mask[N_RBG-1] == 1 due to the test above.
     */
    ue_ctrl->rballoc_sub_UE[CC_id][N_RBG-1] = 1;
    rbgalloc_mask[N_RBG-1] = 0;
    nb_rbg--;
  }
  for (; nb_rbg > 0; start_rbg++) {
    if (!rbgalloc_mask[start_rbg])
      continue;
    ue_ctrl->rballoc_sub_UE[CC_id][start_rbg] = 1;
    rbgalloc_mask[start_rbg] = 0;
    nb_rbg--;
  }
  LOG_D(MAC,
        "%4d.%d n_rbg_sched %d after retransmission reservation for UE %d "
        "retx nb_rb %d pre_nb_available_rbs %d\n",
        frame, subframe, *n_rbg_sched, UE_id,
        UE_info->UE_template[CC_id][UE_id].nb_rb[harq_pid],
        ue_ctrl->pre_nb_available_rbs[CC_id]);
  return true;
}

void *rr_dl_setup(void) {
  void *data = malloc(sizeof(int));
  *(int *) data = 0;
  AssertFatal(data, "could not allocate data in %s()\n", __func__);
  return data;
}
void rr_dl_unset(void **data) {
  if (*data)
    free(*data);
  *data = NULL;
}
int rr_dl_run(module_id_t Mod_id,
              int CC_id,
              int frame,
              int subframe,
              UE_list_t *UE_list,
              int max_num_ue,
              int n_rbg_sched,
              uint8_t *rbgalloc_mask,
              void *data) {
  DevAssert(UE_list->head >= 0);
  DevAssert(n_rbg_sched > 0);
  const int N_RBG = to_rbg(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
  const int RBGsize = get_min_rb_unit(Mod_id, CC_id);
  const int RBGlastsize = get_rbg_size_last(Mod_id, CC_id);
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;

  int rbg = 0;
  for (; !rbgalloc_mask[rbg]; rbg++)
    ; /* fast-forward to first allowed RBG */

  /* just start with the UE after the one we had last time. If it does not
   * exist, this will start at the head */
  int *start_ue = data;
  *start_ue = next_ue_list_looped(UE_list, *start_ue);

  int UE_id = *start_ue;
  UE_list_t UE_sched;
  int *cur_UE = &UE_sched.head;
  // Allocate retransmissions, and mark UEs with new transmissions
  do {
    // check whether there are HARQ retransmissions
    const COMMON_channels_t *cc = &RC.mac[Mod_id]->common_channels[CC_id];
    const uint8_t harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config, frame, subframe);
    UE_sched_ctrl_t *ue_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    const uint8_t round = ue_ctrl->round[CC_id][harq_pid];
    if (round != 8) {
      bool r = try_allocate_harq_retransmission(Mod_id, CC_id, frame, subframe,
                                                UE_id, rbg, &n_rbg_sched,
                                                rbgalloc_mask);
      if (r) {
        /* if there are no more RBG to give, return */
        if (n_rbg_sched <= 0)
          return 0;
        max_num_ue--;
        if (max_num_ue == 0)
          return n_rbg_sched;
        for (; !rbgalloc_mask[rbg]; rbg++) /* fast-forward */ ;
      }
    } else {
      if (UE_info->UE_template[CC_id][UE_id].dl_buffer_total > 0) {
        *cur_UE = UE_id;
        cur_UE = &UE_sched.next[UE_id];
      }
    }
    UE_id = next_ue_list_looped(UE_list, UE_id);
  } while (UE_id != *start_ue);
  *cur_UE = -1; // mark end

  if (UE_sched.head < 0)
    return n_rbg_sched; // no UE has a transmission

  // after allocating retransmissions: pre-allocate CCE, compute number of
  // requested RBGs
  max_num_ue = min(max_num_ue, n_rbg_sched);
  int rb_required[MAX_MOBILES_PER_ENB]; // how much UEs request
  cur_UE = &UE_sched.head;
  while (*cur_UE >= 0 && max_num_ue > 0) {
    const int UE_id = *cur_UE;
    const uint8_t cqi = UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id];
    const int idx = CCE_try_allocate_dlsch(Mod_id, CC_id, subframe, UE_id, cqi);
    if (idx < 0) {
      LOG_D(MAC, "cannot allocate CCE for UE %d, skipping\n", UE_id);
      // SKIP this UE in the list by marking the next as the current
      *cur_UE = UE_sched.next[UE_id];
      continue;
    }
    UE_info->UE_sched_ctrl[UE_id].pre_dci_dl_pdu_idx = idx;
    const int mcs = cqi_to_mcs[cqi];
    UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1 = mcs;
    const uint32_t B = UE_info->UE_template[CC_id][UE_id].dl_buffer_total;
    rb_required[UE_id] = find_nb_rb_DL(mcs, B, n_rbg_sched * RBGsize, RBGsize);
    max_num_ue--;
    cur_UE = &UE_sched.next[UE_id]; // go to next
  }
  *cur_UE = -1; // not all UEs might be allocated, mark end

  /* for one UE after the next: allocate resources */
  cur_UE = &UE_sched.head;
  while (*cur_UE >= 0) {
    const int UE_id = *cur_UE;
    UE_sched_ctrl_t *ue_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    ue_ctrl->rballoc_sub_UE[CC_id][rbg] = 1;
    rbgalloc_mask[rbg] = 0;
    const int sRBG = rbg == N_RBG - 1 ? RBGlastsize : RBGsize;
    ue_ctrl->pre_nb_available_rbs[CC_id] += sRBG;
    rb_required[UE_id] -= sRBG;
    if (rb_required[UE_id] <= 0) {
      *cur_UE = UE_sched.next[*cur_UE];
      if (*cur_UE < 0)
        cur_UE = &UE_sched.head;
    } else {
      cur_UE = UE_sched.next[*cur_UE] < 0 ? &UE_sched.head : &UE_sched.next[*cur_UE];
    }
    n_rbg_sched--;
    if (n_rbg_sched <= 0)
      break;
    for (rbg++; !rbgalloc_mask[rbg]; rbg++) /* fast-forward */ ;
  }

  return n_rbg_sched;
}
default_sched_dl_algo_t round_robin_dl = {
  .name  = "round_robin_dl",
  .setup = rr_dl_setup,
  .unset = rr_dl_unset,
  .run   = rr_dl_run,
  .data  = NULL
};


void *pf_dl_setup(void) {
  void *data = calloc(MAX_MOBILES_PER_ENB, sizeof(float));
  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++)
    *(float *) data = 0.0f;
  AssertFatal(data, "could not allocate data in %s()\n", __func__);
  return data;
}
void pf_dl_unset(void **data) {
  if (*data)
    free(*data);
  *data = NULL;
}
int pf_wbcqi_dl_run(module_id_t Mod_id,
                    int CC_id,
                    int frame,
                    int subframe,
                    UE_list_t *UE_list,
                    int max_num_ue,
                    int n_rbg_sched,
                    uint8_t *rbgalloc_mask,
                    void *data) {
  DevAssert(UE_list->head >= 0);
  DevAssert(n_rbg_sched > 0);
  const int N_RBG = to_rbg(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
  const int RBGsize = get_min_rb_unit(Mod_id, CC_id);
  const int RBGlastsize = get_rbg_size_last(Mod_id, CC_id);
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;

  int rbg = 0;
  for (; !rbgalloc_mask[rbg]; rbg++)
    ; /* fast-forward to first allowed RBG */

  UE_list_t UE_sched; // UEs that could be scheduled
  int *uep = &UE_sched.head;
  float *thr_ue = data;
  float coeff_ue[MAX_MOBILES_PER_ENB];

  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    const float a = 0.0005f; // corresponds to 200ms window
    const uint32_t b = UE_info->eNB_UE_stats[CC_id][UE_id].TBS;
    thr_ue[UE_id] = (1 - a) * thr_ue[UE_id] + a * b;

    // check whether there are HARQ retransmissions
    const COMMON_channels_t *cc = &RC.mac[Mod_id]->common_channels[CC_id];
    const uint8_t harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config, frame, subframe);
    UE_sched_ctrl_t *ue_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    const uint8_t round = ue_ctrl->round[CC_id][harq_pid];
    if (round != 8) {
      bool r = try_allocate_harq_retransmission(Mod_id, CC_id, frame, subframe,
                                                UE_id, rbg, &n_rbg_sched,
                                                rbgalloc_mask);
      if (r) {
        /* if there are no more RBG to give, return */
        if (n_rbg_sched <= 0)
          return 0;
        max_num_ue--;
        if (max_num_ue == 0)
          return n_rbg_sched;
        for (; !rbgalloc_mask[rbg]; rbg++) /* fast-forward */ ;
      }
    } else {
      if (UE_info->UE_template[CC_id][UE_id].dl_buffer_total == 0)
        continue;

      const int mcs = cqi_to_mcs[UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id]];
      const uint32_t tbs = get_TBS_DL(mcs, RBGsize);
      coeff_ue[UE_id] = (float) tbs / thr_ue[UE_id];
      //LOG_I(MAC, "    pf UE %d: old TBS %d thr %f MCS %d TBS %d coeff %f\n",
      //      UE_id, b, thr_ue[UE_id], mcs, tbs, coeff_ue[UE_id]);
      *uep = UE_id;
      uep = &UE_sched.next[UE_id];
    }
  }
  *uep = -1;

  while (max_num_ue > 0 && n_rbg_sched > 0 && UE_sched.head >= 0) {
    int *max = &UE_sched.head; /* assume head is max */
    int *p = &UE_sched.next[*max];
    while (*p >= 0) {
      /* if the current one has larger coeff, save for later */
      if (coeff_ue[*p] > coeff_ue[*max])
        max = p;
      p = &UE_sched.next[*p];
    }
    /* remove the max one */
    const int UE_id = *max;
    p = &UE_sched.next[*max];
    *max = UE_sched.next[*max];
    *p = -1;

    const uint8_t cqi = UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id];
    const int idx = CCE_try_allocate_dlsch(Mod_id, CC_id, subframe, UE_id, cqi);
    if (idx < 0)
      continue;
    UE_info->UE_sched_ctrl[UE_id].pre_dci_dl_pdu_idx = idx;

    max_num_ue--;

    /* allocate as much as possible */
    const int mcs = cqi_to_mcs[cqi];
    UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1 = mcs;
    int req = find_nb_rb_DL(mcs,
                            UE_info->UE_template[CC_id][UE_id].dl_buffer_total,
                            n_rbg_sched * RBGsize,
                            RBGsize);
    UE_sched_ctrl_t *ue_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    while (req > 0 && n_rbg_sched > 0) {
      ue_ctrl->rballoc_sub_UE[CC_id][rbg] = 1;
      rbgalloc_mask[rbg] = 0;
      const int sRBG = rbg == N_RBG - 1 ? RBGlastsize : RBGsize;
      ue_ctrl->pre_nb_available_rbs[CC_id] += sRBG;
      req -= sRBG;
      n_rbg_sched--;
      for (rbg++; n_rbg_sched > 0 && !rbgalloc_mask[rbg]; rbg++) /* fast-forward */ ;
    }
  }

  return n_rbg_sched;
}
default_sched_dl_algo_t proportional_fair_wbcqi_dl = {
  .name  = "proportional_fair_wbcqi_dl",
  .setup = pf_dl_setup,
  .unset = pf_dl_unset,
  .run   = pf_wbcqi_dl_run,
  .data  = NULL
};

void *mt_dl_setup(void) {
  return NULL;
}
void mt_dl_unset(void **data) {
  *data = NULL;
}
int mt_wbcqi_dl_run(module_id_t Mod_id,
                    int CC_id,
                    int frame,
                    int subframe,
                    UE_list_t *UE_list,
                    int max_num_ue,
                    int n_rbg_sched,
                    uint8_t *rbgalloc_mask,
                    void *data) {
  DevAssert(UE_list->head >= 0);
  DevAssert(n_rbg_sched > 0);
  const int N_RBG = to_rbg(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
  const int RBGsize = get_min_rb_unit(Mod_id, CC_id);
  const int RBGlastsize = get_rbg_size_last(Mod_id, CC_id);
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;

  int rbg = 0;
  for (; !rbgalloc_mask[rbg]; rbg++)
    ; /* fast-forward to first allowed RBG */

  UE_list_t UE_sched; // UEs that could be scheduled
  int *uep = &UE_sched.head;

  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    // check whether there are HARQ retransmissions
    const COMMON_channels_t *cc = &RC.mac[Mod_id]->common_channels[CC_id];
    const uint8_t harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config, frame, subframe);
    UE_sched_ctrl_t *ue_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    const uint8_t round = ue_ctrl->round[CC_id][harq_pid];
    if (round != 8) {
      bool r = try_allocate_harq_retransmission(Mod_id, CC_id, frame, subframe,
                                                UE_id, rbg, &n_rbg_sched,
                                                rbgalloc_mask);
      if (r) {
        /* if there are no more RBG to give, return */
        if (n_rbg_sched <= 0)
          return 0;
        max_num_ue--;
        if (max_num_ue == 0)
          return n_rbg_sched;
        for (; !rbgalloc_mask[rbg]; rbg++) /* fast-forward */ ;
      }
    } else {
      if (UE_info->UE_template[CC_id][UE_id].dl_buffer_total == 0)
        continue;
      *uep = UE_id;
      uep = &UE_sched.next[UE_id];
    }
  }
  *uep = -1;

  while (max_num_ue > 0 && n_rbg_sched > 0 && UE_sched.head >= 0) {
    int *max = &UE_sched.head; /* assume head is max */
    int *p = &UE_sched.next[*max];
    while (*p >= 0) {
      /* if the current one has better CQI, or the same and more data */
      const uint8_t maxCqi = UE_info->UE_sched_ctrl[*max].dl_cqi[CC_id];
      const uint32_t maxB = UE_info->UE_template[CC_id][*max].dl_buffer_total;
      const uint8_t pCqi = UE_info->UE_sched_ctrl[*p].dl_cqi[CC_id];
      const uint32_t pB = UE_info->UE_template[CC_id][*p].dl_buffer_total;
      if (pCqi > maxCqi || (pCqi == maxCqi && pB > maxB))
        max = p;
      p = &UE_sched.next[*p];
    }
    /* remove the max one */
    const int UE_id = *max;
    p = &UE_sched.next[*max];
    *max = UE_sched.next[*max];
    *p = -1;

    const uint8_t cqi = UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id];
    const int idx = CCE_try_allocate_dlsch(Mod_id, CC_id, subframe, UE_id, cqi);
    if (idx < 0)
      continue;
    UE_info->UE_sched_ctrl[UE_id].pre_dci_dl_pdu_idx = idx;

    max_num_ue--;

    /* allocate as much as possible */
    const int mcs = cqi_to_mcs[cqi];
    UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1 = mcs;
    int req = find_nb_rb_DL(mcs,
                            UE_info->UE_template[CC_id][UE_id].dl_buffer_total,
                            n_rbg_sched * RBGsize,
                            RBGsize);
    UE_sched_ctrl_t *ue_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    while (req > 0 && n_rbg_sched > 0) {
      ue_ctrl->rballoc_sub_UE[CC_id][rbg] = 1;
      rbgalloc_mask[rbg] = 0;
      const int sRBG = rbg == N_RBG - 1 ? RBGlastsize : RBGsize;
      ue_ctrl->pre_nb_available_rbs[CC_id] += sRBG;
      req -= sRBG;
      n_rbg_sched--;
      for (rbg++; n_rbg_sched > 0 && !rbgalloc_mask[rbg]; rbg++) /* fast-forward */ ;
    }
  }

  return n_rbg_sched;
}
default_sched_dl_algo_t maximum_throughput_wbcqi_dl = {
  .name  = "maximum_throughput_wbcqi_dl",
  .setup = mt_dl_setup,
  .unset = mt_dl_unset,
  .run   = mt_wbcqi_dl_run,
  .data  = NULL
};

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

    /* hack: in schedule_ue_spec, 3 bytes are "reserved" (this should be
     * done better). An RLC AM entity may ask for only 2 bytes for
     * ACKing and get a TBS of 32 bits and due to these 3 reserved bytes we may
     * end up with only 1 byte left for RLC, which is not enough. This hack
     * prevents the problem. To be done better at some point. If the function
     * schedule_ue_spec is (has been) reworked, this hack can be removed.
     * Dig for "TBS - ta_len - header_length_total - sdu_length_total - 3"
     * in schedule_ue_spec.
     */
    if (UE_template->dl_buffer_total > 0 && UE_template->dl_buffer_total <= 2)
      UE_template->dl_buffer_total += 3;

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
  eNB_MAC_INST *mac = RC.mac[Mod_id];
  UE_info_t *UE_info = &mac->UE_info;
  const int N_RBG = to_rbg(mac->common_channels[CC_id].mib->message.dl_Bandwidth);
  const int RBGsize = get_min_rb_unit(Mod_id, CC_id);

  store_dlsch_buffer(Mod_id, CC_id, frameP, subframeP);

  UE_list_t UE_to_sched;
  for (int i = 0; i < MAX_MOBILES_PER_ENB; ++i)
    UE_to_sched.next[i] = -1;
  int *cur = &UE_to_sched.head;

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    UE_sched_ctrl_t *ue_sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    const UE_TEMPLATE *ue_template = &UE_info->UE_template[CC_id][UE_id];

    /* initialize per-UE scheduling information */
    ue_sched_ctrl->pre_nb_available_rbs[CC_id] = 0;
    ue_sched_ctrl->dl_pow_off[CC_id] = 2;
    memset(ue_sched_ctrl->rballoc_sub_UE[CC_id], 0, sizeof(ue_sched_ctrl->rballoc_sub_UE[CC_id]));
    ue_sched_ctrl->pre_dci_dl_pdu_idx = -1;

    const rnti_t rnti = UE_RNTI(Mod_id, UE_id);
    if (rnti == NOT_A_RNTI) {
      LOG_E(MAC, "UE %d has RNTI NOT_A_RNTI!\n", UE_id);
      continue;
    }
    if (UE_info->active[UE_id] != true) {
      LOG_E(MAC, "UE %d RNTI %x is NOT active!\n", UE_id, rnti);
      continue;
    }
    if (ue_template->rach_resource_type > 0) {
      LOG_D(MAC,
            "UE %d is RACH resource type %d\n",
            UE_id,
            ue_template->rach_resource_type);
      continue;
    }
    if (mac_eNB_get_rrc_status(Mod_id, rnti) < RRC_CONNECTED) {
      LOG_D(MAC, "UE %d is not in RRC_CONNECTED\n", UE_id);
      continue;
    }

    /* define UEs to schedule */
    *cur = UE_id;
    cur = &UE_to_sched.next[UE_id];
  }
  *cur = -1;

  if (UE_to_sched.head < 0)
    return;

  uint8_t *vrb_map = mac->common_channels[CC_id].vrb_map;
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

  mac->pre_processor_dl.dl_algo.run(Mod_id,
                                    CC_id,
                                    frameP,
                                    subframeP,
                                    &UE_to_sched,
                                    4, // max_num_ue
                                    n_rbg_sched,
                                    rbgalloc_mask,
                                    mac->pre_processor_dl.dl_algo.data);

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
    LOG_D(MAC, "%4d.%d DL scheduler allocation list: %s\n", frameP, subframeP, t);
#endif
}

/// ULSCH PRE_PROCESSOR

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
  *tx_power = estimate_ue_tx_power(0,tbs * 8, rb_table[*rb_index], 0, Ncp, 0);

  /* find maximum MCS */
  while ((phr - *tx_power < 0 || tbs > bytes) && *mcs > 3) {
    (*mcs)--;
    tbs = get_TBS_UL(*mcs, rb_table[*rb_index]);
    *tx_power = estimate_ue_tx_power(0,tbs * 8, rb_table[*rb_index], 0, Ncp, 0);
  }

  /* find minimum necessary RBs */
  while (tbs < bytes
         && *rb_index < 32
         && rb_table[*rb_index] < max_rbs
         && phr - *tx_power > 0) {
    (*rb_index)++;
    tbs = get_TBS_UL(*mcs, rb_table[*rb_index]);
    *tx_power = estimate_ue_tx_power(0,tbs * 8, rb_table[*rb_index], 0, Ncp, 0);
  }

  /* Decrease if we went to far in last iteration */
  if (rb_table[*rb_index] > max_rbs)
    (*rb_index)--;

  // 1 or 2 PRB with cqi enabled does not work well
  if (rb_table[*rb_index] < 3) {
    *rb_index = 2; //3PRB
  }
}

int pp_find_rb_table_index(int approximate) {
  int lo = 2;
  if (approximate <= rb_table[lo])
    return lo;
  int hi = sizeof(rb_table) - 1;
  if (approximate >= rb_table[hi])
    return hi;
  int p = (hi + lo) / 2;
  for (; lo + 1 != hi; p = (hi + lo) / 2) {
    if (approximate <= rb_table[p])
      hi = p;
    else
      lo = p;
  }
  return p + 1;
}

void *rr_ul_setup(void) {
  void *data = malloc(sizeof(int));
  *(int *) data = 0;
  AssertFatal(data, "could not allocate data in %s()\n", __func__);
  return data;
}
void rr_ul_unset(void **data) {
  if (*data)
    free(*data);
  *data = NULL;
}
int rr_ul_run(module_id_t Mod_id,
              int CC_id,
              int frame,
              int subframe,
              int sched_frame,
              int sched_subframe,
              UE_list_t *UE_list,
              int max_num_ue,
              int num_contig_rb,
              contig_rbs_t *rbs,
              void *data) {
  AssertFatal(num_contig_rb <= 2, "cannot handle more than two contiguous RB regions\n");
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;
  const int max_rb = num_contig_rb > 1 ? MAX(rbs[0].length, rbs[1].length) : rbs[0].length;
  eNB_MAC_INST *mac = RC.mac[Mod_id];
  /* for every UE: check whether we have to handle a retransmission (and
   * allocate, if so). If not, compute how much RBs this UE would need */
  int rb_idx_required[MAX_MOBILES_PER_ENB];
  memset(rb_idx_required, 0, sizeof(rb_idx_required));
  int num_ue_req = 0;
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    UE_TEMPLATE *UE_template = &UE_info->UE_template[CC_id][UE_id];
    uint8_t harq_pid = subframe2harqpid(&mac->common_channels[CC_id],
                                        sched_frame, sched_subframe);
    if (UE_info->UE_sched_ctrl[UE_id].round_UL[CC_id][harq_pid] > 0) {
      /* this UE has a retransmission, allocate it right away */
      const int nb_rb = UE_template->nb_rb_ul[harq_pid];
      if (nb_rb == 0) {
        LOG_E(MAC,
              "%4d.%d UE %d retransmission of 0 RBs in round %d, ignoring\n",
              sched_frame, sched_subframe, UE_id,
              UE_info->UE_sched_ctrl[UE_id].round_UL[CC_id][harq_pid]);
        continue;
      }
      const uint8_t cqi = UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id];
      const int idx = CCE_try_allocate_ulsch(Mod_id, CC_id, subframe, UE_id, cqi);
      if (idx < 0)
        continue; // cannot allocate CCE
      UE_template->pre_dci_ul_pdu_idx = idx;
      if (rbs[0].length >= nb_rb) { // fits in first contiguous region
        UE_template->pre_first_nb_rb_ul = rbs[0].start;
        rbs[0].length -= nb_rb;
        rbs[0].start += nb_rb;
      } else if (num_contig_rb == 2 && rbs[1].length >= nb_rb) { // in second
        UE_template->pre_first_nb_rb_ul = rbs[1].start;
        rbs[1].length -= nb_rb;
        rbs[1].start += nb_rb;
      } else if (num_contig_rb == 2
          && rbs[1].start + rbs[1].length - rbs[0].start >= nb_rb) { // overlapping the middle
        UE_template->pre_first_nb_rb_ul = rbs[0].start;
        rbs[0].length = 0;
        int ol = nb_rb - (rbs[1].start - rbs[0].start); // how much overlap in second region
        if (ol > 0) {
          rbs[1].length -= ol;
          rbs[1].start += ol;
        }
      } else {
        LOG_W(MAC,
              "%d.%d cannot allocate UL retransmission for UE %d (nb_rb %d, rbs0.length %d, rbs1.length %d,rbs0.start %d,rbs1.start %d)\n",
              sched_frame,sched_subframe,UE_id,
              nb_rb, rbs[0].length,rbs[1].length,rbs[0].start,rbs[1].start);
        UE_template->pre_dci_ul_pdu_idx = -1; // do not need CCE
        mac->HI_DCI0_req[CC_id][subframe].hi_dci0_request_body.number_of_dci--;
        continue;
      }
      LOG_D(MAC, "%4d.%d UE %d retx %d RBs at start %d\n",
            sched_frame,
            sched_subframe,
            UE_id,
            UE_template->pre_allocated_nb_rb_ul,
            UE_template->pre_first_nb_rb_ul);
      UE_template->pre_allocated_nb_rb_ul = nb_rb;
      max_num_ue--;
      if (max_num_ue == 0) /* in this case, cannot allocate any other UE anymore */
        return rbs[0].length + (num_contig_rb > 1 ? rbs[1].length : 0);
      continue;
    }

    const int B = cmax(UE_template->estimated_ul_buffer - UE_template->scheduled_ul_bytes, 0);
    const int UE_to_be_scheduled = UE_is_to_be_scheduled(Mod_id, CC_id, UE_id);
    if (B == 0 && !UE_to_be_scheduled)
      continue;

    num_ue_req++;

    /* if UE has pending scheduling request then pre-allocate 3 RBs */
    if (B == 0 && UE_to_be_scheduled) {
      UE_template->pre_assigned_mcs_ul = 10; /* use QPSK mcs only */
      rb_idx_required[UE_id] = 2;
      //UE_template->pre_allocated_nb_rb_ul = 3;
      continue;
    }

    int mcs;
    int rb_table_index;
    int tx_power;
    calculate_max_mcs_min_rb(
        Mod_id,
        CC_id,
        B,
        UE_template->phr_info,
        UE_info->UE_sched_ctrl[UE_id].phr_received == 1 ? 20 : 10,
        &mcs,
        max_rb,
        &rb_table_index,
        &tx_power);

    UE_template->pre_assigned_mcs_ul = mcs;
    /* rb_idx_given >= MAX index: limit RBs to value in configuration file 
     * RBs in the uplink.  */
    rb_idx_required[UE_id] = min(mac->max_ul_rb_index, rb_table_index);
    //UE_template->pre_allocated_nb_rb_ul = rb_table[rb_table_index];
    /* only print log when PHR changed */
    static int phr = 0;
    if (phr != UE_template->phr_info) {
      phr = UE_template->phr_info;
      LOG_D(MAC, "%d.%d UE %d CC %d: pre mcs %d, pre rb_table[%d]=%d RBs (phr %d, tx power %d, bytes %d)\n",
            frame,
            subframe,
            UE_id,
            CC_id,
            UE_template->pre_assigned_mcs_ul,
            UE_template->pre_allocated_rb_table_index_ul,
            UE_template->pre_allocated_nb_rb_ul,
            UE_template->phr_info,
            tx_power,
            B);
    }
  }

  if (num_ue_req == 0)
    return rbs[0].length + (num_contig_rb > 1 ? rbs[1].length : 0);

  // calculate how many users should be in both regions, and to maximize usage,
  // go from the larger to the smaller one which at least will handle a single
  // full load case better.
  const int n = min(num_ue_req, max_num_ue);
  int nr[2] = {n, 0};
  int step = 1; // the order if we have two regions
  int start = 0;
  int end = 1;
  if (num_contig_rb > 1) {
    // proportionally divide between both regions
    int la = rbs[0].length > 0 ? rbs[0].length : 1;
    int lb = rbs[1].length > 0 ? rbs[1].length : 1;
    nr[1] = min(max(n/(la/lb + 1), 1), n - 1);
    nr[0] = n - nr[1];
    step = la > lb ? 1 : -1; // 1: from 0 to 1, -1: from 1 to 0
    start = la > lb ? 0 : 1;
    end = la > lb ? 2 : -1;
  }

  int *start_ue = data;
  if (*start_ue == -1)
    *start_ue = UE_list->head;
  int sUE_id = *start_ue;
  int rb_idx_given[MAX_MOBILES_PER_ENB];
  memset(rb_idx_given, 0, sizeof(rb_idx_given));

  for (int r = start; r != end; r += step) {
    // don't allocate if we have too little RBs
    if (rbs[r].length < 3)
      continue;
    if (nr[r] <= 0)
      continue;

    UE_list_t UE_sched;
    // average RB index: just below the index that fits all UEs
    int start_idx = pp_find_rb_table_index(rbs[r].length / nr[r]) - 1;
    int num_ue_sched = 0;
    int rb_required_add = 0;
    int *cur_UE = &UE_sched.head;
    while (num_ue_sched < nr[r]) {
      while (rb_idx_required[sUE_id] == 0)
        sUE_id = next_ue_list_looped(UE_list, sUE_id);
      const int cqi = UE_info->UE_sched_ctrl[sUE_id].dl_cqi[CC_id];
      const int idx = CCE_try_allocate_ulsch(Mod_id, CC_id, subframe, sUE_id, cqi);
      if (idx < 0) {
        LOG_D(MAC, "cannot allocate CCE for UE %d, skipping\n", sUE_id);
        nr[r]--;
        sUE_id = next_ue_list_looped(UE_list, sUE_id); // next candidate
        continue;
      }
      UE_info->UE_template[CC_id][sUE_id].pre_dci_ul_pdu_idx = idx;
      *cur_UE = sUE_id;
      cur_UE = &UE_sched.next[sUE_id];
      rb_idx_given[sUE_id] = min(start_idx, rb_idx_required[sUE_id]);
      rb_required_add += rb_table[rb_idx_required[sUE_id]] - rb_table[rb_idx_given[sUE_id]];
      rbs[r].length -= rb_table[rb_idx_given[sUE_id]];
      num_ue_sched++;
      sUE_id = next_ue_list_looped(UE_list, sUE_id);
    }
    *cur_UE = -1;

    /* give remaining RBs in RR fashion. Since we don't know in advance the
     * amount of RBs we can give (the "step size" in rb_table is non-linear), go
     * through all UEs and try to give a bit more. Continue until no UE can be
     * given a higher index because the remaining RBs do not suffice to increase */
    int UE_id = UE_sched.head;
    int rb_required_add_old;
    do {
      rb_required_add_old = rb_required_add;
      for (int UE_id = UE_sched.head; UE_id >= 0; UE_id = UE_sched.next[UE_id]) {
        if (rb_idx_given[UE_id] >= rb_idx_required[UE_id])
          continue; // this UE does not need more
        const int new_idx = rb_idx_given[UE_id] + 1;
        const int rb_inc = rb_table[new_idx] - rb_table[rb_idx_given[UE_id]];
        if (rbs[r].length < rb_inc)
          continue;
        rb_idx_given[UE_id] = new_idx;
        rbs[r].length -= rb_inc;
        rb_required_add -= rb_inc;
      }
    } while (rb_required_add != rb_required_add_old);

    for (UE_id = UE_sched.head; UE_id >= 0; UE_id = UE_sched.next[UE_id]) {
      UE_TEMPLATE *UE_template = &UE_info->UE_template[CC_id][UE_id];

      /* MCS has been allocated previously */
      UE_template->pre_first_nb_rb_ul = rbs[r].start;
      UE_template->pre_allocated_rb_table_index_ul = rb_idx_given[UE_id];
      UE_template->pre_allocated_nb_rb_ul = rb_table[rb_idx_given[UE_id]];
      rbs[r].start += rb_table[rb_idx_given[UE_id]];
      LOG_D(MAC, "%4d.%d UE %d allocated %d RBs start %d new start %d\n",
            sched_frame,
            sched_subframe,
            UE_id,
            UE_template->pre_allocated_nb_rb_ul,
            UE_template->pre_first_nb_rb_ul,
            rbs[r].start);
    }
  }

  /* just start with the next UE next time */
  *start_ue = next_ue_list_looped(UE_list, *start_ue);

  return rbs[0].length + (num_contig_rb > 1 ? rbs[1].length : 0);
}
default_sched_ul_algo_t round_robin_ul = {
  .name  = "round_robin_ul",
  .setup = rr_ul_setup,
  .unset = rr_ul_unset,
  .run   = rr_ul_run,
  .data  = NULL
};

void ulsch_scheduler_pre_processor(module_id_t Mod_id,
                                   int CC_id,
                                   frame_t frameP,
                                   sub_frame_t subframeP,
                                   frame_t sched_frameP,
                                   sub_frame_t sched_subframeP) {
  eNB_MAC_INST *mac = RC.mac[Mod_id];
  UE_info_t *UE_info = &mac->UE_info;
  const int N_RB_UL = to_prb(mac->common_channels[CC_id].ul_Bandwidth);
  COMMON_channels_t *cc = &mac->common_channels[CC_id];

  UE_list_t UE_to_sched;
  for (int i = 0; i < MAX_MOBILES_PER_ENB; ++i)
    UE_to_sched.next[i] = -1;
  int *cur = &UE_to_sched.head;

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    UE_TEMPLATE *UE_template = &UE_info->UE_template[CC_id][UE_id];
    UE_sched_ctrl_t *ue_sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

    /* initialize per-UE scheduling information */
    UE_template->pre_assigned_mcs_ul = 0;
    UE_template->pre_allocated_nb_rb_ul = 0;
    UE_template->pre_allocated_rb_table_index_ul = -1;
    UE_template->pre_first_nb_rb_ul = 0;
    UE_template->pre_dci_ul_pdu_idx = -1;

    const rnti_t rnti = UE_RNTI(Mod_id, UE_id);
    if (rnti == NOT_A_RNTI) {
      LOG_E(MAC, "UE %d has RNTI NOT_A_RNTI!\n", UE_id);
      continue;
    }
    if (ue_sched_ctrl->cdrx_configured && !ue_sched_ctrl->in_active_time)
      continue;
    if (UE_info->UE_template[CC_id][UE_id].rach_resource_type > 0)
      continue;

    /* define UEs to schedule */
    *cur = UE_id;
    cur = &UE_to_sched.next[UE_id];
  }
  *cur = -1;

  if (UE_to_sched.head < 0)
    return;

  int last_rb_blocked = 1;
  int n_contig = 0;
  contig_rbs_t rbs[2]; // up to two contig RBs for PRACH in between
  for (int i = 0; i < N_RB_UL; ++i) {
    if (cc->vrb_map_UL[i] == 0 && last_rb_blocked == 1) {
      last_rb_blocked = 0;
      n_contig++;
      AssertFatal(n_contig <= 2, "cannot handle more than two contiguous RB regions\n");
      rbs[n_contig - 1].start = i;
    }
    if (cc->vrb_map_UL[i] == 1 && last_rb_blocked == 0) {
      last_rb_blocked = 1;
      rbs[n_contig - 1].length = i - rbs[n_contig - 1].start;
    }
  }

  mac->pre_processor_ul.ul_algo.run(Mod_id,
                                    CC_id,
                                    frameP,
                                    subframeP,
                                    sched_frameP,
                                    sched_subframeP,
                                    &UE_to_sched,
                                    4, // max_num_ue
                                    n_contig,
                                    rbs,
                                    mac->pre_processor_ul.ul_algo.data);

  // the following block is meant for validation of the pre-processor to check
  // whether all UE allocations are non-overlapping and is not necessary for
  // scheduling functionality
#ifdef DEBUG_eNB_SCHEDULER
  char t[101] = "__________________________________________________"
                "__________________________________________________";
  t[N_RB_UL] = 0;
  for (int j = 0; j < N_RB_UL; j++)
    if (cc->vrb_map_UL[j] != 0)
      t[j] = 'x';
  int print = 0;
  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    UE_TEMPLATE *UE_template = &UE_info->UE_template[CC_id][UE_id];
    if (UE_template->pre_allocated_nb_rb_ul == 0)
      continue;

    print = 1;
    uint8_t harq_pid = subframe2harqpid(&mac->common_channels[CC_id],
                                        sched_frameP, sched_subframeP);
    LOG_D(MAC, "%4d.%d UE%d %d RBs (index %d) at start %d, pre MCS %d %s\n",
          frameP,
          subframeP,
          UE_id,
          UE_template->pre_allocated_nb_rb_ul,
          UE_template->pre_allocated_rb_table_index_ul,
          UE_template->pre_first_nb_rb_ul,
          UE_template->pre_assigned_mcs_ul,
          UE_info->UE_sched_ctrl[UE_id].round_UL[CC_id][harq_pid] > 0 ? "(retx)" : "");

    for (int i = 0; i < UE_template->pre_allocated_nb_rb_ul; ++i) {
      /* only check if this is not a retransmission */
      if (UE_info->UE_sched_ctrl[UE_id].round_UL[CC_id][harq_pid] == 0
          && cc->vrb_map_UL[UE_template->pre_first_nb_rb_ul + i] == 1) {

        LOG_I(MAC, "%4d.%d UL scheduler allocation list: %s\n", frameP, subframeP, t);
        LOG_E(MAC,
              "%4d.%d: UE %d allocated at locked RB %d (is: allocated start "
              "%d/length %d)\n",
              frameP, subframeP, UE_id, UE_template->pre_first_nb_rb_ul + i,
              UE_template->pre_first_nb_rb_ul,
              UE_template->pre_allocated_nb_rb_ul);
      }
      cc->vrb_map_UL[UE_template->pre_first_nb_rb_ul + i] = 1;
      t[UE_template->pre_first_nb_rb_ul + i] = UE_id + '0';
    }
  }
  if (print)
    LOG_D(MAC,
          "%4d.%d UL scheduler allocation list: %s\n",
          sched_frameP,
          sched_subframeP,
          t);
#endif
}
