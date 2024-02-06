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

#include "common/ran_context.h"
#include "nr_rrc_defs.h"
#include "openair2/F1AP/f1ap_ids.h"

static int cuup_compare(const nr_rrc_cuup_container_t *a, const nr_rrc_cuup_container_t *b)
{
  if (a->assoc_id > b->assoc_id)
    return 1;
  if (a->assoc_id == b->assoc_id)
    return 0;
  return -1; /* a->assoc_id < b->assoc_id */
}

/* Tree management functions */
RB_GENERATE/*_STATIC*/(rrc_cuup_tree, nr_rrc_cuup_container_t, entries, cuup_compare);

static const nr_rrc_cuup_container_t *select_cuup_slice(const struct rrc_cuup_tree *t, const gNB_RRC_UE_t *ue, int sst, int sd)
{
  nr_rrc_cuup_container_t *second_best_match = NULL; /* if no NSSAI matches exactly */
  nr_rrc_cuup_container_t *cuup = NULL;
  RB_FOREACH(cuup, rrc_cuup_tree, (struct rrc_cuup_tree *)&t) {
    e1ap_setup_req_t *sr = cuup->setup_req;
    for (int p = 0; p < sr->supported_plmns; ++p) {
      /* actually we should also check that the PLMN selected by the UE matches */
      for (int s = 0; s < sr->plmn[p].supported_slices; ++s) {
        e1ap_nssai_t *nssai = &sr->plmn[p].slice[s];
        if (nssai->sst == sst && nssai->sd == sd) {
          LOG_A(RRC, "selecting CU-UP ID %ld based on exact NSSAI match (%d:0x%06x)\n", sr->gNB_cu_up_id, sst, sd);
          return cuup; /* exact match */
        } else if (nssai->sst == sst && second_best_match == NULL) {
          LOG_I(RRC, "second best match: CU-UP ID %ld matches SST %d\n", sr->gNB_cu_up_id, sst);
          second_best_match = cuup; /* only the SST matches -> "good enough" */
        }
      }
    }
  }
  return second_best_match;
}

static const nr_rrc_cuup_container_t *select_cuup_round_robin(size_t n_t, const struct rrc_cuup_tree *t, const gNB_RRC_UE_t *ue)
{
  /* pick the CU-UP following a "round-robin" fashion: select CU-UP M = RRC UE
   * ID % N with N number of CU-UPs */
  int m = (ue->rrc_ue_id - 1) % n_t;
  nr_rrc_cuup_container_t *cuup = NULL;
  RB_FOREACH(cuup, rrc_cuup_tree, (struct rrc_cuup_tree *)&t) {
    if (m == 0) {
      LOG_W(RRC, "round-robin match: select CU-UP ID %ld (no NSSAI match)\n", cuup->setup_req->gNB_cu_up_id);
      return cuup;
    }
    m--;
  }
  /* this should not happen: no CU-UP available? */
  return NULL;
}

bool ue_associated_to_cuup(const gNB_RRC_INST *rrc, const gNB_RRC_UE_t *ue)
{
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue->rrc_ue_id);
  return ue_data.e1_assoc_id != 0;
}

sctp_assoc_t get_existing_cuup_for_ue(const gNB_RRC_INST *rrc, const gNB_RRC_UE_t *ue)
{
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue->rrc_ue_id);
  AssertFatal(ue_data.e1_assoc_id != 0, "UE %d should be associated to CU-UP, but is not\n", ue->rrc_ue_id);
  LOG_D(RRC, "UE %d using CU-UP assoc_id %d\n", ue->rrc_ue_id, ue_data.e1_assoc_id);
  return ue_data.e1_assoc_id;
}

sctp_assoc_t get_new_cuup_for_ue(const gNB_RRC_INST *rrc, const gNB_RRC_UE_t *ue, int sst, int sd)
{
  /* check if there is already a UE associated */
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue->rrc_ue_id);
  if (ue_data.e1_assoc_id != 0) {
    LOG_I(RRC, "UE %d using CU-UP assoc_id %d\n", ue->rrc_ue_id, ue_data.e1_assoc_id);
    return ue_data.e1_assoc_id;
  }

  /* it is zero -> no CUUP for this UE yet, get the (only) CU-UP that is
   * connected */
  if (RB_EMPTY(&rrc->cuups)) {
    LOG_W(RRC, "no CU-UPs connected: UE %d cannot have a DRB\n", ue->rrc_ue_id);
    return 0; /* no CUUP connected */
  }

  const nr_rrc_cuup_container_t *selected = NULL;
  if (sst != 0) /* only do if there is slice information */
    selected = select_cuup_slice(&rrc->cuups, ue, sst, sd);
  if (selected == NULL) /* nothing found yet */
    selected = select_cuup_round_robin(rrc->num_cuups, &rrc->cuups, ue);
  if (selected == NULL) {
    selected = RB_ROOT(&rrc->cuups);
  }
  AssertFatal(selected != NULL, "logic error: could not select CU-UP, is one connected?\n");

  /* update the association for the UE so it will be picked up later */
  ue_data.e1_assoc_id = selected->assoc_id;
  cu_remove_f1_ue_data(ue->rrc_ue_id);
  cu_add_f1_ue_data(ue->rrc_ue_id, &ue_data);
  LOG_I(RRC, "UE %d associating to CU-UP assoc_id %d out of %ld CU-UPs\n", ue->rrc_ue_id, ue_data.e1_assoc_id, rrc->num_cuups);

  return ue_data.e1_assoc_id;
}

