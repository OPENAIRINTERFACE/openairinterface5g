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
//#include "openair_defs.h"
//#include "openair_proto.h"
#include "defs.h"
#include "extern.h"
//#include "mac_lchan_interface.h"
//#include "openair_rrc_utils.h"
//#include "openair_rrc_main.h"
#include "UTIL/LOG/log.h"
#include "rrc_eNB_UE_context.h"
#include "pdcp.h"
#include "msc.h"
#include "common/ran_context.h"

#ifdef PHY_EMUL
#include "SIMULATION/simulation_defs.h"
extern EMULATION_VARS *Emul_vars;
extern eNB_MAC_INST *eNB_mac_inst;
extern UE_MAC_INST *UE_mac_inst;
#endif

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#include "flexran_agent_extern.h"

//#define RRC_DATA_REQ_DEBUG
//#define DEBUG_RRC 1


extern RAN_CONTEXT_t RC;

//------------------------------------------------------------------------------
int8_t
mac_rrc_data_req(
  const module_id_t Mod_idP,
  const int         CC_id,
  const frame_t     frameP,
  const rb_id_t     Srb_id,
  const uint8_t     Nb_tb,
  uint8_t*    const buffer_pP,
  const uint8_t     mbsfn_sync_area
)
//--------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  SRB_INFO *Srb_info;
  uint8_t Sdu_size                = 0;
  uint8_t sfn                     = (uint8_t)((frameP>>2)&0xff);


#ifdef DEBUG_RRC
  int i;
  LOG_I(RRC,"[eNB %d] mac_rrc_data_req to SRB ID=%d\n",Mod_idP,Srb_id);
