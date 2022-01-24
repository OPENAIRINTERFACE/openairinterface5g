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

/*! \file l2_nr_interface.c
 * \brief layer 2 interface, used to support different RRC sublayer
 * \author Raymond Knopp and Navid Nikaein, WEI-TAI CHEN
 * \date 2010-2014, 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: raymond.knopp@eurecom.fr, kroempa@gmail.com
 */

#include <f1ap_du_rrc_message_transfer.h>
#include "platform_types.h"
#include "nr_rrc_defs.h"
#include "nr_rrc_extern.h"
#include "common/utils/LOG/log.h"
#include "pdcp.h"
#include "msc.h"
#include "common/ran_context.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"

#include "intertask_interface.h"

#include "NR_MIB.h"
#include "NR_BCCH-BCH-Message.h"
#include "rrc_gNB_UE_context.h"
#include <openair2/RRC/NR/MESSAGES/asn1_msg.h>
#include <openair2/F1AP/f1ap_du_rrc_message_transfer.h>


extern RAN_CONTEXT_t RC;

int generate_pdcch_ConfigSIB1(NR_PDCCH_ConfigSIB1_t *pdcch_ConfigSIB1,
                              long ssbSubcarrierSpacing,
                              long subCarrierSpacingCommon,
                              channel_bandwidth_t min_channel_bw) {

  nr_ssb_and_cset_mux_pattern_type_t mux_pattern = 0;

  switch (ssbSubcarrierSpacing) {

    case NR_SubcarrierSpacing_kHz15:
      if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz15) {
        pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_1_NUM_INDEXES;
        mux_pattern = table_38213_13_1_c1[pdcch_ConfigSIB1->controlResourceSetZero];
      } else if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz30) {
        pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_2_NUM_INDEXES;
        mux_pattern = table_38213_13_2_c1[pdcch_ConfigSIB1->controlResourceSetZero];
      } else {
        AssertFatal(true,"Invalid subCarrierSpacingCommon\n");
      }
      break;

    case NR_SubcarrierSpacing_kHz30:
      if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz15) {

        if ( (min_channel_bw == bw_5MHz) || (min_channel_bw == bw_10MHz) ) {
          pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_3_NUM_INDEXES;
          mux_pattern = table_38213_13_3_c1[pdcch_ConfigSIB1->controlResourceSetZero];
        } else if (min_channel_bw == bw_40MHz) {
          pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_5_NUM_INDEXES;
          mux_pattern = table_38213_13_5_c1[pdcch_ConfigSIB1->controlResourceSetZero];
        } else {
          AssertFatal(true,"Invalid min_bandwidth\n");
        }

      } else if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz30) {

        if ( (min_channel_bw == bw_5MHz) || (min_channel_bw == bw_10MHz) ) {
          pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_4_NUM_INDEXES;
          mux_pattern = table_38213_13_4_c1[pdcch_ConfigSIB1->controlResourceSetZero];
        } else if (min_channel_bw == bw_40MHz) {
          pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_6_NUM_INDEXES;
          mux_pattern = table_38213_13_6_c1[pdcch_ConfigSIB1->controlResourceSetZero];
        } else {
          AssertFatal(true,"Invalid min_bandwidth\n");
        }

      } else {
        AssertFatal(true,"Invalid subCarrierSpacingCommon\n");
      }
      break;

    case NR_SubcarrierSpacing_kHz120:
      if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz60) {
        pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_7_NUM_INDEXES;
        mux_pattern = table_38213_13_7_c1[pdcch_ConfigSIB1->controlResourceSetZero];
      } else if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz120) {
        pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_8_NUM_INDEXES;
        mux_pattern = table_38213_13_8_c1[pdcch_ConfigSIB1->controlResourceSetZero];
      } else {
        AssertFatal(true,"Invalid subCarrierSpacingCommon\n");
      }
      break;

    case NR_SubcarrierSpacing_kHz240:
      if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz60) {
        pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_9_NUM_INDEXES;
        mux_pattern = table_38213_13_9_c1[pdcch_ConfigSIB1->controlResourceSetZero];
      } else if (subCarrierSpacingCommon == NR_SubcarrierSpacing_kHz120) {
        pdcch_ConfigSIB1->controlResourceSetZero = rand() % TABLE_38213_13_10_NUM_INDEXES;
        mux_pattern = table_38213_13_10_c1[pdcch_ConfigSIB1->controlResourceSetZero];
      } else {
        AssertFatal(true,"Invalid subCarrierSpacingCommon\n");
      }
      break;

    default:
      AssertFatal(true,"Invalid ssbSubcarrierSpacing\n");
      break;
  }


  frequency_range_t frequency_range = FR1;
  if(ssbSubcarrierSpacing>=60) {
    frequency_range = FR2;
  }

  pdcch_ConfigSIB1->searchSpaceZero = 0;
  if(mux_pattern == NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 && frequency_range == FR1){
    pdcch_ConfigSIB1->searchSpaceZero = rand() % TABLE_38213_13_11_NUM_INDEXES;
  }
  if(mux_pattern == NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 && frequency_range == FR2){
    pdcch_ConfigSIB1->searchSpaceZero = rand() % TABLE_38213_13_12_NUM_INDEXES;
  }

  return 0;
}

