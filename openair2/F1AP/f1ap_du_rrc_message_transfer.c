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

/*! \file f1ap_du_rrc_message_transfer.c
 * \brief f1ap rrc message transfer for DU
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
#include "f1ap_itti_messaging.h"

#include "f1ap_du_rrc_message_transfer.h"

#include "uper_decoder.h"

#include "NR_DL-CCCH-Message.h"
#include "NR_UL-CCCH-Message.h"
#include "NR_DL-DCCH-Message.h"
#include "NR_UL-DCCH-Message.h"
// for SRB1_logicalChannelConfig_defaultValue
#include "rrc_extern.h"
#include "common/ran_context.h"

#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "asn1_msg.h"
#include "intertask_interface.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include <openair3/ocp-gtpu/gtp_itf.h>

#include "openair2/LAYER2/NR_MAC_gNB/mac_rrc_dl_handler.h"

/*  DL RRC Message Transfer */
int DU_handle_DL_RRC_MESSAGE_TRANSFER(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  F1AP_DLRRCMessageTransfer_t    *container;
  F1AP_DLRRCMessageTransferIEs_t *ie;
  int             executeDuplication;
  //uint64_t        subscriberProfileIDforRFP;
  //uint64_t        rAT_FrequencySelectionPriority;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.DLRRCMessageTransfer;
  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  uint32_t cu_ue_f1ap_id = ie->value.choice.GNB_CU_UE_F1AP_ID;
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  uint32_t du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

  /* optional */
  /* oldgNB_DU_UE_F1AP_ID */
  uint32_t *old_gNB_DU_ue_id = NULL;
  uint32_t old_gNB_DU_ue_id_stack = 0;
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_oldgNB_DU_UE_F1AP_ID, false);
  if (ie) {
    /* strange: it is not named OLD_GNB_DU_UE... */
    old_gNB_DU_ue_id_stack = ie->value.choice.GNB_DU_UE_F1AP_ID_1;
    old_gNB_DU_ue_id = &old_gNB_DU_ue_id_stack;
    gtpv1u_update_ue_id(getCxt(instance)->gtpInst, old_gNB_DU_ue_id_stack, du_ue_f1ap_id);
  }

  /* mandatory */
  /* SRBID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SRBID, true);
  uint64_t  srb_id = ie->value.choice.SRBID;
  LOG_D(F1AP, "srb_id %lu \n", srb_id);

  /* optional */
  /* ExecuteDuplication */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_ExecuteDuplication, true);
    executeDuplication = ie->value.choice.ExecuteDuplication;
    LOG_D(F1AP, "ExecuteDuplication %d \n", executeDuplication);
  }

  // issue in here
  /* mandatory */
  /* RRC Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, true);
  /* optional */
  /* RAT_FrequencyPriorityInformation */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation, true);

    switch(ie->value.choice.RAT_FrequencyPriorityInformation.present) {
      case F1AP_RAT_FrequencyPriorityInformation_PR_eNDC:
        //subscriberProfileIDforRFP = ie->value.choice.RAT_FrequencyPriorityInformation.choice.subscriberProfileIDforRFP;
        break;

      case F1AP_RAT_FrequencyPriorityInformation_PR_nGRAN:
        //rAT_FrequencySelectionPriority = ie->value.choice.RAT_FrequencyPriorityInformation.choice.rAT_FrequencySelectionPriority;
        break;

      default:
        LOG_W(F1AP, "unhandled IE RAT_FrequencyPriorityInformation.present\n");
        break;
    }
  }

  f1ap_dl_rrc_message_t dl_rrc = {
    .gNB_CU_ue_id = cu_ue_f1ap_id,
    .gNB_DU_ue_id = du_ue_f1ap_id,
    .old_gNB_DU_ue_id = old_gNB_DU_ue_id,
    .rrc_container_length = ie->value.choice.RRCContainer.size,
    .rrc_container = ie->value.choice.RRCContainer.buf,
    .srb_id = srb_id
  };
  dl_rrc_message_transfer(&dl_rrc);
  return 0;
}

