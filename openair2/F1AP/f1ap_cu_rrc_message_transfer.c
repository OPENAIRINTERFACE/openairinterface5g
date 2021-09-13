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

/*
    Initial UL RRC Message Transfer
*/

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

  container = &pdu->choice.initiatingMessage->value.choice.InitialULRRCMessageTransfer;
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  instance_t du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  /* NRCGI
  * Fixme: process NRCGI
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
  if (f1ap_req(true, instance)->cell_type==CELL_MACRO_GNB) {
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

  LOG_I(F1AP, "%s() RRCContainer (CCCH) size %ld: ", __func__,
        ie->value.choice.RRCContainer.size);

  for (int i = 0; i < ie->value.choice.RRCContainer.size; i++)
    printf("%02x ", RRC_MAC_CCCH_DATA_IND (message_p).sdu[i]);

  printf("\n");
  /* DUtoCURRCContainer */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DUtoCURRCContainer, true);

  if (ie) {
    NR_RRC_MAC_CCCH_DATA_IND (message_p).du_to_cu_rrc_container = malloc(sizeof(OCTET_STRING_t));
    NR_RRC_MAC_CCCH_DATA_IND (message_p).du_to_cu_rrc_container->size = ie->value.choice.DUtoCURRCContainer.size;
    NR_RRC_MAC_CCCH_DATA_IND (message_p).du_to_cu_rrc_container->buf = malloc(ie->value.choice.DUtoCURRCContainer.size);
    memcpy(NR_RRC_MAC_CCCH_DATA_IND (message_p).du_to_cu_rrc_container->buf,
           ie->value.choice.DUtoCURRCContainer.buf,
           ie->value.choice.DUtoCURRCContainer.size);
  }

  int f1ap_uid = f1ap_add_ue(true, instance, rnti);

  if (f1ap_uid  < 0 ) {
    LOG_E(F1AP, "Failed to add UE \n");
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  //getCxt(true,ITTI_MSG_DESTINATION_ID(message_p))->f1ap_ue[f1ap_uid].du_ue_f1ap_id = du_ue_f1ap_id;
  NR_RRC_MAC_CCCH_DATA_IND (message_p).frame     = 0;
  NR_RRC_MAC_CCCH_DATA_IND (message_p).sub_frame = 0;
  NR_RRC_MAC_CCCH_DATA_IND (message_p).sdu_size  = ccch_sdu_len;
  NR_RRC_MAC_CCCH_DATA_IND (message_p).nr_cellid = nr_cellid; // CU instance
  NR_RRC_MAC_CCCH_DATA_IND (message_p).rnti      = rnti;
  NR_RRC_MAC_CCCH_DATA_IND (message_p).CC_id     = CC_id;
  itti_send_msg_to_task (f1ap_req(true,ITTI_MSG_DESTINATION_ID(message_p))->cell_type==CELL_MACRO_GNB?TASK_RRC_GNB:TASK_RRC_ENB, instance, message_p);
  return 0;
}

