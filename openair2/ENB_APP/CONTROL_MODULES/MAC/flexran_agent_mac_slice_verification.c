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

/*! \file flexran_agent_mac_slice_verification.c
 * \brief MAC Agent slice verification helper functions
 * \author Robert Schmidt
 * \date 2018
 * \version 0.1
 */


#include "flexran_agent_mac_slice_verification.h"

/* overlap check for UL slices, helper type */
struct sregion_s {
  int start;
  int length;
};

/* forward declaration of locally-used verification functions */
int flexran_dl_slice_verify_pct(int pct);
int flexran_dl_slice_verify_priority(int prio);
int flexran_dl_slice_verify_position(int pos_low, int pos_high);
int flexran_dl_slice_verify_maxmcs(int maxmcs);
int flexran_ul_slice_verify_pct(int pct);
int flexran_ul_slice_verify_priority(int prio);
int flexran_ul_slice_verify_first_rb(int first_rb);
int flexran_ul_slice_verify_maxmcs(int maxmcs);
int check_ul_slice_overlap(mid_t mod_id, struct sregion_s *sr, int n);

int flexran_verify_dl_slice(mid_t mod_id, Protocol__FlexDlSlice *dls)
{
  /* check mandatory parameters */
  if (!dls->has_id) {
    LOG_E(FLEXRAN_AGENT, "[%d] Incoming DL slice configuration has no ID\n", mod_id);
    return 0;
  }

  /* verify parameters individualy */
  /* label is enum */
  if (!flexran_dl_slice_verify_pct(dls->percentage)) {
    LOG_E(FLEXRAN_AGENT, "[%d][DL slice %d] illegal DL slice percentage (%d)\n",
          mod_id, dls->id, dls->percentage);
    return 0;
  }
  /* isolation is a protobuf bool */
  if (!flexran_dl_slice_verify_priority(dls->priority)) {
    LOG_E(FLEXRAN_AGENT, "[%d][DL slice %d] illegal DL slice priority (%d)\n",
          mod_id, dls->id, dls->priority);
    return 0;
  }
  if (!flexran_dl_slice_verify_position(dls->position_low, dls->position_high)) {
    LOG_E(FLEXRAN_AGENT,
          "[%d][DL slice %d] illegal DL slice position low (%d) and/or high (%d)\n",
          mod_id, dls->id, dls->position_low, dls->position_high);
    return 0;
  }
  if (!flexran_dl_slice_verify_maxmcs(dls->maxmcs)) {
    LOG_E(FLEXRAN_AGENT, "[%d][DL slice %d] illegal DL slice max mcs %d\n",
          mod_id, dls->id, dls->maxmcs);
    return 0;
  }
  if (dls->n_sorting == 0) {
    LOG_E(FLEXRAN_AGENT, "[%d][DL slice %d] no sorting in DL slice\n",
          mod_id, dls->id);
    return 0;
  }
  if (!dls->sorting) {
    LOG_E(FLEXRAN_AGENT, "[%d][DL slice %d] no sorting found in DL slice\n",
          mod_id, dls->id);
    return 0;
  }
  /* sorting is an enum */
  /* accounting is an enum */
  if (!dls->scheduler_name) {
    LOG_E(FLEXRAN_AGENT, "[%d][DL slice %d] no scheduler name found\n",
          mod_id, dls->id);
    return 0;
  }
  if (strcmp(dls->scheduler_name, "schedule_ue_spec") != 0) {
    LOG_E(FLEXRAN_AGENT, "[%d][DL slice %d] setting the scheduler to something "
          "different than schedule_ue_spec is currently not allowed\n",
          mod_id, dls->id);
    return 0;
  }

  return 1;
}

