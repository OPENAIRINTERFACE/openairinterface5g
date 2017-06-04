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
//efine ENABLE_ENB_AGENT_DL_SCHEDULER
//#define DISABLE_SF_TRIGGER
//#define DISABLE_CONT_STATS

//#define DEBUG_HEADER_PARSING 1
//#define DEBUG_PACKET_TRACE 1

/*
  #ifndef USER_MODE
  #define msg debug_msg
  #endif
 */







void eNB_dlsch_ulsch_scheduler(module_id_t module_idP,uint8_t cooperation_flag, frame_t frameP, sub_frame_t subframeP)  //, int calibration_flag) {
{

  int mbsfn_status[MAX_NUM_CCs];
  protocol_ctxt_t   ctxt;
#ifdef EXMIMO
  //int ret;
#endif
#if defined(ENABLE_ITTI)
  MessageDef   *msg_p;
  const char         *msg_name;
  instance_t    instance;
  int           result;
#endif
  DCI_PDU *DCI_pdu[MAX_NUM_CCs];
  int CC_id,i; //,next_i;
  UE_list_t *UE_list=&eNB_mac_inst[module_idP].UE_list;
  rnti_t rnti;
  void         *DLSCH_dci=NULL;
  int size_bits=0,size_bytes=0;
  
  LTE_eNB_UE_stats  *eNB_UE_stats   = NULL;

#if defined(FLEXRAN_AGENT_SB_IF)
  Protocol__FlexranMessage *msg;
#endif

  LOG_D(MAC,"[eNB %d] Frame %d, Subframe %d, entering MAC scheduler (UE_list->head %d)\n",module_idP, frameP, subframeP,UE_list->head);

  start_meas(&eNB_mac_inst[module_idP].eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_IN);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    DCI_pdu[CC_id] = &eNB_mac_inst[module_idP].common_channels[CC_id].DCI_pdu;
    mbsfn_status[CC_id]=0;
    // clear vrb_map
    memset(eNB_mac_inst[module_idP].common_channels[CC_id].vrb_map,0,100);
  }

  // clear DCI and BCCH contents before scheduling
  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    DCI_pdu[CC_id]->Num_common_dci  = 0;
    DCI_pdu[CC_id]->Num_ue_spec_dci = 0;
#if defined(Rel10) || defined(Rel14)
    eNB_mac_inst[module_idP].common_channels[CC_id].mcch_active =0;
