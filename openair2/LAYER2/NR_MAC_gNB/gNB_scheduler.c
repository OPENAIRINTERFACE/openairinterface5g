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
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/NR/nr_rrc_extern.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#include "openair1/PHY/defs_gNB.h"
#include "openair1/PHY/NR_TRANSPORT/nr_dlsch.h"

#include "intertask_interface.h"

#include "executables/softmodem-common.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "executables/nr-softmodem.h"

#include <errno.h>
#include <string.h>

uint16_t nr_pdcch_order_table[6] = { 31, 31, 511, 2047, 2047, 8191 };

void clear_mac_stats(gNB_MAC_INST *gNB) {
  memset((void*)gNB->UE_info.mac_stats,0,MAX_MOBILES_PER_GNB*sizeof(NR_mac_stats_t));
}
#define MACSTATSSTRLEN 16384
void dump_mac_stats(gNB_MAC_INST *gNB)
{
  NR_UE_info_t *UE_info = &gNB->UE_info;
  int num = 1;
  FILE *fd=fopen("nrMAC_stats.log","w");
  AssertFatal(fd!=NULL,"Cannot open nrMAC_stats.log, error %s\n",strerror(errno));
  char output[MACSTATSSTRLEN];
  memset(output,0,MACSTATSSTRLEN);
  int stroff=0;
  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {

    stroff+=sprintf(output+stroff,"UE ID %d RNTI %04x (%d/%d) PH %d dB PCMAX %d dBm\n",
      UE_id,
      UE_info->rnti[UE_id],
      num++,
      UE_info->num_UEs,
      UE_info->UE_sched_ctrl[UE_id].ph,
      UE_info->UE_sched_ctrl[UE_id].pcmax);

    LOG_I(NR_MAC, "UE ID %d RNTI %04x (%d/%d) PH %d dB PCMAX %d dBm\n",
          UE_id,
          UE_info->rnti[UE_id],
          num++,
          UE_info->num_UEs,
          UE_info->UE_sched_ctrl[UE_id].ph,
          UE_info->UE_sched_ctrl[UE_id].pcmax);

    NR_mac_stats_t *stats = &UE_info->mac_stats[UE_id];
    const int avg_rsrp = stats->num_rsrp_meas > 0 ? stats->cumul_rsrp / stats->num_rsrp_meas : 0;
    stroff+=sprintf(output+stroff,"UE %d: dlsch_rounds %d/%d/%d/%d, dlsch_errors %d, pucch0_DTX %d average RSRP %d (%d meas)\n",
          UE_id,
          stats->dlsch_rounds[0], stats->dlsch_rounds[1],
          stats->dlsch_rounds[2], stats->dlsch_rounds[3], stats->dlsch_errors,
          stats->pucch0_DTX,
          avg_rsrp, stats->num_rsrp_meas);
    LOG_I(NR_MAC, "UE %d: dlsch_rounds %d/%d/%d/%d, dlsch_errors %d\n",
      UE_id,
      stats->dlsch_rounds[0], stats->dlsch_rounds[1],
      stats->dlsch_rounds[2], stats->dlsch_rounds[3], stats->dlsch_errors);
    stats->num_rsrp_meas = 0;
    stats->cumul_rsrp = 0 ;
    stroff+=sprintf(output+stroff,"UE %d: dlsch_total_bytes %d\n", UE_id, stats->dlsch_total_bytes);
    stroff+=sprintf(output+stroff,"UE %d: ulsch_rounds %d/%d/%d/%d, ulsch_DTX %d, ulsch_errors %d\n",
                    UE_id,
                    stats->ulsch_rounds[0], stats->ulsch_rounds[1],
                    stats->ulsch_rounds[2], stats->ulsch_rounds[3],
                    stats->ulsch_DTX,
                    stats->ulsch_errors);
    stroff+=sprintf(output+stroff,
                    "UE %d: ulsch_total_bytes_scheduled %d, ulsch_total_bytes_received %d\n",
                    UE_id,
                    stats->ulsch_total_bytes_scheduled, stats->ulsch_total_bytes_rx);
    LOG_I(NR_MAC, "UE %d: dlsch_total_bytes %d\n", UE_id, stats->dlsch_total_bytes);
    LOG_I(NR_MAC, "UE %d: ulsch_rounds %d/%d/%d/%d, ulsch_errors %d\n",
          UE_id,
          stats->ulsch_rounds[0], stats->ulsch_rounds[1],
          stats->ulsch_rounds[2], stats->ulsch_rounds[3],
          stats->ulsch_errors);
    LOG_I(NR_MAC,
          "UE %d: ulsch_total_bytes (scheduled/received): %d / %d\n",
          UE_id,
          stats->ulsch_total_bytes_scheduled, stats->ulsch_total_bytes_rx);
    for (int lc_id = 0; lc_id < 63; lc_id++) {
      if (stats->lc_bytes_tx[lc_id] > 0) {
        stroff+=sprintf(output+stroff, "UE %d: LCID %d: %d bytes TX\n", UE_id, lc_id, stats->lc_bytes_tx[lc_id]);
	LOG_D(NR_MAC, "UE %d: LCID %d: %d bytes TX\n", UE_id, lc_id, stats->lc_bytes_tx[lc_id]);
      }
      if (stats->lc_bytes_rx[lc_id] > 0) {
        stroff+=sprintf(output+stroff, "UE %d: LCID %d: %d bytes RX\n", UE_id, lc_id, stats->lc_bytes_rx[lc_id]);
	LOG_D(NR_MAC, "UE %d: LCID %d: %d bytes RX\n", UE_id, lc_id, stats->lc_bytes_rx[lc_id]);
      }
    }
  }
  print_meas(&gNB->eNB_scheduler, "DL & UL scheduling timing stats", NULL, NULL);
  if (stroff>0) fprintf(fd,"%s",output);
  fclose(fd);
}