/*
    DL RRC Message Transfer.
*/
//void CU_send_DL_RRC_MESSAGE_TRANSFER(F1AP_DLRRCMessageTransfer_t *DLRRCMessageTransfer) {
int CU_send_DL_RRC_MESSAGE_TRANSFER(instance_t                instance,
                                    f1ap_dl_rrc_message_t    *f1ap_dl_rrc) {
  LOG_D(F1AP, "CU send DL_RRC_MESSAGE_TRANSFER \n");
  F1AP_F1AP_PDU_t                 pdu= {0};
  F1AP_DLRRCMessageTransfer_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, F1AP_InitiatingMessage_t, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_DLRRCMessageTransfer;
  tmp->criticality   = F1AP_Criticality_ignore;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_DLRRCMessageTransfer;
  out = &tmp->value.choice.DLRRCMessageTransfer;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_DLRRCMessageTransferIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_DLRRCMessageTransferIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(true, instance, f1ap_dl_rrc->rnti);
  LOG_I(F1AP, "Setting GNB_CU_UE_F1AP_ID %llu associated with UE RNTI %x (instance %ld)\n",
        (unsigned long long int)ie1->value.choice.GNB_CU_UE_F1AP_ID, f1ap_dl_rrc->rnti, instance);
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_DLRRCMessageTransferIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_DLRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(true, instance, f1ap_dl_rrc->rnti);
  LOG_I(F1AP, "GNB_DU_UE_F1AP_ID %llu associated with UE RNTI %x \n", (unsigned long long int)ie2->value.choice.GNB_DU_UE_F1AP_ID, f1ap_dl_rrc->rnti);
  /* optional */
  /* c3. oldgNB_DU_UE_F1AP_ID */
  /* if (f1ap_dl_rrc->old_gNB_DU_ue_id != 0xFFFFFFFF) {
     asn1cSequenceAdd(out->protocolIEs.list, F1AP_DLRRCMessageTransferIEs_t, ie3);
     ie3->id                                = F1AP_ProtocolIE_ID_id_oldgNB_DU_UE_F1AP_ID;
     ie3->criticality                       = F1AP_Criticality_reject;
     ie3->value.present                     = F1AP_DLRRCMessageTransferIEs__value_PR_NOTHING;
     ie3->value.choice.oldgNB_DU_UE_F1AP_ID = f1ap_dl_rrc->old_gNB_DU_ue_id;
   }*/
  /* mandatory */
  /* c4. SRBID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_DLRRCMessageTransferIEs_t, ie4);
  ie4->id                            = F1AP_ProtocolIE_ID_id_SRBID;
  ie4->criticality                   = F1AP_Criticality_reject;
  ie4->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_SRBID;
  ie4->value.choice.SRBID            = f1ap_dl_rrc->srb_id;

  /* optional */
  /* c5. ExecuteDuplication */
  if (f1ap_dl_rrc->execute_duplication) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_DLRRCMessageTransferIEs_t, ie5);
    ie5->id                            = F1AP_ProtocolIE_ID_id_ExecuteDuplication;
    ie5->criticality                   = F1AP_Criticality_ignore;
    ie5->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_ExecuteDuplication;
    ie5->value.choice.ExecuteDuplication = F1AP_ExecuteDuplication_true;
  }

  // issue in here
  /* mandatory */
  /* c6. RRCContainer */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_DLRRCMessageTransferIEs_t, ie6);
  ie6->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie6->criticality                   = F1AP_Criticality_reject;
  ie6->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie6->value.choice.RRCContainer,
                       (const char *)f1ap_dl_rrc->rrc_container, f1ap_dl_rrc->rrc_container_length);

  /* optional */
  /* c7. RAT_FrequencyPriorityInformation */
  /* TODO */
  if (0) {
    int endc=1;
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_DLRRCMessageTransferIEs_t, ie7);
    ie7->id                            = F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation;
    ie7->criticality                   = F1AP_Criticality_reject;
    ie7->value.present                 = F1AP_DLRRCMessageTransferIEs__value_PR_RAT_FrequencyPriorityInformation;

    if (endc==1) {
      ie7->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_eNDC;
      ie7->value.choice.RAT_FrequencyPriorityInformation.choice.eNDC = 123L;
    } else {
      ie7->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_nGRAN;
      ie7->value.choice.RAT_FrequencyPriorityInformation.choice.nGRAN = 11L;
    }

    //ie->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_rAT_FrequencySelectionPriority;
    //ie->value.choice.RAT_FrequencyPriorityInformation.choice.rAT_FrequencySelectionPriority = 123L;
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 DL RRC MESSAGE TRANSFER \n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(true, instance, buffer, len, 0 /* BK: fix me*/);
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
  LOG_D(F1AP, "cu_ue_f1ap_id %lu associated with RNTI %x\n",
        cu_ue_f1ap_id, f1ap_get_rnti_by_cu_id(true, instance, cu_ue_f1ap_id));
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_ULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  LOG_D(F1AP, "du_ue_f1ap_id %lu associated with RNTI %x\n",
        du_ue_f1ap_id, f1ap_get_rnti_by_cu_id(true, instance, du_ue_f1ap_id));
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
  RRC_DCCH_DATA_IND (message_p).rnti = f1ap_get_rnti_by_cu_id(true, instance, cu_ue_f1ap_id);
  RRC_DCCH_DATA_IND (message_p).instance = instance;
  RRC_DCCH_DATA_IND (message_p).eNB_index = instance; // not needed for CU

  itti_send_msg_to_task(TASK_RRC_ENB, instance, message_p);
  */
  protocol_ctxt_t ctxt;
  ctxt.instance = instance;
  ctxt.instance = instance;
  ctxt.rnti = f1ap_get_rnti_by_cu_id(true, instance, cu_ue_f1ap_id);
  ctxt.enb_flag = 1;
  ctxt.eNB_index = 0;
  mem_block_t *mb = get_free_mem_block(ie->value.choice.RRCContainer.size,__func__);
  memcpy((void *)mb->data,(void *)ie->value.choice.RRCContainer.buf,ie->value.choice.RRCContainer.size);
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