#endif
    eNB_mac_inst[module_idP].frame    = frameP;
    eNB_mac_inst[module_idP].subframe = subframeP;
  }

  // refresh UE list based on UEs dropped by PHY in previous subframe
  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    if (UE_list->active[i] != TRUE) continue;

    rnti = UE_RNTI(module_idP, i);
    CC_id = UE_PCCID(module_idP, i);
    if ((frameP==0)&&(subframeP==0)) {
      LTE_eNB_UE_stats *eNB_UE_stats = mac_xface->get_eNB_UE_stats(module_idP, CC_id, rnti);
      int cqi = eNB_UE_stats == NULL ? -1 : eNB_UE_stats->DL_cqi[0];
      LOG_I(MAC,"UE  rnti %x : %s, PHR %d dB CQI %d\n", rnti,
            UE_list->UE_sched_ctrl[i].ul_out_of_sync==0 ? "in synch" : "out of sync",
            UE_list->UE_template[CC_id][i].phr_info,
            cqi);
    }

    PHY_vars_eNB_g[module_idP][CC_id]->pusch_stats_bsr[i][(frameP*10)+subframeP]=-63;
    if (i==UE_list->head)
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BSR,PHY_vars_eNB_g[module_idP][CC_id]->pusch_stats_bsr[i][(frameP*10)+subframeP]); 
    // increment this, it is cleared when we receive an sdu
    eNB_mac_inst[module_idP].UE_list.UE_sched_ctrl[i].ul_inactivity_timer++;

    eNB_mac_inst[module_idP].UE_list.UE_sched_ctrl[i].cqi_req_timer++;
    eNB_UE_stats = mac_xface->get_eNB_UE_stats(module_idP,CC_id,rnti);

    if (eNB_UE_stats==NULL) {
	//mac_remove_ue(module_idP, i, frameP, subframeP);
      //Inform the controller about the UE deactivation. Should be moved to RRC agent in the future
#if defined(FLEXRAN_AGENT_SB_IF)
      if (mac_agent_registered[module_idP]) {
	agent_mac_xface[module_idP]->flexran_agent_notify_ue_state_change(module_idP,
									  rnti,
									  PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED);
      }
#endif
    }
    else {
      // check uplink failure
      if ((UE_list->UE_sched_ctrl[i].ul_failure_timer>0)&&
	  (UE_list->UE_sched_ctrl[i].ul_out_of_sync==0)) {
	LOG_D(MAC,"UE %d rnti %x: UL Failure timer %d \n",i,rnti,UE_list->UE_sched_ctrl[i].ul_failure_timer);
	if (UE_list->UE_sched_ctrl[i].ra_pdcch_order_sent==0) {
	  UE_list->UE_sched_ctrl[i].ra_pdcch_order_sent=1;
	  
	  // add a format 1A dci for this UE to request an RA procedure (only one UE per subframe)
	  LOG_D(MAC,"UE %d rnti %x: sending PDCCH order for RAPROC (failure timer %d) \n",i,rnti,UE_list->UE_sched_ctrl[i].ul_failure_timer);	    
	  DLSCH_dci = (void *)UE_list->UE_template[CC_id][i].DLSCH_DCI[0];
	  *(uint32_t*)DLSCH_dci = 0;
	  if (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.frame_type == TDD) {
	    switch (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
	    case 6:
	      ((DCI1A_1_5MHz_TDD_1_6_t*)DLSCH_dci)->type = 1;
	      ((DCI1A_1_5MHz_TDD_1_6_t*)DLSCH_dci)->rballoc = 31;
	      size_bytes = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
	      size_bits  = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
	      break;
	    case 25:
	      ((DCI1A_5MHz_TDD_1_6_t*)DLSCH_dci)->type = 1;
	      ((DCI1A_5MHz_TDD_1_6_t*)DLSCH_dci)->rballoc = 511;
	      size_bytes = sizeof(DCI1A_5MHz_TDD_1_6_t);
	      size_bits  = sizeof_DCI1A_5MHz_TDD_1_6_t;
	      break;
	    case 50:
	      ((DCI1A_10MHz_TDD_1_6_t*)DLSCH_dci)->type = 1;
	      ((DCI1A_10MHz_TDD_1_6_t*)DLSCH_dci)->rballoc = 2047;
	      size_bytes = sizeof(DCI1A_10MHz_TDD_1_6_t);
	      size_bits  = sizeof_DCI1A_10MHz_TDD_1_6_t;
	      break;
	    case 100:
	      ((DCI1A_20MHz_TDD_1_6_t*)DLSCH_dci)->type = 1;
	      ((DCI1A_20MHz_TDD_1_6_t*)DLSCH_dci)->rballoc = 8191;
	      size_bytes = sizeof(DCI1A_20MHz_TDD_1_6_t);
	      size_bits  = sizeof_DCI1A_20MHz_TDD_1_6_t;
	      break;
	    }
	  }
	  else { // FDD
	    switch (PHY_vars_eNB_g[module_idP][CC_id]->frame_parms.N_RB_DL) {
	    case 6:
	      ((DCI1A_1_5MHz_FDD_t*)DLSCH_dci)->type = 1;
	      ((DCI1A_1_5MHz_FDD_t*)DLSCH_dci)->rballoc = 31;
	      size_bytes = sizeof(DCI1A_1_5MHz_FDD_t);
	      size_bits  = sizeof_DCI1A_1_5MHz_FDD_t;
	      break;
	    case 15:/*
	      ((DCI1A_2_5MHz_FDD_t*)DLSCH_dci)->type = 1;
	      ((DCI1A_2_5MHz_FDD_t*)DLSCH_dci)->rballoc = 31;
	      size_bytes = sizeof(DCI1A_1_5MHz_FDD_t);
	      size_bits  = sizeof_DCI1A_1_5MHz_FDD_t;*/
	      break;
	    case 25:
	      ((DCI1A_5MHz_FDD_t*)DLSCH_dci)->type = 1;
	      ((DCI1A_5MHz_FDD_t*)DLSCH_dci)->rballoc = 511;
	      size_bytes = sizeof(DCI1A_5MHz_FDD_t);
	      size_bits  = sizeof_DCI1A_5MHz_FDD_t;
	      break;
	    case 50:
	      ((DCI1A_10MHz_FDD_t*)DLSCH_dci)->type = 1;
	      ((DCI1A_10MHz_FDD_t*)DLSCH_dci)->rballoc = 2047;
	      size_bytes = sizeof(DCI1A_10MHz_FDD_t);
	      size_bits  = sizeof_DCI1A_10MHz_FDD_t;
		break;
	    case 75:
	      /*	      ((DCI1A_15MHz_FDD_t*)DLSCH_dci)->type = 1;
	      ((DCI1A_15MHz_FDD_t*)DLSCH_dci)->rballoc = 2047;
	      size_bytes = sizeof(DCI1A_10MHz_FDD_t);
	      size_bits  = sizeof_DCI1A_10MHz_FDD_t;*/
		break;
	    case 100:
	      ((DCI1A_20MHz_FDD_t*)DLSCH_dci)->type = 1;
	      ((DCI1A_20MHz_FDD_t*)DLSCH_dci)->rballoc = 8191;
	      size_bytes = sizeof(DCI1A_20MHz_FDD_t);
	      size_bits  = sizeof_DCI1A_20MHz_FDD_t;
	      break;
	    }
	  }
	  
	  add_ue_spec_dci(DCI_pdu[CC_id],
			  DLSCH_dci,
			  rnti,
			  size_bytes,
			  get_aggregation(get_bw_index(module_idP,CC_id),eNB_UE_stats->DL_cqi[0],format1A),
			  size_bits,
			  format1A,
			  0);
	}
	else { // ra_pdcch_sent==1
	  LOG_D(MAC,"UE %d rnti %x: sent PDCCH order for RAPROC waiting (failure timer %d) \n",i,rnti,UE_list->UE_sched_ctrl[i].ul_failure_timer);	    	    
	  if ((UE_list->UE_sched_ctrl[i].ul_failure_timer % 40) == 0)
	    UE_list->UE_sched_ctrl[i].ra_pdcch_order_sent=0; // resend every 4 frames	      
	}
      
	UE_list->UE_sched_ctrl[i].ul_failure_timer++;
	// check threshold
	if (UE_list->UE_sched_ctrl[i].ul_failure_timer > 200) {
	  // inform RRC of failure and clear timer
	  LOG_I(MAC,"UE %d rnti %x: UL Failure after repeated PDCCH orders: Triggering RRC \n",i,rnti);
	  mac_eNB_rrc_ul_failure(module_idP,CC_id,frameP,subframeP,rnti);
	  UE_list->UE_sched_ctrl[i].ul_failure_timer=0;
	  UE_list->UE_sched_ctrl[i].ul_out_of_sync=1;
	}
      }
    } // ul_failure_timer>0
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
  
  //if (subframeP%5 == 0)
  //#ifdef EXMIMO
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES, NOT_A_RNTI, frameP, subframeP,module_idP);
  pdcp_run(&ctxt);
  //#endif

  // check HO
  rrc_rx_tx(&ctxt,
            0, // eNB index, unused in eNB
            CC_id);

