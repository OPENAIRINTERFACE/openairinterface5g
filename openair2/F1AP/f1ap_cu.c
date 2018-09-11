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

/*! \file openair2/F1AP/CU_F1AP.c
* \brief data structures for F1 interface modules
* \author EURECOM/NTUST
* \date 2018
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr, bing-kai.hong@eurecom.fr
* \note
* \warning
*/

#include "conversions.h"
#include "f1ap_common.h"
#include "f1ap_cu_defs.h"
#include "f1ap_encoder.h"
#include "f1ap_decoder.h"
#include "f1ap_cu_task.h"
#include "platform_types.h"
#include "common/utils/LOG/log.h"
#include "intertask_interface.h"
#include "f1ap_itti_messaging.h"
#include <arpa/inet.h>

#include "T.h"

#define MAX_F1AP_BUFFER_SIZE 4096

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

static f1ap_setup_resp_t *f1ap_cu_data;

/* This structure describes association of a DU to a CU */
typedef struct f1ap_info {

  module_id_t enb_mod_idP;
  module_id_t cu_mod_idP;

  /* Unique eNB_id to identify the eNB within EPC.
   * In our case the eNB is a macro eNB so the id will be 20 bits long.
   * For Home eNB id, this field should be 28 bits long.
   */
  uint32_t GNB_DU_ID;
  
  /* This is the optional name provided by the MME */
  char *GNB_CU_Name;
  f1ap_net_ip_address_t    mme_net_ip_address; // useful for joining assoc_id and ip address of packets

  
  /* Number of input/ouput streams */
  uint16_t in_streams;
  uint16_t out_streams;

  /* Connexion id used between SCTP/S1AP */
  uint16_t cnx_id;

  /* SCTP association id */
  int32_t  assoc_id;

  uint16_t  mcc;
  uint16_t  mnc;
  uint8_t   mnc_digit_length;
  
} f1ap_info_t;

// ==============================================================================
static
void CU_handle_sctp_data_ind(sctp_data_ind_t *sctp_data_ind) {
  int result;

  DevAssert(sctp_data_ind != NULL);

  f1ap_handle_message(sctp_data_ind->assoc_id, sctp_data_ind->stream,
                          sctp_data_ind->buffer, sctp_data_ind->buffer_length);

  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
}

void CU_send_sctp_init_req(instance_t enb_id) {
  // 1. get the itti msg, and retrive the enb_id from the message
  // 2. use RC.rrc[enb_id] to fill the sctp_init_t with the ip, port
  // 3. creat an itti message to init

  LOG_I(CU_F1AP, "F1AP_CU_SCTP_REQ\n");
  MessageDef  *message_p = NULL;

  message_p = itti_alloc_new_message (TASK_CU_F1, SCTP_INIT_MSG);
  message_p->ittiMsg.sctp_init.port = F1AP_PORT_NUMBER;
  message_p->ittiMsg.sctp_init.ppid = F1AP_SCTP_PPID;
  message_p->ittiMsg.sctp_init.ipv4 = 1;
  message_p->ittiMsg.sctp_init.ipv6 = 0;
  message_p->ittiMsg.sctp_init.nb_ipv4_addr = 1;
  message_p->ittiMsg.sctp_init.ipv4_address[0] = inet_addr(RC.rrc[enb_id]->eth_params_s.my_addr);
  /*
   * SR WARNING: ipv6 multi-homing fails sometimes for localhost.
   * * * * Disable it for now.
   */
  message_p->ittiMsg.sctp_init.nb_ipv6_addr = 0;
  message_p->ittiMsg.sctp_init.ipv6_address[0] = "0:0:0:0:0:0:0:1";

  LOG_I(CU_F1AP,"CU.my_addr = %s \n", RC.rrc[enb_id]->eth_params_s.my_addr);
  LOG_I(CU_F1AP,"CU.enb_id = %d \n", enb_id);
  itti_send_msg_to_task(TASK_SCTP, enb_id, message_p);
}

void *F1AP_CU_task(void *arg) {
  //sctp_cu_init();

  MessageDef *received_msg = NULL;
  int         result;

  LOG_I(CU_F1AP,"Starting F1AP at CU\n");

  //f1ap_eNB_prepare_internal_data();

  itti_mark_task_ready(TASK_CU_F1);

  CU_send_sctp_init_req(0);

  while (1) {
    itti_receive_msg(TASK_CU_F1, &received_msg);
    switch (ITTI_MSG_ID(received_msg)) {

      // case F1AP_CU_SCTP_REQ: 
      //   LOG_I(CU_F1AP, "F1AP_CU_SCTP_REQ\n");
        
      //   break;

      case SCTP_NEW_ASSOCIATION_IND:
        LOG_I(CU_F1AP, "SCTP_NEW_ASSOCIATION_IND\n");
        LOG_I(DU_F1AP, "--------------3--------------\n");
        CU_handle_sctp_association_ind(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                         &received_msg->ittiMsg.sctp_new_association_ind);
        break;

      case SCTP_NEW_ASSOCIATION_RESP:
        LOG_I(CU_F1AP, "SCTP_NEW_ASSOCIATION_RESP\n");
        LOG_I(DU_F1AP, "--------------4--------------\n");
        CU_handle_sctp_association_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                         &received_msg->ittiMsg.sctp_new_association_resp);
        break;

      case SCTP_DATA_IND:
        LOG_I(CU_F1AP, "SCTP_DATA_IND\n");
        LOG_I(DU_F1AP, "--------------5--------------\n");
        CU_handle_sctp_data_ind(&received_msg->ittiMsg.sctp_data_ind);
        break;

//    case F1AP_SETUP_RESPONSE: // This is from RRC
//    CU_send_F1_SETUP_RESPONSE(instance, *f1ap_setup_ind, &(F1AP_SETUP_RESP) f1ap_setup_resp)   
//        break;
        
//    case F1AP_SETUP_FAILURE: // This is from RRC
//    CU_send_F1_SETUP_FAILURE(instance, *f1ap_setup_ind, &(F1AP_SETUP_FAILURE) f1ap_setup_failure)   
//       break;

      case TERMINATE_MESSAGE:
        LOG_W(CU_F1AP, " *** Exiting CU_F1AP thread\n");
        itti_exit_task();
        break;

      default:
        LOG_E(CU_F1AP, "CU Received unhandled message: %d:%s\n",
                  ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
        break;
    } // switch
    result = itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);

    received_msg = NULL;
  } // while

  return NULL;
}


void CU_handle_sctp_association_ind(instance_t instance, sctp_new_association_ind_t *sctp_new_association_ind) {
  CU_send_F1_SETUP_RESPONSE(instance, sctp_new_association_ind);
}

void CU_handle_sctp_association_resp(instance_t instance, sctp_new_association_ind_t *sctp_new_association_resp) {
  //CU_send_F1_SETUP_RESPONSE(instance, sctp_new_association_resp);
}

// ==============================================================================
void CU_handle_F1_SETUP_REQUEST(F1AP_F1SetupRequest_t *message_p) {
  F1AP_F1AP_PDU_t           pdu;
  
  uint8_t  *buffer;
  uint32_t  len;
  /* Receiver */
  //f1ap_receiver(&buffer);

  if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
    printf("Failed to decode F1 setup request\n");
  }
  /* decode  */
  //CU_F1AP_decode(args_p);
  // fill in f1ap_setup_req message for RRC task
  
  
  /* handle */

  
  // fill f1ap_setup_req_t 
  // send ITTI F1AP_SETUP_REQ to RRC
  // return

  // send successful callback
  //CU_send_F1_SETUP_RESPONSE();
  // or failure callback
  //CU_send_F1_SETUP_FAILURE();

}

