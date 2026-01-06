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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "F1AP_CauseMisc.h"
#include "F1AP_CauseProtocol.h"
#include "F1AP_CauseRadioNetwork.h"
#include "T.h"
#include "asn_codecs.h"
#include "assertions.h"
#include "common/ran_context.h"
#include "common/utils/nr/nr_common.h"
#include "common/utils/T/T.h"
#include "common/utils/alg/foreach.h"
#include "common/utils/ds/seq_arr.h"
#include "constr_TYPE.h"
#include "executables/softmodem-common.h"
#include "f1ap_messages_types.h"
#include "ngap_messages_types.h"
#include "nr_rrc_defs.h"
#include "rrc_gNB_mobility.h"
#include "openair2/F1AP/f1ap_common.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "lib/f1ap_interface_management.h"
#include "rrc_gNB_NGAP.h"
#include "rrc_gNB_UE_context.h"
#include "rrc_messages_types.h"
#include "s1ap_messages_types.h"
#include "tree.h"
#include "uper_decoder.h"
#include "utils.h"
#include "xer_encoder.h"

int get_dl_band(const struct f1ap_served_cell_info_t *cell_info)
{
  return cell_info->mode == F1AP_MODE_TDD ? cell_info->tdd.freqinfo.band : cell_info->fdd.dl_freqinfo.band;
}

int get_ssb_scs(const struct f1ap_served_cell_info_t *cell_info)
{
  return cell_info->mode == F1AP_MODE_TDD ? cell_info->tdd.tbw.scs : cell_info->fdd.dl_tbw.scs;
}

static NR_SSB_MTC_t *get_ssb_mtc(const NR_MeasurementTimingConfiguration_t *mtc)
{
  // TODO verify which element of the list to pick
  NR_MeasTimingList_t *mtlist = mtc->criticalExtensions.choice.c1->choice.measTimingConf->measTiming;
  NR_SSB_MTC_t *ssb_mtc = calloc(1, sizeof(*ssb_mtc));
  *ssb_mtc = mtlist->list.array[0]->frequencyAndTiming->ssb_MeasurementTimingConfiguration;
  return ssb_mtc;
}

static int ssb_arfcn_mtc(const NR_MeasurementTimingConfiguration_t *mtc)
{
  /* format has been verified when accepting MeasurementTimingConfiguration */
  NR_MeasTimingList_t *mtlist = mtc->criticalExtensions.choice.c1->choice.measTimingConf->measTiming;
  return mtlist->list.array[0]->frequencyAndTiming->carrierFreq;
}

int get_ssb_arfcn(const struct nr_rrc_du_container_t *du)
{
  DevAssert(du != NULL && du->mtc != NULL);
  return ssb_arfcn_mtc(du->mtc);
}

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
         && conf->plmn[0].mcc == info->plmn.mcc && conf->plmn[0].mnc == info->plmn.mnc;
}

static bool extract_sys_info(const f1ap_gnb_du_system_info_t *sys_info, NR_MIB_t **mib, NR_SIB1_t **sib1)
{
  DevAssert(sys_info != NULL);
  DevAssert(mib != NULL);
  DevAssert(sib1 != NULL);

  asn_dec_rval_t dec_rval = uper_decode_complete(NULL, &asn_DEF_NR_MIB, (void **)mib, sys_info->mib, sys_info->mib_length);
  if (dec_rval.code != RC_OK) {
    LOG_E(NR_RRC, "Failed to decode NR_MIB (%zu bits) of DU\n", dec_rval.consumed);
    ASN_STRUCT_FREE(asn_DEF_NR_MIB, *mib);
    return false;
  }

  if (sys_info->sib1) {
    dec_rval = uper_decode_complete(NULL, &asn_DEF_NR_SIB1, (void **)sib1, sys_info->sib1, sys_info->sib1_length);
    if (dec_rval.code != RC_OK) {
      ASN_STRUCT_FREE(asn_DEF_NR_MIB, *mib);
      ASN_STRUCT_FREE(asn_DEF_NR_SIB1, *sib1);
      LOG_E(NR_RRC, "Failed to decode NR_SIB1 (%zu bits), rejecting DU\n", dec_rval.consumed);
      return false;
    }
  }

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NR_MIB, *mib);
    xer_fprint(stdout, &asn_DEF_NR_SIB1, *sib1);
  }

  return true;
}

