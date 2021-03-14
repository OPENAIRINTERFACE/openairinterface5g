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

#ifndef _NR_PDCP_ENTITY_DRB_AM_H_
#define _NR_PDCP_ENTITY_DRB_AM_H_

#include "nr_pdcp_entity.h"

typedef struct {
  nr_pdcp_entity_t common;
  int rb_id;
  int sn_size;               /* unit: bits */
  int t_reordering;          /* unit: ms */
  int discard_timer;         /* unit: ms, -1 means infinity */
} nr_pdcp_entity_drb_am_t;

void nr_pdcp_entity_drb_am_recv_pdu(nr_pdcp_entity_t *entity, char *buffer, int size);
void nr_pdcp_entity_drb_am_recv_sdu(nr_pdcp_entity_t *entity, char *buffer, int size,
                                    int sdu_id);
void nr_pdcp_entity_drb_am_set_integrity_key(nr_pdcp_entity_t *entity, char *key);
void nr_pdcp_entity_drb_am_delete(nr_pdcp_entity_t *entity);

#endif /* _NR_PDCP_ENTITY_DRB_AM_H_ */