void CU_send_F1_SETUP_RESPONSE(instance_t instance, sctp_new_association_ind_t *f1ap_setup_ind, f1ap_setup_resp_t *f1ap_setup_resp) {
//void CU_send_F1_SETUP_RESPONSE(F1AP_F1SetupResponse_t *F1SetupResponse) {
  //AssertFatal(1==0,"Not implemented yet\n");
  
  module_id_t enb_mod_idP;
  module_id_t cu_mod_idP;

  enb_mod_idP = (module_id_t)12;
  cu_mod_idP  = (module_id_t)34;

  F1AP_F1AP_PDU_t           pdu;
  F1AP_F1SetupResponse_t    *out;
  F1AP_F1SetupResponseIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  f1ap_info_t f1ap_info;
  f1ap_info.GNB_CU_Name = "ABC";
  f1ap_info.mcc = 208;
  f1ap_info.mnc = 93;
  f1ap_info.mnc_digit_length = 8;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (F1AP_SuccessfulOutcome_t *)calloc(1, sizeof(F1AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome->procedureCode = F1AP_ProcedureCode_id_F1Setup;
  pdu.choice.successfulOutcome->criticality   = F1AP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = F1AP_SuccessfulOutcome__value_PR_F1SetupResponse;
  out = &pdu.choice.successfulOutcome->value.choice.F1SetupResponse;
  
  /* mandatory */
  /* c1. Transaction ID (integer value)*/
  ie = (F1AP_F1SetupResponseIEs_t *)calloc(1, sizeof(F1AP_F1SetupResponseIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_F1SetupResponseIEs__value_PR_TransactionID;
  ie->value.choice.TransactionID = F1AP_get_next_transaction_identifier(enb_mod_idP, cu_mod_idP);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c2. GNB_CU_Name */
  if (f1ap_info.GNB_CU_Name != NULL) {
    ie = (F1AP_F1SetupResponseIEs_t *)calloc(1, sizeof(F1AP_F1SetupResponseIEs_t));
    ie->id                        = F1AP_ProtocolIE_ID_id_gNB_CU_Name;
    ie->criticality               = F1AP_Criticality_ignore;
    ie->value.present             = F1AP_F1SetupResponseIEs__value_PR_GNB_CU_Name;
    OCTET_STRING_fromBuf(&ie->value.choice.GNB_CU_Name, f1ap_info.GNB_CU_Name,
                         strlen(f1ap_info.GNB_CU_Name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c3. cells to be Activated list */
  ie = (F1AP_F1SetupResponseIEs_t *)calloc(1, sizeof(F1AP_F1SetupResponseIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_F1SetupResponseIEs__value_PR_Cells_to_be_Activated_List;


  for (i=0;
       i<1;
       i++) {

       F1AP_Cells_to_be_Activated_List_ItemIEs_t *cells_to_be_activated_list_item_ies;
       cells_to_be_activated_list_item_ies = (F1AP_Cells_to_be_Activated_List_ItemIEs_t *)calloc(1, sizeof(F1AP_Cells_to_be_Activated_List_ItemIEs_t));
       cells_to_be_activated_list_item_ies->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item;
       cells_to_be_activated_list_item_ies->criticality = F1AP_Criticality_reject;
       cells_to_be_activated_list_item_ies->value.present = F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item;

     /* 3.1 cells to be Activated list item */
     F1AP_Cells_to_be_Activated_List_Item_t cells_to_be_activated_list_item;
     memset((void *)&cells_to_be_activated_list_item, 0, sizeof(F1AP_Cells_to_be_Activated_List_Item_t));

     /* - nRCGI */
     F1AP_NRCGI_t nRCGI;
     MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                         &nRCGI.pLMN_Identity);
     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
     cells_to_be_activated_list_item.nRCGI = nRCGI;

     /* optional */
     /* - nRPCI */
     if (1) {
       cells_to_be_activated_list_item.nRPCI = (F1AP_NRPCI_t *)calloc(1, sizeof(F1AP_NRPCI_t));
       *cells_to_be_activated_list_item.nRPCI = 321L;  // int 0..1007
     }

     /* optional */
     /* - gNB-CU System Information */
     if (1) {
       /* 3.1.2 gNB-CUSystem Information */
       F1AP_Cells_to_be_Activated_List_ItemExtIEs_t *cells_to_be_activated_list_itemExtIEs;
       cells_to_be_activated_list_itemExtIEs = (F1AP_Cells_to_be_Activated_List_ItemExtIEs_t *)calloc(1, sizeof(F1AP_Cells_to_be_Activated_List_ItemExtIEs_t));
       cells_to_be_activated_list_itemExtIEs->id                     = F1AP_ProtocolIE_ID_id_gNB_CUSystemInformation;
       cells_to_be_activated_list_itemExtIEs->criticality            = F1AP_Criticality_reject;
       cells_to_be_activated_list_itemExtIEs->extensionValue.present = F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_GNB_CUSystemInformation;
       
       
       F1AP_GNB_CUSystemInformation_t gNB_CUSystemInformation;
       memset((void *)&gNB_CUSystemInformation, 0, sizeof(F1AP_GNB_CUSystemInformation_t));

       OCTET_STRING_fromBuf(&gNB_CUSystemInformation.sImessage,
                             "123456", strlen("123456"));

       cells_to_be_activated_list_itemExtIEs->extensionValue.choice.GNB_CUSystemInformation = gNB_CUSystemInformation;


       F1AP_ProtocolExtensionContainer_160P9_t p_160P9_t;
       memset((void *)&p_160P9_t, 0, sizeof(F1AP_ProtocolExtensionContainer_160P9_t));

       ASN_SEQUENCE_ADD(&p_160P9_t.list,
                        cells_to_be_activated_list_itemExtIEs);
       cells_to_be_activated_list_item.iE_Extensions = &p_160P9_t;

     }
     /* ADD */
     cells_to_be_activated_list_item_ies->value.choice.Cells_to_be_Activated_List_Item = cells_to_be_activated_list_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.Cells_to_be_Activated_List.list,
                      cells_to_be_activated_list_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
  }

  // printf("\n");
  cu_f1ap_itti_send_sctp_data_req(instance, f1ap_setup_ind->assoc_id, buffer, len, 0);
  /* decode */
  // if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
  //    printf("Failed to decode F1 setup request\n");
  // }
  //printf("F1 setup response present = %d\n", out->value.present);
  //f1ap_send_sctp_data_req(instance_p->instance, f1ap_mme_data_p->assoc_id, buffer, len, 0);

}

void CU_send_F1_SETUP_FAILURE(F1AP_F1SetupFailure_t *F1SetupFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
  //AssertFatal(1==0,"Not implemented yet\n");
  //f1ap_send_sctp_data_req(instance_p->instance, f1ap_mme_data_p->assoc_id, buffer, len, 0);
}


void CU_handle_ERROR_INDICATION(F1AP_ErrorIndication_t *ErrorIndication) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_send_ERROR_INDICATION(F1AP_ErrorIndication_t *ErrorIndication) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void CU_send_RESET(F1AP_Reset_t *Reset) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_handle_RESET_ACKKNOWLEDGE(F1AP_ResetAcknowledge_t *ResetAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_handle_RESET(F1AP_Reset_t *Reset) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_send_RESET_ACKKNOWLEDGE(F1AP_ResetAcknowledge_t *ResetAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_handle_UL_INITIAL_RRC_MESSAGE_TRANSFER(void) {

  printf("CU_handle_UL_INITIAL_RRC_MESSAGE_TRANSFER\n");
  // decode the F1 message
  // get the rrc message from the contauiner 
  // call func rrc_eNB_decode_ccch: <-- needs some update here

  // if size > 0 
  // CU_send_DL_RRC_MESSAGE_TRANSFER(C.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.Payload, RC.rrc[ctxt_pP->module_id]->carrier[CC_id].Srb0.Tx_buffer.payload_size)
}

void CU_handle_UL_RRC_MESSAGE_TRANSFER(F1AP_ULRRCMessageTransfer_t *ULRRCMessageTransfer) {
  AssertFatal(1==0,"Not implemented yet\n");
}

//void CU_send_DL_RRC_MESSAGE_TRANSFER(F1AP_DLRRCMessageTransfer_t *DLRRCMessageTransfer) {
void CU_send_DL_RRC_MESSAGE_TRANSFER(void) {
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
  }

  printf("\n");

  /* decode */
  if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
    printf("Failed to decode F1 setup request\n");
  }
  //AssertFatal(1==0,"Not implemented yet\n");
}


void CU_handle_gNB_DU_CONFIGURATION_UPDATE(F1AP_GNBDUConfigurationUpdate_t *GNBDUConfigurationUpdate) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_send_gNB_DU_CONFIGURATION_FAILURE(F1AP_GNBDUConfigurationUpdateFailure_t *GNBDUConfigurationUpdateFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_send_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(F1AP_GNBDUConfigurationUpdateAcknowledge_t *GNBDUConfigurationUpdateAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}


//void CU_send_gNB_CU_CONFIGURATION_UPDATE(F1AP_GNBCUConfigurationUpdate_t *GNBCUConfigurationUpdate) {
void CU_send_gNB_CU_CONFIGURATION_UPDATE(module_id_t enb_mod_idP, module_id_t du_mod_idP) {
  F1AP_F1AP_PDU_t                    pdu;
  F1AP_GNBCUConfigurationUpdate_t    *out;
  F1AP_GNBCUConfigurationUpdateIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  f1ap_info_t f1ap_info;
  f1ap_info.GNB_CU_Name = "ABC";
  f1ap_info.mcc = 208;
  f1ap_info.mnc = 93;
  f1ap_info.mnc_digit_length = 8;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_gNBCUConfigurationUpdate;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_GNBCUConfigurationUpdate;
  out = &pdu.choice.initiatingMessage->value.choice.GNBCUConfigurationUpdate;

  /* mandatory */
  /* c1. Transaction ID (integer value) */
  ie = (F1AP_GNBCUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBCUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_TransactionID;
  ie->value.choice.TransactionID = F1AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);



  /* mandatory */
  /* c2. Cells_to_be_Activated_List */
  ie = (F1AP_GNBCUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBCUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Activated_List;

  for (i=0;
       i<1;
       i++) {

       F1AP_Cells_to_be_Activated_List_ItemIEs_t *cells_to_be_activated_list_item_ies;
       cells_to_be_activated_list_item_ies = (F1AP_Cells_to_be_Activated_List_ItemIEs_t *)calloc(1, sizeof(F1AP_Cells_to_be_Activated_List_ItemIEs_t));
       cells_to_be_activated_list_item_ies->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item;
       cells_to_be_activated_list_item_ies->criticality = F1AP_Criticality_reject;
       cells_to_be_activated_list_item_ies->value.present = F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item;

     /* 2.1 cells to be Activated list item */
     F1AP_Cells_to_be_Activated_List_Item_t cells_to_be_activated_list_item;
     memset((void *)&cells_to_be_activated_list_item, 0, sizeof(F1AP_Cells_to_be_Activated_List_Item_t));

     /* - nRCGI */
     F1AP_NRCGI_t nRCGI;
     MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                         &nRCGI.pLMN_Identity);
     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
     cells_to_be_activated_list_item.nRCGI = nRCGI;

     /* optional */
     /* - nRPCI */
     if (0) {
       cells_to_be_activated_list_item.nRPCI = (F1AP_NRPCI_t *)calloc(1, sizeof(F1AP_NRPCI_t));
       *cells_to_be_activated_list_item.nRPCI = 321L;  // int 0..1007
     }

     /* optional */
     /* - gNB-CU System Information */
     //if (1) {

     //}
     /* ADD */
     cells_to_be_activated_list_item_ies->value.choice.Cells_to_be_Activated_List_Item = cells_to_be_activated_list_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.Cells_to_be_Activated_List.list,
                      cells_to_be_activated_list_item_ies);
  }  
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);



  /* mandatory */
  /* c3. Cells_to_be_Deactivated_List */
  ie = (F1AP_GNBCUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBCUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_Cells_to_be_Deactivated_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Deactivated_List;

  for (i=0;
       i<1;
       i++) {

       F1AP_Cells_to_be_Deactivated_List_ItemIEs_t *cells_to_be_deactivated_list_item_ies;
       cells_to_be_deactivated_list_item_ies = (F1AP_Cells_to_be_Deactivated_List_ItemIEs_t *)calloc(1, sizeof(F1AP_Cells_to_be_Deactivated_List_ItemIEs_t));
       cells_to_be_deactivated_list_item_ies->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item;
       cells_to_be_deactivated_list_item_ies->criticality = F1AP_Criticality_reject;
       cells_to_be_deactivated_list_item_ies->value.present = F1AP_Cells_to_be_Deactivated_List_ItemIEs__value_PR_Cells_to_be_Deactivated_List_Item;

       /* 3.1 cells to be Deactivated list item */
       F1AP_Cells_to_be_Deactivated_List_Item_t cells_to_be_deactivated_list_item;
       memset((void *)&cells_to_be_deactivated_list_item, 0, sizeof(F1AP_Cells_to_be_Deactivated_List_Item_t));

       /* - nRCGI */
       F1AP_NRCGI_t nRCGI;
       MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                           &nRCGI.pLMN_Identity);
       NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
       cells_to_be_deactivated_list_item.nRCGI = nRCGI;

       //}
       /* ADD */
       cells_to_be_deactivated_list_item_ies->value.choice.Cells_to_be_Deactivated_List_Item = cells_to_be_deactivated_list_item;
       ASN_SEQUENCE_ADD(&ie->value.choice.Cells_to_be_Deactivated_List.list,
                        cells_to_be_deactivated_list_item_ies);
  }  
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c4. GNB_CU_TNL_Association_To_Add_List */
  ie = (F1AP_GNBCUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBCUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Add_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_GNB_CU_TNL_Association_To_Add_List;

  for (i=0;
       i<1;
       i++) {

       F1AP_GNB_CU_TNL_Association_To_Add_ItemIEs_t *gnb_cu_tnl_association_to_add_item_ies;
       gnb_cu_tnl_association_to_add_item_ies = (F1AP_GNB_CU_TNL_Association_To_Add_ItemIEs_t *)calloc(1, sizeof(F1AP_GNB_CU_TNL_Association_To_Add_ItemIEs_t));
       gnb_cu_tnl_association_to_add_item_ies->id = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Add_Item;
       gnb_cu_tnl_association_to_add_item_ies->criticality = F1AP_Criticality_reject;
       gnb_cu_tnl_association_to_add_item_ies->value.present = F1AP_GNB_CU_TNL_Association_To_Add_ItemIEs__value_PR_GNB_CU_TNL_Association_To_Add_Item;

       /* 4.1 GNB_CU_TNL_Association_To_Add_Item */
       F1AP_GNB_CU_TNL_Association_To_Add_Item_t gnb_cu_tnl_association_to_add_item;
       memset((void *)&gnb_cu_tnl_association_to_add_item, 0, sizeof(F1AP_GNB_CU_TNL_Association_To_Add_Item_t));


       /* 4.1.1 tNLAssociationTransportLayerAddress */
       F1AP_CP_TransportLayerAddress_t transportLayerAddress;
       memset((void *)&transportLayerAddress, 0, sizeof(F1AP_CP_TransportLayerAddress_t));
       transportLayerAddress.present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address;
       TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &transportLayerAddress.choice.endpoint_IP_address);
       
       // memset((void *)&transportLayerAddress, 0, sizeof(F1AP_CP_TransportLayerAddress_t));
       // transportLayerAddress.present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address_and_port;
       // transportLayerAddress.choice.endpoint_IP_address_and_port = (F1AP_Endpoint_IP_address_and_port_t *)calloc(1, sizeof(F1AP_Endpoint_IP_address_and_port_t));
       // TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &transportLayerAddress.choice.endpoint_IP_address_and_port.endpoint_IP_address);

       gnb_cu_tnl_association_to_add_item.tNLAssociationTransportLayerAddress = transportLayerAddress;

       /* 4.1.2 tNLAssociationUsage */
       gnb_cu_tnl_association_to_add_item.tNLAssociationUsage = F1AP_TNLAssociationUsage_non_ue;
       

       /* ADD */
       gnb_cu_tnl_association_to_add_item_ies->value.choice.GNB_CU_TNL_Association_To_Add_Item = gnb_cu_tnl_association_to_add_item;
       ASN_SEQUENCE_ADD(&ie->value.choice.GNB_CU_TNL_Association_To_Add_List.list,
                        gnb_cu_tnl_association_to_add_item_ies);
  }  
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);



  /* mandatory */
  /* c5. GNB_CU_TNL_Association_To_Remove_List */
  ie = (F1AP_GNBCUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBCUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Remove_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_GNB_CU_TNL_Association_To_Remove_List;
  for (i=0;
       i<1;
       i++) {

       F1AP_GNB_CU_TNL_Association_To_Remove_ItemIEs_t *gnb_cu_tnl_association_to_remove_item_ies;
       gnb_cu_tnl_association_to_remove_item_ies = (F1AP_GNB_CU_TNL_Association_To_Remove_ItemIEs_t *)calloc(1, sizeof(F1AP_GNB_CU_TNL_Association_To_Remove_ItemIEs_t));
       gnb_cu_tnl_association_to_remove_item_ies->id = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Remove_Item;
       gnb_cu_tnl_association_to_remove_item_ies->criticality = F1AP_Criticality_reject;
       gnb_cu_tnl_association_to_remove_item_ies->value.present = F1AP_GNB_CU_TNL_Association_To_Remove_ItemIEs__value_PR_GNB_CU_TNL_Association_To_Remove_Item;

       /* 4.1 GNB_CU_TNL_Association_To_Remove_Item */
       F1AP_GNB_CU_TNL_Association_To_Remove_Item_t gnb_cu_tnl_association_to_remove_item;
       memset((void *)&gnb_cu_tnl_association_to_remove_item, 0, sizeof(F1AP_GNB_CU_TNL_Association_To_Remove_Item_t));


       /* 4.1.1 tNLAssociationTransportLayerAddress */
       F1AP_CP_TransportLayerAddress_t transportLayerAddress;
       memset((void *)&transportLayerAddress, 0, sizeof(F1AP_CP_TransportLayerAddress_t));
       transportLayerAddress.present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address;
       TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &transportLayerAddress.choice.endpoint_IP_address);
       
       // memset((void *)&transportLayerAddress, 0, sizeof(F1AP_CP_TransportLayerAddress_t));
       // transportLayerAddress.present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address_and_port;
       // transportLayerAddress.choice.endpoint_IP_address_and_port = (F1AP_Endpoint_IP_address_and_port_t *)calloc(1, sizeof(F1AP_Endpoint_IP_address_and_port_t));
       // TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &transportLayerAddress.choice.endpoint_IP_address_and_port.endpoint_IP_address);

       gnb_cu_tnl_association_to_remove_item.tNLAssociationTransportLayerAddress = transportLayerAddress;
   

       /* ADD */
       gnb_cu_tnl_association_to_remove_item_ies->value.choice.GNB_CU_TNL_Association_To_Remove_Item = gnb_cu_tnl_association_to_remove_item;
       ASN_SEQUENCE_ADD(&ie->value.choice.GNB_CU_TNL_Association_To_Remove_List.list,
                        gnb_cu_tnl_association_to_remove_item_ies);
  }  
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c6. GNB_CU_TNL_Association_To_Update_List */
  ie = (F1AP_GNBCUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBCUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Update_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_GNB_CU_TNL_Association_To_Update_List;
  for (i=0;
       i<1;
       i++) {

       F1AP_GNB_CU_TNL_Association_To_Update_ItemIEs_t *gnb_cu_tnl_association_to_update_item_ies;
       gnb_cu_tnl_association_to_update_item_ies = (F1AP_GNB_CU_TNL_Association_To_Update_ItemIEs_t *)calloc(1, sizeof(F1AP_GNB_CU_TNL_Association_To_Update_ItemIEs_t));
       gnb_cu_tnl_association_to_update_item_ies->id = F1AP_ProtocolIE_ID_id_GNB_CU_TNL_Association_To_Update_Item;
       gnb_cu_tnl_association_to_update_item_ies->criticality = F1AP_Criticality_reject;
       gnb_cu_tnl_association_to_update_item_ies->value.present = F1AP_GNB_CU_TNL_Association_To_Update_ItemIEs__value_PR_GNB_CU_TNL_Association_To_Update_Item;

       /* 4.1 GNB_CU_TNL_Association_To_Update_Item */
       F1AP_GNB_CU_TNL_Association_To_Update_Item_t gnb_cu_tnl_association_to_update_item;
       memset((void *)&gnb_cu_tnl_association_to_update_item, 0, sizeof(F1AP_GNB_CU_TNL_Association_To_Update_Item_t));


       /* 4.1.1 tNLAssociationTransportLayerAddress */
       F1AP_CP_TransportLayerAddress_t transportLayerAddress;
       memset((void *)&transportLayerAddress, 0, sizeof(F1AP_CP_TransportLayerAddress_t));
       transportLayerAddress.present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address;
       TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &transportLayerAddress.choice.endpoint_IP_address);
       
       // memset((void *)&transportLayerAddress, 0, sizeof(F1AP_CP_TransportLayerAddress_t));
       // transportLayerAddress.present = F1AP_CP_TransportLayerAddress_PR_endpoint_IP_address_and_port;
       // transportLayerAddress.choice.endpoint_IP_address_and_port = (F1AP_Endpoint_IP_address_and_port_t *)calloc(1, sizeof(F1AP_Endpoint_IP_address_and_port_t));
       // TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &transportLayerAddress.choice.endpoint_IP_address_and_port.endpoint_IP_address);

       gnb_cu_tnl_association_to_update_item.tNLAssociationTransportLayerAddress = transportLayerAddress;
   

       /* 4.1.2 tNLAssociationUsage */
       if (1) {
         gnb_cu_tnl_association_to_update_item.tNLAssociationUsage = (F1AP_TNLAssociationUsage_t *)calloc(1, sizeof(F1AP_TNLAssociationUsage_t));
         *gnb_cu_tnl_association_to_update_item.tNLAssociationUsage = F1AP_TNLAssociationUsage_non_ue;
       }
       
       /* ADD */
       gnb_cu_tnl_association_to_update_item_ies->value.choice.GNB_CU_TNL_Association_To_Update_Item = gnb_cu_tnl_association_to_update_item;
       ASN_SEQUENCE_ADD(&ie->value.choice.GNB_CU_TNL_Association_To_Update_List.list,
                        gnb_cu_tnl_association_to_update_item_ies);
  }  
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);



  /* mandatory */
  /* c7. Cells_to_be_Barred_List */
  ie = (F1AP_GNBCUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBCUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_Cells_to_be_Barred_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Barred_List;
  for (i=0;
       i<1;
       i++) {

       F1AP_Cells_to_be_Barred_ItemIEs_t *cells_to_be_barred_item_ies;
       cells_to_be_barred_item_ies = (F1AP_Cells_to_be_Barred_ItemIEs_t *)calloc(1, sizeof(F1AP_Cells_to_be_Barred_ItemIEs_t));
       cells_to_be_barred_item_ies->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item;
       cells_to_be_barred_item_ies->criticality = F1AP_Criticality_reject;
       cells_to_be_barred_item_ies->value.present = F1AP_Cells_to_be_Barred_ItemIEs__value_PR_Cells_to_be_Barred_Item;

       /* 7.1 cells to be Deactivated list item */
       F1AP_Cells_to_be_Barred_Item_t cells_to_be_barred_item;
       memset((void *)&cells_to_be_barred_item, 0, sizeof(F1AP_Cells_to_be_Barred_Item_t));

       /* - nRCGI */
       F1AP_NRCGI_t nRCGI;
       MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                           &nRCGI.pLMN_Identity);
       NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
       cells_to_be_barred_item.nRCGI = nRCGI;
       
       /* 7.2 cellBarred*/
       cells_to_be_barred_item.cellBarred = F1AP_CellBarred_not_barred;

       /* ADD */
       cells_to_be_barred_item_ies->value.choice.Cells_to_be_Barred_Item = cells_to_be_barred_item;
       ASN_SEQUENCE_ADD(&ie->value.choice.Cells_to_be_Barred_List.list,
                        cells_to_be_barred_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);



  /* mandatory */
  /* c8. Protected_EUTRA_Resources_List */
  ie = (F1AP_GNBCUConfigurationUpdateIEs_t *)calloc(1, sizeof(F1AP_GNBCUConfigurationUpdateIEs_t));
  ie->id                        = F1AP_ProtocolIE_ID_id_Protected_EUTRA_Resources_List;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_Protected_EUTRA_Resources_List;

  for (i=0;
       i<1;
       i++) {


       F1AP_Protected_EUTRA_Resources_ItemIEs_t *protected_eutra_resources_item_ies;

       /* 8.1 SpectrumSharingGroupID */
       protected_eutra_resources_item_ies = (F1AP_Protected_EUTRA_Resources_ItemIEs_t *)calloc(1, sizeof(F1AP_Protected_EUTRA_Resources_ItemIEs_t));
       protected_eutra_resources_item_ies->id = F1AP_ProtocolIE_ID_id_Protected_EUTRA_Resources_List;
       protected_eutra_resources_item_ies->criticality = F1AP_Criticality_reject;
       protected_eutra_resources_item_ies->value.present = F1AP_Protected_EUTRA_Resources_ItemIEs__value_PR_SpectrumSharingGroupID;
       protected_eutra_resources_item_ies->value.choice.SpectrumSharingGroupID = 1L;

       ASN_SEQUENCE_ADD(&ie->value.choice.Protected_EUTRA_Resources_List.list, protected_eutra_resources_item_ies);

       /* 8.2 ListofEUTRACellsinGNBDUCoordination */
       protected_eutra_resources_item_ies = (F1AP_Protected_EUTRA_Resources_ItemIEs_t *)calloc(1, sizeof(F1AP_Protected_EUTRA_Resources_ItemIEs_t));
       protected_eutra_resources_item_ies->id = F1AP_ProtocolIE_ID_id_Protected_EUTRA_Resources_List;
       protected_eutra_resources_item_ies->criticality = F1AP_Criticality_reject;
       protected_eutra_resources_item_ies->value.present = F1AP_Protected_EUTRA_Resources_ItemIEs__value_PR_ListofEUTRACellsinGNBDUCoordination;

       F1AP_Served_EUTRA_Cells_Information_t served_eutra_cells_information;
       memset((void *)&served_eutra_cells_information, 0, sizeof(F1AP_Served_EUTRA_Cells_Information_t));

       F1AP_EUTRA_Mode_Info_t eUTRA_Mode_Info;
       memset((void *)&eUTRA_Mode_Info, 0, sizeof(F1AP_EUTRA_Mode_Info_t));

       // eUTRAFDD
       eUTRA_Mode_Info.present = F1AP_EUTRA_Mode_Info_PR_eUTRAFDD;
       F1AP_EUTRA_FDD_Info_t *eutra_fdd_info;
       eutra_fdd_info = (F1AP_EUTRA_FDD_Info_t *)calloc(1, sizeof(F1AP_EUTRA_FDD_Info_t));
       eutra_fdd_info->uL_offsetToPointA = 123L;
       eutra_fdd_info->dL_offsetToPointA = 456L;
       eUTRA_Mode_Info.choice.eUTRAFDD = eutra_fdd_info;

       // eUTRATDD
       // eUTRA_Mode_Info.present = F1AP_EUTRA_Mode_Info_PR_eUTRATDD;
       // F1AP_EUTRA_TDD_Info_t *eutra_tdd_info;
       // eutra_tdd_info = (F1AP_EUTRA_TDD_Info_t *)calloc(1, sizeof(F1AP_EUTRA_TDD_Info_t));
       // eutra_tdd_info->uL_offsetToPointA = 123L;
       // eutra_tdd_info->dL_offsetToPointA = 456L;
       // eUTRA_Mode_Info.choice.eUTRATDD = eutra_tdd_info;

       served_eutra_cells_information.eUTRA_Mode_Info = eUTRA_Mode_Info;

       OCTET_STRING_fromBuf(&served_eutra_cells_information.protectedEUTRAResourceIndication, "asdsa1d32sa1d31asd31as",
                       strlen("asdsa1d32sa1d31asd31as"));

       ASN_SEQUENCE_ADD(&protected_eutra_resources_item_ies->value.choice.ListofEUTRACellsinGNBDUCoordination.list, &served_eutra_cells_information);

       ASN_SEQUENCE_ADD(&ie->value.choice.Protected_EUTRA_Resources_List.list, protected_eutra_resources_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
    return;
  }

  printf("\n");

  /* decode */
  if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
    printf("Failed to decode F1 setup request\n");
  }
}

void CU_handle_gNB_CU_CONFIGURATION_UPDATE_FALIURE(F1AP_GNBCUConfigurationUpdateFailure_t *GNBCUConfigurationUpdateFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_send_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(F1AP_GNBCUConfigurationUpdateAcknowledge_t *GNBCUConfigurationUpdateAcknowledge) {
  AssertFatal(1==0,"Not implemented yet\n");
}


//void CU_send_UE_CONTEXT_SETUP_REQUEST(F1AP_UEContextSetupRequest_t *UEContextSetupRequest) {
void CU_send_UE_CONTEXT_SETUP_REQUEST(void) {
  F1AP_F1AP_PDU_t                 pdu;
  F1AP_UEContextSetupRequest_t    *out;
  F1AP_UEContextSetupRequestIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  f1ap_info_t f1ap_info;
  f1ap_info.GNB_CU_Name = "ABC";
  f1ap_info.mcc = 208;
  f1ap_info.mnc = 93;
  f1ap_info.mnc_digit_length = 8;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_UEContextSetup;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_UEContextSetupRequest;
  out = &pdu.choice.initiatingMessage->value.choice.UEContextSetupRequest;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c2. GNB_DU_UE_F1AP_ID */
  if (0) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_GNB_DU_UE_F1AP_ID;
    ie->value.choice.GNB_DU_UE_F1AP_ID = 651L;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c3. SpCell_ID */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SpCell_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_NRCGI;
  /* - nRCGI */
  F1AP_NRCGI_t nRCGI;
  MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                       &nRCGI.pLMN_Identity);
  NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);

  ie->value.choice.NRCGI = nRCGI;

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c4. ServCellIndex */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_ServCellndex;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_ServCellIndex;
  ie->value.choice.ServCellIndex = 2;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c5. CellULConfigured */
  if (0) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_SpCellULConfigured;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_CellULConfigured;
    ie->value.choice.CellULConfigured = F1AP_CellULConfigured_ul;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c6. CUtoDURRCInformation */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_CUtoDURRCInformation;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_CUtoDURRCInformation;
  ie->value.choice.CUtoDURRCInformation.cG_ConfigInfo = (F1AP_CG_ConfigInfo_t *)calloc(1, sizeof(F1AP_CG_ConfigInfo_t));
  /* optional */
  OCTET_STRING_fromBuf(ie->value.choice.CUtoDURRCInformation.cG_ConfigInfo, "asdsa1d32sa1d31asd31as",
                       strlen("asdsa1d32sa1d31asd31as"));
  ie->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList = (F1AP_UE_CapabilityRAT_ContainerList_t *)calloc(1, sizeof(F1AP_UE_CapabilityRAT_ContainerList_t));
  /* optional */
  OCTET_STRING_fromBuf(ie->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList, "asdsa1d32sa1d31asd31as",
                       strlen("asdsa1d32sa1d31asd31as"));
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c7. Candidate_SpCell_List */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_Candidate_SpCell_List;  //90
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_Candidate_SpCell_List;

  for (i=0;
       i<1;
       i++) {

    F1AP_Candidate_SpCell_ItemIEs_t *candidate_spCell_item_ies;
    candidate_spCell_item_ies = (F1AP_Candidate_SpCell_ItemIEs_t *)calloc(1, sizeof(F1AP_Candidate_SpCell_ItemIEs_t));
    candidate_spCell_item_ies->id            = F1AP_ProtocolIE_ID_id_Candidate_SpCell_Item; // 91
    candidate_spCell_item_ies->criticality   = F1AP_Criticality_reject;
    candidate_spCell_item_ies->value.present = F1AP_Candidate_SpCell_ItemIEs__value_PR_Candidate_SpCell_Item;

    /* 5.1 Candidate_SpCell_Item */
    F1AP_Candidate_SpCell_Item_t candidate_spCell_item;
    memset((void *)&candidate_spCell_item, 0, sizeof(F1AP_Candidate_SpCell_Item_t));

    /* - candidate_SpCell_ID */
    F1AP_NRCGI_t nRCGI;
    MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                       &nRCGI.pLMN_Identity);
    NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);

    candidate_spCell_item.candidate_SpCell_ID = nRCGI;

    /* ADD */
    candidate_spCell_item_ies->value.choice.Candidate_SpCell_Item = candidate_spCell_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.Candidate_SpCell_List.list,
                    candidate_spCell_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c8. DRXCycle */
  if (0) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_DRXCycle;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_DRXCycle;
    ie->value.choice.DRXCycle.longDRXCycleLength = F1AP_LongDRXCycleLength_ms10; // enum
    if (0) {
      ie->value.choice.DRXCycle.shortDRXCycleLength = (F1AP_ShortDRXCycleLength_t *)calloc(1, sizeof(F1AP_ShortDRXCycleLength_t));
      *ie->value.choice.DRXCycle.shortDRXCycleLength = F1AP_ShortDRXCycleLength_ms2; // enum
    }
    if (0) {
      ie->value.choice.DRXCycle.shortDRXCycleTimer = (F1AP_ShortDRXCycleTimer_t *)calloc(1, sizeof(F1AP_ShortDRXCycleTimer_t));
      *ie->value.choice.DRXCycle.shortDRXCycleTimer = 123L;
    }
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c9. ResourceCoordinationTransferContainer */
  if (0) {
    ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_ResourceCoordinationTransferContainer;
    
    ie->value.choice.ResourceCoordinationTransferContainer.buf = malloc(4);
    ie->value.choice.ResourceCoordinationTransferContainer.size = 4;
    *ie->value.choice.ResourceCoordinationTransferContainer.buf = "123";


    OCTET_STRING_fromBuf(&ie->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c10. SCell_ToBeSetup_List */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeSetup_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_SCell_ToBeSetup_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_SCell_ToBeSetup_ItemIEs_t *scell_toBeSetup_item_ies;
     scell_toBeSetup_item_ies = (F1AP_SCell_ToBeSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_ToBeSetup_ItemIEs_t));
     scell_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeSetup_Item; //53
     scell_toBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
     scell_toBeSetup_item_ies->value.present = F1AP_SCell_ToBeSetup_ItemIEs__value_PR_SCell_ToBeSetup_Item;

     /* 8.1 SCell_ToBeSetup_Item */
     F1AP_SCell_ToBeSetup_Item_t scell_toBeSetup_item;
     memset((void *)&scell_toBeSetup_item, 0, sizeof(F1AP_SCell_ToBeSetup_Item_t));

     //   /* - sCell_ID */
     F1AP_NRCGI_t nRCGI;
     MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                        &nRCGI.pLMN_Identity);
     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);

     scell_toBeSetup_item.sCell_ID = nRCGI;

     /* sCellIndex */
     scell_toBeSetup_item.sCellIndex = 3;  // issue here
     //   /* ADD */
     scell_toBeSetup_item_ies->value.choice.SCell_ToBeSetup_Item = scell_toBeSetup_item;

     ASN_SEQUENCE_ADD(&ie->value.choice.SCell_ToBeSetup_List.list,
                     scell_toBeSetup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // /* mandatory */
  /* c11. SRBs_ToBeSetup_List */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetup_List;
  ie->criticality                    = F1AP_Criticality_reject;  // ?
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_SRBs_ToBeSetup_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_SRBs_ToBeSetup_ItemIEs_t *srbs_toBeSetup_item_ies;
    srbs_toBeSetup_item_ies = (F1AP_SRBs_ToBeSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_SRBs_ToBeSetup_ItemIEs_t));
    srbs_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetup_Item; // 73
    srbs_toBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_toBeSetup_item_ies->value.present = F1AP_SRBs_ToBeSetup_ItemIEs__value_PR_SRBs_ToBeSetup_Item;

    /* 9.1 SRBs_ToBeSetup_Item */
    F1AP_SRBs_ToBeSetup_Item_t srbs_toBeSetup_item;
    memset((void *)&srbs_toBeSetup_item, 0, sizeof(F1AP_SRBs_ToBeSetup_Item_t));

    /* - sRBID */
    srbs_toBeSetup_item.sRBID = 2L;

    /* ADD */
    srbs_toBeSetup_item_ies->value.choice.SRBs_ToBeSetup_Item = srbs_toBeSetup_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_ToBeSetup_List.list,
                    srbs_toBeSetup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c12. DRBs_ToBeSetup_List */
  ie = (F1AP_UEContextSetupRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetup_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_DRBs_ToBeSetup_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_ToBeSetup_ItemIEs_t *drbs_toBeSetup_item_ies;
    drbs_toBeSetup_item_ies = (F1AP_DRBs_ToBeSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_ToBeSetup_ItemIEs_t));
    drbs_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetup_Item;
    drbs_toBeSetup_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeSetup_item_ies->value.present = F1AP_DRBs_ToBeSetup_ItemIEs__value_PR_DRBs_ToBeSetup_Item;

    /* 10.1 DRBs_ToBeSetup_Item */
    F1AP_DRBs_ToBeSetup_Item_t drbs_toBeSetup_item;
    memset((void *)&drbs_toBeSetup_item, 0, sizeof(F1AP_DRBs_ToBeSetup_Item_t));

    /* dRBID */
    drbs_toBeSetup_item.dRBID = 30L;

    /* qoSInformation */
    drbs_toBeSetup_item.qoSInformation.present = F1AP_QoSInformation_PR_eUTRANQoS;
    drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS = (F1AP_EUTRANQoS_t *)calloc(1, sizeof(F1AP_EUTRANQoS_t));
    drbs_toBeSetup_item.qoSInformation.choice.eUTRANQoS->qCI = 254L;

    /* ULTunnels_ToBeSetup_List */
    int maxnoofULTunnels = 1; // 2;
    for (i=0;
            i<maxnoofULTunnels;
            i++) {
            /*  ULTunnels_ToBeSetup_Item */
            F1AP_ULUPTNLInformation_ToBeSetup_Item_t *uLUPTNLInformation_ToBeSetup_Item;

            // gTPTunnel
            uLUPTNLInformation_ToBeSetup_Item = (F1AP_ULUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_ULUPTNLInformation_ToBeSetup_Item_t));
            uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
            F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

            TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

            OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1234",
                           strlen("1234"));
            
            uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel = gTPTunnel;

            ASN_SEQUENCE_ADD(&drbs_toBeSetup_item.uLUPTNLInformation_ToBeSetup_List.list, uLUPTNLInformation_ToBeSetup_Item);
    }

    /* rLCMode */
    drbs_toBeSetup_item.rLCMode = F1AP_RLCMode_rlc_um; // enum

    /* OPTIONAL */
    /* ULConfiguration */
    if (0) {
       drbs_toBeSetup_item.uLConfiguration = (F1AP_ULConfiguration_t *)calloc(1, sizeof(F1AP_ULConfiguration_t));
    }

    /* ADD */
    drbs_toBeSetup_item_ies->value.choice.DRBs_ToBeSetup_Item = drbs_toBeSetup_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_ToBeSetup_List.list,
                   drbs_toBeSetup_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* OPTIONAL */
  if (0) {
      //F1AP_InactivityMonitoringRequest_t   InactivityMonitoringRequest;
      //F1AP_RAT_FrequencyPriorityInformation_t  RAT_FrequencyPriorityInformation;
      //F1AP_RRCContainer_t  RRCContainer;
      //F1AP_MaskedIMEISV_t  MaskedIMEISV;
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
    return;
  }

  printf("\n");

  /* decode */
  if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
    printf("Failed to decode F1 setup request\n");
  }
  //AssertFatal(1==0,"Not implemented yet\n");
}

