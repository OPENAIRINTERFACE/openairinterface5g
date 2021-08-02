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

/*! \file rrc_gNB_UE_context.h
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
#include "rrc_gNB_UE_context.h"
#include "msc.h"


//------------------------------------------------------------------------------
void nr_uid_linear_allocator_init(
  nr_uid_allocator_t *const uid_pP
)
//------------------------------------------------------------------------------
{
  memset(uid_pP, 0, sizeof(nr_uid_allocator_t));
}

//------------------------------------------------------------------------------
uid_nr_t nr_uid_linear_allocator_new(gNB_RRC_INST *const rrc_instance_pP)
//------------------------------------------------------------------------------
{
  unsigned int i;
  unsigned int bit_index = 1;
  uid_nr_t        uid = 0;
  nr_uid_allocator_t *uia_p = &rrc_instance_pP->uid_allocator;

  for (i=0; i < UID_LINEAR_ALLOCATOR_BITMAP_SIZE; i++) {
    if (uia_p->bitmap[i] != UINT_MAX) {
      bit_index = 1;
      uid       = 0;

      while ((uia_p->bitmap[i] & bit_index) == bit_index) {
        bit_index = bit_index << 1;
        uid       += 1;
      }

      uia_p->bitmap[i] |= bit_index;
      return uid + (i*sizeof(unsigned int)*8);
    } 
  }

  return UINT_MAX;
}


//------------------------------------------------------------------------------
void
nr_uid_linear_allocator_free(
			     gNB_RRC_INST *rrc_instance_pP,
			     uid_nr_t uidP
)
//------------------------------------------------------------------------------
{
  unsigned int i = uidP/sizeof(unsigned int)/8;
  unsigned int bit = uidP % (sizeof(unsigned int) * 8);
  unsigned int value = ~(0x00000001 << bit);

  if (i < UID_LINEAR_ALLOCATOR_BITMAP_SIZE) {
    rrc_instance_pP->uid_allocator.bitmap[i] &=  value;
  }
}


//------------------------------------------------------------------------------
int rrc_gNB_compare_ue_rnti_id(
  struct rrc_gNB_ue_context_s *c1_pP, struct rrc_gNB_ue_context_s *c2_pP)
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
RB_GENERATE(rrc_nr_ue_tree_s, rrc_gNB_ue_context_s, entries,
            rrc_gNB_compare_ue_rnti_id);



//------------------------------------------------------------------------------
struct rrc_gNB_ue_context_s *
rrc_gNB_allocate_new_UE_context(
  gNB_RRC_INST *rrc_instance_pP
)
//------------------------------------------------------------------------------
{
  struct rrc_gNB_ue_context_s *new_p;
  new_p = (struct rrc_gNB_ue_context_s * )malloc(sizeof(struct rrc_gNB_ue_context_s));

  if (new_p == NULL) {
    LOG_E(RRC, "Cannot allocate new ue context\n");
    return NULL;
  }

  memset(new_p, 0, sizeof(struct rrc_gNB_ue_context_s));
  new_p->local_uid = nr_uid_linear_allocator_new(rrc_instance_pP);

  for(int i = 0; i < NB_RB_MAX; i++) {
    new_p->ue_context.e_rab[i].xid = -1;
    new_p->ue_context.pduSession[i].xid = -1;
    new_p->ue_context.modify_e_rab[i].xid = -1;
  }

  LOG_I(NR_RRC,"Returning new UE context at %p\n",new_p);
  return(new_p);
}


//------------------------------------------------------------------------------
struct rrc_gNB_ue_context_s *
rrc_gNB_get_ue_context(
  gNB_RRC_INST *rrc_instance_pP,
  rnti_t rntiP)
//------------------------------------------------------------------------------
{
  rrc_gNB_ue_context_t temp;
  memset(&temp, 0, sizeof(struct rrc_gNB_ue_context_s));
  /* gNB ue rrc id = 24 bits wide */
  temp.ue_id_rnti = rntiP;
  struct rrc_gNB_ue_context_s   *ue_context_p = NULL;
  ue_context_p = RB_FIND(rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head, &temp);

  if ( ue_context_p != NULL) {
    return ue_context_p;
  } else {
    RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &(rrc_instance_pP->rrc_ue_head)) {
      if (ue_context_p->ue_context.rnti == rntiP) {
        return ue_context_p;
      }
    }
    return NULL;
  }
}