int flexran_verify_group_dl_slices(mid_t mod_id, Protocol__FlexDlSlice **existing,
    int n_ex, Protocol__FlexDlSlice **update, int n_up)
{
  int i, j, n;
  int pct, pct_orig;
  /* for every update, array points to existing slice, or NULL if update
   * creates new slice */
  Protocol__FlexDlSlice *s[n_up];
  for (i = 0; i < n_up; i++) {
    s[i] = NULL;
    for (j = 0; j < n_ex; j++) {
      if (existing[j]->id == update[i]->id)
        s[i] = existing[j];
    }
  }

  /* check that number of created and number of added slices in total matches
   * [1,10] */
  n = n_ex;
  for (i = 0; i < n_up; i++) {
    /* new slice */
    if (!s[i]) n += 1;
    /* slice will be deleted */
    else if (s[i]->percentage == 0) n -= 1;
    /* else "only" an update */
  }

  if (n < 1 || n > MAX_NUM_SLICES) {
    LOG_E(FLEXRAN_AGENT, "[%d] Illegal number of resulting DL slices (%d -> %d)\n",
          mod_id, n_ex, n);
    return 0;
  }

  /* check that the sum of all slices percentages (including removed/added
   * slices) matches [1,100] */
  pct = 0;
  for (i = 0; i < n_ex; i++) {
    pct += existing[i]->percentage;
  }
  pct_orig = pct;
  for (i = 0; i < n_up; i++) {
    /* if there is an existing slice, subtract its percentage and add the
     * update's percentage */
    if (s[i])
      pct -= s[i]->percentage;
    pct += update[i]->percentage;
  }
  if (pct < 1 || pct > 100) {
    LOG_E(FLEXRAN_AGENT,
          "[%d] invalid total RB share for DL slices (%d%% -> %d%%)\n",
          mod_id, pct_orig, pct);
    return 0;
  }

  return 1;
}

int flexran_verify_ul_slice(mid_t mod_id, Protocol__FlexUlSlice *uls)
{
  /* check mandatory parameters */
  if (!uls->has_id) {
    LOG_E(FLEXRAN_AGENT, "[%d] Incoming UL slice configuration has no ID\n", mod_id);
    return 0;
  }

  /* verify parameters individually */
  /* label is enum */
  if (!flexran_ul_slice_verify_pct(uls->percentage)) {
    LOG_E(FLEXRAN_AGENT, "[%d][UL slice %d] illegal UL slice percentage (%d)\n",
          mod_id, uls->id, uls->percentage);
    return 0;
  }
  /* isolation is a protobuf bool */
  if (!flexran_ul_slice_verify_priority(uls->priority)) {
    LOG_E(FLEXRAN_AGENT, "[%d][UL slice %d] illegal UL slice percentage (%d)\n",
          mod_id, uls->id, uls->priority);
    return 0;
  }
  if (!flexran_ul_slice_verify_first_rb(uls->first_rb)) {
    LOG_E(FLEXRAN_AGENT, "[%d][UL slice %d] illegal UL slice first RB (%d)\n",
          mod_id, uls->id, uls->first_rb);
    return 0;
  }
  if (!flexran_ul_slice_verify_maxmcs(uls->maxmcs)) {
    LOG_E(FLEXRAN_AGENT, "[%d][UL slice %d] illegal UL slice max mcs (%d)\n",
          mod_id, uls->id, uls->maxmcs);
    return 0;
  }
  /* TODO
  if (uls->n_sorting == 0) {
    LOG_E(FLEXRAN_AGENT, "[%d][UL slice %d] no sorting in UL slice\n",
          mod_id, uls->id);
    return 0;
  }
  if (!uls->sorting) {
    LOG_E(FLEXRAN_AGENT, "[%d][UL slice %d] no sorting found in UL slice\n",
          mod_id, uls->id);
    return 0;
  }
  */
  /* sorting is an enum */
  /* accounting is an enum */
  if (!uls->scheduler_name) {
    LOG_E(FLEXRAN_AGENT, "[%d][UL slice %d] no scheduler name found\n",
          mod_id, uls->id);
    return 0;
  }
  if (strcmp(uls->scheduler_name, "schedule_ulsch_rnti") != 0) {
    LOG_E(FLEXRAN_AGENT, "[%d][UL slice %d] setting the scheduler to something "
          "different than schedule_ulsch_rnti is currently not allowed\n",
          mod_id, uls->id);
    return 0;
  }

  return 1;
}

