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

#ifndef X2AP_IDS_H_
#define X2AP_IDS_H_

#include <stdint.h>

/* maximum number of simultaneous handovers, do not set too high */
#define X2AP_MAX_IDS	16

/*
 * state:
 * - when starting handover in source, UE is in state X2ID_STATE_SOURCE_PREPARE
 * - after receiving HO_ack in source, UE is in state X2ID_STATE_SOURCE_OVERALL
 * - in target, UE is in state X2ID_STATE_TARGET
 * The state is used to check timers.
 */
typedef enum {
  X2ID_STATE_SOURCE_PREPARE,
  X2ID_STATE_SOURCE_OVERALL,
  X2ID_STATE_TARGET,
  X2ID_STATE_NSA_ENB_PREPARE,
  X2ID_STATE_NSA_GNB_OVERALL,
} x2id_state_t;

typedef struct {
  int           rnti;             /* -1 when free */
  int           id_source;
  int           id_target;

  /* the target eNB. Real type is x2ap_eNB_data_t * */
  void          *target;

  /* state: needed to check timers */
  x2id_state_t  state;

  /* timers */
  uint64_t      t_reloc_prep_start;
  uint64_t      tx2_reloc_overall_start;
  uint64_t      t_dc_prep_start;
  uint64_t      t_dc_overall_start;
} x2ap_id;

typedef struct {
  x2ap_id ids[X2AP_MAX_IDS];
} x2ap_id_manager;

void x2ap_id_manager_init(x2ap_id_manager *m);
int x2ap_allocate_new_id(x2ap_id_manager *m);
void x2ap_release_id(x2ap_id_manager *m, int id);
int x2ap_find_id(x2ap_id_manager *, int id_source, int id_target);
int x2ap_find_id_from_id_source(x2ap_id_manager *, int id_source);
int x2ap_find_id_from_id_target(x2ap_id_manager *, int id_source);
int x2ap_find_id_from_rnti(x2ap_id_manager *, int rnti);
void x2ap_set_ids(x2ap_id_manager *m, int ue_id, int rnti, int id_source, int id_target);
void x2ap_id_set_state(x2ap_id_manager *m, int ue_id, x2id_state_t state);
/* real type of target is x2ap_eNB_data_t * */
void x2ap_id_set_target(x2ap_id_manager *m, int ue_id, void *target);
void x2ap_set_reloc_prep_timer(x2ap_id_manager *m, int ue_id, uint64_t time);
void x2ap_set_reloc_overall_timer(x2ap_id_manager *m, int ue_id, uint64_t time);
void x2ap_set_dc_prep_timer(x2ap_id_manager *m, int ue_id, uint64_t time);
void x2ap_set_dc_overall_timer(x2ap_id_manager *m, int ue_id, uint64_t time);
int x2ap_id_get_id_source(x2ap_id_manager *m, int ue_id);
int x2ap_id_get_id_target(x2ap_id_manager *m, int ue_id);
int x2ap_id_get_rnti(x2ap_id_manager *m, int ue_id);
void *x2ap_id_get_target(x2ap_id_manager *m, int ue_id);

#endif /* X2AP_IDS_H_ */
