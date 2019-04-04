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


#if defined(ENABLE_ITTI)
  #include "intertask_interface.h"
#endif

//#define RRC_DATA_REQ_DEBUG



//------------------------------------------------------------------------------
int8_t
mac_rrc_data_req_ue(
  const module_id_t Mod_idP,
  const int         CC_id,
  const frame_t     frameP,
  const rb_id_t     Srb_id,
  const uint8_t     Nb_tb,
  uint8_t    *const buffer_pP,
  const uint8_t     eNB_index,
  const uint8_t     mbsfn_sync_area
)
//--------------------------------------------------------------------------
{
  LOG_D(RRC,"[eNB %d] mac_rrc_data_req to SRB ID=%d\n",Mod_idP,Srb_id);
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  LOG_D(RRC,"[UE %d] Frame %d Filling SL DISCOVERY SRB_ID %d\n",Mod_idP,frameP,Srb_id);
  LOG_D(RRC,"[UE %d] Frame %d buffer_pP status %d,\n",Mod_idP,frameP, UE_rrc_inst[Mod_idP].SL_Discovery[eNB_index].Tx_buffer.payload_size);

  //TTN (for D2D)
  if (Srb_id  == SL_DISCOVERY && UE_rrc_inst[Mod_idP].SL_Discovery[eNB_index].Tx_buffer.payload_size > 0) {
    memcpy(&buffer_pP[0],&UE_rrc_inst[Mod_idP].SL_Discovery[eNB_index].Tx_buffer.Payload[0],UE_rrc_inst[Mod_idP].SL_Discovery[eNB_index].Tx_buffer.payload_size);
    uint8_t Ret_size=UE_rrc_inst[Mod_idP].SL_Discovery[eNB_index].Tx_buffer.payload_size;
    LOG_I(RRC,"[UE %d] Sending SL_Discovery, size %d bytes\n",Mod_idP,Ret_size);
    UE_rrc_inst[Mod_idP].SL_Discovery[eNB_index].Tx_buffer.payload_size = 0;
    return(Ret_size);
  }

#endif
  LOG_D(RRC,"[UE %d] Frame %d Filling CCCH SRB_ID %d\n",Mod_idP,frameP,Srb_id);
  LOG_D(RRC,"[UE %d] Frame %d buffer_pP status %d,\n",Mod_idP,frameP, UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size);

  if( (UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size > 0) ) {
#if defined(ENABLE_ITTI)
    {
      MessageDef *message_p;
      int ccch_size = UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size;
      int sdu_size = sizeof(RRC_MAC_CCCH_DATA_REQ (message_p).sdu);

      if (ccch_size > sdu_size) {
        LOG_E(RRC, "SDU larger than CCCH SDU buffer size (%d, %d)", ccch_size, sdu_size);
        ccch_size = sdu_size;
      }

      message_p = itti_alloc_new_message (TASK_RRC_UE, RRC_MAC_CCCH_DATA_REQ);
      RRC_MAC_CCCH_DATA_REQ (message_p).frame = frameP;
      RRC_MAC_CCCH_DATA_REQ (message_p).sdu_size = ccch_size;
      memset (RRC_MAC_CCCH_DATA_REQ (message_p).sdu, 0, CCCH_SDU_SIZE);
      memcpy (RRC_MAC_CCCH_DATA_REQ (message_p).sdu, UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.Payload, ccch_size);
      RRC_MAC_CCCH_DATA_REQ (message_p).enb_index = eNB_index;

      itti_send_msg_to_task (TASK_MAC_UE, UE_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
    }
#endif
    memcpy(&buffer_pP[0],&UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.Payload[0],UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size);
    uint8_t Ret_size=UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size;
    //   UE_rrc_inst[Mod_id].Srb0[eNB_index].Tx_buffer.payload_size=0;
    UE_rrc_inst[Mod_idP].Info[eNB_index].T300_active = 1;
    UE_rrc_inst[Mod_idP].Info[eNB_index].T300_cnt = 0;
    //      msg("[RRC][UE %d] Sending rach\n",Mod_id);
    return(Ret_size);
  } else {
    return 0;
  }

  return(0);
}

//------------------------------------------------------------------------------
int8_t
mac_rrc_data_ind_ue(
  const module_id_t     module_idP,
  const int             CC_id,
  const frame_t         frameP,
  const sub_frame_t     sub_frameP,
  const rnti_t          rntiP,
  const rb_id_t         srb_idP,
  const uint8_t        *sduP,
  const sdu_size_t      sdu_lenP,
  const mac_enb_index_t eNB_indexP,
  const uint8_t         mbsfn_sync_areaP
)
//--------------------------------------------------------------------------
{
  protocol_ctxt_t ctxt;
  sdu_size_t      sdu_size = 0;
  /* for no gcc warnings */
  (void)sdu_size;
  /*
  int si_window;
   */
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, 0, rntiP, frameP, sub_frameP,eNB_indexP);

  if(srb_idP == BCCH) {
    LOG_D(RRC,"[UE %d] Received SDU for BCCH on SRB %d from eNB %d\n",module_idP,srb_idP,eNB_indexP);
#if defined(ENABLE_ITTI)
    {
      MessageDef *message_p;
      int msg_sdu_size = sizeof(RRC_MAC_BCCH_DATA_IND (message_p).sdu);

      if (sdu_lenP > msg_sdu_size) {
        LOG_E(RRC, "SDU larger than BCCH SDU buffer size (%d, %d)", sdu_lenP, msg_sdu_size);
        sdu_size = msg_sdu_size;
      } else {
        sdu_size = sdu_lenP;
      }

      message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_BCCH_DATA_IND);
      memset (RRC_MAC_BCCH_DATA_IND (message_p).sdu, 0, BCCH_SDU_SIZE);
      RRC_MAC_BCCH_DATA_IND (message_p).frame     = frameP;
      RRC_MAC_BCCH_DATA_IND (message_p).sub_frame = sub_frameP;
      RRC_MAC_BCCH_DATA_IND (message_p).sdu_size  = sdu_size;
      memcpy (RRC_MAC_BCCH_DATA_IND (message_p).sdu, sduP, sdu_size);
      RRC_MAC_BCCH_DATA_IND (message_p).enb_index = eNB_indexP;
      RRC_MAC_BCCH_DATA_IND (message_p).rsrq      = 30 /* TODO change phy to report rspq */;
      RRC_MAC_BCCH_DATA_IND (message_p).rsrp      = 45 /* TODO change phy to report rspp */;
      itti_send_msg_to_task (TASK_RRC_UE, ctxt.instance, message_p);
    }
#else
    decode_BCCH_DLSCH_Message(&ctxt,eNB_indexP,(uint8_t *)sduP,sdu_lenP, 0, 0);
#endif
  }

  if(srb_idP == PCCH) {
    LOG_D(RRC,"[UE %d] Received SDU for PCCH on SRB %d from eNB %d\n",module_idP,srb_idP,eNB_indexP);
    decode_PCCH_DLSCH_Message(&ctxt,eNB_indexP,(uint8_t *)sduP,sdu_lenP);
  }

  if((srb_idP & RAB_OFFSET) == CCCH) {
    if (sdu_lenP>0) {
      LOG_T(RRC,"[UE %d] Received SDU for CCCH on SRB %d from eNB %d\n",module_idP,srb_idP & RAB_OFFSET,eNB_indexP);
#if defined(ENABLE_ITTI)
      {
        MessageDef *message_p;
        int msg_sdu_size = CCCH_SDU_SIZE;

        if (sdu_lenP > msg_sdu_size) {
          LOG_E(RRC, "SDU larger than CCCH SDU buffer size (%d, %d)", sdu_size, msg_sdu_size);
          sdu_size = msg_sdu_size;
        } else {
          sdu_size =  sdu_lenP;
        }

        message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_CCCH_DATA_IND);
        memset (RRC_MAC_CCCH_DATA_IND (message_p).sdu, 0, CCCH_SDU_SIZE);
        memcpy (RRC_MAC_CCCH_DATA_IND (message_p).sdu, sduP, sdu_size);
        RRC_MAC_CCCH_DATA_IND (message_p).frame     = frameP;
        RRC_MAC_CCCH_DATA_IND (message_p).sub_frame = sub_frameP;
        RRC_MAC_CCCH_DATA_IND (message_p).sdu_size  = sdu_size;
        RRC_MAC_CCCH_DATA_IND (message_p).enb_index = eNB_indexP;
        RRC_MAC_CCCH_DATA_IND (message_p).rnti      = rntiP;
        itti_send_msg_to_task (TASK_RRC_UE, ctxt.instance, message_p);
      }
#else
      SRB_INFO *Srb_info;
      Srb_info = &UE_rrc_inst[module_idP].Srb0[eNB_indexP];
      memcpy(Srb_info->Rx_buffer.Payload,sduP,sdu_lenP);
      Srb_info->Rx_buffer.payload_size = sdu_lenP;
      rrc_ue_decode_ccch(&ctxt, Srb_info, eNB_indexP);
#endif
    }
  }

