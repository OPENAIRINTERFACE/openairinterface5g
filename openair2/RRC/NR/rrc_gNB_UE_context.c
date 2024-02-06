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
#include "openair2/F1AP/f1ap_ids.h"


//------------------------------------------------------------------------------
int rrc_gNB_compare_ue_rnti_id(rrc_gNB_ue_context_t *c1_pP, rrc_gNB_ue_context_t *c2_pP)
//------------------------------------------------------------------------------
{
  if (c1_pP->ue_context.rrc_ue_id > c2_pP->ue_context.rrc_ue_id) {
    return 1;
  }

  if (c1_pP->ue_context.rrc_ue_id < c2_pP->ue_context.rrc_ue_id) {
    return -1;
  }

  return 0;
}

/* Generate the tree management functions */
RB_GENERATE(rrc_nr_ue_tree_s, rrc_gNB_ue_context_s, entries,
            rrc_gNB_compare_ue_rnti_id);

//------------------------------------------------------------------------------
rrc_gNB_ue_context_t *rrc_gNB_allocate_new_ue_context(gNB_RRC_INST *rrc_instance_pP)
//------------------------------------------------------------------------------
{
  rrc_gNB_ue_context_t *new_p = calloc(1, sizeof(*new_p));

  if (new_p == NULL) {
    LOG_E(NR_RRC, "Cannot allocate new ue context\n");
    return NULL;
  }
  new_p->ue_context.rrc_ue_id = uid_linear_allocator_new(&rrc_instance_pP->uid_allocator) + 1;

  for(int i = 0; i < NB_RB_MAX; i++)
    new_p->ue_context.pduSession[i].xid = -1;

  LOG_D(NR_RRC, "Returning new RRC UE context RRC ue id: %d\n", new_p->ue_context.rrc_ue_id);
  return(new_p);
}


//------------------------------------------------------------------------------
rrc_gNB_ue_context_t *rrc_gNB_get_ue_context(gNB_RRC_INST *rrc_instance_pP, ue_id_t ue)
//------------------------------------------------------------------------------
{
  rrc_gNB_ue_context_t temp;
  /* gNB ue rrc id = 24 bits wide */
  temp.ue_context.rrc_ue_id = ue;
  return RB_FIND(rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head, &temp);
}

rrc_gNB_ue_context_t *rrc_gNB_get_ue_context_by_rnti(gNB_RRC_INST *rrc_instance_pP, sctp_assoc_t assoc_id, rnti_t rntiP)
{
  rrc_gNB_ue_context_t *ue_context_p;
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &(rrc_instance_pP->rrc_ue_head)) {
    f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_context_p->ue_context.rrc_ue_id);
    if (ue_data.du_assoc_id == assoc_id && ue_context_p->ue_context.rnti == rntiP)
      return ue_context_p;
  }
  LOG_W(NR_RRC, "search by RNTI %04x and assoc_id %d: no UE found\n", rntiP, assoc_id);
  return NULL;
}

rrc_gNB_ue_context_t *rrc_gNB_get_ue_context_by_rnti_any_du(gNB_RRC_INST *rrc_instance_pP, rnti_t rntiP)
{
  rrc_gNB_ue_context_t *ue_context_p;
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &(rrc_instance_pP->rrc_ue_head))
  {
    if (ue_context_p->ue_context.rnti == rntiP)
      return ue_context_p;
  }
  LOG_W(NR_RRC, "search by rnti not found %04x\n", rntiP);
  return NULL;
}

void rrc_gNB_free_mem_ue_context(rrc_gNB_ue_context_t *const ue_context_pP)
//-----------------------------------------------------------------------------
{
  LOG_T(NR_RRC, " Clearing UE context 0x%p (free internal structs)\n", ue_context_pP);
  free(ue_context_pP);
}

//------------------------------------------------------------------------------
void rrc_gNB_remove_ue_context(gNB_RRC_INST *rrc_instance_pP, rrc_gNB_ue_context_t *ue_context_pP)
//------------------------------------------------------------------------------
{
  if (rrc_instance_pP == NULL) {
    LOG_E(NR_RRC, " Bad RRC instance\n");
    return;
  }

  if (ue_context_pP == NULL) {
    LOG_E(NR_RRC, "Trying to free a NULL UE context\n");
    return;
  }

  RB_REMOVE(rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head, ue_context_pP);
  uid_linear_allocator_free(&rrc_instance_pP->uid_allocator, ue_context_pP->ue_context.rrc_ue_id - 1);
  cu_remove_f1_ue_data(ue_context_pP->ue_context.rrc_ue_id);
  rrc_gNB_free_mem_ue_context(ue_context_pP);
  LOG_I(NR_RRC, "Removed UE context\n");
}

//-----------------------------------------------------------------------------
// return the ue context if there is already an UE with ue_identityP, NULL otherwise
rrc_gNB_ue_context_t *rrc_gNB_ue_context_random_exist(gNB_RRC_INST *rrc_instance_pP, const uint64_t ue_identityP)
//-----------------------------------------------------------------------------
{
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head) {
    if (ue_context_p->ue_context.random_ue_identity == ue_identityP)
      return ue_context_p;
  }
  return NULL;
}

//-----------------------------------------------------------------------------
// return the ue context if there is already an UE with the same S-TMSI, NULL otherwise
rrc_gNB_ue_context_t *rrc_gNB_ue_context_5g_s_tmsi_exist(gNB_RRC_INST *rrc_instance_pP, const uint64_t s_TMSI)
//-----------------------------------------------------------------------------
{
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head)
  {
    LOG_I(NR_RRC, "Checking for UE 5G S-TMSI %ld: RNTI %04x\n", s_TMSI, ue_context_p->ue_context.rnti);
    if (ue_context_p->ue_context.ng_5G_S_TMSI_Part1 == s_TMSI)
      return ue_context_p;
  }
    return NULL;
}

//-----------------------------------------------------------------------------
// return a new ue context structure if ue_identityP, rnti not found in collection
rrc_gNB_ue_context_t *rrc_gNB_create_ue_context(sctp_assoc_t assoc_id,
                                                rnti_t rnti,
                                                gNB_RRC_INST *rrc_instance_pP,
                                                const uint64_t ue_identityP,
                                                uint32_t du_ue_id)
//-----------------------------------------------------------------------------
{
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_allocate_new_ue_context(rrc_instance_pP);
  if (ue_context_p == NULL)
    return NULL;

  gNB_RRC_UE_t *ue = &ue_context_p->ue_context;
  ue->rnti = rnti;
  ue->random_ue_identity = ue_identityP;
  f1_ue_data_t ue_data = {.secondary_ue = du_ue_id, .du_assoc_id = assoc_id};
  AssertFatal(!cu_exists_f1_ue_data(ue->rrc_ue_id),
              "UE F1 Context for ID %d already exists, logic bug\n",
              ue->rrc_ue_id);
  cu_add_f1_ue_data(ue->rrc_ue_id, &ue_data);
  ue->max_delays_pdu_session = 20; /* see rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ() */

  RB_INSERT(rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head, ue_context_p);
  LOG_I(NR_RRC,
        "Created new UE context: CU UE ID %u DU UE ID %u (rnti: %04x, random ue id %lx)\n",
        ue->rrc_ue_id,
        du_ue_id,
        ue->rnti,
        ue->random_ue_identity);
  return ue_context_p;
}
