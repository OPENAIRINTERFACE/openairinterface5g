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

/*! \file l2_interface.c
 * \brief layer 2 interface, added support for FeMBMS RRC sublayer 
 * \author Javier Morgade
 * \date 2020
 * \version 1.0
 * \email: javier.morgade@ieee.org
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

#include "flexran_agent_extern.h"
#include "openair2/F1AP/f1ap_du_rrc_message_transfer.h"

extern RAN_CONTEXT_t RC;

//------------------------------------------------------------------------------
int8_t
mac_rrc_data_req(
  const module_id_t Mod_idP,
  const int         CC_id,
  const frame_t     frameP,
  const rb_id_t     Srb_id,
  const rnti_t      rnti,
  const uint8_t     Nb_tb,
  uint8_t    *const buffer_pP,
  const uint8_t     mbsfn_sync_area
)
//--------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  SRB_INFO *Srb_info;
  uint8_t Sdu_size                = 0;
  uint8_t sfn                     = (uint8_t)((frameP>>2)&0xff);

  uint8_t sfn_fembms              = (uint8_t)((frameP>>4)&0xff);
  sfn_fembms = sfn_fembms<<2;

  if (LOG_DEBUGFLAG(DEBUG_RRC)) {
    LOG_D(RRC,"[eNB %d] mac_rrc_data_req to SRB ID=%ld\n",Mod_idP,Srb_id);
  }

  eNB_RRC_INST *rrc;
  rrc_eNB_carrier_data_t *carrier;
  LTE_BCCH_BCH_Message_t *mib;
  LTE_BCCH_BCH_Message_MBMS_t *mib_fembms;
  rrc     = RC.rrc[Mod_idP];
  carrier = &rrc->carrier[0];
  mib     = &carrier->mib;
  mib_fembms     = &carrier->mib_fembms;

  if((Srb_id & RAB_OFFSET) == BCCH_SI_MBMS){
    if (frameP%8 == 0) {
      memcpy(&buffer_pP[0],
             RC.rrc[Mod_idP]->carrier[CC_id].SIB1_MBMS,
             RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1_MBMS);

      if (LOG_DEBUGFLAG(DEBUG_RRC)) {
        LOG_T(RRC,"[eNB %d] Frame %d : BCCH request => SIB 1 MBMS\n",Mod_idP,frameP);

        for (int i=0; i<RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1_MBMS; i++) {
          LOG_T(RRC,"%x.",buffer_pP[i]);
        }

        LOG_T(RRC,"\n");
      } /* LOG_DEBUGFLAG(DEBUG_RRC) */

      return (RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1_MBMS);
    }
  }

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

      if (LOG_DEBUGFLAG(DEBUG_RRC)) {
        LOG_T(RRC,"[eNB %d] Frame %d : BCCH request => SIB 1\n",Mod_idP,frameP);

        for (int i=0; i<RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1; i++) {
          LOG_T(RRC,"%x.",buffer_pP[i]);
        }

        LOG_T(RRC,"\n");
      } /* LOG_DEBUGFLAG(DEBUG_RRC) */

      return (RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1);
    } // All RFN mod 8 transmit SIB2-3 in SF 5
    else if ((frameP%8) == 1) {
      memcpy(&buffer_pP[0],
             RC.rrc[Mod_idP]->carrier[CC_id].SIB23,
             RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23);

      if (LOG_DEBUGFLAG(DEBUG_RRC)) {
        LOG_T(RRC,"[eNB %d] Frame %d BCCH request => SIB 2-3\n",Mod_idP,frameP);

        for (int i=0; i<RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23; i++) {
          LOG_T(RRC,"%x.",buffer_pP[i]);
        }

        LOG_T(RRC,"\n");
      } /* LOG_DEBUGFLAG(DEBUG_RRC) */

      return(RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23);
    } else {
      return(0);
    }
  }

  if( (Srb_id & RAB_OFFSET ) == MIBCH) {
    mib->message.systemFrameNumber.buf = &sfn;
    enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_BCH_Message,
                                     NULL,
                                     (void *)mib,
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

  if( (Srb_id & RAB_OFFSET ) == MIBCH_MBMS) {
    mib_fembms->message.systemFrameNumber_r14.buf = &sfn_fembms;
    enc_rval = uper_encode_to_buffer(&asn_DEF_LTE_BCCH_BCH_Message_MBMS,
                                     NULL,
                                     (void *)mib_fembms,
                                     carrier->MIB_FeMBMS,
                                     24);
    LOG_I(RRC,"Encoded MIB MBMS for frame %d (%p), bits %lu %x,%x,%x %x bits_unused %d\n",sfn_fembms>>2,carrier->MIB_FeMBMS,enc_rval.encoded,carrier->MIB_FeMBMS[0],carrier->MIB_FeMBMS[1],carrier->MIB_FeMBMS[2],mib_fembms->message.systemFrameNumber_r14.buf[0],mib_fembms->message.systemFrameNumber_r14.bits_unused);
    buffer_pP[0]=carrier->MIB_FeMBMS[0];
    buffer_pP[1]=carrier->MIB_FeMBMS[1];
    buffer_pP[2]=carrier->MIB_FeMBMS[2];
    AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
                 enc_rval.failed_type->name, enc_rval.encoded);
    return(3);
  }

  if( (Srb_id & RAB_OFFSET ) == CCCH) {
    struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[Mod_idP],rnti);

    if (ue_context_p == NULL) return(0);

    eNB_RRC_UE_t *ue_p = &ue_context_p->ue_context;
    LOG_T(RRC,"[eNB %d] Frame %d CCCH request (Srb_id %ld, rnti %x)\n",Mod_idP,frameP, Srb_id,rnti);
    Srb_info=&ue_p->Srb0;

    // check if data is there for MAC
    if(Srb_info->Tx_buffer.payload_size>0) { //Fill buffer
      LOG_D(RRC,"[eNB %d] CCCH (%p) has %d bytes (dest: %p, src %p)\n",Mod_idP,Srb_info,Srb_info->Tx_buffer.payload_size,buffer_pP,Srb_info->Tx_buffer.Payload);
      memcpy(buffer_pP,Srb_info->Tx_buffer.Payload,Srb_info->Tx_buffer.payload_size);
      Sdu_size = Srb_info->Tx_buffer.payload_size;
      Srb_info->Tx_buffer.payload_size=0;
    }

    return (Sdu_size);
  }

  if( (Srb_id & RAB_OFFSET ) == PCCH) {
    LOG_T(RRC,"[eNB %d] Frame %d PCCH request (Srb_id %ld)\n",Mod_idP,frameP, Srb_id);

    // check if data is there for MAC
    if(RC.rrc[Mod_idP]->carrier[CC_id].sizeof_paging[mbsfn_sync_area] > 0) { //Fill buffer
      LOG_D(RRC,"[eNB %d] PCCH (%p) has %d bytes\n",Mod_idP,&RC.rrc[Mod_idP]->carrier[CC_id].paging[mbsfn_sync_area],
            RC.rrc[Mod_idP]->carrier[CC_id].sizeof_paging[mbsfn_sync_area]);
      memcpy(buffer_pP, RC.rrc[Mod_idP]->carrier[CC_id].paging[mbsfn_sync_area], RC.rrc[Mod_idP]->carrier[CC_id].sizeof_paging[mbsfn_sync_area]);
      Sdu_size = RC.rrc[Mod_idP]->carrier[CC_id].sizeof_paging[mbsfn_sync_area];
      RC.rrc[Mod_idP]->carrier[CC_id].sizeof_paging[mbsfn_sync_area] = 0;
    }

    return (Sdu_size);
  }

  if((Srb_id & RAB_OFFSET) == MCCH) {
    if(RC.rrc[Mod_idP]->carrier[CC_id].MCCH_MESS[mbsfn_sync_area].Active==0) {
      return 0;  // this parameter is set in function init_mcch in rrc_eNB.c
    }

    memcpy(&buffer_pP[0],
           RC.rrc[Mod_idP]->carrier[CC_id].MCCH_MESSAGE[mbsfn_sync_area],
           RC.rrc[Mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area]);

      LOG_D(RRC,"[eNB %d] Frame %d : MCCH request => MCCH_MESSAGE \n",Mod_idP,frameP);

    if (LOG_DEBUGFLAG(DEBUG_RRC)) {
      LOG_D(RRC,"[eNB %d] Frame %d : MCCH request => MCCH_MESSAGE \n",Mod_idP,frameP);

      for (int i=0; i<RC.rrc[Mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area]; i++) {
        LOG_T(RRC,"%x.",buffer_pP[i]);
      }

      LOG_T(RRC,"\n");
    } /* LOG_DEBUGFLAG(DEBUG_RRC) */

    return (RC.rrc[Mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area]);
  }

  if((Srb_id & RAB_OFFSET) == MCCH_COUNTING) {
    if(RC.rrc[Mod_idP]->carrier[CC_id].MCCH_MESS_COUNTING[mbsfn_sync_area].Active==0) {
      return 0;  // this parameter is set in function init_mcch in rrc_eNB.c
    }

    memcpy(&buffer_pP[0],
           RC.rrc[Mod_idP]->carrier[CC_id].MCCH_MESSAGE_COUNTING[mbsfn_sync_area],
           RC.rrc[Mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE_COUNTING[mbsfn_sync_area]);

    if (LOG_DEBUGFLAG(DEBUG_RRC)) {
      LOG_W(RRC,"[eNB %d] Frame %d : MCCH request => MCCH_MESSAGE_COUNTING \n",Mod_idP,frameP);

      for (int i=0; i<RC.rrc[Mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE_COUNTING[mbsfn_sync_area]; i++) {
        LOG_T(RRC,"%x.",buffer_pP[i]);
      }

      LOG_T(RRC,"\n");
    } /* LOG_DEBUGFLAG(DEBUG_RRC) */

    return (RC.rrc[Mod_idP]->carrier[CC_id].sizeof_MCCH_MESSAGE_COUNTING[mbsfn_sync_area]);
  }


  if ((Srb_id & RAB_OFFSET) == BCCH_SIB1_BR) {
    memcpy(&buffer_pP[0],
           RC.rrc[Mod_idP]->carrier[CC_id].SIB1_BR,
           RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1_BR);
    return (RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB1_BR);
  }

  if ((Srb_id & RAB_OFFSET) == BCCH_SI_BR) { // First SI message with SIB2/3
    memcpy(&buffer_pP[0],
           RC.rrc[Mod_idP]->carrier[CC_id].SIB23_BR,
           RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23_BR);
    return (RC.rrc[Mod_idP]->carrier[CC_id].sizeof_SIB23_BR);
  }

  return(0);
}


//------------------------------------------------------------------------------
int8_t
mac_rrc_data_ind(
  const module_id_t     module_idP,
  const int             CC_id,
  const frame_t         frameP,
  const sub_frame_t     sub_frameP,
  const int             UE_id,
  const rnti_t          rntiP,
  const rb_id_t         srb_idP,
  const uint8_t        *sduP,
  const sdu_size_t      sdu_lenP,
  const uint8_t         mbsfn_sync_areaP,
  const boolean_t   brOption
)
//--------------------------------------------------------------------------
{
  if (NODE_IS_DU(RC.rrc[module_idP]->node_type)) {
    LOG_W(RRC,"[DU %d][RAPROC] Received SDU for CCCH on SRB %ld length %d for UE id %d RNTI %x \n",
          module_idP, srb_idP, sdu_lenP, UE_id, rntiP);
    /* do ITTI message */
    DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(
      module_idP,
      CC_id,
      UE_id,
      rntiP,
      sduP,
      sdu_lenP,
      NULL,
      0
    );
    return(0);
  }

  //SRB_INFO *Srb_info;
  protocol_ctxt_t ctxt;
  sdu_size_t      sdu_size = 0;
  /* for no gcc warnings */
  (void)sdu_size;
  /*
  int si_window;
   */
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES, rntiP, frameP, sub_frameP,0);

  if((srb_idP & RAB_OFFSET) == CCCH) {
    LOG_D(RRC, "[eNB %d] Received SDU for CCCH on SRB %ld\n", module_idP, srb_idP);
    ctxt.brOption = brOption;

    /*Srb_info = &RC.rrc[module_idP]->carrier[CC_id].Srb0;
    if (sdu_lenP > 0) {
      memcpy(Srb_info->Rx_buffer.Payload,sduP,sdu_lenP);
      Srb_info->Rx_buffer.payload_size = sdu_lenP;
      rrc_eNB_decode_ccch(&ctxt, Srb_info, CC_id);
    }*/
    if (sdu_lenP > 0)  rrc_eNB_decode_ccch(&ctxt, sduP, sdu_lenP, CC_id);
  }

  if((srb_idP & RAB_OFFSET) == DCCH) {
    struct rrc_eNB_ue_context_s    *ue_context_p = NULL;
    ue_context_p = rrc_eNB_get_ue_context(RC.rrc[ctxt.module_id],rntiP);

    if(ue_context_p) {
      if (ue_context_p->ue_context.Status != RRC_RECONFIGURED) {
        LOG_E(RRC,"[eNB %d] Received C-RNTI ,but UE %x status(%d) not RRC_RECONFIGURED\n",module_idP,rntiP,ue_context_p->ue_context.Status);
        return (-1);
      } 
      rrc_eNB_generate_defaultRRCConnectionReconfiguration(&ctxt,ue_context_p,0);
      ue_context_p->ue_context.Status = RRC_RECONFIGURED;
    }
  }

  return(0);
}

