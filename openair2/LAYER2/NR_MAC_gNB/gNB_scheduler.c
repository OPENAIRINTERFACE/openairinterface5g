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

#include "intertask_interface.h"

#include "executables/softmodem-common.h"

uint16_t nr_pdcch_order_table[6] = { 31, 31, 511, 2047, 2047, 8191 };

uint8_t nr_slots_per_frame[5] = {10, 20, 40, 80, 160};

void clear_nr_nfapi_information(gNB_MAC_INST * gNB,
                                int CC_idP,
                                frame_t frameP,
                                sub_frame_t slotP){

  nfapi_nr_dl_tti_request_t    *DL_req = &gNB->DL_req[0];
  nfapi_nr_ul_tti_request_t    *UL_tti_req = &gNB->UL_tti_req[0];
  nfapi_nr_ul_dci_request_t    *UL_dci_req = &gNB->UL_dci_req[0];
  nfapi_nr_tx_data_request_t   *TX_req = &gNB->TX_req[0];

  gNB->pdu_index[CC_idP] = 0;

  if (nfapi_mode==0 || nfapi_mode == 1) { // monolithic or PNF

    DL_req[CC_idP].SFN                                   = frameP;
    DL_req[CC_idP].Slot                                  = slotP;
    DL_req[CC_idP].dl_tti_request_body.nPDUs             = 0;
    DL_req[CC_idP].dl_tti_request_body.nGroup            = 0;
    //DL_req[CC_idP].dl_tti_request_body.transmission_power_pcfich           = 6000;

    UL_dci_req[CC_idP].SFN                         = frameP;
    UL_dci_req[CC_idP].Slot                        = slotP;
    UL_dci_req[CC_idP].numPdus                     = 0;

    UL_tti_req[CC_idP].SFN                         = frameP;
    UL_tti_req[CC_idP].Slot                        = slotP;
    UL_tti_req[CC_idP].n_pdus                      = 0;
    UL_tti_req[CC_idP].n_ulsch                     = 0;
    UL_tti_req[CC_idP].n_ulcch                     = 0;
    UL_tti_req[CC_idP].n_group                     = 0;

    TX_req[CC_idP].Number_of_PDUs                  = 0;

  }
}
/*
void check_nr_ul_failure(module_id_t module_idP,
                         int CC_id,
                         int UE_id,
                         frame_t frameP,
                         sub_frame_t slotP) {

  UE_list_t                     *UE_list  = &RC.nrmac[module_idP]->UE_list;
  nfapi_nr_dl_dci_request_t  *DL_req   = &RC.nrmac[module_idP]->DL_req[0];
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
      nfapi_nr_dl_dci_request_pdu_t *dl_config_pdu                    = &DL_req[CC_id].dl_tti_request_body.dl_config_pdu_list[DL_req[CC_id].dl_tti_request_body.number_pdu];
      memset((void *) dl_config_pdu, 0,sizeof(nfapi_dl_dci_request_pdu_t));
      dl_config_pdu->pdu_type                                         = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
      dl_config_pdu->pdu_size                                         = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag                = NFAPI_DL_DCI_REQUEST_DCI_DL_PDU_REL8_TAG;
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
      DL_req[CC_id].dl_tti_request_body.number_dci++;
      DL_req[CC_id].dl_tti_request_body.number_pdu++;
      DL_req[CC_id].dl_tti_request_body.tl.tag                      = NFAPI_DL_TTI_REQUEST_BODY_TAG;
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
void schedule_nr_prach(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP) {

  gNB_MAC_INST *gNB = RC.nrmac[module_idP];

  // schedule PRACH for iniital BWP 

  if (is_initialBWP_prach_subframe(frameP,subframeP)<0) return;
 
  // fill FAPI
}
*/

