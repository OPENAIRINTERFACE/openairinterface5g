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

#include "mac_rrc_dl_handler.h"

#include "mac_proto.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "F1AP_CauseRadioNetwork.h"

#include "uper_decoder.h"
#include "uper_encoder.h"

// Standarized 5QI values and Default Priority levels as mentioned in 3GPP TS 23.501 Table 5.7.4-1
const uint64_t qos_fiveqi[26] = {1, 2, 3, 4, 65, 66, 67, 71, 72, 73, 74, 76, 5, 6, 7, 8, 9, 69, 70, 79, 80, 82, 83, 84, 85, 86};
const uint64_t qos_priority[26] = {20, 40, 30, 50, 7, 20, 15, 56, 56, 56, 56, 56, 10,
                                   60, 70, 80, 90, 5, 55, 65, 68, 19, 22, 24, 21, 18};

static long get_lcid_from_drbid(int drb_id)
{
  return drb_id + 3; /* LCID is DRB + 3 */
}

static long get_lcid_from_srbid(int srb_id)
{
  return srb_id;
}

static bool check_plmn_identity(const f1ap_plmn_t *check_plmn, const f1ap_plmn_t *plmn)
{
  return plmn->mcc == check_plmn->mcc && plmn->mnc_digit_length == check_plmn->mnc_digit_length && plmn->mnc == check_plmn->mnc;
}

void f1_setup_response(const f1ap_setup_resp_t *resp)
{
  LOG_I(MAC, "received F1 Setup Response from CU %s\n", resp->gNB_CU_name);
  LOG_I(MAC, "CU uses RRC version %d.%d.%d\n", resp->rrc_ver[0], resp->rrc_ver[1], resp->rrc_ver[2]);

  if (resp->num_cells_to_activate == 0) {
    LOG_W(NR_MAC, "no cell to activate: cell remains blocked\n");
    return;
  }

  gNB_MAC_INST *mac = RC.nrmac[0];
  NR_SCHED_LOCK(&mac->sched_lock);
  const f1ap_setup_req_t *setup_req = mac->f1_config.setup_req;
  const f1ap_served_cell_info_t *du_cell = &setup_req->cell[0].info;

  AssertFatal(resp->num_cells_to_activate == 1, "can only handle one cell, but %d activated\n", resp->num_cells_to_activate);
  const served_cells_to_activate_t *cu_cell = &resp->cells_to_activate[0];

  AssertFatal(du_cell->nr_cellid  == cu_cell->nr_cellid, "CellID mismatch: DU %ld vs CU %ld\n", du_cell->nr_cellid, cu_cell->nr_cellid);
  AssertFatal(check_plmn_identity(&du_cell->plmn, &cu_cell->plmn), "PLMN mismatch\n");
  AssertFatal(du_cell->nr_pci == cu_cell->nrpci, "PCI mismatch: DU %d vs CU %d\n", du_cell->nr_pci, cu_cell->nrpci);

  AssertFatal(cu_cell->num_SI == 0, "handling of CU-provided SIBs: not implemented\n");

  mac->f1_config.setup_resp = malloc(sizeof(*mac->f1_config.setup_resp));
  AssertFatal(mac->f1_config.setup_resp != NULL, "out of memory\n");
  *mac->f1_config.setup_resp = *resp;
  if (resp->gNB_CU_name)
    mac->f1_config.setup_resp->gNB_CU_name = strdup(resp->gNB_CU_name);

  NR_SCHED_UNLOCK(&mac->sched_lock);
}

void f1_setup_failure(const f1ap_setup_failure_t *failure)
{
  LOG_E(MAC, "the CU reported F1AP Setup Failure, is there a configuration mismatch?\n");
  exit(1);
}

static NR_RLC_BearerConfig_t *get_bearerconfig_from_srb(const f1ap_srb_to_be_setup_t *srb)
{
  long priority = srb->srb_id; // high priority for SRB
  e_NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration bucket =
      NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms5;
  return get_SRB_RLC_BearerConfig(get_lcid_from_srbid(srb->srb_id), priority, bucket);
}

