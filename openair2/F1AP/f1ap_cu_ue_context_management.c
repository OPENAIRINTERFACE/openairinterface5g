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

/*! \file f1ap_cu_ue_context_management.c
 * \brief F1AP UE Context Management, CU side
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
#include "f1ap_cu_ue_context_management.h"

//void CU_send_UE_CONTEXT_SETUP_REQUEST(F1AP_UEContextSetupRequest_t *UEContextSetupRequest) {
int CU_send_UE_CONTEXT_SETUP_REQUEST(instance_t instance) {
  F1AP_F1AP_PDU_t                 pdu;
  F1AP_UEContextSetupRequest_t    *out;
  F1AP_UEContextSetupRequestIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  // for test
  int mcc = 208;
  int mnc = 93;
  int mnc_digit_length = 8;

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
  MCC_MNC_TO_PLMNID(mcc, mnc, mnc_digit_length,
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
    MCC_MNC_TO_PLMNID(mcc, mnc, mnc_digit_length,
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
     MCC_MNC_TO_PLMNID(mcc, mnc, mnc_digit_length,
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
    LOG_E(CU_F1AP, "Failed to encode F1 setup request\n");
    return -1;
  }

  return 0;
}

int CU_handle_UE_CONTEXT_SETUP_RESPONSE(instance_t       instance,
                                        uint32_t         assoc_id,
                                        uint32_t         stream,
                                        F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_UE_CONTEXT_SETUP_FAILURE(instance_t       instance,
                                       uint32_t         assoc_id,
                                       uint32_t         stream,
                                       F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}


int CU_handle_UE_CONTEXT_RELEASE_REQUEST(instance_t       instance,
                                         uint32_t         assoc_id,
                                         uint32_t         stream,
                                         F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}


int CU_send_UE_CONTEXT_RELEASE_COMMAND(instance_t instance,
                                       F1AP_UEContextReleaseCommand_t *UEContextReleaseCommand) {
  AssertFatal(1==0,"Not implemented yet\n");
}


int CU_handle_UE_CONTEXT_RELEASE_COMPLETE(instance_t       instance,
                                         uint32_t         assoc_id,
                                         uint32_t         stream,
                                         F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}


//void CU_send_UE_CONTEXT_MODIFICATION_REQUEST(F1AP_UEContextModificationRequest_t *UEContextModificationRequest) {
int CU_send_UE_CONTEXT_MODIFICATION_REQUEST(instance_t instance) {
  F1AP_F1AP_PDU_t                        pdu;
  F1AP_UEContextModificationRequest_t    *out;
  F1AP_UEContextModificationRequestIEs_t *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       i = 0;

  // for test
  int mcc = 208;
  int mnc = 93;
  int mnc_digit_length = 8;

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
    MCC_MNC_TO_PLMNID(mcc, mnc, mnc_digit_length,
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
     MCC_MNC_TO_PLMNID(mcc, mnc, mnc_digit_length,
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
     MCC_MNC_TO_PLMNID(mcc, mnc, mnc_digit_length,
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
    LOG_E(CU_F1AP, "Failed to encode F1 setup request\n");
    return -1;
  }

  return 0;
}

int CU_handle_UE_CONTEXT_MODIFICATION_RESPONSE(instance_t       instance,
                                               uint32_t         assoc_id,
                                               uint32_t         stream,
                                               F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_UE_CONTEXT_MODIFICATION_FAILURE(instance_t       instance,
                                              uint32_t         assoc_id,
                                              uint32_t         stream,
                                              F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_UE_CONTEXT_MODIFICATION_REQUIRED(instance_t       instance,
                                               uint32_t         assoc_id,
                                               uint32_t         stream,
                                               F1AP_F1AP_PDU_t *pdu) {
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_send_UE_CONTEXT_MODIFICATION_CONFIRM(instance_t instance,
                                            F1AP_UEContextModificationConfirm_t UEContextModificationConfirm_t) {
  AssertFatal(1==0,"Not implemented yet\n");
}
