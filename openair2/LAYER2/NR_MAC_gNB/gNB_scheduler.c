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

/*! \file gNB_scheduler.c
 * \brief gNB scheduler top level function operates on per subframe basis
 * \author  Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
 * \version 0.5
 * \company Eurecom, NTUST
 * @ingroup _mac

 */

#include "assertions.h"

#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_gNB/mac_proto.h"

#include "common/utils/LOG/log.h"
#include "common/utils/nr/nr_common.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"

#include "openair2/X2AP/x2ap_eNB.h"

#include "nr_pdcp/nr_pdcp_oai_api.h"

#include "intertask_interface.h"

#include "executables/softmodem-common.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "executables/nr-softmodem.h"

#include <errno.h>
#include <string.h>

const uint8_t nr_rv_round_map[4] = {0, 2, 3, 1};

void clear_nr_nfapi_information(gNB_MAC_INST *gNB,
                                int CC_idP,
                                frame_t frameP,
                                sub_frame_t slotP,
                                nfapi_nr_dl_tti_request_t *DL_req,
                                nfapi_nr_tx_data_request_t *TX_req,
                                nfapi_nr_ul_dci_request_t *UL_dci_req)
{
  /* called below and in simulators, so we assume a lock but don't require it */

  NR_ServingCellConfigCommon_t *scc = gNB->common_channels->ServingCellConfigCommon;
  const int num_slots = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];

  UL_tti_req_ahead_initialization(gNB, scc, num_slots, CC_idP, frameP, slotP, *scc->ssbSubcarrierSpacing);

  nfapi_nr_dl_tti_pdcch_pdu_rel15_t **pdcch = (nfapi_nr_dl_tti_pdcch_pdu_rel15_t **)gNB->pdcch_pdu_idx[CC_idP];

  gNB->pdu_index[CC_idP] = 0;

  DL_req[CC_idP].SFN = frameP;
  DL_req[CC_idP].Slot = slotP;
  DL_req[CC_idP].dl_tti_request_body.nPDUs             = 0;
  DL_req[CC_idP].dl_tti_request_body.nGroup = 0;
  memset(pdcch, 0, sizeof(*pdcch) * MAX_NUM_CORESET);

  UL_dci_req[CC_idP].SFN = frameP;
  UL_dci_req[CC_idP].Slot = slotP;
  UL_dci_req[CC_idP].numPdus = 0;

  /* advance last round's future UL_tti_req to be ahead of current frame/slot */
  const int size = gNB->UL_tti_req_ahead_size;
  const int prev_slot = frameP * num_slots + slotP + size - 1;
  nfapi_nr_ul_tti_request_t *future_ul_tti_req = &gNB->UL_tti_req_ahead[CC_idP][prev_slot % size];
  future_ul_tti_req->SFN = (prev_slot / num_slots) % 1024;
  LOG_D(NR_MAC, "%d.%d UL_tti_req_ahead SFN.slot = %d.%d for index %d \n", frameP, slotP, future_ul_tti_req->SFN, future_ul_tti_req->Slot, prev_slot % size);
  /* future_ul_tti_req->Slot is fixed! */
  future_ul_tti_req->n_pdus = 0;
  future_ul_tti_req->n_ulsch = 0;
  future_ul_tti_req->n_ulcch = 0;
  future_ul_tti_req->n_group = 0;

  TX_req[CC_idP].Number_of_PDUs = 0;
}

bool is_xlsch_in_slot(uint64_t bitmap, sub_frame_t slot) {
  return (bitmap >> (slot % 64)) & 0x01;
}

