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

/*! \file f1ap_du_interface_management.h
 * \brief f1ap interface management for DU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"
#include "f1ap_du_ue_context_management.h"

extern f1ap_setup_req_t *f1ap_du_data;

/*
    UE Context Setup
*/

void DU_handle_UE_CONTEXT_SETUP_REQUEST(F1AP_UEContextSetupRequest_t *UEContextSetupRequest) {
  AssertFatal(1==0,"Not implemented yet\n");
}

//void DU_send_UE_CONTEXT_SETUP_RESPONSE(F1AP_UEContextSetupResponse_t *UEContextSetupResponse) {
void DU_send_UE_CONTEXT_SETUP_RESPONSE(void) {
  F1AP_F1AP_PDU_t                  pdu;
  F1AP_UEContextSetupResponse_t    *out;
  F1AP_UEContextSetupResponseIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (F1AP_SuccessfulOutcome_t *)calloc(1, sizeof(F1AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome->procedureCode = F1AP_ProcedureCode_id_UEContextSetup;
  pdu.choice.successfulOutcome->criticality   = F1AP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = F1AP_SuccessfulOutcome__value_PR_UEContextSetupResponse;
  out = &pdu.choice.successfulOutcome->value.choice.UEContextSetupResponse;

  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = 651L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c3. DUtoCURRCInformation */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DUtoCURRCInformation;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_DUtoCURRCInformation;

  OCTET_STRING_fromBuf(&ie->value.choice.DUtoCURRCInformation.cellGroupConfig, "asdsa",
                       strlen("asdsa"));
  /* OPTIONAL */
  if (0) {
    ie->value.choice.DUtoCURRCInformation.measGapConfig = (F1AP_MeasGapConfig_t *)calloc(1, sizeof(F1AP_MeasGapConfig_t));
    OCTET_STRING_fromBuf( ie->value.choice.DUtoCURRCInformation.measGapConfig, "asdsa",
                         strlen("asdsa"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c4. ResourceCoordinationTransferContainer */
  if (0) {
    ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.ResourceCoordinationTransferContainer, "asdsa",
                         strlen("asdsa"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  // /*  */
  /* c5. DRBs_Setup_List */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_Setup_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_DRBs_Setup_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_DRBs_Setup_ItemIEs_t *drbs_setup_item_ies;
     drbs_setup_item_ies = (F1AP_DRBs_Setup_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_Setup_ItemIEs_t));
     drbs_setup_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_Setup_Item;
     drbs_setup_item_ies->criticality   = F1AP_Criticality_ignore;
     drbs_setup_item_ies->value.present = F1AP_SRBs_FailedToBeSetup_ItemIEs__value_PR_SRBs_FailedToBeSetup_Item;

     /* 5.1 DRBs_Setup_Item */
     F1AP_DRBs_Setup_Item_t drbs_setup_item;
     memset((void *)&drbs_setup_item, 0, sizeof(F1AP_DRBs_Setup_Item_t));

     drbs_setup_item.dRBID = 12;
     
     int j;
     for (j=0;
       j<1;
       j++) {

       F1AP_DLUPTNLInformation_ToBeSetup_Item_t *dLUPTNLInformation_ToBeSetup_Item;
       dLUPTNLInformation_ToBeSetup_Item = (F1AP_DLUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_DLUPTNLInformation_ToBeSetup_Item_t));
       dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
       
       F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

       // F1AP_TransportLayerAddress_t transportLayerAddress;
       // transportLayerAddress.buf = malloc((36+7)/8);
       // transportLayerAddress.size = (36+7)/8;
       // transportLayerAddress.bits_unused = 4;
       // *transportLayerAddress.buf = 123;
       // dLUPTNLInformation_ToBeSetup_Item.dL_GTP_Tunnel_EndPoint.transportLayerAddress = transportLayerAddress;

       TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

       OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                             strlen("1204"));

       dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.choice.gTPTunnel = gTPTunnel;

       ASN_SEQUENCE_ADD(&drbs_setup_item.dLUPTNLInformation_ToBeSetup_List.list,
                        dLUPTNLInformation_ToBeSetup_Item);
     }

     //   /* ADD */
     drbs_setup_item_ies->value.choice.DRBs_Setup_Item = drbs_setup_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_Setup_List.list,
                     drbs_setup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // /*  */
  /* c6. SRBs_FailedToBeSetup_List */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetup_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_SRBs_FailedToBeSetup_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_SRBs_FailedToBeSetup_ItemIEs_t *srbs_failedToBeSetup_item_ies;
     srbs_failedToBeSetup_item_ies = (F1AP_SRBs_FailedToBeSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_SRBs_FailedToBeSetup_ItemIEs_t));
     srbs_failedToBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetup_Item;
     srbs_failedToBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
     srbs_failedToBeSetup_item_ies->value.present = F1AP_SRBs_FailedToBeSetup_ItemIEs__value_PR_SRBs_FailedToBeSetup_Item;

     /* 6.1 SRBs_Setup_Item */
     F1AP_SRBs_FailedToBeSetup_Item_t srbs_failedToBeSetup_item;
     memset((void *)&srbs_failedToBeSetup_item, 0, sizeof(F1AP_SRBs_FailedToBeSetup_Item_t));

     srbs_failedToBeSetup_item.sRBID = 13;
     srbs_failedToBeSetup_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
     srbs_failedToBeSetup_item.cause->present = F1AP_Cause_PR_radioNetwork;
     srbs_failedToBeSetup_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_cu_ue_f1ap_id;

     //   /* ADD */
     srbs_failedToBeSetup_item_ies->value.choice.SRBs_FailedToBeSetup_Item = srbs_failedToBeSetup_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_FailedToBeSetup_List.list,
                     srbs_failedToBeSetup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // /*  */
  /* c7. DRBs_FailedToBeSetup_List */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetup_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_DRBs_FailedToBeSetup_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_DRBs_FailedToBeSetup_ItemIEs_t *drbs_failedToBeSetup_item_ies;
     drbs_failedToBeSetup_item_ies = (F1AP_DRBs_FailedToBeSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_FailedToBeSetup_ItemIEs_t));
     drbs_failedToBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetup_Item;
     drbs_failedToBeSetup_item_ies->criticality   = F1AP_Criticality_ignore;
     drbs_failedToBeSetup_item_ies->value.present = F1AP_DRBs_FailedToBeSetup_ItemIEs__value_PR_DRBs_FailedToBeSetup_Item;

     /* 7.1 DRBs_Setup_Item */
     F1AP_DRBs_FailedToBeSetup_Item_t drbs_failedToBeSetup_item;
     memset((void *)&drbs_failedToBeSetup_item, 0, sizeof(F1AP_DRBs_FailedToBeSetup_Item_t));

     drbs_failedToBeSetup_item.dRBID = 14;
     drbs_failedToBeSetup_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
     drbs_failedToBeSetup_item.cause->present = F1AP_Cause_PR_radioNetwork;
     drbs_failedToBeSetup_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_cu_ue_f1ap_id;

     //   /* ADD */
     drbs_failedToBeSetup_item_ies->value.choice.DRBs_FailedToBeSetup_Item = drbs_failedToBeSetup_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_FailedToBeSetup_List.list,
                     drbs_failedToBeSetup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // /*  */
  /* c8. SCell_FailedtoSetup_List */
  ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetup_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_SCell_FailedtoSetup_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_SCell_FailedtoSetup_ItemIEs_t *sCell_FailedtoSetup_item_ies;
     sCell_FailedtoSetup_item_ies = (F1AP_SCell_FailedtoSetup_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_FailedtoSetup_ItemIEs_t));
     sCell_FailedtoSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetup_Item;
     sCell_FailedtoSetup_item_ies->criticality   = F1AP_Criticality_ignore;
     sCell_FailedtoSetup_item_ies->value.present = F1AP_SCell_FailedtoSetup_ItemIEs__value_PR_SCell_FailedtoSetup_Item;

     /* 8.1 DRBs_Setup_Item */
     F1AP_SCell_FailedtoSetup_Item_t sCell_FailedtoSetup_item;
     memset((void *)&sCell_FailedtoSetup_item, 0, sizeof(F1AP_SCell_FailedtoSetup_Item_t));

     /* - nRCGI */
     F1AP_NRCGI_t nRCGI;  // issue here
     MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i], &nRCGI.pLMN_Identity);