static int handle_ue_context_srbs_setup(int rnti,
                                        int srbs_len,
                                        const f1ap_srb_to_be_setup_t *req_srbs,
                                        f1ap_srb_to_be_setup_t **resp_srbs,
                                        NR_CellGroupConfig_t *cellGroupConfig)
{
  DevAssert(req_srbs != NULL && resp_srbs != NULL && cellGroupConfig != NULL);

  *resp_srbs = calloc(srbs_len, sizeof(**resp_srbs));
  AssertFatal(*resp_srbs != NULL, "out of memory\n");
  for (int i = 0; i < srbs_len; i++) {
    const f1ap_srb_to_be_setup_t *srb = &req_srbs[i];
    NR_RLC_BearerConfig_t *rlc_BearerConfig = get_bearerconfig_from_srb(srb);
    nr_rlc_add_srb(rnti, srb->srb_id, rlc_BearerConfig);

    (*resp_srbs)[i] = *srb;

    int ret = ASN_SEQUENCE_ADD(&cellGroupConfig->rlc_BearerToAddModList->list, rlc_BearerConfig);
    DevAssert(ret == 0);
  }
  return srbs_len;
}

static NR_RLC_BearerConfig_t *get_bearerconfig_from_drb(const f1ap_drb_to_be_setup_t *drb)
{
  const NR_RLC_Config_PR rlc_conf = drb->rlc_mode == RLC_MODE_UM ? NR_RLC_Config_PR_um_Bi_Directional : NR_RLC_Config_PR_am;
  long priority = 13; // hardcoded for the moment
  return get_DRB_RLC_BearerConfig(get_lcid_from_drbid(drb->drb_id), drb->drb_id, rlc_conf, priority);
}

static int handle_ue_context_drbs_setup(int rnti,
                                        int drbs_len,
                                        const f1ap_drb_to_be_setup_t *req_drbs,
                                        f1ap_drb_to_be_setup_t **resp_drbs,
                                        NR_CellGroupConfig_t *cellGroupConfig)
{
  DevAssert(req_drbs != NULL && resp_drbs != NULL && cellGroupConfig != NULL);

  /* Note: the actual GTP tunnels are created in the F1AP breanch of
   * ue_context_*_response() */
  *resp_drbs = calloc(drbs_len, sizeof(**resp_drbs));
  AssertFatal(*resp_drbs != NULL, "out of memory\n");
  for (int i = 0; i < drbs_len; i++) {
    const f1ap_drb_to_be_setup_t *drb = &req_drbs[i];
    NR_RLC_BearerConfig_t *rlc_BearerConfig = get_bearerconfig_from_drb(drb);
    nr_rlc_add_drb(rnti, drb->drb_id, rlc_BearerConfig);

    (*resp_drbs)[i] = *drb;
    // just put same number of tunnels in DL as in UL
    (*resp_drbs)[i].up_dl_tnl_length = drb->up_ul_tnl_length;

    int ret = ASN_SEQUENCE_ADD(&cellGroupConfig->rlc_BearerToAddModList->list, rlc_BearerConfig);
    DevAssert(ret == 0);
  }
  return drbs_len;
}

static int handle_ue_context_drbs_release(int rnti,
                                          int drbs_len,
                                          const f1ap_drb_to_be_released_t *req_drbs,
                                          NR_CellGroupConfig_t *cellGroupConfig)
{
  DevAssert(req_drbs != NULL && cellGroupConfig != NULL);

  cellGroupConfig->rlc_BearerToReleaseList = calloc(1, sizeof(*cellGroupConfig->rlc_BearerToReleaseList));
  AssertFatal(cellGroupConfig->rlc_BearerToReleaseList != NULL, "out of memory\n");

  /* Note: the actual GTP tunnels are already removed in the F1AP message
   * decoding */
  for (int i = 0; i < drbs_len; i++) {
    const f1ap_drb_to_be_released_t *drb = &req_drbs[i];

    long lcid = get_lcid_from_drbid(drb->rb_id);
    int idx = 0;
    while (idx < cellGroupConfig->rlc_BearerToAddModList->list.count) {
      const NR_RLC_BearerConfig_t *bc = cellGroupConfig->rlc_BearerToAddModList->list.array[idx];
      if (bc->logicalChannelIdentity == lcid)
        break;
      ++idx;
    }
    if (idx < cellGroupConfig->rlc_BearerToAddModList->list.count) {
      nr_rlc_release_entity(rnti, lcid);
      asn_sequence_del(&cellGroupConfig->rlc_BearerToAddModList->list, idx, 1);
      long *plcid = malloc(sizeof(*plcid));
      AssertFatal(plcid != NULL, "out of memory\n");
      *plcid = lcid;
      int ret = ASN_SEQUENCE_ADD(&cellGroupConfig->rlc_BearerToReleaseList->list, plcid);
      DevAssert(ret == 0);
    }
  }
  return drbs_len;
}

