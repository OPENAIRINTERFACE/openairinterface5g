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

#include "LAYER2/MAC/mac.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"

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

//Agent-related headers
#include "flexran_agent_extern.h"
#include "flexran_agent_mac.h"

#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#endif

#include "assertions.h"
#include <openair1/PHY/LTE_TRANSPORT/transport_proto.h>

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

uint16_t nr_pdcch_order_table[6] = { 31, 31, 511, 2047, 2047, 8191 };

void clear_nr_nfapi_information(gNB_MAC_INST * gNB,
                                int CC_idP,
                                frame_t frameP,
                                sub_frame_t slotP){

  nfapi_nr_dl_config_request_t    *DL_req = &gNB->DL_req[0];
  nfapi_nr_ul_tti_request_t          *UL_tti_req = &gNB->UL_tti_req[0];
  nfapi_hi_dci0_request_t   *     HI_DCI0_req = &gNB->HI_DCI0_req[0];
  nfapi_tx_request_t              *TX_req = &gNB->TX_req[0];

  gNB->pdu_index[CC_idP] = 0;

  if (nfapi_mode==0 || nfapi_mode == 1) { // monolithic or PNF

    DL_req[CC_idP].dl_config_request_body.number_dci                          = 0;
    DL_req[CC_idP].dl_config_request_body.number_pdu                          = 0;
    DL_req[CC_idP].dl_config_request_body.number_pdsch_rnti                   = 0;
    //DL_req[CC_idP].dl_config_request_body.transmission_power_pcfich           = 6000;

    HI_DCI0_req[CC_idP].hi_dci0_request_body.sfnsf                            = slotP + (frameP<<7);
    HI_DCI0_req[CC_idP].hi_dci0_request_body.number_of_dci                    = 0;


    UL_tti_req[CC_idP].n_pdus                      = 0;
    UL_tti_req[CC_idP].n_ulsch                     = 0;
    UL_tti_req[CC_idP].n_ulcch                     = 0;
    UL_tti_req[CC_idP].n_group                     = 0;

    TX_req[CC_idP].tx_request_body.number_of_pdus                 = 0;

  }
}
/*
void check_nr_ul_failure(module_id_t module_idP,
                         int CC_id,
                         int UE_id,
                         frame_t frameP,
                         sub_frame_t slotP) {

  UE_list_t                     *UE_list  = &RC.nrmac[module_idP]->UE_list;
  nfapi_nr_dl_config_request_t  *DL_req   = &RC.nrmac[module_idP]->DL_req[0];
  uint16_t                      rnti      = UE_RNTI(module_idP, UE_id);
  NR_COMMON_channels_t          *cc       = RC.nrmac[module_idP]->common_channels;

  // check uplink failure
  if ((UE_list->UE_sched_ctrl[UE_id].ul_failure_timer > 0) &&
      (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 0)) {
    LOG_I(MAC, "UE %d rnti %x: UL Failure timer %d \n", UE_id, rnti,
    UE_list->UE_sched_ctrl[UE_id].ul_failure_timer);
    if (UE_list->UE_sched_ctrl[UE_id].ra_pdcch_order_sent == 0) {
      UE_list->UE_sched_ctrl[UE_id].ra_pdcch_order_sent = 1;

      // add a format 1A dci for this UE to request an RA procedure (only one UE per subframe)
      nfapi_nr_dl_config_request_pdu_t *dl_config_pdu                    = &DL_req[CC_id].dl_config_request_body.dl_config_pdu_list[DL_req[CC_id].dl_config_request_body.number_pdu];
      memset((void *) dl_config_pdu, 0,sizeof(nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type                                         = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
      dl_config_pdu->pdu_size                                         = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag                = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format            = NFAPI_DL_DCI_FORMAT_1A;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level     = get_aggregation(get_bw_index(module_idP, CC_id),
                      UE_list->UE_sched_ctrl[UE_id].
                      dl_cqi[CC_id], format1A);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                  = rnti;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type             = 1;  // CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power    = 6000; // equal to RS power

      AssertFatal((cc[CC_id].mib->message.dl_Bandwidth >= 0) && (cc[CC_id].mib->message.dl_Bandwidth < 6),
      "illegal dl_Bandwidth %d\n",
      (int) cc[CC_id].mib->message.dl_Bandwidth);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = nr_pdcch_order_table[cc[CC_id].mib->message.dl_Bandwidth];
      DL_req[CC_id].dl_config_request_body.number_dci++;
      DL_req[CC_id].dl_config_request_body.number_pdu++;
      DL_req[CC_id].dl_config_request_body.tl.tag                      = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
      LOG_I(MAC,
      "UE %d rnti %x: sending PDCCH order for RAPROC (failure timer %d), resource_block_coding %d \n",
      UE_id, rnti,
      UE_list->UE_sched_ctrl[UE_id].ul_failure_timer,
      dl_config_pdu->dci_dl_pdu.
      dci_dl_pdu_rel8.resource_block_coding);
    } else {    // ra_pdcch_sent==1
      LOG_I(MAC,
      "UE %d rnti %x: sent PDCCH order for RAPROC waiting (failure timer %d) \n",
      UE_id, rnti,
      UE_list->UE_sched_ctrl[UE_id].ul_failure_timer);
      if ((UE_list->UE_sched_ctrl[UE_id].ul_failure_timer % 40) == 0) UE_list->UE_sched_ctrl[UE_id].ra_pdcch_order_sent = 0;  // resend every 4 frames
    }

    UE_list->UE_sched_ctrl[UE_id].ul_failure_timer++;
    // check threshold
    if (UE_list->UE_sched_ctrl[UE_id].ul_failure_timer > 20000) {
      // inform RRC of failure and clear timer
      LOG_I(MAC,
      "UE %d rnti %x: UL Failure after repeated PDCCH orders: Triggering RRC \n",
      UE_id, rnti);
      mac_eNB_rrc_ul_failure(module_idP, CC_id, frameP, subframeP,rnti);
      UE_list->UE_sched_ctrl[UE_id].ul_failure_timer = 0;
      UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync   = 1;

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
  UE_list_t *UE_list = &gNB->UE_list;
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
    if (!RC.nrmac[module_idP]->UE_list.active[UE_id]) continue;
    ul_req = &RC.nrmac[module_idP]->UL_req[CC_id].ul_config_request_body;
    // drop the allocation if the UE hasn't send RRCConnectionSetupComplete yet
    if (mac_eNB_get_rrc_status(module_idP,UE_RNTI(module_idP, UE_id)) < RRC_CONNECTED) continue;

    AssertFatal(UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated != NULL,
          "physicalConfigDedicated is null for UE %d\n",
          UE_id);

    if ((soundingRS_UL_ConfigDedicated = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->soundingRS_UL_ConfigDedicated) != NULL) {
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
    ul_config_pdu->srs_pdu.srs_pdu_rel8.rnti = UE_list->UE_template[CC_id][UE_id].rnti;
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
    }   // if ((soundingRS_UL_ConfigDedicated = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->soundingRS_UL_ConfigDedicated)!=NULL)
  }   // for (UE_id ...
      }     // if((1<<tmp) & deltaTSFC)

    }     // SRS config
  }
}
*/

