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

#include "rrc_gNB_du.h"
#include "common/ran_context.h"
#include "nr_rrc_defs.h"
#include "rrc_gNB_UE_context.h"
#include "openair2/F1AP/f1ap_common.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "executables/softmodem-common.h"


static int du_compare(const nr_rrc_du_container_t *a, const nr_rrc_du_container_t *b)
{
  if (a->assoc_id > b->assoc_id)
    return 1;
  if (a->assoc_id == b->assoc_id)
    return 0;
  return -1; /* a->assoc_id < b->assoc_id */
}

/* Tree management functions */
RB_GENERATE/*_STATIC*/(rrc_du_tree, nr_rrc_du_container_t, entries, du_compare);

static bool rrc_gNB_plmn_matches(const gNB_RRC_INST *rrc, const f1ap_served_cell_info_t *info)
{
  const gNB_RrcConfigurationReq *conf = &rrc->configuration;
  return conf->num_plmn == 1 // F1 supports only one
    && conf->mcc[0] == info->plmn.mcc
    && conf->mnc[0] == info->plmn.mnc;
}

void rrc_gNB_process_f1_setup_req(f1ap_setup_req_t *req, sctp_assoc_t assoc_id)
{
  AssertFatal(assoc_id != 0, "illegal assoc_id == 0: should be -1 (monolithic) or >0 (split)\n");
  gNB_RRC_INST *rrc = RC.nrrrc[0];
  DevAssert(rrc);

  LOG_I(NR_RRC, "Received F1 Setup Request from gNB_DU %lu (%s) on assoc_id %d\n", req->gNB_DU_id, req->gNB_DU_name, assoc_id);

  // check:
  // - it is one cell
  // - PLMN and Cell ID matches
  // - no previous DU with the same ID
  // else reject
  if (req->num_cells_available != 1) {
    LOG_E(NR_RRC, "can only handle on DU cell, but gNB_DU %ld has %d\n", req->gNB_DU_id, req->num_cells_available);
    f1ap_setup_failure_t fail = {.cause = F1AP_CauseRadioNetwork_gNB_CU_Cell_Capacity_Exceeded};
    rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
    return;
  }
  f1ap_served_cell_info_t *cell_info = &req->cell[0].info;
  if (!rrc_gNB_plmn_matches(rrc, cell_info)) {
    LOG_E(NR_RRC,
          "PLMN mismatch: CU %d%d, DU %d%d\n",
          rrc->configuration.mcc[0],
          rrc->configuration.mnc[0],
          cell_info->plmn.mcc,
          cell_info->plmn.mnc);
    f1ap_setup_failure_t fail = {.cause = F1AP_CauseRadioNetwork_plmn_not_served_by_the_gNB_CU};
    rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
    return;
  }
  nr_rrc_du_container_t *it = NULL;
  RB_FOREACH(it, rrc_du_tree, &rrc->dus) {
    if (it->setup_req->gNB_DU_id == req->gNB_DU_id) {
      LOG_E(NR_RRC,
            "gNB-DU ID: existing DU %s on assoc_id %d already has ID %ld, rejecting requesting gNB-DU\n",
            it->setup_req->gNB_DU_name,
            it->assoc_id,
            it->setup_req->gNB_DU_id);
      f1ap_setup_failure_t fail = {.cause = F1AP_CauseMisc_unspecified};
      rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
      return;
    }
    // note: we assume that each DU contains only one cell; otherwise, we would
    // need to check every cell in the requesting DU to any existing cell.
    const f1ap_served_cell_info_t *exist_info = &it->setup_req->cell[0].info;
    const f1ap_served_cell_info_t *new_info = &req->cell[0].info;
    if (exist_info->nr_cellid == new_info->nr_cellid || exist_info->nr_pci == new_info->nr_pci) {
      LOG_E(NR_RRC,
            "existing DU %s on assoc_id %d already has cellID %ld/physCellId %d, rejecting requesting gNB-DU with cellID %ld/physCellId %d\n",
            it->setup_req->gNB_DU_name,
            it->assoc_id,
            exist_info->nr_cellid,
            exist_info->nr_pci,
            new_info->nr_cellid,
            new_info->nr_pci);
      f1ap_setup_failure_t fail = {.cause = F1AP_CauseMisc_unspecified};
      rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
      return;
    }
  }

  // if there is no system info or no SIB1 and we run in SA mode, we cannot handle it
  const f1ap_gnb_du_system_info_t *sys_info = req->cell[0].sys_info;
  NR_BCCH_BCH_Message_t *mib = NULL;
  NR_SIB1_t *sib1 = NULL;

  if (sys_info != NULL && sys_info->mib != NULL && !(sys_info->sib1 == NULL && get_softmodem_params()->sa)) {
    asn_dec_rval_t dec_rval =
        uper_decode_complete(NULL, &asn_DEF_NR_BCCH_BCH_Message, (void **)&mib, sys_info->mib, sys_info->mib_length);
    if (dec_rval.code != RC_OK || mib->message.present != NR_BCCH_BCH_MessageType_PR_mib
        || mib->message.choice.messageClassExtension == NULL) {
      LOG_E(RRC, "Failed to decode NR_BCCH_BCH_MESSAGE (%zu bits) of DU, rejecting DU\n", dec_rval.consumed);
      f1ap_setup_failure_t fail = {.cause = F1AP_CauseProtocol_semantic_error};
      rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
      ASN_STRUCT_FREE(asn_DEF_NR_BCCH_BCH_Message, mib);
      return;
    }

    if (sys_info->sib1) {
      dec_rval = uper_decode_complete(NULL, &asn_DEF_NR_SIB1, (void **)&sib1, sys_info->sib1, sys_info->sib1_length);
      if (dec_rval.code != RC_OK) {
        LOG_E(RRC, "Failed to decode NR_SIB1 (%zu bits) of DU, rejecting DU\n", dec_rval.consumed);
        f1ap_setup_failure_t fail = {.cause = F1AP_CauseProtocol_semantic_error};
        rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
        ASN_STRUCT_FREE(asn_DEF_NR_SIB1, sib1);
        return;
      }
      if (LOG_DEBUGFLAG(DEBUG_ASN1))
        xer_fprint(stdout, &asn_DEF_NR_SIB1, sib1);
    }
  }
  LOG_I(RRC, "Accepting DU %ld (%s), sending F1 Setup Response\n", req->gNB_DU_id, req->gNB_DU_name);
  LOG_I(RRC, "DU uses RRC version %u.%u.%u\n", req->rrc_ver[0], req->rrc_ver[1], req->rrc_ver[2]);

  // we accept the DU
  nr_rrc_du_container_t *du = calloc(1, sizeof(*du));
  AssertFatal(du != NULL, "out of memory\n");
  du->assoc_id = assoc_id;

  /* ITTI will free the setup request message via free(). So the memory
   * "inside" of the message will remain, but the "outside" container no, so
   * allocate memory and copy it in */
  du->setup_req = calloc(1,sizeof(*du->setup_req));
  AssertFatal(du->setup_req != NULL, "out of memory\n");
  *du->setup_req = *req;
  if (mib != NULL && sib1 != NULL) {
    du->mib = mib->message.choice.mib;
    mib->message.choice.mib = NULL;
    ASN_STRUCT_FREE(asn_DEF_NR_BCCH_BCH_MessageType, mib);
    du->sib1 = sib1;
  }
  RB_INSERT(rrc_du_tree, &rrc->dus, du);
  rrc->num_dus++;

  served_cells_to_activate_t cell = {
      .plmn = cell_info->plmn,
      .nr_cellid = cell_info->nr_cellid,
      .nrpci = cell_info->nr_pci,
      .num_SI = 0,
  };  

  f1ap_setup_resp_t resp = {.transaction_id = req->transaction_id,
                            .num_cells_to_activate = 1,
                            .cells_to_activate[0] = cell};
  int num = read_version(TO_STRING(NR_RRC_VERSION), &resp.rrc_ver[0], &resp.rrc_ver[1], &resp.rrc_ver[2]);
  AssertFatal(num == 3, "could not read RRC version string %s\n", TO_STRING(NR_RRC_VERSION));
  if (rrc->node_name != NULL)
    resp.gNB_CU_name = strdup(rrc->node_name);
  rrc->mac_rrc.f1_setup_response(assoc_id, &resp);

  /*
  MessageDef *msg_p2 = itti_alloc_new_message(TASK_RRC_GNB, 0, F1AP_GNB_CU_CONFIGURATION_UPDATE);
  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p2).gNB_CU_name = rrc->node_name;
  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p2).cells_to_activate[0].plmn.mcc = rrc->configuration.mcc[0];
  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p2).cells_to_activate[0].plmn.mnc = rrc->configuration.mnc[0];
  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p2).cells_to_activate[0].plmn.mnc_digit_length = rrc->configuration.mnc_digit_length[0];
  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p2).cells_to_activate[0].nr_cellid = rrc->nr_cellid;
  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p2).cells_to_activate[0].nrpci = req->cell[0].info.nr_pci;
  int num_SI = 0;

  if (rrc->carrier.SIB23) {
    F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p2).cells_to_activate[0].SI_container[2] = rrc->carrier.SIB23;
    F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p2).cells_to_activate[0].SI_container_length[2] = rrc->carrier.sizeof_SIB23;
    num_SI++;
  }

  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p2).cells_to_activate[0].num_SI = num_SI;
  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p2).num_cells_to_activate = 1;
  // send
  itti_send_msg_to_task(TASK_CU_F1, 0, msg_p2);
  */
}