/*  UL RRC Message Transfer */
int DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(sctp_assoc_t assoc_id, const f1ap_initial_ul_rrc_message_t *msg)
{
  F1AP_F1AP_PDU_t                       pdu= {0};
  F1AP_InitialULRRCMessageTransfer_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;

  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_InitialULRRCMessageTransfer;
  tmp->criticality   = F1AP_Criticality_ignore;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_InitialULRRCMessageTransfer;
  out = &tmp->value.choice.InitialULRRCMessageTransfer;
  /* mandatory */
  /* c1. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie1->value.choice.GNB_DU_UE_F1AP_ID = msg->gNB_DU_ue_id;
  /* mandatory */
  /* c2. NRCGI */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_NRCGI;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_NRCGI;
  //Fixme: takes always the first cell
  addnRCGI(ie2->value.choice.NRCGI, &getCxt(0)->setupReq.cell[0].info);
  /* mandatory */
  /* c3. C_RNTI */  // 16
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie3);
  ie3->id                             = F1AP_ProtocolIE_ID_id_C_RNTI;
  ie3->criticality                    = F1AP_Criticality_reject;
  ie3->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_C_RNTI;
  ie3->value.choice.C_RNTI = msg->crnti;
  /* mandatory */
  /* c4. RRCContainer */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie4);
  ie4->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie4->criticality                   = F1AP_Criticality_reject;
  ie4->value.present                 = F1AP_InitialULRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie4->value.choice.RRCContainer, (const char *)msg->rrc_container, msg->rrc_container_length);

  /* optional */
  /* c5. DUtoCURRCContainer */
  if (msg->du2cu_rrc_container != NULL) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie5);
    ie5->id                             = F1AP_ProtocolIE_ID_id_DUtoCURRCContainer;
    ie5->criticality                    = F1AP_Criticality_reject;
    ie5->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_DUtoCURRCContainer;
    OCTET_STRING_fromBuf(&ie5->value.choice.DUtoCURRCContainer,
                         (const char *)msg->du2cu_rrc_container,
                         msg->du2cu_rrc_container_length);
  }
  /* mandatory */
  /* c6. Transaction ID (integer value) */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie6);
  ie6->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie6->criticality               = F1AP_Criticality_ignore;
  ie6->value.present             = F1AP_InitialULRRCMessageTransferIEs__value_PR_TransactionID;
  ie6->value.choice.TransactionID = F1AP_get_next_transaction_identifier(0, 0);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 INITIAL UL RRC MESSAGE TRANSFER\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int DU_send_UL_NR_RRC_MESSAGE_TRANSFER(sctp_assoc_t assoc_id, const f1ap_ul_rrc_message_t *msg)
{
  F1AP_F1AP_PDU_t                pdu= {0};
  F1AP_ULRRCMessageTransfer_t    *out;
  uint8_t *buffer = NULL;
  uint32_t len;
  LOG_D(F1AP,
        "size %d UE RNTI %x in SRB %d\n",
        msg->rrc_container_length,
        msg->gNB_DU_ue_id,
        msg->srb_id);
  //for (int i = 0;i < msg->rrc_container_length; i++)
  //  printf("%02x ", msg->rrc_container[i]);
  //printf("\n");
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_ULRRCMessageTransfer;
  tmp->criticality   = F1AP_Criticality_ignore;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_ULRRCMessageTransfer;
  out = &tmp->value.choice.ULRRCMessageTransfer;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_ULRRCMessageTransferIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = msg->gNB_CU_ue_id;
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_ULRRCMessageTransferIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = msg->gNB_DU_ue_id;
  /* mandatory */
  /* c3. SRBID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_ULRRCMessageTransferIEs_t, ie3);
  ie3->id                            = F1AP_ProtocolIE_ID_id_SRBID;
  ie3->criticality                   = F1AP_Criticality_reject;
  ie3->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_SRBID;
  ie3->value.choice.SRBID            = msg->srb_id;
  // issue in here
  /* mandatory */
  /* c4. RRCContainer */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_ULRRCMessageTransferIEs_t, ie4);
  ie4->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie4->criticality                   = F1AP_Criticality_reject;
  ie4->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie4->value.choice.RRCContainer,
                       (const char *) msg->rrc_container,
                       msg->rrc_container_length);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UL RRC MESSAGE TRANSFER \n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}
