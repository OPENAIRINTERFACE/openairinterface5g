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

/*! \file rrc_eNB_GTPV1U.c
 * \brief rrc GTPV1U procedures for eNB
 * \author Lionel GAUTHIER
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */

# include "rrc_defs.h"
# include "rrc_extern.h"
# include "RRC/LTE/MESSAGES/asn1_msg.h"
# include "rrc_eNB_GTPV1U.h"
# include "rrc_eNB_UE_context.h"


#include "oai_asn1.h"
#include "intertask_interface.h"
#include "pdcp.h"


# include "common/ran_context.h"
#include <openair3/ocp-gtpu/gtp_itf.h>

extern RAN_CONTEXT_t RC;

int
rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  const gtpv1u_enb_create_tunnel_resp_t *const create_tunnel_resp_pP,
  uint8_t                         *inde_list
) {
  rnti_t                         rnti;
  int                            i;
  struct rrc_eNB_ue_context_s   *ue_context_p = NULL;

  if (create_tunnel_resp_pP) {
    LOG_D(RRC, PROTOCOL_RRC_CTXT_UE_FMT" RX CREATE_TUNNEL_RESP num tunnels %u \n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
          create_tunnel_resp_pP->num_tunnels);
    rnti = create_tunnel_resp_pP->rnti;
    ue_context_p = rrc_eNB_get_ue_context(RC.rrc[ctxt_pP->module_id], ctxt_pP->rntiMaybeUEid);

    for (i = 0; i < create_tunnel_resp_pP->num_tunnels; i++) {
      ue_context_p->ue_context.enb_gtp_teid[inde_list[i]]  = create_tunnel_resp_pP->enb_S1u_teid[i];
      ue_context_p->ue_context.enb_gtp_addrs[inde_list[i]] = create_tunnel_resp_pP->enb_addr;
      ue_context_p->ue_context.enb_gtp_ebi[inde_list[i]]   = create_tunnel_resp_pP->eps_bearer_id[i];
      LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP tunnel (%u, %u) bearer UE context index %u, msg index %u, id %u, gtp addr len %d \n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            create_tunnel_resp_pP->enb_S1u_teid[i],
            ue_context_p->ue_context.enb_gtp_teid[inde_list[i]],            
            inde_list[i],
	    i,
            create_tunnel_resp_pP->eps_bearer_id[i],
	    create_tunnel_resp_pP->enb_addr.length);
    }

        (void)rnti; /* avoid gcc warning "set but not used" */
    return 0;
  } else {
    return -1;
  }
}

//------------------------------------------------------------------------------
bool gtpv_data_req(const protocol_ctxt_t*   const ctxt_pP,
                   const rb_id_t                  rb_idP,
                   const mui_t                    muiP,
                   const confirm_t                confirmP,
                   const sdu_size_t               sdu_sizeP,
                   uint8_t*                 const buffer_pP,
                   const pdcp_transmission_mode_t modeP,
                   uint32_t task_id)