int
nr_rrc_mac_remove_ue(module_id_t mod_idP,
                  rnti_t rntiP){
  // todo
  return 0;
}

//------------------------------------------------------------------------------
uint8_t
nr_rrc_data_req(
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
    LOG_D(RRC,"sdu_sizeP == 255");
    return FALSE;
  }

  MSC_LOG_TX_MESSAGE(
    ctxt_pP->enb_flag ? MSC_RRC_GNB : MSC_RRC_UE,
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
                     ctxt_pP->enb_flag ? TASK_RRC_GNB : TASK_RRC_UE,
                     ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE,
                     sdu_sizeP);
  memcpy (message_buffer, buffer_pP, sdu_sizeP);
  message_p = itti_alloc_new_message (ctxt_pP->enb_flag ? TASK_RRC_GNB : TASK_RRC_UE, 0, RRC_DCCH_DATA_REQ);
  RRC_DCCH_DATA_REQ (message_p).frame     = ctxt_pP->frame;
  RRC_DCCH_DATA_REQ (message_p).enb_flag  = ctxt_pP->enb_flag;
  RRC_DCCH_DATA_REQ (message_p).rb_id     = rb_idP;
  RRC_DCCH_DATA_REQ (message_p).muip      = muiP;
  RRC_DCCH_DATA_REQ (message_p).confirmp  = confirmP;
  RRC_DCCH_DATA_REQ (message_p).sdu_size  = sdu_sizeP;
  RRC_DCCH_DATA_REQ (message_p).sdu_p     = message_buffer;
  //memcpy (NR_RRC_DCCH_DATA_REQ (message_p).sdu_p, buffer_pP, sdu_sizeP);
  RRC_DCCH_DATA_REQ (message_p).mode      = modeP;
  RRC_DCCH_DATA_REQ (message_p).module_id = ctxt_pP->module_id;
  RRC_DCCH_DATA_REQ (message_p).rnti      = ctxt_pP->rnti;
  RRC_DCCH_DATA_REQ (message_p).eNB_index = ctxt_pP->eNB_index;
  itti_send_msg_to_task (
    ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE,
    ctxt_pP->instance,
    message_p);
  LOG_I(NR_RRC,"send RRC_DCCH_DATA_REQ to PDCP\n");

  /* Hack: only trigger PDCP if in CU, otherwise it is triggered by RU threads
   * Ideally, PDCP would not neet to be triggered like this but react to ITTI
   * messages automatically */
  if (ctxt_pP->enb_flag && NODE_IS_CU(RC.nrrrc[ctxt_pP->module_id]->node_type))
    pdcp_run(ctxt_pP);

  return TRUE; // TODO should be changed to a CNF message later, currently RRC lite does not used the returned value anyway.
}

