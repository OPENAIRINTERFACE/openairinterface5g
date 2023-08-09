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

/*!
 * \file   slicing.c
 * \brief  Generic slicing helper functions and Static Slicing Implementation
 * \author Robert Schmidt
 * \date   2020
 * \email  robert.schmidt@eurecom.fr
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <dlfcn.h>

#include "assertions.h"
#include "common/utils/LOG/log.h"

#include "slicing.h"
#include "slicing_internal.h"

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

#define RET_FAIL(ret, x...) do { LOG_E(MAC, x); return ret; } while (0)

int slicing_get_UE_slice_idx(slice_info_t *si, int UE_id) {
  return si->UE_assoc_slice[UE_id];
}

void slicing_add_UE(slice_info_t *si, int UE_id) {
  add_ue_list(&si->s[0]->UEs, UE_id);
  si->UE_assoc_slice[UE_id] = 0;
}

void _remove_UE(slice_t **s, uint8_t *assoc, int UE_id) {
  const uint8_t i = assoc[UE_id];
  DevAssert(remove_ue_list(&s[i]->UEs, UE_id));
  assoc[UE_id] = -1;
}

void slicing_remove_UE(slice_info_t *si, int UE_id) {
  _remove_UE(si->s, si->UE_assoc_slice, UE_id);
}

void _move_UE(slice_t **s, uint8_t *assoc, int UE_id, int to) {
  const uint8_t i = assoc[UE_id];
  const int ri = remove_ue_list(&s[i]->UEs, UE_id);
  if (!ri)
    LOG_W(MAC, "did not find UE %d in DL slice index %d\n", UE_id, i);
  add_ue_list(&s[to]->UEs, UE_id);
  assoc[UE_id] = to;
}

void slicing_move_UE(slice_info_t *si, int UE_id, int idx) {
  DevAssert(idx >= -1 && idx < si->num);
  if (idx >= 0)
    _move_UE(si->s, si->UE_assoc_slice, UE_id, idx);
}

int _exists_slice(uint8_t n, slice_t **s, int id) {
  for (int i = 0; i < n; ++i)
    if (s[i]->id == id)
      return i;
  return -1;
}

slice_t *_add_slice(uint8_t *n, slice_t **s) {
  s[*n] = calloc(1, sizeof(slice_t));
  if (!s[*n])
    return NULL;
  init_ue_list(&s[*n]->UEs);
  *n += 1;
  return s[*n - 1];
}

slice_t *_remove_slice(uint8_t *n, slice_t **s, uint8_t *assoc, int idx) {
  if (idx >= *n)
    return NULL;

  slice_t *sr = s[idx];
  while (sr->UEs.head >= 0)
    _move_UE(s, assoc, sr->UEs.head, 0);

  for (int i = idx + 1; i < *n; ++i)
    s[i - 1] = s[i];
  *n -= 1;
  s[*n] = NULL;

  for (int i = 0; i < MAX_MOBILES_PER_ENB; ++i)
    if (assoc[i] > idx)
      assoc[i] -= 1;

  if (sr->label)
    free(sr->label);

  return sr;
}

/************************ Static Slicing Implementation ************************/

