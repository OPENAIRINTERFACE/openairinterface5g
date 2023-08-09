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
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "openair2/RRC/NR/MESSAGES/asn1_msg.h"

#include "uper_decoder.h"
#include "uper_encoder.h"

static NR_RLC_BearerConfig_t *get_bearerconfig_from_srb(const f1ap_srb_to_be_setup_t *srb)
{
  long priority = srb->srb_id; // high priority for SRB
  e_NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration bucket =
      NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms5;
  return get_SRB_RLC_BearerConfig(srb->srb_id, priority, bucket);
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
  return get_DRB_RLC_BearerConfig(3 + drb->drb_id, drb->drb_id, rlc_conf, priority);
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

void ue_context_setup_request(const f1ap_ue_context_setup_t *req)
{
  gNB_MAC_INST *mac = RC.nrmac[0];
  /* response has same type as request... */
  f1ap_ue_context_setup_t resp = {
    .gNB_CU_ue_id = req->gNB_CU_ue_id,
    .gNB_DU_ue_id = req->gNB_DU_ue_id,
    .rnti = req->rnti,
  };

  if (req->cu_to_du_rrc_information != NULL) {
    AssertFatal(req->cu_to_du_rrc_information->cG_ConfigInfo == NULL, "CG-ConfigInfo not handled\n");
    AssertFatal(req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList == NULL, "UE capabilities not handled yet\n");
    AssertFatal(req->cu_to_du_rrc_information->measConfig == NULL, "MeasConfig not handled\n");
  }

  NR_SCHED_LOCK(&mac->sched_lock);

  NR_UE_info_t *UE = find_nr_UE(&RC.nrmac[0]->UE_info, req->rnti);
  AssertFatal(UE != NULL, "did not find UE with RNTI %04x, but UE Context Setup Failed not implemented\n", req->rnti);

  if (req->srbs_to_be_setup_length > 0) {
    resp.srbs_to_be_setup_length = handle_ue_context_srbs_setup(req->rnti,
                                                                req->srbs_to_be_setup_length,
                                                                req->srbs_to_be_setup,
                                                                &resp.srbs_to_be_setup,
                                                                UE->CellGroup);
  }

  if (req->drbs_to_be_setup_length > 0) {
    resp.drbs_to_be_setup_length = handle_ue_context_drbs_setup(req->rnti,
                                                                req->drbs_to_be_setup_length,
                                                                req->drbs_to_be_setup,
                                                                &resp.drbs_to_be_setup,
                                                                UE->CellGroup);
  }

  if (req->rrc_container != NULL)
    nr_rlc_srb_recv_sdu(req->rnti, DCCH, req->rrc_container, req->rrc_container_length);

  //nr_mac_update_cellgroup()
  resp.du_to_cu_rrc_information = calloc(1, sizeof(du_to_cu_rrc_information_t));
  AssertFatal(resp.du_to_cu_rrc_information != NULL, "out of memory\n");
  resp.du_to_cu_rrc_information->cellGroupConfig = calloc(1,1024);
  AssertFatal(resp.du_to_cu_rrc_information->cellGroupConfig != NULL, "out of memory\n");
  asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig,
                                                  NULL,
                                                  UE->CellGroup,
                                                  resp.du_to_cu_rrc_information->cellGroupConfig,
                                                  1024);
  AssertFatal(enc_rval.encoded > 0, "Could not encode CellGroup, failed element %s\n", enc_rval.failed_type->name);
  resp.du_to_cu_rrc_information->cellGroupConfig_length = (enc_rval.encoded + 7) >> 3;

  /* TODO: need to apply after UE context reconfiguration confirmed? */
  process_CellGroup(UE->CellGroup, UE);

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
    .rnti = req->rnti,
  };

  if (req->cu_to_du_rrc_information != NULL) {
    AssertFatal(req->cu_to_du_rrc_information->cG_ConfigInfo == NULL, "CG-ConfigInfo not handled\n");
    AssertFatal(req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList == NULL, "UE capabilities not handled yet\n");
    AssertFatal(req->cu_to_du_rrc_information->measConfig == NULL, "MeasConfig not handled\n");
  }

  NR_SCHED_LOCK(&mac->sched_lock);
  NR_UE_info_t *UE = find_nr_UE(&RC.nrmac[0]->UE_info, req->rnti);

  if (req->srbs_to_be_setup_length > 0) {
    resp.srbs_to_be_setup_length = handle_ue_context_srbs_setup(req->rnti,
                                                                req->srbs_to_be_setup_length,
                                                                req->srbs_to_be_setup,
                                                                &resp.srbs_to_be_setup,
                                                                UE->CellGroup);
  }

  if (req->drbs_to_be_setup_length > 0) {
    resp.drbs_to_be_setup_length = handle_ue_context_drbs_setup(req->rnti,
                                                                req->drbs_to_be_setup_length,
                                                                req->drbs_to_be_setup,
                                                                &resp.drbs_to_be_setup,
                                                                UE->CellGroup);
  }

  if (req->rrc_container != NULL)
    nr_rlc_srb_recv_sdu(req->rnti, DCCH, req->rrc_container, req->rrc_container_length);

  if (req->ReconfigComplOutcome != RRCreconf_info_not_present && req->ReconfigComplOutcome != RRCreconf_success) {
    LOG_E(NR_MAC,
          "RRC reconfiguration outcome unsuccessful, but no rollback mechanism implemented to come back to old configuration\n");
  }

  if (req->srbs_to_be_setup_length > 0 || req->drbs_to_be_setup_length > 0) {
    /* TODO: if we change e.g. BWP or MCS table, need to automatically call
     * configure_UE_BWP() (form nr_mac_update_timers()) after some time? */

    resp.du_to_cu_rrc_information = calloc(1, sizeof(du_to_cu_rrc_information_t));
    AssertFatal(resp.du_to_cu_rrc_information != NULL, "out of memory\n");
    resp.du_to_cu_rrc_information->cellGroupConfig = calloc(1, 1024);
    AssertFatal(resp.du_to_cu_rrc_information->cellGroupConfig != NULL, "out of memory\n");
    asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig,
                                                    NULL,
                                                    UE->CellGroup,
                                                    resp.du_to_cu_rrc_information->cellGroupConfig,
                                                    1024);
    AssertFatal(enc_rval.encoded > 0, "Could not encode CellGroup, failed element %s\n", enc_rval.failed_type->name);
    resp.du_to_cu_rrc_information->cellGroupConfig_length = (enc_rval.encoded + 7) >> 3;

    /* works? */
    nr_mac_update_cellgroup(RC.nrmac[0], req->rnti, UE->CellGroup);
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

