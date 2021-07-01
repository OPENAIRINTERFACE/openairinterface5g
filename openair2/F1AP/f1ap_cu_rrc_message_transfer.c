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

/*! \file f1ap_cu_rrc_message_transfer.c
 * \brief f1ap rrc message transfer for CU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"
#include "f1ap_encoder.h"
#include "f1ap_decoder.h"
#include "f1ap_itti_messaging.h"
#include "f1ap_cu_rrc_message_transfer.h"
#include "common/ran_context.h"
#include "openair3/UTILS/conversions.h"

// undefine C_RNTI from
// openair1/PHY/LTE_TRANSPORT/transport_common.h which
// replaces in ie->value.choice.C_RNTI, causing
// a compile error
#undef C_RNTI 


// Bing Kai: create CU and DU context, and put all the information there.
uint64_t        du_ue_f1ap_id = 0;
uint32_t        f1ap_assoc_id = 0;
uint32_t        f1ap_stream = 0;


extern f1ap_cudu_inst_t f1ap_cu_inst[MAX_eNB];

/*
    Initial UL RRC Message Transfer
*/

extern RAN_CONTEXT_t RC;

int CU_handle_INITIAL_UL_RRC_MESSAGE_TRANSFER(instance_t             instance,
                                              uint32_t               assoc_id,
                                              uint32_t               stream,
                                              F1AP_F1AP_PDU_t       *pdu) {

  LOG_D(F1AP, "CU_handle_INITIAL_UL_RRC_MESSAGE_TRANSFER\n");
  // decode the F1 message
  // get the rrc message from the contauiner 
  // call func rrc_eNB_decode_ccch: <-- needs some update here
  MessageDef                            *message_p;
  F1AP_InitialULRRCMessageTransfer_t    *container;
  F1AP_InitialULRRCMessageTransferIEs_t *ie;
  
  rnti_t          rnti;
  sdu_size_t      ccch_sdu_len;
  int             CC_id =0;
  

  DevAssert(pdu != NULL);
  
  if (stream != 0) {
    LOG_E(F1AP, "[SCTP %d] Received F1 on stream != 0 (%d)\n",
          assoc_id, stream);
    return -1;
  }
  // TODO: use context 
  f1ap_stream    = stream;
  f1ap_assoc_id = assoc_id;

  container = &pdu->choice.initiatingMessage->value.choice.InitialULRRCMessageTransfer;

  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  LOG_D(F1AP, "du_ue_f1ap_id %lu \n", du_ue_f1ap_id);

  /* NRCGI 
  * TODO: process NRCGI
  */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_NRCGI, true);


  uint64_t nr_cellid;
  BIT_STRING_TO_NR_CELL_IDENTITY(&ie->value.choice.NRCGI.nRCellIdentity,nr_cellid);
					       
  /* RNTI */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_C_RNTI, true);
  rnti = ie->value.choice.C_RNTI;

  /* RRC Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, true);
  AssertFatal(ie!=NULL,"RRCContainer is missing\n");
  // create an ITTI message and copy SDU
  if (RC.nrrrc && RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instance)]->node_type == ngran_gNB_CU) {
    message_p = itti_alloc_new_message (TASK_CU_F1, 0, NR_RRC_MAC_CCCH_DATA_IND);
    memset (NR_RRC_MAC_CCCH_DATA_IND (message_p).sdu, 0, CCCH_SDU_SIZE);
    ccch_sdu_len = ie->value.choice.RRCContainer.size;
    memcpy(NR_RRC_MAC_CCCH_DATA_IND (message_p).sdu, ie->value.choice.RRCContainer.buf,
           ccch_sdu_len);
  } else {
    message_p = itti_alloc_new_message (TASK_CU_F1, 0, RRC_MAC_CCCH_DATA_IND);
    memset (RRC_MAC_CCCH_DATA_IND (message_p).sdu, 0, CCCH_SDU_SIZE);
    ccch_sdu_len = ie->value.choice.RRCContainer.size;
    memcpy(RRC_MAC_CCCH_DATA_IND (message_p).sdu, ie->value.choice.RRCContainer.buf,
           ccch_sdu_len);
  }

  if (RC.nrrrc && RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instance)]->node_type == ngran_gNB_CU) {

    LOG_D(F1AP, "%s() RRCContainer (CCCH) size %ld: ", __func__, ie->value.choice.RRCContainer.size);
    //for (int i = 0; i < ie->value.choice.RRCContainer.size; i++)
    //  printf("%02x ", RRC_MAC_CCCH_DATA_IND (message_p).sdu[i]);
    //printf("\n");

    /* DUtoCURRCContainer */
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_DUtoCURRCContainer, true);
    if (ie) {
      NR_RRC_MAC_CCCH_DATA_IND (message_p).du_to_cu_rrc_container = malloc(sizeof(OCTET_STRING_t));
      NR_RRC_MAC_CCCH_DATA_IND (message_p).du_to_cu_rrc_container->size = ie->value.choice.DUtoCURRCContainer.size;
      NR_RRC_MAC_CCCH_DATA_IND (message_p).du_to_cu_rrc_container->buf = malloc(
          ie->value.choice.DUtoCURRCContainer.size);
      memcpy(NR_RRC_MAC_CCCH_DATA_IND (message_p).du_to_cu_rrc_container->buf,
             ie->value.choice.DUtoCURRCContainer.buf,
             ie->value.choice.DUtoCURRCContainer.size);
    }
  }

  // Find instance from nr_cellid
  int rrc_inst = -1;
  if (RC.nrrrc && RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instance)]->node_type == ngran_gNB_CU) {
    for (int i=0;i<RC.nb_nr_inst;i++) {
      // first get RRC instance (note, no the ITTI instance)
      gNB_RRC_INST *rrc = RC.nrrrc[i];
      if (rrc->nr_cellid == nr_cellid) {
        rrc_inst = i;
        break;
      }
    }
  } else {
    for (int i=0;i<RC.nb_inst;i++) {
          // first get RRC instance (note, no the ITTI instance)
      eNB_RRC_INST *rrc = RC.rrc[i];
      if (rrc->nr_cellid == nr_cellid) {
        rrc_inst = i;
        break;
      }
    }
  }
  AssertFatal(rrc_inst>=0,"couldn't find an RRC instance for nr_cell %llu\n",(unsigned long long int)nr_cellid);

  int f1ap_uid = f1ap_add_ue(&f1ap_cu_inst[rrc_inst], rrc_inst, CC_id, 0, rnti);
  if (f1ap_uid  < 0 ) {
    LOG_E(F1AP, "Failed to add UE \n");
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }
  f1ap_cu_inst[rrc_inst].f1ap_ue[f1ap_uid].du_ue_f1ap_id = du_ue_f1ap_id;

  if (RC.nrrrc && RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instance)]->node_type == ngran_gNB_CU) {
    NR_RRC_MAC_CCCH_DATA_IND (message_p).frame     = 0;
    NR_RRC_MAC_CCCH_DATA_IND (message_p).sub_frame = 0;
    NR_RRC_MAC_CCCH_DATA_IND (message_p).sdu_size  = ccch_sdu_len;
    NR_RRC_MAC_CCCH_DATA_IND (message_p).gnb_index = rrc_inst; // CU instance
    NR_RRC_MAC_CCCH_DATA_IND (message_p).rnti      = rnti;
    NR_RRC_MAC_CCCH_DATA_IND (message_p).CC_id     = CC_id;
    itti_send_msg_to_task (TASK_RRC_GNB, instance, message_p);
  } else {
    RRC_MAC_CCCH_DATA_IND (message_p).frame      = 0;
    RRC_MAC_CCCH_DATA_IND (message_p).sub_frame  = 0;
    RRC_MAC_CCCH_DATA_IND (message_p).sdu_size   = ccch_sdu_len;
    RRC_MAC_CCCH_DATA_IND (message_p).enb_index  = rrc_inst; // CU instance
    RRC_MAC_CCCH_DATA_IND (message_p).rnti       = rnti;
    RRC_MAC_CCCH_DATA_IND (message_p).CC_id      = CC_id;
    itti_send_msg_to_task (TASK_RRC_ENB, instance, message_p);
  }

  return 0;
}