#endif

  eNB_RRC_INST *rrc;
  rrc_eNB_carrier_data_t *carrier;
  BCCH_BCH_Message_t *mib;


    rrc     = RC.rrc[Mod_idP];
    carrier = &rrc->carrier[0];
    mib     = &carrier->mib;

    if((Srb_id & RAB_OFFSET) == BCCH) {
      if(RC.rrc[Mod_idP]->carrier[CC_id].SI.Active==0) {
        return 0;
      }

      // All even frames transmit SIB in SF 5
      AssertFatal(RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1 != 255, 
		  "[eNB %d] MAC Request for SIB1 and SIB1 not initialized\n",Mod_idP);

      if ((frameP%2) == 0) {
        memcpy(&buffer_pP[0],
               RC.rrc[Mod_idP]->carrier[CC_id].SIB1,
               RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1);

#if 0 //defined(ENABLE_ITTI)
        {
          MessageDef *message_p;
          int sib1_size = RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1;
          int sdu_size = sizeof(RRC_MAC_BCCH_DATA_REQ (message_p).sdu);

          if (sib1_size > sdu_size) {
            LOG_E(RRC, "SIB1 SDU larger than BCCH SDU buffer size (%d, %d)", sib1_size, sdu_size);
            sib1_size = sdu_size;
          }

          message_p = itti_alloc_new_message (TASK_RRC_ENB, RRC_MAC_BCCH_DATA_REQ);
          RRC_MAC_BCCH_DATA_REQ (message_p).frame    = frameP;
          RRC_MAC_BCCH_DATA_REQ (message_p).sdu_size = sib1_size;
          memset (RRC_MAC_BCCH_DATA_REQ (message_p).sdu, 0, BCCH_SDU_SIZE);
          memcpy (RRC_MAC_BCCH_DATA_REQ (message_p).sdu,
                  RC.rrc[Mod_idP]->carrier[CC_id].SIB1,
                  sib1_size);
          RRC_MAC_BCCH_DATA_REQ (message_p).enb_index = eNB_index;

          itti_send_msg_to_task (TASK_MAC_ENB, ENB_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
        }
#endif

#ifdef DEBUG_RRC
        LOG_T(RRC,"[eNB %d] Frame %d : BCCH request => SIB 1\n",Mod_idP,frameP);

        for (i=0; i<RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1; i++) {
          LOG_T(RRC,"%x.",buffer_pP[i]);
        }

        LOG_T(RRC,"\n");
#endif

        return (RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1);
      } // All RFN mod 8 transmit SIB2-3 in SF 5
      else if ((frameP%8) == 1) {
        memcpy(&buffer_pP[0],
               RC.rrc[Mod_idP]->carrier[CC_id].SIB23,
               RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23);

#if 0 //defined(ENABLE_ITTI)
        {
          MessageDef *message_p;
          int sib23_size = RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23;
          int sdu_size = sizeof(RRC_MAC_BCCH_DATA_REQ (message_p).sdu);

          if (sib23_size > sdu_size) {
            LOG_E(RRC, "SIB23 SDU larger than BCCH SDU buffer size (%d, %d)", sib23_size, sdu_size);
            sib23_size = sdu_size;
          }

          message_p = itti_alloc_new_message (TASK_RRC_ENB, RRC_MAC_BCCH_DATA_REQ);
          RRC_MAC_BCCH_DATA_REQ (message_p).frame = frameP;
          RRC_MAC_BCCH_DATA_REQ (message_p).sdu_size = sib23_size;
          memset (RRC_MAC_BCCH_DATA_REQ (message_p).sdu, 0, BCCH_SDU_SIZE);
          memcpy (RRC_MAC_BCCH_DATA_REQ (message_p).sdu,
                  RC.rrc[Mod_idP]->carrier[CC_id].SIB23,
                  sib23_size);
          RRC_MAC_BCCH_DATA_REQ (message_p).enb_index = eNB_index;

          itti_send_msg_to_task (TASK_MAC_ENB, ENB_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
        }
#endif

#ifdef DEBUG_RRC
        LOG_T(RRC,"[eNB %d] Frame %d BCCH request => SIB 2-3\n",Mod_idP,frameP);

        for (i=0; i<RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23; i++) {
          LOG_T(RRC,"%x.",buffer_pP[i]);
        }

        LOG_T(RRC,"\n");
#endif
        return(RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23);
      } else {
        return(0);
      }
    }
    if( (Srb_id & RAB_OFFSET ) == MIBCH) {

        mib->message.systemFrameNumber.buf = &sfn;
	enc_rval = uper_encode_to_buffer(&asn_DEF_BCCH_BCH_Message,
					 (void*)mib,
					 carrier->MIB,
					 24);
	LOG_D(RRC,"Encoded MIB for frame %d (%p), bits %lu\n",sfn,carrier->MIB,enc_rval.encoded);
	buffer_pP[0]=carrier->MIB[0];
	buffer_pP[1]=carrier->MIB[1];
	buffer_pP[2]=carrier->MIB[2];
	AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
		     enc_rval.failed_type->name, enc_rval.encoded);
	return(3);
    }

    if( (Srb_id & RAB_OFFSET ) == CCCH) {
      LOG_T(RRC,"[eNB %d] Frame %d CCCH request (Srb_id %d)\n",Mod_idP,frameP, Srb_id);

      if(RC.rrc[Mod_idP]->carrier[CC_id].Srb0.Active==0) {
        LOG_E(RRC,"[eNB %d] CCCH Not active\n",Mod_idP);
        return -1;
      }

      Srb_info=&RC.rrc[Mod_idP]->carrier[CC_id].Srb0;

      // check if data is there for MAC
      if(Srb_info->Tx_buffer.payload_size>0) { //Fill buffer
        LOG_D(RRC,"[eNB %d] CCCH (%p) has %d bytes (dest: %p, src %p)\n",Mod_idP,Srb_info,Srb_info->Tx_buffer.payload_size,buffer_pP,Srb_info->Tx_buffer.Payload);

#if 0 // defined(ENABLE_ITTI)
        {
          MessageDef *message_p;
          int ccch_size = Srb_info->Tx_buffer.payload_size;
          int sdu_size = sizeof(RRC_MAC_CCCH_DATA_REQ (message_p).sdu);

          if (ccch_size > sdu_size) {
            LOG_E(RRC, "SDU larger than CCCH SDU buffer size (%d, %d)", ccch_size, sdu_size);
            ccch_size = sdu_size;
          }

          message_p = itti_alloc_new_message (TASK_RRC_ENB, RRC_MAC_CCCH_DATA_REQ);
          RRC_MAC_CCCH_DATA_REQ (message_p).frame = frameP;
          RRC_MAC_CCCH_DATA_REQ (message_p).sdu_size = ccch_size;
          memset (RRC_MAC_CCCH_DATA_REQ (message_p).sdu, 0, CCCH_SDU_SIZE);
          memcpy (RRC_MAC_CCCH_DATA_REQ (message_p).sdu, Srb_info->Tx_buffer.Payload, ccch_size);
          RRC_MAC_CCCH_DATA_REQ (message_p).enb_index = eNB_index;

          itti_send_msg_to_task (TASK_MAC_ENB, ENB_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
        }
#endif

        memcpy(buffer_pP,Srb_info->Tx_buffer.Payload,Srb_info->Tx_buffer.payload_size);
        Sdu_size = Srb_info->Tx_buffer.payload_size;
        Srb_info->Tx_buffer.payload_size=0;
      }

      return (Sdu_size);
    }

    if( (Srb_id & RAB_OFFSET ) == PCCH) {
      LOG_T(RRC,"[eNB %d] Frame %d PCCH request (Srb_id %d)\n",Mod_idP,frameP, Srb_id);

      // check if data is there for MAC
      if(RC.rrc[Mod_idP]->carrier[CC_id].sizeof_paging[mbsfn_sync_area] > 0) { //Fill buffer
        LOG_D(RRC,"[eNB %d] PCCH (%p) has %d bytes\n",Mod_idP,&RC.rrc[Mod_idP]->carrier[CC_id].paging[mbsfn_sync_area],
               RC.rrc[Mod_idP]->carrier[CC_id].sizeof_paging[mbsfn_sync_area]);

#if 0 //defined(ENABLE_ITTI)
        {
          MessageDef *message_p;
          int pcch_size = RC.rrc[Mod_idP]->arrier[CC_id].sizeof_paging[mbsfn_sync_area];
          int sdu_size = sizeof(RRC_MAC_PCCH_DATA_REQ (message_p).sdu);

          if (pcch_size > sdu_size) {
            LOG_E(RRC, "SDU larger than PCCH SDU buffer size (%d, %d)", pcch_size, sdu_size);
            pcch_size = sdu_size;
          }

          message_p = itti_alloc_new_message (TASK_RRC_ENB, RRC_MAC_PCCH_DATA_REQ);
          RRC_MAC_PCCH_DATA_REQ (message_p).frame = frameP;
          RRC_MAC_PCCH_DATA_REQ (message_p).sdu_size = pcch_size;
          memset (RRC_MAC_PCCH_DATA_REQ (message_p).sdu, 0, PCCH_SDU_SIZE);
          memcpy (RRC_MAC_PCCH_DATA_REQ (message_p).sdu, RC.rrc[Mod_idP]->carrier[CC_id].paging[mbsfn_sync_area], pcch_size);
          RRC_MAC_PCCH_DATA_REQ (message_p).enb_index = eNB_index;

          itti_send_msg_to_task (TASK_MAC_ENB, ENB_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
        }
#endif

        memcpy(buffer_pP, RC.rrc[Mod_idP]->carrier[CC_id].paging[mbsfn_sync_area], RC.rrc[Mod_idP]->carrier[CC_id].sizeof_paging[mbsfn_sync_area]);
        Sdu_size = RC.rrc[Mod_idP]->carrier[CC_id].sizeof_paging[mbsfn_sync_area];
        RC.rrc[Mod_idP]->carrier[CC_id].sizeof_paging[mbsfn_sync_area] = 0;
      }

      return (Sdu_size);
    }

#if defined(Rel10) || defined(Rel14)

    if((Srb_id & RAB_OFFSET) == MCCH) {
      if(RC.rrc[Mod_idP]->carrier[CC_id].MCCH_MESS[mbsfn_sync_area].Active==0) {
        return 0;  // this parameter is set in function init_mcch in rrc_eNB.c
      }


#if 0 // defined(ENABLE_ITTI)
      {
        MessageDef *message_p;
        int mcch_size = RC.rrc[Mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area];
        int sdu_size = sizeof(RRC_MAC_MCCH_DATA_REQ (message_p).sdu);

        if (mcch_size > sdu_size) {
          LOG_E(RRC, "SDU larger than MCCH SDU buffer size (%d, %d)", mcch_size, sdu_size);
          mcch_size = sdu_size;
        }

        message_p = itti_alloc_new_message (TASK_RRC_ENB, RRC_MAC_MCCH_DATA_REQ);
        RRC_MAC_MCCH_DATA_REQ (message_p).frame = frameP;
        RRC_MAC_MCCH_DATA_REQ (message_p).sdu_size = mcch_size;
        memset (RRC_MAC_MCCH_DATA_REQ (message_p).sdu, 0, MCCH_SDU_SIZE);
        memcpy (RRC_MAC_MCCH_DATA_REQ (message_p).sdu,
                RC.rrc[Mod_idP]->carrier[CC_id].MCCH_MESSAGE[mbsfn_sync_area],
                mcch_size);
        RRC_MAC_MCCH_DATA_REQ (message_p).enb_index = eNB_index;
        RRC_MAC_MCCH_DATA_REQ (message_p).mbsfn_sync_area = mbsfn_sync_area;

        itti_send_msg_to_task (TASK_MAC_ENB, ENB_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
      }
#endif

      memcpy(&buffer_pP[0],
             RC.rrc[Mod_idP]->carrier[CC_id].MCCH_MESSAGE[mbsfn_sync_area],
             RC.rrc[Mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area]);

#ifdef DEBUG_RRC
      LOG_D(RRC,"[eNB %d] Frame %d : MCCH request => MCCH_MESSAGE \n",Mod_idP,frameP);

      for (i=0; i<RC.rrc[Mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area]; i++) {
        LOG_T(RRC,"%x.",buffer_pP[i]);
      }

      LOG_T(RRC,"\n");
#endif

      return (RC.rrc[Mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area]);
      //      }
      //else
      //return(0);
    }

#endif //Rel10 || Rel14

#ifdef Rel14
    if ((Srb_id & RAB_OFFSET) == BCCH_SIB1_BR){
        memcpy(&buffer_pP[0],
               RC.rrc[Mod_idP]->carrier[CC_id].SIB1_BR,
               RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1_BR);
        return (RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1_BR);
    }

    if ((Srb_id & RAB_OFFSET) == BCCH_SI_BR){ // First SI message with SIB2/3
        memcpy(&buffer_pP[0],
               RC.rrc[Mod_idP]->carrier[CC_id].SIB23_BR,
               RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23_BR);
        return (RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23_BR);
    }

#endif


  return(0);
}

//------------------------------------------------------------------------------
int8_t
mac_rrc_data_ind(
  const module_id_t     module_idP,
  const int             CC_id,
  const frame_t         frameP,
  const sub_frame_t     sub_frameP,
  const rnti_t          rntiP,
  const rb_id_t         srb_idP,
  const uint8_t*        sduP,
  const sdu_size_t      sdu_lenP,
  const uint8_t         mbsfn_sync_areaP
)
//--------------------------------------------------------------------------
{
  SRB_INFO *Srb_info;
  protocol_ctxt_t ctxt;
  sdu_size_t      sdu_size = 0;

  /* for no gcc warnings */
  (void)sdu_size;

  /*
  int si_window;
   */
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES, rntiP, frameP, sub_frameP,0);

    Srb_info = &RC.rrc[module_idP]->carrier[CC_id].Srb0;
    LOG_D(RRC,"[eNB %d] Received SDU for CCCH on SRB %d\n",module_idP,Srb_info->Srb_id);
    
#if 0 //defined(ENABLE_ITTI)
    {
      MessageDef *message_p;
      int msg_sdu_size = sizeof(RRC_MAC_CCCH_DATA_IND (message_p).sdu);

      if (sdu_lenP > msg_sdu_size) {
        LOG_E(RRC, "SDU larger than CCCH SDU buffer size (%d, %d)", sdu_lenP, msg_sdu_size);
        sdu_size = msg_sdu_size;
      } else {
        sdu_size = sdu_lenP;
      }

      message_p = itti_alloc_new_message (TASK_MAC_ENB, RRC_MAC_CCCH_DATA_IND);
      RRC_MAC_CCCH_DATA_IND (message_p).frame     = frameP;
      RRC_MAC_CCCH_DATA_IND (message_p).sub_frame = sub_frameP;
      RRC_MAC_CCCH_DATA_IND (message_p).rnti      = rntiP;
      RRC_MAC_CCCH_DATA_IND (message_p).sdu_size  = sdu_size;
      RRC_MAC_CCCH_DATA_IND (message_p).CC_id = CC_id;
      memset (RRC_MAC_CCCH_DATA_IND (message_p).sdu, 0, CCCH_SDU_SIZE);
      memcpy (RRC_MAC_CCCH_DATA_IND (message_p).sdu, sduP, sdu_size);
      LOG_D(RRC,"[eNB %d] Sending message to RRC task\n",module_idP);
      itti_send_msg_to_task (TASK_RRC_ENB, ctxt.instance, message_p);
    }
#else

    //    msg("\n******INST %d Srb_info %p, Srb_id=%d****\n\n",Mod_id,Srb_info,Srb_info->Srb_id);
    if (sdu_lenP > 0) {
      memcpy(Srb_info->Rx_buffer.Payload,sduP,sdu_lenP);
      Srb_info->Rx_buffer.payload_size = sdu_lenP;
      rrc_eNB_decode_ccch(&ctxt, Srb_info, CC_id);
    }

#endif

  return(0);

}

//------------------------------------------------------------------------------
int
mac_eNB_get_rrc_status(
  const module_id_t Mod_idP,
  const rnti_t      rntiP
)
//------------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(
                   RC.rrc[Mod_idP],
                   rntiP);

  if (ue_context_p != NULL) {
    return(ue_context_p->ue_context.Status);
  } else {
    return RRC_INACTIVE;
  }
}