//------------------------------------------------------------------------------
{
  if(sdu_sizeP == 0)
  {
    LOG_I(GTPU,"gtpv_data_req sdu_sizeP == 0");
    return false;
  }
  LOG_D(GTPU, "gtpv_data_req ue rnti %lx sdu_sizeP %d rb id %ld", ctxt_pP->rntiMaybeUEid, sdu_sizeP, rb_idP);
  MessageDef *message_p;
  // Uses a new buffer to avoid issue with PDCP buffer content that could be changed by PDCP (asynchronous message handling).
  uint8_t *message_buffer;
  
  if(task_id == TASK_DATA_FORWARDING){
    
    LOG_I(GTPU,"gtpv_data_req task_id = TASK_DATA_FORWARDING\n");
    
    message_buffer = itti_malloc (TASK_GTPV1_U, TASK_DATA_FORWARDING, sdu_sizeP);
    
    memcpy (message_buffer, buffer_pP, sdu_sizeP);
    
    message_p = itti_alloc_new_message (TASK_GTPV1_U, 0, GTPV1U_ENB_DATA_FORWARDING_IND);
    GTPV1U_ENB_DATA_FORWARDING_IND (message_p).frame 	= ctxt_pP->frame;
    GTPV1U_ENB_DATA_FORWARDING_IND (message_p).enb_flag	= ctxt_pP->enb_flag;
    GTPV1U_ENB_DATA_FORWARDING_IND (message_p).rb_id 	= rb_idP;
    GTPV1U_ENB_DATA_FORWARDING_IND (message_p).muip		= muiP;
    GTPV1U_ENB_DATA_FORWARDING_IND (message_p).confirmp	= confirmP;
    GTPV1U_ENB_DATA_FORWARDING_IND (message_p).sdu_size	= sdu_sizeP;
    GTPV1U_ENB_DATA_FORWARDING_IND (message_p).sdu_p 	= message_buffer;
    GTPV1U_ENB_DATA_FORWARDING_IND (message_p).mode		= modeP;
    GTPV1U_ENB_DATA_FORWARDING_IND (message_p).module_id = ctxt_pP->module_id;
    GTPV1U_ENB_DATA_FORWARDING_IND(message_p).rnti = ctxt_pP->rntiMaybeUEid;
    GTPV1U_ENB_DATA_FORWARDING_IND (message_p).eNB_index = ctxt_pP->eNB_index;
    
    itti_send_msg_to_task (TASK_DATA_FORWARDING, ctxt_pP->instance, message_p);
    return true; // TODO should be changed to a CNF message later, currently RRC lite does not used the returned value anyway.
  } else if (task_id == TASK_END_MARKER){
    
    LOG_I(GTPU,"gtpv_data_req task_id = TASK_END_MARKER\n");
    
    message_buffer = itti_malloc (TASK_GTPV1_U, TASK_END_MARKER, sdu_sizeP);
    
    memcpy (message_buffer, buffer_pP, sdu_sizeP);
    
    message_p = itti_alloc_new_message (TASK_GTPV1_U, 0, GTPV1U_ENB_END_MARKER_IND);
    GTPV1U_ENB_END_MARKER_IND (message_p).frame 	= ctxt_pP->frame;
    GTPV1U_ENB_END_MARKER_IND (message_p).enb_flag	= ctxt_pP->enb_flag;
    GTPV1U_ENB_END_MARKER_IND (message_p).rb_id 	= rb_idP;
    GTPV1U_ENB_END_MARKER_IND (message_p).muip	= muiP;
    GTPV1U_ENB_END_MARKER_IND (message_p).confirmp	= confirmP;
    GTPV1U_ENB_END_MARKER_IND (message_p).sdu_size	= sdu_sizeP;
    GTPV1U_ENB_END_MARKER_IND (message_p).sdu_p 	= message_buffer;
    GTPV1U_ENB_END_MARKER_IND (message_p).mode	= modeP;
    GTPV1U_ENB_END_MARKER_IND (message_p).module_id = ctxt_pP->module_id;
    GTPV1U_ENB_END_MARKER_IND(message_p).rnti = ctxt_pP->rntiMaybeUEid;
    GTPV1U_ENB_END_MARKER_IND (message_p).eNB_index = ctxt_pP->eNB_index;
    
    itti_send_msg_to_task (TASK_END_MARKER, ctxt_pP->instance, message_p);
    return true; // TODO should be changed to a CNF message later, currently RRC lite does not used the returned value anyway.
  }
  LOG_E(RRC, "Impossible state\n");
  return false;
}

