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

#ifndef _NR_PDCP_UE_MANAGER_H_
#define _NR_PDCP_UE_MANAGER_H_

#include "nr_pdcp_entity.h"

#include "common/platform_types.h"

typedef void nr_pdcp_ue_manager_t;

typedef struct nr_pdcp_ue_t {
  ue_id_t ue_id;
  nr_pdcp_entity_t *srb[3];
  nr_pdcp_entity_t *drb[MAX_DRBS_PER_UE];
} nr_pdcp_ue_t;

/***********************************************************************/
/* manager functions                                                   */
/***********************************************************************/

nr_pdcp_ue_manager_t *new_nr_pdcp_ue_manager(int enb_flag);

int nr_pdcp_manager_get_enb_flag(nr_pdcp_ue_manager_t *m);

void nr_pdcp_manager_lock(nr_pdcp_ue_manager_t *m);
void nr_pdcp_manager_unlock(nr_pdcp_ue_manager_t *m);

nr_pdcp_ue_t *nr_pdcp_manager_get_ue(nr_pdcp_ue_manager_t *m, ue_id_t UEid);
void nr_pdcp_manager_remove_ue(nr_pdcp_ue_manager_t *m, ue_id_t UEid);

nr_pdcp_ue_t **nr_pdcp_manager_get_ue_list(nr_pdcp_ue_manager_t *_m);
int nr_pdcp_manager_get_ue_count(nr_pdcp_ue_manager_t *_m);

/***********************************************************************/
/* ue functions                                                        */
/***********************************************************************/

void nr_pdcp_ue_add_srb_pdcp_entity(nr_pdcp_ue_t *ue, int srb_id,
                                    nr_pdcp_entity_t *entity);
void nr_pdcp_ue_add_drb_pdcp_entity(nr_pdcp_ue_t *ue, int drb_id,
                                    nr_pdcp_entity_t *entity);

/***********************************************************************/
/* hacks                                                               */
/***********************************************************************/

/* returns 0 if no UE, 1 otherwise */
bool nr_pdcp_get_first_ue_id(nr_pdcp_ue_manager_t *m, ue_id_t *ret);

#endif /* _NR_PDCP_UE_MANAGER_H_ */
