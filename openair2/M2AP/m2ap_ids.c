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

#include "m2ap_ids.h"

#include <string.h>

void m2ap_id_manager_init(m2ap_id_manager *m)
{
  int i;
  memset(m, 0, sizeof(m2ap_id_manager));
  for (i = 0; i < M2AP_MAX_IDS; i++)
    m->ids[i].rnti = -1;
}

int m2ap_allocate_new_id(m2ap_id_manager *m)
{
  int i;
  for (i = 0; i < M2AP_MAX_IDS; i++)
    if (m->ids[i].rnti == -1) {
      m->ids[i].rnti = 0;
      m->ids[i].id_source = -1;
      m->ids[i].id_target = -1;
      return i;
    }
  return -1;
}

void m2ap_release_id(m2ap_id_manager *m, int id)
{
  m->ids[id].rnti = -1;
}

int m2ap_find_id(m2ap_id_manager *m, int id_source, int id_target)
{
  int i;
  for (i = 0; i < M2AP_MAX_IDS; i++)
    if (m->ids[i].rnti != -1 &&
        m->ids[i].id_source == id_source &&
        m->ids[i].id_target == id_target)
      return i;
  return -1;
}

int m2ap_find_id_from_id_source(m2ap_id_manager *m, int id_source)
{
  int i;
  for (i = 0; i < M2AP_MAX_IDS; i++)
    if (m->ids[i].rnti != -1 &&
        m->ids[i].id_source == id_source)
      return i;
  return -1;
}

int m2ap_find_id_from_rnti(m2ap_id_manager *m, int rnti)
{
  int i;
  for (i = 0; i < M2AP_MAX_IDS; i++)
    if (m->ids[i].rnti == rnti)
      return i;
  return -1;
}

void m2ap_set_ids(m2ap_id_manager *m, int ue_id, int rnti, int id_source, int id_target)
{
  m->ids[ue_id].rnti      = rnti;
  m->ids[ue_id].id_source = id_source;
  m->ids[ue_id].id_target = id_target;
}

/* real type of target is m2ap_eNB_data_t * */
void m2ap_id_set_target(m2ap_id_manager *m, int ue_id, void *target)
{
  m->ids[ue_id].target = target;
}

void m2ap_id_set_state(m2ap_id_manager *m, int ue_id, m2id_state_t state)
{
  m->ids[ue_id].state = state;
}

void m2ap_set_reloc_prep_timer(m2ap_id_manager *m, int ue_id, uint64_t time)
{
  m->ids[ue_id].t_reloc_prep_start = time;
}

void m2ap_set_reloc_overall_timer(m2ap_id_manager *m, int ue_id, uint64_t time)
{
  m->ids[ue_id].tm2_reloc_overall_start = time;
}

int m2ap_id_get_id_source(m2ap_id_manager *m, int ue_id)
{
  return m->ids[ue_id].id_source;
}

int m2ap_id_get_id_target(m2ap_id_manager *m, int ue_id)
{
  return m->ids[ue_id].id_target;
}

int m2ap_id_get_rnti(m2ap_id_manager *m, int ue_id)
{
  return m->ids[ue_id].rnti;
}

void *m2ap_id_get_target(m2ap_id_manager *m, int ue_id)
{
  return m->ids[ue_id].target;
}