/*
    DL RRC Message Transfer.
*/

//void CU_send_DL_RRC_MESSAGE_TRANSFER(F1AP_DLRRCMessageTransfer_t *DLRRCMessageTransfer) {
int CU_send_DL_RRC_MESSAGE_TRANSFER(instance_t                instance,
                                    f1ap_dl_rrc_message_t    *f1ap_dl_rrc)
                                    {

  LOG_D(F1AP, "CU send DL_RRC_MESSAGE_TRANSFER \n");
  F1AP_F1AP_PDU_t                 pdu;
  F1AP_DLRRCMessageTransfer_t    *out;
  F1AP_DLRRCMessageTransferIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;

  /* Create */
  /* 0. Message Type */ 
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_DLRRCMessageTransfer;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_DLRRCMessageTransfer;
  out = &pdu.choice.initiatingMessage->value.choice.DLRRCMessageTransfer;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */

  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_DLRRCMessageTransferIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(&f1ap_cu_inst[instance], f1ap_dl_rrc->rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  LOG_I(F1AP, "Setting GNB_CU_UE_F1AP_ID %llu associated with UE RNTI %x (instance %ld)\n",
        (unsigned long long int)ie->value.choice.GNB_CU_UE_F1AP_ID, f1ap_dl_rrc->rnti, instance);


  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_DLRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(&f1ap_cu_inst[instance], f1ap_dl_rrc->rnti);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  LOG_I(F1AP, "GNB_DU_UE_F1AP_ID %llu associated with UE RNTI %x \n", (unsigned long long int)ie->value.choice.GNB_DU_UE_F1AP_ID, f1ap_dl_rrc->rnti);

  /* optional */
  /* c3. oldgNB_DU_UE_F1AP_ID */
 /* if (f1ap_dl_rrc->old_gNB_DU_ue_id != 0xFFFFFFFF) {
    ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
    ie->id                                = F1AP_ProtocolIE_ID_id_oldgNB_DU_UE_F1AP_ID;
    ie->criticality                       = F1AP_Criticality_reject;
    ie->value.present                     = F1AP_DLRRCMessageTransferIEs__value_PR_NOTHING;
    ie->value.choice.oldgNB_DU_UE_F1AP_ID = f1ap_dl_rrc->old_gNB_DU_ue_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }*/ 

  /* mandatory */
  /* c4. SRBID */
  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_SRBID;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_SRBID;
  ie->value.choice.SRBID            = f1ap_dl_rrc->srb_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c5. ExecuteDuplication */
  if (f1ap_dl_rrc->execute_duplication) {
    ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
    ie->id                            = F1AP_ProtocolIE_ID_id_ExecuteDuplication;
    ie->criticality                   = F1AP_Criticality_ignore;
    ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_ExecuteDuplication;
    ie->value.choice.ExecuteDuplication = F1AP_ExecuteDuplication_true;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  // issue in here
  /* mandatory */
  /* c6. RRCContainer */
  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, (const char*)f1ap_dl_rrc->rrc_container, f1ap_dl_rrc->rrc_container_length);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (RC.nrrrc && RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instance)]->node_type == ngran_gNB_CU) {
    LOG_I(F1AP, "%s() RRCContainer size %d: ", __func__, f1ap_dl_rrc->rrc_container_length);
    for (int i = 0; i < ie->value.choice.RRCContainer.size; i++)
      printf("%02x ", f1ap_dl_rrc->rrc_container[i]);
    printf("\n");
  }

  /* optional */
  /* c7. RAT_FrequencyPriorityInformation */
  /* TODO */ 
  int endc=1;
  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_RAT_FrequencyPriorityInformation;
  if (endc==1) {
    ie->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_eNDC;
    ie->value.choice.RAT_FrequencyPriorityInformation.choice.eNDC = 123L;
  }
  else {
    ie->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_nGRAN;
    ie->value.choice.RAT_FrequencyPriorityInformation.choice.nGRAN = 11L;
  }
    //ie->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_rAT_FrequencySelectionPriority;
    //ie->value.choice.RAT_FrequencyPriorityInformation.choice.rAT_FrequencySelectionPriority = 123L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
 
  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 DL RRC MESSAGE TRANSFER \n");
    return -1;
  }

  cu_f1ap_itti_send_sctp_data_req(instance, f1ap_assoc_id /* BK: fix me*/ , buffer, len, 0 /* BK: fix me*/);

  return 0;
}

