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

/*! \file rrc_common.c
 * \brief rrc common procedures for eNB and UE
 * \author Navid Nikaein and Raymond Knopp
 * \date 2011 - 2014
 * \version 1.0
 * \company Eurecom
 * \email:  navid.nikaein@eurecom.fr and raymond.knopp@eurecom.fr
 */

#include "defs.h"
#include "extern.h"
#include "LAYER2/MAC/extern.h"
#include "COMMON/openair_defs.h"
#include "COMMON/platform_types.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "LAYER2/RLC/rlc.h"
#include "COMMON/mac_rrc_primitives.h"
#include "UTIL/LOG/log.h"
#include "asn1_msg.h"
#include "pdcp.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "rrc_eNB_UE_context.h"
#include "common/ran_context.h"

#ifdef LOCALIZATION
#include <sys/time.h>
#endif

#define DEBUG_RRC 1
extern RAN_CONTEXT_t RC;
extern UE_MAC_INST *UE_mac_inst;

extern mui_t rrc_eNB_mui;

//configure  BCCH & CCCH Logical Channels and associated rrc_buffers, configure associated SRBs
//-----------------------------------------------------------------------------
void
openair_rrc_on(
  const protocol_ctxt_t* const ctxt_pP
)
//-----------------------------------------------------------------------------
{
  unsigned short i;
  int            CC_id;

  if (ctxt_pP->enb_flag == ENB_FLAG_YES) {
    LOG_I(RRC, PROTOCOL_RRC_CTXT_FMT" OPENAIR RRC IN....\n",
          PROTOCOL_RRC_CTXT_ARGS(ctxt_pP));
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
      rrc_config_buffer (&RC.rrc[ctxt_pP->module_id]->carrier[CC_id].SI, BCCH, 1);
      RC.rrc[ctxt_pP->module_id]->carrier[CC_id].SI.Active = 1;
      rrc_config_buffer (&RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0, CCCH, 1);
      RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Active = 1;
    }
  } else {
    LOG_I(RRC, PROTOCOL_RRC_CTXT_FMT" OPENAIR RRC IN....\n",
          PROTOCOL_RRC_CTXT_ARGS(ctxt_pP));

    for (i = 0; i < NB_eNB_INST; i++) {
      LOG_D(RRC, PROTOCOL_RRC_CTXT_FMT" Activating CCCH (eNB %d)\n",
            PROTOCOL_RRC_CTXT_ARGS(ctxt_pP), i);
      UE_rrc_inst[ctxt_pP->module_id].Srb0[i].Srb_id = CCCH;
      memcpy (&UE_rrc_inst[ctxt_pP->module_id].Srb0[i].Lchan_desc[0], &CCCH_LCHAN_DESC, LCHAN_DESC_SIZE);
      memcpy (&UE_rrc_inst[ctxt_pP->module_id].Srb0[i].Lchan_desc[1], &CCCH_LCHAN_DESC, LCHAN_DESC_SIZE);
      rrc_config_buffer (&UE_rrc_inst[ctxt_pP->module_id].Srb0[i], CCCH, 1);
      UE_rrc_inst[ctxt_pP->module_id].Srb0[i].Active = 1;
    }
  }
}

