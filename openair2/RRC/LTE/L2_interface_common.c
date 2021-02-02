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

/*! \file l2_interface.c
 * \brief layer 2 interface, used to support different RRC sublayer
 * \author Raymond Knopp and Navid Nikaein
 * \date 2010-2014
 * \version 1.0
 * \company Eurecom
 * \email: raymond.knopp@eurecom.fr
 */

#include "platform_types.h"
#include "rrc_defs.h"
#include "rrc_extern.h"
#include "common/utils/LOG/log.h"
#include "rrc_eNB_UE_context.h"
#include "pdcp.h"
#include "msc.h"
#include "common/ran_context.h"

#include "intertask_interface.h"

//#define RRC_DATA_REQ_DEBUG
//#define DEBUG_RRC 1

extern RAN_CONTEXT_t RC;

//------------------------------------------------------------------------------
uint8_t
rrc_data_req(
  const protocol_ctxt_t   *const ctxt_pP,
  const rb_id_t                  rb_idP,
  const mui_t                    muiP,
  const confirm_t                confirmP,
  const sdu_size_t               sdu_sizeP,
  uint8_t                 *const buffer_pP,
  const pdcp_transmission_mode_t modeP
)
//------------------------------------------------------------------------------
{
  if(sdu_sizeP == 255) {
    LOG_I(RRC,"sdu_sizeP == 255");
    return FALSE;
  }

  MSC_LOG_TX_MESSAGE(
    ctxt_pP->enb_flag ? MSC_RRC_ENB : MSC_RRC_UE,
    ctxt_pP->enb_flag ? MSC_PDCP_ENB : MSC_PDCP_UE,
    buffer_pP,
    sdu_sizeP,
    MSC_AS_TIME_FMT"RRC_DCCH_DATA_REQ UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ctxt_pP->rnti,
    muiP,
    sdu_sizeP);
  MessageDef *message_p;
  // Uses a new buffer to avoid issue with PDCP buffer content that could be changed by PDCP (asynchronous message handling).
  uint8_t *message_buffer;
  message_buffer = itti_malloc (
                     ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE,
                     ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE,
                     sdu_sizeP);
  memcpy (message_buffer, buffer_pP, sdu_sizeP);
  message_p = itti_alloc_new_message (ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE, 0, RRC_DCCH_DATA_REQ);
  RRC_DCCH_DATA_REQ (message_p).frame     = ctxt_pP->frame;
  RRC_DCCH_DATA_REQ (message_p).enb_flag  = ctxt_pP->enb_flag;
  RRC_DCCH_DATA_REQ (message_p).rb_id     = rb_idP;
  RRC_DCCH_DATA_REQ (message_p).muip      = muiP;
  RRC_DCCH_DATA_REQ (message_p).confirmp  = confirmP;
  RRC_DCCH_DATA_REQ (message_p).sdu_size  = sdu_sizeP;
  RRC_DCCH_DATA_REQ (message_p).sdu_p     = message_buffer;
  //memcpy (RRC_DCCH_DATA_REQ (message_p).sdu_p, buffer_pP, sdu_sizeP);
  RRC_DCCH_DATA_REQ (message_p).mode      = modeP;
  RRC_DCCH_DATA_REQ (message_p).module_id = ctxt_pP->module_id;
  RRC_DCCH_DATA_REQ (message_p).rnti      = ctxt_pP->rnti;
  RRC_DCCH_DATA_REQ (message_p).eNB_index = ctxt_pP->eNB_index;
  itti_send_msg_to_task (
    ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE,
    ctxt_pP->instance,
    message_p);
  LOG_I(RRC,"sent RRC_DCCH_DATA_REQ to TASK_PDCP_ENB\n");

  /* Hack: only trigger PDCP if in CU, otherwise it is triggered by RU threads
   * Ideally, PDCP would not neet to be triggered like this but react to ITTI
   * messages automatically */
  if (ctxt_pP->enb_flag && NODE_IS_CU(RC.rrc[ctxt_pP->module_id]->node_type))
    pdcp_run(ctxt_pP);

  return TRUE; // TODO should be changed to a CNF message later, currently RRC lite does not used the returned value anyway.
}

//------------------------------------------------------------------------------
void
rrc_data_ind(
  const protocol_ctxt_t *const ctxt_pP,
  const rb_id_t                Srb_id,
  const sdu_size_t             sdu_sizeP,
  const uint8_t   *const       buffer_pP
)
//------------------------------------------------------------------------------
{
  rb_id_t    DCCH_index = Srb_id;

  if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
    LOG_I(RRC, "[UE %x] Frame %d: received a DCCH %ld message on SRB %ld with Size %d from eNB %d\n",
          ctxt_pP->module_id, ctxt_pP->frame, DCCH_index,Srb_id,sdu_sizeP,  ctxt_pP->eNB_index);
  } else {
    LOG_D(RRC, "[eNB %d] Frame %d: received a DCCH %ld message on SRB %ld with Size %d from UE %x\n",
          ctxt_pP->module_id,
          ctxt_pP->frame,
          DCCH_index,
          Srb_id,
          sdu_sizeP,
          ctxt_pP->rnti);
  }

  {
    MessageDef *message_p;
    // Uses a new buffer to avoid issue with PDCP buffer content that could be changed by PDCP (asynchronous message handling).
    uint8_t *message_buffer;
    message_buffer = itti_malloc (ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE, ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE, sdu_sizeP);
    memcpy (message_buffer, buffer_pP, sdu_sizeP);
    message_p = itti_alloc_new_message (ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE, 0, RRC_DCCH_DATA_IND);
    RRC_DCCH_DATA_IND (message_p).frame      = ctxt_pP->frame;
    RRC_DCCH_DATA_IND (message_p).dcch_index = DCCH_index;
    RRC_DCCH_DATA_IND (message_p).sdu_size   = sdu_sizeP;
    RRC_DCCH_DATA_IND (message_p).sdu_p      = message_buffer;
    RRC_DCCH_DATA_IND (message_p).rnti       = ctxt_pP->rnti;
    RRC_DCCH_DATA_IND (message_p).module_id  = ctxt_pP->module_id;
    RRC_DCCH_DATA_IND (message_p).eNB_index  = ctxt_pP->eNB_index;
    itti_send_msg_to_task (ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE, ctxt_pP->instance, message_p);
  }
}