/*
void copy_nr_ulreq(module_id_t module_idP, frame_t frameP, sub_frame_t slotP)
{
  int CC_id;
  gNB_MAC_INST *mac = RC.nrmac[module_idP];

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

    nfapi_ul_config_request_t *ul_req                 = &mac->UL_tti_req[CC_id];

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

void nr_schedule_pucch(int Mod_idP,
                       int UE_id,
                       frame_t frameP,
                       sub_frame_t slotP) {

  uint16_t O_uci;
  uint16_t O_ack;
  uint8_t SR_flag = 0; // no SR in PUCCH implemented for now
  NR_ServingCellConfigCommon_t *scc = RC.nrmac[Mod_idP]->common_channels->ServingCellConfigCommon;
  NR_UE_list_t *UE_list = &RC.nrmac[Mod_idP]->UE_list;
  AssertFatal(UE_list->active[UE_id] >=0,"Cannot find UE_id %d is not active\n",UE_id);

  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
  int bwp_id=1;
  NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_id-1];
  nfapi_nr_ul_tti_request_t *UL_tti_req = &RC.nrmac[Mod_idP]->UL_tti_req[0];

  NR_sched_pucch *curr_pucch = UE_list->UE_sched_ctrl[UE_id].sched_pucch;
  NR_sched_pucch *temp_pucch;
  int release_pucch = 0;

  if (curr_pucch != NULL) {
    if ((frameP == curr_pucch->frame) && (slotP == curr_pucch->ul_slot)) {
      UL_tti_req->SFN = frameP;
      UL_tti_req->Slot = slotP;
      UL_tti_req->pdus_list[UL_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE;
      UL_tti_req->pdus_list[UL_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pucch_pdu_t);
      nfapi_nr_pucch_pdu_t  *pucch_pdu = &UL_tti_req->pdus_list[UL_tti_req->n_pdus].pucch_pdu;
      memset(pucch_pdu,0,sizeof(nfapi_nr_pucch_pdu_t));
      UL_tti_req->n_pdus+=1;
      O_ack = curr_pucch->dai_c;
      O_uci = O_ack; // for now we are just sending acknacks in pucch

      nr_configure_pucch(pucch_pdu,
			 scc,
			 ubwp,
                         curr_pucch->resource_indicator,
                         O_uci,
                         O_ack,
                         SR_flag);

      release_pucch = 1;
    }
  }

  if (release_pucch) {
    temp_pucch = UE_list->UE_sched_ctrl[UE_id].sched_pucch;
    UE_list->UE_sched_ctrl[UE_id].sched_pucch = UE_list->UE_sched_ctrl[UE_id].sched_pucch->next_sched_pucch;
    free(temp_pucch);
  }

}

bool is_xlsch_in_slot(uint64_t bitmap, sub_frame_t slot){

  if((bitmap>>slot)&0x01)
    return true;
  else
    return false;
}

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP,
                               frame_t frame_rxP,
                               sub_frame_t slot_rxP,
                               frame_t frame_txP,
                               sub_frame_t slot_txP){

  //printf("gNB_dlsch_ulsch_scheduler frameRX %d slotRX %d frameTX %d slotTX %d\n",frame_rxP,slot_rxP,frame_txP,slot_txP);
			       
  protocol_ctxt_t   ctxt;
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES, NOT_A_RNTI, frame_txP, slot_txP,module_idP);
  int CC_id, UE_id = 0;
  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_UE_list_t *UE_list = &gNB->UE_list;
  NR_UE_sched_ctrl_t *ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
  NR_COMMON_channels_t *cc = gNB->common_channels;
  NR_sched_pucch *pucch_sched = (NR_sched_pucch*) malloc(sizeof(NR_sched_pucch));

  start_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_IN);

  pdcp_run(&ctxt);
  //rrc_rx_tx(&ctxt, CC_id);

  RC.nrmac[module_idP]->frame    = frame_rxP;
  RC.nrmac[module_idP]->slot     = slot_rxP;

  // Check if there are downlink symbols in the slot, 
  if (is_nr_DL_slot(cc->ServingCellConfigCommon,slot_txP)) {

    memset(RC.nrmac[module_idP]->cce_list[1][0],0,MAX_NUM_CCE*sizeof(int));
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
      //mbsfn_status[CC_id] = 0;

      // clear vrb_maps
      memset(cc[CC_id].vrb_map, 0, 100);
      memset(cc[CC_id].vrb_map_UL, 0, 100);

      clear_nr_nfapi_information(RC.nrmac[module_idP], CC_id, frame_txP, slot_txP);
    }

    // refresh UE list based on UEs dropped by PHY in previous subframe
    /*
    for (i = 0; i < MAX_MOBILES_PER_GNB; i++) {
      if (UE_list->active[i]) {

        nfapi_nr_config_request_t *cfg = &RC.nrmac[module_idP]->config[CC_id];
      
      
        rnti = 0;//UE_RNTI(module_idP, i);
        CC_id = 0;//UE_PCCID(module_idP, i);
      
    } //END if (UE_list->active[i])
  } //END for (i = 0; i < MAX_MOBILES_PER_GNB; i++)
  */
  
  // This schedules MIB
  if((slot_txP == 0) && (frame_txP & 7) == 0){
    schedule_nr_mib(module_idP, frame_txP, slot_txP);
  }

  // TbD once RACH is available, start ta_timer when UE is connected
#if 0
   if (ue_sched_ctl->ta_timer) ue_sched_ctl->ta_timer--;

  if (ue_sched_ctl->ta_timer == 0) {
    gNB->ta_command = ue_sched_ctl->ta_update;
    /* if time is up, then set the timer to not send it for 5 frames
    // regardless of the TA value */
    ue_sched_ctl->ta_timer = 100;
    /* reset ta_update */
    ue_sched_ctl->ta_update = 31;
    /* MAC CE flag indicating TA length */
    gNB->ta_len = 2;
  }
#endif

  // Phytest scheduling
  if (get_softmodem_params()->phy_test && slot_txP==1){
    nr_schedule_uss_dlsch_phytest(module_idP, frame_txP, slot_txP, pucch_sched, NULL);
        // resetting ta flag
    gNB->ta_len = 0;
  }

    /*
    // Allocate CCEs for good after scheduling is done
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++)
      allocate_CCEs(module_idP, CC_id, subframeP, 0);
    */

  } //is_nr_DL_slot

  if (is_nr_UL_slot(cc->ServingCellConfigCommon,slot_rxP)) { 
    if (get_softmodem_params()->phy_test && slot_rxP==8){
      nr_schedule_uss_ulsch_phytest(module_idP, frame_rxP, slot_rxP);
    }
  }

  stop_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_OUT);
}