//------------------------------------------------------------------------------
/*
* Get RRC status (Connected, Idle...) of UE from RNTI
*/
int
mac_eNB_get_rrc_status(
  const module_id_t Mod_idP,
  const rnti_t      rntiP
)
//------------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s *ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(RC.rrc[Mod_idP], rntiP);

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
                            const rnti_t rntiP) {
  struct rrc_eNB_ue_context_s *ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(
                   RC.rrc[Mod_instP],
                   rntiP);

  if (ue_context_p != NULL) {
    LOG_I(RRC,"Frame %d, Subframe %d: UE %x UL failure, activating timer\n",frameP,subframeP,rntiP);

    if(ue_context_p->ue_context.ul_failure_timer == 0)
      ue_context_p->ue_context.ul_failure_timer=1;
  } else {
    LOG_W(RRC,"Frame %d, Subframe %d: UL failure: UE %x unknown \n",frameP,subframeP,rntiP);
  }

  if (flexran_agent_get_rrc_xface(Mod_instP)) {
    flexran_agent_get_rrc_xface(Mod_instP)->flexran_agent_notify_ue_state_change(Mod_instP,
        rntiP, PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED);
  }

  //rrc_mac_remove_ue(Mod_instP,rntiP);
}

void mac_eNB_rrc_uplane_failure(const module_id_t Mod_instP,
                                const int CC_idP,
                                const frame_t frameP,
                                const sub_frame_t subframeP,
                                const rnti_t rntiP) {
  struct rrc_eNB_ue_context_s *ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(
                   RC.rrc[Mod_instP],
                   rntiP);

  if (ue_context_p != NULL) {
    LOG_I(RRC,"Frame %d, Subframe %d: UE %x U-Plane failure, activating timer\n",frameP,subframeP,rntiP);

    if(ue_context_p->ue_context.ul_failure_timer == 0)
      ue_context_p->ue_context.ul_failure_timer=19999;
  } else {
    LOG_W(RRC,"Frame %d, Subframe %d: U-Plane failure: UE %x unknown \n",frameP,subframeP,rntiP);
  }
}

void mac_eNB_rrc_ul_in_sync(const module_id_t Mod_instP,
                            const int CC_idP,
                            const frame_t frameP,
                            const sub_frame_t subframeP,
                            const rnti_t rntiP) {
  struct rrc_eNB_ue_context_s *ue_context_p = NULL;
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