void mac_eNB_rrc_ul_failure(const module_id_t Mod_instP,
			    const int CC_idP,
			    const frame_t frameP,
			    const sub_frame_t subframeP,
			    const rnti_t rntiP)
{
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(
                   RC.rrc[Mod_instP],
                   rntiP);

  if (ue_context_p != NULL) {
    LOG_I(RRC,"Frame %d, Subframe %d: UE %x UL failure, activating timer\n",frameP,subframeP,rntiP);
    if(ue_context_p->ue_context.ul_failure_timer == 0)
      ue_context_p->ue_context.ul_failure_timer=1;
  }
  else {
    LOG_W(RRC,"Frame %d, Subframe %d: UL failure: UE %x unknown \n",frameP,subframeP,rntiP);
  }
  if (rrc_agent_registered[Mod_instP]) {
    agent_rrc_xface[Mod_instP]->flexran_agent_notify_ue_state_change(Mod_instP,
								     rntiP,
								     PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED);
  }
//  rrc_mac_remove_ue(Mod_instP,rntiP);
}

void mac_eNB_rrc_uplane_failure(const module_id_t Mod_instP,
                const int CC_idP,
                const frame_t frameP,
                const sub_frame_t subframeP,
                const rnti_t rntiP)
{
    struct rrc_eNB_ue_context_s* ue_context_p = NULL;
    ue_context_p = rrc_eNB_get_ue_context(
                     RC.rrc[Mod_instP],
                     rntiP);
    if (ue_context_p != NULL) {
      LOG_I(RRC,"Frame %d, Subframe %d: UE %x U-Plane failure, activating timer\n",frameP,subframeP,rntiP);

      if(ue_context_p->ue_context.ul_failure_timer == 0)
          ue_context_p->ue_context.ul_failure_timer=19999;
    }
    else {
      LOG_W(RRC,"Frame %d, Subframe %d: U-Plane failure: UE %x unknown \n",frameP,subframeP,rntiP);
    }
}

void mac_eNB_rrc_ul_in_sync(const module_id_t Mod_instP, 
			    const int CC_idP, 
			    const frame_t frameP,
			    const sub_frame_t subframeP,
			    const rnti_t rntiP)
{
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(
                   RC.rrc[Mod_instP],
                   rntiP);

  if (ue_context_p != NULL) {
    LOG_I(RRC,"Frame %d, Subframe %d: UE %x to UL in synch\n",
          frameP, subframeP, rntiP);
    ue_context_p->ue_context.ul_failure_timer = 0;
  } else {
    LOG_E(RRC,"Frame %d, Subframe %d: UE %x unknown \n",
          frameP, subframeP, rntiP);
  }
}