static NR_UE_NR_Capability_t *get_ue_nr_cap(int rnti, uint8_t *buf, uint32_t len)
{
  if (buf == NULL || len == 0)
    return NULL;

  NR_UE_CapabilityRAT_ContainerList_t *clist = NULL;
  asn_dec_rval_t dec_rval = uper_decode(NULL, &asn_DEF_NR_UE_CapabilityRAT_ContainerList, (void **)&clist, buf, len, 0, 0);
  if (dec_rval.code != RC_OK) {
    LOG_W(NR_MAC, "cannot decode UE capability container list of UE RNTI %04x, ignoring capabilities\n", rnti);
    return NULL;
  }

  NR_UE_NR_Capability_t *cap = NULL;
  for (int i = 0; i < clist->list.count; i++) {
    const NR_UE_CapabilityRAT_Container_t *c = clist->list.array[i];
    if (cap != NULL || c->rat_Type != NR_RAT_Type_nr) {
      LOG_W(NR_MAC, "UE RNTI %04x: ignoring capability of type %ld\n", rnti, c->rat_Type);
      continue;
    }

    asn_dec_rval_t dec_rval = uper_decode(NULL,
                                          &asn_DEF_NR_UE_NR_Capability,
                                          (void **)&cap,
                                          c->ue_CapabilityRAT_Container.buf,
                                          c->ue_CapabilityRAT_Container.size,
                                          0,
                                          0);
    if (dec_rval.code != RC_OK) {
      LOG_W(NR_MAC, "cannot decode NR UE capabilities of UE RNTI %04x, ignoring NR capability\n", rnti);
      cap = NULL;
      continue;
    }
  }
  return cap;
}

NR_CellGroupConfig_t *clone_CellGroupConfig(const NR_CellGroupConfig_t *orig)
{
  uint8_t buf[16636];
  asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig, NULL, orig, buf, sizeof(buf));
  AssertFatal(enc_rval.encoded > 0, "could not clone CellGroupConfig: problem while encoding\n");
  NR_CellGroupConfig_t *cloned = NULL;
  asn_dec_rval_t dec_rval = uper_decode(NULL, &asn_DEF_NR_CellGroupConfig, (void **)&cloned, buf, enc_rval.encoded, 0, 0);
  AssertFatal(dec_rval.code == RC_OK && dec_rval.consumed == enc_rval.encoded,
              "could not clone CellGroupConfig: problem while decodung\n");
  return cloned;
}

static void set_nssaiConfig(const int drb_len, const f1ap_drb_to_be_setup_t *req_drbs, NR_UE_sched_ctrl_t *sched_ctrl)
{
  for (int i = 0; i < drb_len; i++) {
    const f1ap_drb_to_be_setup_t *drb = &req_drbs[i];

    long lcid = get_lcid_from_drbid(drb->drb_id);
    sched_ctrl->dl_lc_nssai[lcid] = drb->nssai;
    LOG_I(NR_MAC, "Setting NSSAI sst: %d, sd: %d for DRB: %ld\n", drb->nssai.sst, drb->nssai.sd, drb->drb_id);
  }
}

