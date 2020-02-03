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

#ifndef M2AP_IDS_H_
#define M2AP_IDS_H_

#include <stdint.h>

/* maximum number of simultaneous handovers, do not set too high */
#define M2AP_MAX_IDS	16

/*
 * state:
 * - when starting handover in source, UE is in state M2ID_STATE_SOURCE_PREPARE
 * - after receiving HO_ack in source, UE is in state M2ID_STATE_SOURCE_OVERALL
 * - in target, UE is in state X2ID_STATE_TARGET
 * The state is used to check timers.
 */
typedef enum {
  M2ID_STATE_SOURCE_PREPARE,
  M2ID_STATE_SOURCE_OVERALL,
  M2ID_STATE_TARGET
} m2id_state_t;

typedef struct {
  int           rnti;             /* -1 when free */
  int           id_source;
  int           id_target;

  /* the target eNB. Real type is m2ap_eNB_data_t * */
  void          *target;

  /* state: needed to check timers */
  m2id_state_t  state;

  /* timers */
  uint64_t      t_reloc_prep_start;
  uint64_t      tm2_reloc_overall_start;
} m2ap_id;

typedef struct {
  m2ap_id ids[M2AP_MAX_IDS];
} m2ap_id_manager;

void m2ap_id_manager_init(m2ap_id_manager *m);
int m2ap_allocate_new_id(m2ap_id_manager *m);
void m2ap_release_id(m2ap_id_manager *m, int id);
int m2ap_find_id(m2ap_id_manager *, int id_source, int id_target);
int m2ap_find_id_from_id_source(m2ap_id_manager *, int id_source);
int m2ap_find_id_from_rnti(m2ap_id_manager *, int rnti);
void m2ap_set_ids(m2ap_id_manager *m, int ue_id, int rnti, int id_source, int id_target);
void m2ap_id_set_state(m2ap_id_manager *m, int ue_id, m2id_state_t state);
/* real type of target is m2ap_eNB_data_t * */
void m2ap_id_set_target(m2ap_id_manager *m, int ue_id, void *target);
void m2ap_set_reloc_prep_timer(m2ap_id_manager *m, int ue_id, uint64_t time);
void m2ap_set_reloc_overall_timer(m2ap_id_manager *m, int ue_id, uint64_t time);
int m2ap_id_get_id_source(m2ap_id_manager *m, int ue_id);
int m2ap_id_get_id_target(m2ap_id_manager *m, int ue_id);
int m2ap_id_get_rnti(m2ap_id_manager *m, int ue_id);
void *m2ap_id_get_target(m2ap_id_manager *m, int ue_id);

#endif /* M2AP_IDS_H_ */
