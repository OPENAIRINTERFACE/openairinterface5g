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

/*! \file gNB_scheduler_uci.c
 * \brief MAC procedures related to UCI
 * \date 2020
 * \version 1.0
 * \company Eurecom
 */

#include "LAYER2/MAC/mac.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "common/ran_context.h"
#include "nfapi/oai_integration/vendor_ext.h"

extern RAN_CONTEXT_t RC;

void nr_fill_nfapi_pucch(module_id_t mod_id,
                         frame_t frame,
                         sub_frame_t slot,
                         const NR_sched_pucch_t *pucch,
                         int UE_id)
{
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;

  nfapi_nr_ul_tti_request_t *future_ul_tti_req =
      &RC.nrmac[mod_id]->UL_tti_req_ahead[0][pucch->ul_slot];
  AssertFatal(future_ul_tti_req->SFN == pucch->frame
              && future_ul_tti_req->Slot == pucch->ul_slot,
              "future UL_tti_req's frame.slot %d.%d does not match PUCCH %d.%d\n",
              future_ul_tti_req->SFN,
              future_ul_tti_req->Slot,
              pucch->frame,
              pucch->ul_slot);
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE;
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pucch_pdu_t);
  nfapi_nr_pucch_pdu_t *pucch_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pucch_pdu;
  memset(pucch_pdu, 0, sizeof(nfapi_nr_pucch_pdu_t));
  future_ul_tti_req->n_pdus += 1;

  LOG_D(MAC,
        "%4d.%2d Scheduling pucch reception in %4d.%2d: bits SR %d, ACK %d, CSI %d on res %d\n",
        frame,
        slot,
        pucch->frame,
        pucch->ul_slot,
        pucch->sr_flag,
        pucch->dai_c,
        pucch->csi_bits,
        pucch->resource_indicator);

  NR_ServingCellConfigCommon_t *scc = RC.nrmac[mod_id]->common_channels->ServingCellConfigCommon;
  nr_configure_pucch(pucch_pdu,
                     scc,
                     UE_info->UE_sched_ctrl[UE_id].active_ubwp,
                     UE_info->rnti[UE_id],
                     pucch->resource_indicator,
                     pucch->csi_bits,
                     pucch->dai_c,
                     pucch->sr_flag);
}

void nr_schedule_pucch(int Mod_idP,
                       frame_t frameP,
                       sub_frame_t slotP) {
  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  const NR_list_t *UE_list = &UE_info->list;

  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    const int n = sizeof(sched_ctrl->sched_pucch) / sizeof(*sched_ctrl->sched_pucch);
    for (int i = 0; i < n; i++) {
      NR_sched_pucch_t *curr_pucch = &UE_info->UE_sched_ctrl[UE_id].sched_pucch[i];
      const uint16_t O_ack = curr_pucch->dai_c;
      const uint16_t O_csi = curr_pucch->csi_bits;
      const uint8_t O_sr = curr_pucch->sr_flag;
      if (O_ack + O_csi + O_sr == 0
          || frameP != curr_pucch->frame
          || slotP != curr_pucch->ul_slot)
        continue;

      nr_fill_nfapi_pucch(Mod_idP, frameP, slotP, curr_pucch, UE_id);
      memset(curr_pucch, 0, sizeof(*curr_pucch));
    }
  }
}