//-----------------------------------------------------------------------------
int
rrc_init_global_param(
  void
)
//-----------------------------------------------------------------------------
{

  //#ifdef USER_MODE
  //  Rrc_xface = (RRC_XFACE*)malloc16(sizeof(RRC_XFACE));
  //#endif //USRE_MODE

  //  Rrc_xface->openair_rrc_top_init = openair_rrc_top_init;
  //  Rrc_xface->openair_rrc_eNB_init = openair_rrc_eNB_init;
  //  Rrc_xface->openair_rrc_UE_init  = openair_rrc_ue_init;
  //  Rrc_xface->mac_rrc_data_ind     = mac_rrc_data_ind;
  //Rrc_xface->mac_rrc_data_req     = mac_rrc_data_req;
  // Rrc_xface->rrc_data_indP        = (void *)rlcrrc_data_ind;
  //  Rrc_xface->rrc_rx_tx            = rrc_rx_tx;
  //  Rrc_xface->mac_rrc_meas_ind     = mac_rrc_meas_ind;
  //  Rrc_xface->get_rrc_status       = get_rrc_status;

  //Rrc_xface->rrc_get_status = ...

  //  Mac_rlc_xface->mac_out_of_sync_ind=mac_out_of_sync_ind;

#ifndef NO_RRM
  //  Rrc_xface->fn_rrc=fn_rrc;
#endif
  //  LOG_D(RRC, "[RRC]INIT_GLOBAL_PARAM: Mac_rlc_xface %p, rrc_rlc_register %p,rlcrrc_data_ind%p\n",Mac_rlc_xface,Mac_rlc_xface->rrc_rlc_register_rrc,rlcrrc_data_ind);
  /*
   if((Mac_rlc_xface==NULL) || (Mac_rlc_xface->rrc_rlc_register_rrc==NULL) ||
   (rlcrrc_data_ind==NULL)) {
   LOG_E(RRC,"Data structured is not initialized \n");
   return -1;
   }
   */
  rrc_rlc_register_rrc (rrc_data_ind, NULL); //register with rlc

  DCCH_LCHAN_DESC.transport_block_size = 4;
  DCCH_LCHAN_DESC.max_transport_blocks = 16;
  DCCH_LCHAN_DESC.Delay_class = 1;
  DTCH_DL_LCHAN_DESC.transport_block_size = 52;
  DTCH_DL_LCHAN_DESC.max_transport_blocks = 20;
  DTCH_DL_LCHAN_DESC.Delay_class = 1;
  DTCH_UL_LCHAN_DESC.transport_block_size = 52;
  DTCH_UL_LCHAN_DESC.max_transport_blocks = 20;
  DTCH_UL_LCHAN_DESC.Delay_class = 1;

  Rlc_info_um.rlc_mode = RLC_MODE_UM;
  Rlc_info_um.rlc.rlc_um_info.timer_reordering = 5;
  Rlc_info_um.rlc.rlc_um_info.sn_field_length = 10;
  Rlc_info_um.rlc.rlc_um_info.is_mXch = 0;
  //Rlc_info_um.rlc.rlc_um_info.sdu_discard_mode=16;

  Rlc_info_am_config.rlc_mode = RLC_MODE_AM;
  Rlc_info_am_config.rlc.rlc_am_info.max_retx_threshold = 50;
  Rlc_info_am_config.rlc.rlc_am_info.poll_pdu = 8;
  Rlc_info_am_config.rlc.rlc_am_info.poll_byte = 1000;
  Rlc_info_am_config.rlc.rlc_am_info.t_poll_retransmit = 15;
  Rlc_info_am_config.rlc.rlc_am_info.t_reordering = 50;
  Rlc_info_am_config.rlc.rlc_am_info.t_status_prohibit = 10;

  return 0;
}

//-----------------------------------------------------------------------------
void
rrc_config_buffer(
  SRB_INFO* Srb_info,
  uint8_t Lchan_type,
  uint8_t Role
)
//-----------------------------------------------------------------------------
{

  Srb_info->Rx_buffer.payload_size = 0;
  Srb_info->Tx_buffer.payload_size = 0;
}