void clear_nr_nfapi_information(gNB_MAC_INST * gNB,
                                int CC_idP,
                                frame_t frameP,
                                sub_frame_t slotP){
  NR_ServingCellConfigCommon_t *scc = gNB->common_channels->ServingCellConfigCommon;
  const int num_slots = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];

  nfapi_nr_dl_tti_request_t    *DL_req = &gNB->DL_req[0];
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t ***pdcch = (nfapi_nr_dl_tti_pdcch_pdu_rel15_t ***)gNB->pdcch_pdu_idx[CC_idP];
  nfapi_nr_ul_tti_request_t    *future_ul_tti_req =
      &gNB->UL_tti_req_ahead[CC_idP][(slotP + num_slots - 1) % num_slots];
  nfapi_nr_ul_dci_request_t    *UL_dci_req = &gNB->UL_dci_req[0];
  nfapi_nr_tx_data_request_t   *TX_req = &gNB->TX_req[0];

  gNB->pdu_index[CC_idP] = 0;

  if (NFAPI_MODE == NFAPI_MONOLITHIC || NFAPI_MODE == NFAPI_MODE_PNF) { // monolithic or PNF

    DL_req[CC_idP].SFN                                   = frameP;
    DL_req[CC_idP].Slot                                  = slotP;
    DL_req[CC_idP].dl_tti_request_body.nPDUs             = 0;
    DL_req[CC_idP].dl_tti_request_body.nGroup            = 0;
    //DL_req[CC_idP].dl_tti_request_body.transmission_power_pcfich           = 6000;
    memset(pdcch, 0, sizeof(**pdcch) * MAX_NUM_BWP * MAX_NUM_CORESET);

    UL_dci_req[CC_idP].SFN                         = frameP;
    UL_dci_req[CC_idP].Slot                        = slotP;
    UL_dci_req[CC_idP].numPdus                     = 0;

    /* advance last round's future UL_tti_req to be ahead of current frame/slot */
    future_ul_tti_req->SFN = (slotP == 0 ? frameP : frameP + 1) % 1024;
    /* future_ul_tti_req->Slot is fixed! */
    future_ul_tti_req->n_pdus = 0;
    future_ul_tti_req->n_ulsch = 0;
    future_ul_tti_req->n_ulcch = 0;
    future_ul_tti_req->n_group = 0;

    /* UL_tti_req is a simple pointer into the current UL_tti_req_ahead, i.e.,
     * it walks over UL_tti_req_ahead in a circular fashion */
    gNB->UL_tti_req[CC_idP] = &gNB->UL_tti_req_ahead[CC_idP][slotP];

    TX_req[CC_idP].Number_of_PDUs                  = 0;

  }
}
/*
void check_nr_ul_failure(module_id_t module_idP,
                         int CC_id,
                         int UE_id,
                         frame_t frameP,
                         sub_frame_t slotP) {

  NR_UE_info_t                 *UE_info  = &RC.nrmac[module_idP]->UE_info;
  nfapi_nr_dl_dci_request_t  *DL_req   = &RC.nrmac[module_idP]->DL_req[0];
  uint16_t                      rnti      = UE_RNTI(module_idP, UE_id);
  NR_COMMON_channels_t          *cc       = RC.nrmac[module_idP]->common_channels;

  // check uplink failure
  if ((UE_info->UE_sched_ctrl[UE_id].ul_failure_timer > 0) &&
      (UE_info->UE_sched_ctrl[UE_id].ul_out_of_sync == 0)) {
    LOG_I(MAC, "UE %d rnti %x: UL Failure timer %d \n", UE_id, rnti,
    UE_info->UE_sched_ctrl[UE_id].ul_failure_timer);
    if (UE_info->UE_sched_ctrl[UE_id].ra_pdcch_order_sent == 0) {
      UE_info->UE_sched_ctrl[UE_id].ra_pdcch_order_sent = 1;

      // add a format 1A dci for this UE to request an RA procedure (only one UE per subframe)
      nfapi_nr_dl_dci_request_pdu_t *dl_config_pdu                    = &DL_req[CC_id].dl_tti_request_body.dl_config_pdu_list[DL_req[CC_id].dl_tti_request_body.number_pdu];
      memset((void *) dl_config_pdu, 0,sizeof(nfapi_dl_dci_request_pdu_t));
      dl_config_pdu->pdu_type                                         = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
      dl_config_pdu->pdu_size                                         = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag                = NFAPI_DL_DCI_REQUEST_DCI_DL_PDU_REL8_TAG;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format            = NFAPI_DL_DCI_FORMAT_1A;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level     = get_aggregation(get_bw_index(module_idP, CC_id),
                      UE_info->UE_sched_ctrl[UE_id].
                      dl_cqi[CC_id], format1A);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                  = rnti;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type             = 1;  // CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power    = 6000; // equal to RS power

      AssertFatal((cc[CC_id].mib->message.dl_Bandwidth >= 0) && (cc[CC_id].mib->message.dl_Bandwidth < 6),
      "illegal dl_Bandwidth %d\n",
      (int) cc[CC_id].mib->message.dl_Bandwidth);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = nr_pdcch_order_table[cc[CC_id].mib->message.dl_Bandwidth];
      DL_req[CC_id].dl_tti_request_body.number_dci++;
      DL_req[CC_id].dl_tti_request_body.number_pdu++;
      DL_req[CC_id].dl_tti_request_body.tl.tag                      = NFAPI_DL_TTI_REQUEST_BODY_TAG;
      LOG_I(MAC,
      "UE %d rnti %x: sending PDCCH order for RAPROC (failure timer %d), resource_block_coding %d \n",
      UE_id, rnti,
      UE_info->UE_sched_ctrl[UE_id].ul_failure_timer,
      dl_config_pdu->dci_dl_pdu.
      dci_dl_pdu_rel8.resource_block_coding);
    } else {    // ra_pdcch_sent==1
      LOG_I(MAC,
      "UE %d rnti %x: sent PDCCH order for RAPROC waiting (failure timer %d) \n",
      UE_id, rnti,
      UE_info->UE_sched_ctrl[UE_id].ul_failure_timer);
      if ((UE_info->UE_sched_ctrl[UE_id].ul_failure_timer % 40) == 0) UE_info->UE_sched_ctrl[UE_id].ra_pdcch_order_sent = 0;  // resend every 4 frames
    }

    UE_info->UE_sched_ctrl[UE_id].ul_failure_timer++;
    // check threshold
    if (UE_info->UE_sched_ctrl[UE_id].ul_failure_timer > 20000) {
      // inform RRC of failure and clear timer
      LOG_I(MAC,
      "UE %d rnti %x: UL Failure after repeated PDCCH orders: Triggering RRC \n",
      UE_id, rnti);
      mac_eNB_rrc_ul_failure(module_idP, CC_id, frameP, subframeP,rnti);
      UE_info->UE_sched_ctrl[UE_id].ul_failure_timer = 0;
      UE_info->UE_sched_ctrl[UE_id].ul_out_of_sync   = 1;

      //Inform the controller about the UE deactivation. Should be moved to RRC agent in the future
      if (rrc_agent_registered[module_idP]) {
        LOG_W(MAC, "notify flexran Agent of UE state change\n");
        agent_rrc_xface[module_idP]->flexran_agent_notify_ue_state_change(module_idP,
            rnti, PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED);
      }
    }
  }       // ul_failure_timer>0

}
*/
/*
void schedule_nr_SRS(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)
{
  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_UE_info_t *UE_info = &gNB->UE_info;
  nfapi_ul_config_request_body_t *ul_req;
  int CC_id, UE_id;
  NR_COMMON_channels_t *cc = RC.nrmac[module_idP]->common_channels;
  SoundingRS_UL_ConfigCommon_t *soundingRS_UL_ConfigCommon;
  struct SoundingRS_UL_ConfigDedicated *soundingRS_UL_ConfigDedicated;
  uint8_t TSFC;
  uint16_t deltaTSFC;   // bitmap
  uint8_t srs_SubframeConfig;

  // table for TSFC (Period) and deltaSFC (offset)
  const uint16_t deltaTSFCTabType1[15][2] = { {1, 1}, {1, 2}, {2, 2}, {1, 5}, {2, 5}, {4, 5}, {8, 5}, {3, 5}, {12, 5}, {1, 10}, {2, 10}, {4, 10}, {8, 10}, {351, 10}, {383, 10} };  // Table 5.5.3.3-2 3GPP 36.211 FDD
  const uint16_t deltaTSFCTabType2[14][2] = { {2, 5}, {6, 5}, {10, 5}, {18, 5}, {14, 5}, {22, 5}, {26, 5}, {30, 5}, {70, 10}, {74, 10}, {194, 10}, {326, 10}, {586, 10}, {210, 10} }; // Table 5.5.3.3-2 3GPP 36.211 TDD

  uint16_t srsPeriodicity, srsOffset;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    soundingRS_UL_ConfigCommon = &cc[CC_id].radioResourceConfigCommon->soundingRS_UL_ConfigCommon;
    // check if SRS is enabled in this frame/subframe
    if (soundingRS_UL_ConfigCommon) {
      srs_SubframeConfig = soundingRS_UL_ConfigCommon->choice.setup.srs_SubframeConfig;
      if (cc[CC_id].tdd_Config == NULL) { // FDD
  deltaTSFC = deltaTSFCTabType1[srs_SubframeConfig][0];
  TSFC = deltaTSFCTabType1[srs_SubframeConfig][1];
      } else {    // TDD
  deltaTSFC = deltaTSFCTabType2[srs_SubframeConfig][0];
  TSFC = deltaTSFCTabType2[srs_SubframeConfig][1];
      }
      // Sounding reference signal subframes are the subframes satisfying ns/2 mod TSFC (- deltaTSFC
      uint16_t tmp = (subframeP % TSFC);

      if ((1 << tmp) & deltaTSFC) {
  // This is an SRS subframe, loop over UEs
  for (UE_id = 0; UE_id < MAX_MOBILES_PER_GNB; UE_id++) {
    if (!RC.nrmac[module_idP]->UE_info.active[UE_id]) continue;
    ul_req = &RC.nrmac[module_idP]->UL_req[CC_id].ul_config_request_body;
    // drop the allocation if the UE hasn't send RRCConnectionSetupComplete yet
    if (mac_eNB_get_rrc_status(module_idP,UE_RNTI(module_idP, UE_id)) < RRC_CONNECTED) continue;

    AssertFatal(UE_info->UE_template[CC_id][UE_id].physicalConfigDedicated != NULL,
          "physicalConfigDedicated is null for UE %d\n",
          UE_id);

    if ((soundingRS_UL_ConfigDedicated = UE_info->UE_template[CC_id][UE_id].physicalConfigDedicated->soundingRS_UL_ConfigDedicated) != NULL) {
      if (soundingRS_UL_ConfigDedicated->present == SoundingRS_UL_ConfigDedicated_PR_setup) {
        get_srs_pos(&cc[CC_id],
        soundingRS_UL_ConfigDedicated->choice.
        setup.srs_ConfigIndex,
        &srsPeriodicity, &srsOffset);
        if (((10 * frameP + subframeP) % srsPeriodicity) == srsOffset) {
    // Program SRS
    ul_req->srs_present = 1;
    nfapi_ul_config_request_pdu_t * ul_config_pdu = &ul_req->ul_config_pdu_list[ul_req->number_of_pdus];
    memset((void *) ul_config_pdu, 0, sizeof(nfapi_ul_config_request_pdu_t));
    ul_config_pdu->pdu_type =  NFAPI_UL_CONFIG_SRS_PDU_TYPE;
    ul_config_pdu->pdu_size =  2 + (uint8_t) (2 + sizeof(nfapi_ul_config_srs_pdu));
    ul_config_pdu->srs_pdu.srs_pdu_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL8_TAG;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.size = (uint8_t)sizeof(nfapi_ul_config_srs_pdu);
    ul_config_pdu->srs_pdu.srs_pdu_rel8.rnti = UE_info->UE_template[CC_id][UE_id].rnti;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.srs_bandwidth = soundingRS_UL_ConfigDedicated->choice.setup.srs_Bandwidth;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.frequency_domain_position = soundingRS_UL_ConfigDedicated->choice.setup.freqDomainPosition;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.srs_hopping_bandwidth = soundingRS_UL_ConfigDedicated->choice.setup.srs_HoppingBandwidth;;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.transmission_comb = soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.i_srs = soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.sounding_reference_cyclic_shift = soundingRS_UL_ConfigDedicated->choice.setup.cyclicShift;    //              ul_config_pdu->srs_pdu.srs_pdu_rel10.antenna_port                   = ;//
    //              ul_config_pdu->srs_pdu.srs_pdu_rel13.number_of_combs                = ;//
    RC.nrmac[module_idP]->UL_req[CC_id].sfn_sf = (frameP << 4) + subframeP;
    RC.nrmac[module_idP]->UL_req[CC_id].header.message_id = NFAPI_UL_CONFIG_REQUEST;
    ul_req->number_of_pdus++;
        } // if (((10*frameP+subframeP) % srsPeriodicity) == srsOffset)
      } // if (soundingRS_UL_ConfigDedicated->present == SoundingRS_UL_ConfigDedicated_PR_setup)
    }   // if ((soundingRS_UL_ConfigDedicated = UE_info->UE_template[CC_id][UE_id].physicalConfigDedicated->soundingRS_UL_ConfigDedicated)!=NULL)
  }   // for (UE_id ...
      }     // if((1<<tmp) & deltaTSFC)

    }     // SRS config
  }
}
*/