static void set_QoSConfig(const f1ap_ue_context_modif_req_t *req, NR_UE_sched_ctrl_t *sched_ctrl)
{
  AssertFatal(req != NULL, "f1ap_ue_context_modif_req is NULL\n");
  uint8_t drb_count = req->drbs_to_be_setup_length;
  uint8_t srb_count = req->srbs_to_be_setup_length;
  LOG_I(NR_MAC, "Number of DRBs = %d and SRBs = %d\n", drb_count, srb_count);

  /* DRBs*/
  for (int i = 0; i < drb_count; i++) {
    f1ap_drb_to_be_setup_t *drb_p = &req->drbs_to_be_setup[i];
    uint8_t nb_qos_flows = drb_p->drb_info.flows_to_be_setup_length;
    long drb_id = drb_p->drb_id;
    LOG_I(NR_MAC, "number of QOS flows mapped to DRB_id %ld: %d\n", drb_id, nb_qos_flows);

    for (int q = 0; q < nb_qos_flows; q++) {
      f1ap_flows_mapped_to_drb_t *qos_flow = &drb_p->drb_info.flows_mapped_to_drb[q];

      f1ap_qos_characteristics_t *qos_char = &qos_flow->qos_params.qos_characteristics;
      uint64_t priority = qos_char->non_dynamic.qos_priority_level;
      int64_t fiveqi = qos_char->non_dynamic.fiveqi;
      if (qos_char->qos_type == dynamic) {
        priority = qos_char->dynamic.qos_priority_level;
        fiveqi = qos_char->dynamic.fiveqi > 0 ? qos_char->dynamic.fiveqi : 0;
      }
      if (qos_char->qos_type == non_dynamic) {
        LOG_D(NR_MAC, "Qos Priority level is considered from the standarsdized 5QI to QoS mapping table\n");
        for (int id = 0; id < 26; id++) {
          if (qos_fiveqi[id] == fiveqi)
            priority = qos_priority[id];
        }
      }
      sched_ctrl->qos_config[drb_id - 1][q].fiveQI = fiveqi;
      sched_ctrl->qos_config[drb_id - 1][q].priority = priority;
      LOG_D(NR_MAC,
            "In %s: drb_id %ld: 5QI %lu priority %lu\n",
            __func__,
            drb_id,
            sched_ctrl->qos_config[drb_id - 1][q].fiveQI,
            sched_ctrl->qos_config[drb_id - 1][q].priority);
    }
  }
}

void ue_context_setup_request(const f1ap_ue_context_setup_t *req)
{
  gNB_MAC_INST *mac = RC.nrmac[0];
  /* response has same type as request... */
  f1ap_ue_context_setup_t resp = {
    .gNB_CU_ue_id = req->gNB_CU_ue_id,
    .gNB_DU_ue_id = req->gNB_DU_ue_id,
  };

  NR_UE_NR_Capability_t *ue_cap = NULL;
  if (req->cu_to_du_rrc_information != NULL) {
    AssertFatal(req->cu_to_du_rrc_information->cG_ConfigInfo == NULL, "CG-ConfigInfo not handled\n");
    ue_cap = get_ue_nr_cap(req->gNB_DU_ue_id,
                           req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList,
                           req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList_length);
    AssertFatal(req->cu_to_du_rrc_information->measConfig == NULL, "MeasConfig not handled\n");
  }

  NR_SCHED_LOCK(&mac->sched_lock);

  NR_UE_info_t *UE = find_nr_UE(&RC.nrmac[0]->UE_info, req->gNB_DU_ue_id);
  AssertFatal(UE != NULL, "did not find UE with RNTI %04x, but UE Context Setup Failed not implemented\n", req->gNB_DU_ue_id);

  NR_CellGroupConfig_t *new_CellGroup = clone_CellGroupConfig(UE->CellGroup);

  if (req->srbs_to_be_setup_length > 0) {
    resp.srbs_to_be_setup_length = handle_ue_context_srbs_setup(req->gNB_DU_ue_id,
                                                                req->srbs_to_be_setup_length,
                                                                req->srbs_to_be_setup,
                                                                &resp.srbs_to_be_setup,
                                                                new_CellGroup);
  }

  if (req->drbs_to_be_setup_length > 0) {
    resp.drbs_to_be_setup_length = handle_ue_context_drbs_setup(req->gNB_DU_ue_id,
                                                                req->drbs_to_be_setup_length,
                                                                req->drbs_to_be_setup,
                                                                &resp.drbs_to_be_setup,
                                                                new_CellGroup);
  }

  if (req->rrc_container != NULL)
    nr_rlc_srb_recv_sdu(req->gNB_DU_ue_id, DCCH, req->rrc_container, req->rrc_container_length);

  UE->capability = ue_cap;
  if (ue_cap != NULL) {
    // store the new UE capabilities, and update the cellGroupConfig
    NR_ServingCellConfigCommon_t *scc = mac->common_channels[0].ServingCellConfigCommon;
    update_cellGroupConfig(new_CellGroup, UE->uid, UE->capability, &mac->radio_config, scc);
  }

  resp.du_to_cu_rrc_information = calloc(1, sizeof(du_to_cu_rrc_information_t));
  AssertFatal(resp.du_to_cu_rrc_information != NULL, "out of memory\n");
  resp.du_to_cu_rrc_information->cellGroupConfig = calloc(1,1024);
  AssertFatal(resp.du_to_cu_rrc_information->cellGroupConfig != NULL, "out of memory\n");
  asn_enc_rval_t enc_rval =
      uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig, NULL, new_CellGroup, resp.du_to_cu_rrc_information->cellGroupConfig, 1024);
  AssertFatal(enc_rval.encoded > 0, "Could not encode CellGroup, failed element %s\n", enc_rval.failed_type->name);
  resp.du_to_cu_rrc_information->cellGroupConfig_length = (enc_rval.encoded + 7) >> 3;

  /* TODO: need to apply after UE context reconfiguration confirmed? */
  nr_mac_prepare_cellgroup_update(mac, UE, new_CellGroup);

  /* Fill the QoS config in MAC for each active DRB */
  set_QoSConfig(req, &UE->UE_sched_ctrl);

  /* Set NSSAI config in MAC for each active DRB */
  set_nssaiConfig(req->drbs_to_be_setup_length, req->drbs_to_be_setup, &UE->UE_sched_ctrl);

  NR_SCHED_UNLOCK(&mac->sched_lock);

  /* some sanity checks, since we use the same type for request and response */
  DevAssert(resp.cu_to_du_rrc_information == NULL);
  DevAssert(resp.du_to_cu_rrc_information != NULL);
  DevAssert(resp.rrc_container == NULL && resp.rrc_container_length == 0);

  mac->mac_rrc.ue_context_setup_response(req, &resp);

  /* free the memory we allocated above */
  free(resp.srbs_to_be_setup);
  free(resp.drbs_to_be_setup);
  free(resp.du_to_cu_rrc_information->cellGroupConfig);
  free(resp.du_to_cu_rrc_information);
}

