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

/* \file l2_interface_ue.c
 * \brief layer 2 interface, used to support different RRC sublayer
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include "rrc_defs.h"
#include "rrc_proto.h"
#include "assertions.h"

typedef uint32_t channel_t;

int8_t
nr_mac_rrc_data_ind_ue(
    const module_id_t     module_id,
    const int             CC_id,
    const uint8_t         gNB_index,
    const channel_t       channel,
    const uint8_t*        pduP,
    const sdu_size_t      pdu_len){

    switch(channel){
        case NR_BCCH_BCH:
            AssertFatal( nr_rrc_ue_decode_NR_BCCH_BCH_Message( module_id, gNB_index, (uint8_t*)pduP, pdu_len) == 0, "UE decode BCCH-BCH error!\n");
            break;
        default:
            break;
    }


    return(0);

}

int8_t mac_rrc_nr_data_req_ue(const module_id_t Mod_idP,
                              const int         CC_id,
                              const frame_t     frameP,
                              const rb_id_t     Srb_id,
                              uint8_t *const    buffer_pP ){

  // todo

  return 0;
}

uint8_t
rrc_data_req_ue(
  const protocol_ctxt_t   *const ctxt_pP,
  const rb_id_t                  rb_idP,
  const mui_t                    muiP,
  const confirm_t                confirmP,
  const sdu_size_t               sdu_sizeP,
  uint8_t                 *const buffer_pP,
  const pdcp_transmission_mode_t modeP
)
{
    MessageDef *message_p;
    // Uses a new buffer to avoid issue with PDCP buffer content that could be changed by PDCP (asynchronous message handling).
    uint8_t *message_buffer;
    message_buffer = itti_malloc (
                       ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE,
                       ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE,
                       sdu_sizeP);
    memcpy (message_buffer, buffer_pP, sdu_sizeP);
    message_p = itti_alloc_new_message (ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE, RRC_DCCH_DATA_REQ);
    RRC_DCCH_DATA_REQ (message_p).frame     = ctxt_pP->frame;
    RRC_DCCH_DATA_REQ (message_p).enb_flag  = ctxt_pP->enb_flag;
    RRC_DCCH_DATA_REQ (message_p).rb_id     = rb_idP;
    RRC_DCCH_DATA_REQ (message_p).muip      = muiP;
    RRC_DCCH_DATA_REQ (message_p).confirmp  = confirmP;
    RRC_DCCH_DATA_REQ (message_p).sdu_size  = sdu_sizeP;
    RRC_DCCH_DATA_REQ (message_p).sdu_p     = message_buffer;
    RRC_DCCH_DATA_REQ (message_p).mode      = modeP;
    RRC_DCCH_DATA_REQ (message_p).module_id = ctxt_pP->module_id;
    RRC_DCCH_DATA_REQ (message_p).rnti      = ctxt_pP->rnti;
    RRC_DCCH_DATA_REQ (message_p).eNB_index = ctxt_pP->eNB_index;
    itti_send_msg_to_task (
      TASK_PDCP_UE,
      ctxt_pP->instance,
      message_p);
    return TRUE; // TODO should be changed to a CNF message later, currently RRC lite does not used the returned value anyway.

}