bool is_xlsch_in_slot(uint64_t bitmap, sub_frame_t slot) {
  if (slot>=64) return false; //quickfix for FR2 where there are more than 64 slots (bitmap to be removed)
  return (bitmap >> slot) & 0x01;
}

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP,
                               frame_t frame,
                               sub_frame_t slot){

  protocol_ctxt_t   ctxt={0};
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES, NOT_A_RNTI, frame, slot,module_idP);

  const int bwp_id = 1;

  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = gNB->common_channels;
  NR_ServingCellConfigCommon_t        *scc     = cc->ServingCellConfigCommon;

  if (slot==0 && (*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0]>=257)) {
    const NR_TDD_UL_DL_Pattern_t *tdd = &scc->tdd_UL_DL_ConfigurationCommon->pattern1;
    const int n = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
    const int nr_mix_slots = tdd->nrofDownlinkSymbols != 0 || tdd->nrofUplinkSymbols != 0;
    const int nr_slots_period = tdd->nrofDownlinkSlots + tdd->nrofUplinkSlots + nr_mix_slots;
    const int nb_periods_per_frame = n / nr_slots_period;
    // re-initialization of tdd_beam_association at beginning of frame (only for FR2)
    for (int i=0; i<nb_periods_per_frame; i++)
      gNB->tdd_beam_association[i] = -1;
  }

  start_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_IN);

  pdcp_run(&ctxt);
  /* send tick to RLC and RRC every ms */
  if ((slot & ((1 << *scc->ssbSubcarrierSpacing) - 1)) == 0) {
    void nr_rlc_tick(int frame, int subframe);
    void nr_pdcp_tick(int frame, int subframe);
    nr_rlc_tick(frame, slot >> *scc->ssbSubcarrierSpacing);
    nr_pdcp_tick(frame, slot >> *scc->ssbSubcarrierSpacing);
    nr_rrc_trigger(&ctxt, 0 /*CC_id*/, frame, slot >> *scc->ssbSubcarrierSpacing);
  }

  memset(RC.nrmac[module_idP]->cce_list[0][0],0,MAX_NUM_CCE*sizeof(int)); // coreset0
  memset(RC.nrmac[module_idP]->cce_list[0][1],0,MAX_NUM_CCE*sizeof(int)); // coreset1 on initialBWP
  memset(RC.nrmac[module_idP]->cce_list[bwp_id][1],0,MAX_NUM_CCE*sizeof(int)); // coresetid 1
  NR_UE_info_t *UE_info = &RC.nrmac[module_idP]->UE_info;
  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id])
    for (int i=0; i<MAX_NUM_CORESET; i++)
      UE_info->num_pdcch_cand[UE_id][i] = 0;
  for (int CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    //mbsfn_status[CC_id] = 0;

    // clear vrb_maps
    memset(cc[CC_id].vrb_map, 0, sizeof(uint16_t) * MAX_BWP_SIZE);
    // clear last scheduled slot's content (only)!
    const int num_slots = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
    const int last_slot = (slot + num_slots - 1) % num_slots;
    uint16_t *vrb_map_UL = cc[CC_id].vrb_map_UL;
    memset(&vrb_map_UL[last_slot * MAX_BWP_SIZE], 0, sizeof(uint16_t) * MAX_BWP_SIZE);

    clear_nr_nfapi_information(RC.nrmac[module_idP], CC_id, frame, slot);
  }


  if ((slot == 0) && (frame & 127) == 0) dump_mac_stats(RC.nrmac[module_idP]);


  // This schedules MIB
  schedule_nr_mib(module_idP, frame, slot);

  // This schedules SIB1
  if ( get_softmodem_params()->sa == 1 )
    schedule_nr_sib1(module_idP, frame, slot);


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

  // This schedule SR
  nr_sr_reporting(module_idP, frame, slot);

  // Schedule CSI-RS transmission
  nr_csirs_scheduling(module_idP, frame, slot, nr_slots_per_frame[*scc->ssbSubcarrierSpacing]);

  // Schedule CSI measurement reporting: check in slot 0 for the whole frame
  if (slot == 0)
    nr_csi_meas_reporting(module_idP, frame, slot);

  // This schedule RA procedure if not in phy_test mode
  // Otherwise already consider 5G already connected
  if (get_softmodem_params()->phy_test == 0) {
    nr_schedule_RA(module_idP, frame, slot);
  }

  // This schedules the DCI for Uplink and subsequently PUSCH
  nr_schedule_ulsch(module_idP, frame, slot);

  // This schedules the DCI for Downlink and PDSCH
  nr_schedule_ue_spec(module_idP, frame, slot);

  nr_schedule_pucch(module_idP, frame, slot);

  stop_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_OUT);
}