#if defined(Rel10) || defined(Rel14)

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    if (eNB_mac_inst[module_idP].common_channels[CC_id].MBMS_flag >0) {
      start_meas(&eNB_mac_inst[module_idP].schedule_mch);
      mbsfn_status[CC_id] = schedule_MBMS(module_idP,CC_id,frameP,subframeP);
      stop_meas(&eNB_mac_inst[module_idP].schedule_mch);
    }
  }

#endif
  // refresh UE list based on UEs dropped by PHY in previous subframe
  /*
  i=UE_list->head;
  while (i>=0) {
    next_i = UE_list->next[i];
    LOG_T(MAC,"UE %d : rnti %x, stats %p\n",i,UE_RNTI(module_idP,i),mac_xface->get_eNB_UE_stats(module_idP,0,UE_RNTI(module_idP,i)));
    if (mac_xface->get_eNB_UE_stats(module_idP,0,UE_RNTI(module_idP,i))==NULL) {
      mac_remove_ue(module_idP,i,frameP);
    }
    i=next_i;
  }
  */

  switch (subframeP) {
  case 0:

    // FDD/TDD Schedule Downlink RA transmissions (RA response, Msg4 Contention resolution)
    // Schedule ULSCH for FDD or subframeP 4 (TDD config 0,3,6)
    // Schedule Normal DLSCH


    schedule_RA(module_idP,frameP,subframeP,2);


    if (mac_xface->frame_parms->frame_type == FDD) {  //FDD
      schedule_ulsch(module_idP,frameP,cooperation_flag,0,4);//,calibration_flag);
    } else if  ((mac_xface->frame_parms->tdd_config == 0) || //TDD
                (mac_xface->frame_parms->tdd_config == 3) ||
                (mac_xface->frame_parms->tdd_config == 6)) {
      schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,4);//,calibration_flag);
    }