void ue_context_modification_request(const f1ap_ue_context_modif_req_t *req)
{
  gNB_MAC_INST *mac = RC.nrmac[0];
  f1ap_ue_context_modif_resp_t resp = {
    .gNB_CU_ue_id = req->gNB_CU_ue_id,
    .gNB_DU_ue_id = req->gNB_DU_ue_id,
  };

  NR_UE_NR_Capability_t *ue_cap = NULL;
  if (req->cu_to_du_rrc_information != NULL) {
    AssertFatal(req->cu_to_du_rrc_information->cG_ConfigInfo == NULL, "CG-ConfigInfo not handled\n");
    ue_cap = get_ue_nr_cap(req->gNB_DU_ue_id,
                           req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList,
                           req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList_length);
    AssertFatal(req->cu_to_du_rrc_information->measConfig == NULL, "MeasConfig not handled\n");
  }

  NR_SCHED_LOCK(&mac->sched_lock);
  NR_UE_info_t *UE = find_nr_UE(&RC.nrmac[0]->UE_info, req->gNB_DU_ue_id);
  if (!UE) {
    LOG_E(NR_MAC, "could not find UE with RNTI %04x\n", req->gNB_DU_ue_id);
    NR_SCHED_UNLOCK(&mac->sched_lock);
    return;
  }

  NR_CellGroupConfig_t *new_CellGroup = clone_CellGroupConfig(UE->CellGroup);

  if (req->srbs_to_be_setup_length > 0) {
    resp.srbs_to_be_setup_length = handle_ue_context_srbs_setup(req->gNB_DU_ue_id,
                                                                req->srbs_to_be_setup_length,
                                                                req->srbs_to_be_setup,
                                                                &resp.srbs_to_be_setup,
                                                                new_CellGroup);
  }

  if (req->drbs_to_be_setup_length > 0) {
    resp.drbs_to_be_setup_length = handle_ue_context_drbs_setup(req->gNB_DU_ue_id,
                                                                req->drbs_to_be_setup_length,
                                                                req->drbs_to_be_setup,
                                                                &resp.drbs_to_be_setup,
                                                                new_CellGroup);
  }

  if (req->drbs_to_be_released_length > 0) {
    resp.drbs_to_be_released_length =
        handle_ue_context_drbs_release(req->gNB_DU_ue_id, req->drbs_to_be_released_length, req->drbs_to_be_released, new_CellGroup);
  }

  if (req->rrc_container != NULL)
    nr_rlc_srb_recv_sdu(req->gNB_DU_ue_id, DCCH, req->rrc_container, req->rrc_container_length);

  if (req->ReconfigComplOutcome != RRCreconf_info_not_present && req->ReconfigComplOutcome != RRCreconf_success) {
    LOG_E(NR_MAC,
          "RRC reconfiguration outcome unsuccessful, but no rollback mechanism implemented to come back to old configuration\n");
  }

  if (ue_cap != NULL) {
    // store the new UE capabilities, and update the cellGroupConfig
    ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability, UE->capability);
    UE->capability = ue_cap;
    LOG_I(NR_MAC, "UE %04x: received capabilities, updating CellGroupConfig\n", UE->rnti);
    NR_ServingCellConfigCommon_t *scc = mac->common_channels[0].ServingCellConfigCommon;
    update_cellGroupConfig(new_CellGroup, UE->uid, UE->capability, &mac->radio_config, scc);
  }

  if (req->srbs_to_be_setup_length > 0 || req->drbs_to_be_setup_length > 0 || req->drbs_to_be_released_length > 0
      || ue_cap != NULL) {
    resp.du_to_cu_rrc_information = calloc(1, sizeof(du_to_cu_rrc_information_t));
    AssertFatal(resp.du_to_cu_rrc_information != NULL, "out of memory\n");
    resp.du_to_cu_rrc_information->cellGroupConfig = calloc(1, 1024);
    AssertFatal(resp.du_to_cu_rrc_information->cellGroupConfig != NULL, "out of memory\n");
    asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig,
                                                    NULL,
                                                    new_CellGroup,
                                                    resp.du_to_cu_rrc_information->cellGroupConfig,
                                                    1024);
    AssertFatal(enc_rval.encoded > 0, "Could not encode CellGroup, failed element %s\n", enc_rval.failed_type->name);
    resp.du_to_cu_rrc_information->cellGroupConfig_length = (enc_rval.encoded + 7) >> 3;

    nr_mac_prepare_cellgroup_update(mac, UE, new_CellGroup);

    /* Fill the QoS config in MAC for each active DRB */
    set_QoSConfig(req, &UE->UE_sched_ctrl);

    /* Set NSSAI config in MAC for each active DRB */
    set_nssaiConfig(req->drbs_to_be_setup_length, req->drbs_to_be_setup, &UE->UE_sched_ctrl);
  } else {
    ASN_STRUCT_FREE(asn_DEF_NR_CellGroupConfig, new_CellGroup); // we actually don't need it
  }
  NR_SCHED_UNLOCK(&mac->sched_lock);

  /* some sanity checks, since we use the same type for request and response */
  DevAssert(resp.cu_to_du_rrc_information == NULL);
  // resp.du_to_cu_rrc_information can be either NULL or not
  DevAssert(resp.rrc_container == NULL && resp.rrc_container_length == 0);

  mac->mac_rrc.ue_context_modification_response(req, &resp);

  /* free the memory we allocated above */
  free(resp.srbs_to_be_setup);
  free(resp.drbs_to_be_setup);
  if (resp.du_to_cu_rrc_information != NULL) {
    free(resp.du_to_cu_rrc_information->cellGroupConfig);
    free(resp.du_to_cu_rrc_information);
  }
}