/* the structure nfapi_nr_ul_tti_request_t is very big, let's copy only what is necessary */
static void copy_ul_tti_req(nfapi_nr_ul_tti_request_t *to, nfapi_nr_ul_tti_request_t *from)
{
  int i;

  to->header = from->header;
  to->SFN = from->SFN;
  to->Slot = from->Slot;
  to->n_pdus = from->n_pdus;
  to->rach_present = from->rach_present;
  to->n_ulsch = from->n_ulsch;
  to->n_ulcch = from->n_ulcch;
  to->n_group = from->n_group;

  for (i = 0; i < from->n_pdus; i++) {
    to->pdus_list[i].pdu_type = from->pdus_list[i].pdu_type;
    to->pdus_list[i].pdu_size = from->pdus_list[i].pdu_size;

    switch (from->pdus_list[i].pdu_type) {
      case NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE:
        to->pdus_list[i].prach_pdu = from->pdus_list[i].prach_pdu;
        break;
      case NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE:
        to->pdus_list[i].pusch_pdu = from->pdus_list[i].pusch_pdu;
        break;
      case NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE:
        to->pdus_list[i].pucch_pdu = from->pdus_list[i].pucch_pdu;
        break;
      case NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE:
        to->pdus_list[i].srs_pdu = from->pdus_list[i].srs_pdu;
        break;
    }
  }

  for (i = 0; i < from->n_group; i++)
    to->groups_list[i] = from->groups_list[i];
}

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP, frame_t frame, sub_frame_t slot, NR_Sched_Rsp_t *sched_info)
{
  protocol_ctxt_t ctxt = {0};
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES, NOT_A_RNTI, frame, slot,module_idP);

  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = gNB->common_channels;
  NR_ServingCellConfigCommon_t        *scc     = cc->ServingCellConfigCommon;

  NR_SCHED_LOCK(&gNB->sched_lock);

  if (slot==0 && (*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0]>=257)) {
    //FR2
    const NR_TDD_UL_DL_Pattern_t *tdd = &scc->tdd_UL_DL_ConfigurationCommon->pattern1;
    AssertFatal(tdd,"Dynamic TDD not handled yet\n");
    const int nb_periods_per_frame = get_nb_periods_per_frame(tdd->dl_UL_TransmissionPeriodicity);
    // re-initialization of tdd_beam_association at beginning of frame
    for (int i=0; i<nb_periods_per_frame; i++)
      gNB->tdd_beam_association[i] = -1;
  }

  gNB->frame = frame;
  gNB->slot = slot;

  start_meas(&gNB->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_IN);

  /* send tick to RLC, PDCP, and X2AP every ms */
  if ((slot & ((1 << *scc->ssbSubcarrierSpacing) - 1)) == 0) {
    void nr_rlc_tick(int frame, int subframe);
    void nr_pdcp_tick(int frame, int subframe);
    nr_rlc_tick(frame, slot >> *scc->ssbSubcarrierSpacing);
    nr_pdcp_tick(frame, slot >> *scc->ssbSubcarrierSpacing);
    if (is_x2ap_enabled())
      x2ap_trigger();
  }

  for (int CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    //mbsfn_status[CC_id] = 0;

    // clear vrb_maps
    memset(cc[CC_id].vrb_map, 0, sizeof(uint16_t) * MAX_BWP_SIZE);
    // clear last scheduled slot's content (only)!
    const int num_slots = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
    const int size = gNB->vrb_map_UL_size;
    const int prev_slot = frame * num_slots + slot + size - 1;
    uint16_t *vrb_map_UL = cc[CC_id].vrb_map_UL;
    memcpy(&vrb_map_UL[prev_slot % size * MAX_BWP_SIZE], &gNB->ulprbbl, sizeof(uint16_t) * MAX_BWP_SIZE);

    clear_nr_nfapi_information(gNB, CC_id, frame, slot, &sched_info->DL_req, &sched_info->TX_req, &sched_info->UL_dci_req);
  }

  if ((slot == 0) && (frame & 127) == 0) {
    char stats_output[16000] = {0};
    dump_mac_stats(gNB, stats_output, sizeof(stats_output), true);
    LOG_I(NR_MAC, "Frame.Slot %d.%d\n%s\n", frame, slot, stats_output);
  }

  nr_mac_update_timers(module_idP, frame, slot);

  // This schedules MIB
  schedule_nr_mib(module_idP, frame, slot, &sched_info->DL_req);

  // This schedules SIB1
  if (get_softmodem_params()->sa == 1)
    schedule_nr_sib1(module_idP, frame, slot, &sched_info->DL_req, &sched_info->TX_req);

  // This schedule PRACH if we are not in phy_test mode
  if (get_softmodem_params()->phy_test == 0) {
    /* we need to make sure that resources for PRACH are free. To avoid that
       e.g. PUSCH has already been scheduled, make sure we schedule before
       anything else: below, we simply assume an advance one frame (minus one
       slot, because otherwise we would allocate the current slot in
       UL_tti_req_ahead), but be aware that, e.g., K2 is allowed to be larger
       (schedule_nr_prach will assert if resources are not free). */
    const sub_frame_t n_slots_ahead = nr_slots_per_frame[*scc->ssbSubcarrierSpacing] - 1;
    const frame_t f = (frame + (slot + n_slots_ahead) / nr_slots_per_frame[*scc->ssbSubcarrierSpacing]) % 1024;
    const sub_frame_t s = (slot + n_slots_ahead) % nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
    schedule_nr_prach(module_idP, f, s);
  }

  // Schedule CSI-RS transmission
  nr_csirs_scheduling(module_idP, frame, slot, nr_slots_per_frame[*scc->ssbSubcarrierSpacing], &sched_info->DL_req);

  // Schedule CSI measurement reporting
  nr_csi_meas_reporting(module_idP, frame, slot);

  nr_schedule_srs(module_idP, frame, slot);

  // This schedule RA procedure if not in phy_test mode
  // Otherwise consider 5G already connected
  if (get_softmodem_params()->phy_test == 0) {
    nr_schedule_RA(module_idP, frame, slot, &sched_info->UL_dci_req, &sched_info->DL_req, &sched_info->TX_req);
  }

  // This schedules the DCI for Uplink and subsequently PUSCH
  nr_schedule_ulsch(module_idP, frame, slot, &sched_info->UL_dci_req);

  // This schedules the DCI for Downlink and PDSCH
  start_meas(&gNB->schedule_dlsch);
  nr_schedule_ue_spec(module_idP, frame, slot, &sched_info->DL_req, &sched_info->TX_req);
  stop_meas(&gNB->schedule_dlsch);

  nr_sr_reporting(gNB, frame, slot);

  nr_schedule_pucch(gNB, frame, slot);

  /* TODO: we copy from gNB->UL_tti_req_ahead[0][current_index], ie. CC_id == 0,
   * is more than 1 CC supported?
   */
  AssertFatal(MAX_NUM_CCs == 1, "only 1 CC supported\n");
  const int current_index = ul_buffer_index(frame, slot, *scc->ssbSubcarrierSpacing, gNB->UL_tti_req_ahead_size);
  copy_ul_tti_req(&sched_info->UL_tti_req, &gNB->UL_tti_req_ahead[0][current_index]);

  stop_meas(&gNB->eNB_scheduler);
  NR_SCHED_UNLOCK(&gNB->sched_lock);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_OUT);
}