bool gtpv_data_req_new(protocol_ctxt_t  *ctxt,
                       const srb_flag_t     srb_flagP,
                       const rb_id_t        rb_idP,
                       const mui_t          muiP,
                       const confirm_t      confirmP,
                       const sdu_size_t     sdu_buffer_sizeP,
                       unsigned char *const sdu_buffer_pP,
                       const pdcp_transmission_mode_t modeP,
                       const uint32_t *sourceL2Id,
                       const uint32_t *destinationL2Id) {
  int task;

  if (sdu_buffer_sizeP==0)
    task=TASK_END_MARKER;
  else
    task=TASK_DATA_FORWARDING;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[ctxt->module_id], ctxt->rntiMaybeUEid);
  if(ue_context_p == NULL || ue_context_p->ue_context.handover_info == NULL ||
     ue_context_p->ue_context.StatusRrc != RRC_HO_EXECUTION) {
    LOG_E(RRC,"incoming GTP-U for X2 in non HO context\n");
    return false;
  }

  /* in the source enb, UE in RRC_HO_EXECUTION mode */
  // We have 2*2=4 cases (data or end marker) * (from slave, from EPC)
  if (ue_context_p->ue_context.handover_info->state == HO_COMPLETE) {
      LOG_I(GTPU, "source receive END MARKER\n");
      /* set handover state */
      //ue_context_p->ue_context.handover_info->state = HO_END_MARKER;
      MessageDef *msg;
      // Configure end marker
      msg = itti_alloc_new_message(TASK_GTPV1_U, 0, GTPV1U_ENB_END_MARKER_REQ);
      GTPV1U_ENB_END_MARKER_REQ(msg).buffer = itti_malloc(TASK_GTPV1_U, TASK_GTPV1_U, GTPU_HEADER_OVERHEAD_MAX + sdu_buffer_sizeP);
      memcpy(&GTPV1U_ENB_END_MARKER_REQ(msg).buffer[GTPU_HEADER_OVERHEAD_MAX],  sdu_buffer_pP, sdu_buffer_sizeP);
      GTPV1U_ENB_END_MARKER_REQ(msg).length = sdu_buffer_sizeP;
      GTPV1U_ENB_END_MARKER_REQ(msg).rnti = ctxt->rntiMaybeUEid;
      GTPV1U_ENB_END_MARKER_REQ(msg).rab_id = rb_idP;
      GTPV1U_ENB_END_MARKER_REQ(msg).offset = GTPU_HEADER_OVERHEAD_MAX;
      LOG_I(GTPU, "Send End Marker to GTPV1-U at frame %d and subframe %d \n", ctxt->frame,ctxt->subframe);
      itti_send_msg_to_task(TASK_GTPV1_U, ENB_MODULE_ID_TO_INSTANCE(ctxt->module_id), msg);
      return 0;
  }
  
  /* target enb */
  //  We have 2*2=4 cases (data or end marker) * (from source, from EPC)
  if (ue_context_p->ue_context.StatusRrc == RRC_RECONFIGURED) {
    // It should come from remote eNB
    // case end marker by EPC is not possible ?
    if (task==TASK_END_MARKER) { 
      LOG_I(GTPU, "target end receive END MARKER\n");
      // We close the HO, nothing to send to remote ?
      ue_context_p->ue_context.handover_info->state = HO_END_MARKER;
      gtpv1u_enb_delete_tunnel_req_t delete_tunnel_req;
      memset(&delete_tunnel_req, 0, sizeof(delete_tunnel_req));
      delete_tunnel_req.rnti = ctxt->rntiMaybeUEid;
      gtpv1u_delete_x2u_tunnel(ctxt->module_id, &delete_tunnel_req);
      return true;
    } else {
      /* data packet */
      LOG_I(GTPU, "Received a message data forwarding length %d\n", sdu_buffer_sizeP);
      // Is it from remote ENB
      // We have to push it to the UE ?
      if(ue_context_p->ue_context.handover_info->state != HO_COMPLETE) {
	int result = pdcp_data_req(
			       ctxt,
			       srb_flagP,
			       rb_idP,
			       muiP, // mui
			       confirmP, // confirm
			        sdu_buffer_sizeP,
			       sdu_buffer_pP,
			       modeP,
			       sourceL2Id,
			       destinationL2Id
			       );
	ue_context_p->ue_context.handover_info->forwarding_state = FORWARDING_NO_EMPTY;
	return result;
      } else {  /* It is from from epc message */
	/* in the source enb, UE in RRC_HO_EXECUTION mode */
	// ?????
	return true;
      }
    }
  }
  LOG_E(RRC,"This function should return before the end\n");
  return false;
}	


void rrc_eNB_send_GTPV1U_ENB_DELETE_TUNNEL_REQ(
  module_id_t enb_mod_idP,
  rrc_eNB_ue_context_t*  ue_context_pP
)
{
  if (!ue_context_pP) {
    LOG_W(RRC, "[eNB] In %s: invalid UE\n", __func__);
    return;
  }
  eNB_RRC_UE_t *ue = &ue_context_pP->ue_context;
  gtpv1u_enb_delete_tunnel_req_t tmp={0};

  tmp.rnti = ue->rnti;
  tmp.from_gnb = 0;
  tmp.num_erab = ue->nb_of_e_rabs;
  for (int e_rab = 0; e_rab < ue->nb_of_e_rabs; e_rab++) {
    const rb_id_t gtp_ebi = ue->enb_gtp_ebi[e_rab];
    tmp.eps_bearer_id[e_rab] = gtp_ebi;
  }
  gtpv1u_delete_s1u_tunnel(enb_mod_idP,&tmp);
  if ( ue->ue_release_timer_rrc > 0
       && (ue->handover_info == NULL ||
	   (ue->handover_info->state != HO_RELEASE &&
	    ue->handover_info->state != HO_CANCEL
	   )
       )
    )
    ue->ue_release_timer_rrc = ue->ue_release_timer_thres_rrc;
  
}


