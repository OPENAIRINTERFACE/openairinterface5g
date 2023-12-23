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
#include "f1ap_ids.h"
#include "f1ap_itti_messaging.h"
#include "f1ap_cu_rrc_message_transfer.h"
#include "common/ran_context.h"
#include "openair3/UTILS/conversions.h"
#include "LAYER2/nr_pdcp/nr_pdcp_oai_api.h"

/*
    Initial UL RRC Message Transfer
*/

int CU_handle_INITIAL_UL_RRC_MESSAGE_TRANSFER(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  LOG_D(F1AP, "CU_handle_INITIAL_UL_RRC_MESSAGE_TRANSFER\n");
  // decode the F1 message
  // get the rrc message from the contauiner
  // call func rrc_eNB_decode_ccch: <-- needs some update here
  MessageDef                            *message_p;
  F1AP_InitialULRRCMessageTransfer_t    *container;
  F1AP_InitialULRRCMessageTransferIEs_t *ie;
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
  uint32_t du_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

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
  rnti_t rnti = ie->value.choice.C_RNTI;
  F1AP_InitialULRRCMessageTransferIEs_t *rrccont;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, rrccont, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, true);
  AssertFatal(rrccont!=NULL,"RRCContainer is missing\n");

  F1AP_InitialULRRCMessageTransferIEs_t *du2cu;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_InitialULRRCMessageTransferIEs_t, du2cu, container,
                             F1AP_ProtocolIE_ID_id_DUtoCURRCContainer, false);

  // create an ITTI message and copy SDU
  message_p = itti_alloc_new_message(TASK_CU_F1, 0, F1AP_INITIAL_UL_RRC_MESSAGE);
  message_p->ittiMsgHeader.originInstance = assoc_id;
  f1ap_initial_ul_rrc_message_t *ul_rrc = &F1AP_INITIAL_UL_RRC_MESSAGE(message_p);
  ul_rrc->gNB_DU_ue_id = du_ue_id;
  ul_rrc->nr_cellid = nr_cellid; // CU instance
  ul_rrc->crnti = rnti;
  ul_rrc->rrc_container_length = rrccont->value.choice.RRCContainer.size;
  ul_rrc->rrc_container = malloc(ul_rrc->rrc_container_length);
  memcpy(ul_rrc->rrc_container, rrccont->value.choice.RRCContainer.buf, ul_rrc->rrc_container_length);
  AssertFatal(du2cu != NULL, "no masterCellGroup in initial UL RRC message\n");
  ul_rrc->du2cu_rrc_container_length = du2cu->value.choice.DUtoCURRCContainer.size;
  ul_rrc->du2cu_rrc_container = malloc(ul_rrc->du2cu_rrc_container_length);
  memcpy(ul_rrc->du2cu_rrc_container, du2cu->value.choice.DUtoCURRCContainer.buf, ul_rrc->du2cu_rrc_container_length);
  itti_send_msg_to_task(TASK_RRC_GNB, instance, message_p);

  return 0;
}

/*
    DL RRC Message Transfer.
*/
int CU_send_DL_RRC_MESSAGE_TRANSFER(sctp_assoc_t assoc_id, f1ap_dl_rrc_message_t *f1ap_dl_rrc)
{
  LOG_D(F1AP, "CU send DL_RRC_MESSAGE_TRANSFER \n");
  F1AP_F1AP_PDU_t                 pdu= {0};
  F1AP_DLRRCMessageTransfer_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, tmp);
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
  DevAssert(f1ap_dl_rrc->gNB_CU_ue_id != 0);
  ie1->value.choice.GNB_CU_UE_F1AP_ID = f1ap_dl_rrc->gNB_CU_ue_id;

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_DLRRCMessageTransferIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_DLRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  DevAssert(f1ap_dl_rrc->gNB_DU_ue_id != 0);
  ie2->value.choice.GNB_DU_UE_F1AP_ID = f1ap_dl_rrc->gNB_DU_ue_id;

  /* optional */
  /* c3. oldgNB_DU_UE_F1AP_ID */
  if (f1ap_dl_rrc->old_gNB_DU_ue_id != NULL) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_DLRRCMessageTransferIEs_t, ie3);
    ie3->id                                = F1AP_ProtocolIE_ID_id_oldgNB_DU_UE_F1AP_ID;
    ie3->criticality                       = F1AP_Criticality_reject;
    ie3->value.present                     = F1AP_DLRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID_1;
    ie3->value.choice.GNB_DU_UE_F1AP_ID_1 = *f1ap_dl_rrc->old_gNB_DU_ue_id;
  }

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

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

/*
    UL RRC Message Transfer
*/
int CU_handle_UL_RRC_MESSAGE_TRANSFER(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
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
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_ULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  /* the RLC-PDCP does not transport the DU UE ID (yet), so we drop it here.
   * For the moment, let's hope this won't become relevant; to sleep in peace,
   * let's put an assert to check that it is the expected DU UE ID. */
  f1_ue_data_t ue_data = cu_get_f1_ue_data(cu_ue_f1ap_id);
  AssertFatal(ue_data.secondary_ue == du_ue_f1ap_id,
              "unexpected DU UE ID %d received, expected it to be %ld\n",
              ue_data.secondary_ue,
              du_ue_f1ap_id);

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
  protocol_ctxt_t ctxt={0};
  ctxt.instance = instance;
  ctxt.module_id = instance;
  ctxt.rntiMaybeUEid = cu_ue_f1ap_id;
  ctxt.enb_flag = 1;
  ctxt.eNB_index = 0;
  uint8_t *mb = malloc16(ie->value.choice.RRCContainer.size);
  memcpy(mb, ie->value.choice.RRCContainer.buf, ie->value.choice.RRCContainer.size);
  LOG_D(F1AP, "Calling pdcp_data_ind for UE RNTI %lx srb_id %lu with size %ld (DCCH) \n", ctxt.rntiMaybeUEid, srb_id, ie->value.choice.RRCContainer.size);
  //for (int i = 0; i < ie->value.choice.RRCContainer.size; i++)
  //  printf("%02x ", mb->data[i]);
  //printf("\n");
  pdcp_data_ind (&ctxt,
                 1, // srb_flag
                 0, // embms_flag
                 srb_id,
                 ie->value.choice.RRCContainer.size,
                 mb, NULL, NULL);
  return 0;
}
