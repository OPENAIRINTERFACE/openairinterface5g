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

#ifndef _NR_RLC_UE_MANAGER_H_
#define _NR_RLC_UE_MANAGER_H_

#include "nr_rlc_entity.h"

typedef void nr_rlc_ue_manager_t;

typedef struct nr_rlc_ue_t {
  int rnti;
  nr_rlc_entity_t *srb[3];
  nr_rlc_entity_t *drb[5];
} nr_rlc_ue_t;

/***********************************************************************/
/* manager functions                                                   */
/***********************************************************************/

nr_rlc_ue_manager_t *new_nr_rlc_ue_manager(int enb_flag);

int nr_rlc_manager_get_enb_flag(nr_rlc_ue_manager_t *m);

void nr_rlc_manager_lock(nr_rlc_ue_manager_t *m);
void nr_rlc_manager_unlock(nr_rlc_ue_manager_t *m);

nr_rlc_ue_t *nr_rlc_manager_get_ue(nr_rlc_ue_manager_t *m, int rnti);
void nr_rlc_manager_remove_ue(nr_rlc_ue_manager_t *m, int rnti);

/***********************************************************************/
/* ue functions                                                        */
/***********************************************************************/

void nr_rlc_ue_add_srb_rlc_entity(nr_rlc_ue_t *ue, int srb_id, nr_rlc_entity_t *entity);
void nr_rlc_ue_add_drb_rlc_entity(nr_rlc_ue_t *ue, int drb_id, nr_rlc_entity_t *entity);

#endif /* _NR_RLC_UE_MANAGER_H_ */
