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
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 0.5
 * @ingroup _mac

 */

#include "assertions.h"

#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/NR/nr_rrc_extern.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

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

uint16_t pdcch_order_table[6] = { 31, 31, 511, 2047, 2047, 8191 };


void gNB_dlsch_ulsch_scheduler(module_id_t module_idP, 
                               frame_t frameP,
                               sub_frame_t subframeP){

  int               mbsfn_status[MAX_NUM_CCs];
  protocol_ctxt_t   ctxt;

  int               CC_id, i = -1;
  UE_list_t         *UE_list = &RC.nrmac[module_idP]->UE_list;
  rnti_t            rnti;

  COMMON_channels_t *cc      = RC.nrmac[module_idP]->common_channels;

  start_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_IN);

  RC.nrmac[module_idP]->frame    = frameP;
  RC.nrmac[module_idP]->subframe = subframeP;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    mbsfn_status[CC_id] = 0;

    // clear vrb_maps
    memset(cc[CC_id].vrb_map, 0, 100);
    memset(cc[CC_id].vrb_map_UL, 0, 100);

    clear_nfapi_information(RC.nrmac[module_idP], CC_id, frameP, subframeP);
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
      
      check_ul_failure(module_idP, CC_id, i, frameP, subframeP);
      
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
            ul_req_tmp = &RC.mac[module_idP]->UL_req_tmp[CC_id][j].ul_config_request_body;
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

  rrc_rx_tx(&ctxt, CC_id);

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
    copy_ulreq(module_idP, frameP, subframeP);
    // This schedules SRS in subframeP
    schedule_SRS(module_idP, frameP, subframeP);
    // This schedules ULSCH in subframeP (dci0)
    schedule_ulsch(module_idP, frameP, subframeP);
    // This schedules UCI_SR in subframeP
    schedule_SR(module_idP, frameP, subframeP);
    // This schedules UCI_CSI in subframeP
    schedule_CSI(module_idP, frameP, subframeP);
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
  
  // Allocate CCEs for good after scheduling is done
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++)
    allocate_CCEs(module_idP, CC_id, subframeP, 0);

  stop_meas(&RC.mac[module_idP]->eNB_scheduler);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_OUT);
}