static NR_MeasurementTimingConfiguration_t *extract_mtc(uint8_t *buf, int buf_len)
{
  NR_MeasurementTimingConfiguration_t *mtc = NULL;
  asn_dec_rval_t dec_rval = uper_decode_complete(NULL, &asn_DEF_NR_MeasurementTimingConfiguration, (void **)&mtc, buf, buf_len);
  if (dec_rval.code != RC_OK) {
    ASN_STRUCT_FREE(asn_DEF_NR_MeasurementTimingConfiguration, mtc);
    return NULL;
  }
  /* verify that it has the format we need */
  if (mtc->criticalExtensions.present != NR_MeasurementTimingConfiguration__criticalExtensions_PR_c1
      || mtc->criticalExtensions.choice.c1 == NULL
      || mtc->criticalExtensions.choice.c1->present != NR_MeasurementTimingConfiguration__criticalExtensions__c1_PR_measTimingConf
      || mtc->criticalExtensions.choice.c1->choice.measTimingConf == NULL
      || mtc->criticalExtensions.choice.c1->choice.measTimingConf->measTiming == NULL
      || mtc->criticalExtensions.choice.c1->choice.measTimingConf->measTiming->list.count == 0) {
    LOG_E(NR_RRC, "error: measurementTimingConfiguration does not have expected format (at least one measTiming entry\n");
    if (LOG_DEBUGFLAG(DEBUG_ASN1))
      xer_fprint(stdout, &asn_DEF_NR_MeasurementTimingConfiguration, mtc);
    ASN_STRUCT_FREE(asn_DEF_NR_MeasurementTimingConfiguration, mtc);
    return NULL;
  }
  return mtc;
}

static int nr_cell_id_match(const void *key, const void *element)
{
  const int *key_id = (const int *)key;
  const neighbour_cell_configuration_t *config_element = (const neighbour_cell_configuration_t *)element;

  if (*key_id < config_element->nr_cell_id) {
    return -1;
  } else if (*key_id == config_element->nr_cell_id) {
    return 0;
  }

  return 1;
}

static neighbour_cell_configuration_t *get_cell_neighbour_list(const gNB_RRC_INST *rrc, const f1ap_served_cell_info_t *cell_info)
{
  void *base = seq_arr_front(rrc->neighbour_cell_configuration);
  size_t nmemb = seq_arr_size(rrc->neighbour_cell_configuration);
  size_t size = sizeof(neighbour_cell_configuration_t);

  void *it = bsearch((void *)&cell_info->nr_cellid, base, nmemb, size, nr_cell_id_match);

  return (neighbour_cell_configuration_t *)it;
}

const struct f1ap_served_cell_info_t *get_cell_information_by_phycellId(int phyCellId)
{
  gNB_RRC_INST *rrc = RC.nrrrc[0];
  nr_rrc_du_container_t *it = NULL;
  RB_FOREACH (it, rrc_du_tree, &rrc->dus) {
    for (int cellIdx = 0; cellIdx < it->setup_req->num_cells_available; cellIdx++) {
      const f1ap_served_cell_info_t *cell_info = &(it->setup_req->cell[cellIdx].info);
      if (cell_info->nr_pci == phyCellId) {
        LOG_D(NR_RRC, "HO LOG: Found cell with phyCellId %d\n", phyCellId);
        return cell_info;
      }
    }
  }
  return NULL;
}