//!TODO : same function can be written to handle csi_resources
void compute_csi_bitlen (NR_CellGroupConfig_t *secondaryCellGroup, NR_UE_info_t *UE_info, int UE_id) {
  uint8_t csi_report_id = 0;
  uint8_t csi_resourceidx =0;
  uint8_t csi_ssb_idx =0;

  NR_CSI_MeasConfig_t *csi_MeasConfig = secondaryCellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup;
  NR_CSI_ResourceConfigId_t csi_ResourceConfigId;
  for (csi_report_id=0; csi_report_id < csi_MeasConfig->csi_ReportConfigToAddModList->list.count; csi_report_id++){
    csi_ResourceConfigId=csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->resourcesForChannelMeasurement;
    UE_info->csi_report_template[UE_id][csi_report_id].reportQuantity_type = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->reportQuantity.present;

    for ( csi_resourceidx = 0; csi_resourceidx < csi_MeasConfig->csi_ResourceConfigToAddModList->list.count; csi_resourceidx++) {
      if ( csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[csi_resourceidx]->csi_ResourceConfigId != csi_ResourceConfigId)
	continue;
      else {
      //Finding the CSI_RS or SSB Resources
        UE_info->csi_report_template[UE_id][csi_report_id].CSI_Resource_type= csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[csi_resourceidx]->csi_RS_ResourceSetList.present;
        if (NR_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_nzp_CSI_RS_SSB ==UE_info->csi_report_template[UE_id][csi_report_id].CSI_Resource_type){
          struct NR_CSI_ResourceConfig__csi_RS_ResourceSetList__nzp_CSI_RS_SSB * nzp_CSI_RS_SSB = csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[csi_resourceidx]->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB;

          UE_info->csi_report_template[UE_id][csi_report_id].nb_of_nzp_csi_report = nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList!=NULL ? nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList->list.count:0;
          UE_info->csi_report_template[UE_id][csi_report_id].nb_of_csi_ssb_report = nzp_CSI_RS_SSB->csi_SSB_ResourceSetList!=NULL ? nzp_CSI_RS_SSB->csi_SSB_ResourceSetList->list.count:0;
        }

        if (0 != UE_info->csi_report_template[UE_id][csi_report_id].nb_of_csi_ssb_report){
	  uint8_t nb_ssb_resources =0;
          for ( csi_ssb_idx = 0; csi_ssb_idx < csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.count; csi_ssb_idx++) {
            if (csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[csi_ssb_idx]->csi_SSB_ResourceSetId ==
                *(csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[csi_resourceidx]->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList->list.array[0])) { 
              ///We can configure only one SSB resource set from spec 38.331 IE CSI-ResourceConfig
              if (NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled ==
                csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->groupBasedBeamReporting.present ) {
	        if (NULL != csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->groupBasedBeamReporting.choice.disabled->nrofReportedRS)
                  UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].nb_ssbri_cri = *(csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id]->groupBasedBeamReporting.choice.disabled->nrofReportedRS)+1;
                else
                  /*! From Spec 38.331
                  * nrofReportedRS
                  * The number (N) of measured RS resources to be reported per report setting in a non-group-based report. N <= N_max, where N_max is either 2 or 4 depending on UE
                  * capability. FFS: The signaling mechanism for the gNB to select a subset of N beams for the UE to measure and report.
                  * When the field is absent the UE applies the value 1
                  */
                  UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].nb_ssbri_cri= 1;
              } else
                UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].nb_ssbri_cri= 2;

              nb_ssb_resources=  csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[csi_ssb_idx]->csi_SSB_ResourceList.list.count;
              if (nb_ssb_resources){
                UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].cri_ssbri_bitlen =ceil(log2 (nb_ssb_resources));
                UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].rsrp_bitlen = 7; //From spec 38.212 Table 6.3.1.1.2-6: CRI, SSBRI, and RSRP 
                UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].diff_rsrp_bitlen =4; //From spec 38.212 Table 6.3.1.1.2-6: CRI, SSBRI, and RSRP
              }
              else{
                UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].cri_ssbri_bitlen =0;
                UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].rsrp_bitlen = 0;
                UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].diff_rsrp_bitlen =0;
              }

              LOG_I (MAC, "UCI: CSI_bit len : ssbri %d, rsrp: %d, diff_rsrp: %d\n",
                     UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].cri_ssbri_bitlen,
                     UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].rsrp_bitlen,
                     UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0].diff_rsrp_bitlen);
              break ;
            }
          }
        }
        if (0 != UE_info->csi_report_template[UE_id][csi_report_id].nb_of_nzp_csi_report)
          AssertFatal(1==0,"Currently configuring only SSB beamreporting.");
        break;
      }
    }
  }
}


uint16_t nr_get_csi_bitlen(const nr_csi_report_t *csi_report)
{
  const CRI_SSBRI_RSRP_bitlen_t *bitlen = &csi_report->CSI_report_bitlen[0];
  return bitlen->cri_ssbri_bitlen * bitlen->nb_ssbri_cri
         + bitlen->rsrp_bitlen
         + bitlen->diff_rsrp_bitlen * (bitlen->nb_ssbri_cri - 1) * csi_report->nb_of_csi_ssb_report;
}