/*
void copy_nr_ulreq(module_id_t module_idP, frame_t frameP, sub_frame_t slotP)
{
  int CC_id;
  gNB_MAC_INST *mac = RC.nrmac[module_idP];

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

    nfapi_ul_config_request_t *ul_req_tmp             = &mac->UL_req_tmp[CC_id][slotP];
    nfapi_ul_config_request_t *ul_req                 = &mac->UL_req[CC_id];
    nfapi_ul_config_request_pdu_t *ul_req_pdu         = ul_req->ul_config_request_body.ul_config_pdu_list;

    *ul_req = *ul_req_tmp;

    // Restore the pointer
    ul_req->ul_config_request_body.ul_config_pdu_list = ul_req_pdu;
    ul_req->sfn_sf                                    = (frameP<<7) + slotP;
    ul_req_tmp->ul_config_request_body.number_of_pdus = 0;

    if (ul_req->ul_config_request_body.number_of_pdus>0)
      {
        LOG_D(PHY, "%s() active NOW (frameP:%d slotP:%d) pdus:%d\n", __FUNCTION__, frameP, slotP, ul_req->ul_config_request_body.number_of_pdus);
      }

    memcpy((void*)ul_req->ul_config_request_body.ul_config_pdu_list,
     (void*)ul_req_tmp->ul_config_request_body.ul_config_pdu_list,
     ul_req->ul_config_request_body.number_of_pdus*sizeof(nfapi_ul_config_request_pdu_t));
  }
}
*/

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP,
                               frame_t frame_rxP,
                               sub_frame_t slot_rxP,
                               frame_t frame_txP,
                               sub_frame_t slot_txP){
			       
  protocol_ctxt_t   ctxt;

  int               CC_id, i = -1;
  UE_list_t         *UE_list = &RC.nrmac[module_idP]->UE_list;
  rnti_t            rnti;

  NR_COMMON_channels_t *cc      = RC.nrmac[module_idP]->common_channels;
  //nfapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config = NULL;

  start_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_IN);

  RC.nrmac[module_idP]->frame    = frame_rxP;
  RC.nrmac[module_idP]->slot     = slot_rxP;


  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    //mbsfn_status[CC_id] = 0;

    // clear vrb_maps
    memset(cc[CC_id].vrb_map, 0, 100);
    memset(cc[CC_id].vrb_map_UL, 0, 100);

    clear_nr_nfapi_information(RC.nrmac[module_idP], CC_id, frame_txP, slot_txP);
  }

  // refresh UE list based on UEs dropped by PHY in previous subframe
  for (i = 0; i < MAX_MOBILES_PER_GNB; i++) {
    if (0 /*UE_list->active[i]*/) {

      nfapi_nr_config_request_t *cfg = &RC.nrmac[module_idP]->config[CC_id];

      nfapi_nr_coreset_t coreset = RC.nrmac[module_idP]->coreset[CC_id][1];
      nfapi_nr_search_space_t search_space = RC.nrmac[module_idP]->search_space[CC_id][1];

      if (nr_is_dci_opportunity(search_space,
                                    coreset,
                                    frame_txP,
                                    slot_txP,
                                    *cfg)){
          nr_schedule_uss_dlsch_phytest(module_idP, frame_txP, slot_txP, NULL);
          }

      rnti = UE_RNTI(module_idP, i);
      CC_id = UE_PCCID(module_idP, i);
      //int spf = get_spf(cfg);
  
      if (((frame_txP&127) == 0) && (slot_txP == 0)) {
        LOG_I(MAC,
        "UE  rnti %x : %s, PHR %d dB DL CQI %d PUSCH SNR %d PUCCH SNR %d\n",
        rnti,
        UE_list->UE_sched_ctrl[i].ul_out_of_sync ==
        0 ? "in synch" : "out of sync",
        UE_list->UE_template[CC_id][i].phr_info,
        UE_list->UE_sched_ctrl[i].dl_cqi[CC_id],
        (UE_list->UE_sched_ctrl[i].pusch_snr[CC_id] - 128) / 2,
        (UE_list->UE_sched_ctrl[i].pucch1_snr[CC_id] - 128) / 2);
      }
      
    } //END if (UE_list->active[i])
  } //END for (i = 0; i < MAX_MOBILES_PER_GNB; i++)
  
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES,NOT_A_RNTI, frame_txP, slot_txP,module_idP);
  
  pdcp_run(&ctxt);

  //rrc_rx_tx(&ctxt, CC_id);

  // This schedules MIB
  if((slot_txP == 0) && (frame_txP & 7) == 0){
    schedule_nr_mib(module_idP, frame_txP, slot_txP);
  }

  // Phytest scheduling
 
  if (slot_rxP == NR_UPLINK_SLOT){
    nr_schedule_uss_ulsch_phytest(&RC.nrmac[module_idP]->UL_tti_req[0], frame_rxP, slot_rxP);
  }
  
  if (slot_txP == NR_DOWNLINK_SLOT){
    nr_schedule_ue_spec(module_idP, frame_txP, slot_txP);
    nr_schedule_uss_dlsch_phytest(module_idP, frame_txP, slot_txP,NULL);
  }

  /*
  // Allocate CCEs for good after scheduling is done
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++)
    allocate_CCEs(module_idP, CC_id, subframeP, 0);
  */

  stop_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_OUT);
}