static void is_intra_frequency_neighbour(void *ssb_arfcn, void *neighbour_cell)
{
  uint32_t *ssb_arfcn_ptr = (uint32_t *)ssb_arfcn;
  nr_neighbour_cell_t *neighbour_cell_ptr = (nr_neighbour_cell_t *)neighbour_cell;

  if (*ssb_arfcn_ptr == neighbour_cell_ptr->absoluteFrequencySSB) {
    LOG_D(NR_RRC, "HO LOG: found intra frequency neighbour %lu!\n", neighbour_cell_ptr->nrcell_id);
    neighbour_cell_ptr->isIntraFrequencyNeighbour = true;
  }
}
/**
 * @brief Labels neighbour cells if they are intra frequency to prepare meas config only for intra frequency ho
 * @param[in] rrc     Pointer to RRC instance
 * @param[in] cell_info Pointer to cell information
 */
static void label_intra_frequency_neighbours(gNB_RRC_INST *rrc,
                                             const nr_rrc_du_container_t *du,
                                             const f1ap_served_cell_info_t *cell_info)
{
  if (!rrc->neighbour_cell_configuration)
    return;

  neighbour_cell_configuration_t *neighbour_cell_config = get_cell_neighbour_list(rrc, cell_info);
  if (!neighbour_cell_config)
    return;

  LOG_D(NR_RRC, "HO LOG: Cell: %lu has neighbour cell configuration!\n", cell_info->nr_cellid);
  uint32_t ssb_arfcn = get_ssb_arfcn(du);

  seq_arr_t *cell_neighbour_list = neighbour_cell_config->neighbour_cells;
  for_each(cell_neighbour_list, (void *)&ssb_arfcn, is_intra_frequency_neighbour);
}

static bool valid_du_in_neighbour_configs(const seq_arr_t *neighbour_cell_configuration, const f1ap_served_cell_info_t *cell, int ssb_arfcn)
{
  for (int c = 0; c < neighbour_cell_configuration->size; c++) {
    const neighbour_cell_configuration_t *neighbour_config = seq_arr_at(neighbour_cell_configuration, c);
    for (int ni = 0; ni < neighbour_config->neighbour_cells->size; ni++) {
      const nr_neighbour_cell_t *nc = seq_arr_at(neighbour_config->neighbour_cells, ni);
      if (nc->nrcell_id != cell->nr_cellid)
        continue;
      // current cell is in the nc config, check that config matches
      if (nc->physicalCellId != cell->nr_pci) {
        LOG_W(NR_RRC, "Cell %ld in neighbour config: PCI mismatch (%d vs %d)\n", cell->nr_cellid, cell->nr_pci, nc->physicalCellId);
        return false;
      }
      if (cell->tac && nc->tac != *cell->tac) {
        LOG_W(NR_RRC, "Cell %ld in neighbour config: TAC mismatch (%d vs %d)\n", cell->nr_cellid, *cell->tac, nc->tac);
        return false;
      }
      if (ssb_arfcn != nc->absoluteFrequencySSB) {
        LOG_W(NR_RRC,
              "Cell %ld in neighbour config: SSB ARFCN mismatch (%d vs %d)\n",
              cell->nr_cellid,
              ssb_arfcn,
              nc->absoluteFrequencySSB);
        return false;
      }
      // Check neighbour band
      if (get_dl_band(cell) != nc->band) {
        LOG_W(NR_RRC, "Cell %ld in neighbour config: Band mismatch (%d vs %d)\n", cell->nr_cellid, get_dl_band(cell), nc->band);
        return false;
      }
      if (get_ssb_scs(cell) != nc->subcarrierSpacing) {
        LOG_W(NR_RRC,
              "Cell %ld in neighbour config: SCS mismatch (%d vs %d)\n",
              cell->nr_cellid,
              get_ssb_scs(cell),
              nc->subcarrierSpacing);
        return false;
      }
      if (cell->plmn.mcc != nc->plmn.mcc || cell->plmn.mnc != nc->plmn.mnc
          || cell->plmn.mnc_digit_length != nc->plmn.mnc_digit_length) {
        LOG_W(NR_RRC,
              "Cell %ld in neighbour config: PLMN mismatch (%03d.%0*d vs %03d.%0*d)\n",
              cell->nr_cellid,
              cell->plmn.mcc,
              cell->plmn.mnc_digit_length,
              cell->plmn.mnc,
              nc->plmn.mcc,
              nc->plmn.mnc_digit_length,
              nc->plmn.mnc);
        return false;
      }
      LOG_D(NR_RRC, "Cell %ld is neighbor of cell %ld\n", cell->nr_cellid, neighbour_config->nr_cell_id);
    }
  }
  return true;
}