void nr_csi_meas_reporting(int Mod_idP,
                           frame_t frame,
                           sub_frame_t slot)
{
  NR_ServingCellConfigCommon_t *scc =
      RC.nrmac[Mod_idP]->common_channels->ServingCellConfigCommon;
  const int n_slots_frame = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];

  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  NR_list_t *UE_list = &UE_info->list;
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    const NR_CellGroupConfig_t *secondaryCellGroup = UE_info->secondaryCellGroup[UE_id];
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    const NR_CSI_MeasConfig_t *csi_measconfig = secondaryCellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup;
    AssertFatal(csi_measconfig->csi_ReportConfigToAddModList->list.count > 0,
                "NO CSI report configuration available");
    NR_PUCCH_Config_t *pucch_Config = sched_ctrl->active_ubwp->bwp_Dedicated->pucch_Config->choice.setup;

    for (int csi_report_id = 0; csi_report_id < csi_measconfig->csi_ReportConfigToAddModList->list.count; csi_report_id++){
      const NR_CSI_ReportConfig_t *csirep = csi_measconfig->csi_ReportConfigToAddModList->list.array[csi_report_id];

      AssertFatal(csirep->reportConfigType.choice.periodic,
                  "Only periodic CSI reporting is implemented currently\n");
      int period, offset;
      csi_period_offset(csirep, &period, &offset);
      const int sched_slot = (period + offset) % n_slots_frame;
      // prepare to schedule csi measurement reception according to 5.2.1.4 in 38.214
      // preparation is done in first slot of tdd period
      if (frame % (period / n_slots_frame) != offset / n_slots_frame)
        continue;
      LOG_D(MAC, "CSI in frame %d slot %d\n", frame, sched_slot);

      const NR_PUCCH_CSI_Resource_t *pucchcsires = csirep->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list.array[0];
      const NR_PUCCH_ResourceSet_t *pucchresset = pucch_Config->resourceSetToAddModList->list.array[1]; // set with formats >1
      const int n = pucchresset->resourceList.list.count;
      int res_index = 0;
      for (; res_index < n; res_index++)
        if (*pucchresset->resourceList.list.array[res_index] == pucchcsires->pucch_Resource)
          break;
      AssertFatal(res_index < n,
                  "CSI resource not found among PUCCH resources\n");

      // find free PUCCH that is in order with possibly existing PUCCH
      // schedulings (other CSI, SR)
      NR_sched_pucch_t *curr_pucch = &sched_ctrl->sched_pucch[2];
      if(NFAPI_MODE == NFAPI_MODE_VNF)
        curr_pucch->csi_bits = 0;
      AssertFatal(curr_pucch->csi_bits == 0
                  && !curr_pucch->sr_flag
                  && curr_pucch->dai_c == 0,
                  "PUCCH not free at index 2 for UE %04x\n",
                  UE_info->rnti[UE_id]);
      curr_pucch->frame = frame;
      curr_pucch->ul_slot = sched_slot;
      curr_pucch->resource_indicator = res_index;
      curr_pucch->csi_bits +=
          nr_get_csi_bitlen(&UE_info->csi_report_template[UE_id][csi_report_id]);

      // going through the list of PUCCH resources to find the one indexed by resource_id
      uint16_t *vrb_map_UL = &RC.nrmac[Mod_idP]->common_channels[0].vrb_map_UL[sched_slot * MAX_BWP_SIZE];
      const int m = pucch_Config->resourceToAddModList->list.count;
      for (int j = 0; j < m; j++) {
        NR_PUCCH_Resource_t *pucchres = pucch_Config->resourceToAddModList->list.array[j];
        if (pucchres->pucch_ResourceId != *pucchresset->resourceList.list.array[res_index])
          continue;
        int start = pucchres->startingPRB;
        int len = 1;
        uint64_t mask = 0;
        switch(pucchres->format.present){
          case NR_PUCCH_Resource__format_PR_format2:
            len = pucchres->format.choice.format2->nrofPRBs;
            mask = ((1 << pucchres->format.choice.format2->nrofSymbols) - 1) << pucchres->format.choice.format2->startingSymbolIndex;
            curr_pucch->simultaneous_harqcsi = pucch_Config->format2->choice.setup->simultaneousHARQ_ACK_CSI;
            break;
          case NR_PUCCH_Resource__format_PR_format3:
            len = pucchres->format.choice.format3->nrofPRBs;
            mask = ((1 << pucchres->format.choice.format3->nrofSymbols) - 1) << pucchres->format.choice.format3->startingSymbolIndex;
            curr_pucch->simultaneous_harqcsi = pucch_Config->format3->choice.setup->simultaneousHARQ_ACK_CSI;
            break;
          case NR_PUCCH_Resource__format_PR_format4:
            mask = ((1 << pucchres->format.choice.format4->nrofSymbols) - 1) << pucchres->format.choice.format4->startingSymbolIndex;
            curr_pucch->simultaneous_harqcsi = pucch_Config->format4->choice.setup->simultaneousHARQ_ACK_CSI;
            break;
        default:
          AssertFatal(0, "Invalid PUCCH format type\n");
        }
        // verify resources are free
        for (int i = start; i < start + len; ++i) {
          vrb_map_UL[i] |= mask;
        }
        AssertFatal(!curr_pucch->simultaneous_harqcsi,
                    "UE %04x has simultaneous HARQ/CSI configured, but we don't support that\n",
                    UE_info->rnti[UE_id]);
      }
    }
  }
}