#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))

  if ((srb_idP & RAB_OFFSET) == MCCH) {
    LOG_T(RRC,"[UE %d] Frame %d: Received SDU on MBSFN sync area %d for MCCH on SRB %d from eNB %d\n",
          module_idP,frameP, mbsfn_sync_areaP, srb_idP & RAB_OFFSET,eNB_indexP);
#if defined(ENABLE_ITTI)
    {
      MessageDef *message_p;
      int msg_sdu_size = sizeof(RRC_MAC_MCCH_DATA_IND (message_p).sdu);

      if (sdu_size > msg_sdu_size) {
        LOG_E(RRC, "SDU larger than MCCH SDU buffer size (%d, %d)", sdu_size, msg_sdu_size);
        sdu_size = msg_sdu_size;
      }

      message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_MCCH_DATA_IND);
      RRC_MAC_MCCH_DATA_IND (message_p).frame           = frameP;
      RRC_MAC_MCCH_DATA_IND (message_p).sub_frame       = sub_frameP;
      RRC_MAC_MCCH_DATA_IND (message_p).sdu_size        = sdu_lenP;
      memset (RRC_MAC_MCCH_DATA_IND (message_p).sdu, 0, MCCH_SDU_SIZE);
      memcpy (RRC_MAC_MCCH_DATA_IND (message_p).sdu, sduP, sdu_lenP);
      RRC_MAC_MCCH_DATA_IND (message_p).enb_index       = eNB_indexP;
      RRC_MAC_MCCH_DATA_IND (message_p).mbsfn_sync_area = mbsfn_sync_areaP;
      itti_send_msg_to_task (TASK_RRC_UE, ctxt.instance, message_p);
    }
