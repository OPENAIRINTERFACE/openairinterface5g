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

#include "rlc_sdu.h"

#include <stdlib.h>
#include <string.h>

#include "LOG/log.h"

rlc_sdu_t *rlc_new_sdu(char *buffer, int size, int upper_layer_id)
{
  rlc_sdu_t *ret = calloc(1, sizeof(rlc_sdu_t));
  if (ret == NULL)
    goto oom;

  ret->upper_layer_id = upper_layer_id;

  ret->data = malloc(size);
  if (ret->data == NULL)
    goto oom;

  memcpy(ret->data, buffer, size);

  ret->size = size;

  return ret;

oom:
  LOG_E(RLC, "%s:%d:%s: out of memory\n", __FILE__, __LINE__,  __FUNCTION__);
  exit(1);
}

void rlc_free_sdu(rlc_sdu_t *sdu)
{
  free(sdu->data);
  free(sdu);
}

void rlc_sdu_list_add(rlc_sdu_t **list, rlc_sdu_t **end, rlc_sdu_t *sdu)
{
  if (*list == NULL) {
    *list = sdu;
    *end = sdu;
    return;
  }

  (*end)->next = sdu;
  *end = sdu;
}