int flexran_verify_group_ul_slices(mid_t mod_id, Protocol__FlexUlSlice **existing,
    int n_ex, Protocol__FlexUlSlice **update, int n_up)
{
  int i, j, n;
  int pct, pct_orig;
  /* for every update, array "s" points to existing slice, or NULL if update
   * creates new slice; array "offs" gives the offset of this slice */
  Protocol__FlexUlSlice *s[n_up];
  int offs[n_up];
  for (i = 0; i < n_up; i++) {
    s[i] = NULL;
    offs[i] = 0;
    for (j = 0; j < n_ex; j++) {
      if (existing[j]->id == update[i]->id) {
        s[i] = existing[j];
        offs[i] = j;
      }
    }
  }

  /* check that number of created and number of added slices in total matches
   * [1,10] */
  n = n_ex;
  for (i = 0; i < n_up; i++) {
    /* new slice */
    if (!s[i]) n += 1;
    /* slice will be deleted */
    else if (s[i]->percentage == 0) n -= 1;
    /* else "only" an update */
  }

  if (n < 1 || n > MAX_NUM_SLICES) {
    LOG_E(FLEXRAN_AGENT, "[%d] Illegal number of resulting UL slices (%d -> %d)\n",
          mod_id, n_ex, n);
    return 0;
  }

  /* check that the sum of all slices percentages (including removed/added
   * slices) matches [1,100] */
  pct = 0;
  for (i = 0; i < n_ex; i++) {
    pct += existing[i]->percentage;
  }
  pct_orig = pct;
  for (i = 0; i < n_up; i++) {
    /* if there is an existing slice, subtract its percentage and add the
     * update's percentage */
    if (s[i])
      pct -= s[i]->percentage;
    pct += update[i]->percentage;
  }
  if (pct < 1 || pct > 100) {
    LOG_E(FLEXRAN_AGENT, "[%d] invalid total RB share (%d%% -> %d%%)\n",
          mod_id, pct_orig, pct);
    return 0;
  }

  /* check that there is no overlap in slices resulting as the combination of
   * first_rb and percentage */
  struct sregion_s sregion[n];
  const int N_RB = flexran_get_N_RB_UL(mod_id, 0); /* assume PCC */
  int k = n_ex;
  for (i = 0; i < n_ex; i++) {
    sregion[i].start = existing[i]->first_rb;
    sregion[i].length = existing[i]->percentage * N_RB / 100;
  }
  for (i = 0; i < n_up; i++) {
    ptrdiff_t d = s[i] ? offs[i] : k++;
    AssertFatal(d >= 0 && d < k, "illegal pointer offset (%ld, k=%d)\n", d, k);
    sregion[d].start = update[i]->first_rb;
    sregion[d].length = update[i]->percentage * N_RB / 100;
  }
  AssertFatal(k == n, "illegal number of slices while calculating overlap\n");
  if (!check_ul_slice_overlap(mod_id, sregion, k)) {
    LOG_E(FLEXRAN_AGENT, "[%d] UL slices are overlapping\n", mod_id);
    return 0;
  }

  return 1;
}

int flexran_dl_slice_verify_pct(int pct)
{
  return pct >= 0 && pct <= 100;
}

int flexran_dl_slice_verify_priority(int prio)
{
  return prio >= 0;
}

int flexran_dl_slice_verify_position(int pos_low, int pos_high)
{
  return pos_low < pos_high && pos_low >= 0 && pos_high <= N_RBG_MAX;
}

int flexran_dl_slice_verify_maxmcs(int maxmcs)
{
  return maxmcs >= 0 && maxmcs <= 28;
}

int flexran_ul_slice_verify_pct(int pct)
{
  return pct >= 0 && pct <= 100;
}

int flexran_ul_slice_verify_priority(int prio)
{
  return prio >= 0;
}

int flexran_ul_slice_verify_first_rb(int first_rb)
{
  return first_rb >= 0 && first_rb < 100;
}

int flexran_ul_slice_verify_maxmcs(int maxmcs)
{
  return maxmcs >= 0 && maxmcs <= 20;
}

int sregion_compare(const void *_a, const void *_b)
{
  const struct sregion_s *a = (const struct sregion_s *)_a;
  const struct sregion_s *b = (const struct sregion_s *)_b;
  const int res = a->start - b->start;
  if (res < 0)       return -1;
  else if (res == 0) return 0;
  else               return 1;
}

int check_ul_slice_overlap(mid_t mod_id, struct sregion_s *sr, int n)
{
  int i;
  int overlap, op, u;
  const int N_RB = flexran_get_N_RB_UL(mod_id, 0); /* assume PCC */
  qsort(sr, n, sizeof(sr[0]), sregion_compare);
  for (i = 0; i < n; i++) {
    u = i == n-1 ? N_RB : sr[i+1].start;
    AssertFatal(sr[i].start <= u, "unsorted slice list\n");
    overlap = sr[i].start + sr[i].length - u;
    if (overlap <= 0) continue;
    op = overlap * 100 / sr[i].length;
    LOG_W(FLEXRAN_AGENT, "[%d] slice overlap of %d%% detected\n", mod_id, op);
    if (op >= 10) /* more than 10% overlap -> refuse */
      return 0;
  }
  return 1;
}
