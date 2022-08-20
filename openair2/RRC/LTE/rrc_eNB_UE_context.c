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

/*! \file rrc_eNB_UE_context.h
 * \brief rrc procedures for UE context
 * \author Lionel GAUTHIER
 * \date 2015
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "common/utils/LOG/log.h"
#include "rrc_eNB_UE_context.h"



//------------------------------------------------------------------------------
int rrc_eNB_compare_ue_rnti_id(
  struct rrc_eNB_ue_context_s *c1_pP, struct rrc_eNB_ue_context_s *c2_pP)
//------------------------------------------------------------------------------
{
  if (c1_pP->ue_id_rnti > c2_pP->ue_id_rnti) {
    return 1;
  }

  if (c1_pP->ue_id_rnti < c2_pP->ue_id_rnti) {
    return -1;
  }

  return 0;
}

/* Generate the tree management functions */
RB_GENERATE(rrc_ue_tree_s, rrc_eNB_ue_context_s, entries,
            rrc_eNB_compare_ue_rnti_id);



//------------------------------------------------------------------------------
struct rrc_eNB_ue_context_s *
rrc_eNB_allocate_new_UE_context(
  eNB_RRC_INST *rrc_instance_pP
)
//------------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s *new_p;
  new_p = (struct rrc_eNB_ue_context_s * )malloc(sizeof(struct rrc_eNB_ue_context_s));

  if (new_p == NULL) {
    LOG_E(RRC, "Cannot allocate new ue context\n");
    return NULL;
  }

  memset(new_p, 0, sizeof(struct rrc_eNB_ue_context_s));
  new_p->local_uid = uid_linear_allocator_new(&rrc_instance_pP->uid_allocator);

  for(int i = 0; i < NB_RB_MAX; i++) {
    new_p->ue_context.e_rab[i].xid = -1;
    new_p->ue_context.modify_e_rab[i].xid = -1;
  }

  return new_p;
}


//------------------------------------------------------------------------------
struct rrc_eNB_ue_context_s *
rrc_eNB_get_ue_context(
  eNB_RRC_INST *rrc_instance_pP,
  rnti_t rntiP)
//------------------------------------------------------------------------------
{
  rrc_eNB_ue_context_t temp;
  memset(&temp, 0, sizeof(struct rrc_eNB_ue_context_s));
  /* eNB ue rrc id = 24 bits wide */
  temp.ue_id_rnti = rntiP;
  struct rrc_eNB_ue_context_s   *ue_context_p = NULL;
  ue_context_p = RB_FIND(rrc_ue_tree_s, &rrc_instance_pP->rrc_ue_head, &temp);

  if ( ue_context_p != NULL) {
    return ue_context_p;
  } else {
    RB_FOREACH(ue_context_p, rrc_ue_tree_s, &(rrc_instance_pP->rrc_ue_head)) {
      if (ue_context_p->ue_context.rnti == rntiP) {
        return ue_context_p;
      }
    }
    return NULL;
  }
}


//------------------------------------------------------------------------------
struct rrc_eNB_ue_context_s *
rrc_eNB_find_ue_context_from_gnb_rnti(
  eNB_RRC_INST *rrc_instance_pP,
  int gnb_rnti)
//------------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s   *ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_ue_tree_s, &(rrc_instance_pP->rrc_ue_head)) {
    if (ue_context_p->ue_context.gnb_rnti == gnb_rnti) {
      return ue_context_p;
    }
  }
  return NULL;
}


//------------------------------------------------------------------------------
void rrc_eNB_remove_ue_context(
  const protocol_ctxt_t *const ctxt_pP,
  eNB_RRC_INST                *rrc_instance_pP,
  struct rrc_eNB_ue_context_s *ue_context_pP)
//------------------------------------------------------------------------------
{
  if (rrc_instance_pP == NULL) {
    LOG_E(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Bad RRC instance\n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
    return;
  }

  if (ue_context_pP == NULL) {
    LOG_E(RRC, PROTOCOL_RRC_CTXT_UE_FMT" Trying to free a NULL UE context\n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
    return;
  }

  RB_REMOVE(rrc_ue_tree_s, &rrc_instance_pP->rrc_ue_head, ue_context_pP);
  rrc_eNB_free_mem_UE_context(ctxt_pP, ue_context_pP);
  uid_linear_allocator_free(&rrc_instance_pP->uid_allocator, ue_context_pP->local_uid);
  free(ue_context_pP);
  rrc_instance_pP->Nb_ue --;
  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" Removed UE context\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
}


