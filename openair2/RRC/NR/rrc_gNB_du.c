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


static bool rrc_gNB_plmn_matches(const gNB_RRC_INST *rrc, const f1ap_served_cell_info_t *info)
{
  const gNB_RrcConfigurationReq *conf = &rrc->configuration;
  return conf->num_plmn == 1 // F1 supports only one
    && conf->mcc[0] == info->plmn.mcc
    && conf->mnc[0] == info->plmn.mnc
    && rrc->nr_cellid == info->nr_cellid;
}

void rrc_gNB_process_f1_setup_req(f1ap_setup_req_t *req, sctp_assoc_t assoc_id)
{
  AssertFatal(assoc_id != 0, "illegal assoc_id == 0: should be -1 (monolithic) or >0 (split)\n");
  gNB_RRC_INST *rrc = RC.nrrrc[0];
  DevAssert(rrc);

  LOG_I(NR_RRC, "Received F1 Setup Request from gNB_DU %lu (%s) on assoc_id %d\n", req->gNB_DU_id, req->gNB_DU_name, assoc_id);

  // check:
  // - it is the first DU
  // - it is one cell
  // - PLMN and Cell ID matches
  // else reject
  if (rrc->du != NULL) {
    const f1ap_setup_req_t *other = rrc->du->setup_req;
    LOG_E(NR_RRC, "can only handle one DU, but already serving DU %ld (%s)\n", other->gNB_DU_id, other->gNB_DU_name);
    f1ap_setup_failure_t fail = {.cause = F1AP_CauseRadioNetwork_gNB_CU_Cell_Capacity_Exceeded};
    rrc->mac_rrc.f1_setup_failure(&fail);
    return;
  }
  if (req->num_cells_available != 1) {
    LOG_E(NR_RRC, "can only handle on DU cell, but gNB_DU %ld has %d\n", req->gNB_DU_id, req->num_cells_available);
    f1ap_setup_failure_t fail = {.cause = F1AP_CauseRadioNetwork_gNB_CU_Cell_Capacity_Exceeded};
    rrc->mac_rrc.f1_setup_failure(&fail);
    return;
  }
  f1ap_served_cell_info_t *cell_info = &req->cell[0].info;
  if (!rrc_gNB_plmn_matches(rrc, cell_info)) {
    LOG_E(NR_RRC,
          "PLMN mismatch: CU %d%d cellID %ld, DU %d%d cellID %ld\n",
          rrc->configuration.mcc[0],
          rrc->configuration.mnc[0],
          rrc->nr_cellid,
          cell_info->plmn.mcc,
          cell_info->plmn.mnc,
          cell_info->nr_cellid);
    f1ap_setup_failure_t fail = {.cause = F1AP_CauseRadioNetwork_plmn_not_served_by_the_gNB_CU};
    rrc->mac_rrc.f1_setup_failure(&fail);
    return;
  }
  // if there is no system info or no SIB1 and we run in SA mode, we cannot handle it
  const f1ap_gnb_du_system_info_t *sys_info = req->cell[0].sys_info;
  if (sys_info == NULL || sys_info->mib == NULL || (sys_info->sib1 == NULL && get_softmodem_params()->sa)) {
    LOG_E(NR_RRC, "no system information provided by DU, rejecting\n");
    f1ap_setup_failure_t fail = {.cause = F1AP_CauseProtocol_semantic_error};
    rrc->mac_rrc.f1_setup_failure(&fail);
    return;
  }

  /* do we need the MIB? for the moment, just check it is valid, then drop it */
  NR_BCCH_BCH_Message_t *mib = NULL;
  asn_dec_rval_t dec_rval =
      uper_decode_complete(NULL, &asn_DEF_NR_BCCH_BCH_Message, (void **)&mib, sys_info->mib, sys_info->mib_length);
  if (dec_rval.code != RC_OK || mib->message.present != NR_BCCH_BCH_MessageType_PR_mib
      || mib->message.choice.messageClassExtension == NULL) {
    LOG_E(RRC, "Failed to decode NR_BCCH_BCH_MESSAGE (%zu bits) of DU, rejecting DU\n", dec_rval.consumed);
    f1ap_setup_failure_t fail = {.cause = F1AP_CauseProtocol_semantic_error};
    rrc->mac_rrc.f1_setup_failure(&fail);
    ASN_STRUCT_FREE(asn_DEF_NR_BCCH_BCH_Message, mib);
    return;
  }

  NR_SIB1_t *sib1 = NULL;
  if (sys_info->sib1) {
    dec_rval = uper_decode_complete(NULL, &asn_DEF_NR_SIB1, (void **)&sib1, sys_info->sib1, sys_info->sib1_length);
    if (dec_rval.code != RC_OK) {
      LOG_E(RRC, "Failed to decode NR_SIB1 (%zu bits) of DU, rejecting DU\n", dec_rval.consumed);
      f1ap_setup_failure_t fail = {.cause = F1AP_CauseProtocol_semantic_error};
      rrc->mac_rrc.f1_setup_failure(&fail);
      ASN_STRUCT_FREE(asn_DEF_NR_SIB1, sib1);
      return;
    }
    if (LOG_DEBUGFLAG(DEBUG_ASN1))
      xer_fprint(stdout, &asn_DEF_NR_SIB1, sib1);
  }

  LOG_I(RRC, "Accepting DU %ld (%s), sending F1 Setup Response\n", req->gNB_DU_id, req->gNB_DU_name);

  // we accept the DU
  rrc->du = calloc(1, sizeof(*rrc->du));
  AssertFatal(rrc->du != NULL, "out of memory\n");
  rrc->du->assoc_id = assoc_id;

  /* ITTI will free the setup request message via free(). So the memory
   * "inside" of the message will remain, but the "outside" container no, so
   * allocate memory and copy it in */
  rrc->du->setup_req = malloc(sizeof(*rrc->du->setup_req));
  AssertFatal(rrc->du->setup_req != NULL, "out of memory\n");
  *rrc->du->setup_req = *req;
  rrc->du->mib = mib->message.choice.mib;
  mib->message.choice.mib = NULL;
  ASN_STRUCT_FREE(asn_DEF_NR_BCCH_BCH_MessageType, mib);
  rrc->du->sib1 = sib1;

  served_cells_to_activate_t cell = {
      .plmn = cell_info->plmn,
      .nr_cellid = cell_info->nr_cellid,
      .nrpci = cell_info->nr_pci,
      .num_SI = 0,
  };
  f1ap_setup_resp_t resp = {.num_cells_to_activate = 1, .cells_to_activate[0] = cell};
  if (rrc->node_name != NULL)
    resp.gNB_CU_name = strdup(rrc->node_name);
  rrc->mac_rrc.f1_setup_response(&resp);

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
  AssertFatal(rrc->du != NULL, "no DU connected, cannot received F1 lost connection\n");
  AssertFatal(rrc->du->assoc_id == assoc_id,
              "previously connected DU (%d) does not match DU for which connection has been lost (%d)\n",
              rrc->du->assoc_id,
              assoc_id);
  (void) lc; // unused for the moment

  nr_rrc_du_container_t *du = rrc->du;
  f1ap_setup_req_t *req = du->setup_req;
  LOG_I(RRC, "releasing DU ID %ld (%s) on assoc_id %d\n", req->gNB_DU_id, req->gNB_DU_name, assoc_id);
  ASN_STRUCT_FREE(asn_DEF_NR_MIB, du->mib);
  ASN_STRUCT_FREE(asn_DEF_NR_SIB1, du->sib1);
  /* TODO: free setup request */
  free(rrc->du);
  rrc->du = NULL;

  int num = invalidate_du_connections(rrc, assoc_id);
  if (num > 0) {
    LOG_I(NR_RRC, "%d UEs lost through DU disconnect\n", num);
  }
}

nr_rrc_du_container_t *get_du_for_ue(gNB_RRC_INST *rrc, uint32_t ue_id)
{
  nr_rrc_du_container_t *du = rrc->du;
  if (du == NULL)
    return NULL;
  return du;
}

nr_rrc_du_container_t *get_du_by_assoc_id(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id)
{
  AssertFatal(assoc_id == rrc->du->assoc_id, "cannot handle multiple DUs yet\n");
  return rrc->du;
}