#ifndef FLEXRAN_AGENT_SB_IF
    schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
    fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
    if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
    break;

  case 1:

    // TDD, schedule UL for subframeP 7 (TDD config 0,1) / subframeP 8 (TDD Config 6)
    // FDD, schedule normal UL/DLSCH
    if (mac_xface->frame_parms->frame_type == TDD) { // TDD
      switch (mac_xface->frame_parms->tdd_config) {
      case 0:
      case 1:
        schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,7);
#ifndef FLEXRAN_AGENT_SB_IF
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#endif
        break;

      case 6:
        schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,8);
#ifndef FLEXRAN_AGENT_SB_IF
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#endif
        break;

      default:
        break;
      }
    } else { //FDD
      schedule_ulsch(module_idP,frameP,cooperation_flag,1,5);
#ifndef FLEXRAN_AGENT_SB_IF
      schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
      fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
      if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
    }

    break;

  case 2:

    // TDD, nothing
    // FDD, normal UL/DLSCH
    if (mac_xface->frame_parms->frame_type == FDD) {  //FDD
      schedule_ulsch(module_idP,frameP,cooperation_flag,2,6);
#ifndef FLEXRAN_AGENT_SB_IF
      schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
      fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
      if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
    }

    break;

  case 3:

    // TDD Config 2, ULSCH for subframeP 7
    // TDD Config 2/5 normal DLSCH
    // FDD, normal UL/DLSCH
    if (mac_xface->frame_parms->frame_type == TDD) {
      switch (mac_xface->frame_parms->tdd_config) {
      case 2:
        schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,7);

        // no break here!
      case 5:
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
        break;

      default:
        break;
      }
    } else { //FDD

      schedule_ulsch(module_idP,frameP,cooperation_flag,3,7);
#ifndef FLEXRAN_AGENT_SB_IF
      schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
      fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
      if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
      }
