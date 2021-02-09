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

/*! \file ngap_gNB_ue_context.c
 * \brief ngap UE context management within gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \version 0.1
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "tree.h"

#include "intertask_interface.h"

#include "ngap_common.h"
#include "ngap_gNB_defs.h"
#include "ngap_gNB_ue_context.h"

int ngap_gNB_compare_gNB_ue_ngap_id(
  struct ngap_gNB_ue_context_s *p1, struct ngap_gNB_ue_context_s *p2)
{
  if (p1->gNB_ue_ngap_id > p2->gNB_ue_ngap_id) {
    return 1;
  }

  if (p1->gNB_ue_ngap_id < p2->gNB_ue_ngap_id) {
    return -1;
  }

  return 0;
}

/* Generate the tree management functions */
RB_GENERATE(ngap_ue_map, ngap_gNB_ue_context_s, entries,
            ngap_gNB_compare_gNB_ue_ngap_id);

struct ngap_gNB_ue_context_s *ngap_gNB_allocate_new_UE_context(void)
{
  struct ngap_gNB_ue_context_s *new_p;

  new_p = malloc(sizeof(struct ngap_gNB_ue_context_s));

  if (new_p == NULL) {
    NGAP_ERROR("Cannot allocate new ue context\n");
    return NULL;
  }

  memset(new_p, 0, sizeof(struct ngap_gNB_ue_context_s));

  return new_p;
}

struct ngap_gNB_ue_context_s *ngap_gNB_get_ue_context(
  ngap_gNB_instance_t *instance_p,
  uint32_t gNB_ue_ngap_id)
{
  ngap_gNB_ue_context_t temp;

  memset(&temp, 0, sizeof(struct ngap_gNB_ue_context_s));

  /* gNB ue ngap id = 32 bits wide */
  temp.gNB_ue_ngap_id = gNB_ue_ngap_id & 0xFFFFFFFF;

  return RB_FIND(ngap_ue_map, &instance_p->ngap_ue_head, &temp);
}

void ngap_gNB_free_ue_context(struct ngap_gNB_ue_context_s *ue_context_p)
{
  if (ue_context_p == NULL) {
    NGAP_ERROR("Trying to free a NULL context\n");
    return;
  }

  /* TODO: check that context is currently not in the tree of known
   * contexts.
   */

  free(ue_context_p);
}
