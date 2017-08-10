/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file eNB_scheduler.c
 * \brief eNB scheduler top level function operates on per subframe basis
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 0.5
 * @ingroup _mac

 */

#include "assertions.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"

#include "LAYER2/MAC/proto.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#if defined(FLEXRAN_AGENT_SB_IF)
//Agent-related headers
#include "flexran_agent_extern.h"
#include "flexran_agent_mac.h"
#include "flexran_agent_mac_proto.h"
#endif

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

extern RAN_CONTEXT_t RC;

uint16_t pdcch_order_table[6] = {31,31,511,2047,2047,8191};

void check_ul_failure(module_id_t module_idP,int CC_id,int UE_id,
		      frame_t frameP, sub_frame_t subframeP) {

  UE_list_t                           *UE_list      = &RC.mac[module_idP]->UE_list;
  nfapi_dl_config_request_t           *DL_req       = &RC.mac[module_idP]->DL_req[0];
  uint16_t                            rnti          = UE_RNTI(module_idP,UE_id);
  eNB_UE_STATS                        *eNB_UE_stats = &RC.mac[module_idP]->UE_list.eNB_UE_stats[CC_id][UE_id];
  COMMON_channels_t                   *cc           = RC.mac[module_idP]->common_channels;

  // check uplink failure
  if ((UE_list->UE_sched_ctrl[UE_id].ul_failure_timer>0)&&
      (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync==0)) {
    LOG_D(MAC,"UE %d rnti %x: UL Failure timer %d \n",UE_id,rnti,UE_list->UE_sched_ctrl[UE_id].ul_failure_timer);
    if (UE_list->UE_sched_ctrl[UE_id].ra_pdcch_order_sent==0) {
      UE_list->UE_sched_ctrl[UE_id].ra_pdcch_order_sent=1;
      
      // add a format 1A dci for this UE to request an RA procedure (only one UE per subframe)
      LOG_D(MAC,"UE %d rnti %x: sending PDCCH order for RAPROC (failure timer %d) \n",UE_id,rnti,UE_list->UE_sched_ctrl[UE_id].ul_failure_timer);	    
      nfapi_dl_config_request_pdu_t* dl_config_pdu                    = &DL_req[CC_id].dl_config_request_body.dl_config_pdu_list[DL_req[CC_id].dl_config_request_body.number_pdu]; 
      memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type                                         = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE; 
      dl_config_pdu->pdu_size                                         = (uint8_t)(2+sizeof(nfapi_dl_config_dci_dl_pdu));
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format            = NFAPI_DL_DCI_FORMAT_1A;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level     = get_aggregation(get_bw_index(module_idP,CC_id),eNB_UE_stats->dl_cqi,format1A);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                  = rnti;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type             = 1;    // CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power    = 6000; // equal to RS power
      
      AssertFatal((cc[CC_id].mib->message.dl_Bandwidth >=0) && (cc[CC_id].mib->message.dl_Bandwidth<6),
		  "illegal dl_Bandwidth %d\n",(int)cc[CC_id].mib->message.dl_Bandwidth);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = pdcch_order_table[cc[CC_id].mib->message.dl_Bandwidth];
      DL_req[CC_id].dl_config_request_body.number_dci++;
      DL_req[CC_id].dl_config_request_body.number_pdu++;
     
      /* 
	 add_ue_spec_dci(&DL_req[CC_id],
	 rnti,
	 get_aggregation(get_tw_index(module_idP,CC_id),eNB_UE_stats->DL_cqi[0],format1A),
	 format1A,
	 NO_DLSCH);*/
    }
    else { // ra_pdcch_sent==1
      LOG_D(MAC,"UE %d rnti %x: sent PDCCH order for RAPROC waiting (failure timer %d) \n",UE_id,rnti,UE_list->UE_sched_ctrl[UE_id].ul_failure_timer);	    	    
      if ((UE_list->UE_sched_ctrl[UE_id].ul_failure_timer % 40) == 0)
	UE_list->UE_sched_ctrl[UE_id].ra_pdcch_order_sent=0; // resend every 4 frames	      
    }
    
    UE_list->UE_sched_ctrl[UE_id].ul_failure_timer++;
    // check threshold
    if (UE_list->UE_sched_ctrl[UE_id].ul_failure_timer > 200) {
      // inform RRC of failure and clear timer
      LOG_I(MAC,"UE %d rnti %x: UL Failure after repeated PDCCH orders: Triggering RRC \n",UE_id,rnti);
      mac_eNB_rrc_ul_failure(module_idP,CC_id,frameP,subframeP,rnti);
      UE_list->UE_sched_ctrl[UE_id].ul_failure_timer=0;
      UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync=1;
    }
  } // ul_failure_timer>0
  
}