__attribute__((unused))
static void handle_dl_harq(module_id_t mod_id,
                           int UE_id,
                           int8_t harq_pid,
                           bool success)
{
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  NR_UE_harq_t *harq = &UE_info->UE_sched_ctrl[UE_id].harq_processes[harq_pid];
  harq->feedback_slot = -1;
  harq->is_waiting = false;
  if (success) {
    add_tail_nr_list(&UE_info->UE_sched_ctrl[UE_id].available_dl_harq, harq_pid);
    harq->round = 0;
    harq->ndi ^= 1;
  } else if (harq->round >= MAX_HARQ_ROUNDS -1) {
    add_tail_nr_list(&UE_info->UE_sched_ctrl[UE_id].available_dl_harq, harq_pid);
    harq->round = 0;
    harq->ndi ^= 1;
    NR_mac_stats_t *stats = &UE_info->mac_stats[UE_id];
    stats->dlsch_errors++;
    LOG_D(MAC, "retransmission error for UE %d (total %d)\n", UE_id, stats->dlsch_errors);
  } else {
    add_tail_nr_list(&UE_info->UE_sched_ctrl[UE_id].retrans_dl_harq, harq_pid);
    harq->round++;
  }
}

static NR_UE_harq_t *find_harq(module_id_t mod_id, frame_t frame, sub_frame_t slot, int UE_id)
{
  /* In case of realtime problems: we can only identify a HARQ process by
   * timing. If the HARQ process's feedback_frame/feedback_slot is not the one we
   * expected, we assume that processing has been aborted and we need to
   * skip this HARQ process, which is what happens in the loop below.
   * Similarly, we might be "in advance", in which case we need to skip
   * this result. */
  NR_UE_sched_ctrl_t *sched_ctrl = &RC.nrmac[mod_id]->UE_info.UE_sched_ctrl[UE_id];
  int8_t pid = sched_ctrl->feedback_dl_harq.head;
  if (pid < 0)
    return NULL;
  NR_UE_harq_t *harq = &sched_ctrl->harq_processes[pid];
  /* old feedbacks we missed: mark for retransmission */
  while (harq->feedback_slot < slot) {
    LOG_W(MAC,
          "expected HARQ pid %d feedback at slot %d, but is at %d.%d instead (HARQ feedback is in the past)\n",
          pid,
          harq->feedback_slot,
          frame,
          slot);
    remove_front_nr_list(&sched_ctrl->feedback_dl_harq);
    handle_dl_harq(mod_id, UE_id, pid, 0);
    pid = sched_ctrl->feedback_dl_harq.head;
    if (pid < 0)
      return NULL;
    harq = &sched_ctrl->harq_processes[pid];
  }
  /* feedbacks that we wait for in the future: don't do anything */
  if (harq->feedback_slot > slot) {
    LOG_W(MAC,
          "expected HARQ pid %d feedback at slot %d, but is at %d.%d instead (HARQ feedback is in the future)\n",
          pid,
          harq->feedback_slot,
          frame,
          slot);
    return NULL;
  }
  return harq;
}

void handle_nr_uci_pucch_0_1(module_id_t mod_id,
                             frame_t frame,
                             sub_frame_t slot,
                             const nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_01)
{
  int UE_id = find_nr_UE_id(mod_id, uci_01->rnti);
  if (UE_id < 0) {
    LOG_E(MAC, "%s(): unknown RNTI %04x in PUCCH UCI\n", __func__, uci_01->rnti);
    return;
  }
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

  if (((uci_01->pduBitmap >> 1) & 0x01)) {
    // iterate over received harq bits
    for (int harq_bit = 0; harq_bit < uci_01->harq->num_harq; harq_bit++) {
      const uint8_t harq_value = uci_01->harq->harq_list[harq_bit].harq_value;
      const uint8_t harq_confidence = uci_01->harq->harq_confidence_level;
      NR_UE_harq_t *harq = find_harq(mod_id, frame, slot, UE_id);
      if (!harq) {
        LOG_E(NR_MAC, "Oh no! Could not find a harq in %s!\n", __FUNCTION__);
        break;
      }
      DevAssert(harq->is_waiting);
      const int8_t pid = sched_ctrl->feedback_dl_harq.head;
      remove_front_nr_list(&sched_ctrl->feedback_dl_harq);
      /* Melissa: according to nfapi_nr_interface_scf.h, harq_value = 0 is a pass
      (check below was for harq_value == 1 in develop branch) */
      handle_dl_harq(mod_id, UE_id, pid, harq_value == 0 && harq_confidence == 0);
    }
  }
}