void ue_context_modification_confirm(const f1ap_ue_context_modif_confirm_t *confirm)
{
  LOG_I(MAC, "Received UE Context Modification Confirm for UE %04x\n", confirm->gNB_DU_ue_id);

  gNB_MAC_INST *mac = RC.nrmac[0];
  NR_SCHED_LOCK(&mac->sched_lock);
  /* check first that the scheduler knows such UE */
  NR_UE_info_t *UE = find_nr_UE(&mac->UE_info, confirm->gNB_DU_ue_id);
  if (UE == NULL) {
    LOG_E(MAC, "ERROR: unknown UE with RNTI %04x, ignoring UE Context Modification Confirm\n", confirm->gNB_DU_ue_id);
    NR_SCHED_UNLOCK(&mac->sched_lock);
    return;
  }
  NR_SCHED_UNLOCK(&mac->sched_lock);

  if (confirm->rrc_container_length > 0)
    nr_rlc_srb_recv_sdu(confirm->gNB_DU_ue_id, DCCH, confirm->rrc_container, confirm->rrc_container_length);

  /* nothing else to be done? */
}

void ue_context_modification_refuse(const f1ap_ue_context_modif_refuse_t *refuse)
{
  /* Currently, we only use the UE Context Modification Required procedure to
   * trigger a RRC reconfigurtion after Msg.3 with C-RNTI MAC CE. If the CU
   * refuses, it cannot do this reconfiguration, leaving the UE in an
   * unconfigured state. Therefore, we just free all RA-related info, and
   * request the release of the UE.  */
  LOG_W(MAC, "Received UE Context Modification Refuse for %04x, requesting release\n", refuse->gNB_DU_ue_id);

  gNB_MAC_INST *mac = RC.nrmac[0];
  NR_SCHED_LOCK(&mac->sched_lock);
  NR_UE_info_t *UE = find_nr_UE(&RC.nrmac[0]->UE_info, refuse->gNB_DU_ue_id);
  if (UE == NULL) {
    LOG_E(MAC, "ERROR: unknown UE with RNTI %04x, ignoring UE Context Modification Refuse\n", refuse->gNB_DU_ue_id);
    NR_SCHED_UNLOCK(&mac->sched_lock);
    return;
  }

  const int CC_id = 0;
  NR_COMMON_channels_t *cc = &mac->common_channels[CC_id];
  for (int i = 0; i < NR_NB_RA_PROC_MAX; i++) {
    NR_RA_t *ra = &cc->ra[i];
    if (ra->rnti == UE->rnti)
      nr_clear_ra_proc(0, CC_id, 0 /* frame */, ra);
  }
  NR_SCHED_UNLOCK(&mac->sched_lock);

  f1ap_ue_context_release_req_t request = {
    .gNB_CU_ue_id = refuse->gNB_CU_ue_id,
    .gNB_DU_ue_id = refuse->gNB_DU_ue_id,
    .cause = F1AP_CAUSE_RADIO_NETWORK,
    .cause_value = F1AP_CauseRadioNetwork_procedure_cancelled,
  };
  mac->mac_rrc.ue_context_release_request(&request);
}