#endif
    }

    break;

  case 4:

    // TDD Config 1, ULSCH for subframeP 8
    // TDD Config 1/2/4/5 DLSCH
    // FDD UL/DLSCH
    if (mac_xface->frame_parms->frame_type == 1) { // TDD
      switch (mac_xface->frame_parms->tdd_config) {
      case 1:
        //        schedule_RA(module_idP,frameP,subframeP);
        schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,8);

        // no break here!
      case 2:

        // no break here!
      case 4:

        // no break here!
      case 5:
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
	fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
					       frameP,
					       subframeP,
					       mbsfn_status,
					       msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif	
        break;

      default:
        break;
      }
    } else {
      if (mac_xface->frame_parms->frame_type == FDD) {  //FDD

	schedule_ulsch(module_idP, frameP, cooperation_flag, 4, 8);
#ifndef FLEXRAN_AGENT_SB_IF
	schedule_ue_spec(module_idP, frameP, subframeP,  mbsfn_status);
        fill_DLSCH_dci(module_idP, frameP, subframeP,   mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}     
#endif
      }
    }

    break;

  case 5:
    // TDD/FDD Schedule SI
    // TDD Config 0,6 ULSCH for subframes 9,3 resp.
    // TDD normal DLSCH
    // FDD normal UL/DLSCH
    schedule_SI(module_idP,frameP,subframeP);

    //schedule_RA(module_idP,frameP,subframeP,5);
    if (mac_xface->frame_parms->frame_type == FDD) {
      schedule_RA(module_idP,frameP,subframeP,1);
      schedule_ulsch(module_idP,frameP,cooperation_flag,5,9);
#ifndef FLEXRAN_AGENT_SB_IF
      schedule_ue_spec(module_idP, frameP, subframeP,  mbsfn_status);
      fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
      if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
					  frameP,
					  subframeP,
					  mbsfn_status,
					  msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
    } else if ((mac_xface->frame_parms->tdd_config == 0) || // TDD Config 0
               (mac_xface->frame_parms->tdd_config == 6)) { // TDD Config 6
      //schedule_ulsch(module_idP,cooperation_flag,subframeP);
#ifndef FLEXRAN_AGENT_SB_IF
      fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#endif
    } else {
#ifndef FLEXRAN_AGENT_SB_IF
      schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
      fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
      if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
    }

    break;

  case 6:

    // TDD Config 0,1,6 ULSCH for subframes 2,3
    // TDD Config 3,4,5 Normal DLSCH
    // FDD normal ULSCH/DLSCH
    if (mac_xface->frame_parms->frame_type == TDD) { // TDD
      switch (mac_xface->frame_parms->tdd_config) {
      case 0:
        break;

      case 1:
        schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,2);
        //  schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
#ifndef FLEXRAN_AGENT_SB_IF
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#endif
        break;

      case 6:
        schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,3);
        //  schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
#ifndef FLEXRAN_AGENT_SB_IF
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#endif
        break;

      case 5:
        schedule_RA(module_idP,frameP,subframeP,2);
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
        break;

      case 3:
      case 4:
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
        break;

      default:
        break;
      }
    } else { //FDD
      schedule_ulsch(module_idP,frameP,cooperation_flag,6,0);
#ifndef FLEXRAN_AGENT_SB_IF
      schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
      fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
      if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
    }

    break;

  case 7:

    // TDD Config 3,4,5 Normal DLSCH
    // FDD Normal UL/DLSCH
    if (mac_xface->frame_parms->frame_type == TDD) { // TDD
      switch (mac_xface->frame_parms->tdd_config) {
      case 3:
      case 4:
        schedule_RA(module_idP,frameP,subframeP,3);  // 3 = Msg3 subframeP, not
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
        break;

      case 5:
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
        break;

      default:
        break;
      }
    } else { //FDD
      schedule_ulsch(module_idP,frameP,cooperation_flag,7,1);
#ifndef FLEXRAN_AGENT_SB_IF
      schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
      fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
      if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
    }

    break;

  case 8:

    // TDD Config 2,3,4,5 ULSCH for subframeP 2
    //
    // FDD Normal UL/DLSCH
    if (mac_xface->frame_parms->frame_type == TDD) { // TDD
      switch (mac_xface->frame_parms->tdd_config) {
      case 2:
      case 3:
      case 4:
      case 5:

        //  schedule_RA(module_idP,subframeP);
        schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,2);
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
        break;

      default:
        break;
      }
    } else { //FDD
      schedule_ulsch(module_idP,frameP,cooperation_flag,8,2);
#ifndef FLEXRAN_AGENT_SB_IF
      schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
      fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
      if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
    }

    break;

  case 9:

    // TDD Config 1,3,4,6 ULSCH for subframes 3,3,3,4
    if (mac_xface->frame_parms->frame_type == TDD) {
      switch (mac_xface->frame_parms->tdd_config) {
      case 1:
        schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,3);
        schedule_RA(module_idP,frameP,subframeP,7);  // 7 = Msg3 subframeP, not
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
        break;

      case 3:
      case 4:
        schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,3);
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
        break;

      case 6:
        schedule_ulsch(module_idP,frameP,cooperation_flag,subframeP,4);
        //schedule_RA(module_idP,frameP,subframeP);
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
        break;

      case 2:
      case 5:
        //schedule_RA(module_idP,frameP,subframeP);
