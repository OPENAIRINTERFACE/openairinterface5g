/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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

#include "UTIL/LOG/log.h"
#include "rrc_eNB_UE_context.h"
#include "msc.h"


//------------------------------------------------------------------------------
void
uid_linear_allocator_init(
  uid_allocator_t* const uid_pP
)
//------------------------------------------------------------------------------
{
  memset(uid_pP, 0, sizeof(uid_allocator_t));
}

//------------------------------------------------------------------------------
uid_t
uid_linear_allocator_new(
  eNB_RRC_INST* const rrc_instance_pP
)
//------------------------------------------------------------------------------
{
  unsigned int i;
  unsigned int bit_index = 1;
  uid_t        uid = 0;
  uid_allocator_t* uia_p = &rrc_instance_pP->uid_allocator;

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
uid_linear_allocator_free(
  eNB_RRC_INST* rrc_instance_pP,
  uid_t uidP
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
int rrc_eNB_compare_ue_rnti_id(
  struct rrc_eNB_ue_context_s* c1_pP, struct rrc_eNB_ue_context_s* c2_pP)
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
struct rrc_eNB_ue_context_s*
rrc_eNB_allocate_new_UE_context(
  eNB_RRC_INST* rrc_instance_pP
)
//------------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s* new_p;
  new_p = malloc(sizeof(struct rrc_eNB_ue_context_s));

  if (new_p == NULL) {
    LOG_E(RRC, "Cannot allocate new ue context\n");
    return NULL;
  }

  memset(new_p, 0, sizeof(struct rrc_eNB_ue_context_s));
  new_p->local_uid = uid_linear_allocator_new(rrc_instance_pP);
  return new_p;
}


//------------------------------------------------------------------------------
struct rrc_eNB_ue_context_s*
rrc_eNB_get_ue_context(
  eNB_RRC_INST* rrc_instance_pP,
  rnti_t rntiP)
//------------------------------------------------------------------------------
{
  rrc_eNB_ue_context_t temp;
  memset(&temp, 0, sizeof(struct rrc_eNB_ue_context_s));
  /* eNB ue rrc id = 24 bits wide */
  temp.ue_id_rnti = rntiP;
  return RB_FIND(rrc_ue_tree_s, &rrc_instance_pP->rrc_ue_head, &temp);
}


//------------------------------------------------------------------------------
void rrc_eNB_remove_ue_context(
  const protocol_ctxt_t* const ctxt_pP,
  eNB_RRC_INST*                rrc_instance_pP,
  struct rrc_eNB_ue_context_s* ue_context_pP)
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

  MSC_LOG_EVENT(
    MSC_RRC_ENB,
    "0 Removed UE %"PRIx16" ",
    ue_context_pP->ue_context.rnti);

  rrc_eNB_free_mem_UE_context(ctxt_pP, ue_context_pP);
  uid_linear_allocator_free(rrc_instance_pP, ue_context_pP->local_uid);
  free(ue_context_pP);
  LOG_I(RRC,
        PROTOCOL_RRC_CTXT_UE_FMT" Removed UE context\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP));
}


