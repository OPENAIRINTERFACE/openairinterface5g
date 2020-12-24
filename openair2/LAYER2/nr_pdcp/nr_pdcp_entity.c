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

#include "nr_pdcp_entity.h"

#include "nr_pdcp_entity_drb_am.h"

#include "LOG/log.h"

nr_pdcp_entity_t *new_nr_pdcp_entity_srb(
    int rb_id,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size),
    void *deliver_sdu_data,
    void (*deliver_pdu)(void *deliver_pdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size, int sdu_id),
    void *deliver_pdu_data)
{
  abort();
}

nr_pdcp_entity_t *new_nr_pdcp_entity_drb_am(
    int rb_id,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size),
    void *deliver_sdu_data,
    void (*deliver_pdu)(void *deliver_pdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size, int sdu_id),
    void *deliver_pdu_data,
    int sn_size,
    int t_reordering,
    int discard_timer)
{
  nr_pdcp_entity_drb_am_t *ret;

  ret = calloc(1, sizeof(nr_pdcp_entity_drb_am_t));
  if (ret == NULL) {
    LOG_E(PDCP, "%s:%d:%s: out of memory\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  ret->common.recv_pdu          = nr_pdcp_entity_drb_am_recv_pdu;
  ret->common.recv_sdu          = nr_pdcp_entity_drb_am_recv_sdu;
  ret->common.set_integrity_key = nr_pdcp_entity_drb_am_set_integrity_key;

  ret->common.delete = nr_pdcp_entity_drb_am_delete;

  ret->common.deliver_sdu = deliver_sdu;
  ret->common.deliver_sdu_data = deliver_sdu_data;

  ret->common.deliver_pdu = deliver_pdu;
  ret->common.deliver_pdu_data = deliver_pdu_data;

  ret->rb_id         = rb_id;
  ret->sn_size       = sn_size;
  ret->t_reordering  = t_reordering;
  ret->discard_timer = discard_timer;

  ret->common.maximum_nr_pdcp_sn = (1 << sn_size) - 1;

  return (nr_pdcp_entity_t *)ret;
}