void rrc_gNB_process_f1_setup_req(f1ap_setup_req_t *req, sctp_assoc_t assoc_id)
{
  AssertFatal(assoc_id != 0, "illegal assoc_id == 0: should be -1 (monolithic) or >0 (split)\n");
  gNB_RRC_INST *rrc = RC.nrrrc[0];
  DevAssert(rrc);

  LOG_I(NR_RRC, "Received F1 Setup Request from gNB_DU %lu (%s) on assoc_id %d\n", req->gNB_DU_id, req->gNB_DU_name, assoc_id);
  // pre-fill F1 Setup Failure message
  f1ap_setup_failure_t fail = {.transaction_id = F1AP_get_next_transaction_identifier(0, 0)};

  // check:
  // - it is one cell
  // - PLMN and Cell ID matches
  // - no previous DU with the same ID
  // else reject
  if (req->num_cells_available != 1) {
    LOG_E(NR_RRC, "can only handle on DU cell, but gNB_DU %ld has %d\n", req->gNB_DU_id, req->num_cells_available);
    fail.cause = F1AP_CauseRadioNetwork_gNB_CU_Cell_Capacity_Exceeded;
    rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
    return;
  }
  f1ap_served_cell_info_t *cell_info = &req->cell[0].info;
  // I have to comment this single PLMN match between CU and DU to enable multiple PLMNs broadcasting
  // if (!rrc_gNB_plmn_matches(rrc, cell_info)) {
  //   LOG_E(NR_RRC,
  //         "PLMN mismatch: CU %03d.%0*d, DU %03d%0*d\n",
  //         rrc->configuration.plmn[0].mcc,
  //         rrc->configuration.plmn[0].mnc_digit_length,
  //         rrc->configuration.plmn[0].mnc,
  //         cell_info->plmn.mcc,
  //         cell_info->plmn.mnc_digit_length,
  //         cell_info->plmn.mnc);
  //   fail.cause = F1AP_CauseRadioNetwork_plmn_not_served_by_the_gNB_CU;
  //   rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
  //   return;
  // }
  nr_rrc_du_container_t *it = NULL;
  RB_FOREACH(it, rrc_du_tree, &rrc->dus) {
    if (it->setup_req->gNB_DU_id == req->gNB_DU_id) {
      LOG_E(NR_RRC,
            "gNB-DU ID: existing DU %s on assoc_id %d already has ID %ld, rejecting requesting gNB-DU\n",
            it->setup_req->gNB_DU_name,
            it->assoc_id,
            it->setup_req->gNB_DU_id);
      fail.cause = F1AP_CauseMisc_unspecified;
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
      fail.cause = F1AP_CauseMisc_unspecified;
      rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
      return;
    }
  }

  // MTC is mandatory, but some DUs don't send it in the F1 Setup Request, so
  // "tolerate" this behavior, despite it being mandatory
  NR_MeasurementTimingConfiguration_t *mtc =
      extract_mtc(cell_info->measurement_timing_config, cell_info->measurement_timing_config_len);

  if (rrc->neighbour_cell_configuration
      && !valid_du_in_neighbour_configs(rrc->neighbour_cell_configuration, cell_info, ssb_arfcn_mtc(mtc))) {
    LOG_E(NR_RRC, "problem with DU %ld in neighbor configuration, rejecting DU\n", req->gNB_DU_id);
    f1ap_setup_failure_t fail = {.cause = F1AP_CauseMisc_unspecified};
    rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
    ASN_STRUCT_FREE(asn_DEF_NR_MeasurementTimingConfiguration, mtc);
    return;
  }

  const f1ap_gnb_du_system_info_t *sys_info = req->cell[0].sys_info;
  NR_MIB_t *mib = NULL;
  NR_SIB1_t *sib1 = NULL;

  if (sys_info != NULL && sys_info->mib != NULL && !(sys_info->sib1 == NULL && IS_SA_MODE(get_softmodem_params()))) {
    if (!extract_sys_info(sys_info, &mib, &sib1)) {
      LOG_W(NR_RRC, "rejecting DU ID %ld\n", req->gNB_DU_id);
      fail.cause = F1AP_CauseProtocol_semantic_error;
      rrc->mac_rrc.f1_setup_failure(assoc_id, &fail);
      ASN_STRUCT_FREE(asn_DEF_NR_MeasurementTimingConfiguration, mtc);
      return;
    }
  }
  LOG_I(NR_RRC, "Accepting DU %ld (%s), sending F1 Setup Response\n", req->gNB_DU_id, req->gNB_DU_name);
  LOG_I(NR_RRC, "DU uses RRC version %u.%u.%u\n", req->rrc_ver[0], req->rrc_ver[1], req->rrc_ver[2]);

  // we accept the DU
  nr_rrc_du_container_t *du = calloc(1, sizeof(*du));
  AssertFatal(du, "out of memory\n");
  du->assoc_id = assoc_id;

  /* ITTI will free the setup request message via free(). So the memory
   * "inside" of the message will remain, but the "outside" container no, so
   * allocate memory and copy it in */
  du->setup_req = calloc(1,sizeof(*du->setup_req));
  AssertFatal(du->setup_req, "out of memory\n");
  // Copy F1AP message
  *du->setup_req = cp_f1ap_setup_request(req);
  // MIB can be null and configured later via DU Configuration Update
  du->mib = mib;
  du->sib1 = sib1;
  du->mtc = mtc;
  RB_INSERT(rrc_du_tree, &rrc->dus, du);
  rrc->num_dus++;

  served_cells_to_activate_t cell = {
      .plmn = cell_info->plmn,
      .nr_cellid = cell_info->nr_cellid,
      .nrpci = cell_info->nr_pci,
      .num_SI = 0,
  };

  // Encode CU SIBs and configure setup response with sysinfo
  seq_arr_t *sibs = rrc->SIBs;
  if (sibs) {
    for (int i = 0; i < sibs->size; i++) {
      nr_SIBs_t *sib = (nr_SIBs_t *)seq_arr_at(sibs, i);
      switch (sib->SIB_type) {
        case 2: {
          NR_SSB_MTC_t *ssbmtc = get_ssb_mtc(mtc);
          sib->SIB_size = do_SIB2_NR(&sib->SIB_buffer, ssbmtc);
          cell.SI_msg[cell.num_SI].SI_container = sib->SIB_buffer;
          cell.SI_msg[cell.num_SI].SI_container_length = sib->SIB_size;
          cell.SI_msg[cell.num_SI].SI_type = sib->SIB_type;
          cell.num_SI++;
        } break;
        default:
          AssertFatal(false, "SIB%d not handled yet\n", sib->SIB_type);
      }
    }
  }

  if (du->mib != NULL && du->sib1 != NULL)
    label_intra_frequency_neighbours(rrc, du, cell_info);

  f1ap_setup_resp_t resp = {.transaction_id = req->transaction_id,
                            .num_cells_to_activate = 1,
                            .cells_to_activate[0] = cell};
  int num = read_version(TO_STRING(NR_RRC_VERSION), &resp.rrc_ver[0], &resp.rrc_ver[1], &resp.rrc_ver[2]);
  AssertFatal(num == 3, "could not read RRC version string %s\n", TO_STRING(NR_RRC_VERSION));
  if (rrc->node_name != NULL)
    resp.gNB_CU_name = strdup(rrc->node_name);
  rrc->mac_rrc.f1_setup_response(assoc_id, &resp);
  free_f1ap_setup_response(&resp);

  /* we need to setup one default UE for phy-test and do-ra modes in the MAC */
  if (get_softmodem_params()->phy_test > 0 || get_softmodem_params()->do_ra > 0)
    rrc_add_nsa_user(rrc, NULL, assoc_id);
}

