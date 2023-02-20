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
#include "f1ap_decoder.h"
#include "f1ap_itti_messaging.h"

#include "f1ap_du_rrc_message_transfer.h"


#include "LTE_DL-CCCH-Message.h"
#include "LTE_DL-DCCH-Message.h"
#include "LTE_UL-DCCH-Message.h"

#include "NR_DL-CCCH-Message.h"
#include "NR_UL-CCCH-Message.h"
#include "NR_DL-DCCH-Message.h"
#include "NR_UL-DCCH-Message.h"
// for SRB1_logicalChannelConfig_defaultValue
#include "rrc_extern.h"
#include "common/ran_context.h"

#include "rrc_eNB_UE_context.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "asn1_msg.h"
#include "intertask_interface.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"

#include "openair2/LAYER2/NR_MAC_gNB/mac_rrc_dl_handler.h"

/*  DL RRC Message Transfer */
int DU_handle_DL_RRC_MESSAGE_TRANSFER(instance_t       instance,
                                      uint32_t         assoc_id,
                                      uint32_t         stream,
                                      F1AP_F1AP_PDU_t *pdu) {
  F1AP_DLRRCMessageTransfer_t    *container;
  F1AP_DLRRCMessageTransferIEs_t *ie;
  uint64_t        cu_ue_f1ap_id;
  uint64_t        du_ue_f1ap_id;
  int             executeDuplication;
  //uint64_t        subscriberProfileIDforRFP;
  //uint64_t        rAT_FrequencySelectionPriority;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.DLRRCMessageTransfer;
  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  cu_ue_f1ap_id = ie->value.choice.GNB_CU_UE_F1AP_ID;
  LOG_D(F1AP, "cu_ue_f1ap_id %lu \n", cu_ue_f1ap_id);
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  LOG_D(F1AP, "du_ue_f1ap_id %lu associated with UE RNTI %x \n",
        du_ue_f1ap_id,
        f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id)); // this should be the one transmitted via initial ul rrc message transfer

  if (f1ap_du_add_cu_ue_id(instance,du_ue_f1ap_id, cu_ue_f1ap_id) < 0 ) {
    LOG_E(F1AP, "Failed to find the F1AP UID \n");
    //return -1;
  }

  /* optional */
  /* oldgNB_DU_UE_F1AP_ID */
  if (0) {
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_DLRRCMessageTransferIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_oldgNB_DU_UE_F1AP_ID, true);
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
    .rrc_container_length = ie->value.choice.RRCContainer.size,
    .rrc_container = ie->value.choice.RRCContainer.buf,
    .rnti = f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id),
    .srb_id = srb_id
  };
  int rc = dl_rrc_message(instance, &dl_rrc);
  if (rc == 0)
    return 0; /* has been handled, otherwise continue below */

  // decode RRC Container and act on the message type
  AssertFatal(srb_id<3,"illegal srb_id\n");
  MessageDef *msg = itti_alloc_new_message(TASK_DU_F1, 0, NR_DU_RRC_DL_INDICATION);
  NRDuDlReq_t *req=&NRDuDlReq(msg);
  req->rnti=f1ap_get_rnti_by_du_id(DUtype, instance, du_ue_f1ap_id);
  req->srb_id=srb_id;
  req->buf= get_free_mem_block( ie->value.choice.RRCContainer.size, __func__);
  memcpy(req->buf->data, ie->value.choice.RRCContainer.buf, ie->value.choice.RRCContainer.size);
  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg);
  return 0;
}