/*
    UL RRC Message Transfer
*/

int CU_handle_UL_RRC_MESSAGE_TRANSFER(instance_t       instance,
                                      uint32_t         assoc_id,
                                      uint32_t         stream,
                                      F1AP_F1AP_PDU_t *pdu) {

  LOG_D(F1AP, "CU_handle_UL_RRC_MESSAGE_TRANSFER \n");
  
  F1AP_ULRRCMessageTransfer_t    *container;
  F1AP_ULRRCMessageTransferIEs_t *ie;

  uint64_t        cu_ue_f1ap_id;
  uint64_t        du_ue_f1ap_id;
  uint64_t        srb_id;

  DevAssert(pdu != NULL);
  
  if (stream != 0) {
    LOG_E(F1AP, "[SCTP %d] Received F1 on stream != 0 (%d)\n",
               assoc_id, stream);
    return -1;
  }

  container = &pdu->choice.initiatingMessage->value.choice.ULRRCMessageTransfer;


  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_ULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  cu_ue_f1ap_id = ie->value.choice.GNB_CU_UE_F1AP_ID;
  LOG_D(F1AP, "cu_ue_f1ap_id %lu associated with RNTI %x\n", cu_ue_f1ap_id, f1ap_get_rnti_by_cu_id(&f1ap_cu_inst[instance], cu_ue_f1ap_id));


  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_ULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  LOG_D(F1AP, "du_ue_f1ap_id %lu associated with RNTI %x\n", du_ue_f1ap_id, f1ap_get_rnti_by_cu_id(&f1ap_cu_inst[instance], du_ue_f1ap_id));


  /* mandatory */
  /* SRBID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_ULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SRBID, true);
  srb_id = ie->value.choice.SRBID;
  if (srb_id < 1 ) 
    LOG_E(F1AP, "Unexpected UL RRC MESSAGE for srb_id %lu \n", srb_id);
  else  
    LOG_D(F1AP, "UL RRC MESSAGE for srb_id %lu in DCCH \n", srb_id);


  // issue in here
  /* mandatory */
  /* RRC Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_ULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, true);
  // print message in debug mode 

  // create an ITTI message and copy SDU

  /*
  
  message_p = itti_alloc_new_message (TASK_CU_F1, 0, RRC_DCCH_DATA_IND);

  RRC_DCCH_DATA_IND (message_p).sdu_p = malloc(ie->value.choice.RRCContainer.size);

  RRC_DCCH_DATA_IND (message_p).sdu_size = ie->value.choice.RRCContainer.size;
  memcpy(RRC_DCCH_DATA_IND (message_p).sdu_p, ie->value.choice.RRCContainer.buf,
         ie->value.choice.RRCContainer.size);

  RRC_DCCH_DATA_IND (message_p).dcch_index = srb_id;
  RRC_DCCH_DATA_IND (message_p).rnti = f1ap_get_rnti_by_cu_id(&f1ap_cu_inst[instance], cu_ue_f1ap_id);
  RRC_DCCH_DATA_IND (message_p).module_id = instance;
  RRC_DCCH_DATA_IND (message_p).eNB_index = instance; // not needed for CU

  itti_send_msg_to_task(TASK_RRC_ENB, instance, message_p);
  */
  protocol_ctxt_t ctxt;
  ctxt.module_id = instance;
  ctxt.instance = instance;
  ctxt.rnti = f1ap_get_rnti_by_cu_id(&f1ap_cu_inst[instance], cu_ue_f1ap_id);
  ctxt.enb_flag = 1;
  ctxt.eNB_index = 0;
  ctxt.configured = 1;
  mem_block_t *mb = get_free_mem_block(ie->value.choice.RRCContainer.size,__func__);
  memcpy((void*)mb->data,(void*)ie->value.choice.RRCContainer.buf,ie->value.choice.RRCContainer.size);
  LOG_I(F1AP, "Calling pdcp_data_ind for UE RNTI %x srb_id %lu with size %ld (DCCH) \n", ctxt.rnti, srb_id, ie->value.choice.RRCContainer.size);

  //LOG_I(F1AP, "%s() RRCContainer size %lu: ", __func__, ie->value.choice.RRCContainer.size);
  //for (int i = 0; i < ie->value.choice.RRCContainer.size; i++)
  //  printf("%02x ", mb->data[i]);
  //printf("\n");

  pdcp_data_ind (&ctxt,
     1, // srb_flag
     0, // embms_flag
     srb_id,
     ie->value.choice.RRCContainer.size,
     mb);
  return 0;
}