static int invalidate_du_connections(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id)
{
  int count = 0;
  seq_arr_t ue_context_to_remove;
  seq_arr_init(&ue_context_to_remove, sizeof(rrc_gNB_ue_context_t *));
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &rrc->rrc_ue_head) {
    gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
    uint32_t ue_id = UE->rrc_ue_id;
    if (UE->ho_context != NULL)
      LOG_W(NR_RRC, "DU disconnected while handover for UE %d active\n", ue_id);
    f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_id);
    if (ue_data.du_assoc_id != assoc_id)
      continue; /* this UE is on another DU */
    if (IS_SA_MODE(get_softmodem_params())) {
      /* this UE belongs to the DU that disconnected, set du_assoc_id to 0,
       * meaning DU is offline, then trigger release request */
      ue_data.du_assoc_id = 0;
      bool success = cu_update_f1_ue_data(ue_id, &ue_data);
      DevAssert(success);
      ngap_cause_t cause = {.type = NGAP_CAUSE_RADIO_NETWORK, .value = NGAP_CAUSE_RADIO_NETWORK_RADIO_CONNECTION_WITH_UE_LOST};
      rrc_gNB_send_NGAP_UE_CONTEXT_RELEASE_REQ(0, ue_context_p, cause);
      count++;
    } else {
      seq_arr_push_back(&ue_context_to_remove, &ue_context_p, sizeof(ue_context_p));
    }
  }
  for (int i = 0; i < seq_arr_size(&ue_context_to_remove); ++i) {
    /* we retrieve a pointer (=iterator) to the UE context pointer
     * (ue_context_p), so dereference once */
    rrc_gNB_ue_context_t *p = *(rrc_gNB_ue_context_t **)seq_arr_at(&ue_context_to_remove, i);
    rrc_remove_nsa_user_context(rrc, p);
  }
  seq_arr_free(&ue_context_to_remove, NULL);
  return count;
}