int addmod_static_slice_dl(slice_info_t *si,
                           int id,
                           char *label,
                           void *algo,
                           void *slice_params_dl) {
  static_slice_param_t *dl = slice_params_dl;
  if (dl && dl->posLow > dl->posHigh)
    RET_FAIL(-1, "%s(): slice id %d posLow > posHigh\n", __func__, id);

  uint8_t rbgMap[25] = { 0 };
  int index = _exists_slice(si->num, si->s, id);
  if (index >= 0) {
    for (int s = 0; s < si->num; ++s) {
      static_slice_param_t *sd = dl && si->s[s]->id == id ? dl : si->s[s]->algo_data;
      for (int i = sd->posLow; i <= sd->posHigh; ++i) {
        if (rbgMap[i])
          RET_FAIL(-33, "%s(): overlap of slices detected at RBG %d\n", __func__, i);
        rbgMap[i] = 1;
      }
    }
    /* no problem, can allocate */
    slice_t *s = si->s[index];
    if (label) {
      if (s->label) free(s->label);
      s->label = label;
    }
    if (algo) {
      s->dl_algo.unset(&s->dl_algo.data);
      s->dl_algo = *(default_sched_dl_algo_t *) algo;
      if (!s->dl_algo.data)
        s->dl_algo.data = s->dl_algo.setup();
    }
    if (dl) {
      free(s->algo_data);
      s->algo_data = dl;
    }
    return index;
  }

  if (!dl)
    RET_FAIL(-100, "%s(): no parameters for new slice %d, aborting\n", __func__, id);

  if (si->num >= MAX_STATIC_SLICES)
    RET_FAIL(-2, "%s(): cannot have more than %d slices\n", __func__, MAX_STATIC_SLICES);
  for (int s = 0; s < si->num; ++s) {
    static_slice_param_t *sd = si->s[s]->algo_data;
    for (int i = sd->posLow; i <= sd->posHigh; ++i)
      rbgMap[i] = 1;
  }

  for (int i = dl->posLow; i <= dl->posHigh; ++i)
    if (rbgMap[i])
      RET_FAIL(-3, "%s(): overlap of slices detected at RBG %d\n", __func__, i);

  if (!algo)
    RET_FAIL(-14, "%s(): no scheduler algorithm provided\n", __func__);

  slice_t *ns = _add_slice(&si->num, si->s);
  if (!ns)
    RET_FAIL(-4, "%s(): could not create new slice\n", __func__);
  ns->id = id;
  ns->label = label;
  ns->dl_algo = *(default_sched_dl_algo_t *) algo;
  if (!ns->dl_algo.data)
    ns->dl_algo.data = ns->dl_algo.setup();
  ns->algo_data = dl;

  return si->num - 1;
}

int addmod_static_slice_ul(slice_info_t *si,
                           int id,
                           char *label,
                           void *algo,
                           void *slice_params_ul) {
  static_slice_param_t *ul = slice_params_ul;
  /* Minimum 3RBs, because LTE stack requires this */
  if (ul && ul->posLow + 2 > ul->posHigh)
    RET_FAIL(-1, "%s(): slice id %d posLow + 2 > posHigh\n", __func__, id);

  uint8_t rbMap[110] = { 0 };
  int index = _exists_slice(si->num, si->s, id);
  if (index >= 0) {
    for (int s = 0; s < si->num; ++s) {
      static_slice_param_t *su = ul && si->s[s]->id == id && ul ? ul : si->s[s]->algo_data;
      for (int i = su->posLow; i <= su->posHigh; ++i) {
        if (rbMap[i])
          RET_FAIL(-33, "%s(): overlap of slices detected at RBG %d\n", __func__, i);
        rbMap[i] = 1;
      }
    }
    /* no problem, can allocate */
    slice_t *s = si->s[index];
    if (algo) {
      s->ul_algo.unset(&s->ul_algo.data);
      s->ul_algo = *(default_sched_ul_algo_t *) algo;
      if (!s->ul_algo.data)
        s->ul_algo.data = s->ul_algo.setup();
    }
    if (label) {
      if (s->label) free(s->label);
      s->label = label;
    }
    if (ul) {
      free(s->algo_data);
      s->algo_data = ul;
    }
    return index;
  }

  if (!ul)
    RET_FAIL(-100, "%s(): no parameters for new slice %d, aborting\n", __func__, id);

  if (si->num >= MAX_STATIC_SLICES)
    RET_FAIL(-2, "%s(): cannot have more than %d slices\n", __func__, MAX_STATIC_SLICES);
  for (int s = 0; s < si->num; ++s) {
    static_slice_param_t *sd = si->s[s]->algo_data;
    for (int i = sd->posLow; i <= sd->posHigh; ++i)
      rbMap[i] = 1;
  }

  for (int i = ul->posLow; i <= ul->posHigh; ++i)
    if (rbMap[i])
      RET_FAIL(-3, "%s(): overlap of slices detected at RBG %d\n", __func__, i);

  if (!algo)
    RET_FAIL(-14, "%s(): no scheduler algorithm provided\n", __func__);

  slice_t *ns = _add_slice(&si->num, si->s);
  if (!ns)
    RET_FAIL(-4, "%s(): could not create new slice\n", __func__);
  ns->id = id;
  ns->label = label;
  ns->ul_algo = *(default_sched_ul_algo_t *) algo;
  if (!ns->ul_algo.data)
    ns->ul_algo.data = ns->ul_algo.setup();
  ns->algo_data = ul;

  return si->num - 1;
}