void handle_nr_uci_pucch_2_3_4(module_id_t mod_id,
                               frame_t frame,
                               sub_frame_t slot,
                               const nfapi_nr_uci_pucch_pdu_format_2_3_4_t *uci_234)
{ NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  UE_info->active[0] = 1;
  UE_info->rnti[0] = uci_234->rnti;
  int UE_id = find_nr_UE_id(mod_id, uci_234->rnti);
  if (UE_id < 0) {
    LOG_E(MAC, "%s(): unknown RNTI %04x in PUCCH UCI\n", __func__, uci_234->rnti);
    return;
  }
  //NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

  // tpc (power control)
  sched_ctrl->tpc1 = nr_get_tpc(RC.nrmac[mod_id]->pucch_target_snrx10,
                                uci_234->ul_cqi,
                                30);

  // NR_ServingCellConfigCommon_t *scc = RC.nrmac[mod_id]->common_channels->ServingCellConfigCommon;
  // const int num_slots = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
  //const int num_slots = 20;
  // if ((uci_234->pduBitmap >> 1) & 0x01) {
  //   // iterate over received harq bits
  //   for (int harq_bit = 0; harq_bit < uci_234->harq.harq_bit_len; harq_bit++) {
  //     const int acknack = ((uci_234->harq.harq_payload[harq_bit >> 3]) >> harq_bit) & 0x01;
  //     const int feedback_slot = (slot - 1 + num_slots) % num_slots;
  //     /* In case of realtime problems: we can only identify a HARQ process by
  //      * timing. If the HARQ process's feedback_slot is not the one we
  //      * expected, we assume that processing has been aborted and we need to
  //      * skip this HARQ process, which is what happens in the loop below. If
  //      * you don't experience real-time problems, you might simply revert the
  //      * commit that introduced these changes. */
  //     int8_t pid = sched_ctrl->feedback_dl_harq.head;
  //     DevAssert(pid >= 0);
  //     while (sched_ctrl->harq_processes[pid].feedback_slot != feedback_slot) {
  //       LOG_W(MAC,
  //             "expected feedback slot %d, but found %d instead\n",
  //             sched_ctrl->harq_processes[pid].feedback_slot,
  //             feedback_slot);
  //       remove_front_nr_list(&sched_ctrl->feedback_dl_harq);
  //       handle_dl_harq(mod_id, UE_id, pid, 0);
  //       pid = sched_ctrl->feedback_dl_harq.head;
  //       DevAssert(pid >= 0);
  //     }
  //     remove_front_nr_list(&sched_ctrl->feedback_dl_harq);
  //     NR_UE_harq_t *harq = &sched_ctrl->harq_processes[pid];
  //     DevAssert(harq->is_waiting);
  //     handle_dl_harq(mod_id, UE_id, pid, uci_234->harq.harq_crc != 1 && acknack);
  //   }
  // }
}