int DU_send_UL_RRC_MESSAGE_TRANSFER(instance_t instance,
                                    const f1ap_ul_rrc_message_t *msg) {
  const rnti_t rnti = msg->rnti;
  F1AP_F1AP_PDU_t                pdu;
  F1AP_ULRRCMessageTransfer_t    *out;
  F1AP_ULRRCMessageTransferIEs_t *ie;
  uint8_t *buffer = NULL;
  uint32_t len;
  LOG_I(F1AP, "[DU %ld] %s: size %d UE RNTI %x in SRB %d\n",
        instance, __func__, msg->rrc_container_length, rnti, msg->srb_id);
  //LOG_I(F1AP, "%s() RRCContainer size %d: ", __func__, msg->rrc_container_length);
  //for (int i = 0;i < msg->rrc_container_length; i++)
  //  printf("%02x ", msg->rrc_container[i]);
  //printf("\n");
  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_ULRRCMessageTransfer;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_ULRRCMessageTransfer;
  out = &pdu.choice.initiatingMessage->value.choice.ULRRCMessageTransfer;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(DUtype, instance, rnti);
  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(DUtype, instance, rnti);
  asn1cSeqAdd(&out->protocolIEs.list, ie);
  /* mandatory */
  /* c3. SRBID */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_SRBID;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_SRBID;
  ie->value.choice.SRBID            = msg->srb_id;
  asn1cSeqAdd(&out->protocolIEs.list, ie);
  // issue in here
  /* mandatory */
  /* c4. RRCContainer */
  ie = (F1AP_ULRRCMessageTransferIEs_t *)calloc(1, sizeof(F1AP_ULRRCMessageTransferIEs_t));
  ie->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie->criticality                   = F1AP_Criticality_reject;
  ie->value.present                 = F1AP_ULRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer,
                       (const char *) msg->rrc_container,
                       msg->rrc_container_length);
  asn1cSeqAdd(&out->protocolIEs.list, ie);

  if (msg->srb_id == 1 || msg->srb_id == 2) {
    struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[instance], rnti);
    LTE_UL_DCCH_Message_t *ul_dcch_msg=NULL;
    asn_dec_rval_t dec_rval;
    dec_rval = uper_decode(NULL,
                           &asn_DEF_LTE_UL_DCCH_Message,
                           (void **)&ul_dcch_msg,
                           &ie->value.choice.RRCContainer.buf[1], // buf[0] includes the pdcp header
                           msg->rrc_container_length, 0, 0);

    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0))
      LOG_E(F1AP, " Failed to decode UL-DCCH (%zu bytes)\n",dec_rval.consumed);
    else
      LOG_I(F1AP, "Received message: present %d and c1 present %d\n",
            ul_dcch_msg->message.present, ul_dcch_msg->message.choice.c1.present);

    if (ul_dcch_msg->message.present == LTE_UL_DCCH_MessageType_PR_c1) {
      switch (ul_dcch_msg->message.choice.c1.present) {
        case LTE_UL_DCCH_MessageType__c1_PR_NOTHING:   /* No components present */
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_csfbParametersRequestCDMA2000:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_measurementReport:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_rrcConnectionReconfigurationComplete:
          LOG_I(F1AP, "[MSG] RRC UL rrcConnectionReconfigurationComplete\n");
          /* CDRX: activated when RRC Connection Reconfiguration Complete is received */
          int UE_id_mac = find_UE_id(instance, rnti);

          if (UE_id_mac == -1) {
            LOG_E(F1AP, "Can't find UE_id(MAC) of UE rnti %x\n", rnti);
            break;
          }

          UE_sched_ctrl_t *UE_scheduling_control = &(RC.mac[instance]->UE_info.UE_sched_ctrl[UE_id_mac]);

          if (UE_scheduling_control->cdrx_waiting_ack == true) {
            UE_scheduling_control->cdrx_waiting_ack = false;
            UE_scheduling_control->cdrx_configured = true; // Set to TRUE when RRC Connection Reconfiguration Complete is received
            LOG_I(F1AP, "CDRX configuration activated after RRC Connection Reconfiguration Complete reception\n");
          }

          /* End of CDRX processing */
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_rrcConnectionReestablishmentComplete:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_rrcConnectionSetupComplete:
          LOG_I(F1AP, "[MSG] RRC UL rrcConnectionSetupComplete \n");

          if(!ue_context_p) {
            LOG_E(F1AP, "Did not find the UE context associated with UE RNTOI %x, ue_context_p is NULL\n", rnti);
          } else {
            LOG_I(F1AP, "Processing RRCConnectionSetupComplete UE %x\n", rnti);
            ue_context_p->ue_context.StatusRrc = RRC_CONNECTED;
          }

          break;

        case LTE_UL_DCCH_MessageType__c1_PR_securityModeComplete:
          LOG_I(F1AP, "[MSG] RRC securityModeComplete \n");
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_securityModeFailure:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_ueCapabilityInformation:
          LOG_I(F1AP, "[MSG] RRC ueCapabilityInformation \n");
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_ulHandoverPreparationTransfer:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_ulInformationTransfer:
          LOG_I(F1AP,"[MSG] RRC UL Information Transfer \n");
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_counterCheckResponse:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_ueInformationResponse_r9:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_proximityIndication_r9:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_rnReconfigurationComplete_r10:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_mbmsCountingResponse_r10:
          break;

        case LTE_UL_DCCH_MessageType__c1_PR_interFreqRSTDMeasurementIndication_r10:
          break;
      }
    }
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UL RRC MESSAGE TRANSFER\n");
    return -1;
  }

  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(false, instance, buffer, len, getCxt(DUtype, instance)->default_sctp_stream_id);
  return 0;
}