int remove_static_slice_dl(slice_info_t *si, uint8_t slice_idx) {
  if (slice_idx == 0)
    return 0;
  slice_t *sr = _remove_slice(&si->num, si->s, si->UE_assoc_slice, slice_idx);
  if (!sr)
    return 0;
  free(sr->algo_data);
  sr->dl_algo.unset(&sr->dl_algo.data);
  free(sr);
  return 1;
}

int remove_static_slice_ul(slice_info_t *si, uint8_t slice_idx) {
  if (slice_idx == 0)
    return 0;
  slice_t *sr = _remove_slice(&si->num, si->s, si->UE_assoc_slice, slice_idx);
  if (!sr)
    return 0;
  free(sr->algo_data);
  sr->ul_algo.unset(&sr->ul_algo.data);
  free(sr);
  return 1;
}

void static_dl(module_id_t mod_id,
               int CC_id,
               frame_t frame,
               sub_frame_t subframe) {
  UE_info_t *UE_info = &RC.mac[mod_id]->UE_info;

  store_dlsch_buffer(mod_id, CC_id, frame, subframe);

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    UE_sched_ctrl_t *ue_sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

    /* initialize per-UE scheduling information */
    ue_sched_ctrl->pre_nb_available_rbs[CC_id] = 0;
    ue_sched_ctrl->dl_pow_off[CC_id] = 2;
    memset(ue_sched_ctrl->rballoc_sub_UE[CC_id], 0, sizeof(ue_sched_ctrl->rballoc_sub_UE[CC_id]));
    ue_sched_ctrl->pre_dci_dl_pdu_idx = -1;
  }

  const int N_RBG = to_rbg(RC.mac[mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
  const int RBGsize = get_min_rb_unit(mod_id, CC_id);
  uint8_t *vrb_map = RC.mac[mod_id]->common_channels[CC_id].vrb_map;
  uint8_t rbgalloc_mask[N_RBG_MAX];
  for (int i = 0; i < N_RBG; i++) {
    // calculate mask: init to one + "AND" with vrb_map:
    // if any RB in vrb_map is blocked (1), the current RBG will be 0
    rbgalloc_mask[i] = 1;
    for (int j = 0; j < RBGsize; j++)
      rbgalloc_mask[i] &= !vrb_map[RBGsize * i + j];
  }

  slice_info_t *s = RC.mac[mod_id]->pre_processor_dl.slices;
  int max_num_ue;
  switch (s->num) {
    case 1:
      max_num_ue = 4;
      break;
    case 2:
      max_num_ue = 2;
      break;
    default:
      max_num_ue = 1;
      break;
  }
  for (int i = 0; i < s->num; ++i) {
    if (s->s[i]->UEs.head < 0)
      continue;
    uint8_t rbgalloc_slice_mask[N_RBG_MAX];
    memset(rbgalloc_slice_mask, 0, sizeof(rbgalloc_slice_mask));
    static_slice_param_t *p = s->s[i]->algo_data;
    int n_rbg_sched = 0;
    for (int rbg = p->posLow; rbg <= p->posHigh && rbg <= N_RBG; ++rbg) {
      rbgalloc_slice_mask[rbg] = rbgalloc_mask[rbg];
      n_rbg_sched += rbgalloc_mask[rbg];
    }
    if (n_rbg_sched == 0) /* no free RBGs, e.g., taken by RA */
      continue;

    s->s[i]->dl_algo.run(mod_id,
                         CC_id,
                         frame,
                         subframe,
                         &s->s[i]->UEs,
                         max_num_ue, // max_num_ue
                         n_rbg_sched,
                         rbgalloc_slice_mask,
                         s->s[i]->dl_algo.data);
  }

  // the following block is meant for validation of the pre-processor to check
  // whether all UE allocations are non-overlapping and is not necessary for
  // scheduling functionality
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
          frame,
          subframe,
          UE_id,
          ue_sched_ctrl->pre_nb_available_rbs[CC_id],
          UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1);

    print = 1;

    for (int i = 0; i < N_RBG; i++) {
      if (!ue_sched_ctrl->rballoc_sub_UE[CC_id][i])
        continue;
      for (int j = 0; j < RBGsize; j++) {
        if (vrb_map[RBGsize*i+j] != 0) {
          LOG_I(MAC, "%4d.%d DL scheduler allocation list: %s\n", frame, subframe, t);
          LOG_E(MAC, "%4d.%d: UE %d allocated at locked RB %d/RBG %d\n", frame,
                subframe, UE_id, RBGsize * i + j, i);
        }
        vrb_map[RBGsize*i+j] = 1;
      }
      t[i] = '0' + UE_id;
    }
  }
  if (print)
    LOG_D(MAC, "%4d.%d DL scheduler allocation list: %s\n", frame, subframe, t);
}