#else
    decode_MCCH_Message(&ctxt, eNB_indexP, sduP, sdu_lenP, mbsfn_sync_areaP);
#endif
  }

  //TTN (for D2D)
  if(srb_idP == SL_DISCOVERY) {
    LOG_I(RRC,"[UE %d] Received SDU (%d bytes) for SL_DISCOVERY on SRB %d from eNB %d\n",module_idP, sdu_lenP, srb_idP,eNB_indexP);
    decode_SL_Discovery_Message(&ctxt, eNB_indexP, sduP, sdu_lenP);
  }

#endif // #if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  return(0);
}


//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
{
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
#if defined(ENABLE_ITTI)
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
#else
  return pdcp_data_req (
           ctxt_pP,
           SRB_FLAG_YES,
           rb_idP,
           muiP,
           confirmP,
           sdu_sizeP,
           buffer_pP,
           modeP
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
           ,NULL, NULL
#endif
         );
#endif
}

//------------------------------------------------------------------------------
void
rrc_data_ind_ue(
  const protocol_ctxt_t *const ctxt_pP,
  const rb_id_t                Srb_id,
  const sdu_size_t             sdu_sizeP,
  const uint8_t   *const       buffer_pP
)
//------------------------------------------------------------------------------
{
  rb_id_t    DCCH_index = Srb_id;
  LOG_I(RRC, "[UE %x] Frame %d: received a DCCH %d message on SRB %d with Size %d from eNB %d\n",
        ctxt_pP->module_id, ctxt_pP->frame, DCCH_index,Srb_id,sdu_sizeP,  ctxt_pP->eNB_index);
#if defined(ENABLE_ITTI)
  {
    MessageDef *message_p;
    // Uses a new buffer to avoid issue with PDCP buffer content that could be changed by PDCP (asynchronous message handling).
    uint8_t *message_buffer;
    message_buffer = itti_malloc (ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE, ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE, sdu_sizeP);
    memcpy (message_buffer, buffer_pP, sdu_sizeP);
    message_p = itti_alloc_new_message (ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE, RRC_DCCH_DATA_IND);
    RRC_DCCH_DATA_IND (message_p).frame      = ctxt_pP->frame;
    RRC_DCCH_DATA_IND (message_p).dcch_index = DCCH_index;
    RRC_DCCH_DATA_IND (message_p).sdu_size   = sdu_sizeP;
    RRC_DCCH_DATA_IND (message_p).sdu_p      = message_buffer;
    RRC_DCCH_DATA_IND (message_p).rnti       = ctxt_pP->rnti;
    RRC_DCCH_DATA_IND (message_p).module_id  = ctxt_pP->module_id;
    RRC_DCCH_DATA_IND (message_p).eNB_index  = ctxt_pP->eNB_index;
    itti_send_msg_to_task (TASK_RRC_UE, ctxt_pP->instance, message_p);
  }
#else
  //#warning "LG put 0 to arg4 that is eNB index"
  rrc_ue_decode_dcch(
    ctxt_pP,
    DCCH_index,
    buffer_pP,
    0);
#endif
}

