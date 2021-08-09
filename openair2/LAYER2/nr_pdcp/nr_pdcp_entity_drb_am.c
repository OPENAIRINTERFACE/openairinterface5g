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

#include "nr_pdcp_entity_drb_am.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/utils/LOG/log.h"
#include "assertions.h"

void nr_pdcp_entity_drb_am_recv_pdu(nr_pdcp_entity_t *_entity, char *buffer, int size)
{
  nr_pdcp_entity_drb_am_t *entity = (nr_pdcp_entity_drb_am_t *)_entity;
  int sn;

  if (size < 3) {
    LOG_I(PDCP, "Size < 3. Size = %d. No data, so discarding.", size);
    return;
  }

  if (!(buffer[0] & 0x80))
    LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);

  sn = (((unsigned char)buffer[0] & 0x3) << 16) |
        ((unsigned char)buffer[1]        <<  8) |
         (unsigned char)buffer[2];

  if (entity->common.has_ciphering)
    entity->common.cipher(entity->common.security_context, (unsigned char *)buffer+3, size-3,
                          entity->rb_id, sn, entity->common.is_gnb ? 0 : 1);
  LOG_I(RLC, "Melissa Elkadi, calling deliver SDU to send to GTP from PDCP %s\n", __FUNCTION__);
  entity->common.deliver_sdu(entity->common.deliver_sdu_data,
                             (nr_pdcp_entity_t *)entity, buffer+3, size-3);
}

void nr_pdcp_entity_drb_am_recv_sdu(nr_pdcp_entity_t *_entity, char *buffer, int size,
                              int sdu_id)
{
  nr_pdcp_entity_drb_am_t *entity = (nr_pdcp_entity_drb_am_t *)_entity;
  int sn;
  char buf[size+3];
  char buf_mel[1024];
  hexdump(buffer, size, buf_mel, sizeof(buf_mel));
  LOG_I(PDCP, "Melissa Elkadi in %s, this is hexdump of pdu %s recevied in PDCP layer directly\n",
        __FUNCTION__, buf_mel);

  sn = entity->common.next_nr_pdcp_tx_sn;

  entity->common.next_nr_pdcp_tx_sn++;
  if (entity->common.next_nr_pdcp_tx_sn > entity->common.maximum_nr_pdcp_sn) {
    entity->common.next_nr_pdcp_tx_sn = 0;
    entity->common.tx_hfn++;
  }

  buf[0] = 0x80 | ((sn >> 16) & 0x3);
  buf[1] = (sn >> 8) & 0xff;
  buf[2] = sn & 0xff;
  memcpy(buf+3, buffer, size);
  char buf_melissa[1024];
  hexdump(buf+3, size, buf_melissa, sizeof(buf_melissa));
  LOG_I(PDCP, "Melissa Elkadi in %s, this is hexdump of pdu %s copied into buf+3 in PDCP. And this is the sdu_id %d\n",
        __FUNCTION__, buf_melissa, sdu_id);

  if (entity->common.has_ciphering)
    entity->common.cipher(entity->common.security_context, (unsigned char *)buf+3, size,
                          entity->rb_id, sn, entity->common.is_gnb ? 1 : 0);

  entity->common.deliver_pdu(entity->common.deliver_pdu_data,
                             (nr_pdcp_entity_t *)entity, buf, size+3, sdu_id);
  char buf_meli[1024];
  hexdump(buf, size + 3, buf_meli, sizeof(buf_meli));
  LOG_I(PDCP, "Melissa Elkadi in %s, this is hexdump of pdu %s After delivering to RLC\n",
        __FUNCTION__, buf_meli);
}

void nr_pdcp_entity_drb_am_set_integrity_key(nr_pdcp_entity_t *_entity, char *key)
{
  /* nothing to do */
}

void nr_pdcp_entity_drb_am_delete(nr_pdcp_entity_t *_entity)
{
  nr_pdcp_entity_drb_am_t *entity = (nr_pdcp_entity_drb_am_t *)_entity;
  if (entity->common.free_security != NULL)
    entity->common.free_security(entity->common.security_context);
  free(entity);
}
