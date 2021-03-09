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

#ifndef _NR_PDCP_ENTITY_SRB_H_
#define _NR_PDCP_ENTITY_SRB_H_

#include "nr_pdcp_entity.h"

typedef struct {
  nr_pdcp_entity_t common;
  int srb_id;
} nr_pdcp_entity_srb_t;

void nr_pdcp_entity_srb_recv_pdu(nr_pdcp_entity_t *_entity, char *buffer, int size);
void nr_pdcp_entity_srb_recv_sdu(nr_pdcp_entity_t *_entity, char *buffer, int size, int sdu_id);
void nr_pdcp_entity_srb_set_integrity_key(nr_pdcp_entity_t *_entity, char *key);
void nr_pdcp_entity_srb_delete(nr_pdcp_entity_t *_entity);


#endif /* _NR_PDCP_ENTITY_SRB_H_ */
