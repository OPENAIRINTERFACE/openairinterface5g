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

#include "nr_pdcp_entity_srb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void nr_pdcp_entity_srb_recv_pdu(nr_pdcp_entity_t *_entity, char *buffer, int size)
{
  nr_pdcp_entity_srb_t *entity = (nr_pdcp_entity_srb_t *)_entity;

  if (size < 2) abort();
  entity->common.deliver_sdu(entity->common.deliver_sdu_data,
                             (nr_pdcp_entity_t *)entity, buffer+2, size-6);
}

void nr_pdcp_entity_srb_recv_sdu(nr_pdcp_entity_t *_entity, char *buffer, int size,
                              int sdu_id)
{
  nr_pdcp_entity_srb_t *entity = (nr_pdcp_entity_srb_t *)_entity;
  int sn;
  char buf[size+6];

  sn = entity->common.next_nr_pdcp_tx_sn;

  entity->common.next_nr_pdcp_tx_sn++;
  if (entity->common.next_nr_pdcp_tx_sn > entity->common.maximum_nr_pdcp_sn) {
    entity->common.next_nr_pdcp_tx_sn = 0;
    entity->common.tx_hfn++;
  }

  buf[0] = (sn >> 8) & 0x0f;
  buf[1] = sn & 0xff;
  memcpy(buf+2, buffer, size);

  /* For now use padding for the MAC-I bytes (normally carrying message authentication code)
   * which come after the data payload bytes (38.323, section 6.2.2.1) */
  for (int i=size+2; i<size+6; i++)
    buf[i] = 0x11*(i-size-1);

  entity->common.deliver_pdu(entity->common.deliver_pdu_data,
                             (nr_pdcp_entity_t *)entity, buf, size+6, sdu_id);
}

void nr_pdcp_entity_srb_set_integrity_key(nr_pdcp_entity_t *_entity, char *key)
{
  /* nothing to do */
}

void nr_pdcp_entity_srb_delete(nr_pdcp_entity_t *_entity)
{
  nr_pdcp_entity_srb_t *entity = (nr_pdcp_entity_srb_t *)_entity;
  free(entity);
}