//-----------------------------------------------------------------------------
void
rrc_t310_expiration(
  const protocol_ctxt_t* const ctxt_pP,
  const uint8_t                 eNB_index
)
//-----------------------------------------------------------------------------
{

  if (UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State != RRC_CONNECTED) {
    LOG_D(RRC, "Timer 310 expired, going to RRC_IDLE\n");
    UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].State = RRC_IDLE;
    UE_rrc_inst[ctxt_pP->module_id].Info[eNB_index].UE_index = 0xffff;
    UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Rx_buffer.payload_size = 0;
    UE_rrc_inst[ctxt_pP->module_id].Srb0[eNB_index].Tx_buffer.payload_size = 0;
    UE_rrc_inst[ctxt_pP->module_id].Srb1[eNB_index].Srb_info.Rx_buffer.payload_size = 0;
    UE_rrc_inst[ctxt_pP->module_id].Srb1[eNB_index].Srb_info.Tx_buffer.payload_size = 0;

    if (UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Active == 1) {
      msg ("[RRC Inst %d] eNB_index %d, Remove RB %d\n ", ctxt_pP->module_id, eNB_index,
           UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Srb_info.Srb_id);
      rrc_pdcp_config_req (ctxt_pP,
                           SRB_FLAG_YES,
                           CONFIG_ACTION_REMOVE,
                           UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Srb_info.Srb_id,
                           0);
      rrc_rlc_config_req (ctxt_pP,
                          SRB_FLAG_YES,
                          MBMS_FLAG_NO,
                          CONFIG_ACTION_REMOVE,
                          UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Srb_info.Srb_id,
                          Rlc_info_um);
      UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Active = 0;
      UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Status = IDLE;
      UE_rrc_inst[ctxt_pP->module_id].Srb2[eNB_index].Next_check_frame = 0;
    }
  } else { // Restablishment procedure
    LOG_D(RRC, "Timer 310 expired, trying RRCRestablishment ...\n");
  }
}