static void update_cell_info(nr_rrc_du_container_t *du, const f1ap_served_cell_info_t *new_ci)
{
  DevAssert(du != NULL);
  DevAssert(new_ci != NULL);

  AssertFatal(du->setup_req->num_cells_available == 1, "expected 1 cell for DU, but has %d\n", du->setup_req->num_cells_available);
  f1ap_served_cell_info_t *ci = &du->setup_req->cell[0].info;
  // make sure no memory is allocated
  free_f1ap_cell(ci, NULL);
  *ci = copy_f1ap_served_cell_info(new_ci);

  NR_MeasurementTimingConfiguration_t *new_mtc =
      extract_mtc(new_ci->measurement_timing_config, new_ci->measurement_timing_config_len);
  if (new_mtc != NULL) {
    ASN_STRUCT_FREE(asn_DEF_NR_MeasurementTimingConfiguration, du->mtc);
    du->mtc = new_mtc;
  } else {
    LOG_E(NR_RRC, "error decoding MeasurementTimingConfiguration during cell update, ignoring new config\n");
    ASN_STRUCT_FREE(asn_DEF_NR_MeasurementTimingConfiguration, new_mtc);
  }
}

void rrc_gNB_process_f1_du_configuration_update(f1ap_gnb_du_configuration_update_t *conf_up, sctp_assoc_t assoc_id)
{
  AssertFatal(assoc_id != 0, "illegal assoc_id == 0: should be -1 (monolithic) or >0 (split)\n");
  gNB_RRC_INST *rrc = RC.nrrrc[0];
  DevAssert(rrc);

  // check:
  // - it is one cell
  // - PLMN and Cell ID matches
  // - no previous DU with the same ID
  // else reject

  nr_rrc_du_container_t *du = get_du_by_assoc_id(rrc, assoc_id);
  AssertError(du != NULL, return, "no DU found for assoc_id %d\n", assoc_id);

  const f1ap_served_cell_info_t *info = &du->setup_req->cell[0].info;
  if (conf_up->num_cells_to_add > 0) {
    // Here we check if the number of cell limit is respectet, otherwise send failure
    LOG_W(NR_RRC, "du_configuration_update->cells_to_add_list is not supported yet");
  }

  if (conf_up->num_cells_to_modify > 0) {
    // here the old nrcgi is used to find the cell information, if it exist then we modify consequently otherwise we fail
    AssertFatal(conf_up->num_cells_to_modify == 1, "cannot handle more than one cell!\n");

    if (info->nr_cellid != conf_up->cell_to_modify[0].old_nr_cellid) {
      LOG_W(NR_RRC, "no cell with ID %ld found, ignoring gNB-DU configuration update\n", conf_up->cell_to_modify[0].old_nr_cellid);
      return;
    }

    // verify the new plmn of the cell
    if (!rrc_gNB_plmn_matches(rrc, &conf_up->cell_to_modify[0].info)) {
      LOG_W(NR_RRC, "PLMN does not match, ignoring gNB-DU configuration update\n");
      return;
    }

    update_cell_info(du, &conf_up->cell_to_modify[0].info);

    const f1ap_gnb_du_system_info_t *sys_info = conf_up->cell_to_modify[0].sys_info;

    if (sys_info != NULL && sys_info->mib != NULL && !(sys_info->sib1 == NULL && IS_SA_MODE(get_softmodem_params()))) {
      // MIB is mandatory, so will be overwritten. SIB1 is optional, so will
      // only be overwritten if present in sys_info
      ASN_STRUCT_FREE(asn_DEF_NR_MIB, du->mib);
      if (sys_info->sib1 != NULL) {
        ASN_STRUCT_FREE(asn_DEF_NR_SIB1, du->sib1);
        du->sib1 = NULL;
      }

      NR_MIB_t *mib = NULL;
      if (!extract_sys_info(sys_info, &mib, &du->sib1)) {
        LOG_W(NR_RRC, "cannot update sys_info for DU %ld\n", du->setup_req->gNB_DU_id);
      } else {
        DevAssert(mib != NULL);
        du->mib = mib;
        LOG_I(NR_RRC, "update system information of DU %ld\n", du->setup_req->gNB_DU_id);
      }
    }
    if (du->mib != NULL && du->sib1 != NULL) {
      const f1ap_served_cell_info_t *cell_info = &du->setup_req->cell[0].info;
      label_intra_frequency_neighbours(rrc, du, cell_info);
    }
  }

  if (conf_up->num_cells_to_delete > 0) {
    // delete the cell and send cell to desactive IE in the response.
    LOG_W(NR_RRC, "du_configuration_update->cells_to_delete_list is not supported yet");
  }

  for (int i = 0; i < conf_up->num_status; ++i) {
    const f1ap_cell_status_t *cs = &conf_up->status[i];
    const char *status = cs->service_state == F1AP_STATE_IN_SERVICE ? "in service" : "out of service";
    const plmn_id_t *p = &cs->plmn;
    LOG_I(NR_RRC, "cell PLMN %03d.%0*d Cell ID %ld is %s\n", p->mcc, p->mnc_digit_length, p->mnc, cs->nr_cellid, status);
  }

  /* Send DU Configuration Acknowledgement */
  f1ap_gnb_du_configuration_update_acknowledge_t ack = {.transaction_id = conf_up->transaction_id};
  rrc->mac_rrc.gnb_du_configuration_update_acknowledge(assoc_id, &ack);
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
  LOG_I(NR_RRC, "releasing DU ID %ld (%s) on assoc_id %d\n", req->gNB_DU_id, req->gNB_DU_name, assoc_id);
  ASN_STRUCT_FREE(asn_DEF_NR_MIB, du->mib);
  ASN_STRUCT_FREE(asn_DEF_NR_SIB1, du->sib1);
  ASN_STRUCT_FREE(asn_DEF_NR_MeasurementTimingConfiguration, du->mtc);
  if (du->setup_req)
    free_f1ap_setup_request(du->setup_req);
  free(du->setup_req);
  nr_rrc_du_container_t *removed = RB_REMOVE(rrc_du_tree, &rrc->dus, du);
  DevAssert(removed != NULL);
  free(du);
  rrc->num_dus--;

  int num = invalidate_du_connections(rrc, assoc_id);
  if (num > 0) {
    LOG_I(NR_RRC, "%d UEs lost through DU disconnect\n", num);
  }
}