//-------------------------------------------------------------------------------------------//
void rrc_in_sync_ind(module_id_t Mod_idP, frame_t frameP, uint16_t eNB_index) {
  //-------------------------------------------------------------------------------------------//
#if defined(ENABLE_ITTI)
  {
    MessageDef *message_p;
    //LOG_I(RRC,"sending a message to task_mac_ue\n");
    message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_IN_SYNC_IND);
    RRC_MAC_IN_SYNC_IND (message_p).frame = frameP;
    RRC_MAC_IN_SYNC_IND (message_p).enb_index = eNB_index;
    itti_send_msg_to_task (TASK_RRC_UE, UE_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
  }
#else
  UE_rrc_inst[Mod_idP].Info[eNB_index].N310_cnt=0;

  if (UE_rrc_inst[Mod_idP].Info[eNB_index].T310_active==1) {
    UE_rrc_inst[Mod_idP].Info[eNB_index].N311_cnt++;
  }

#endif
}

//-------------------------------------------------------------------------------------------//
void rrc_out_of_sync_ind(module_id_t Mod_idP, frame_t frameP, uint16_t eNB_index) {
  //-------------------------------------------------------------------------------------------//
  if (UE_rrc_inst[Mod_idP].Info[eNB_index].N310_cnt>10)
    LOG_I(RRC,"[UE %d] Frame %d: OUT OF SYNC FROM eNB %d (T310 active %d : T310 %d, N310 %d, N311 %d)\n ",
          Mod_idP,frameP,eNB_index,
          UE_rrc_inst[Mod_idP].Info[eNB_index].T300_active,
          UE_rrc_inst[Mod_idP].Info[eNB_index].T310_cnt,
          UE_rrc_inst[Mod_idP].Info[eNB_index].N310_cnt,
          UE_rrc_inst[Mod_idP].Info[eNB_index].N311_cnt);

#if defined(ENABLE_ITTI)
  {
    MessageDef *message_p;
    message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_OUT_OF_SYNC_IND);
    RRC_MAC_OUT_OF_SYNC_IND (message_p).frame = frameP;
    RRC_MAC_OUT_OF_SYNC_IND (message_p).enb_index = eNB_index;
    itti_send_msg_to_task (TASK_RRC_UE, UE_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
  }
#else
  UE_rrc_inst[Mod_idP].Info[eNB_index].N310_cnt++;
#endif
}

//------------------------------------------------------------------------------
int
mac_UE_get_rrc_status(
  const module_id_t Mod_idP,
  const uint8_t     indexP
)
//------------------------------------------------------------------------------
{
  if (UE_rrc_inst)
    return(UE_rrc_inst[Mod_idP].Info[indexP].State);
  else
    return(-1);
}

//-------------------------------------------------------------------------------------------//
int mac_ue_ccch_success_ind(module_id_t Mod_idP, uint8_t eNB_index) {
  //-------------------------------------------------------------------------------------------//
#if defined(ENABLE_ITTI)
  {
    MessageDef *message_p;
    message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_CCCH_DATA_CNF);
    RRC_MAC_CCCH_DATA_CNF (message_p).enb_index = eNB_index;
    itti_send_msg_to_task (TASK_RRC_UE, UE_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
  }
#else
  // reset the tx buffer to indicate RRC that ccch was successfully transmitted (for example if contention resolution succeeds)
  UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size=0;
#endif
  return 0;
}
