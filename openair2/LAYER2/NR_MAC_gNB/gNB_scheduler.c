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
#include "LAYER2/MAC/mac_extern.h"
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

//Agent-related headers
#include "flexran_agent_extern.h"
#include "flexran_agent_mac.h"

#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#endif

#include "assertions.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

extern RAN_CONTEXT_t RC;
extern int phy_test;
extern uint8_t nfapi_mode;

uint16_t nr_pdcch_order_table[6] = { 31, 31, 511, 2047, 2047, 8191 };

void clear_nr_nfapi_information(gNB_MAC_INST * gNB, 
                                int CC_idP,
                                frame_t frameP, 
                                sub_frame_t subframeP){

  nfapi_nr_dl_config_request_t    *DL_req = &gNB->DL_req[0];
  nfapi_ul_config_request_t       *UL_req = &gNB->UL_req[0];
  nfapi_hi_dci0_request_t   *     HI_DCI0_req = &gNB->HI_DCI0_req[0];
  nfapi_tx_request_t              *TX_req = &gNB->TX_req[0];

  gNB->pdu_index[CC_idP] = 0;

  if (nfapi_mode==0 || nfapi_mode == 1) { // monolithic or PNF

    DL_req[CC_idP].dl_config_request_body.number_pdcch_ofdm_symbols           = 1;
    DL_req[CC_idP].dl_config_request_body.number_dci                          = 0;
    DL_req[CC_idP].dl_config_request_body.number_pdu                          = 0;
    DL_req[CC_idP].dl_config_request_body.number_pdsch_rnti                   = 0;
    //DL_req[CC_idP].dl_config_request_body.transmission_power_pcfich           = 6000;

    HI_DCI0_req[CC_idP].hi_dci0_request_body.sfnsf                            = subframeP + (frameP<<4);
    HI_DCI0_req[CC_idP].hi_dci0_request_body.number_of_dci                    = 0;


    UL_req[CC_idP].ul_config_request_body.number_of_pdus                      = 0;
    UL_req[CC_idP].ul_config_request_body.rach_prach_frequency_resources      = 0; // ignored, handled by PHY for now
    UL_req[CC_idP].ul_config_request_body.srs_present                         = 0; // ignored, handled by PHY for now

    TX_req[CC_idP].tx_request_body.number_of_pdus                 = 0;

  }
}
/*
void check_nr_ul_failure(module_id_t module_idP, 
                         int CC_id, 
                         int UE_id,
                         frame_t frameP, 
                         sub_frame_t subframeP) {

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
void copy_nr_ulreq(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)
{
  int CC_id;
  gNB_MAC_INST *mac = RC.nrmac[module_idP];

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

    nfapi_ul_config_request_t *ul_req_tmp             = &mac->UL_req_tmp[CC_id][subframeP];
    nfapi_ul_config_request_t *ul_req                 = &mac->UL_req[CC_id];
    nfapi_ul_config_request_pdu_t *ul_req_pdu         = ul_req->ul_config_request_body.ul_config_pdu_list;

    *ul_req = *ul_req_tmp;

    // Restore the pointer
    ul_req->ul_config_request_body.ul_config_pdu_list = ul_req_pdu;
    ul_req->sfn_sf                                    = (frameP<<4) + subframeP;
    ul_req_tmp->ul_config_request_body.number_of_pdus = 0;

    if (ul_req->ul_config_request_body.number_of_pdus>0)
      {
        LOG_D(PHY, "%s() active NOW (frameP:%d subframeP:%d) pdus:%d\n", __FUNCTION__, frameP, subframeP, ul_req->ul_config_request_body.number_of_pdus);
      }

    memcpy((void*)ul_req->ul_config_request_body.ul_config_pdu_list,
     (void*)ul_req_tmp->ul_config_request_body.ul_config_pdu_list,
     ul_req->ul_config_request_body.number_of_pdus*sizeof(nfapi_ul_config_request_pdu_t));
  }
}

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP, 
                               frame_t frameP,
                               sub_frame_t subframeP){
  protocol_ctxt_t   ctxt;

  int               CC_id, i = -1;
  UE_list_t         *UE_list = &RC.nrmac[module_idP]->UE_list;
  rnti_t            rnti;

  NR_COMMON_channels_t *cc      = RC.nrmac[module_idP]->common_channels;

  start_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_IN);

  RC.nrmac[module_idP]->frame    = frameP;
  RC.nrmac[module_idP]->subframe = subframeP;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    //mbsfn_status[CC_id] = 0;

    // clear vrb_maps
    memset(cc[CC_id].vrb_map, 0, 100);
    memset(cc[CC_id].vrb_map_UL, 0, 100);

    clear_nr_nfapi_information(RC.nrmac[module_idP], CC_id, frameP, subframeP);
  }

  // refresh UE list based on UEs dropped by PHY in previous subframe
  for (i = 0; i < MAX_MOBILES_PER_GNB; i++) {
    if (UE_list->active[i]) {

      rnti = UE_RNTI(module_idP, i);
      CC_id = UE_PCCID(module_idP, i);
      
      if (((frameP&127) == 0) && (subframeP == 0)) {
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
      
      RC.gNB[module_idP][CC_id]->pusch_stats_bsr[i][(frameP * 10) + subframeP] = -63;
      
      if (i == UE_list->head)
        VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BSR,RC.gNB[module_idP][CC_id]->
                                                pusch_stats_bsr[i][(frameP * 10) + subframeP]);
      
      // increment this, it is cleared when we receive an sdu
      RC.nrmac[module_idP]->UE_list.UE_sched_ctrl[i].ul_inactivity_timer++;
      RC.nrmac[module_idP]->UE_list.UE_sched_ctrl[i].cqi_req_timer++;
      
      LOG_D(MAC, "UE %d/%x : ul_inactivity %d, cqi_req %d\n", 
            i, 
            rnti,
            RC.nrmac[module_idP]->UE_list.UE_sched_ctrl[i].ul_inactivity_timer,
            RC.nrmac[module_idP]->UE_list.UE_sched_ctrl[i].cqi_req_timer);
      
      //check_nr_ul_failure(module_idP, CC_id, i, frameP, subframeP);
      
      if (RC.nrmac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer > 0) {
        RC.nrmac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer++;
      
        if(RC.nrmac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer >=
          RC.nrmac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer_thres) {
          RC.nrmac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer = 0;
        
          for (int ue_id_l = 0; ue_id_l < MAX_MOBILES_PER_GNB; ue_id_l++) {
            if (reestablish_rnti_map[ue_id_l][0] == rnti) {
            // clear currentC-RNTI from map
            reestablish_rnti_map[ue_id_l][0] = 0;
            reestablish_rnti_map[ue_id_l][1] = 0;
            break;
            }
          }
      
          // Note: This should not be done in the MAC!
          for (int ii=0; ii<MAX_MOBILES_PER_GNB; ii++) {
            LTE_eNB_ULSCH_t *ulsch = RC.gNB[module_idP][CC_id]->ulsch[ii];       
            if((ulsch != NULL) && (ulsch->rnti == rnti)){
              LOG_I(MAC, "clean_eNb_ulsch UE %x \n", rnti);
              clean_eNb_ulsch(ulsch);
            }
          }

          for (int ii=0; ii<MAX_MOBILES_PER_GNB; ii++) {
            LTE_eNB_DLSCH_t *dlsch = RC.gNB[module_idP][CC_id]->dlsch[ii][0];
            if((dlsch != NULL) && (dlsch->rnti == rnti)){
              LOG_I(MAC, "clean_eNb_dlsch UE %x \n", rnti);
              clean_eNb_dlsch(dlsch);
            }
          }
    
          for(int j = 0; j < 10; j++){
            nfapi_ul_config_request_body_t *ul_req_tmp = NULL;
            ul_req_tmp = &RC.nrmac[module_idP]->UL_req_tmp[CC_id][j].ul_config_request_body;
            if(ul_req_tmp){
              int pdu_number = ul_req_tmp->number_of_pdus;
              for(int pdu_index = pdu_number-1; pdu_index >= 0; pdu_index--){
                if(ul_req_tmp->ul_config_pdu_list[pdu_index].ulsch_pdu.ulsch_pdu_rel8.rnti == rnti){
                  LOG_I(MAC, "remove UE %x from ul_config_pdu_list %d/%d\n", rnti, pdu_index, pdu_number);
                 if(pdu_index < pdu_number -1){
                    memcpy(&ul_req_tmp->ul_config_pdu_list[pdu_index], &ul_req_tmp->ul_config_pdu_list[pdu_index+1], (pdu_number-1-pdu_index) * sizeof(nfapi_ul_config_request_pdu_t));
                  }
                  ul_req_tmp->number_of_pdus--;
                }
              }
            }
          }
          rrc_mac_remove_ue(module_idP,rnti);
        }
      } //END if (RC.nrmac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer > 0)
    } //END if (UE_list->active[i])
  } //END for (i = 0; i < MAX_MOBILES_PER_GNB; i++)
  
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES,NOT_A_RNTI, frameP, subframeP,module_idP);
  
  pdcp_run(&ctxt);

  //rrc_rx_tx(&ctxt, CC_id);

  // This schedules MIB
  if((subframeP == 0) && (frameP & 7) == 0){
    schedule_nr_mib(module_idP, frameP, subframeP);
  }
  /*
  if (phy_test == 0){
    // This schedules SI for legacy LTE and eMTC starting in subframeP
    schedule_SI(module_idP, frameP, subframeP);
    // This schedules Paging in subframeP
    schedule_PCH(module_idP,frameP,subframeP);
    // This schedules Random-Access for legacy LTE and eMTC starting in subframeP
    schedule_RA(module_idP, frameP, subframeP);
    // copy previously scheduled UL resources (ULSCH + HARQ)
    copy_nr_ulreq(module_idP, frameP, subframeP);
    // This schedules SRS in subframeP
    schedule_nr_SRS(module_idP, frameP, subframeP);
    // This schedules ULSCH in subframeP (dci0)
    schedule_ulsch(module_idP, frameP, subframeP);
    // This schedules UCI_SR in subframeP
    schedule_nr_SR(module_idP, frameP, subframeP);
    // This schedules UCI_CSI in subframeP
    schedule_nr_CSI(module_idP, frameP, subframeP);
    // This schedules DLSCH in subframeP
    schedule_dlsch(module_idP, frameP, subframeP, mbsfn_status);
  }
  else{
    schedule_ulsch_phy_test(module_idP,frameP,subframeP);
    schedule_ue_spec_phy_test(module_idP,frameP,subframeP,mbsfn_status);
  }
  */

  if (RC.flexran[module_idP]->enabled)
    flexran_agent_send_update_stats(module_idP);
  
  /*
  // Allocate CCEs for good after scheduling is done
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++)
    allocate_CCEs(module_idP, CC_id, subframeP, 0);

  stop_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  */
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_OUT);
}