//
     // INT32_TO_BIT_STRING(123, &nRCGI.nRCellIdentity);
     // nRCGI.nRCellIdentity.buf = malloc((36+7)/8);

     // nRCGI.nRCellIdentity.size = (36+7)/8;
     // nRCGI.nRCellIdentity.bits_unused = 4;

     // nRCGI.nRCellIdentity.buf[0] = 123;

     //nRCGI.nRCellIdentity = 15;

     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);

     sCell_FailedtoSetup_item.sCell_ID = nRCGI;

     sCell_FailedtoSetup_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
     sCell_FailedtoSetup_item.cause->present = F1AP_Cause_PR_radioNetwork;
     sCell_FailedtoSetup_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_cu_ue_f1ap_id;

     //   /* ADD */
     sCell_FailedtoSetup_item_ies->value.choice.SCell_FailedtoSetup_Item = sCell_FailedtoSetup_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.SCell_FailedtoSetup_List.list,
                      sCell_FailedtoSetup_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
 
  // /*  */
  /* c9. CriticalityDiagnostics */
  if (0) {
    ie = (F1AP_UEContextSetupResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextSetupResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextSetupResponseIEs__value_PR_CriticalityDiagnostics;
    ie->value.choice.CriticalityDiagnostics.procedureCode = (F1AP_ProcedureCode_t *)calloc(1, sizeof(F1AP_ProcedureCode_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCode = F1AP_ProcedureCode_id_UEContextSetup;
    ie->value.choice.CriticalityDiagnostics.triggeringMessage = (F1AP_TriggeringMessage_t *)calloc(1, sizeof(F1AP_TriggeringMessage_t));
    *ie->value.choice.CriticalityDiagnostics.triggeringMessage = F1AP_TriggeringMessage_initiating_message;
    ie->value.choice.CriticalityDiagnostics.procedureCriticality = (F1AP_Criticality_t *)calloc(1, sizeof(F1AP_Criticality_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCriticality = F1AP_Criticality_reject;
    ie->value.choice.CriticalityDiagnostics.transactionID = (F1AP_TransactionID_t *)calloc(1, sizeof(F1AP_TransactionID_t));
    *ie->value.choice.CriticalityDiagnostics.transactionID = 0;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }


  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
    return;
  }

  printf("\n");

  /* decode */
  // if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
  //   printf("Failed to decode F1 setup request\n");
  // }
  //du_f1ap_itti_send_sctp_data_req(instance, f1ap_setup_req->assoc_id, buffer, len, 0);
}

void DU_send_UE_CONTEXT_SETUP_FAILURE(F1AP_UEContextSetupFailure_t UEContextSetupFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

/*
    UE Context Release Request (gNB-DU initiated).
*/

void DU_send_UE_CONTEXT_RELEASE_REQUEST(F1AP_UEContextReleaseRequest_t *UEContextReleaseRequest) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void DU_handle_UE_CONTEXT_RELEASE_COMMAND(F1AP_UEContextReleaseCommand_t *UEContextReleaseCommand) {
  AssertFatal(1==0,"Not implemented yet\n");
}


void DU_send_UE_CONTEXT_RELEASE_COMPLETE(F1AP_UEContextReleaseComplete_t *UEContextReleaseComplete) {
  AssertFatal(1==0,"Not implemented yet\n");
}


/*
    UE Context Modification (gNB-CU initiated)
*/

void DU_handle_UE_CONTEXT_MODIFICATION_REQUEST(F1AP_UEContextModificationRequest_t *UEContextModificationRequest) {
  AssertFatal(1==0,"Not implemented yet\n");
}

//void DU_send_UE_CONTEXT_MODIFICATION_RESPONSE(F1AP_UEContextModificationResponse_t *UEContextModificationResponse) {
void DU_send_UE_CONTEXT_MODIFICATION_RESPONSE(void) {
  F1AP_F1AP_PDU_t                        pdu;
  F1AP_UEContextModificationResponse_t    *out;
  F1AP_UEContextModificationResponseIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  /* Create */
  /* 0. Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (F1AP_SuccessfulOutcome_t *)calloc(1, sizeof(F1AP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome->procedureCode = F1AP_ProcedureCode_id_UEContextModification;
  pdu.choice.successfulOutcome->criticality   = F1AP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = F1AP_SuccessfulOutcome__value_PR_UEContextModificationResponse;
  out = &pdu.choice.successfulOutcome->value.choice.UEContextModificationResponse;
  
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie->value.choice.GNB_CU_UE_F1AP_ID = 126L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie->value.choice.GNB_DU_UE_F1AP_ID = 651L;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  /* c3. ResourceCoordinationTransferContainer */
  if (0) {
    ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  /* c4. DUtoCURRCInformation */
  if (0) {
    ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_DUtoCURRCInformation;
    ie->criticality                    = F1AP_Criticality_reject;
    ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DUtoCURRCInformation;

    OCTET_STRING_fromBuf(&ie->value.choice.DUtoCURRCInformation.cellGroupConfig, "asdsa1d32sa1d31asd31as",
                       strlen("asdsa1d32sa1d31asd31as"));
    /* OPTIONAL */
    if (1) {
      ie->value.choice.DUtoCURRCInformation.measGapConfig = (F1AP_MeasGapConfig_t *)calloc(1, sizeof(F1AP_MeasGapConfig_t));
      OCTET_STRING_fromBuf( ie->value.choice.DUtoCURRCInformation.measGapConfig, "asdsa1d32sa1d31asd31as",
                           strlen("asdsa1d32sa1d31asd31as"));
      ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }
  }


  /* mandatory */
  /* c5. DRBs_SetupMod_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_SetupMod_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_SetupMod_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_SetupMod_ItemIEs_t *drbs_setupMod_item_ies;
    drbs_setupMod_item_ies = (F1AP_DRBs_SetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_SetupMod_ItemIEs_t));
    drbs_setupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_SetupMod_Item;
    drbs_setupMod_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_setupMod_item_ies->value.present = F1AP_DRBs_SetupMod_ItemIEs__value_PR_DRBs_SetupMod_Item;

    /* 10.1 DRBs_SetupMod_Item */
    F1AP_DRBs_SetupMod_Item_t drbs_setupMod_item;
    memset((void *)&drbs_setupMod_item, 0, sizeof(F1AP_DRBs_SetupMod_Item_t));

    /* dRBID */
    drbs_setupMod_item.dRBID = 30L;

    /* DLTunnels_SetupMod_List */
    int j = 0;
    int maxnoofDLUPTNLInformation = 1; // 2;
    for (j=0;
        j<maxnoofDLUPTNLInformation;
        j++) {
        /*  DLTunnels_ToBeSetup_Item */
        F1AP_DLUPTNLInformation_ToBeSetup_Item_t *dLUPTNLInformation_ToBeSetup_Item;
        dLUPTNLInformation_ToBeSetup_Item = (F1AP_DLUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_DLUPTNLInformation_ToBeSetup_Item_t));
        dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
        F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

        TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

        OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                             strlen("1204"));

        dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.choice.gTPTunnel = gTPTunnel;

        ASN_SEQUENCE_ADD(&drbs_setupMod_item.dLUPTNLInformation_ToBeSetup_List.list, dLUPTNLInformation_ToBeSetup_Item);
    }

    /* ADD */
    drbs_setupMod_item_ies->value.choice.DRBs_SetupMod_Item = drbs_setupMod_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_SetupMod_List.list,
                   drbs_setupMod_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c6. DRBs_Modified_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_Modified_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_Modified_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_Modified_ItemIEs_t *drbs_modified_item_ies;
    drbs_modified_item_ies = (F1AP_DRBs_Modified_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_Modified_ItemIEs_t));
    drbs_modified_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_Modified_Item;
    drbs_modified_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_modified_item_ies->value.present = F1AP_DRBs_Modified_ItemIEs__value_PR_DRBs_Modified_Item;

    /* 13.1 SRBs_modified_Item */
    F1AP_DRBs_Modified_Item_t drbs_modified_item;
    memset((void *)&drbs_modified_item, 0, sizeof(F1AP_DRBs_Modified_Item_t));

    /* dRBID */
    drbs_modified_item.dRBID = 25L;

    /* ULTunnels_Modified_List */
    int maxnoofULTunnels = 1; // 2;
    int j = 0;
    for (j=0;
         j<maxnoofULTunnels;
         j++) {
      /*  DLTunnels_Modified_Item */
        F1AP_DLUPTNLInformation_ToBeSetup_Item_t *dLUPTNLInformation_ToBeSetup_Item;
        dLUPTNLInformation_ToBeSetup_Item = (F1AP_DLUPTNLInformation_ToBeSetup_Item_t *)calloc(1, sizeof(F1AP_DLUPTNLInformation_ToBeSetup_Item_t));
        dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
        F1AP_GTPTunnel_t *gTPTunnel = (F1AP_GTPTunnel_t *)calloc(1, sizeof(F1AP_GTPTunnel_t));

        TRANSPORT_LAYER_ADDRESS_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);

        OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                              strlen("1204"));

        dLUPTNLInformation_ToBeSetup_Item->dLUPTNLInformation.choice.gTPTunnel = gTPTunnel;

        ASN_SEQUENCE_ADD(&drbs_modified_item.dLUPTNLInformation_ToBeSetup_List.list, dLUPTNLInformation_ToBeSetup_Item);
    }

    /* ADD */
    drbs_modified_item_ies->value.choice.DRBs_Modified_Item = drbs_modified_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_Modified_List.list,
                    drbs_modified_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c7. SRBs_FailedToBeSetupMod_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetupMod_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_SRBs_FailedToBeSetupMod_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_SRBs_FailedToBeSetupMod_ItemIEs_t *srbs_failedToBeSetupMod_item_ies;
    srbs_failedToBeSetupMod_item_ies = (F1AP_SRBs_FailedToBeSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_SRBs_FailedToBeSetupMod_ItemIEs_t));
    srbs_failedToBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetupMod_Item;
    srbs_failedToBeSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
    srbs_failedToBeSetupMod_item_ies->value.present = F1AP_SRBs_FailedToBeSetupMod_ItemIEs__value_PR_SRBs_FailedToBeSetupMod_Item;

    /* 9.1 SRBs_FailedToBeSetupMod_Item */
    F1AP_SRBs_FailedToBeSetupMod_Item_t srbs_failedToBeSetupMod_item;
    memset((void *)&srbs_failedToBeSetupMod_item, 0, sizeof(F1AP_SRBs_FailedToBeSetupMod_Item_t));

    /* - sRBID */
    srbs_failedToBeSetupMod_item.sRBID = 50L;

    srbs_failedToBeSetupMod_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
    srbs_failedToBeSetupMod_item.cause->present = F1AP_Cause_PR_radioNetwork;
    srbs_failedToBeSetupMod_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnd_du_ue_f1ap_id;

    /* ADD */
    srbs_failedToBeSetupMod_item_ies->value.choice.SRBs_FailedToBeSetupMod_Item = srbs_failedToBeSetupMod_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.SRBs_FailedToBeSetupMod_List.list,
                     srbs_failedToBeSetupMod_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c8. DRBs_FailedToBeSetupMod_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetupMod_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_FailedToBeSetupMod_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_FailedToBeSetupMod_ItemIEs_t *drbs_failedToBeSetupMod_item_ies;
    drbs_failedToBeSetupMod_item_ies = (F1AP_DRBs_FailedToBeSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_FailedToBeSetupMod_ItemIEs_t));
    drbs_failedToBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetupMod_Item;
    drbs_failedToBeSetupMod_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_failedToBeSetupMod_item_ies->value.present = F1AP_DRBs_FailedToBeSetupMod_ItemIEs__value_PR_DRBs_FailedToBeSetupMod_Item;

    /* 10.1 DRBs_ToBeSetupMod_Item */
    F1AP_DRBs_FailedToBeSetupMod_Item_t drbs_failedToBeSetupMod_item;
    memset((void *)&drbs_failedToBeSetupMod_item, 0, sizeof(F1AP_DRBs_FailedToBeSetupMod_Item_t));

    /* dRBID */
    drbs_failedToBeSetupMod_item.dRBID = 30L;

    drbs_failedToBeSetupMod_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
    drbs_failedToBeSetupMod_item.cause->present = F1AP_Cause_PR_radioNetwork;
    drbs_failedToBeSetupMod_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnd_du_ue_f1ap_id;

    /* ADD */
    drbs_failedToBeSetupMod_item_ies->value.choice.DRBs_FailedToBeSetupMod_Item = drbs_failedToBeSetupMod_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_FailedToBeSetupMod_List.list,
                     drbs_failedToBeSetupMod_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* mandatory */
  /* c9. SCell_FailedtoSetupMod_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetupMod_List;
  ie->criticality                    = F1AP_Criticality_ignore;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_SCell_FailedtoSetupMod_List;

  for (i=0;
       i<1;
       i++) {
     //
     F1AP_SCell_FailedtoSetupMod_ItemIEs_t *scell_failedtoSetupMod_item_ies;
     scell_failedtoSetupMod_item_ies = (F1AP_SCell_FailedtoSetupMod_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_FailedtoSetupMod_ItemIEs_t));
     scell_failedtoSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_FailedtoSetupMod_Item;
     scell_failedtoSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
     scell_failedtoSetupMod_item_ies->value.present = F1AP_SCell_FailedtoSetupMod_ItemIEs__value_PR_SCell_FailedtoSetupMod_Item;

     /* 8.1 SCell_ToBeSetup_Item */
     F1AP_SCell_FailedtoSetupMod_Item_t scell_failedtoSetupMod_item;
     memset((void *)&scell_failedtoSetupMod_item, 0, sizeof(F1AP_SCell_FailedtoSetupMod_Item_t));

     /* - sCell_ID */
     F1AP_NRCGI_t nRCGI;
     MCC_MNC_TO_PLMNID(f1ap_du_data->mcc[i], f1ap_du_data->mnc[i], f1ap_du_data->mnc_digit_length[i],
                                        &nRCGI.pLMN_Identity);

     NR_CELL_ID_TO_BIT_STRING(123456, &nRCGI.nRCellIdentity);

     scell_failedtoSetupMod_item.sCell_ID = nRCGI;

     scell_failedtoSetupMod_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
     scell_failedtoSetupMod_item.cause->present = F1AP_Cause_PR_radioNetwork;
     scell_failedtoSetupMod_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnd_du_ue_f1ap_id;

        /* ADD */
     scell_failedtoSetupMod_item_ies->value.choice.SCell_FailedtoSetupMod_Item = scell_failedtoSetupMod_item;
     ASN_SEQUENCE_ADD(&ie->value.choice.SCell_FailedtoSetupMod_List.list,
                      scell_failedtoSetupMod_item_ies);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  /* c10. DRBs_FailedToBeModified_List */
  ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
  ie->id                             = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeModified_List;
  ie->criticality                    = F1AP_Criticality_reject;
  ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_DRBs_FailedToBeModified_List;

  for (i=0;
       i<1;
       i++) {
    //
    F1AP_DRBs_FailedToBeModified_ItemIEs_t *drbs_failedToBeModified_item_ies;
    drbs_failedToBeModified_item_ies = (F1AP_DRBs_FailedToBeModified_ItemIEs_t *)calloc(1, sizeof(F1AP_DRBs_FailedToBeModified_ItemIEs_t));
    drbs_failedToBeModified_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_FailedToBeModified_Item;
    drbs_failedToBeModified_item_ies->criticality   = F1AP_Criticality_reject;
    drbs_failedToBeModified_item_ies->value.present = F1AP_DRBs_FailedToBeModified_ItemIEs__value_PR_DRBs_FailedToBeModified_Item;

    /* 13.1 DRBs_FailedToBeModified_Item */
    F1AP_DRBs_FailedToBeModified_Item_t drbs_failedToBeModified_item;
    memset((void *)&drbs_failedToBeModified_item, 0, sizeof(F1AP_DRBs_FailedToBeModified_Item_t));

    /* dRBID */
    drbs_failedToBeModified_item.dRBID = 30L;

    drbs_failedToBeModified_item.cause = (F1AP_Cause_t *)calloc(1, sizeof(F1AP_Cause_t));
    drbs_failedToBeModified_item.cause->present = F1AP_Cause_PR_radioNetwork;
    drbs_failedToBeModified_item.cause->choice.radioNetwork = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnd_du_ue_f1ap_id;

    /* ADD */
    drbs_failedToBeModified_item_ies->value.choice.DRBs_FailedToBeModified_Item = drbs_failedToBeModified_item;
    ASN_SEQUENCE_ADD(&ie->value.choice.DRBs_FailedToBeModified_List.list,
                     drbs_failedToBeModified_item_ies);

  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // /*  */
  /* c11. CriticalityDiagnostics */
  if (0) {
    ie = (F1AP_UEContextModificationResponseIEs_t *)calloc(1, sizeof(F1AP_UEContextModificationResponseIEs_t));
    ie->id                             = F1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality                    = F1AP_Criticality_ignore;
    ie->value.present                  = F1AP_UEContextModificationResponseIEs__value_PR_CriticalityDiagnostics;
    ie->value.choice.CriticalityDiagnostics.procedureCode = (F1AP_ProcedureCode_t *)calloc(1, sizeof(F1AP_ProcedureCode_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCode = F1AP_ProcedureCode_id_UEContextModification;
    ie->value.choice.CriticalityDiagnostics.triggeringMessage = (F1AP_TriggeringMessage_t *)calloc(1, sizeof(F1AP_TriggeringMessage_t));
    *ie->value.choice.CriticalityDiagnostics.triggeringMessage = F1AP_TriggeringMessage_initiating_message;
    ie->value.choice.CriticalityDiagnostics.procedureCriticality = (F1AP_Criticality_t *)calloc(1, sizeof(F1AP_Criticality_t));
    *ie->value.choice.CriticalityDiagnostics.procedureCriticality = F1AP_Criticality_reject;
    ie->value.choice.CriticalityDiagnostics.transactionID = (F1AP_TransactionID_t *)calloc(1, sizeof(F1AP_TransactionID_t));
    *ie->value.choice.CriticalityDiagnostics.transactionID = 0;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    printf("Failed to encode F1 setup request\n");
    return;
  }

  printf("\n");

  /* decode */
  // if (f1ap_decode_pdu(&pdu, buffer, len) > 0) {
  //   printf("Failed to decode F1 setup request\n");
  // }
  //du_f1ap_itti_send_sctp_data_req(instance, f1ap_setup_req->assoc_id, buffer, len, 0);

}

void DU_send_UE_CONTEXT_MODIFICATION_FAILURE(F1AP_UEContextModificationFailure_t UEContextModificationFailure) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_send_UE_CONTEXT_MODIFICATION_REQUIRED(F1AP_UEContextModificationRequired_t *UEContextModificationRequired) {
  AssertFatal(1==0,"Not implemented yet\n");
}

void DU_handle_UE_CONTEXT_MODIFICATION_CONFIRM(F1AP_UEContextModificationConfirm_t UEContextModificationConfirm_t) {
  AssertFatal(1==0,"Not implemented yet\n");
}