//-----------------------------------------------------------------------------
RRC_status_t
rrc_rx_tx(
  protocol_ctxt_t* const ctxt_pP,
  const uint8_t      enb_indexP,
  const int          CC_id
)
//-----------------------------------------------------------------------------
{
  //uint8_t        UE_id;
  int32_t        current_timestamp_ms, ref_timestamp_ms;
  struct timeval ts;
  struct rrc_eNB_ue_context_s   *ue_context_p = NULL,*ue_to_be_removed = NULL;

#ifdef LOCALIZATION
  double                         estimated_distance;
  protocol_ctxt_t                ctxt;
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_IN);

  if(ctxt_pP->enb_flag == ENB_FLAG_NO) {
    // check timers

    if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_active == 1) {
      if ((UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_cnt % 10) == 0)
        LOG_D(RRC,
              "[UE %d][RAPROC] Frame %d T300 Count %d ms\n", ctxt_pP->module_id, ctxt_pP->frame, UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_cnt);

      if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_cnt
          == T300[UE_rrc_inst[ctxt_pP->module_id].sib2[enb_indexP]->ue_TimersAndConstants.t300]) {
        UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_active = 0;
        // ALLOW CCCH to be used
        UE_rrc_inst[ctxt_pP->module_id].Srb0[enb_indexP].Tx_buffer.payload_size = 0;
        rrc_ue_generate_RRCConnectionRequest (ctxt_pP, enb_indexP);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
        return (RRC_ConnSetup_failed);
      }

      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T300_cnt++;
    }

    if ((UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].SIStatus&2)>0) {
      if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].N310_cnt
          == N310[UE_rrc_inst[ctxt_pP->module_id].sib2[enb_indexP]->ue_TimersAndConstants.n310]) {
	LOG_I(RRC,"Activating T310\n");
        UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_active = 1;
      }
    } else { // in case we have not received SIB2 yet
      /*      if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].N310_cnt == 100) {
        UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].N310_cnt = 0;

	}*/
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
      return RRC_OK;
    }

    if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_active == 1) {
      if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].N311_cnt
          == N311[UE_rrc_inst[ctxt_pP->module_id].sib2[enb_indexP]->ue_TimersAndConstants.n311]) {
        UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_active = 0;
        UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].N311_cnt = 0;
      }

      if ((UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_cnt % 10) == 0) {
        LOG_D(RRC, "[UE %d] Frame %d T310 Count %d ms\n", ctxt_pP->module_id, ctxt_pP->frame, UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_cnt);
      }

      if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_cnt    == T310[UE_rrc_inst[ctxt_pP->module_id].sib2[enb_indexP]->ue_TimersAndConstants.t310]) {
        UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_active = 0;
        rrc_t310_expiration (ctxt_pP, enb_indexP);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
	LOG_I(RRC,"Returning RRC_PHY_RESYNCH: T310 expired\n");
        return RRC_PHY_RESYNCH;
      }

      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T310_cnt++;
    }

    if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_active==1) {
      if ((UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_cnt % 10) == 0)
        LOG_D(RRC,"[UE %d][RAPROC] Frame %d T304 Count %d ms\n",ctxt_pP->module_id,ctxt_pP->frame,
              UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_cnt);

      if (UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_cnt == 0) {
        UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_active = 0;
        UE_rrc_inst[ctxt_pP->module_id].HandoverInfoUe.measFlag = 1;
        LOG_E(RRC,"[UE %d] Handover failure..initiating connection re-establishment procedure... \n",
              ctxt_pP->module_id);
        //Implement 36.331, section 5.3.5.6 here
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
        return(RRC_Handover_failed);
      }

      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].T304_cnt--;
    }

    // Layer 3 filtering of RRC measurements
    if (UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[0] != NULL) {
      ue_meas_filtering(ctxt_pP,enb_indexP);
    }

    ue_measurement_report_triggering(ctxt_pP,enb_indexP);

    if (UE_rrc_inst[ctxt_pP->module_id].Info[0].handoverTarget > 0) {
      LOG_I(RRC,"[UE %d] Frame %d : RRC handover initiated\n", ctxt_pP->module_id, ctxt_pP->frame);
    }

    if((UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].State == RRC_HO_EXECUTION)   &&
        (UE_rrc_inst[ctxt_pP->module_id].HandoverInfoUe.targetCellId != 0xFF)) {
      UE_rrc_inst[ctxt_pP->module_id].Info[enb_indexP].State= RRC_IDLE;
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
      return(RRC_HO_STARTED);
    }

  } else { // eNB
    check_handovers(ctxt_pP);
    // counetr, and get the value and aggregate


    // check for UL failure
    RB_FOREACH(ue_context_p, rrc_ue_tree_s, &(RC.rrc[ctxt_pP->module_id]->rrc_ue_head)) {
      LOG_D(RRC,"SFN.SN %d.%d => release timer %d/%d\n",ctxt_pP->frame,ctxt_pP->subframe,
	    ue_context_p->ue_context.ue_release_timer,ue_context_p->ue_context.ue_release_timer_thres);
      if ((ctxt_pP->frame == 0) && (ctxt_pP->subframe==0)) {
	if (ue_context_p->ue_context.Initialue_identity_s_TMSI.presence == TRUE) {
	  LOG_I(RRC,"UE rnti %x:S-TMSI %x failure timer %d/20000\n",
		ue_context_p->ue_context.rnti,
		ue_context_p->ue_context.Initialue_identity_s_TMSI.m_tmsi,
		ue_context_p->ue_context.ul_failure_timer);
	}
	else {
	  LOG_I(RRC,"UE rnti %x failure timer %d/20000\n",
		ue_context_p->ue_context.rnti,
		ue_context_p->ue_context.ul_failure_timer);
	}
      }
      if (ue_context_p->ue_context.ul_failure_timer>0) {
	ue_context_p->ue_context.ul_failure_timer++;
	if (ue_context_p->ue_context.ul_failure_timer >= 20000) {
	  // remove UE after 20 seconds after MAC has indicated UL failure
	  LOG_I(RRC,"Removing UE %x instance (failure)\n",ue_context_p->ue_context.rnti);
	  ue_to_be_removed = ue_context_p;
	  break;
	}
      }
      if (ue_context_p->ue_context.ue_release_timer>0) {
	ue_context_p->ue_context.ue_release_timer++;
	if (ue_context_p->ue_context.ue_release_timer >=
	    ue_context_p->ue_context.ue_release_timer_thres) {
	  LOG_I(RRC,"Removing UE %x instance (release timer %d)\n",ue_context_p->ue_context.rnti,ue_context_p->ue_context.ue_release_timer);
	  ue_to_be_removed = ue_context_p;
	  exit(-1);
	  break;
	}
      }
    }
    if (ue_to_be_removed)
      rrc_eNB_free_UE(ctxt_pP->module_id,ue_to_be_removed);

#ifdef RRC_LOCALIZATION

    /* for the localization, only primary CC_id might be relevant*/
    gettimeofday(&ts, NULL);
    current_timestamp_ms = ts.tv_sec * 1000 + ts.tv_usec / 1000;
    ref_timestamp_ms = RC.rrc[ctxt_pP->module_id]->reference_timestamp_ms;
    RB_FOREACH(ue_context_p, rrc_ue_tree_s, &(RC.rrc[ctxt_pP->module_id]->rrc_ue_head)) {
      ctxt = *ctxt_pP;
      ctxt.rnti = ue_context_p->ue_context.rnti;
      estimated_distance = rrc_get_estimated_ue_distance(
                             &ctxt,
                             CC_id,
                             RC.rrc[ctxt_pP->module_id]->loc_type);

      if ((current_timestamp_ms - ref_timestamp_ms > RC.rrc[ctxt_pP->module_id]->aggregation_period_ms) &&
          estimated_distance != -1) {
        LOG_D(LOCALIZE, " RRC [UE/id %d -> eNB/id %d] timestamp %d frame %d estimated r = %f\n",
              ctxt.rnti,
              ctxt_pP->module_id,
              current_timestamp_ms,
              ctxt_pP->frame,
              estimated_distance);
        LOG_D(LOCALIZE, " RRC status %d\n", ue_context_p->ue_context.StatusRrc);
        push_front(&RC.rrc[ctxt_pP->module_id]->loc_list,
                   estimated_distance);
        RC.rrc[ctxt_pP->module_id]->reference_timestamp_ms = current_timestamp_ms;
      }
    }

#endif
    (void)ts; /* remove gcc warning "unused variable" */
    (void)ref_timestamp_ms; /* remove gcc warning "unused variable" */
    (void)current_timestamp_ms; /* remove gcc warning "unused variable" */
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_RX_TX,VCD_FUNCTION_OUT);
  return (RRC_OK);
}