void ue_context_release_command(const f1ap_ue_context_release_cmd_t *cmd)
{
  /* mark UE as to be deleted after PUSCH failure */
  gNB_MAC_INST *mac = RC.nrmac[0];
  NR_SCHED_LOCK(&mac->sched_lock);
  NR_UE_info_t *UE = find_nr_UE(&mac->UE_info, cmd->gNB_DU_ue_id);
  if (UE == NULL) {
    LOG_E(MAC, "ERROR: unknown UE with RNTI %04x, ignoring UE Context Release Command\n", cmd->gNB_DU_ue_id);
    NR_SCHED_UNLOCK(&mac->sched_lock);
    return;
  }

  if (UE->UE_sched_ctrl.ul_failure || cmd->rrc_container_length == 0) {
    /* The UE is already not connected anymore or we have nothing to forward*/
    nr_mac_release_ue(mac, cmd->gNB_DU_ue_id);
  } else {
    /* UE is in sync: forward release message and mark to be deleted
     * after UL failure */
    nr_rlc_srb_recv_sdu(cmd->gNB_DU_ue_id, cmd->srb_id, cmd->rrc_container, cmd->rrc_container_length);
    nr_mac_trigger_release_timer(&UE->UE_sched_ctrl, UE->current_UL_BWP.scs);
  }
  NR_SCHED_UNLOCK(&mac->sched_lock);
}