void rrc_gNB_free_mem_UE_context(
  const protocol_ctxt_t               *const ctxt_pP,
  struct rrc_gNB_ue_context_s         *const ue_context_pP
)
//-----------------------------------------------------------------------------
{

  LOG_T(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" Clearing UE context 0x%p (free internal structs)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
        ue_context_pP);

  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_LTE_SCellToAddMod_r10, &ue_context_pP->ue_context.sCell_config[0]);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_LTE_SCellToAddMod_r10, &ue_context_pP->ue_context.sCell_config[1]);

  // empty the internal fields of the UE context here
}

//------------------------------------------------------------------------------
void rrc_gNB_remove_ue_context(
  const protocol_ctxt_t       *const ctxt_pP,
  gNB_RRC_INST                *rrc_instance_pP,
  struct rrc_gNB_ue_context_s *ue_context_pP)
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

  RB_REMOVE(rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head, ue_context_pP);
  MSC_LOG_EVENT(
    MSC_RRC_ENB,
    "0 Removed UE %"PRIx16" ",
    ue_context_pP->ue_context.rnti);
  rrc_gNB_free_mem_UE_context(ctxt_pP, ue_context_pP);
  nr_uid_linear_allocator_free(rrc_instance_pP, ue_context_pP->local_uid);
  free(ue_context_pP);
  rrc_instance_pP->Nb_ue --;
  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" Removed UE context\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
}

//-----------------------------------------------------------------------------
// return the ue context if there is already an UE with ue_identityP, NULL otherwise
struct rrc_gNB_ue_context_s *
rrc_gNB_ue_context_random_exist(
  gNB_RRC_INST                *rrc_instance_pP,
  const uint64_t               ue_identityP
)
//-----------------------------------------------------------------------------
{
  struct rrc_gNB_ue_context_s        *ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head) {
    if (ue_context_p->ue_context.random_ue_identity == ue_identityP)
      return ue_context_p;
  }
  return NULL;
}

//-----------------------------------------------------------------------------
// return the ue context if there is already an UE with the same S-TMSI, NULL otherwise
struct rrc_gNB_ue_context_s *
rrc_gNB_ue_context_5g_s_tmsi_exist(
    gNB_RRC_INST                *rrc_instance_pP,
    const uint64_t              s_TMSI
)
//-----------------------------------------------------------------------------
{
    struct rrc_gNB_ue_context_s        *ue_context_p = NULL;
    RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head) {
        LOG_I(NR_RRC,"checking for UE 5G S-TMSI %ld: rnti %d \n",
              s_TMSI, ue_context_p->ue_context.rnti);

        if (ue_context_p->ue_context.ng_5G_S_TMSI_Part1 == s_TMSI) {
            return ue_context_p;
        }
    }
    return NULL;
}

//-----------------------------------------------------------------------------
// return a new ue context structure if ue_identityP, ctxt_pP->rnti not found in collection
struct rrc_gNB_ue_context_s *
rrc_gNB_get_next_free_ue_context(
  const protocol_ctxt_t       *const ctxt_pP,
  gNB_RRC_INST                *rrc_instance_pP,
  const uint64_t               ue_identityP
)
//-----------------------------------------------------------------------------
{
  struct rrc_gNB_ue_context_s        *ue_context_p = NULL;
  ue_context_p = rrc_gNB_get_ue_context(rrc_instance_pP, ctxt_pP->rnti);

  if (ue_context_p == NULL) {
    ue_context_p = rrc_gNB_allocate_new_UE_context(rrc_instance_pP);

    if (ue_context_p == NULL) {
      LOG_E(NR_RRC,
            PROTOCOL_NR_RRC_CTXT_UE_FMT" Cannot create new UE context, no memory\n",
            PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));
      return NULL;
    }

    ue_context_p->ue_id_rnti                    = ctxt_pP->rnti; // here ue_id_rnti is just a key, may be something else
    ue_context_p->ue_context.rnti               = ctxt_pP->rnti; // yes duplicate, 1 may be removed
    ue_context_p->ue_context.random_ue_identity = ue_identityP;
    RB_INSERT(rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head, ue_context_p);
    LOG_D(NR_RRC,
          PROTOCOL_NR_RRC_CTXT_UE_FMT" Created new UE context uid %u\n",
          PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
          ue_context_p->local_uid);
    return ue_context_p;
  } else {
    LOG_E(NR_RRC,
          PROTOCOL_NR_RRC_CTXT_UE_FMT" Cannot create new UE context, already exist\n",
          PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP));
    return NULL;
  }

  return(ue_context_p);
}