// function to update pucch scheduling parameters in UE list when a USS DL is scheduled
// this function returns an index to NR_sched_pucch structure
// currently this structure contains PUCCH0 at index 0 and PUCCH2 at index 1
// if the function returns -1 it was not possible to schedule acknack
// when current pucch is ready to be scheduled nr_fill_nfapi_pucch is called
int nr_acknack_scheduling(int mod_id,
                          int UE_id,
                          frame_t frame,
                          sub_frame_t slot)
{
  const NR_ServingCellConfigCommon_t *scc = RC.nrmac[mod_id]->common_channels->ServingCellConfigCommon;
  const int n_slots_frame = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
  const NR_TDD_UL_DL_Pattern_t *tdd = &scc->tdd_UL_DL_ConfigurationCommon->pattern1;
  const int nr_ulmix_slots = tdd->nrofUplinkSlots + (tdd->nrofUplinkSymbols != 0);
  const int nr_mix_slots = tdd->nrofDownlinkSymbols != 0 || tdd->nrofUplinkSymbols != 0;
  const int nr_slots_period = tdd->nrofDownlinkSlots + tdd->nrofUplinkSlots + nr_mix_slots;
  const int first_ul_slot_tdd = tdd->nrofDownlinkSlots + nr_slots_period * (slot / nr_slots_period);
  const int CC_id = 0;
  NR_sched_pucch_t *csi_pucch;

  AssertFatal(slot < first_ul_slot_tdd + (tdd->nrofUplinkSymbols != 0),
              "cannot handle multiple TDD periods (yet): slot %d first_ul_slot_tdd %d nrofUplinkSlots %ld\n",
              slot,
              first_ul_slot_tdd,
              tdd->nrofUplinkSlots);

  /* for the moment, we consider:
   * * only pucch_sched[0] holds HARQ (and SR)
   * * we do not multiplex with CSI, which is always in pucch_sched[2]
   * * SR uses format 0 and is allocated in the first UL (mixed) slot (and not
   *   later)
   * * each UE has dedicated PUCCH Format 0 resources, and we use index 0! */
  NR_UE_sched_ctrl_t *sched_ctrl = &RC.nrmac[mod_id]->UE_info.UE_sched_ctrl[UE_id];
  NR_sched_pucch_t *pucch = &sched_ctrl->sched_pucch[0];
  AssertFatal(pucch->csi_bits == 0,
              "%s(): csi_bits %d in sched_pucch[0]\n",
              __func__,
              pucch->csi_bits);
  /* if the currently allocated PUCCH of this UE is full, allocate it */
  if (pucch->dai_c == 2) {
    /* advance the UL slot information in PUCCH by one so we won't schedule in
     * the same slot again */
    const int f = pucch->frame;
    const int s = pucch->ul_slot;
    nr_fill_nfapi_pucch(mod_id, frame, slot, pucch, UE_id);
    memset(pucch, 0, sizeof(*pucch));
    pucch->frame = s == n_slots_frame - 1 ? (f + 1) % 1024 : f;
    pucch->ul_slot = (s + 1) % n_slots_frame;
    // we assume that only two indices over the array sched_pucch exist
    csi_pucch = &sched_ctrl->sched_pucch[1];
    // skip the CSI PUCCH if it is present and if in the next frame/slot
    // and if we don't multiplex
    if (csi_pucch->csi_bits > 0
        && csi_pucch->frame == pucch->frame
        && csi_pucch->ul_slot == pucch->ul_slot
        && !csi_pucch->simultaneous_harqcsi) {
      nr_fill_nfapi_pucch(mod_id, frame, slot, csi_pucch, UE_id);
      memset(csi_pucch, 0, sizeof(*csi_pucch));
      pucch->frame = s >= n_slots_frame - 2 ?  (f + 1) % 1024 : f;
      pucch->ul_slot = (s + 2) % n_slots_frame;
    }
  }

  /* if the UE's next PUCCH occasion is after the possible UL slots (within the
   * same frame) or wrapped around to the next frame, then we assume there is
   * no possible PUCCH allocation anymore */
  if ((pucch->frame == frame
       && (pucch->ul_slot >= first_ul_slot_tdd + nr_ulmix_slots))
      || (pucch->frame == frame + 1))
    return -1;

  // this is hardcoded for now as ue specific only if we are not on the initialBWP (to be fixed to allow ue_Specific also on initialBWP
  NR_SearchSpace__searchSpaceType_PR ss_type = sched_ctrl->active_bwp ? NR_SearchSpace__searchSpaceType_PR_ue_Specific: NR_SearchSpace__searchSpaceType_PR_common;
  uint8_t pdsch_to_harq_feedback[8];
  get_pdsch_to_harq_feedback(mod_id, UE_id, ss_type, pdsch_to_harq_feedback);

  /* there is a HARQ. Check whether we can use it for this ACKNACK */
  //TODO:: The 2nd condition should be removed.
  if (pucch->dai_c > 0 && pucch->frame == frame) {
    /* this UE already has a PUCCH occasion */
    DevAssert(pucch->frame == frame);

    // Find the right timing_indicator value.
    int i = 0;
    while (i < 8) {
      if (pdsch_to_harq_feedback[i] == pucch->ul_slot - slot)
        break;
      ++i;
    }
    if (i >= 8) {
      // we cannot reach this timing anymore, allocate and try again
      const int f = pucch->frame;
      const int s = pucch->ul_slot;
      const int n_slots_frame = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
      nr_fill_nfapi_pucch(mod_id, frame, slot, pucch, UE_id);
      memset(pucch, 0, sizeof(*pucch));
      pucch->frame = s == n_slots_frame - 1 ? (f + 1) % 1024 : f;
      pucch->ul_slot = (s + 1) % n_slots_frame;
      return nr_acknack_scheduling(mod_id, UE_id, frame, slot);
    }

    pucch->timing_indicator = i;
    pucch->dai_c++;
    // retain old resource indicator, and we are good
    return 0;
  }

  /* we need to find a new PUCCH occasion */

  /* if time information is outdated (e.g., last PUCCH occasion in last frame),
   * set to first possible UL occasion in this frame. Note that if such UE is
   * scheduled a lot and used all AckNacks, pucch->frame might have been
   * wrapped around to next frame */
  if (frame != pucch->frame || pucch->ul_slot < first_ul_slot_tdd) {
    pucch->dai_c = 0;
    AssertFatal(pucch->sr_flag + pucch->dai_c == 0,
                "expected no SR/AckNack for UE %d in %4d.%2d, but has %d/%d for %4d.%2d\n",
                UE_id, frame, slot, pucch->sr_flag, pucch->dai_c, pucch->frame, pucch->ul_slot);
    AssertFatal(frame + 1 != pucch->frame,
                "frame wrap around not handled in %s() yet\n",
                __func__);
    pucch->frame = frame;
    pucch->ul_slot = first_ul_slot_tdd;
  }

  // advance ul_slot if it is not reachable by UE
  pucch->ul_slot = max(pucch->ul_slot, slot + pdsch_to_harq_feedback[0]);

  // Find the right timing_indicator value.
  int i = 0;
  while (i < 8) {
    if (pdsch_to_harq_feedback[i] == pucch->ul_slot - slot)
      break;
    ++i;
  }
  if (i >= 8) {
    LOG_W(MAC,
          "%4d.%2d could not find pdsch_to_harq_feedback for UE %d: earliest "
          "ack slot %d\n",
          frame,
          slot,
          UE_id,
          pucch->ul_slot);
    return -1;
  }

  // is there already CSI in this slot?
  csi_pucch = &sched_ctrl->sched_pucch[1];
  if (csi_pucch->csi_bits > 0
      && csi_pucch->frame == pucch->frame
      && csi_pucch->ul_slot == pucch->ul_slot) {
    // skip the CSI PUCCH if it is present and if in the next frame/slot
    // and if we don't multiplex
    // FIXME currently we support at most 11 bits in pucch2 so skip also in that case
    if(!csi_pucch->simultaneous_harqcsi
       || ((csi_pucch->csi_bits + csi_pucch->dai_c) >= 11)) {
      nr_fill_nfapi_pucch(mod_id, frame, slot, csi_pucch, UE_id);
      memset(csi_pucch, 0, sizeof(*csi_pucch));
      /* advance the UL slot information in PUCCH by one so we won't schedule in
       * the same slot again */
      const int f = pucch->frame;
      const int s = pucch->ul_slot;
      memset(pucch, 0, sizeof(*pucch));
      pucch->frame = s == n_slots_frame - 1 ? (f + 1) % 1024 : f;
      pucch->ul_slot = (s + 1) % n_slots_frame;
      return nr_acknack_scheduling(mod_id, UE_id, frame, slot);
    }
    // multiplexing harq and csi in a pucch
    else {
      csi_pucch->timing_indicator = i;
      csi_pucch->dai_c++;
      return 1;
    }
  }

  pucch->timing_indicator = i; // index in the list of timing indicators

  LOG_I(NR_MAC,"2. DL slot %d, UL_ACK %d (index %d)\n", slot, pucch->ul_slot, i);

  pucch->dai_c++;
  pucch->resource_indicator = 0; // each UE has dedicated PUCCH resources

  NR_PUCCH_Config_t *pucch_Config = NULL;
  if (sched_ctrl->active_ubwp) {
    pucch_Config = sched_ctrl->active_ubwp->bwp_Dedicated->pucch_Config->choice.setup;
  } else if (RC.nrmac[mod_id]->UE_info.secondaryCellGroup[UE_id] &&
             RC.nrmac[mod_id]->UE_info.secondaryCellGroup[UE_id]->spCellConfig &&
             RC.nrmac[mod_id]->UE_info.secondaryCellGroup[UE_id]->spCellConfig->spCellConfigDedicated &&
             RC.nrmac[mod_id]->UE_info.secondaryCellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig &&
             RC.nrmac[mod_id]->UE_info.secondaryCellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
             RC.nrmac[mod_id]->UE_info.secondaryCellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup) {
    pucch_Config = RC.nrmac[mod_id]->UE_info.secondaryCellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup;
  }

  /* verify that at that slot and symbol, resources are free. We only do this
   * for initialCyclicShift 0 (we assume it always has that one), so other
   * initialCyclicShifts can overlap with ICS 0!*/
  const NR_PUCCH_Resource_t *resource = pucch_Config->resourceToAddModList->list.array[pucch->resource_indicator];
  DevAssert(resource->format.present == NR_PUCCH_Resource__format_PR_format0);
  if (resource->format.choice.format0->initialCyclicShift == 0) {
    uint16_t *vrb_map_UL = &RC.nrmac[mod_id]->common_channels[CC_id].vrb_map_UL[pucch->ul_slot * MAX_BWP_SIZE];
    const uint16_t symb = 1 << resource->format.choice.format0->startingSymbolIndex;
    if ((vrb_map_UL[resource->startingPRB] & symb) != 0)
      LOG_W(MAC, "symbol 0x%x is not free for PUCCH alloc in vrb_map_UL at RB %ld and slot %d.%d\n", symb, resource->startingPRB, pucch->frame, pucch->ul_slot);
    vrb_map_UL[resource->startingPRB] |= symb;
  }
  return 0;
}