void dl_rrc_message_transfer(const f1ap_dl_rrc_message_t *dl_rrc)
{
  LOG_D(NR_MAC,
        "DL RRC Message Transfer with %d bytes for RNTI %04x SRB %d\n",
        dl_rrc->rrc_container_length,
        dl_rrc->gNB_DU_ue_id,
        dl_rrc->srb_id);

  gNB_MAC_INST *mac = RC.nrmac[0];
  pthread_mutex_lock(&mac->sched_lock);
  /* check first that the scheduler knows such UE */
  NR_UE_info_t *UE = find_nr_UE(&mac->UE_info, dl_rrc->gNB_DU_ue_id);
  if (UE == NULL) {
    LOG_E(MAC, "ERROR: unknown UE with RNTI %04x, ignoring DL RRC Message Transfer\n", dl_rrc->gNB_DU_ue_id);
    pthread_mutex_unlock(&mac->sched_lock);
    return;
  }
  pthread_mutex_unlock(&mac->sched_lock);

  if (!du_exists_f1_ue_data(dl_rrc->gNB_DU_ue_id)) {
    LOG_I(NR_MAC, "No CU UE ID stored for UE RNTI %04x, adding CU UE ID %d\n", dl_rrc->gNB_DU_ue_id, dl_rrc->gNB_CU_ue_id);
    f1_ue_data_t new_ue_data = {.secondary_ue = dl_rrc->gNB_CU_ue_id};
    du_add_f1_ue_data(dl_rrc->gNB_DU_ue_id, &new_ue_data);
  }

  if (UE->expect_reconfiguration && dl_rrc->srb_id == DCCH) {
    /* we expected a reconfiguration, and this is on DCCH. We assume this is
     * the reconfiguration: nr_mac_prepare_cellgroup_update() already stored
     * the CellGroupConfig. Below, we trigger a timer, and the CellGroupConfig
     * will be applied after its expiry in nr_mac_apply_cellgroup().*/
    NR_SCHED_LOCK(&mac->sched_lock);
    nr_mac_enable_ue_rrc_processing_timer(mac, UE, /* apply_cellGroup = */ true);
    NR_SCHED_UNLOCK(&mac->sched_lock);
    UE->expect_reconfiguration = false;
  }

  if (dl_rrc->old_gNB_DU_ue_id != NULL) {
    AssertFatal(*dl_rrc->old_gNB_DU_ue_id != dl_rrc->gNB_DU_ue_id,
                "logic bug: current and old gNB DU UE ID cannot be the same\n");
    /* 38.401 says: "Find UE context based on old gNB-DU UE F1AP ID, replace
     * old C-RNTI/PCI with new C-RNTI/PCI". Below, we do the inverse: we keep
     * the new UE context (with new C-RNTI), but set up everything to reuse the
     * old config. */
    NR_UE_info_t *oldUE = find_nr_UE(&mac->UE_info, *dl_rrc->old_gNB_DU_ue_id);
    DevAssert(oldUE);
    pthread_mutex_lock(&mac->sched_lock);
    /* 38.331 5.3.7.2 says that the UE releases the spCellConfig, so we drop it
     * from the current configuration. Also, expect the reconfiguration from
     * the CU, so save the old UE's CellGroup for the new UE */
    UE->CellGroup->spCellConfig = NULL;
    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
    NR_ServingCellConfigCommon_t *scc = mac->common_channels[0].ServingCellConfigCommon;
    uid_t temp_uid = UE->uid;
    UE->uid = oldUE->uid;
    oldUE->uid = temp_uid;
    configure_UE_BWP(mac, scc, sched_ctrl, NULL, UE, -1, -1);

    nr_mac_prepare_cellgroup_update(mac, UE, oldUE->CellGroup);
    oldUE->CellGroup = NULL;
    mac_remove_nr_ue(mac, *dl_rrc->old_gNB_DU_ue_id);
    pthread_mutex_unlock(&mac->sched_lock);
    nr_rlc_remove_ue(dl_rrc->gNB_DU_ue_id);
    nr_rlc_update_rnti(*dl_rrc->old_gNB_DU_ue_id, dl_rrc->gNB_DU_ue_id);
  }

  /* the DU ue id is the RNTI */
  nr_rlc_srb_recv_sdu(dl_rrc->gNB_DU_ue_id, dl_rrc->srb_id, dl_rrc->rrc_container, dl_rrc->rrc_container_length);
}
