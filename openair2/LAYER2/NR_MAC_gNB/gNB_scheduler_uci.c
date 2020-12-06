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

extern RAN_CONTEXT_t RC;

void nr_schedule_pucch(int Mod_idP,
                       int UE_id,
                       int nr_ulmix_slots,
                       frame_t frameP,
                       sub_frame_t slotP) {
  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  AssertFatal(UE_info->active[UE_id],"Cannot find UE_id %d is not active\n",UE_id);

  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
  const int n = sizeof(sched_ctrl->sched_pucch) / sizeof(*sched_ctrl->sched_pucch);
  for (int i = 0; i < n; i++) {
    NR_sched_pucch_t *curr_pucch = &sched_ctrl->sched_pucch[i];
    const uint16_t O_ack = curr_pucch->dai_c;
    const uint16_t O_csi = curr_pucch->csi_bits;
    const uint8_t O_sr = 0; // no SR in PUCCH implemented for now
    if (O_ack + O_csi + O_sr == 0
        || frameP != curr_pucch->frame
        || slotP != curr_pucch->ul_slot)
      continue;

    nfapi_nr_ul_tti_request_t *future_ul_tti_req =
        &RC.nrmac[Mod_idP]->UL_tti_req_ahead[0][curr_pucch->ul_slot];
    AssertFatal(future_ul_tti_req->SFN == curr_pucch->frame
                && future_ul_tti_req->Slot == curr_pucch->ul_slot,
                "future UL_tti_req's frame.slot %d.%d does not match PUCCH %d.%d\n",
                future_ul_tti_req->SFN,
                future_ul_tti_req->Slot,
                curr_pucch->frame,
                curr_pucch->ul_slot);
    future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE;
    future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pucch_pdu_t);
    nfapi_nr_pucch_pdu_t *pucch_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pucch_pdu;
    memset(pucch_pdu, 0, sizeof(nfapi_nr_pucch_pdu_t));
    future_ul_tti_req->n_pdus += 1;

    LOG_I(MAC,
          "%4d.%2d Scheduling pucch reception in %4d.%2d: bits SR %d, ACK %d, CSI %d\n",
          frameP,
          slotP,
          curr_pucch->frame,
          curr_pucch->ul_slot,
          O_sr,
          O_ack,
          O_csi);

    NR_ServingCellConfigCommon_t *scc = RC.nrmac[Mod_idP]->common_channels->ServingCellConfigCommon;
    nr_configure_pucch(pucch_pdu,
                       scc,
                       UE_info->UE_sched_ctrl[UE_id].active_ubwp,
                       UE_info->rnti[UE_id],
                       curr_pucch->resource_indicator,
                       O_csi,
                       O_ack,
                       O_sr);

    memset(curr_pucch, 0, sizeof(*curr_pucch));
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


uint16_t nr_get_csi_bitlen(int Mod_idP,
                           int UE_id,
                           uint8_t csi_report_id) {

  uint16_t csi_bitlen =0;
  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  CRI_SSBRI_RSRP_bitlen_t * CSI_report_bitlen = NULL;

  CSI_report_bitlen = &(UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen[0]);
  csi_bitlen = ((CSI_report_bitlen->cri_ssbri_bitlen * CSI_report_bitlen->nb_ssbri_cri) +
               CSI_report_bitlen->rsrp_bitlen +(CSI_report_bitlen->diff_rsrp_bitlen *
               (CSI_report_bitlen->nb_ssbri_cri -1 )) *UE_info->csi_report_template[UE_id][csi_report_id].nb_of_csi_ssb_report);

  return csi_bitlen;
}


void nr_csi_meas_reporting(int Mod_idP,
                           int UE_id,
                           frame_t frame,
                           sub_frame_t slot,
                           int slots_per_tdd,
                           int ul_slots,
                           int n_slots_frame) {

  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  NR_sched_pucch_t *curr_pucch;
  NR_PUCCH_ResourceSet_t *pucchresset;
  NR_CSI_ReportConfig_t *csirep;
  NR_CellGroupConfig_t *secondaryCellGroup = UE_info->secondaryCellGroup[UE_id];
  NR_CSI_MeasConfig_t *csi_measconfig = secondaryCellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup;
  NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[0];
  NR_PUCCH_Config_t *pucch_Config = ubwp->bwp_Dedicated->pucch_Config->choice.setup;

  AssertFatal(csi_measconfig->csi_ReportConfigToAddModList->list.count>0,"NO CSI report configuration available");

  for (int csi_report_id = 0; csi_report_id < csi_measconfig->csi_ReportConfigToAddModList->list.count; csi_report_id++){

    csirep = csi_measconfig->csi_ReportConfigToAddModList->list.array[csi_report_id];

    AssertFatal(csirep->reportConfigType.choice.periodic!=NULL,"Only periodic CSI reporting is implemented currently");
    int period, offset, sched_slot;
    csi_period_offset(csirep,&period,&offset);
    sched_slot = (period+offset)%n_slots_frame;
    // prepare to schedule csi measurement reception according to 5.2.1.4 in 38.214
    // preparation is done in first slot of tdd period
    if ( (frame%(period/n_slots_frame)==(offset/n_slots_frame)) && (slot==((sched_slot/slots_per_tdd)*slots_per_tdd))) {

      // we are scheduling pucch for csi in the first pucch occasion (this comes before ack/nack)
      // FIXME: for the moment, we statically put it into the second sched_pucch!
      curr_pucch = &UE_info->UE_sched_ctrl[UE_id].sched_pucch[1];

      NR_PUCCH_CSI_Resource_t *pucchcsires = csirep->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list.array[0];

      int found = -1;
      pucchresset = pucch_Config->resourceSetToAddModList->list.array[1]; // set with formats >1
      int n_list = pucchresset->resourceList.list.count;
      for (int i=0; i<n_list; i++) {
        if (*pucchresset->resourceList.list.array[i] == pucchcsires->pucch_Resource)
          found = i;
      }
      AssertFatal(found>-1,"CSI resource not found among PUCCH resources");

      curr_pucch->resource_indicator = found;

      n_list = pucch_Config->resourceToAddModList->list.count;

      // going through the list of PUCCH resources to find the one indexed by resource_id
      for (int i=0; i<n_list; i++) {
        NR_PUCCH_Resource_t *pucchres = pucch_Config->resourceToAddModList->list.array[i];
        if (pucchres->pucch_ResourceId == *pucchresset->resourceList.list.array[found]) {
          switch(pucchres->format.present){
            case NR_PUCCH_Resource__format_PR_format2:
              if (pucch_Config->format2->choice.setup->simultaneousHARQ_ACK_CSI == NULL)
                curr_pucch->simultaneous_harqcsi = false;
              else
                curr_pucch->simultaneous_harqcsi = true;
              break;
            case NR_PUCCH_Resource__format_PR_format3:
              if (pucch_Config->format3->choice.setup->simultaneousHARQ_ACK_CSI == NULL)
                curr_pucch->simultaneous_harqcsi = false;
              else
                curr_pucch->simultaneous_harqcsi = true;
              break;
            case NR_PUCCH_Resource__format_PR_format4:
              if (pucch_Config->format4->choice.setup->simultaneousHARQ_ACK_CSI == NULL)
                curr_pucch->simultaneous_harqcsi = false;
              else
                curr_pucch->simultaneous_harqcsi = true;
              break;
          default:
            AssertFatal(1==0,"Invalid PUCCH format type");
          }
        }
      }
      curr_pucch->csi_bits += nr_get_csi_bitlen(Mod_idP,UE_id,csi_report_id); // TODO function to compute CSI meas report bit size
      curr_pucch->frame = frame;
      curr_pucch->ul_slot = sched_slot;
    }
  }
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

  // tpc (power control)
  sched_ctrl->tpc1 = nr_get_tpc(RC.nrmac[mod_id]->pucch_target_snrx10,
                                uci_01->ul_cqi,
                                30);

  // TODO
  int max_harq_rounds = 4; // TODO define macro
  if (((uci_01->pduBitmap >> 1) & 0x01)) {
    // handle harq
    int harq_idx_s = 0;

    // iterate over received harq bits
    for (int harq_bit = 0; harq_bit < uci_01->harq->num_harq; harq_bit++) {
      // search for the right harq process
      for (int harq_idx = harq_idx_s; harq_idx < NR_MAX_NB_HARQ_PROCESSES; harq_idx++) {
        // if the gNB received ack with a good confidence
        if ((slot - 1) == sched_ctrl->harq_processes[harq_idx].feedback_slot) {
          sched_ctrl->harq_processes[harq_idx].feedback_slot = -1;
          if ((uci_01->harq->harq_list[harq_bit].harq_value == 1) &&
              (uci_01->harq->harq_confidence_level == 0)) {
            // toggle NDI and reset round
            sched_ctrl->harq_processes[harq_idx].ndi ^= 1;
            sched_ctrl->harq_processes[harq_idx].round = 0;
          }
          else
            sched_ctrl->harq_processes[harq_idx].round++;
          sched_ctrl->harq_processes[harq_idx].is_waiting = 0;
          harq_idx_s = harq_idx + 1;
          // if the max harq rounds was reached
          if (sched_ctrl->harq_processes[harq_idx].round == max_harq_rounds) {
            sched_ctrl->harq_processes[harq_idx].ndi ^= 1;
            sched_ctrl->harq_processes[harq_idx].round = 0;
            UE_info->mac_stats[UE_id].dlsch_errors++;
          }
          break;
        }
        // if feedback slot processing is aborted
        else if (sched_ctrl->harq_processes[harq_idx].feedback_slot != -1
                 && (slot - 1) > sched_ctrl->harq_processes[harq_idx].feedback_slot
                 && sched_ctrl->harq_processes[harq_idx].is_waiting) {
          sched_ctrl->harq_processes[harq_idx].feedback_slot = -1;
          sched_ctrl->harq_processes[harq_idx].round++;
          if (sched_ctrl->harq_processes[harq_idx].round == max_harq_rounds) {
            sched_ctrl->harq_processes[harq_idx].ndi ^= 1;
            sched_ctrl->harq_processes[harq_idx].round = 0;
          }
          sched_ctrl->harq_processes[harq_idx].is_waiting = 0;
        }
      }
    }
  }
}

void handle_nr_uci_pucch_2_3_4(module_id_t mod_id,
                               frame_t frame,
                               sub_frame_t slot,
                               const nfapi_nr_uci_pucch_pdu_format_2_3_4_t *uci_234)
{
  int UE_id = find_nr_UE_id(mod_id, uci_234->rnti);
  if (UE_id < 0) {
    LOG_E(MAC, "%s(): unknown RNTI %04x in PUCCH UCI\n", __func__, uci_234->rnti);
    return;
  }
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

  // tpc (power control)
  sched_ctrl->tpc1 = nr_get_tpc(RC.nrmac[mod_id]->pucch_target_snrx10,
                                uci_234->ul_cqi,
                                30);

  // TODO
  int max_harq_rounds = 4; // TODO define macro
  if ((uci_234->pduBitmap >> 1) & 0x01) {
    int harq_idx_s = 0;
    int acknack;

    // iterate over received harq bits
    for (int harq_bit = 0; harq_bit < uci_234->harq.harq_bit_len; harq_bit++) {
      acknack = ((uci_234->harq.harq_payload[harq_bit>>3])>>harq_bit)&0x01;
      for (int harq_idx = harq_idx_s; harq_idx < NR_MAX_NB_HARQ_PROCESSES-1; harq_idx++) {
        // if the gNB received ack with a good confidence or if the max harq rounds was reached
        if ((slot - 1) == sched_ctrl->harq_processes[harq_idx].feedback_slot) {
          // TODO add some confidence level for when there is no CRC
          sched_ctrl->harq_processes[harq_idx].feedback_slot = -1;
          if ((uci_234->harq.harq_crc != 1) && acknack) {
            // toggle NDI and reset round
            sched_ctrl->harq_processes[harq_idx].ndi ^= 1;
            sched_ctrl->harq_processes[harq_idx].round = 0;
          }
          else
            sched_ctrl->harq_processes[harq_idx].round++;
          sched_ctrl->harq_processes[harq_idx].is_waiting = 0;
          harq_idx_s = harq_idx + 1;
          // if the max harq rounds was reached
          if (sched_ctrl->harq_processes[harq_idx].round == max_harq_rounds) {
            sched_ctrl->harq_processes[harq_idx].ndi ^= 1;
            sched_ctrl->harq_processes[harq_idx].round = 0;
            UE_info->mac_stats[UE_id].dlsch_errors++;
          }
          break;
        }
        // if feedback slot processing is aborted
        else if (sched_ctrl->harq_processes[harq_idx].feedback_slot != -1
                 && (slot - 1) > sched_ctrl->harq_processes[harq_idx].feedback_slot
                 && sched_ctrl->harq_processes[harq_idx].is_waiting) {
          sched_ctrl->harq_processes[harq_idx].feedback_slot = -1;
          sched_ctrl->harq_processes[harq_idx].round++;
          if (sched_ctrl->harq_processes[harq_idx].round == max_harq_rounds) {
            sched_ctrl->harq_processes[harq_idx].ndi ^= 1;
            sched_ctrl->harq_processes[harq_idx].round = 0;
          }
          sched_ctrl->harq_processes[harq_idx].is_waiting = 0;
        }
      }
    }
  }
}


// function to update pucch scheduling parameters in UE list when a USS DL is scheduled
bool nr_acknack_scheduling(int mod_id,
                           int UE_id,
                           frame_t frame,
                           sub_frame_t slot)
{
  /* FIXME: for the moment, we consider that
   * * only pucch_sched[0] holds HARQ
   * * a UE is not scheduled in more than two slots, and their ACKs come in the same slot!
   * * we do not multiplex with CSI
   * * we do not mux two UEs in the same PUCCH slot (on the two symbols)
   * * we only use the first TDD period (5/10ms) */
  NR_UE_sched_ctrl_t *sched_ctrl = &RC.nrmac[mod_id]->UE_info.UE_sched_ctrl[UE_id];
  NR_sched_pucch_t *curr_pucch = &sched_ctrl->sched_pucch[0];
  AssertFatal(curr_pucch->csi_bits == 0,
              "%s(): csi_bits %d in sched_pucch[0]\n",
              __func__,
              curr_pucch->csi_bits);

  const int max_acknacks = 2;
  AssertFatal(curr_pucch->dai_c <= max_acknacks,
              "%s() called but already %d dai_c in sched_pucch[0]\n",
              __func__,
              curr_pucch->dai_c);

  const NR_ServingCellConfigCommon_t *scc = RC.nrmac[mod_id]->common_channels->ServingCellConfigCommon;
  const NR_TDD_UL_DL_Pattern_t *tdd_pattern = &scc->tdd_UL_DL_ConfigurationCommon->pattern1;
  //const int nr_ulmix_slots = tdd_pattern->nrofUplinkSlots + (tdd_pattern->nrofUplinkSymbols != 0);
  const int first_ul_slot_tdd = tdd_pattern->nrofDownlinkSlots;
  const int CC_id = 0;

  // this is hardcoded for now as ue specific
  NR_SearchSpace__searchSpaceType_PR ss_type = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
  uint8_t pdsch_to_harq_feedback[8];
  get_pdsch_to_harq_feedback(mod_id, UE_id, ss_type, pdsch_to_harq_feedback);

  NR_PUCCH_Config_t *pucch_Config = sched_ctrl->active_ubwp->bwp_Dedicated->pucch_Config->choice.setup;
  DevAssert(pucch_Config->resourceToAddModList->list.count > 0);

  curr_pucch->frame = frame;
  curr_pucch->dai_c++;

  if (curr_pucch->dai_c == 1) {
    /* FIXME for first allocation: find free resource, here assume first PUCCH
     * resource and first_ul_slot_tdd */
    const int pucch_res = 0;
    curr_pucch->resource_indicator = pucch_res;
    curr_pucch->ul_slot = first_ul_slot_tdd;

    /* verify that at that slot and symbol, resources are free.
     * Note: this does not handle potential mux of PUCCH in the same symbol! */
    const NR_PUCCH_Resource_t *resource =
        pucch_Config->resourceToAddModList->list.array[pucch_res];
    DevAssert(resource->format.present == NR_PUCCH_Resource__format_PR_format0);
    uint16_t *vrb_map_UL =
        &RC.nrmac[mod_id]->common_channels[CC_id].vrb_map_UL[first_ul_slot_tdd * 275];
    const uint16_t symb = 1 << resource->format.choice.format0->startingSymbolIndex;
    AssertFatal((vrb_map_UL[resource->startingPRB] & symb) == 0,
                "symbol %x is not free for PUCCH alloc in vrb_map_UL at RB %ld and slot %d\n",
                symb, resource->startingPRB, first_ul_slot_tdd);
    vrb_map_UL[resource->startingPRB] |= symb;
  }

  /* Find the right timing_indicator value. FIXME: if previously ul_slot is not
   * possible (anymore), we need to allocate previous HARQ feedback (since we
   * cannot "reach" it anymore) and search a new one! */
  int i = 0;
  while (i < 8) {
    if (pdsch_to_harq_feedback[i] == curr_pucch->ul_slot - slot)
      break;
    ++i;
  }
  AssertFatal(i < 8,
              "could not find pdsch_to_harq_feedback: slot %d, ack slot %d\n",
              slot, first_ul_slot_tdd);
  curr_pucch->timing_indicator = i; // index in the list of timing indicators
  return true;
}


void csi_period_offset(NR_CSI_ReportConfig_t *csirep,
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