static int invalidate_du_connections(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id)
{
  int count = 0;
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &rrc->rrc_ue_head) {
    uint32_t ue_id = ue_context_p->ue_context.rrc_ue_id;
    f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_id);
    if (ue_data.du_assoc_id == assoc_id) {
      /* this UE belongs to the DU that disconnected, set du_assoc_id to 0,
       * meaning DU is offline */
      cu_remove_f1_ue_data(ue_id);
      f1_ue_data_t new = {.secondary_ue = ue_data.secondary_ue, .du_assoc_id = 0 };
      cu_add_f1_ue_data(ue_id, &new);
      count++;
    }
  }
  return count;
}

void rrc_CU_process_f1_lost_connection(gNB_RRC_INST *rrc, f1ap_lost_connection_t *lc, sctp_assoc_t assoc_id)
{
  AssertFatal(assoc_id != 0, "illegal assoc_id == 0: should be -1 (monolithic) or >0 (split)\n");
  (void) lc; // unused for the moment

  nr_rrc_du_container_t e = {.assoc_id = assoc_id};
  nr_rrc_du_container_t *du = RB_FIND(rrc_du_tree, &rrc->dus, &e);
  if (du == NULL) {
    LOG_W(NR_RRC, "no DU connected or not found for assoc_id %d: F1 Setup Failed?\n", assoc_id);
    return;
  }

  f1ap_setup_req_t *req = du->setup_req;
  LOG_I(RRC, "releasing DU ID %ld (%s) on assoc_id %d\n", req->gNB_DU_id, req->gNB_DU_name, assoc_id);
  ASN_STRUCT_FREE(asn_DEF_NR_MIB, du->mib);
  ASN_STRUCT_FREE(asn_DEF_NR_SIB1, du->sib1);
  /* TODO: free setup request */
  nr_rrc_du_container_t *removed = RB_REMOVE(rrc_du_tree, &rrc->dus, du);
  DevAssert(removed != NULL);
  rrc->num_dus--;

  int num = invalidate_du_connections(rrc, assoc_id);
  if (num > 0) {
    LOG_I(NR_RRC, "%d UEs lost through DU disconnect\n", num);
  }
}