/*  UL RRC Message Transfer */
int DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(instance_t     instanceP,
    int             CC_idP,
    int             UE_id,
    rnti_t          rntiP,
    const uint8_t   *sduP,
    sdu_size_t      sdu_lenP,
    const uint8_t   *sdu2P,
    sdu_size_t      sdu2_lenP) {
  F1AP_F1AP_PDU_t                       pdu= {0};
  F1AP_InitialULRRCMessageTransfer_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  int f1ap_uid = f1ap_add_ue (DUtype, instanceP, rntiP);

  if (f1ap_uid  < 0 ) {
    LOG_E(F1AP, "Failed to add UE \n");
    return -1;
  }

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
  ie1->value.choice.GNB_DU_UE_F1AP_ID = getCxt(DUtype, instanceP)->f1ap_ue[f1ap_uid].du_ue_f1ap_id;
  /* mandatory */
  /* c2. NRCGI */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_NRCGI;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_NRCGI;
  //Fixme: takes always the first cell
  addnRCGI(ie2->value.choice.NRCGI, getCxt(DUtype, instanceP)->setupReq.cell);
  /* mandatory */
  /* c3. C_RNTI */  // 16
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie3);
  ie3->id                             = F1AP_ProtocolIE_ID_id_C_RNTI;
  ie3->criticality                    = F1AP_Criticality_reject;
  ie3->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_C_RNTI;
  ie3->value.choice.C_RNTI=rntiP;
  /* mandatory */
  /* c4. RRCContainer */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie4);
  ie4->id                            = F1AP_ProtocolIE_ID_id_RRCContainer;
  ie4->criticality                   = F1AP_Criticality_reject;
  ie4->value.present                 = F1AP_InitialULRRCMessageTransferIEs__value_PR_RRCContainer;
  OCTET_STRING_fromBuf(&ie4->value.choice.RRCContainer, (const char *)sduP, sdu_lenP);

  /* optional */
  /* c5. DUtoCURRCContainer */
  if (sdu2P) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie5);
    ie5->id                             = F1AP_ProtocolIE_ID_id_DUtoCURRCContainer;
    ie5->criticality                    = F1AP_Criticality_reject;
    ie5->value.present                  = F1AP_InitialULRRCMessageTransferIEs__value_PR_DUtoCURRCContainer;
    OCTET_STRING_fromBuf(&ie5->value.choice.DUtoCURRCContainer,
                         (const char *)sdu2P,
                         sdu2_lenP);
  }
  /* mandatory */
  /* c6. Transaction ID (integer value) */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_InitialULRRCMessageTransferIEs_t, ie6);
  ie6->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie6->criticality               = F1AP_Criticality_ignore;
  ie6->value.present             = F1AP_F1SetupRequestIEs__value_PR_TransactionID;
  ie6->value.choice.TransactionID = F1AP_get_next_transaction_identifier(f1ap_req(false, instanceP)->gNB_DU_id, f1ap_req(false, instanceP)->gNB_DU_id);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 INITIAL UL RRC MESSAGE TRANSFER\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(false, instanceP, buffer, len, getCxt(DUtype, instanceP)->default_sctp_stream_id);
  return 0;
}


int DU_send_UL_NR_RRC_MESSAGE_TRANSFER(instance_t instance,
                                       const f1ap_ul_rrc_message_t *msg) {
  const rnti_t rnti = msg->rnti;
  F1AP_F1AP_PDU_t                pdu= {0};
  F1AP_ULRRCMessageTransfer_t    *out;
  uint8_t *buffer = NULL;
  uint32_t len;
  LOG_I(F1AP, "[DU %ld] %s: size %d UE RNTI %x in SRB %d\n",
        instance, __func__, msg->rrc_container_length, rnti, msg->srb_id);
  //LOG_I(F1AP, "%s() RRCContainer size %d: ", __func__, msg->rrc_container_length);
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
  ie1->value.choice.GNB_CU_UE_F1AP_ID = f1ap_get_cu_ue_f1ap_id(DUtype, instance, rnti);
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_ULRRCMessageTransferIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_ULRRCMessageTransferIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = f1ap_get_du_ue_f1ap_id(DUtype, instance, rnti);
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

  f1ap_itti_send_sctp_data_req(false, instance, buffer, len, getCxt(DUtype, instance)->default_sctp_stream_id);
  return 0;
}