//-----------------------------------------------------------------------------
long
binary_search_int(
  int elements[],
  long numElem,
  int value
)
//-----------------------------------------------------------------------------
{
  long first, last, middle, search = -1;
  first = 0;
  last = numElem-1;
  middle = (first+last)/2;

  if(value < elements[0]) {
    return first;
  }

  if(value > elements[last]) {
    return last;
  }

  while (first <= last) {
    if (elements[middle] < value) {
      first = middle+1;
    } else if (elements[middle] == value) {
      search = middle+1;
      break;
    } else {
      last = middle -1;
    }

    middle = (first+last)/2;
  }

  if (first > last) {
    LOG_E(RRC,"Error in binary search!");
  }

  return search;
}

/* This is a binary search routine which operates on an array of floating
   point numbers and returns the index of the range the value lies in
   Used for RSRP and RSRQ measurement mapping. Can potentially be used for other things
*/
//-----------------------------------------------------------------------------
long
binary_search_float(
  float elements[],
  long numElem,
  float value
)
//-----------------------------------------------------------------------------
{
  long first, last, middle;
  first = 0;
  last = numElem-1;
  middle = (first+last)/2;

  if(value <= elements[0]) {
    return first;
  }

  if(value >= elements[last]) {
    return last;
  }

  while (last - first > 1) {
    if (elements[middle] > value) {
      last = middle;
    } else {
      first = middle;
    }

    middle = (first+last)/2;
  }

  if (first < 0 || first >= numElem) {
    LOG_E(RRC,"\n Error in binary search float!");
  }

  return first;
}