#ifndef FLEXRAN_AGENT_SB_IF
        schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
        fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
	if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
	}
#endif
        break;

      default:
        break;
      }
    } else { //FDD
      schedule_ulsch(module_idP,frameP,cooperation_flag,9,3);
#ifndef FLEXRAN_AGENT_SB_IF
      schedule_ue_spec(module_idP,frameP,subframeP,mbsfn_status);
      fill_DLSCH_dci(module_idP,frameP,subframeP,mbsfn_status);
#else
      if (mac_agent_registered[module_idP]) {                                  
	  agent_mac_xface[module_idP]->flexran_agent_schedule_ue_spec(
								      module_idP,
								      frameP,                  
								      subframeP,
								      mbsfn_status,
								      &msg);
	  
	  flexran_apply_dl_scheduling_decisions(module_idP,
						frameP,
						subframeP,
						mbsfn_status,
						msg);
	  flexran_agent_mac_destroy_dl_config(msg);
      }
#endif
    }

    break;

  }

  LOG_D(MAC,"FrameP %d, subframeP %d : Scheduling CCEs\n",frameP,subframeP);

  // Allocate CCEs for good after scheduling is done
  for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++)
    allocate_CCEs(module_idP,CC_id,subframeP,0);

#if defined(FLEXRAN_AGENT_SB_IF)
#ifndef DISABLE_CONT_STATS
  //Send subframe trigger to the controller
  if (mac_agent_registered[module_idP]) {
    agent_mac_xface[module_idP]->flexran_agent_send_update_mac_stats(module_idP);
  }
#endif
#endif

  /*
  int dummy=0;
  for (i=0;
       i<DCI_pdu[CC_id]->Num_common_dci+DCI_pdu[CC_id]->Num_ue_spec_dci;
       i++)
    if (DCI_pdu[CC_id]->dci_alloc[i].rnti==2)
      dummy=1;
	
  if (dummy==1)
    for (i=0;
	 i<DCI_pdu[CC_id]->Num_common_dci+DCI_pdu[CC_id]->Num_ue_spec_dci;
	 i++)
      LOG_I(MAC,"Frame %d, subframe %d: DCI %d/%d, format %d, rnti %x, NCCE %d(num_pdcch_symb %d)\n",
	    frameP,subframeP,i,DCI_pdu[CC_id]->Num_common_dci+DCI_pdu[CC_id]->Num_ue_spec_dci,
	    DCI_pdu[CC_id]->dci_alloc[i].format,
	    DCI_pdu[CC_id]->dci_alloc[i].rnti,
	    DCI_pdu[CC_id]->dci_alloc[i].firstCCE,
	    DCI_pdu[CC_id]->num_pdcch_symbols);

  LOG_D(MAC,"frameP %d, subframeP %d\n",frameP,subframeP);
  */

  stop_meas(&eNB_mac_inst[module_idP].eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_OUT);

}



