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

#include "nr_pdcp_sdu.h"

#include <stdlib.h>
#include <string.h>

nr_pdcp_sdu_t *nr_pdcp_new_sdu(uint32_t count, char *buffer, int size)
{
  nr_pdcp_sdu_t *ret = calloc(1, sizeof(nr_pdcp_sdu_t));
  if (ret == NULL)
    exit(1);
  ret->count = count;
  ret->buffer = malloc(size);
  if (ret->buffer == NULL)
    exit(1);
  memcpy(ret->buffer, buffer, size);
  ret->size = size;
  return ret;
}

nr_pdcp_sdu_t *nr_pdcp_sdu_list_add(nr_pdcp_sdu_t *l, nr_pdcp_sdu_t *sdu)
{
  nr_pdcp_sdu_t head;
  nr_pdcp_sdu_t *cur;
  nr_pdcp_sdu_t *prev;

  head.next = l;
  cur = l;
  prev = &head;

  /* order is by 'count' */
  while (cur != NULL) {
    /* check if 'sdu' is before 'cur' in the list */
    if (sdu->count < cur->count)
      break;
    prev = cur;
    cur = cur->next;
  }
  prev->next = sdu;
  sdu->next = cur;
  return head.next;
}

int nr_pdcp_sdu_in_list(nr_pdcp_sdu_t *l, uint32_t count)
{
  while (l != NULL) {
    if (l->count == count)
      return 1;
    l = l->next;
  }
  return 0;
}

void nr_pdcp_free_sdu(nr_pdcp_sdu_t *sdu)
{
  free(sdu->buffer);
  free(sdu);
}