void csi_period_offset(const NR_CSI_ReportConfig_t *csirep,
                       int *period, int *offset) {

    NR_CSI_ReportPeriodicityAndOffset_PR p_and_o = csirep->reportConfigType.choice.periodic->reportSlotConfig.present;

    switch(p_and_o){
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots4:
        *period = 4;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots4;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots5:
        *period = 5;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots5;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots8:
        *period = 8;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots8;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots10:
        *period = 10;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots10;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots16:
        *period = 16;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots16;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots20:
        *period = 20;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots20;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots40:
        *period = 40;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots40;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots80:
        *period = 80;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots80;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots160:
        *period = 160;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots160;
        break;
      case NR_CSI_ReportPeriodicityAndOffset_PR_slots320:
        *period = 320;
        *offset = csirep->reportConfigType.choice.periodic->reportSlotConfig.choice.slots320;
        break;
    default:
      AssertFatal(1==0,"No periodicity and offset resource found in CSI report");
    }
}

uint16_t compute_pucch_prb_size(uint8_t format,
                                uint8_t nr_prbs,
                                uint16_t O_tot,
                                uint16_t O_csi,
                                NR_PUCCH_MaxCodeRate_t *maxCodeRate,
                                uint8_t Qm,
                                uint8_t n_symb,
                                uint8_t n_re_ctrl) {

  uint16_t O_crc;

  if (O_tot<12)
    O_crc = 0;
  else{
    if (O_tot<20)
      O_crc = 6;
    else {
      if (O_tot<360)
        O_crc = 11;
      else
        AssertFatal(1==0,"Case for segmented PUCCH not yet implemented");
    }
  }

  int rtimes100;
  switch(*maxCodeRate){
    case NR_PUCCH_MaxCodeRate_zeroDot08 :
      rtimes100 = 8;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot15 :
      rtimes100 = 15;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot25 :
      rtimes100 = 25;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot35 :
      rtimes100 = 35;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot45 :
      rtimes100 = 45;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot60 :
      rtimes100 = 60;
      break;
    case NR_PUCCH_MaxCodeRate_zeroDot80 :
      rtimes100 = 80;
      break;
  default :
    AssertFatal(1==0,"Invalid MaxCodeRate");
  }

  float r = (float)rtimes100/100;

  if (O_csi == O_tot) {
    if ((O_tot+O_csi)>(nr_prbs*n_re_ctrl*n_symb*Qm*r))
      AssertFatal(1==0,"MaxCodeRate %.2f can't support %d UCI bits and %d CRC bits with %d PRBs",
                  r,O_tot,O_crc,nr_prbs);
    else
      return nr_prbs;
  }

  if (format==2){
    // TODO fix this for multiple CSI reports
    for (int i=1; i<=nr_prbs; i++){
      if((O_tot+O_crc)<=(i*n_symb*Qm*n_re_ctrl*r) &&
         (O_tot+O_crc)>((i-1)*n_symb*Qm*n_re_ctrl*r))
        return i;
    }
    AssertFatal(1==0,"MaxCodeRate %.2f can't support %d UCI bits and %d CRC bits with at most %d PRBs",
                r,O_tot,O_crc,nr_prbs);
  }
  else{
    AssertFatal(1==0,"Not yet implemented");
  }

}