void CU_handle_UE_CONTEXT_SETUP_RESPONSE(F1AP_UEContextSetupResponse_t *UEContextSetupResponse) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_handle_UE_CONTEXT_SETUP_FAILURE(F1AP_UEContextSetupFailure_t UEContextSetupFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void CU_handle_UE_CONTEXT_RELEASE_REQUEST(F1AP_UEContextReleaseRequest_t *UEContextReleaseRequest) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void CU_send_UE_CONTEXT_RELEASE_COMMAND(F1AP_UEContextReleaseCommand_t *UEContextReleaseCommand) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void CU_handle_UE_CONTEXT_RELEASE_COMPLETE(F1AP_UEContextReleaseComplete_t *UEContextReleaseComplete) {
  AssertFatal(1==0,"Not implemented yet\n");
}

//void CU_send_UE_CONTEXT_MODIFICATION_REQUEST(F1AP_UEContextModificationRequest_t *UEContextModificationRequest) {
void CU_send_UE_CONTEXT_MODIFICATION_REQUEST(void) {
  F1AP_F1AP_PDU_t                        pdu;
  F1AP_UEContextModificationRequest_t    *out;
  F1AP_UEContextModificationRequestIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  f1ap_info_t f1ap_info;
  f1ap_info.GNB_CU_Name = "ABC";
  f1ap_info.mcc = 208;
  f1ap_info.mnc = 93;
  f1ap_info.mnc_digit_length = 8;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (F1AP_InitiatingMessage_t *)calloc(1, sizeof(F1AP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = F1AP_ProcedureCode_id_UEContextModification;
  pdu.choice.initiatingMessage->criticality   = F1AP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = F1AP_InitiatingMessage__value_PR_UEContextModificationRequest;
  out = &pdu.choice.initiatingMessage->value.choice.UEContextModificationRequest;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = 651L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c3. NRCGI */
  if (1) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_SpCell_ID;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_NRCGI;
    /* - nRCGI */
    F1AP_NRCGI_t nRCGI;
    MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                         &nRCGI.pLMN_Identity);
    NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
    ie->value.choice.NRCGI = nRCGI;

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c4. ServCellIndex */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_ServCellndex;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_ServCellIndex;
  ie->value.choice.ServCellIndex     = 5L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c5. DRXCycle */
  if (0) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_DRXCycle;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRXCycle;
    ie->value.choice.DRXCycle.longDRXCycleLength = F1AP_LongDRXCycleLength_ms10; // enum
    if (0) {
      ie->value.choice.DRXCycle.shortDRXCycleLength = (F1AP_ShortDRXCycleLength_t *)calloc(1, sizeof(F1AP_ShortDRXCycleLength_t));
      *ie->value.choice.DRXCycle.shortDRXCycleLength = F1AP_ShortDRXCycleLength_ms2; // enum
    }
    if (0) {
      ie->value.choice.DRXCycle.shortDRXCycleTimer = (F1AP_ShortDRXCycleTimer_t *)calloc(1, sizeof(F1AP_ShortDRXCycleTimer_t));
      *ie->value.choice.DRXCycle.shortDRXCycleTimer = 123L;
    }
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c5. CUtoDURRCInformation */
  if (1) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_CUtoDURRCInformation;
    ie->criticality                    = F1AP_Criticality_reject;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_CUtoDURRCInformation;
    ie->value.choice.CUtoDURRCInformation.cG_ConfigInfo = (F1AP_CG_ConfigInfo_t *)calloc(1, sizeof(F1AP_CG_ConfigInfo_t));
    /* optional */
    OCTET_STRING_fromBuf(ie->value.choice.CUtoDURRCInformation.cG_ConfigInfo, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    ie->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList = (F1AP_UE_CapabilityRAT_ContainerList_t *)calloc(1, sizeof(F1AP_UE_CapabilityRAT_ContainerList_t));
    /* optional */
    OCTET_STRING_fromBuf(ie->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c6. TransmissionStopIndicator */
  if (1) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                                     = F1AP_ProtocolIE_ID_id_TransmissionStopIndicator;
    ie->criticality                            = F1AP_Criticality_ignore;
    ie->value.present                          = F1AP_UEContextModificationRequestIEs__value_PR_TransmissionStopIndicator;
    ie->value.choice.TransmissionStopIndicator = F1AP_TransmissionStopIndicator_true;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c7. ResourceCoordinationTransferContainer */
  if (0) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c7. RRCRconfigurationCompleteIndicator */
  if (1) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_RRCRconfigurationCompleteIndicator;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_RRCRconfigurationCompleteIndicator;
    ie->value.choice.RRCRconfigurationCompleteIndicator = F1AP_RRCRconfigurationCompleteIndicator_true;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c8. RRCContainer */
  if (1) {
    ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_RRCContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_RRCContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  /* c9. SCell_ToBeSetupMod_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeSetupMod_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SCell_ToBeSetupMod_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_SCell_ToBeSetupMod_ItemIEs_t *scell_toBeSetupMod_item_ies;
     scell_toBeSetupMod_item_ies = (F1AP_SCell_ToBeSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_ToBeSetupMod_ItemIEs_t));
     //memset((void *)&scell_toBeSetupMod_item_ies, 0, sizeof(F1AP_SCell_ToBeSetupMod_ItemIEs_t));
     scell_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeSetupMod_Item;
     scell_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
     scell_toBeSetupMod_item_ies->value.present = F1AP_SCell_ToBeSetupMod_ItemIEs__value_PR_SCell_ToBeSetupMod_Item;

     /* 8.1 SCell_ToBeSetup_Item */
     F1AP_SCell_ToBeSetupMod_Item_t scell_toBeSetupMod_item;
     memset((void *)&scell_toBeSetupMod_item, 0, sizeof(F1AP_SCell_ToBeSetupMod_Item_t));

  //   /* - sCell_ID */
     F1AP_NRCGI_t nRCGI;
     MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                        &nRCGI.pLMN_Identity);
     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
     scell_toBeSetupMod_item.sCell_ID = nRCGI;

     /* sCellIndex */
     scell_toBeSetupMod_item.sCellIndex = 6;  // issue here

     //   /* ADD */
     scell_toBeSetupMod_item_ies->value.choice.SCell_ToBeSetupMod_Item = scell_toBeSetupMod_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.SCell_ToBeSetupMod_List.list,
                      scell_toBeSetupMod_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c10. SCell_ToBeRemoved_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeRemoved_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SCell_ToBeRemoved_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_SCell_ToBeRemoved_ItemIEs_t *scell_toBeRemoved_item_ies;
     scell_toBeRemoved_item_ies = (F1AP_SCell_ToBeRemoved_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_ToBeRemoved_ItemIEs_t));
     //memset((void *)&scell_toBeRemoved_item_ies, 0, sizeof(F1AP_SCell_ToBeRemoved_ItemIEs_t));
     scell_toBeRemoved_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeRemoved_Item;
     scell_toBeRemoved_item_ies->criticality   = F1AP_Criticality_ignore;
     scell_toBeRemoved_item_ies->value.present = F1AP_SCell_ToBeRemoved_ItemIEs__value_PR_SCell_ToBeRemoved_Item;

     /* 10.1 SCell_ToBeRemoved_Item */
     F1AP_SCell_ToBeRemoved_Item_t scell_toBeRemoved_item;
     memset((void *)&scell_toBeRemoved_item, 0, sizeof(F1AP_SCell_ToBeRemoved_Item_t));

     /* - sCell_ID */
     F1AP_NRCGI_t nRCGI;
     MCC_MNC_TO_PLMNID(f1ap_info.mcc, f1ap_info.mnc, f1ap_info.mnc_digit_length,
                                        &nRCGI.pLMN_Identity);
     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);
     scell_toBeRemoved_item.sCell_ID = nRCGI;

     /* ADD */
     scell_toBeRemoved_item_ies->value.choice.SCell_ToBeRemoved_Item = scell_toBeRemoved_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.SCell_ToBeRemoved_List.list,
                      scell_toBeRemoved_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c11. SRBs_ToBeSetupMod_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetupMod_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SRBs_ToBeSetupMod_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_SRBs_ToBeSetupMod_ItemIEs_t *srbs_toBeSetupMod_item_ies;
    srbs_toBeSetupMod_item_ies = (F1AP_SRBs_ToBeSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_SRBs_ToBeSetupMod_ItemIEs_t));
    //memset((void *)&srbs_toBeSetupMod_item_ies, 0, sizeof(F1AP_SRBs_ToBeSetupMod_ItemIEs_t));
    srbs_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetupMod_Item;
    srbs_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_toBeSetupMod_item_ies->value.present = F1AP_SRBs_ToBeSetupMod_ItemIEs__value_PR_SRBs_ToBeSetupMod_Item;

    /* 9.1 SRBs_ToBeSetupMod_Item */
    F1AP_SRBs_ToBeSetupMod_Item_t srbs_toBeSetupMod_item;
    memset((void *)&srbs_toBeSetupMod_item, 0, sizeof(F1AP_SRBs_ToBeSetupMod_Item_t));

    /* - sRBID */
    srbs_toBeSetupMod_item.sRBID = 3L;

    /* ADD */
    srbs_toBeSetupMod_item_ies->value.choice.SRBs_ToBeSetupMod_Item = srbs_toBeSetupMod_item;

    ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_ToBeSetupMod_List.list,
                     srbs_toBeSetupMod_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c12. DRBs_ToBeSetupMod_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetupMod_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeSetupMod_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_ToBeSetupMod_ItemIEs_t *drbs_toBeSetupMod_item_ies;
    drbs_toBeSetupMod_item_ies = (F1AP_DRBs_ToBeSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_ToBeSetupMod_ItemIEs_t));
    drbs_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetupMod_Item;
    drbs_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeSetupMod_item_ies->value.present = F1AP_DRBs_ToBeSetupMod_ItemIEs__value_PR_DRBs_ToBeSetupMod_Item;

    /* 12.1 DRBs_ToBeSetupMod_Item */
    F1AP_DRBs_ToBeSetupMod_Item_t drbs_toBeSetupMod_item;
    memset((void *)&drbs_toBeSetupMod_item, 0, sizeof(F1AP_DRBs_ToBeSetupMod_Item_t));

    /* dRBID */
    drbs_toBeSetupMod_item.dRBID = 30L;

    /* qoSInformation */
    drbs_toBeSetupMod_item.qoSInformation.present = F1AP_QoSInformation_PR_eUTRANQoS;
    drbs_toBeSetupMod_item.qoSInformation.choice.eUTRANQoS = (F1AP_EUTRANQoS_t *)calloc(1, sizeof(F1AP_EUTRANQoS_t));
    drbs_toBeSetupMod_item.qoSInformation.choice.eUTRANQoS->qCI = 253L;

    /* ULTunnels_ToBeSetupMod_List */
    int j = 0;
    int maxnoofULTunnels = 1; // 2;
    for (j=0;
            j<maxnoofULTunnels;
            j++) {
            /*  ULTunnels_ToBeSetup_Item */

            F1AP_ULUPTNLInformation_ToBeSetup_Item_t *uLUPTNLInformation_ToBeSetup_Item;
            uLUPTNLInformation_ToBeSetup_Item = (F1AP_ULUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_ULUPTNLInformation_ToBeSetup_Item_t));
            uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
            F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

            TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

            OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "4567",
                             strlen("4567"));

            uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel = gTPTunnel;

            ASN_SEQUENCE_ADD(&drbs_toBeSetupMod_item.uLUPTNLInformation_ToBeSetup_List.list, uLUPTNLInformation_ToBeSetup_Item);
    }

    /* rLCMode */
    drbs_toBeSetupMod_item.rLCMode = F1AP_RLCMode_rlc_um; // enum

    /* OPTIONAL */
    /* ULConfiguration */
    if (0) {
       drbs_toBeSetupMod_item.uLConfiguration = (F1AP_ULConfiguration_t *)calloc(1, sizeof(F1AP_ULConfiguration_t));
    }

    /* ADD */
    drbs_toBeSetupMod_item_ies->value.choice.DRBs_ToBeSetupMod_Item = drbs_toBeSetupMod_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_ToBeSetupMod_List.list,
                   drbs_toBeSetupMod_item_ies);
    
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c13. DRBs_ToBeModified_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeModified_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeModified_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_ToBeModified_ItemIEs_t *drbs_toBeModified_item_ies;
    drbs_toBeModified_item_ies = (F1AP_DRBs_ToBeModified_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_ToBeModified_ItemIEs_t));
    drbs_toBeModified_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeModified_Item;
    drbs_toBeModified_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeModified_item_ies->value.present = F1AP_DRBs_ToBeModified_ItemIEs__value_PR_DRBs_ToBeModified_Item;

    /* 13.1 SRBs_ToBeModified_Item */
    F1AP_DRBs_ToBeModified_Item_t drbs_toBeModified_item;
    memset((void *)&drbs_toBeModified_item, 0, sizeof(F1AP_DRBs_ToBeModified_Item_t));

    /* dRBID */
    drbs_toBeModified_item.dRBID = 30L;

    /* qoSInformation */
    drbs_toBeModified_item.qoSInformation.present = F1AP_QoSInformation_PR_eUTRANQoS;
    drbs_toBeModified_item.qoSInformation.choice.eUTRANQoS = (F1AP_EUTRANQoS_t *)calloc(1, sizeof(F1AP_EUTRANQoS_t));
    drbs_toBeModified_item.qoSInformation.choice.eUTRANQoS->qCI = 254L;

    /* ULTunnels_ToBeModified_List */
    int j = 0;
    int maxnoofULTunnels = 1; // 2;
    for (j=0;
            j<maxnoofULTunnels;
            j++) {
            /*  ULTunnels_ToBeModified_Item */
            F1AP_ULUPTNLInformation_ToBeSetup_Item_t *uLUPTNLInformation_ToBeSetup_Item;
            uLUPTNLInformation_ToBeSetup_Item = (F1AP_ULUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_ULUPTNLInformation_ToBeSetup_Item_t));
            uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
            F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

            TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

            OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                                 strlen("1204"));

            uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel = gTPTunnel;

            ASN_SEQUENCE_ADD(&drbs_toBeModified_item.uLUPTNLInformation_ToBeSetup_List.list, uLUPTNLInformation_ToBeSetup_Item);
    }

    /* OPTIONAL */
    /* ULConfiguration */
    if (0) {
       drbs_toBeModified_item.uLConfiguration = (F1AP_ULConfiguration_t *)calloc(1, sizeof(F1AP_ULConfiguration_t));
    }

    /* ADD */
    drbs_toBeModified_item_ies->value.choice.DRBs_ToBeModified_Item = drbs_toBeModified_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_ToBeModified_List.list,
                   drbs_toBeModified_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c14. SRBs_ToBeReleased_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeReleased_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SRBs_ToBeReleased_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_SRBs_ToBeReleased_ItemIEs_t *srbs_toBeReleased_item_ies;
    srbs_toBeReleased_item_ies = (F1AP_SRBs_ToBeReleased_ItemIEs_t *)calloc(1, sizeof(F1AP_SRBs_ToBeReleased_ItemIEs_t));
    //memset((void *)&srbs_toBeReleased_item_ies, 0, sizeof(F1AP_SRBs_ToBeReleased_ItemIEs_t));
    srbs_toBeReleased_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeReleased_Item;
    srbs_toBeReleased_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_toBeReleased_item_ies->value.present = F1AP_SRBs_ToBeReleased_ItemIEs__value_PR_SRBs_ToBeReleased_Item;

    /* 9.1 SRBs_ToBeReleased_Item */
    F1AP_SRBs_ToBeReleased_Item_t srbs_toBeReleased_item;
    memset((void *)&srbs_toBeReleased_item, 0, sizeof(F1AP_SRBs_ToBeReleased_Item_t));

    /* - sRBID */
    srbs_toBeReleased_item.sRBID = 2L;

    /* ADD */
    srbs_toBeReleased_item_ies->value.choice.SRBs_ToBeReleased_Item = srbs_toBeReleased_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_ToBeReleased_List.list,
                    srbs_toBeReleased_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c15. DRBs_ToBeReleased_List */
  ie = (F1AP_UEContextModificationRequestIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationRequestIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeReleased_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeReleased_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_ToBeReleased_ItemIEs_t *drbs_toBeReleased_item_ies;
    drbs_toBeReleased_item_ies = (F1AP_DRBs_ToBeReleased_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_ToBeReleased_ItemIEs_t));
    drbs_toBeReleased_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeReleased_Item;
    drbs_toBeReleased_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_toBeReleased_item_ies->value.present = F1AP_DRBs_ToBeReleased_ItemIEs__value_PR_DRBs_ToBeReleased_Item;

    /* 14.1 SRBs_ToBeReleased_Item */
    F1AP_DRBs_ToBeReleased_Item_t drbs_toBeReleased_item;
    memset((void *)&drbs_toBeReleased_item, 0, sizeof(F1AP_DRBs_ToBeReleased_Item_t));

    /* dRBID */
    drbs_toBeReleased_item.dRBID = 30L;

    /* ADD */
    drbs_toBeReleased_item_ies->value.choice.DRBs_ToBeReleased_Item = drbs_toBeReleased_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_ToBeReleased_List.list,
                    drbs_toBeReleased_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
    return;
  }

  printf("\n");

  /* decode */
  if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
    printf("Failed to decode F1 setup request\n");
  }

}

