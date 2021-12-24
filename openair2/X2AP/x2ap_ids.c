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

#include "x2ap_ids.h"

#include <string.h>

void x2ap_id_manager_init(x2ap_id_manager *m)
{
  int i;
  memset(m, 0, sizeof(x2ap_id_manager));
  for (i = 0; i < X2AP_MAX_IDS; i++)
    m->ids[i].rnti = -1;
}

int x2ap_allocate_new_id(x2ap_id_manager *m)
{
  int i;
  for (i = 0; i < X2AP_MAX_IDS; i++)
    if (m->ids[i].rnti == -1) {
      m->ids[i].rnti = 0;
      m->ids[i].id_source = -1;
      m->ids[i].id_target = -1;
      return i;
    }
  return -1;
}

void x2ap_release_id(x2ap_id_manager *m, int id)
{
  m->ids[id].rnti = -1;
}

int x2ap_find_id(x2ap_id_manager *m, int id_source, int id_target)
{
  int i;
  for (i = 0; i < X2AP_MAX_IDS; i++)
    if (m->ids[i].rnti != -1 &&
        m->ids[i].id_source == id_source &&
        m->ids[i].id_target == id_target)
      return i;
  return -1;
}

int x2ap_find_id_from_id_source(x2ap_id_manager *m, int id_source)
{
  int i;
  for (i = 0; i < X2AP_MAX_IDS; i++)
    if (m->ids[i].rnti != -1 &&
        m->ids[i].id_source == id_source)
      return i;
  return -1;
}

int x2ap_find_id_from_id_target(x2ap_id_manager *m, int id_target)
{
  int i;
  for (i = 0; i < X2AP_MAX_IDS; i++)
    if (m->ids[i].rnti != -1 &&
        m->ids[i].id_target == id_target)
      return i;
  return -1;
}

int x2ap_find_id_from_rnti(x2ap_id_manager *m, int rnti)
{
  int i;
  for (i = 0; i < X2AP_MAX_IDS; i++)
    if (m->ids[i].rnti == rnti)
      return i;
  return -1;
}

void x2ap_set_ids(x2ap_id_manager *m, int ue_id, int rnti, int id_source, int id_target)
{
  m->ids[ue_id].rnti      = rnti;
  m->ids[ue_id].id_source = id_source;
  m->ids[ue_id].id_target = id_target;
}

/* real type of target is x2ap_eNB_data_t * */
void x2ap_id_set_target(x2ap_id_manager *m, int ue_id, void *target)
{
  m->ids[ue_id].target = target;
}

void x2ap_id_set_state(x2ap_id_manager *m, int ue_id, x2id_state_t state)
{
  m->ids[ue_id].state = state;
}

void x2ap_set_reloc_prep_timer(x2ap_id_manager *m, int ue_id, uint64_t time)
{
  m->ids[ue_id].t_reloc_prep_start = time;
}

void x2ap_set_reloc_overall_timer(x2ap_id_manager *m, int ue_id, uint64_t time)
{
  m->ids[ue_id].tx2_reloc_overall_start = time;
}

void x2ap_set_dc_prep_timer(x2ap_id_manager *m, int ue_id, uint64_t time)
{
  m->ids[ue_id].t_dc_prep_start = time;
}

void x2ap_set_dc_overall_timer(x2ap_id_manager *m, int ue_id, uint64_t time)
{
  m->ids[ue_id].t_dc_overall_start = time;
}

int x2ap_id_get_id_source(x2ap_id_manager *m, int ue_id)
{
  return m->ids[ue_id].id_source;
}

int x2ap_id_get_id_target(x2ap_id_manager *m, int ue_id)
{
  return m->ids[ue_id].id_target;
}

int x2ap_id_get_rnti(x2ap_id_manager *m, int ue_id)
{
  return m->ids[ue_id].rnti;
}

void *x2ap_id_get_target(x2ap_id_manager *m, int ue_id)
{
  return m->ids[ue_id].target;
}