nr_rrc_du_container_t *get_du_for_ue(gNB_RRC_INST *rrc, uint32_t ue_id)
{
  if (!cu_exists_f1_ue_data(ue_id))
    return NULL;
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_id);
  return get_du_by_assoc_id(rrc, ue_data.du_assoc_id);
}

nr_rrc_du_container_t *get_du_by_assoc_id(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id)
{
  nr_rrc_du_container_t e = {.assoc_id = assoc_id};
  return RB_FIND(rrc_du_tree, &rrc->dus, &e);
}

/* \brief find DU by cell ID. Note: currently the CU is limited to one cell per
 * DU, hence here, DU == cell. Modify this to look up a specific cell. */
nr_rrc_du_container_t *get_du_by_cell_id(gNB_RRC_INST *rrc, uint64_t cell_id)
{
  nr_rrc_du_container_t *du = NULL;
  RB_FOREACH(du, rrc_du_tree, &rrc->dus) {
    if (cell_id == du->setup_req->cell[0].info.nr_cellid)
      return du;
  }
  return NULL;
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
    fprintf(f, ": nrCellID %ld, PCI %d, SSB ARFCN %d\n", info->nr_cellid, info->nr_pci, get_ssb_arfcn(du));

    if (info->mode == F1AP_MODE_TDD) {
      const f1ap_nr_frequency_info_t *fi = &info->tdd.freqinfo;
      const f1ap_transmission_bandwidth_t *tb = &info->tdd.tbw;
      fprintf(f, "    TDD: band %d ARFCN %d SCS %d (kHz) PRB %d\n", fi->band, fi->arfcn, 15 * (1 << tb->scs), tb->nrb);
    } else {
      const f1ap_nr_frequency_info_t *dfi = &info->fdd.dl_freqinfo;
      const f1ap_transmission_bandwidth_t *dtb = &info->fdd.dl_tbw;
      fprintf(f, "    FDD: DL band %d ARFCN %d SCS %d (kHz) PRB %d\n", dfi->band, dfi->arfcn, 15 * (1 << dtb->scs), dtb->nrb);
      const f1ap_nr_frequency_info_t *ufi = &info->fdd.ul_freqinfo;
      const f1ap_transmission_bandwidth_t *utb = &info->fdd.ul_tbw;
      fprintf(f, "         UL band %d ARFCN %d SCS %d (kHz) PRB %d\n", ufi->band, ufi->arfcn, 15 * (1 << utb->scs), utb->nrb);
    }
  }
}