int    mac_rrc_nr_data_req(const module_id_t Mod_idP,
                           const int         CC_id,
                           const frame_t     frameP,
                           const rb_id_t     Srb_id,
                           const rnti_t      rnti,
                           const uint8_t     Nb_tb,
                           uint8_t *const    buffer_pP ){

#ifdef DEBUG_RRC
  LOG_D(RRC,"[eNB %d] mac_rrc_data_req to SRB ID=%ld\n",Mod_idP,Srb_id);
#endif

    // MIBCH
    if ((Srb_id & RAB_OFFSET) == MIBCH) {

      asn_enc_rval_t enc_rval;
      uint8_t sfn_msb = (uint8_t)((frameP>>4)&0x3f);
      rrc_gNB_carrier_data_t *carrier = &RC.nrrrc[Mod_idP]->carrier;
      NR_BCCH_BCH_Message_t *mib = &carrier->mib;

        // Currently we are getting the pdcch_ConfigSIB1 from the configuration file.
        // Uncomment this function for a dynamic pdcch_ConfigSIB1.
        //channel_bandwidth_t min_channel_bw = bw_10MHz; // Must be obtained based on TS 38.101-1 Table 5.3.5-1
        //generate_pdcch_ConfigSIB1(carrier->pdcch_ConfigSIB1,
        //                          *carrier->servingcellconfigcommon->ssbSubcarrierSpacing,
        //                          carrier->mib.message.choice.mib->subCarrierSpacingCommon,
        //                          min_channel_bw);

        mib->message.choice.mib->pdcch_ConfigSIB1.controlResourceSetZero = carrier->pdcch_ConfigSIB1->controlResourceSetZero;
        mib->message.choice.mib->pdcch_ConfigSIB1.searchSpaceZero = carrier->pdcch_ConfigSIB1->searchSpaceZero;

        mib->message.choice.mib->systemFrameNumber.buf[0] = sfn_msb << 2;
        enc_rval = uper_encode_to_buffer(&asn_DEF_NR_BCCH_BCH_Message,
                                         NULL,
                                         (void *) mib,
                                         carrier->MIB,
                                         24);
        LOG_D(NR_RRC, "Encoded MIB for frame %d sfn_msb %d (%p), bits %lu\n", frameP, sfn_msb, carrier->MIB,
              enc_rval.encoded);
        buffer_pP[0] = carrier->MIB[0];
        buffer_pP[1] = carrier->MIB[1];
        buffer_pP[2] = carrier->MIB[2];
        LOG_D(NR_RRC, "MIB PDU buffer_pP[0]=%x , buffer_pP[1]=%x, buffer_pP[2]=%x\n", buffer_pP[0], buffer_pP[1],
              buffer_pP[2]);
        AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
                     enc_rval.failed_type->name, enc_rval.encoded);
        return (3);
    }

  // TODO BCCH SIB1 SIBs
  if ((Srb_id & RAB_OFFSET ) == BCCH) {
    memcpy(&buffer_pP[0],
           RC.nrrrc[Mod_idP]->carrier.SIB1,
           RC.nrrrc[Mod_idP]->carrier.sizeof_SIB1);

    return RC.nrrrc[Mod_idP]->carrier.sizeof_SIB1;
  }

  // CCCH
  if( (Srb_id & RAB_OFFSET ) == CCCH) {

    char *payload_pP;
    uint8_t Sdu_size = 0;
    struct rrc_gNB_ue_context_s *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[Mod_idP], rnti);

    LOG_D(NR_RRC,"[gNB %d] Frame %d CCCH request (Srb_id %ld)\n", Mod_idP, frameP, Srb_id);

    AssertFatal(ue_context_p!=NULL,"failed to get ue_context, rnti %x\n",rnti);
    int payload_size = ue_context_p->ue_context.Srb0.Tx_buffer.payload_size;

    // check if data is there for MAC
    if (payload_size > 0) {
      payload_pP = ue_context_p->ue_context.Srb0.Tx_buffer.Payload;
      LOG_D(NR_RRC,"[gNB %d] CCCH has %d bytes (dest: %p, src %p)\n", Mod_idP, payload_size, buffer_pP, payload_pP);
      // Fill buffer
      memcpy((void *)buffer_pP, (void*)payload_pP, payload_size);
      Sdu_size = payload_size;
      ue_context_p->ue_context.Srb0.Tx_buffer.payload_size = 0;
    }
    return Sdu_size;
  }

  return(0);

}

