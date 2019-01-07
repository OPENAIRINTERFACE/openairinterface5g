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

/*! \file s1ap_eNB_ue_context.c
 * \brief s1ap UE context management within eNB
 * \author Sebastien ROUX <sebastien.roux@eurecom.fr>
 * \date 2012
 * \version 0.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "tree.h"

#include "intertask_interface.h"

#include "s1ap_common.h"
#include "s1ap_eNB_defs.h"
#include "s1ap_eNB_ue_context.h"

int s1ap_eNB_compare_eNB_ue_s1ap_id(
  struct s1ap_eNB_ue_context_s *p1, struct s1ap_eNB_ue_context_s *p2)
{
  if (p1->eNB_ue_s1ap_id > p2->eNB_ue_s1ap_id) {
    return 1;
  }

  if (p1->eNB_ue_s1ap_id < p2->eNB_ue_s1ap_id) {
    return -1;
  }

  return 0;
}

/* Generate the tree management functions */
RB_GENERATE(s1ap_ue_map, s1ap_eNB_ue_context_s, entries,
            s1ap_eNB_compare_eNB_ue_s1ap_id);

struct s1ap_eNB_ue_context_s *s1ap_eNB_allocate_new_UE_context(void)
{
  struct s1ap_eNB_ue_context_s *new_p;

  new_p = malloc(sizeof(struct s1ap_eNB_ue_context_s));

  if (new_p == NULL) {
    S1AP_ERROR("Cannot allocate new ue context\n");
    return NULL;
  }

  memset(new_p, 0, sizeof(struct s1ap_eNB_ue_context_s));

  return new_p;
}

struct s1ap_eNB_ue_context_s *s1ap_eNB_get_ue_context(
  s1ap_eNB_instance_t *instance_p,
  uint32_t eNB_ue_s1ap_id)
{
  s1ap_eNB_ue_context_t temp;

  memset(&temp, 0, sizeof(struct s1ap_eNB_ue_context_s));

  /* eNB ue s1ap id = 24 bits wide */
  temp.eNB_ue_s1ap_id = eNB_ue_s1ap_id & 0x00FFFFFF;

  return RB_FIND(s1ap_ue_map, &instance_p->s1ap_ue_head, &temp);
}

void s1ap_eNB_free_ue_context(struct s1ap_eNB_ue_context_s *ue_context_p)
{
  if (ue_context_p == NULL) {
    S1AP_ERROR("Trying to free a NULL context\n");
    return;
  }

  /* TODO: check that context is currently not in the tree of known
   * contexts.
   */

  free(ue_context_p);
}