void static_ul(module_id_t mod_id,
               int CC_id,
               frame_t frame,
               sub_frame_t subframe,
               frame_t sched_frame,
               sub_frame_t sched_subframe) {
  UE_info_t *UE_info = &RC.mac[mod_id]->UE_info;
  const int N_RB_UL = to_prb(RC.mac[mod_id]->common_channels[CC_id].ul_Bandwidth);
  COMMON_channels_t *cc = &RC.mac[mod_id]->common_channels[CC_id];

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    UE_TEMPLATE *UE_template = &UE_info->UE_template[CC_id][UE_id];
    UE_template->pre_assigned_mcs_ul = 0;
    UE_template->pre_allocated_nb_rb_ul = 0;
    UE_template->pre_allocated_rb_table_index_ul = -1;
    UE_template->pre_first_nb_rb_ul = 0;
    UE_template->pre_dci_ul_pdu_idx = -1;
  }

  slice_info_t *s = RC.mac[mod_id]->pre_processor_ul.slices;
  int max_num_ue;
  switch (s->num) {
    case 1:
      max_num_ue = 4;
      break;
    case 2:
      max_num_ue = 2;
      break;
    default:
      max_num_ue = 1;
      break;
  }
  for (int i = 0; i < s->num; ++i) {
    if (s->s[i]->UEs.head < 0)
      continue;
    int last_rb_blocked = 1;
    int n_contig = 0;
    contig_rbs_t rbs[2]; // up to two contig RBs for PRACH in between
    static_slice_param_t *p = s->s[i]->algo_data;
    for (int rb = p->posLow; rb <= p->posHigh && rb < N_RB_UL; ++rb) {
      if (cc->vrb_map_UL[rb] == 0 && last_rb_blocked) {
        last_rb_blocked = 0;
        n_contig++;
        AssertFatal(n_contig <= 2, "cannot handle more than two contiguous RB regions\n");
        rbs[n_contig - 1].start = rb;
      }
      if (cc->vrb_map_UL[rb] == 1 && !last_rb_blocked) {
        last_rb_blocked = 1;
        rbs[n_contig - 1].length = rb - rbs[n_contig - 1].start;
      }
    }
    if (!last_rb_blocked)
      rbs[n_contig - 1].length = p->posHigh - rbs[n_contig - 1].start + 1;
    if (n_contig == 1 && rbs[0].length == 0) /* no RBs, e.g., taken by RA */
      continue;

    s->s[i]->ul_algo.run(mod_id,
                         CC_id,
                         frame,
                         subframe,
                         sched_frame,
                         sched_subframe,
                         &s->s[i]->UEs,
                         max_num_ue, // max_num_ue
                         n_contig,
                         rbs,
                         s->s[i]->ul_algo.data);
  }

  // the following block is meant for validation of the pre-processor to check
  // whether all UE allocations are non-overlapping and is not necessary for
  // scheduling functionality
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
    uint8_t harq_pid = subframe2harqpid(&RC.mac[mod_id]->common_channels[CC_id],
                                        sched_frame, sched_subframe);
    LOG_D(MAC, "%4d.%d UE%d %d RBs (index %d) at start %d, pre MCS %d %s\n",
          frame,
          subframe,
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

        LOG_I(MAC, "%4d.%d UL scheduler allocation list: %s\n", frame, subframe, t);
        LOG_E(MAC,
              "%4d.%d: UE %d allocated at locked RB %d (is: allocated start "
              "%d/length %d)\n",
              frame, subframe, UE_id, UE_template->pre_first_nb_rb_ul + i,
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
          sched_frame,
          sched_subframe,
          t);
}