nr_rrc_du_container_t *find_target_du(gNB_RRC_INST *rrc, sctp_assoc_t source_assoc_id)
{
  nr_rrc_du_container_t *target_du = NULL;
  nr_rrc_du_container_t *it = NULL;
  bool next_du = false;
  RB_FOREACH (it, rrc_du_tree, &rrc->dus) {
    if (next_du == false && source_assoc_id != it->assoc_id) {
      continue;
    } else if (source_assoc_id == it->assoc_id) {
      next_du = true;
    } else {
      target_du = it;
      break;
    }
  }
  if (target_du == NULL) {
    RB_FOREACH (it, rrc_du_tree, &rrc->dus) {
      if (source_assoc_id == it->assoc_id) {
        continue;
      } else {
        target_du = it;
        break;
      }
    }
  }
  return target_du;
}

void trigger_f1_reset(gNB_RRC_INST *rrc, sctp_assoc_t du_assoc_id)
{
  f1ap_reset_t reset = {
    .transaction_id = F1AP_get_next_transaction_identifier(0, 0),
    .cause = F1AP_CAUSE_TRANSPORT,
    .cause_value = 1, // F1AP_CauseTransport_transport_resource_unavailable
    .reset_type = F1AP_RESET_ALL, // DU does not support partial reset yet
  };
  rrc->mac_rrc.f1_reset(du_assoc_id, &reset);
}