nr_rrc_du_container_t *get_du_for_ue(gNB_RRC_INST *rrc, uint32_t ue_id)
{
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_id);
  return get_du_by_assoc_id(rrc, ue_data.du_assoc_id);
}

nr_rrc_du_container_t *get_du_by_assoc_id(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id)
{
  nr_rrc_du_container_t e = {.assoc_id = assoc_id};
  return RB_FIND(rrc_du_tree, &rrc->dus, &e);
}

void dump_du_info(const gNB_RRC_INST *rrc, FILE *f)
{
  fprintf(f, "%ld connected DUs \n", rrc->num_dus);
  int i = 1;
  nr_rrc_du_container_t *du = NULL;
  /* cast is necessary to eliminate warning "discards ‘const’ qualifier" */
  RB_FOREACH(du, rrc_du_tree, &((gNB_RRC_INST *)rrc)->dus) {
    const f1ap_setup_req_t *sr = du->setup_req;
    fprintf(f, "[%d] DU ID %ld (%s) ", i++, sr->gNB_DU_id, sr->gNB_DU_name);
    if (du->assoc_id == -1) {
      fprintf(f, "integrated DU-CU");
    } else {
      fprintf(f, "assoc_id %d", du->assoc_id);
    }
    const f1ap_served_cell_info_t *info = &sr->cell[0].info;
    fprintf(f, ": nrCellID %ld, PCI %d\n", info->nr_cellid, info->nr_pci);
  }
}