void static_destroy(slice_info_t **si) {
  const int n = (*si)->num;
  (*si)->num = 0;
  for (int i = 0; i < n; ++i) {
    slice_t *s = (*si)->s[i];
    if (s->label)
      free(s->label);
    free(s->algo_data);
    free(s);
  }
  free((*si)->s);
  free(*si);
}

pp_impl_param_t static_dl_init(module_id_t mod_id, int CC_id) {
  slice_info_t *si = calloc(1, sizeof(slice_info_t));
  DevAssert(si);

  si->num = 0;
  si->s = calloc(MAX_STATIC_SLICES, sizeof(slice_t));
  DevAssert(si->s);
  for (int i = 0; i < MAX_MOBILES_PER_ENB; ++i)
    si->UE_assoc_slice[i] = -1;

  /* insert default slice, all resources */
  static_slice_param_t *dlp = malloc(sizeof(static_slice_param_t));
  dlp->posLow = 0;
  dlp->posHigh = to_rbg(RC.mac[mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth) - 1;
  default_sched_dl_algo_t *algo = &RC.mac[mod_id]->pre_processor_dl.dl_algo;
  algo->data = NULL;
  DevAssert(0 == addmod_static_slice_dl(si, 0, strdup("default"), algo, dlp));
  const UE_list_t *UE_list = &RC.mac[mod_id]->UE_info.list;
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id])
    slicing_add_UE(si, UE_id);

  pp_impl_param_t sttc;
  sttc.algorithm = STATIC_SLICING;
  sttc.add_UE = slicing_add_UE;
  sttc.remove_UE = slicing_remove_UE;
  sttc.move_UE = slicing_move_UE;
  sttc.addmod_slice = addmod_static_slice_dl;
  sttc.remove_slice = remove_static_slice_dl;
  sttc.dl = static_dl;
  // current DL algo becomes default scheduler
  sttc.dl_algo = *algo;
  sttc.destroy = static_destroy;
  sttc.slices = si;

  return sttc;
}

pp_impl_param_t static_ul_init(module_id_t mod_id, int CC_id) {
  slice_info_t *si = calloc(1, sizeof(slice_info_t));
  DevAssert(si);

  si->num = 0;
  si->s = calloc(MAX_STATIC_SLICES, sizeof(slice_t));
  DevAssert(si->s);
  for (int i = 0; i < MAX_MOBILES_PER_ENB; ++i)
    si->UE_assoc_slice[i] = -1;

  /* insert default slice, all resources */
  static_slice_param_t *ulp = malloc(sizeof(static_slice_param_t));
  ulp->posLow = 0;
  ulp->posHigh = to_prb(RC.mac[mod_id]->common_channels[CC_id].ul_Bandwidth) - 1;
  default_sched_ul_algo_t *algo = &RC.mac[mod_id]->pre_processor_ul.ul_algo;
  algo->data = NULL;
  DevAssert(0 == addmod_static_slice_ul(si, 0, strdup("default"), algo, ulp));
  const UE_list_t *UE_list = &RC.mac[mod_id]->UE_info.list;
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id])
    slicing_add_UE(si, UE_id);

  pp_impl_param_t sttc;
  sttc.algorithm = STATIC_SLICING;
  sttc.add_UE = slicing_add_UE;
  sttc.remove_UE = slicing_remove_UE;
  sttc.move_UE = slicing_move_UE;
  sttc.addmod_slice = addmod_static_slice_ul;
  sttc.remove_slice = remove_static_slice_ul;
  sttc.ul = static_ul;
  // current DL algo becomes default scheduler
  sttc.ul_algo = *algo;
  sttc.destroy = static_destroy;
  sttc.slices = si;

  return sttc;
}