/* CU-CP Functions */

/**
 * @brief Trigger E1AP Setup Failure on CU-CP
*/
static void e1ap_setup_failure(sctp_assoc_t assoc_id, uint64_t transac_id)
{
  MessageDef *msg_p = itti_alloc_new_message(TASK_RRC_GNB, 0, E1AP_SETUP_FAIL);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  e1ap_setup_fail_t *setup_fail = &E1AP_SETUP_FAIL(msg_p);
  setup_fail->transac_id = transac_id;
  LOG_I(NR_RRC, "Triggering E1AP Setup Failure for transac_id %ld, assoc_id %ld\n",
        transac_id,
        msg_p->ittiMsgHeader.originInstance);
  itti_send_msg_to_task(TASK_CUCP_E1, 0 /*unused by callee*/, msg_p);
}

/**
 * @brief E1AP Setup Request processing on CU-CP
*/
int rrc_gNB_process_e1_setup_req(sctp_assoc_t assoc_id, e1ap_setup_req_t *req)
{
  AssertFatal(req->supported_plmns <= PLMN_LIST_MAX_SIZE, "Supported PLMNs is more than PLMN_LIST_MAX_SIZE\n");
  gNB_RRC_INST *rrc = RC.nrrrc[0];

  nr_rrc_cuup_container_t *c = NULL;
  RB_FOREACH(c, rrc_cuup_tree, &rrc->cuups) {
    if (req->gNB_cu_up_id == c->setup_req->gNB_cu_up_id) {
      LOG_E(NR_RRC,
            "Connecting CU-UP ID %ld name %s (assoc_id %d) has same ID as existing CU-UP ID %ld name %s (assoc_id %d)\n",
            req->gNB_cu_up_id,
            req->gNB_cu_up_name,
            assoc_id,
            c->setup_req->gNB_cu_up_id,
            c->setup_req->gNB_cu_up_name,
            c->assoc_id);
      e1ap_setup_failure(assoc_id, req->transac_id);
      return -1;
    }
  }

  for (int i = 0; i < req->supported_plmns; i++) {
    PLMN_ID_t *id = &req->plmn[i].id;
    if (rrc->configuration.mcc[i] != id->mcc || rrc->configuration.mnc[i] != id->mnc) {
      LOG_E(NR_RRC,
            "PLMNs received from CUUP (mcc:%d, mnc:%d) did not match with PLMNs in RRC (mcc:%d, mnc:%d)\n",
            id->mcc,
            id->mnc,
            rrc->configuration.mcc[i],
            rrc->configuration.mnc[i]);
      e1ap_setup_failure(assoc_id, req->transac_id);
      return -1;
    }
  }

  LOG_I(NR_RRC, "Accepting new CU-UP ID %ld name %s (assoc_id %d)\n", req->gNB_cu_up_id, req->gNB_cu_up_name, assoc_id);
  nr_rrc_cuup_container_t *cuup = malloc(sizeof(*cuup));
  AssertFatal(cuup, "out of memory\n");
  cuup->setup_req = malloc(sizeof(*cuup->setup_req));
  *cuup->setup_req = *req;
  cuup->assoc_id = assoc_id;
  RB_INSERT(rrc_cuup_tree, &rrc->cuups, cuup);
  rrc->num_cuups++;

  MessageDef *msg_p = itti_alloc_new_message(TASK_RRC_GNB, 0, E1AP_SETUP_RESP);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  e1ap_setup_resp_t *resp = &E1AP_SETUP_RESP(msg_p);
  resp->transac_id = req->transac_id;
  itti_send_msg_to_task(TASK_CUCP_E1, 0, msg_p);

  return 0;
}

/**
 * @brief RRC Processing of the indication of E1 connection loss on CU-CP
*/
void rrc_gNB_process_e1_lost_connection(gNB_RRC_INST *rrc, e1ap_lost_connection_t *lc, sctp_assoc_t assoc_id)
{
  LOG_I(NR_RRC, "Received E1 connection loss indication on RRC\n");
  AssertFatal(assoc_id != 0, "illegal assoc_id == 0: should be -1 (monolithic) or >0 (split)\n");
  nr_rrc_cuup_container_t e = {.assoc_id = assoc_id};
  nr_rrc_cuup_container_t *cuup = RB_FIND(rrc_cuup_tree, &rrc->cuups, &e);
  if (cuup == NULL) {
    LOG_W(NR_RRC, "CU-UP for assoc_id %d not found!\n", assoc_id);
    return;
  }
  if (cuup->setup_req != NULL) {
    e1ap_setup_req_t *req = cuup->setup_req;
    LOG_I(NR_RRC, "releasing CU-UP %s on assoc_id %d\n", req->gNB_cu_up_name, assoc_id);
    free(cuup->setup_req);
  }
  nr_rrc_cuup_container_t *removed = RB_REMOVE(rrc_cuup_tree, &rrc->cuups, cuup);
  // Free relevant CU-UP structures
  free(cuup);
  DevAssert(removed != NULL);
  rrc->num_cuups--;
}