void CU_handle_UE_CONTEXT_MODIFICATION_RESPONSE(F1AP_UEContextModificationResponse_t *UEContextModificationResponse) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_handle_UE_CONTEXT_MODIFICATION_FAILURE(F1AP_UEContextModificationFailure_t EContextModificationFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_handle_UE_CONTEXT_MODIFICATION_REQUIRED(F1AP_UEContextModificationRequired_t *UEContextModificationRequired) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void CU_send_UE_CONTEXT_MODIFICATION_CONFIRM(F1AP_UEContextModificationConfirm_t UEContextModificationConfirm_t) {
  AssertFatal(1==0,"Not implemented yet\n");
}


static int F1AP_CU_decode_initiating_message(F1AP_InitiatingMessage_t *initiating_p) {

  switch (initiating_p->value.present) {
    
  case F1AP_InitiatingMessage__value_PR_NOTHING: /* No components present */
    AssertFatal(1==0,"Should not receive NOTHING on CU\n");
    break;
    
  case F1AP_InitiatingMessage__value_PR_Reset:
    CU_send_RESET(&initiating_p->value.choice.Reset);
    break;
  case F1AP_InitiatingMessage__value_PR_F1SetupRequest:
    CU_handle_F1_SETUP_REQUEST(&initiating_p->value.choice.F1SetupRequest);
    break;
  case F1AP_InitiatingMessage__value_PR_GNBDUConfigurationUpdate:
    AssertFatal(1==0,"Should not receive GNBDUConfigurationUpdate on CU\n");
    break;
  case F1AP_InitiatingMessage__value_PR_GNBCUConfigurationUpdate:
    CU_handle_gNB_CU_CONFIGURATION_UPDATE(&initiating_p->value.choice.GNBCUConfigurationUpdate);
    break;
  case F1AP_InitiatingMessage__value_PR_UEContextSetupRequest:
    CU_handle_UE_CONTEXT_SETUP_REQUEST(&initiating_p->value.choice.UEContextSetupRequest);
    break;
  case F1AP_InitiatingMessage__value_PR_UEContextReleaseCommand:
    CU_handle_UE_CONTEXT_SETUP_RELEASE_COMMAND(&initiating_p->value.choice.UEContextReleaseCommand);
    break;
  case F1AP_InitiatingMessage__value_PR_UEContextModificationRequest:
    CU_handle_UE_CONTEXT_MODIFICATION_REQUEST(&initiating_p->value.choice.UEContextModificationRequest);
    break;
  case F1AP_InitiatingMessage__value_PR_UEContextModificationRequired:
    AssertFatal(1==0,"Should not receive UECONTEXTMODIFICATIONREQUIRED on CU\n");
    break;
  case F1AP_InitiatingMessage__value_PR_ErrorIndication:
    AssertFatal(1==0,"Should not receive ErrorIndication on CU\n");
    break;
  case F1AP_InitiatingMessage__value_PR_UEContextReleaseRequest:
    AssertFatal(1==0,"Should not receive UECONTEXTRELEASEREQUEST on CU\n");    
    break;
  case F1AP_InitiatingMessage__value_PR_DLRRCMessageTransfer:
    CU_handle_DL_RRC_MESSAGE_TRANSFER(&initiating_p->value.choice.DLRRCMessageTransfer);
    break;
  case F1AP_InitiatingMessage__value_PR_ULRRCMessageTransfer:
    AssertFatal(1==0,"Should not receive ULRRCMESSAGETRANSFER on CU\n");
    break;
  case F1AP_InitiatingMessage__value_PR_PrivateMessage:
    AssertFatal(1==0,"Should not receive PRIVATEMESSAGE on CU\n");
    break;
  default:
    AssertFatal(1==0,"Shouldn't get here\n");
  }

}

static int F1AP_CU_decode_successful_outcome(F1AP_SuccessfulOutcome_t *successfulOutcome_p)
{

    switch (successfulOutcome_p->value.present) {
      
    case F1AP_SuccessfulOutcome__value_PR_NOTHING:    /* No components present */
      AssertFatal(1==0,"Should not received NOTHING!\n");
      break;
    case F1AP_SuccessfulOutcome__value_PR_ResetAcknowledge:
      AssertFatal(1==0,"CU Should not receive ResetAcknowled!\n");
      break;
    case F1AP_SuccessfulOutcome__value_PR_F1SetupResponse:
      AssertFatal(1==0,"CU Should not receive F1SetupResponse!\n");      
      break;
    case F1AP_SuccessfulOutcome__value_PR_GNBDUConfigurationUpdateAcknowledge:
      CU_handle_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(successfulOutcome_p->value.choice.GNBDUConfigurationUpdateAcknowledge);
      break;
    case F1AP_SuccessfulOutcome__value_PR_GNBCUConfigurationUpdateAcknowledge:
      AssertFatal(1==0,"CU Should not receive GNBCUConfigurationUpdateAcknowledge!\n");      
      break;
    case F1AP_SuccessfulOutcome__value_PR_UEContextSetupResponse:
      AssertFatal(1==0,"CU Should not receive UEContextSetupResponse!\n");      
      break;
    case F1AP_SuccessfulOutcome__value_PR_UEContextReleaseComplete:
      AssertFatal(1==0,"CU Should not receive UEContextReleaseComplete!\n");      
      break;
    case F1AP_SuccessfulOutcome__value_PR_UEContextModificationResponse:
      AssertFatal(1==0,"CU Should not receive UEContextModificationResponse!\n");   
      break;
    case F1AP_SuccessfulOutcome__value_PR_UEContextModificationConfirm:
      CU_send_UE_CONTEXT_MODIFICATION_CONFIRM(successfulOutcome_p->value.choice.UEContextModificationConfirm);
      break;  
    }
}

static int F1AP_CU_decode_unsuccessful_outcome(F1AP_UnsuccessfulOutcome_t *unSuccessfulOutcome_p)
{

  switch (unSuccessfulOutcome_p->value.present) {
    
  case F1AP_UnsuccessfulOutcome__value_PR_NOTHING:
    AssertFatal(1==0,"Should not receive NOTHING!\n");
    break;  /* No components present */
  case F1AP_UnsuccessfulOutcome__value_PR_F1SetupFailure:
    AssertFatal(1==0,"Should not receive F1SetupFailure\n");
    break;
  case F1AP_UnsuccessfulOutcome__value_PR_GNBDUConfigurationUpdateFailure:
    CU_handle_gNB_CU_CONFIGURATION_FAILURE(unSuccessfulOutcome_p->value.choice.GNBDUConfigurationUpdateFailure);
    break;
  case F1AP_UnsuccessfulOutcome__value_PR_GNBCUConfigurationUpdateFailure:
    AssertFatal(1==0,"Should not receive GNBCUConfigurationUpdateFailure\n");
    break;
  case F1AP_UnsuccessfulOutcome__value_PR_UEContextSetupFailure:
    CU_handle_UE_CONTEXT_SETUP_FAILURE(unSuccessfulOutcome_p->value.choice.UEContextSetupFailure);
    break;
  case F1AP_UnsuccessfulOutcome__value_PR_UEContextModificationFailure:
    CU_handle_UE_CONTEXT_MODIFICATION_FAILURE(unSuccessfulOutcome_p->value.choice.UEContextModificationFailure);
    break;
  }

}
