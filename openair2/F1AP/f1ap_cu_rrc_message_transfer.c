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

/*
    Initial UL RRC Message Transfer
*/

extern RAN_CONTEXT_t RC;

int CU_handle_INITIAL_UL_RRC_MESSAGE_TRANSFER(instance_t             instance,
                                              uint32_t               assoc_id,
                                              uint32_t               stream,
                                              F1AP_F1AP_PDU_t       *pdu) {

  printf("CU_handle_INITIAL_UL_RRC_MESSAGE_TRANSFER\n");
  // decode the F1 message
  // get the rrc message from the contauiner 
  // call func rrc_eNB_decode_ccch: <-- needs some update here
  MessageDef                            *message_p;
  F1AP_InitialULRRCMessageTransfer_t    *container;
  F1AP_InitialULRRCMessageTransferIEs_t *ie;
  
  rnti_t          rnti;
  uint8_t        *ccch_sdu;
  sdu_size_t      ccch_sdu_len;
  int             CC_id =0;
  uint64_t        du_ue_f1ap_id;

  DevAssert(pdu != NULL);
  
  if (stream != 0) {
    LOG_E(F1AP, "[SCTP %d] Received F1 on stream != 0 (%d)\n",
               assoc_id, stream);
    return -1;
  }

  container = &pdu->choice.initiatingMessage->value.choice.InitialULRRCMessageTransfer;

  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  printf("du_ue_f1ap_id %lu \n", du_ue_f1ap_id);

  /* NRCGI */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_NRCGI, true);

  uint64_t nr_cellid;
  BIT_STRING_TO_NR_CELL_IDENTITY(&ie->value.choice.NRCGI.nRCellIdentity,nr_cellid);
					       
  /* RNTI */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_C_RNTI, true);
  BUFFER_TO_INT16(ie->value.choice.C_RNTI.buf, rnti);

  /* RRC Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, true);

  // create an ITTI message and copy SDU
  message_p = itti_alloc_new_message (TASK_CU_F1, RRC_MAC_CCCH_DATA_IND);
  memset (RRC_MAC_CCCH_DATA_IND (message_p).sdu, 0, CCCH_SDU_SIZE);
  ccch_sdu_len = ie->value.choice.RRCContainer.size;
  memcpy(RRC_MAC_CCCH_DATA_IND (message_p).sdu, ie->value.choice.RRCContainer.buf,
         ccch_sdu_len);
  printf ("RRCContainer(CCCH) :");
  for (int i=0;i<ie->value.choice.RRCContainer.size;i++) printf("%2x ",RRC_MAC_CCCH_DATA_IND (message_p).sdu[i]);

  // Find instance from nr_cellid
  int rrc_inst = -1;
  for (int i=0;i<RC.nb_inst;i++) {
          // first get RRC instance (note, no the ITTI instance)
    eNB_RRC_INST *rrc = RC.rrc[i];
    if (rrc->nr_cellid == nr_cellid) {
      rrc_inst = i; 
      break;
    }
  }
  AssertFatal(rrc_inst>=0,"couldn't find an RRC instance for nr_cell %ll\n",nr_cellid);

  RRC_MAC_CCCH_DATA_IND (message_p).frame     = 0; 
  RRC_MAC_CCCH_DATA_IND (message_p).sub_frame = 0;
  RRC_MAC_CCCH_DATA_IND (message_p).sdu_size  = ccch_sdu_len;
  RRC_MAC_CCCH_DATA_IND (message_p).enb_index = rrc_inst; // CU instance 
  RRC_MAC_CCCH_DATA_IND (message_p).rnti      = rnti;
  RRC_MAC_CCCH_DATA_IND (message_p).CC_id      = CC_id; 
  itti_send_msg_to_task (TASK_RRC_ENB, instance, message_p);


 // OR  creat the ctxt and srb_info struct required by rrc_eNB_decode_ccch
/*
 PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt,
                                instance, // to fix
                                ENB_FLAG_YES,
                                rnti,
                                0, // frame 
                                0); // slot 

  CC_id = RRC_MAC_CCCH_DATA_IND(msg_p).CC_id;
  srb_info_p = &RC.rrc[instance]->carrier[CC_id].Srb0;

  if (ccch_sdu_len >= RRC_BUFFER_SIZE_MAX) {
      LOG_E(RRC, "CCCH message has size %d > %d\n",ccch_sdu_len,RRC_BUFFER_SIZE_MAX);
      break;
  }
  memcpy(srb_info_p->Rx_buffer.Payload,
         ccch_sdu,
         ccch_sdu_len);
  srb_info->Rx_buffer.payload_size = ccch_sdu_len;

  rrc_eNB_decode_ccch(&ctxt, srb_info, CC_id);
  */ 
  // if size > 0 
  // CU_send_DL_RRC_MESSAGE_TRANSFER(C.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Payload, RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size)

  return 0;
}


/*
    DL RRC Message Transfer.
*/

//void CU_send_DL_RRC_MESSAGE_TRANSFER(F1AP_DLRRCMessageTransfer_t *DLRRCMessageTransfer) {
int CU_send_DL_RRC_MESSAGE_TRANSFER(instance_t instance) {
  F1AP_F1AP_PDU_t                pdu;
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
  ie->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_DLRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = 651L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c3. oldgNB_DU_UE_F1AP_ID */
  if (0) {
    ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
    ie->id                            = F1AP_ProtocolIE_ID_id_oldgNB_DU_UE_F1AP_ID;
    ie->criticality                   = F1AP_Criticality_reject;
    //ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_NOTHING;
    //ie->value.choice.            = 1;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c4. SRBID */
  ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_SRBID;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_SRBID;
  ie->value.choice.SRBID            = 2L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c5. ExecuteDuplication */
  if (0) {
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
  OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, "A", strlen("A"));
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c7. RAT_FrequencyPriorityInformation */
  if (0) {
    ie = (F1AP_DLRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_DLRRCMessageTransferIEs_t));
    ie->id                            = F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation;
    ie->criticality                   = F1AP_Criticality_reject;
    ie->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_RAT_FrequencyPriorityInformation;

    ie->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_subscriberProfileIDforRFP;
    ie->value.choice.RAT_FrequencyPriorityInformation.choice.subscriberProfileIDforRFP = 123L;

    //ie->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_rAT_FrequencySelectionPriority;
    //ie->value.choice.RAT_FrequencyPriorityInformation.choice.rAT_FrequencySelectionPriority = 123L;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
    return -1;
  }

  printf("\n");

  /* decode */
  if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
    printf("Failed to decode F1 setup request\n");
    return -1;
  }

  return 0;
}

/*
    UL RRC Message Transfer
*/

int CU_handle_UL_RRC_MESSAGE_TRANSFER(instance_t       instance,
                                      uint32_t         assoc_id,
                                      uint32_t         stream,
                                      F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}