int8_t nr_mac_rrc_data_ind(const module_id_t     module_idP,
                           const int             CC_id,
                           const frame_t         frameP,
                           const sub_frame_t     sub_frameP,
                           const int             UE_id,
                           const rnti_t          rntiP,
                           const rb_id_t         srb_idP,
                           const uint8_t        *sduP,
                           const sdu_size_t      sdu_lenP,
                           const boolean_t   brOption) {

  if (NODE_IS_DU(RC.nrrrc[module_idP]->node_type)) {
    LOG_W(RRC,"[DU %d][RAPROC] Received SDU for CCCH on SRB %ld length %d for UE id %d RNTI %x \n",
          module_idP, srb_idP, sdu_lenP, UE_id, rntiP);

    // Generate DUtoCURRCContainer
    // call do_RRCSetup like full procedure and extract masterCellGroup
    NR_CellGroupConfig_t cellGroupConfig;
    NR_ServingCellConfigCommon_t *scc=RC.nrrrc[module_idP]->carrier.servingcellconfigcommon;
    memset(&cellGroupConfig,0,sizeof(cellGroupConfig));

    fill_initial_cellGroupConfig(rntiP,&cellGroupConfig,scc,&RC.nrrrc[module_idP]->carrier);
    MessageDef* tmp=itti_alloc_new_message_sized(TASK_RRC_GNB, 0, F1AP_INITIAL_UL_RRC_MESSAGE, sizeof(f1ap_initial_ul_rrc_message_t) + sdu_lenP);
    f1ap_initial_ul_rrc_message_t *msg = &F1AP_INITIAL_UL_RRC_MESSAGE(tmp);

    asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig,
						    NULL,
						    (void *)&cellGroupConfig,
						    msg->du2cu_rrc_container,
						    1024); //sizeof(msg->du2cu_rrc_container));

    if (enc_rval.encoded == -1) {
      LOG_E(F1AP,"Could not encoded cellGroupConfig, failed element %s\n",enc_rval.failed_type->name);
      exit(-1);
    }
    /* do ITTI message */
    msg->du2cu_rrc_container_length = (enc_rval.encoded+7)/8;
    msg->crnti=rntiP;
    msg->rrc_container=(uint8_t*) (msg+1); // Made extra room after the struct with itti_alloc_msg_sized()
    memcpy(msg->rrc_container, sduP, sdu_lenP);
    msg->rrc_container_length=sdu_lenP;
    itti_send_msg_to_task(TASK_DU_F1, 0, tmp);
    
    struct rrc_gNB_ue_context_s *ue_context_p = rrc_gNB_allocate_new_UE_context(RC.nrrrc[module_idP]);
    ue_context_p->ue_id_rnti                    = rntiP;
    ue_context_p->ue_context.rnti               = rntiP;
    ue_context_p->ue_context.random_ue_identity = rntiP;
    ue_context_p->ue_context.Srb0.Active        = 1;
    RB_INSERT(rrc_nr_ue_tree_s, &RC.nrrrc[module_idP]->rrc_ue_head, ue_context_p);
    
    return(0);
  }

  protocol_ctxt_t ctxt;
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, GNB_FLAG_YES, rntiP, frameP, sub_frameP,0);

  if((srb_idP & RAB_OFFSET) == CCCH) {
    LOG_D(NR_RRC, "[gNB %d] Received SDU for CCCH on SRB %ld\n", module_idP, srb_idP);
    ctxt.brOption = brOption;
    if (sdu_lenP > 0) {
      nr_rrc_gNB_decode_ccch(&ctxt, sduP, sdu_lenP, NULL, CC_id);
    }
  }

  return 0;
}

void nr_mac_gNB_rrc_ul_failure(const module_id_t Mod_instP,
                               const int CC_idP,
                               const frame_t frameP,
                               const sub_frame_t subframeP,
                               const rnti_t rntiP) {
  struct rrc_gNB_ue_context_s *ue_context_p = NULL;
  ue_context_p = rrc_gNB_get_ue_context(
                   RC.nrrrc[Mod_instP],
                   rntiP);

  if (ue_context_p != NULL) {
    LOG_D(RRC,"Frame %d, Subframe %d: UE %x UL failure, activating timer\n",frameP,subframeP,rntiP);
    if(ue_context_p->ue_context.ul_failure_timer == 0)
      ue_context_p->ue_context.ul_failure_timer=1;
  } else {
    LOG_D(RRC,"Frame %d, Subframe %d: UL failure: UE %x unknown \n",frameP,subframeP,rntiP);
  }
}

void nr_mac_gNB_rrc_ul_failure_reset(const module_id_t Mod_instP,
                                     const frame_t frameP,
                                     const sub_frame_t subframeP,
                                     const rnti_t rntiP) {
  struct rrc_gNB_ue_context_s *ue_context_p = NULL;
  ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[Mod_instP], rntiP);
  if (ue_context_p != NULL) {
    LOG_W(RRC,"Frame %d, Subframe %d: UE %x UL failure reset, deactivating timer\n",frameP,subframeP,rntiP);
    ue_context_p->ue_context.ul_failure_timer=0;
  } else {
    LOG_W(RRC,"Frame %d, Subframe %d: UL failure reset: UE %x unknown \n",frameP,subframeP,rntiP);
  }
}