void clear_nfapi_information(eNB_MAC_INST *eNB,int CC_idP,frame_t frameP,sub_frame_t subframeP);

void clear_nfapi_information(eNB_MAC_INST *eNB,int CC_idP,frame_t frameP,sub_frame_t subframeP) {

  nfapi_dl_config_request_t *DL_req         = &eNB->DL_req[0];
  nfapi_ul_config_request_t *UL_req         = &eNB->UL_req[0];
  nfapi_hi_dci0_request_t   *HI_DCI0_req    = &eNB->HI_DCI0_req[0];
  nfapi_tx_request_t        *TX_req         = &eNB->TX_req[0];
  
  eNB->pdu_index[CC_idP]                                     = 0;
  DL_req[CC_idP].dl_config_request_body.number_pdcch_ofdm_symbols           = 1;
  DL_req[CC_idP].dl_config_request_body.number_dci                          = 0;
  DL_req[CC_idP].dl_config_request_body.number_pdu                          = 0;
  DL_req[CC_idP].dl_config_request_body.number_pdsch_rnti                   = 0;
  DL_req[CC_idP].dl_config_request_body.transmission_power_pcfich           = 6000;
  
  HI_DCI0_req[CC_idP].hi_dci0_request_body.sfnsf                            = subframeP + (frameP<<3);
  HI_DCI0_req[CC_idP].hi_dci0_request_body.number_of_dci                    = 0;
  
  UL_req[CC_idP].ul_config_request_body.number_of_pdus                      = 0;
  UL_req[CC_idP].ul_config_request_body.rach_prach_frequency_resources      = 0; // ignored, handled by PHY for now
  UL_req[CC_idP].ul_config_request_body.srs_present                         = 0; // ignored, handled by PHY for now
  TX_req[CC_idP].tx_request_body.number_of_pdus                 = 0;

}

void eNB_dlsch_ulsch_scheduler(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)  
{

  int mbsfn_status[MAX_NUM_CCs];
  protocol_ctxt_t   ctxt;

#if defined(ENABLE_ITTI)
  MessageDef   *msg_p;
  const char         *msg_name;
  instance_t    instance;
  int           result;
#endif
  int CC_id,i; //,next_i;
  UE_list_t *UE_list=&RC.mac[module_idP]->UE_list;
  rnti_t rnti;
  
  COMMON_channels_t         *cc             = RC.mac[module_idP]->common_channels;

  eNB_UE_STATS              *eNB_UE_stats;
#if defined(FLEXRAN_AGENT_SB_IF)
  Protocol__FlexranMessage *msg;
#endif

  
  //  LOG_I(MAC,"[eNB %d] Frame %d, Subframe %d, entering MAC scheduler (UE_list->head %d)\n",module_idP, frameP, subframeP,UE_list->head);

  start_meas(&RC.mac[module_idP]->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_IN);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    mbsfn_status[CC_id]=0;
    // clear vrb_map
    memset(cc[CC_id].vrb_map,0,100);

    // clear DL/UL info for new scheduling round
    clear_nfapi_information(RC.mac[module_idP],CC_id,frameP,subframeP);

#if defined(Rel10) || defined(Rel14)
    cc[CC_id].mcch_active =0;
#endif
    RC.mac[module_idP]->frame    = frameP;
    RC.mac[module_idP]->subframe = subframeP;
  }

  // refresh UE list based on UEs dropped by PHY in previous subframe
  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    if (UE_list->active[i] != TRUE) continue;

    rnti         = UE_RNTI(module_idP, i);
    CC_id        = UE_PCCID(module_idP, i);
    eNB_UE_stats = &RC.mac[module_idP]->UE_list.eNB_UE_stats[CC_id][i];

    if ((frameP==0)&&(subframeP==0)) {
      LOG_I(MAC,"UE  rnti %x : %s, PHR %d dB CQI %d\n", rnti,
            UE_list->UE_sched_ctrl[i].ul_out_of_sync==0 ? "in synch" : "out of sync",
            UE_list->UE_template[CC_id][i].phr_info,
            eNB_UE_stats->dl_cqi);
    }

    RC.eNB[module_idP][CC_id]->pusch_stats_bsr[i][(frameP*10)+subframeP]=-63;
    if (i==UE_list->head)
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BSR,RC.eNB[module_idP][CC_id]->pusch_stats_bsr[i][(frameP*10)+subframeP]); 
    // increment this, it is cleared when we receive an sdu
    RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].ul_inactivity_timer++;

    RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].cqi_req_timer++;

    /*
      //Inform the controller about the UE deactivation. Should be moved to RRC agent in the future
#if defined(FLEXRAN_AGENT_S_IF)
      if (mac_agent_registered[module_idP]) {
	agent_mac_xface[module_idP]->flexran_agent_notify_ue_state_change(module_idP,
									  rnti,
									  PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED);
      }
#endif
}*/
    check_ul_failure(module_idP,CC_id,i,frameP,subframeP);

  }