void ue_context_release_command(const f1ap_ue_context_release_cmd_t *cmd)
{
  /* mark UE as to be deleted after PUSCH failure */
  gNB_MAC_INST *mac = RC.nrmac[0];
  pthread_mutex_lock(&mac->sched_lock);
  NR_UE_info_t *UE = find_nr_UE(&mac->UE_info, cmd->rnti);
  if (UE->UE_sched_ctrl.ul_failure || cmd->rrc_container_length == 0) {
    /* The UE is already not connected anymore or we have nothing to forward*/
    nr_rlc_remove_ue(cmd->rnti);
    mac_remove_nr_ue(mac, cmd->rnti);
  } else {
    /* UE is in sync: forward release message and mark to be deleted
     * after UL failure */
    nr_rlc_srb_recv_sdu(cmd->rnti, cmd->srb_id, cmd->rrc_container, cmd->rrc_container_length);
    nr_mac_trigger_release_timer(&UE->UE_sched_ctrl, UE->current_UL_BWP.scs);
  }
  pthread_mutex_unlock(&mac->sched_lock);

  f1ap_ue_context_release_complete_t complete = {
    .rnti = cmd->rnti,
  };
  mac->mac_rrc.ue_context_release_complete(&complete);

  if (cmd->rrc_container)
    free(cmd->rrc_container);
}

int dl_rrc_message(module_id_t module_id, const f1ap_dl_rrc_message_t *dl_rrc)
{
  LOG_D(NR_MAC, "DL RRC Message Transfer with %d bytes for RNTI %04x SRB %d\n", dl_rrc->rrc_container_length, dl_rrc->rnti, dl_rrc->srb_id);

  nr_rlc_srb_recv_sdu(dl_rrc->rnti, dl_rrc->srb_id, dl_rrc->rrc_container, dl_rrc->rrc_container_length);
  return 0;
}