#if defined(ENABLE_ITTI)

  do {
    // Checks if a message has been sent to MAC sub-task
    itti_poll_msg (TASK_MAC_ENB, &msg_p);

    if (msg_p != NULL) {
      msg_name = ITTI_MSG_NAME (msg_p);
      instance = ITTI_MSG_INSTANCE (msg_p);

      switch (ITTI_MSG_ID(msg_p)) {
      case MESSAGE_TEST:
        LOG_D(MAC, "Received %s\n", ITTI_MSG_NAME(msg_p));
        break;

      case RRC_MAC_BCCH_DATA_REQ:
        LOG_D(MAC, "Received %s from %s: instance %d, frameP %d, eNB_index %d\n",
              msg_name, ITTI_MSG_ORIGIN_NAME(msg_p), instance,
              RRC_MAC_BCCH_DATA_REQ (msg_p).frame, RRC_MAC_BCCH_DATA_REQ (msg_p).enb_index);

        // TODO process BCCH data req.
        break;

      case RRC_MAC_CCCH_DATA_REQ:
        LOG_D(MAC, "Received %s from %s: instance %d, frameP %d, eNB_index %d\n",
              msg_name, ITTI_MSG_ORIGIN_NAME(msg_p), instance,
              RRC_MAC_CCCH_DATA_REQ (msg_p).frame, RRC_MAC_CCCH_DATA_REQ (msg_p).enb_index);

        // TODO process CCCH data req.
        break;

#if defined(Rel10) || defined(Rel14)

      case RRC_MAC_MCCH_DATA_REQ:
        LOG_D(MAC, "Received %s from %s: instance %d, frameP %d, eNB_index %d, mbsfn_sync_area %d\n",
              msg_name, ITTI_MSG_ORIGIN_NAME(msg_p), instance,
              RRC_MAC_MCCH_DATA_REQ (msg_p).frame, RRC_MAC_MCCH_DATA_REQ (msg_p).enb_index, RRC_MAC_MCCH_DATA_REQ (msg_p).mbsfn_sync_area);

        // TODO process MCCH data req.
        break;
#endif

      default:
        LOG_E(MAC, "Received unexpected message %s\n", msg_name);
        break;
      }

      result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
      AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    }
  } while(msg_p != NULL);

#endif

/* #ifndef DISABLE_SF_TRIGGER */
/*   //Send subframe trigger to the controller */
/*   if (mac_agent_registered[module_idP]) { */
/*     agent_mac_xface[module_idP]->flexran_agent_send_sf_trigger(module_idP); */
/*   } */
/* #endif */
  
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES, NOT_A_RNTI, frameP, subframeP,module_idP);
  pdcp_run(&ctxt);

  rrc_rx_tx(&ctxt,
            0, // eNB index, unused in eNB
            CC_id);

#if defined(Rel10) || defined(Rel14)

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    if (cc[CC_id].MBMS_flag >0) {
      start_meas(&RC.mac[module_idP]->schedule_mch);
      mbsfn_status[CC_id] = schedule_MBMS(module_idP,CC_id,frameP,subframeP);
      stop_meas(&RC.mac[module_idP]->schedule_mch);
    }
  }

#endif

  // This schedules MIB
  if ((subframeP==0) && (frameP&3) == 0) schedule_mib(module_idP,frameP,subframeP);

  // This schedules SI for legacy LTE and eMTC
  schedule_SI(module_idP,frameP,subframeP);

  // This schedules Random-Access for legacy LTE and eMTC
  schedule_RA(module_idP,frameP,subframeP);

  // This schedules ULSCH in subframeP+k
  schedule_ulsch(module_idP,frameP,subframeP);

  // This schedules DLSCH in subframeP
  schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);

  // Allocate CCEs and Msg3 for good after scheduling is done
  for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
    allocate_CCEs(module_idP,CC_id,subframeP,0);
    check_and_add_msg3(module_idP,frameP,subframeP);
  }

#if defined(FLEXRAN_AGENT_SB_IF)
#ifndef DISABLE_CONT_STATS
  //Send subframe trigger to the controller
  if (mac_agent_registered[module_idP]) {
    agent_mac_xface[module_idP]->flexran_agent_send_update_mac_stats(module_idP);
  }
#endif
#endif


  stop_meas(&RC.mac[module_idP]->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_OUT);

}



